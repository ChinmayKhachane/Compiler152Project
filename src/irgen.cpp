#include "irgen.hpp"
#include <sstream>
#include <stdexcept>

namespace {
std::string join(const std::vector<std::string> &items, const std::string &sep) {
  std::ostringstream out;
  for (size_t i = 0; i < items.size(); ++i) {
    if (i > 0)
      out << sep;
    out << items[i];
  }
  return out.str();
}
} // namespace

std::vector<std::string> IRGenerator::generate(const ASTNode &root) {
  lines_.clear();
  tempCounter_ = 0;
  labelCounter_ = 0;
  indentLevel_ = 0;

  emitNode(root);
  return lines_;
}

void IRGenerator::emit(const std::string &line) {
  lines_.push_back(indent() + line);
}

std::string IRGenerator::nextTemp() {
  return "%t" + std::to_string(++tempCounter_);
}

std::string IRGenerator::nextLabel(const std::string &prefix) {
  return prefix + "_" + std::to_string(++labelCounter_);
}

std::string IRGenerator::indent() const {
  return std::string(static_cast<size_t>(indentLevel_) * 2, ' ');
}

std::string IRGenerator::opToString(TokenType op) const {
  switch (op) {
  case TokenType::PLUS: return "+";
  case TokenType::MINUS: return "-";
  case TokenType::STAR: return "*";
  case TokenType::SLASH: return "/";
  case TokenType::DOUBLE_SLASH: return "//";
  case TokenType::PERCENT: return "%";
  case TokenType::DOUBLE_STAR: return "**";
  case TokenType::EQ: return "==";
  case TokenType::NEQ: return "!=";
  case TokenType::LT: return "<";
  case TokenType::LTE: return "<=";
  case TokenType::GT: return ">";
  case TokenType::GTE: return ">=";
  case TokenType::KW_AND: return "and";
  case TokenType::KW_OR: return "or";
  case TokenType::KW_NOT: return "not";
  case TokenType::PLUS_ASSIGN: return "+=";
  case TokenType::MINUS_ASSIGN: return "-=";
  case TokenType::STAR_ASSIGN: return "*=";
  case TokenType::SLASH_ASSIGN: return "/=";
  default:
    return tokenTypeName(op);
  }
}

void IRGenerator::emitBlock(const Block &block) {
  for (const auto &stmt : block.statements) {
    emitNode(*stmt);
  }
}

void IRGenerator::emitNode(const ASTNode &node) {
  switch (node.kind) {
  case NodeKind::BLOCK:
    emitBlock(static_cast<const Block &>(node));
    return;

  case NodeKind::EXPR_STMT: {
    const auto &n = static_cast<const ExprStatement &>(node);
    std::string value = emitExpr(*n.expr);
    emit("eval " + value);
    return;
  }

  case NodeKind::ASSIGNMENT: {
    const auto &n = static_cast<const Assignment &>(node);
    std::string rhs = emitExpr(*n.value);
    emit(n.name + " = " + rhs);
    return;
  }

  case NodeKind::AUG_ASSIGNMENT: {
    const auto &n = static_cast<const AugAssignment &>(node);
    std::string rhs = emitExpr(*n.value);
    std::string op = opToString(n.op);
    std::string baseOp = op;
    if (op.size() == 2 && op.back() == '=')
      baseOp = op.substr(0, 1);
    emit(n.name + " = " + n.name + " " + baseOp + " " + rhs);
    return;
  }

  case NodeKind::LIST_ASSIGNMENT: {
    const auto &n = static_cast<const ListAssignment &>(node);
    if (n.target->kind != NodeKind::INDEX_EXPR)
      throw std::runtime_error("IRGenerator: list assignment target is not index");
    const auto &idx = static_cast<const IndexExpr &>(*n.target);
    std::string object = emitExpr(*idx.object);
    std::string index = emitExpr(*idx.index);
    std::string rhs = emitExpr(*n.value);
    emit(object + "[" + index + "] = " + rhs);
    return;
  }

  case NodeKind::PRINT_STMT: {
    const auto &n = static_cast<const PrintStatement &>(node);
    std::vector<std::string> args;
    args.reserve(n.args.size());
    for (const auto &arg : n.args)
      args.push_back(emitExpr(*arg));
    emit("print " + join(args, ", "));
    return;
  }

  case NodeKind::RETURN_STMT: {
    const auto &n = static_cast<const ReturnStatement &>(node);
    if (n.value) {
      emit("return " + emitExpr(*n.value));
    } else {
      emit("return");
    }
    return;
  }

  case NodeKind::IF_STMT: {
    const auto &n = static_cast<const IfStatement &>(node);
    std::string endLabel = nextLabel("if_end");
    std::string nextLabelName;

    for (size_t i = 0; i < n.branches.size(); ++i) {
      const auto &branch = n.branches[i];
      bool isElse = (branch.condition == nullptr);

      if (!isElse) {
        nextLabelName = nextLabel("if_next");
        std::string cond = emitExpr(*branch.condition);
        emit("if_false " + cond + " goto " + nextLabelName);
      }

      const auto &body = static_cast<const Block &>(*branch.body);
      indentLevel_++;
      emitBlock(body);
      indentLevel_--;

      if (!isElse) {
        emit("goto " + endLabel);
        emit(nextLabelName + ":");
      }
    }

    emit(endLabel + ":");
    return;
  }

  case NodeKind::WHILE_STMT: {
    const auto &n = static_cast<const WhileStatement &>(node);
    std::string startLabel = nextLabel("while_start");
    std::string endLabel = nextLabel("while_end");

    emit(startLabel + ":");
    std::string cond = emitExpr(*n.condition);
    emit("if_false " + cond + " goto " + endLabel);
    indentLevel_++;
    emitBlock(static_cast<const Block &>(*n.body));
    indentLevel_--;
    emit("goto " + startLabel);
    emit(endLabel + ":");
    return;
  }

  case NodeKind::FOR_STMT: {
    const auto &n = static_cast<const ForStatement &>(node);
    std::string iter = emitExpr(*n.iterable);
    std::string idx = nextTemp();
    std::string startLabel = nextLabel("for_start");
    std::string endLabel = nextLabel("for_end");

    emit(idx + " = 0");
    emit(startLabel + ":");
    emit("if_false " + idx + " < len(" + iter + ") goto " + endLabel);
    emit(n.variable + " = " + iter + "[" + idx + "]");
    indentLevel_++;
    emitBlock(static_cast<const Block &>(*n.body));
    indentLevel_--;
    emit(idx + " = " + idx + " + 1");
    emit("goto " + startLabel);
    emit(endLabel + ":");
    return;
  }

  case NodeKind::FUNCTION_DEF: {
    const auto &n = static_cast<const FunctionDef &>(node);
    emit("func " + n.name + "(" + join(n.params, ", ") + "):");
    indentLevel_++;
    emitBlock(static_cast<const Block &>(*n.body));
    indentLevel_--;
    emit("endfunc " + n.name);
    return;
  }

  case NodeKind::BREAK_STMT:
    emit("break");
    return;

  case NodeKind::CONTINUE_STMT:
    emit("continue");
    return;

  case NodeKind::PASS_STMT:
    emit("pass");
    return;

  default:
    throw std::runtime_error("IRGenerator: unsupported statement node kind");
  }
}

