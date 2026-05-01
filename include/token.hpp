#pragma once
#include <string>
#include <unordered_map>

enum class TokenType {
  INTEGER_LIT, // 42, 0, 1000
  FLOAT_LIT,   // 3.14, 0.5
  STRING_LIT,  // "hello", 'world'

  KW_AND,
  KW_OR,
  KW_NOT,
  KW_IF,
  KW_ELIF,
  KW_ELSE,
  KW_WHILE,
  KW_FOR,
  KW_IN,
  KW_DEF,
  KW_RETURN,
  KW_BREAK,
  KW_CONTINUE,
  KW_PASS,
  KW_TRUE,
  KW_FALSE,
  KW_NONE,
  KW_PRINT,
  KW_RANGE,
  KW_INPUT,
  KW_LEN,
  KW_INT,
  KW_FLOAT,
  KW_STR,
  KW_BOOL,

  // ─── Identifier ─────────────────────────────────────────
  IDENTIFIER,

  // ─── Arithmetic Operators ────────────────────────────────
  PLUS,         // +
  MINUS,        // -
  STAR,         // *
  SLASH,        // /
  DOUBLE_SLASH, // //  (floor division)
  PERCENT,      // %   (modulo)
  DOUBLE_STAR,  // **  (exponentiation)

  // ─── Assignment Operators ────────────────────────────────
  ASSIGN,       // =
  PLUS_ASSIGN,  // +=
  MINUS_ASSIGN, // -=
  STAR_ASSIGN,  // *=
  SLASH_ASSIGN, // /=

  // ─── Relational / Equality Operators ─────────────────────
  EQ,  // ==
  NEQ, // !=
  LT,  // <
  GT,  // >
  LTE, // <=
  GTE, // >=

  // ─── Punctuation / Delimiters ────────────────────────────
  LPAREN,   // (
  RPAREN,   // )
  LBRACKET, // [
  RBRACKET, // ]
  COLON,    // :
  COMMA,    // ,

  // ─── Structural (Python indentation model) ───────────────
  NEWLINE,   // end of logical line
  INDENT,    // increased indentation level
  DEDENT,    // decreased indentation level
  EOF_TOKEN, // end of source

  // ─── Error Recovery ──────────────────────────────────────
  ERROR_TOKEN // unrecognized character / malformed token
};

/* ── Token Category (for classification display) ──────────────── */
enum class TokenCategory {
  KEYWORD,
  IDENTIFIER,
  LITERAL,
  ARITHMETIC_OP,
  RELATIONAL_OP,
  LOGICAL_OP,
  ASSIGNMENT_OP,
  PUNCTUATION,
  STRUCTURAL,
  ERROR
};

/* ── The Token structure itself ───────────────────────────────── */
struct Token {
  TokenType type;
  std::string lexeme; // the raw text that was matched
  int line;           // 1-indexed source line number
  int col;            // 1-indexed column number

  Token() : type(TokenType::EOF_TOKEN), lexeme(""), line(0), col(0) {}

  Token(TokenType t, const std::string &lex, int ln, int c)
      : type(t), lexeme(lex), line(ln), col(c) {}
};

/* ── Keyword lookup table ─────────────────────────────────────── */
inline const std::unordered_map<std::string, TokenType> &keywordMap() {
  static const std::unordered_map<std::string, TokenType> kw = {
      {"and", TokenType::KW_AND},
      {"or", TokenType::KW_OR},
      {"not", TokenType::KW_NOT},
      {"if", TokenType::KW_IF},
      {"elif", TokenType::KW_ELIF},
      {"else", TokenType::KW_ELSE},
      {"while", TokenType::KW_WHILE},
      {"for", TokenType::KW_FOR},
      {"in", TokenType::KW_IN},
      {"def", TokenType::KW_DEF},
      {"return", TokenType::KW_RETURN},
      {"break", TokenType::KW_BREAK},
      {"continue", TokenType::KW_CONTINUE},
      {"pass", TokenType::KW_PASS},
      {"True", TokenType::KW_TRUE},
      {"False", TokenType::KW_FALSE},
      {"None", TokenType::KW_NONE},
      {"print", TokenType::KW_PRINT},
      {"range", TokenType::KW_RANGE},
      {"input", TokenType::KW_INPUT},
      {"len", TokenType::KW_LEN},
      {"int", TokenType::KW_INT},
      {"float", TokenType::KW_FLOAT},
      {"str", TokenType::KW_STR},
      {"bool", TokenType::KW_BOOL},
  };
  return kw;
}

