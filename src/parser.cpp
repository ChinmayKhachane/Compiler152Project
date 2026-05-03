#include "parser.hpp"
#include <stdexcept>

Parser::Parser(const std::vector<Token> &tokens, ErrorReporter &errors)
    : tokens_(tokens), errors_(errors), pos_(0) {}

ASTNodePtr Parser::parse() {
  std::vector<ASTNodePtr> stmts;
  skipNewlines();

  while (!isAtEnd()) {
    try {
      auto stmt = parseStatement();
      if (stmt)
        stmts.push_back(std::move(stmt));
    } catch (const std::runtime_error &) {
      synchronize(); // error recovery: skip to next statement
    }
  }

  return std::make_unique<Block>(std::move(stmts), 1);
}

/* ═══════════════════════════════════════════════════════════
 *  TOKEN STREAM NAVIGATION
 * ═══════════════════════════════════════════════════════════ */

const Token &Parser::current() const {
  return (pos_ < tokens_.size()) ? tokens_[pos_] : tokens_.back();
}

const Token &Parser::previous() const {
  return tokens_[pos_ > 0 ? pos_ - 1 : 0];
}

bool Parser::isAtEnd() const {
  return current().type == TokenType::EOF_TOKEN;
}

const Token &Parser::advance() {
  if (!isAtEnd())
    pos_++;
  return previous();
}

bool Parser::check(TokenType t) const {
  return !isAtEnd() && current().type == t;
}

bool Parser::match(TokenType t) {
  if (check(t)) {
    advance();
    return true;
  }
  return false;
}

const Token &Parser::expect(TokenType t, const std::string &errMsg) {
  if (check(t))
    return advance();
  reportError(errMsg);
  throw std::runtime_error("parse error");
}

void Parser::skipNewlines() {
  while (check(TokenType::NEWLINE))
    advance();
}

/* ═══════════════════════════════════════════════════════════
 *  ERROR HANDLING
 * ═══════════════════════════════════════════════════════════ */

void Parser::reportError(const std::string &msg) {
  const Token &tok = current();
  errors_.report(tok.line, tok.col, ErrorPhase::PARSER,
                 msg + " (got '" + tok.lexeme + "')");
}

void Parser::synchronize() {
  int depth = 0;

  while (!isAtEnd()) {
    TokenType t = current().type;

    if (t == TokenType::INDENT) {
      depth++;
      advance();
      continue;
    }
    if (t == TokenType::DEDENT) {
      if (depth > 0) {
        depth--;
        advance();
        continue;
      }
      advance();
      skipNewlines();
      return;
    }
    if (t == TokenType::NEWLINE) {
      advance();
      skipNewlines();
      if (depth == 0)
        return;
      continue;
    }
    if (t == TokenType::ERROR_TOKEN) {
      advance();
      continue;
    }

    if (depth == 0) {
      switch (t) {
      case TokenType::KW_IF:
      case TokenType::KW_WHILE:
      case TokenType::KW_FOR:
      case TokenType::KW_DEF:
      case TokenType::KW_RETURN:
      case TokenType::KW_PRINT:
      case TokenType::KW_BREAK:
      case TokenType::KW_CONTINUE:
      case TokenType::KW_PASS:
        return;
      default:
        break;
      }
    }
    advance();
  }
}

/* ═══════════════════════════════════════════════════════════
 *  STATEMENT PARSING
 * ═══════════════════════════════════════════════════════════ */

ASTNodePtr Parser::parseStatement() {
  skipNewlines();
  if (isAtEnd())
    return nullptr;

  // Skip orphaned INDENT/DEDENT tokens left by error recovery
  while (check(TokenType::INDENT) || check(TokenType::DEDENT)) {
    advance();
    skipNewlines();
    if (isAtEnd())
      return nullptr;
  }

  // Skip error tokens from lexer
  while (check(TokenType::ERROR_TOKEN)) {
    advance();
    skipNewlines();
    if (isAtEnd())
      return nullptr;
  }

  // Compound statements (blocks with indentation)
  if (check(TokenType::KW_IF))    return parseIfStatement();
  if (check(TokenType::KW_WHILE)) return parseWhileStatement();
  if (check(TokenType::KW_FOR))   return parseForStatement();
  if (check(TokenType::KW_DEF))   return parseFunctionDef();

  // Simple statements
  return parseSimpleStatement();
}

