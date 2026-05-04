#include "interpreter.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
struct RuntimeFailure : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct ReturnSignal {
  Value value;
};
struct BreakSignal {};
struct ContinueSignal {};
} // namespace

Value::ListPtr MemoryManager::makeList(Value::List elements) {
  auto list = std::make_shared<Value::List>(std::move(elements));
  heapLists_.push_back(list);
  totalAllocations_++;
  return list;
}

void MemoryManager::collectGarbage() {
  heapLists_.erase(
      std::remove_if(heapLists_.begin(), heapLists_.end(),
                     [](const std::weak_ptr<Value::List> &ptr) {
                       return ptr.expired();
                     }),
      heapLists_.end());
}

size_t MemoryManager::liveHeapObjects() const {
  size_t count = 0;
  for (const auto &ptr : heapLists_) {
    if (!ptr.expired())
      count++;
  }
  return count;
}

Interpreter::Interpreter(ErrorReporter &errors, MemoryManager &memory,
                         std::ostream &out)
    : errors_(errors), memory_(memory), out_(out) {
  scopes_.push_back({});
}

void Interpreter::execute(const ASTNode &root) {
  try {
    if (root.kind == NodeKind::BLOCK) {
      executeBlock(static_cast<const Block &>(root), false);
    } else {
      executeStatement(root);
    }
  } catch (const ReturnSignal &) {
    runtimeError(root.line, "return used outside function");
  } catch (const BreakSignal &) {
    runtimeError(root.line, "break used outside loop");
  } catch (const ContinueSignal &) {
    runtimeError(root.line, "continue used outside loop");
  } catch (const RuntimeFailure &) {
    // Already reported into ErrorReporter.
  }
  memory_.collectGarbage();
}

void Interpreter::executeBlock(const Block &block, bool createScope) {
  if (createScope)
    scopes_.push_back({});
  try {
    for (const auto &stmt : block.statements) {
      executeStatement(*stmt);
    }
  } catch (...) {
    if (createScope)
      scopes_.pop_back();
    throw;
  }
  if (createScope)
    scopes_.pop_back();
}