/* ── Human-readable token type names ──────────────────────────── */
inline std::string tokenTypeName(TokenType t) {
  switch (t) {
  case TokenType::INTEGER_LIT:
    return "INTEGER_LIT";
  case TokenType::FLOAT_LIT:
    return "FLOAT_LIT";
  case TokenType::STRING_LIT:
    return "STRING_LIT";
  case TokenType::KW_AND:
    return "KW_AND";
  case TokenType::KW_OR:
    return "KW_OR";
  case TokenType::KW_NOT:
    return "KW_NOT";
  case TokenType::KW_IF:
    return "KW_IF";
  case TokenType::KW_ELIF:
    return "KW_ELIF";
  case TokenType::KW_ELSE:
    return "KW_ELSE";
  case TokenType::KW_WHILE:
    return "KW_WHILE";
  case TokenType::KW_FOR:
    return "KW_FOR";
  case TokenType::KW_IN:
    return "KW_IN";
  case TokenType::KW_DEF:
    return "KW_DEF";
  case TokenType::KW_RETURN:
    return "KW_RETURN";
  case TokenType::KW_BREAK:
    return "KW_BREAK";
  case TokenType::KW_CONTINUE:
    return "KW_CONTINUE";
  case TokenType::KW_PASS:
    return "KW_PASS";
  case TokenType::KW_TRUE:
    return "KW_TRUE";
  case TokenType::KW_FALSE:
    return "KW_FALSE";
  case TokenType::KW_NONE:
    return "KW_NONE";
  case TokenType::KW_PRINT:
    return "KW_PRINT";
  case TokenType::KW_RANGE:
    return "KW_RANGE";
  case TokenType::KW_INPUT:
    return "KW_INPUT";
  case TokenType::KW_LEN:
    return "KW_LEN";
  case TokenType::KW_INT:
    return "KW_INT";
  case TokenType::KW_FLOAT:
    return "KW_FLOAT";
  case TokenType::KW_STR:
    return "KW_STR";
  case TokenType::KW_BOOL:
    return "KW_BOOL";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::PLUS:
    return "PLUS";
  case TokenType::MINUS:
    return "MINUS";
  case TokenType::STAR:
    return "STAR";
  case TokenType::SLASH:
    return "SLASH";
  case TokenType::DOUBLE_SLASH:
    return "DOUBLE_SLASH";
  case TokenType::PERCENT:
    return "PERCENT";
  case TokenType::DOUBLE_STAR:
    return "DOUBLE_STAR";
  case TokenType::ASSIGN:
    return "ASSIGN";
  case TokenType::PLUS_ASSIGN:
    return "PLUS_ASSIGN";
  case TokenType::MINUS_ASSIGN:
    return "MINUS_ASSIGN";
  case TokenType::STAR_ASSIGN:
    return "STAR_ASSIGN";
  case TokenType::SLASH_ASSIGN:
    return "SLASH_ASSIGN";
  case TokenType::EQ:
    return "EQ";
  case TokenType::NEQ:
    return "NEQ";
  case TokenType::LT:
    return "LT";
  case TokenType::GT:
    return "GT";
  case TokenType::LTE:
    return "LTE";
  case TokenType::GTE:
    return "GTE";
  case TokenType::LPAREN:
    return "LPAREN";
  case TokenType::RPAREN:
    return "RPAREN";
  case TokenType::LBRACKET:
    return "LBRACKET";
  case TokenType::RBRACKET:
    return "RBRACKET";
  case TokenType::COLON:
    return "COLON";
  case TokenType::COMMA:
    return "COMMA";
  case TokenType::NEWLINE:
    return "NEWLINE";
  case TokenType::INDENT:
    return "INDENT";
  case TokenType::DEDENT:
    return "DEDENT";
  case TokenType::EOF_TOKEN:
    return "EOF";
  case TokenType::ERROR_TOKEN:
    return "ERROR";
  }
  return "UNKNOWN";
}

