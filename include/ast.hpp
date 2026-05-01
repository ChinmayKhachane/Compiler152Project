/**
 * ast.hpp — Abstract Syntax Tree Node Definitions
 * ==================================================
 * The AST is the bridge between Pass 1 (parsing) and Pass 2 (interpretation).
 * Each node type represents a distinct Python construct.
 *
 * Design choices:
 *   - Uses std::unique_ptr for child ownership (no manual memory management)
 *   - Visitor-less design: each node carries a `kind` enum for dispatch
 *   - Line numbers propagated for runtime error reporting
 *
 * Node Hierarchy:
 *   ASTNode (base)
 *   ├── Expressions
 *   │   ├── IntLiteral, FloatLiteral, StringLiteral, BoolLiteral, NoneLiteral
 *   │   ├── Identifier
 *   │   ├── UnaryOp, BinaryOp
 *   │   ├── FunctionCall
 *   │   └── ListLiteral, IndexExpr
 *   └── Statements
 *       ├── ExprStatement, Assignment, AugAssignment
 *       ├── PrintStatement, ReturnStatement
 *       ├── IfStatement, WhileStatement, ForStatement
 *       ├── FunctionDef, Block
 *       ├── BreakStatement, ContinueStatement, PassStatement
 *       └── ListAssignment
 */

#pragma once
#include "token.hpp"
#include <memory>
#include <string>
#include <vector>

/* ── Forward-declare the base so unique_ptr can reference it ──── */
struct ASTNode;
using ASTNodePtr = std::unique_ptr<ASTNode>;

/* ── Node kind enum — used for safe downcasting ───────────────── */
enum class NodeKind {
  // Expressions
  INT_LITERAL,
  FLOAT_LITERAL,
  STRING_LITERAL,
  BOOL_LITERAL,
  NONE_LITERAL,
  IDENTIFIER,
  UNARY_OP,
  BINARY_OP,
  FUNCTION_CALL,
  LIST_LITERAL,
  INDEX_EXPR,

  // Statements
  EXPR_STMT,
  ASSIGNMENT,
  AUG_ASSIGNMENT,
  PRINT_STMT,
  RETURN_STMT,
  IF_STMT,
  WHILE_STMT,
  FOR_STMT,
  FUNCTION_DEF,
  BLOCK,
  BREAK_STMT,
  CONTINUE_STMT,
  PASS_STMT,
  LIST_ASSIGNMENT
};

/* ── Base AST Node ────────────────────────────────────────────── */
struct ASTNode {
  NodeKind kind;
  int line; // source line for error reporting

  ASTNode(NodeKind k, int ln) : kind(k), line(ln) {}
  virtual ~ASTNode() = default;
};

/* ═══════════════════════════════════════════════════════════════
 *  EXPRESSION NODES
 * ═══════════════════════════════════════════════════════════════ */

struct IntLiteral : ASTNode {
  int64_t value;
  IntLiteral(int64_t v, int ln)
      : ASTNode(NodeKind::INT_LITERAL, ln), value(v) {}
};

struct FloatLiteral : ASTNode {
  double value;
  FloatLiteral(double v, int ln)
      : ASTNode(NodeKind::FLOAT_LITERAL, ln), value(v) {}
};

struct StringLiteral : ASTNode {
  std::string value;
  StringLiteral(const std::string &v, int ln)
      : ASTNode(NodeKind::STRING_LITERAL, ln), value(v) {}
};

struct BoolLiteral : ASTNode {
  bool value;
  BoolLiteral(bool v, int ln) : ASTNode(NodeKind::BOOL_LITERAL, ln), value(v) {}
};

struct NoneLiteral : ASTNode {
  NoneLiteral(int ln) : ASTNode(NodeKind::NONE_LITERAL, ln) {}
};

struct Identifier : ASTNode {
  std::string name;
  Identifier(const std::string &n, int ln)
      : ASTNode(NodeKind::IDENTIFIER, ln), name(n) {}
};

struct UnaryOp : ASTNode {
  TokenType op; // MINUS, PLUS, or KW_NOT
  ASTNodePtr operand;
  UnaryOp(TokenType o, ASTNodePtr opnd, int ln)
      : ASTNode(NodeKind::UNARY_OP, ln), op(o), operand(std::move(opnd)) {}
};

struct BinaryOp : ASTNode {
  TokenType op; // any arithmetic, relational, or logical operator
  ASTNodePtr left;
  ASTNodePtr right;
  BinaryOp(TokenType o, ASTNodePtr l, ASTNodePtr r, int ln)
      : ASTNode(NodeKind::BINARY_OP, ln), op(o), left(std::move(l)),
        right(std::move(r)) {}
};

struct FunctionCall : ASTNode {
  std::string name;
  std::vector<ASTNodePtr> args;
  FunctionCall(const std::string &n, std::vector<ASTNodePtr> a, int ln)
      : ASTNode(NodeKind::FUNCTION_CALL, ln), name(n), args(std::move(a)) {}
};