void Interpreter::executeStatement(const ASTNode &node) {
  switch (node.kind) {
  case NodeKind::EXPR_STMT: {
    auto &n = static_cast<const ExprStatement &>(node);
    (void)evaluate(*n.expr);
    return;
  }
  case NodeKind::ASSIGNMENT: {
    auto &n = static_cast<const Assignment &>(node);
    assign(n.name, evaluate(*n.value));
    return;
  }
  case NodeKind::AUG_ASSIGNMENT: {
    auto &n = static_cast<const AugAssignment &>(node);
    Value *slot = lookup(n.name);
    if (!slot) {
      runtimeError(node.line, "Undefined variable in augmented assignment: " +
                                n.name);
      return;
    }
    TokenType baseOp = TokenType::PLUS;
    if (n.op == TokenType::PLUS_ASSIGN)
      baseOp = TokenType::PLUS;
    else if (n.op == TokenType::MINUS_ASSIGN)
      baseOp = TokenType::MINUS;
    else if (n.op == TokenType::STAR_ASSIGN)
      baseOp = TokenType::STAR;
    else if (n.op == TokenType::SLASH_ASSIGN)
      baseOp = TokenType::SLASH;
    *slot = evalBinary(baseOp, *slot, evaluate(*n.value), node.line);
    return;
  }
  case NodeKind::LIST_ASSIGNMENT: {
    auto &n = static_cast<const ListAssignment &>(node);
    if (n.target->kind != NodeKind::INDEX_EXPR) {
      runtimeError(node.line, "Invalid list assignment target");
      return;
    }
    auto &idxExpr = static_cast<const IndexExpr &>(*n.target);
    Value obj = evaluate(*idxExpr.object);
    Value idx = evaluate(*idxExpr.index);
    int64_t i = asInt(idx, node.line);
    Value val = evaluate(*n.value);

    auto listPtr = std::get_if<Value::ListPtr>(&obj.data);
    if (!listPtr || !(*listPtr)) {
      runtimeError(node.line, "Target of indexed assignment is not a list");
      return;
    }
    if (i < 0 || static_cast<size_t>(i) >= (*listPtr)->size()) {
      runtimeError(node.line, "List index out of bounds");
      return;
    }
    (**listPtr)[static_cast<size_t>(i)] = val;
    return;
  }
  case NodeKind::PRINT_STMT: {
    auto &n = static_cast<const PrintStatement &>(node);
    for (size_t i = 0; i < n.args.size(); ++i) {
      if (i > 0)
        out_ << " ";
      out_ << toString(evaluate(*n.args[i]));
    }
    out_ << "\n";
    return;
  }
  case NodeKind::RETURN_STMT: {
    auto &n = static_cast<const ReturnStatement &>(node);
    if (n.value)
      throw ReturnSignal{evaluate(*n.value)};
    throw ReturnSignal{Value()};
  }
  case NodeKind::IF_STMT: {
    auto &n = static_cast<const IfStatement &>(node);
    for (const auto &branch : n.branches) {
      if (!branch.condition || isTruthy(evaluate(*branch.condition))) {
        executeBlock(static_cast<const Block &>(*branch.body), true);
        break;
      }
    }
    return;
  }
  case NodeKind::WHILE_STMT: {
    auto &n = static_cast<const WhileStatement &>(node);
    while (isTruthy(evaluate(*n.condition))) {
      try {
        executeBlock(static_cast<const Block &>(*n.body), true);
      } catch (const ContinueSignal &) {
        continue;
      } catch (const BreakSignal &) {
        break;
      }
    }
    return;
  }
  case NodeKind::FOR_STMT: {
    auto &n = static_cast<const ForStatement &>(node);
    Value iterable = evaluate(*n.iterable);
    auto listPtr = std::get_if<Value::ListPtr>(&iterable.data);
    if (!listPtr || !(*listPtr)) {
      runtimeError(node.line, "for-loop iterable must be a list/range result");
      return;
    }
    for (const auto &item : **listPtr) {
      scopes_.push_back({});
      define(n.variable, item);
      try {
        executeBlock(static_cast<const Block &>(*n.body), false);
      } catch (const ContinueSignal &) {
        scopes_.pop_back();
        continue;
      } catch (const BreakSignal &) {
        scopes_.pop_back();
        break;
      }
      scopes_.pop_back();
    }
    return;
  }
  case NodeKind::FUNCTION_DEF: {
    auto &n = static_cast<const FunctionDef &>(node);
    auto fn = std::make_shared<FunctionObject>();
    fn->name = n.name;
    fn->params = n.params;
    fn->body = n.body.get();
    define(n.name, Value(fn));
    return;
  }
  case NodeKind::BLOCK: {
    executeBlock(static_cast<const Block &>(node), true);
    return;
  }
  case NodeKind::BREAK_STMT:
    throw BreakSignal{};
  case NodeKind::CONTINUE_STMT:
    throw ContinueSignal{};
  case NodeKind::PASS_STMT:
    return;
  default:
    runtimeError(node.line, "Unsupported statement node kind");
    return;
  }
}

Value Interpreter::evaluate(const ASTNode &node) {
  switch (node.kind) {
  case NodeKind::INT_LITERAL:
    return Value(static_cast<const IntLiteral &>(node).value);
  case NodeKind::FLOAT_LITERAL:
    return Value(static_cast<const FloatLiteral &>(node).value);
  case NodeKind::STRING_LITERAL:
    return Value(static_cast<const StringLiteral &>(node).value);
  case NodeKind::BOOL_LITERAL:
    return Value(static_cast<const BoolLiteral &>(node).value);
  case NodeKind::NONE_LITERAL:
    return Value();
  case NodeKind::IDENTIFIER: {
    auto &n = static_cast<const Identifier &>(node);
    const Value *v = lookup(n.name);
    if (!v) {
      runtimeError(node.line, "Undefined variable: " + n.name);
      return Value();
    }
    return *v;
  }
  case NodeKind::UNARY_OP: {
    auto &n = static_cast<const UnaryOp &>(node);
    return evalUnary(n.op, evaluate(*n.operand), node.line);
  }
  case NodeKind::BINARY_OP: {
    auto &n = static_cast<const BinaryOp &>(node);
    return evalBinary(n.op, evaluate(*n.left), evaluate(*n.right), node.line);
  }
  case NodeKind::FUNCTION_CALL: {
    auto &n = static_cast<const FunctionCall &>(node);
    std::vector<Value> args;
    args.reserve(n.args.size());
    for (const auto &arg : n.args)
      args.push_back(evaluate(*arg));
    return callFunction(n.name, args, node.line);
  }
  case NodeKind::LIST_LITERAL: {
    auto &n = static_cast<const ListLiteral &>(node);
    Value::List values;
    values.reserve(n.elements.size());
    for (const auto &el : n.elements)
      values.push_back(evaluate(*el));
    return Value(memory_.makeList(std::move(values)));
  }
  case NodeKind::INDEX_EXPR: {
    auto &n = static_cast<const IndexExpr &>(node);
    Value obj = evaluate(*n.object);
    int64_t idx = asInt(evaluate(*n.index), node.line);

    if (auto listPtr = std::get_if<Value::ListPtr>(&obj.data)) {
      if (!(*listPtr)) {
        runtimeError(node.line, "Attempted to index null list");
        return Value();
      }
      if (idx < 0 || static_cast<size_t>(idx) >= (*listPtr)->size()) {
        runtimeError(node.line, "List index out of bounds");
        return Value();
      }
      return (**listPtr)[static_cast<size_t>(idx)];
    }

    if (auto str = std::get_if<std::string>(&obj.data)) {
      if (idx < 0 || static_cast<size_t>(idx) >= str->size()) {
        runtimeError(node.line, "String index out of bounds");
        return Value();
      }
      return Value(std::string(1, (*str)[static_cast<size_t>(idx)]));
    }

    runtimeError(node.line, "Indexed object is neither list nor string");
    return Value();
  }
  default:
    runtimeError(node.line, "Unsupported expression node kind");
    return Value();
  }
}