std::string IRGenerator::emitExpr(const ASTNode &expr) {
  switch (expr.kind) {
  case NodeKind::INT_LITERAL:
    return std::to_string(static_cast<const IntLiteral &>(expr).value);
  case NodeKind::FLOAT_LITERAL: {
    std::ostringstream out;
    out << static_cast<const FloatLiteral &>(expr).value;
    return out.str();
  }
  case NodeKind::STRING_LITERAL:
    return "\"" + static_cast<const StringLiteral &>(expr).value + "\"";
  case NodeKind::BOOL_LITERAL:
    return static_cast<const BoolLiteral &>(expr).value ? "True" : "False";
  case NodeKind::NONE_LITERAL:
    return "None";
  case NodeKind::IDENTIFIER:
    return static_cast<const Identifier &>(expr).name;

  case NodeKind::UNARY_OP: {
    const auto &n = static_cast<const UnaryOp &>(expr);
    std::string operand = emitExpr(*n.operand);
    std::string temp = nextTemp();
    emit(temp + " = " + opToString(n.op) + " " + operand);
    return temp;
  }

  case NodeKind::BINARY_OP: {
    const auto &n = static_cast<const BinaryOp &>(expr);
    std::string left = emitExpr(*n.left);
    std::string right = emitExpr(*n.right);
    std::string temp = nextTemp();
    emit(temp + " = " + left + " " + opToString(n.op) + " " + right);
    return temp;
  }

  case NodeKind::FUNCTION_CALL: {
    const auto &n = static_cast<const FunctionCall &>(expr);
    std::vector<std::string> args;
    args.reserve(n.args.size());
    for (const auto &arg : n.args)
      args.push_back(emitExpr(*arg));
    std::string temp = nextTemp();
    emit(temp + " = call " + n.name + "(" + join(args, ", ") + ")");
    return temp;
  }

  case NodeKind::LIST_LITERAL: {
    const auto &n = static_cast<const ListLiteral &>(expr);
    std::vector<std::string> elems;
    elems.reserve(n.elements.size());
    for (const auto &el : n.elements)
      elems.push_back(emitExpr(*el));
    std::string temp = nextTemp();
    emit(temp + " = [" + join(elems, ", ") + "]");
    return temp;
  }

  case NodeKind::INDEX_EXPR: {
    const auto &n = static_cast<const IndexExpr &>(expr);
    std::string obj = emitExpr(*n.object);
    std::string idx = emitExpr(*n.index);
    std::string temp = nextTemp();
    emit(temp + " = " + obj + "[" + idx + "]");
    return temp;
  }

  default:
    throw std::runtime_error("IRGenerator: unsupported expression node kind");
  }
}