/* ── Token category classifier ────────────────────────────────── */
inline TokenCategory classifyToken(TokenType t) {
  switch (t) {
  // Keywords
  case TokenType::KW_AND:
  case TokenType::KW_OR:
  case TokenType::KW_NOT:
  case TokenType::KW_IF:
  case TokenType::KW_ELIF:
  case TokenType::KW_ELSE:
  case TokenType::KW_WHILE:
  case TokenType::KW_FOR:
  case TokenType::KW_IN:
  case TokenType::KW_DEF:
  case TokenType::KW_RETURN:
  case TokenType::KW_BREAK:
  case TokenType::KW_CONTINUE:
  case TokenType::KW_PASS:
  case TokenType::KW_TRUE:
  case TokenType::KW_FALSE:
  case TokenType::KW_NONE:
  case TokenType::KW_PRINT:
  case TokenType::KW_RANGE:
  case TokenType::KW_INPUT:
  case TokenType::KW_LEN:
  case TokenType::KW_INT:
  case TokenType::KW_FLOAT:
  case TokenType::KW_STR:
  case TokenType::KW_BOOL:
    return TokenCategory::KEYWORD;

  case TokenType::IDENTIFIER:
    return TokenCategory::IDENTIFIER;

  // Literals
  case TokenType::INTEGER_LIT:
  case TokenType::FLOAT_LIT:
  case TokenType::STRING_LIT:
    return TokenCategory::LITERAL;

  // Arithmetic operators
  case TokenType::PLUS:
  case TokenType::MINUS:
  case TokenType::STAR:
  case TokenType::SLASH:
  case TokenType::DOUBLE_SLASH:
  case TokenType::PERCENT:
  case TokenType::DOUBLE_STAR:
    return TokenCategory::ARITHMETIC_OP;

  // Relational operators
  case TokenType::EQ:
  case TokenType::NEQ:
  case TokenType::LT:
  case TokenType::GT:
  case TokenType::LTE:
  case TokenType::GTE:
    return TokenCategory::RELATIONAL_OP;

  // Logical operators (keyword-based, but classified separately)
  // (handled via keywords above; this exists for completeness)

  // Assignment operators
  case TokenType::ASSIGN:
  case TokenType::PLUS_ASSIGN:
  case TokenType::MINUS_ASSIGN:
  case TokenType::STAR_ASSIGN:
  case TokenType::SLASH_ASSIGN:
    return TokenCategory::ASSIGNMENT_OP;

  // Punctuation
  case TokenType::LPAREN:
  case TokenType::RPAREN:
  case TokenType::LBRACKET:
  case TokenType::RBRACKET:
  case TokenType::COLON:
  case TokenType::COMMA:
    return TokenCategory::PUNCTUATION;

  // Structural
  case TokenType::NEWLINE:
  case TokenType::INDENT:
  case TokenType::DEDENT:
  case TokenType::EOF_TOKEN:
    return TokenCategory::STRUCTURAL;

  case TokenType::ERROR_TOKEN:
    return TokenCategory::ERROR;
  }
  return TokenCategory::ERROR;
}

/* ── Category name for display ────────────────────────────────── */
inline std::string categoryName(TokenCategory c) {
  switch (c) {
  case TokenCategory::KEYWORD:
    return "Keyword";
  case TokenCategory::IDENTIFIER:
    return "Identifier";
  case TokenCategory::LITERAL:
    return "Literal";
  case TokenCategory::ARITHMETIC_OP:
    return "Arithmetic Op";
  case TokenCategory::RELATIONAL_OP:
    return "Relational Op";
  case TokenCategory::LOGICAL_OP:
    return "Logical Op";
  case TokenCategory::ASSIGNMENT_OP:
    return "Assignment Op";
  case TokenCategory::PUNCTUATION:
    return "Punctuation";
  case TokenCategory::STRUCTURAL:
    return "Structural";
  case TokenCategory::ERROR:
    return "Error";
  }
  return "Unknown";
}