Value Interpreter::evalUnary(TokenType op, const Value &v, int line) {
  if (op == TokenType::KW_NOT)
    return Value(!isTruthy(v));

  if (op == TokenType::PLUS) {
    if (std::holds_alternative<int64_t>(v.data))
      return Value(std::get<int64_t>(v.data));
    if (std::holds_alternative<double>(v.data))
      return Value(std::get<double>(v.data));
    runtimeError(line, "Unary '+' expects numeric operand");
    return Value();
  }

  if (op == TokenType::MINUS) {
    if (std::holds_alternative<int64_t>(v.data))
      return Value(-std::get<int64_t>(v.data));
    if (std::holds_alternative<double>(v.data))
      return Value(-std::get<double>(v.data));
    runtimeError(line, "Unary '-' expects numeric operand");
    return Value();
  }

  runtimeError(line, "Unsupported unary operator");
  return Value();
}

Value Interpreter::evalBinary(TokenType op, const Value &left, const Value &right,
                              int line) {
  if (op == TokenType::KW_AND)
    return Value(isTruthy(left) && isTruthy(right));
  if (op == TokenType::KW_OR)
    return Value(isTruthy(left) || isTruthy(right));

  if (op == TokenType::PLUS) {
    if (std::holds_alternative<std::string>(left.data) &&
        std::holds_alternative<std::string>(right.data)) {
      return Value(std::get<std::string>(left.data) +
                   std::get<std::string>(right.data));
    }
    if (auto l = std::get_if<Value::ListPtr>(&left.data)) {
      if (auto r = std::get_if<Value::ListPtr>(&right.data)) {
        if (!(*l) || !(*r)) {
          runtimeError(line, "Cannot concatenate null lists");
          return Value();
        }
        Value::List out = **l;
        out.insert(out.end(), (**r).begin(), (**r).end());
        return Value(memory_.makeList(std::move(out)));
      }
    }
  }

  if (isNumeric(left) && isNumeric(right)) {
    bool useFloat =
        std::holds_alternative<double>(left.data) ||
        std::holds_alternative<double>(right.data) || op == TokenType::SLASH;
    double l = asDouble(left, line);
    double r = asDouble(right, line);

    switch (op) {
    case TokenType::PLUS:
      return useFloat ? Value(l + r)
                      : Value(asInt(left, line) + asInt(right, line));
    case TokenType::MINUS:
      return useFloat ? Value(l - r)
                      : Value(asInt(left, line) - asInt(right, line));
    case TokenType::STAR:
      return useFloat ? Value(l * r)
                      : Value(asInt(left, line) * asInt(right, line));
    case TokenType::SLASH:
      if (r == 0.0) {
        runtimeError(line, "Division by zero");
        return Value();
      }
      return Value(l / r);
    case TokenType::DOUBLE_SLASH:
      if (r == 0.0) {
        runtimeError(line, "Division by zero");
        return Value();
      }
      return Value(static_cast<int64_t>(std::floor(l / r)));
    case TokenType::PERCENT:
      if (r == 0.0) {
        runtimeError(line, "Modulo by zero");
        return Value();
      }
      return Value(asInt(left, line) % asInt(right, line));
    case TokenType::DOUBLE_STAR:
      return Value(std::pow(l, r));
    case TokenType::EQ:
      return Value(l == r);
    case TokenType::NEQ:
      return Value(l != r);
    case TokenType::LT:
      return Value(l < r);
    case TokenType::LTE:
      return Value(l <= r);
    case TokenType::GT:
      return Value(l > r);
    case TokenType::GTE:
      return Value(l >= r);
    default:
      break;
    }
  }

  if (op == TokenType::EQ || op == TokenType::NEQ) {
    bool eq = toString(left) == toString(right);
    return Value(op == TokenType::EQ ? eq : !eq);
  }

  if ((op == TokenType::LT || op == TokenType::LTE || op == TokenType::GT ||
       op == TokenType::GTE) &&
      std::holds_alternative<std::string>(left.data) &&
      std::holds_alternative<std::string>(right.data)) {
    const auto &l = std::get<std::string>(left.data);
    const auto &r = std::get<std::string>(right.data);
    switch (op) {
    case TokenType::LT: return Value(l < r);
    case TokenType::LTE: return Value(l <= r);
    case TokenType::GT: return Value(l > r);
    case TokenType::GTE: return Value(l >= r);
    default: break;
    }
  }

  runtimeError(line, "Invalid operands for binary operator");
  return Value();
}

