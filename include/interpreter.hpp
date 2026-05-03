#pragma once

#include "ast.hpp"
#include "errorreport.hpp"
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct FunctionObject;

struct Value {
  using List = std::vector<Value>;
  using ListPtr = std::shared_ptr<List>;
  using FuncPtr = std::shared_ptr<FunctionObject>;
  using Data = std::variant<std::monostate, int64_t, double, bool, std::string,
                            ListPtr, FuncPtr>;

  Data data;

  Value() : data(std::monostate{}) {}
  explicit Value(int64_t v) : data(v) {}
  explicit Value(double v) : data(v) {}
  explicit Value(bool v) : data(v) {}
  explicit Value(std::string v) : data(std::move(v)) {}
  explicit Value(const char *v) : data(std::string(v)) {}
  explicit Value(ListPtr v) : data(std::move(v)) {}
  explicit Value(FuncPtr v) : data(std::move(v)) {}
};

struct FunctionObject {
  std::string name;
  std::vector<std::string> params;
  const ASTNode *body;
};

class MemoryManager {
public:
  Value::ListPtr makeList(Value::List elements);
  void collectGarbage();

  size_t liveHeapObjects() const;
  size_t totalHeapAllocations() const { return totalAllocations_; }
  size_t estimatedBytesAllocated() const { return estimatedBytesAllocated_; }

private:
  std::vector<std::weak_ptr<Value::List>> heapLists_;
  size_t totalAllocations_ = 0;
  size_t estimatedBytesAllocated_ = 0;
};

class Interpreter {
public:
  Interpreter(ErrorReporter &errors, MemoryManager &memory, std::ostream &out);

  void execute(const ASTNode &root);

private:
  ErrorReporter &errors_;
  MemoryManager &memory_;
  std::ostream &out_;
  std::vector<std::unordered_map<std::string, Value>> scopes_;

  void executeStatement(const ASTNode &node);
  void executeBlock(const Block &block, bool createScope);
  Value evaluate(const ASTNode &node);
  Value callFunction(const std::string &name, const std::vector<Value> &args,
                     int line);
  Value callUserFunction(const FunctionObject &fn, const std::vector<Value> &args,
                         int line);
  Value evalBinary(TokenType op, const Value &left, const Value &right, int line);
  Value evalUnary(TokenType op, const Value &v, int line);

  bool isTruthy(const Value &v) const;
  std::string toString(const Value &v) const;
  bool isNumeric(const Value &v) const;
  double asDouble(const Value &v, int line);
  int64_t asInt(const Value &v, int line);

  Value *lookup(const std::string &name);
  const Value *lookup(const std::string &name) const;
  void assign(const std::string &name, const Value &value);
  void define(const std::string &name, const Value &value);
  void runtimeError(int line, const std::string &msg);
};
