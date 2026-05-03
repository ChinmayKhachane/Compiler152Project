/**
 * errorreport.hpp — Error Reporting Module
 * ==========================================
 * Centralized error collection for all compiler phases.
 * Errors are accumulated (not fatal), enabling the interpreter
 * to continue analysis and report ALL errors in a single pass.
 * Target code is generated only when this list is empty.
 */

#pragma once
#include <string>
#include <vector>

/* ── Error severity levels ────────────────────────────────────── */
enum class ErrorPhase {
  LEXER,  // tokenization errors (bad chars, unterminated strings)
  PARSER, // syntax errors (unexpected tokens, missing colons)
  RUNTIME // interpretation errors (type mismatch, undefined var)
};

/* ── Single error record ──────────────────────────────────────── */
struct CompileError {
  int line;            // source line number (1-indexed)
  int col;             // column number (1-indexed)
  ErrorPhase phase;    // which compiler phase caught it
  std::string message; // human-readable description

  CompileError(int ln, int c, ErrorPhase p, const std::string &msg)
      : line(ln), col(c), phase(p), message(msg) {}
};

/* ── The Error Reporter ───────────────────────────────────────── */
class ErrorReporter {
public:
  /* Record a new error — compilation continues regardless */
  void report(int line, int col, ErrorPhase phase, const std::string &msg);

  /* Quick check: are we error-free? */
  bool hasErrors() const { return !errors_.empty(); }

  /* Total error count */
  size_t count() const { return errors_.size(); }

  /* Access the raw list (for structured output) */
  const std::vector<CompileError> &errors() const { return errors_; }

  /* Pretty-print all errors with source context */
  void display(const std::vector<std::string> &sourceLines) const;

  /* Reset for a new compilation unit */
  void clear() { errors_.clear(); }

private:
  std::vector<CompileError> errors_;
};