Value Interpreter::callUserFunction(const FunctionObject &fn,
                                    const std::vector<Value> &args, int line) {
  if (fn.params.size() != args.size()) {
    std::ostringstream oss;
    oss << "Function '" << fn.name << "' expects " << fn.params.size()
        << " argument(s), got " << args.size();
    runtimeError(line, oss.str());
    return Value();
  }

  scopes_.push_back({});
  for (size_t i = 0; i < fn.params.size(); ++i)
    define(fn.params[i], args[i]);

  try {
    executeBlock(static_cast<const Block &>(*fn.body), false);
  } catch (const ReturnSignal &ret) {
    scopes_.pop_back();
    return ret.value;
  }

  scopes_.pop_back();
  return Value();
}

Value Interpreter::callFunction(const std::string &name,
                                const std::vector<Value> &args, int line) {
  if (name == "range") {
    if (args.empty() || args.size() > 3) {
      runtimeError(line, "range() expects 1 to 3 integer args");
      return Value();
    }
    int64_t start = 0;
    int64_t stop = 0;
    int64_t step = 1;
    if (args.size() == 1) {
      stop = asInt(args[0], line);
    } else if (args.size() == 2) {
      start = asInt(args[0], line);
      stop = asInt(args[1], line);
    } else {
      start = asInt(args[0], line);
      stop = asInt(args[1], line);
      step = asInt(args[2], line);
      if (step == 0) {
        runtimeError(line, "range() step cannot be zero");
        return Value();
      }
    }

    Value::List list;
    if (step > 0) {
      for (int64_t v = start; v < stop; v += step)
        list.emplace_back(v);
    } else {
      for (int64_t v = start; v > stop; v += step)
        list.emplace_back(v);
    }
    return Value(memory_.makeList(std::move(list)));
  }

  if (name == "len") {
    if (args.size() != 1) {
      runtimeError(line, "len() expects 1 argument");
      return Value();
    }
    if (auto s = std::get_if<std::string>(&args[0].data))
      return Value(static_cast<int64_t>(s->size()));
    if (auto l = std::get_if<Value::ListPtr>(&args[0].data)) {
      if (!(*l)) {
        runtimeError(line, "len() received null list");
        return Value();
      }
      return Value(static_cast<int64_t>((*l)->size()));
    }
    runtimeError(line, "len() expects a list or string");
    return Value();
  }

  if (name == "int") {
    if (args.size() != 1) {
      runtimeError(line, "int() expects 1 argument");
      return Value();
    }
    return Value(asInt(args[0], line));
  }

  if (name == "float") {
    if (args.size() != 1) {
      runtimeError(line, "float() expects 1 argument");
      return Value();
    }
    return Value(asDouble(args[0], line));
  }

  if (name == "str") {
    if (args.size() != 1) {
      runtimeError(line, "str() expects 1 argument");
      return Value();
    }
    return Value(toString(args[0]));
  }

  if (name == "bool") {
    if (args.size() != 1) {
      runtimeError(line, "bool() expects 1 argument");
      return Value();
    }
    return Value(isTruthy(args[0]));
  }

  if (name == "input") {
    std::string lineInput;
    std::getline(std::cin, lineInput);
    return Value(lineInput);
  }

  const Value *fnVal = lookup(name);
  if (!fnVal || !std::holds_alternative<Value::FuncPtr>(fnVal->data)) {
    runtimeError(line, "Undefined function: " + name);
    return Value();
  }

  auto fn = std::get<Value::FuncPtr>(fnVal->data);
  if (!fn || !fn->body) {
    runtimeError(line, "Function object is invalid: " + name);
    return Value();
  }
  return callUserFunction(*fn, args, line);
}

