#include "errorreport.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>

void ErrorReporter::report(int line, int col, ErrorPhase phase,
                           const std::string &msg) {
  errors_.emplace_back(line, col, phase, msg);
}

void ErrorReporter::display(const std::vector<std::string> &sourceLines) const {
  if (errors_.empty())
    return;

  size_t lexicalCount = 0;
  size_t syntaxCount = 0;
  size_t runtimeCount = 0;
  for (const auto &err : errors_) {
    switch (err.phase) {
    case ErrorPhase::LEXER: lexicalCount++; break;
    case ErrorPhase::PARSER: syntaxCount++; break;
    case ErrorPhase::RUNTIME: runtimeCount++; break;
    }
  }

  std::cout << "\n";
  std::cout << "====================================================\n";
  std::cout << "COMPILATION ERROR REPORT\n";
  std::cout << "----------------------------------------------------\n";
  std::cout << "Total errors : " << errors_.size() << "\n";
  std::cout << "Lexical      : " << lexicalCount << "\n";
  std::cout << "Syntax       : " << syntaxCount << "\n";
  std::cout << "Runtime      : " << runtimeCount << "\n";
  std::cout << "====================================================\n\n";

  for (size_t i = 0; i < errors_.size(); ++i) {
    const auto &err = errors_[i];
    std::string phaseName;
    switch (err.phase) {
    case ErrorPhase::LEXER: phaseName = "LEXICAL_ERROR"; break;
    case ErrorPhase::PARSER: phaseName = "SYNTAX_ERROR"; break;
    case ErrorPhase::RUNTIME: phaseName = "RUNTIME_ERROR"; break;
    }

    std::cout << "Error #" << (i + 1) << "\n";
    std::cout << "Type    : " << phaseName << "\n";
    std::cout << "Line/Col: " << err.line << ":" << err.col << "\n";
    std::cout << "Message : " << err.message << "\n";

    if (err.line > 0 && err.line <= (int)sourceLines.size()) {
      std::cout << "Code    : " << err.line << " | " << sourceLines[err.line - 1]
                << "\n";
      std::cout << "          " << std::string(std::to_string(err.line).size(), ' ')
                << " | " << std::string(std::max(0, err.col - 1), ' ')
                << "^\n";
    }
    std::cout << "----------------------------------------------------\n";
  }

  std::cout << "Compilation FAILED.\n\n";
}
