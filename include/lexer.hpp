/**
 * lexer.hpp — Lexical Analyzer (Scanner)
 * ========================================
 * PASS 1 of the two-pass interpreter pipeline.
 *
 * Transforms raw Python source text into a stream of classified tokens.
 * Key challenges unique to Python lexing:
 *   - Indentation-sensitive: INDENT/DEDENT tokens generated from
 *     leading whitespace, using an indentation level stack.
 *   - Significant newlines: NEWLINE tokens delimit statements.
 *   - Blank/comment-only lines are ignored for indentation purposes.
 *
 * Token Classification (per project spec):
 *   ┌─────────────────┬───────────────────────────────────────┐
 *   │ Category        │ Examples                              │
 *   ├─────────────────┼───────────────────────────────────────┤
 *   │ Keywords        │ if, else, while, def, return, ...     │
 *   │ Identifiers     │ variable names, function names        │
 *   │ Literals        │ 42, 3.14, "hello", True, None         │
 *   │ Operators       │ +, -, *, ==, !=, <=, and, or, not     │
 *   │ Punctuation     │ (, ), [, ], :, ,                      │
 *   │ Structural      │ INDENT, DEDENT, NEWLINE, EOF          │
 *   └─────────────────┴───────────────────────────────────────┘
 */

#pragma once
#include "errorreport.hpp"
#include "token.hpp"
#include <stack>
#include <string>
#include <vector>

class Lexer {
public:
  /**
   * Construct a lexer for the given source code.
   * Errors are reported through the shared ErrorReporter.
   */
  Lexer(const std::string &source, ErrorReporter &errors);

  /**
   * Tokenize the entire source, returning the full token stream.
   * This is the main entry point — call once, get all tokens.
   */
  std::vector<Token> tokenize();

private:
  const std::string &source_;
  ErrorReporter &errors_;
  size_t pos_;
  int line_;
  int col_;
  std::stack<int> indentStack_; // tracks nesting depth via spaces

  /* ── Character-level helpers ───────────────────────────────── */
  char peek() const;
  char peekNext() const;
  char advance();

  /* ── Indentation handling ─────────────────────────────────── */
  bool isBlankOrCommentLine() const;
  void skipToNextLine();
  void skipToEndOfLine();
  int measureIndentation();
  void emitIndentationTokens(int indent, std::vector<Token> &tokens);

  /* ── Token scanners ───────────────────────────────────────── */
  Token scanString();
  Token scanNumber();
  Token scanIdentifierOrKeyword();
  Token scanOperatorOrPunctuation();
};
