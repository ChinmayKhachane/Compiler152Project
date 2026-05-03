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

/* ── Free functions implemented in token.cpp ──────────────────── */
const std::unordered_map<std::string, TokenType> &keywordMap();
std::string tokenTypeName(TokenType t);
TokenCategory classifyToken(TokenType t);
std::string categoryName(TokenCategory c);
