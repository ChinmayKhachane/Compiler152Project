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
#include <cctype>
#include <stack>
#include <string>
#include <vector>

class Lexer {
public:
  /**
   * Construct a lexer for the given source code.
   * Errors are reported through the shared ErrorReporter.
   */
  Lexer(const std::string &source, ErrorReporter &errors)
      : source_(source), errors_(errors), pos_(0), line_(1), col_(1) {
    indentStack_.push(0); // base indentation level is 0
  }

  /**
   * Tokenize the entire source, returning the full token stream.
   * This is the main entry point — call once, get all tokens.
   */
  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    bool atLineStart = true;

    while (pos_ < source_.size()) {
      /* ── Handle beginning-of-line indentation ─────────── */
      if (atLineStart) {
        atLineStart = false;

        // Skip fully blank or comment-only lines
        if (isBlankOrCommentLine()) {
          skipToNextLine();
          atLineStart = true;
          continue;
        }

        // Measure indentation and emit INDENT/DEDENT tokens
        int indent = measureIndentation();
        emitIndentationTokens(indent, tokens);
      }

      char c = peek();

      /* ── Skip inline whitespace (spaces/tabs mid-line) ── */
      if (c == ' ' || c == '\t') {
        advance();
        continue;
      }

      /* ── Comments: skip to end of line ────────────────── */
      if (c == '#') {
        skipToEndOfLine();
        continue;
      }

      /* ── Newline: emit NEWLINE token ──────────────────── */
      if (c == '\n') {
        tokens.emplace_back(TokenType::NEWLINE, "\\n", line_, col_);
        advance();
        atLineStart = true;
        continue;
      }

      /* ── Carriage return (Windows line endings) ────────── */
      if (c == '\r') {
        advance();
        if (peek() == '\n')
          advance();
        tokens.emplace_back(TokenType::NEWLINE, "\\n", line_, col_);
        atLineStart = true;
        continue;
      }

      /* ── String literals ──────────────────────────────── */
      if (c == '"' || c == '\'') {
        tokens.push_back(scanString());
        continue;
      }

      /* ── Numeric literals (integer and float) ─────────── */
      if (std::isdigit(c)) {
        tokens.push_back(scanNumber());
        continue;
      }

      /* ── Identifiers and keywords ─────────────────────── */
      if (std::isalpha(c) || c == '_') {
        tokens.push_back(scanIdentifierOrKeyword());
        continue;
      }

      /* ── Operators and punctuation ─────────────────────── */
      Token opTok = scanOperatorOrPunctuation();
      if (opTok.type != TokenType::ERROR_TOKEN) {
        tokens.push_back(opTok);
      } else {
        // Unrecognized character — report error but continue
        errors_.report(line_, col_, ErrorPhase::LEXER,
                       "Unexpected character: '" + std::string(1, c) + "'");
        tokens.push_back(opTok);
        advance();
      }
    }

    // ── End of file: emit remaining DEDENTs and EOF ──────────
    while (indentStack_.size() > 1) {
      indentStack_.pop();
      tokens.emplace_back(TokenType::DEDENT, "<DEDENT>", line_, col_);
    }
    tokens.emplace_back(TokenType::EOF_TOKEN, "<EOF>", line_, col_);

    return tokens;
  }