ASTNodePtr Parser::parseSimpleStatement() {
  ASTNodePtr stmt;

  if (check(TokenType::KW_PRINT))
    stmt = parsePrintStatement();
  else if (check(TokenType::KW_RETURN))
    stmt = parseReturnStatement();
  else if (check(TokenType::KW_BREAK)) {
    int ln = current().line;
    advance();
    stmt = std::make_unique<BreakStatement>(ln);
  } else if (check(TokenType::KW_CONTINUE)) {
    int ln = current().line;
    advance();
    stmt = std::make_unique<ContinueStatement>(ln);
  } else if (check(TokenType::KW_PASS)) {
    int ln = current().line;
    advance();
    stmt = std::make_unique<PassStatement>(ln);
  } else
    stmt = parseAssignmentOrExpr();

  // Expect newline (or EOF/DEDENT) after simple statement
  if (!isAtEnd() && !check(TokenType::DEDENT) &&
      !check(TokenType::EOF_TOKEN)) {
    if (!match(TokenType::NEWLINE)) {
      reportError("Expected newline after statement");
      throw std::runtime_error("parse error");
    }
    skipNewlines();
  }

  return stmt;
}

ASTNodePtr Parser::parseAssignmentOrExpr() {
  ASTNodePtr expr = parseExpression();
  int ln = expr->line;

  // Simple assignment
  if (match(TokenType::ASSIGN)) {
    if (expr->kind == NodeKind::IDENTIFIER) {
      auto *id = static_cast<Identifier *>(expr.get());
      std::string name = id->name;
      ASTNodePtr value = parseExpression();
      return std::make_unique<Assignment>(name, std::move(value), ln);
    } else if (expr->kind == NodeKind::INDEX_EXPR) {
      ASTNodePtr value = parseExpression();
      return std::make_unique<ListAssignment>(std::move(expr),
                                              std::move(value), ln);
    } else {
      reportError("Invalid assignment target");
      throw std::runtime_error("parse error");
    }
  }

  // Augmented assignment (+=, -=, *=, /=)
  if (check(TokenType::PLUS_ASSIGN) || check(TokenType::MINUS_ASSIGN) ||
      check(TokenType::STAR_ASSIGN) || check(TokenType::SLASH_ASSIGN)) {
    TokenType op = current().type;
    advance();
    if (expr->kind == NodeKind::IDENTIFIER) {
      auto *id = static_cast<Identifier *>(expr.get());
      std::string name = id->name;
      ASTNodePtr value = parseExpression();
      return std::make_unique<AugAssignment>(name, op, std::move(value), ln);
    } else {
      reportError("Invalid augmented assignment target");
      throw std::runtime_error("parse error");
    }
  }

  return std::make_unique<ExprStatement>(std::move(expr), ln);
}

ASTNodePtr Parser::parsePrintStatement() {
  int ln = current().line;
  advance(); // consume 'print'
  expect(TokenType::LPAREN, "Expected '(' after 'print'");

  std::vector<ASTNodePtr> args;
  if (!check(TokenType::RPAREN)) {
    args.push_back(parseExpression());
    while (match(TokenType::COMMA)) {
      args.push_back(parseExpression());
    }
  }

  expect(TokenType::RPAREN, "Expected ')' after print arguments");
  return std::make_unique<PrintStatement>(std::move(args), ln);
}

ASTNodePtr Parser::parseReturnStatement() {
  int ln = current().line;
  advance(); // consume 'return'

  ASTNodePtr value = nullptr;
  if (!check(TokenType::NEWLINE) && !check(TokenType::EOF_TOKEN) &&
      !check(TokenType::DEDENT)) {
    value = parseExpression();
  }

  return std::make_unique<ReturnStatement>(std::move(value), ln);
}

/* ═══════════════════════════════════════════════════════════
 *  COMPOUND STATEMENT PARSING
 * ═══════════════════════════════════════════════════════════ */

ASTNodePtr Parser::parseBlock() {
  skipNewlines();
  expect(TokenType::INDENT, "Expected indented block");

  std::vector<ASTNodePtr> stmts;
  skipNewlines();

  while (!check(TokenType::DEDENT) && !isAtEnd()) {
    try {
      auto stmt = parseStatement();
      if (stmt)
        stmts.push_back(std::move(stmt));
    } catch (const std::runtime_error &) {
      synchronize();
    }
    skipNewlines();
  }

  if (!isAtEnd()) {
    expect(TokenType::DEDENT, "Expected dedent after block");
  }

  int ln = stmts.empty() ? 0 : stmts.front()->line;
  return std::make_unique<Block>(std::move(stmts), ln);
}