bool Interpreter::isNumeric(const Value &v) const {
  return std::holds_alternative<int64_t>(v.data) ||
         std::holds_alternative<double>(v.data);
}

double Interpreter::asDouble(const Value &v, int line) {
  if (std::holds_alternative<double>(v.data))
    return std::get<double>(v.data);
  if (std::holds_alternative<int64_t>(v.data))
    return static_cast<double>(std::get<int64_t>(v.data));
  if (std::holds_alternative<bool>(v.data))
    return std::get<bool>(v.data) ? 1.0 : 0.0;
  if (std::holds_alternative<std::string>(v.data)) {
    try {
      return std::stod(std::get<std::string>(v.data));
    } catch (...) {
      runtimeError(line, "Cannot convert string to float");
      return 0.0;
    }
  }
  runtimeError(line, "Expected numeric value");
  return 0.0;
}

int64_t Interpreter::asInt(const Value &v, int line) {
  if (std::holds_alternative<int64_t>(v.data))
    return std::get<int64_t>(v.data);
  if (std::holds_alternative<double>(v.data))
    return static_cast<int64_t>(std::get<double>(v.data));
  if (std::holds_alternative<bool>(v.data))
    return std::get<bool>(v.data) ? 1 : 0;
  if (std::holds_alternative<std::string>(v.data)) {
    try {
      return std::stoll(std::get<std::string>(v.data));
    } catch (...) {
      runtimeError(line, "Cannot convert string to int");
      return 0;
    }
  }
  runtimeError(line, "Expected integer-compatible value");
  return 0;
}

bool Interpreter::isTruthy(const Value &v) const {
  if (std::holds_alternative<std::monostate>(v.data))
    return false;
  if (std::holds_alternative<bool>(v.data))
    return std::get<bool>(v.data);
  if (std::holds_alternative<int64_t>(v.data))
    return std::get<int64_t>(v.data) != 0;
  if (std::holds_alternative<double>(v.data))
    return std::get<double>(v.data) != 0.0;
  if (std::holds_alternative<std::string>(v.data))
    return !std::get<std::string>(v.data).empty();
  if (auto l = std::get_if<Value::ListPtr>(&v.data))
    return (*l) && !(*l)->empty();
  if (std::holds_alternative<Value::FuncPtr>(v.data))
    return true;
  return false;
}

std::string Interpreter::toString(const Value &v) const {
  if (std::holds_alternative<std::monostate>(v.data))
    return "None";
  if (std::holds_alternative<int64_t>(v.data))
    return std::to_string(std::get<int64_t>(v.data));
  if (std::holds_alternative<double>(v.data)) {
    std::ostringstream oss;
    oss << std::get<double>(v.data);
    return oss.str();
  }
  if (std::holds_alternative<bool>(v.data))
    return std::get<bool>(v.data) ? "True" : "False";
  if (std::holds_alternative<std::string>(v.data))
    return std::get<std::string>(v.data);
  if (auto l = std::get_if<Value::ListPtr>(&v.data)) {
    if (!(*l))
      return "[]";
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < (*l)->size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << toString((**l)[i]);
    }
    oss << "]";
    return oss.str();
  }
  if (auto fn = std::get_if<Value::FuncPtr>(&v.data)) {
    if (*fn)
      return "<function " + (*fn)->name + ">";
  }
  return "<unknown>";
}

Value *Interpreter::lookup(const std::string &name) {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end())
      return &found->second;
  }
  return nullptr;
}

const Value *Interpreter::lookup(const std::string &name) const {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end())
      return &found->second;
  }
  return nullptr;
}

void Interpreter::assign(const std::string &name, const Value &value) {
  if (auto *slot = lookup(name)) {
    *slot = value;
    return;
  }
  define(name, value);
}

void Interpreter::define(const std::string &name, const Value &value) {
  scopes_.back()[name] = value;
}

void Interpreter::runtimeError(int line, const std::string &msg) {
  errors_.report(line, 1, ErrorPhase::RUNTIME, msg);
  throw RuntimeFailure(msg);
}
