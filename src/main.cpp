#include "automata.hpp"
#include "errorreport.hpp"
#include "interpreter.hpp"
#include "irgen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "token.hpp"
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> splitLines(const std::string &text) {
  std::vector<std::string> lines;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line))
    lines.push_back(line);
  return lines;
}

bool readFile(const std::string &path, std::string &contents) {
  std::ifstream in(path);
  if (!in)
    return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  contents = ss.str();
  return true;
}
} // namespace

int main(int argc, char **argv) {
  std::string inputPath = "test/test.py";
  if (argc > 1)
    inputPath = argv[1];

  std::string source;
  if (!readFile(inputPath, source)) {
    std::cerr << "Failed to open source file: " << inputPath << "\n";
    return 1;
  }
  auto sourceLines = splitLines(source);

  ErrorReporter errors;

  Lexer lexer(source, errors);
  auto tokens = lexer.tokenize();

  // Demonstrate CFG validation on arithmetic expressions found in source lines.
  CFGExpressionValidator cfg;
  for (const auto &line : sourceLines) {
    if (line.find('=') == std::string::npos)
      continue;
    std::optional<std::string> err;
    std::string rhs = line.substr(line.find('=') + 1);
    (void)cfg.validate(rhs, err);
  }

  if (errors.hasErrors()) {
    errors.display(sourceLines);
    return 1;
  }

  Parser parser(tokens, errors);
  auto ast = parser.parse();
  if (errors.hasErrors()) {
    errors.display(sourceLines);
    return 1;
  }
  if (!ast) {
    std::cerr << "Parser failed to build AST.\n";
    return 1;
  }

  IRGenerator irGen;
  auto irLines = irGen.generate(*ast);
  std::cout << "\n====================================================\n";
  std::cout << "SOURCE: " << inputPath << "\n";
  std::cout << "STATUS: OK\n";
  std::cout << "====================================================\n\n";

  std::cout << "Intermediate Code\n";
  std::cout << "-----------------\n";
  for (size_t i = 0; i < irLines.size(); ++i) {
    std::cout << (i + 1) << ". " << irLines[i] << "\n";
  }
  std::cout << "-----------------\n\n";

  MemoryManager memory;
  std::ostringstream programOutput;
  Interpreter interpreter(errors, memory, programOutput);
  interpreter.execute(*ast);

  if (errors.hasErrors()) {
    errors.display(sourceLines);
    return 1;
  }

  std::cout << "Program Output\n";
  std::cout << "--------------\n";
  std::cout << programOutput.str();
  if (programOutput.str().empty())
    std::cout << "(no output)\n";
  std::cout << "--------------\n\n";

  std::cout << "Memory Summary\n";
  std::cout << "--------------\n";
  std::cout << "Total allocations : " << memory.totalHeapAllocations() << "\n";
  std::cout << "Live heap objects : " << memory.liveHeapObjects() << "\n";
  std::cout << "--------------\n";

  return 0;
}