private:
  const std::string &source_;
  ErrorReporter &errors_;
  size_t pos_;
  int line_;
  int col_;
  std::stack<int> indentStack_; // tracks nesting depth via spaces

  /* ── Character-level helpers ───────────────────────────────── */

  char peek() const { return (pos_ < source_.size()) ? source_[pos_] : '\0'; }

  char peekNext() const {
    return (pos_ + 1 < source_.size()) ? source_[pos_ + 1] : '\0';
  }

  char advance() {
    char c = source_[pos_++];
    if (c == '\n') {
      line_++;
      col_ = 1;
    } else {
      col_++;
    }
    return c;
  }

  /* ── Indentation handling ─────────────────────────────────── */

  /**
   * Check if the current line is blank or contains only a comment.
   * These lines are invisible to Python's indentation rules.
   */
  bool isBlankOrCommentLine() const {
    size_t p = pos_;
    while (p < source_.size() && (source_[p] == ' ' || source_[p] == '\t'))
      p++;
    return (p >= source_.size() || source_[p] == '\n' || source_[p] == '\r' ||
            source_[p] == '#');
  }

  void skipToNextLine() {
    while (pos_ < source_.size() && source_[pos_] != '\n')
      advance();
    if (pos_ < source_.size())
      advance(); // consume the '\n'
  }

  void skipToEndOfLine() {
    while (pos_ < source_.size() && source_[pos_] != '\n')
      advance();
  }

  /**
   * Count leading spaces at the current position.
   * Tabs are expanded to 4 spaces (Python convention).
   */
  int measureIndentation() {
    int spaces = 0;
    while (pos_ < source_.size() &&
           (source_[pos_] == ' ' || source_[pos_] == '\t')) {
      if (source_[pos_] == '\t')
        spaces += 4;
      else
        spaces += 1;
      advance();
    }
    return spaces;
  }

  /**
   * Compare current indentation to the stack and emit
   * INDENT or DEDENT tokens as appropriate.
   */
  void emitIndentationTokens(int indent, std::vector<Token> &tokens) {
    if (indent > indentStack_.top()) {
      // Deeper nesting — push new level
      indentStack_.push(indent);
      tokens.emplace_back(TokenType::INDENT, "<INDENT>", line_, 1);
    } else {
      // May need to pop multiple levels
      while (indent < indentStack_.top()) {
        indentStack_.pop();
        tokens.emplace_back(TokenType::DEDENT, "<DEDENT>", line_, 1);
      }
      // Indentation must match some previous level exactly
      if (indent != indentStack_.top()) {
        errors_.report(line_, 1, ErrorPhase::LEXER,
                       "Indentation does not match any outer level");
      }
    }
  }

  /* ── Token scanners ───────────────────────────────────────── */

  /**
   * Scan a string literal (single or double quoted).
   * Handles escape sequences: \n, \t, \\, \', \"
   */
  Token scanString() {
    int startLine = line_, startCol = col_;
    char quote = advance(); // consume opening quote
    std::string value;

    while (pos_ < source_.size() && peek() != quote) {
      if (peek() == '\n') {
        // Unterminated string — report error but recover
        errors_.report(startLine, startCol, ErrorPhase::LEXER,
                       "Unterminated string literal");
        return Token(TokenType::ERROR_TOKEN, value, startLine, startCol);
      }
      if (peek() == '\\') {
        advance(); // consume backslash
        char esc = advance();
        switch (esc) {
        case 'n':
          value += '\n';
          break;
        case 't':
          value += '\t';
          break;
        case '\\':
          value += '\\';
          break;
        case '\'':
          value += '\'';
          break;
        case '"':
          value += '"';
          break;
        default:
          value += '\\';
          value += esc;
          break;
        }
      } else {
        value += advance();
      }
    }

    if (pos_ >= source_.size()) {
      errors_.report(startLine, startCol, ErrorPhase::LEXER,
                     "Unterminated string literal at end of file");
      return Token(TokenType::ERROR_TOKEN, value, startLine, startCol);
    }

    advance(); // consume closing quote
    return Token(TokenType::STRING_LIT, value, startLine, startCol);
  }

  /**
   * Scan a numeric literal — integers or floats.
   * Supports: 42, 3.14, 0.5, 100 (no hex/oct/bin for simplicity)
   */
  Token scanNumber() {
    int startLine = line_, startCol = col_;
    std::string num;
    bool isFloat = false;

    while (pos_ < source_.size() && std::isdigit(peek())) {
      num += advance();
    }

    // Check for decimal point (but not range like 1..5)
    if (peek() == '.' && std::isdigit(peekNext())) {
      isFloat = true;
      num += advance(); // consume '.'
      while (pos_ < source_.size() && std::isdigit(peek())) {
        num += advance();
      }
    }

    return Token(isFloat ? TokenType::FLOAT_LIT : TokenType::INTEGER_LIT, num,
                 startLine, startCol);
  }

  /**
   * Scan an identifier or keyword.
   * After collecting the word, check against the keyword table.
   */
  Token scanIdentifierOrKeyword() {
    int startLine = line_, startCol = col_;
    std::string word;

    while (pos_ < source_.size() && (std::isalnum(peek()) || peek() == '_')) {
      word += advance();
    }

    // Look up in keyword table
    auto it = keywordMap().find(word);
    if (it != keywordMap().end()) {
      return Token(it->second, word, startLine, startCol);
    }

    return Token(TokenType::IDENTIFIER, word, startLine, startCol);
  }

  /**
   * Scan operators and punctuation characters.
   * Handles multi-character operators like ==, !=, <=, **, //
   */
  Token scanOperatorOrPunctuation() {
    int startLine = line_, startCol = col_;
    char c = peek();

    switch (c) {
    /* ── Two-char operators checked first ─────────────── */
    case '+':
      advance();
      if (peek() == '=') {
        advance();
        return Token(TokenType::PLUS_ASSIGN, "+=", startLine, startCol);
      }
      return Token(TokenType::PLUS, "+", startLine, startCol);

    case '-':
      advance();
      if (peek() == '=') {
        advance();
        return Token(TokenType::MINUS_ASSIGN, "-=", startLine, startCol);
      }
      return Token(TokenType::MINUS, "-", startLine, startCol);

    case '*':
      advance();
      if (peek() == '*') {
        advance();
        return Token(TokenType::DOUBLE_STAR, "**", startLine, startCol);
      }
      if (peek() == '=') {
        advance();
        return Token(TokenType::STAR_ASSIGN, "*=", startLine, startCol);
      }
      return Token(TokenType::STAR, "*", startLine, startCol);

    case '/':
      advance();
      if (peek() == '/') {
        advance();
        return Token(TokenType::DOUBLE_SLASH, "//", startLine, startCol);
      }
      if (peek() == '=') {
        advance();
        return Token(TokenType::SLASH_ASSIGN, "/=", startLine, startCol);
      }
      return Token(TokenType::SLASH, "/", startLine, startCol);

    case '%':
      advance();
      return Token(TokenType::PERCENT, "%", startLine, startCol);

    case '=':
      advance();
      if (peek() == '=') {
        advance();
        return Token(TokenType::EQ, "==", startLine, startCol);
      }
      return Token(TokenType::ASSIGN, "=", startLine, startCol);

    case '!':
      advance();
      if (peek() == '=') {
        advance();
        return Token(TokenType::NEQ, "!=", startLine, startCol);
      }
      // Standalone '!' is not valid Python — report error
      errors_.report(startLine, startCol, ErrorPhase::LEXER,
                     "Invalid operator '!' — did you mean 'not'?");
      return Token(TokenType::ERROR_TOKEN, "!", startLine, startCol);

    case '<':
      advance();
      if (peek() == '=') {
        advance();
        return Token(TokenType::LTE, "<=", startLine, startCol);
      }
      return Token(TokenType::LT, "<", startLine, startCol);

    case '>':
      advance();
      if (peek() == '=') {
        advance();
        return Token(TokenType::GTE, ">=", startLine, startCol);
      }
      return Token(TokenType::GT, ">", startLine, startCol);

    /* ── Single-char punctuation ──────────────────────── */
    case '(':
      advance();
      return Token(TokenType::LPAREN, "(", startLine, startCol);
    case ')':
      advance();
      return Token(TokenType::RPAREN, ")", startLine, startCol);
    case '[':
      advance();
      return Token(TokenType::LBRACKET, "[", startLine, startCol);
    case ']':
      advance();
      return Token(TokenType::RBRACKET, "]", startLine, startCol);
    case ':':
      advance();
      return Token(TokenType::COLON, ":", startLine, startCol);
    case ',':
      advance();
      return Token(TokenType::COMMA, ",", startLine, startCol);

    default:
      // Unrecognized — caller will report and advance
      return Token(TokenType::ERROR_TOKEN, std::string(1, c), startLine,
                   startCol);
    }
  }
};