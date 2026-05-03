/**
 * parser.hpp — Recursive Descent Parser with Error Recovery
 * ===========================================================
 * Consumes the token stream from the Lexer and builds an AST.
 *
 * Grammar (subset of Python, simplified):
 *   program     → statement* EOF
 *   statement   → simple_stmt NEWLINE | compound_stmt
 *   simple_stmt → assignment | aug_assign | print_stmt | return_stmt
 *                | break | continue | pass | expr_stmt
 *   compound    → if_stmt | while_stmt | for_stmt | func_def
 *
 * Expression precedence (lowest to highest):
 *   or → and → not → comparison → addition → multiplication → unary → power →
 * primary
 *
 * Error Recovery Strategy:
 *   On syntax error, synchronize by advancing to the next NEWLINE
 *   or DEDENT token, then resume parsing. This ensures ALL errors
 *   are reported in a single pass, as required by the project spec.
 */

#pragma once
#include "ast.hpp"
#include "errorreport.hpp"
#include "token.hpp"
#include <string>
#include <vector>

class Parser {
public:
  Parser(const std::vector<Token> &tokens, ErrorReporter &errors);

  /**
   * Parse the full token stream into a Block (program root).
   * Returns nullptr only on catastrophic failure.
   */
  ASTNodePtr parse();

private:
  const std::vector<Token> &tokens_;
  ErrorReporter &errors_;
  size_t pos_;

  /* ── Token stream navigation ─────────────────────────────── */
  const Token &current() const;
  const Token &previous() const;
  bool isAtEnd() const;
  const Token &advance();
  bool check(TokenType t) const;
  bool match(TokenType t);
  const Token &expect(TokenType t, const std::string &errMsg);
  void skipNewlines();

  /* ── Error handling ──────────────────────────────────────── */
  void reportError(const std::string &msg);
  void synchronize();

  /* ── Statement parsing ───────────────────────────────────── */
  ASTNodePtr parseStatement();
  ASTNodePtr parseSimpleStatement();
  ASTNodePtr parseAssignmentOrExpr();
  ASTNodePtr parsePrintStatement();
  ASTNodePtr parseReturnStatement();

  /* ── Compound statement parsing ──────────────────────────── */
  ASTNodePtr parseBlock();
  ASTNodePtr parseIfStatement();
  ASTNodePtr parseWhileStatement();
  ASTNodePtr parseForStatement();
  ASTNodePtr parseFunctionDef();

  /* ── Expression parsing (precedence climbing) ────────────── */
  ASTNodePtr parseExpression();
  ASTNodePtr parseOr();
  ASTNodePtr parseAnd();
  ASTNodePtr parseNot();
  ASTNodePtr parseComparison();
  ASTNodePtr parseAddition();
  ASTNodePtr parseMultiplication();
  ASTNodePtr parseUnary();
  ASTNodePtr parsePower();
  ASTNodePtr parsePostfix();
  ASTNodePtr parsePrimary();
  ASTNodePtr parseFunctionCallArgs(const std::string &name, int ln);
};