struct ListLiteral : ASTNode {
  std::vector<ASTNodePtr> elements;
  ListLiteral(std::vector<ASTNodePtr> elems, int ln)
      : ASTNode(NodeKind::LIST_LITERAL, ln), elements(std::move(elems)) {}
};

struct IndexExpr : ASTNode {
  ASTNodePtr object;
  ASTNodePtr index;
  IndexExpr(ASTNodePtr obj, ASTNodePtr idx, int ln)
      : ASTNode(NodeKind::INDEX_EXPR, ln), object(std::move(obj)),
        index(std::move(idx)) {}
};

/* ═══════════════════════════════════════════════════════════════
 *  STATEMENT NODES
 * ═══════════════════════════════════════════════════════════════ */

struct Block : ASTNode {
  std::vector<ASTNodePtr> statements;
  Block(std::vector<ASTNodePtr> stmts, int ln)
      : ASTNode(NodeKind::BLOCK, ln), statements(std::move(stmts)) {}
};

struct ExprStatement : ASTNode {
  ASTNodePtr expr;
  ExprStatement(ASTNodePtr e, int ln)
      : ASTNode(NodeKind::EXPR_STMT, ln), expr(std::move(e)) {}
};

struct Assignment : ASTNode {
  std::string name;
  ASTNodePtr value;
  Assignment(const std::string &n, ASTNodePtr v, int ln)
      : ASTNode(NodeKind::ASSIGNMENT, ln), name(n), value(std::move(v)) {}
};

struct AugAssignment : ASTNode {
  std::string name;
  TokenType op; // PLUS_ASSIGN, MINUS_ASSIGN, etc.
  ASTNodePtr value;
  AugAssignment(const std::string &n, TokenType o, ASTNodePtr v, int ln)
      : ASTNode(NodeKind::AUG_ASSIGNMENT, ln), name(n), op(o),
        value(std::move(v)) {}
};

struct ListAssignment : ASTNode {
  ASTNodePtr target; // an IndexExpr
  ASTNodePtr value;
  ListAssignment(ASTNodePtr t, ASTNodePtr v, int ln)
      : ASTNode(NodeKind::LIST_ASSIGNMENT, ln), target(std::move(t)),
        value(std::move(v)) {}
};

struct PrintStatement : ASTNode {
  std::vector<ASTNodePtr> args;
  PrintStatement(std::vector<ASTNodePtr> a, int ln)
      : ASTNode(NodeKind::PRINT_STMT, ln), args(std::move(a)) {}
};

struct ReturnStatement : ASTNode {
  ASTNodePtr value; // may be nullptr for bare 'return'
  ReturnStatement(ASTNodePtr v, int ln)
      : ASTNode(NodeKind::RETURN_STMT, ln), value(std::move(v)) {}
};

struct BreakStatement : ASTNode {
  BreakStatement(int ln) : ASTNode(NodeKind::BREAK_STMT, ln) {}
};

struct ContinueStatement : ASTNode {
  ContinueStatement(int ln) : ASTNode(NodeKind::CONTINUE_STMT, ln) {}
};

struct PassStatement : ASTNode {
  PassStatement(int ln) : ASTNode(NodeKind::PASS_STMT, ln) {}
};

/* ── Conditional branches for if/elif/else ────────────────────── */
struct CondBranch {
  ASTNodePtr condition; // nullptr for 'else'
  ASTNodePtr body;      // Block node
};

struct IfStatement : ASTNode {
  std::vector<CondBranch> branches; // if, then elif*, then optional else
  IfStatement(std::vector<CondBranch> br, int ln)
      : ASTNode(NodeKind::IF_STMT, ln), branches(std::move(br)) {}
};

struct WhileStatement : ASTNode {
  ASTNodePtr condition;
  ASTNodePtr body;
  WhileStatement(ASTNodePtr cond, ASTNodePtr b, int ln)
      : ASTNode(NodeKind::WHILE_STMT, ln), condition(std::move(cond)),
        body(std::move(b)) {}
};

struct ForStatement : ASTNode {
  std::string variable; // loop variable name
  ASTNodePtr iterable;  // typically a range() call or list
  ASTNodePtr body;
  ForStatement(const std::string &var, ASTNodePtr iter, ASTNodePtr b, int ln)
      : ASTNode(NodeKind::FOR_STMT, ln), variable(var),
        iterable(std::move(iter)), body(std::move(b)) {}
};

struct FunctionDef : ASTNode {
  std::string name;
  std::vector<std::string> params;
  ASTNodePtr body;
  FunctionDef(const std::string &n, std::vector<std::string> p, ASTNodePtr b,
              int ln)
      : ASTNode(NodeKind::FUNCTION_DEF, ln), name(n), params(std::move(p)),
        body(std::move(b)) {}
};
