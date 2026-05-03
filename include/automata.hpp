#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class DFA {
public:
  DFA() : startState_(0) {}

  void setStartState(int state) { startState_ = state; }
  void addAcceptState(int state) { acceptStates_.insert(state); }
  void addTransition(int from, char symbol, int to);

  bool accepts(const std::string &input) const;

private:
  int startState_;
  std::unordered_set<int> acceptStates_;
  std::unordered_map<int, std::unordered_map<char, int>> transitions_;
};

class NFA {
public:
  NFA() : startState_(0) {}

  void setStartState(int state) { startState_ = state; }
  void addAcceptState(int state) { acceptStates_.insert(state); }
  void addTransition(int from, char symbol, int to);
  void addEpsilonTransition(int from, int to);

  bool accepts(const std::string &input) const;

private:
  std::unordered_set<int>
  epsilonClosure(const std::unordered_set<int> &states) const;

  int startState_;
  std::unordered_set<int> acceptStates_;
  std::unordered_map<int, std::unordered_map<char, std::unordered_set<int>>>
      transitions_;
  std::unordered_map<int, std::unordered_set<int>> epsilonTransitions_;
};

class CFGExpressionValidator {
public:
  bool validate(const std::string &expr,
                std::optional<std::string> &error) const;

private:
  enum class TokKind { IDENT, NUMBER, PLUS, MINUS, STAR, SLASH, LPAREN, RPAREN, END };

  struct Tok {
    TokKind kind;
    std::string lexeme;
  };

  std::vector<Tok> tokenize(const std::string &expr,
                            std::optional<std::string> &error) const;
  bool parseExpr(const std::vector<Tok> &tokens, size_t &pos) const;
  bool parseTerm(const std::vector<Tok> &tokens, size_t &pos) const;
  bool parseFactor(const std::vector<Tok> &tokens, size_t &pos) const;
};

DFA buildIdentifierDFA();
NFA buildNumberNFA();