ASTNodePtr Parser::parseIfStatement() {
  int ln = current().line;
  std::vector<CondBranch> branches;

  advance(); // consume 'if'
  ASTNodePtr cond = parseExpression();
  expect(TokenType::COLON, "Expected ':' after if condition");
  ASTNodePtr body = parseBlock();
  branches.push_back({std::move(cond), std::move(body)});

  skipNewlines();
  while (check(TokenType::KW_ELIF)) {
    advance();
    ASTNodePtr elifCond = parseExpression();
    expect(TokenType::COLON, "Expected ':' after elif condition");
    ASTNodePtr elifBody = parseBlock();
    branches.push_back({std::move(elifCond), std::move(elifBody)});
    skipNewlines();
  }

  if (check(TokenType::KW_ELSE)) {
    advance();
    expect(TokenType::COLON, "Expected ':' after else");
    ASTNodePtr elseBody = parseBlock();
    branches.push_back({nullptr, std::move(elseBody)});
  }

  return std::make_unique<IfStatement>(std::move(branches), ln);
}

ASTNodePtr Parser::parseWhileStatement() {
  int ln = current().line;
  advance(); // consume 'while'
  ASTNodePtr cond = parseExpression();
  expect(TokenType::COLON, "Expected ':' after while condition");
  ASTNodePtr body = parseBlock();
  return std::make_unique<WhileStatement>(std::move(cond), std::move(body), ln);
}

ASTNodePtr Parser::parseForStatement() {
  int ln = current().line;
  advance(); // consume 'for'
  const Token &varTok =
      expect(TokenType::IDENTIFIER, "Expected loop variable after 'for'");
  std::string varName = varTok.lexeme;
  expect(TokenType::KW_IN, "Expected 'in' after loop variable");
  ASTNodePtr iterable = parseExpression();
  expect(TokenType::COLON, "Expected ':' after for iterable");
  ASTNodePtr body = parseBlock();
  return std::make_unique<ForStatement>(varName, std::move(iterable),
                                        std::move(body), ln);
}

ASTNodePtr Parser::parseFunctionDef() {
  int ln = current().line;
  advance(); // consume 'def'
  const Token &nameTok =
      expect(TokenType::IDENTIFIER, "Expected function name after 'def'");
  std::string name = nameTok.lexeme;
  expect(TokenType::LPAREN, "Expected '(' after function name");

  std::vector<std::string> params;
  if (!check(TokenType::RPAREN)) {
    const Token &p = expect(TokenType::IDENTIFIER, "Expected parameter name");
    params.push_back(p.lexeme);
    while (match(TokenType::COMMA)) {
      const Token &p2 =
          expect(TokenType::IDENTIFIER, "Expected parameter name");
      params.push_back(p2.lexeme);
    }
  }

  expect(TokenType::RPAREN, "Expected ')' after parameters");
  expect(TokenType::COLON, "Expected ':' after function signature");
  ASTNodePtr body = parseBlock();
  return std::make_unique<FunctionDef>(name, std::move(params), std::move(body),
                                       ln);
}

/* ═══════════════════════════════════════════════════════════
 *  EXPRESSION PARSING (Precedence Climbing)
 * ═══════════════════════════════════════════════════════════ */

ASTNodePtr Parser::parseExpression() { return parseOr(); }

ASTNodePtr Parser::parseOr() {
  auto left = parseAnd();
  while (check(TokenType::KW_OR)) {
    int ln = current().line;
    advance();
    auto right = parseAnd();
    left = std::make_unique<BinaryOp>(TokenType::KW_OR, std::move(left),
                                      std::move(right), ln);
  }
  return left;
}

ASTNodePtr Parser::parseAnd() {
  auto left = parseNot();
  while (check(TokenType::KW_AND)) {
    int ln = current().line;
    advance();
    auto right = parseNot();
    left = std::make_unique<BinaryOp>(TokenType::KW_AND, std::move(left),
                                      std::move(right), ln);
  }
  return left;
}

ASTNodePtr Parser::parseNot() {
  if (check(TokenType::KW_NOT)) {
    int ln = current().line;
    advance();
    auto operand = parseNot();
    return std::make_unique<UnaryOp>(TokenType::KW_NOT, std::move(operand), ln);
  }
  return parseComparison();
}

ASTNodePtr Parser::parseComparison() {
  auto left = parseAddition();
  while (check(TokenType::EQ) || check(TokenType::NEQ) ||
         check(TokenType::LT) || check(TokenType::GT) ||
         check(TokenType::LTE) || check(TokenType::GTE)) {
    int ln = current().line;
    TokenType op = current().type;
    advance();
    auto right = parseAddition();
    left =
        std::make_unique<BinaryOp>(op, std::move(left), std::move(right), ln);
  }
  return left;
}

ASTNodePtr Parser::parseAddition() {
  auto left = parseMultiplication();
  while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
    int ln = current().line;
    TokenType op = current().type;
    advance();
    auto right = parseMultiplication();
    left =
        std::make_unique<BinaryOp>(op, std::move(left), std::move(right), ln);
  }
  return left;
}

