#pragma once

#include "ast.hpp"
#include "token.hpp"
#include <string>
#include <vector>

class IRGenerator {
public:
  std::vector<std::string> generate(const ASTNode &root);

private:
  std::vector<std::string> lines_;
  int tempCounter_ = 0;
  int labelCounter_ = 0;
  int indentLevel_ = 0;

  void emit(const std::string &line);
  void emitNode(const ASTNode &node);
  void emitBlock(const Block &block);
  std::string emitExpr(const ASTNode &expr);

  std::string nextTemp();
  std::string nextLabel(const std::string &prefix);
  std::string opToString(TokenType op) const;
  std::string indent() const;
};
