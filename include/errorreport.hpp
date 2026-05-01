/**
 * error.hpp — Error Reporting Module
 * ====================================
 * Centralized error collection for all compiler phases.
 * Errors are accumulated (not fatal), enabling the interpreter
 * to continue analysis and report ALL errors in a single pass.
 * Target code is generated only when this list is empty.
 */

#pragma once
#include <iomanip>
#include <iostream>
#include <sstream>
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
  void report(int line, int col, ErrorPhase phase, const std::string &msg) {
    errors_.emplace_back(line, col, phase, msg);
  }

  /* Quick check: are we error-free? */
  bool hasErrors() const { return !errors_.empty(); }

  /* Total error count */
  size_t count() const { return errors_.size(); }

  /* Access the raw list (for structured output) */
  const std::vector<CompileError> &errors() const { return errors_; }

  /* Pretty-print all errors with source context */
  void display(const std::vector<std::string> &sourceLines) const {
    if (errors_.empty())
      return;

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║             SYNTAX ERROR REPORT                 ║\n";
    std::cout << "╠══════════════════════════════════════════════════╣\n";
    std::cout << "║  Total errors found: " << std::left << std::setw(28)
              << errors_.size() << "║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    for (size_t i = 0; i < errors_.size(); ++i) {
      const auto &err = errors_[i];
      std::string phaseName;
      switch (err.phase) {
      case ErrorPhase::LEXER:
        phaseName = "Lexical";
        break;
      case ErrorPhase::PARSER:
        phaseName = "Syntax";
        break;
      case ErrorPhase::RUNTIME:
        phaseName = "Runtime";
        break;
      }

      std::cout << "  Error #" << (i + 1) << " [" << phaseName << "]\n";
      std::cout << "  Line " << err.line << ", Col " << err.col << ": "
                << err.message << "\n";

      // Show the offending source line if available
      if (err.line > 0 && err.line <= (int)sourceLines.size()) {
        std::cout << "    " << err.line << " | " << sourceLines[err.line - 1]
                  << "\n";
        // Caret pointer to error column
        std::cout << "    " << std::string(std::to_string(err.line).size(), ' ')
                  << " | " << std::string(std::max(0, err.col - 1), ' ')
                  << "^\n";
      }
      std::cout << "\n";
    }

    std::cout << "  Compilation FAILED — no target code generated.\n\n";
  }

  /* Reset for a new compilation unit */
  void clear() { errors_.clear(); }

private:
  std::vector<CompileError> errors_;
};