ASTNodePtr Parser::parseMultiplication() {
  auto left = parseUnary();
  while (check(TokenType::STAR) || check(TokenType::SLASH) ||
         check(TokenType::DOUBLE_SLASH) || check(TokenType::PERCENT)) {
    int ln = current().line;
    TokenType op = current().type;
    advance();
    auto right = parseUnary();
    left =
        std::make_unique<BinaryOp>(op, std::move(left), std::move(right), ln);
  }
  return left;
}

ASTNodePtr Parser::parseUnary() {
  if (check(TokenType::MINUS) || check(TokenType::PLUS)) {
    int ln = current().line;
    TokenType op = current().type;
    advance();
    auto operand = parseUnary();
    return std::make_unique<UnaryOp>(op, std::move(operand), ln);
  }
  return parsePower();
}

ASTNodePtr Parser::parsePower() {
  auto base = parsePostfix();
  if (check(TokenType::DOUBLE_STAR)) {
    int ln = current().line;
    advance();
    auto exp = parseUnary(); // right-associative
    return std::make_unique<BinaryOp>(TokenType::DOUBLE_STAR, std::move(base),
                                      std::move(exp), ln);
  }
  return base;
}

ASTNodePtr Parser::parsePostfix() {
  auto expr = parsePrimary();
  while (check(TokenType::LBRACKET)) {
    int ln = current().line;
    advance(); // consume '['
    auto index = parseExpression();
    expect(TokenType::RBRACKET, "Expected ']' after index");
    expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index), ln);
  }
  return expr;
}

ASTNodePtr Parser::parsePrimary() {
  int ln = current().line;

  if (check(TokenType::INTEGER_LIT)) {
    int64_t val = std::stoll(current().lexeme);
    advance();
    return std::make_unique<IntLiteral>(val, ln);
  }

  if (check(TokenType::FLOAT_LIT)) {
    double val = std::stod(current().lexeme);
    advance();
    return std::make_unique<FloatLiteral>(val, ln);
  }

  if (check(TokenType::STRING_LIT)) {
    std::string val = current().lexeme;
    advance();
    return std::make_unique<StringLiteral>(val, ln);
  }

  if (check(TokenType::KW_TRUE)) {
    advance();
    return std::make_unique<BoolLiteral>(true, ln);
  }
  if (check(TokenType::KW_FALSE)) {
    advance();
    return std::make_unique<BoolLiteral>(false, ln);
  }

  if (check(TokenType::KW_NONE)) {
    advance();
    return std::make_unique<NoneLiteral>(ln);
  }

  // Built-in function calls: range(), len(), input(), int(), float(), str(),
  // bool()
  if (check(TokenType::KW_RANGE) || check(TokenType::KW_LEN) ||
      check(TokenType::KW_INPUT) || check(TokenType::KW_INT) ||
      check(TokenType::KW_FLOAT) || check(TokenType::KW_STR) ||
      check(TokenType::KW_BOOL)) {
    std::string name = current().lexeme;
    advance();
    return parseFunctionCallArgs(name, ln);
  }

  // Identifier or user function call
  if (check(TokenType::IDENTIFIER)) {
    std::string name = current().lexeme;
    advance();

    if (check(TokenType::LPAREN)) {
      return parseFunctionCallArgs(name, ln);
    }

    return std::make_unique<Identifier>(name, ln);
  }

  // Parenthesized expression
  if (match(TokenType::LPAREN)) {
    auto expr = parseExpression();
    expect(TokenType::RPAREN, "Expected ')' after expression");
    return expr;
  }

  // List literal: [expr, expr, ...]
  if (match(TokenType::LBRACKET)) {
    std::vector<ASTNodePtr> elements;
    if (!check(TokenType::RBRACKET)) {
      elements.push_back(parseExpression());
      while (match(TokenType::COMMA)) {
        if (check(TokenType::RBRACKET))
          break; // trailing comma
        elements.push_back(parseExpression());
      }
    }
    expect(TokenType::RBRACKET, "Expected ']' after list elements");
    return std::make_unique<ListLiteral>(std::move(elements), ln);
  }

  reportError("Unexpected token in expression");
  throw std::runtime_error("parse error");
}

ASTNodePtr Parser::parseFunctionCallArgs(const std::string &name, int ln) {
  expect(TokenType::LPAREN, "Expected '(' for function call");

  std::vector<ASTNodePtr> args;
  if (!check(TokenType::RPAREN)) {
    args.push_back(parseExpression());
    while (match(TokenType::COMMA)) {
      args.push_back(parseExpression());
    }
  }

  expect(TokenType::RPAREN, "Expected ')' after function arguments");
  return std::make_unique<FunctionCall>(name, std::move(args), ln);
}
