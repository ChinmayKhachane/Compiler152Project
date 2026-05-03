#include "automata.hpp"
#include <cctype>
#include <queue>

void DFA::addTransition(int from, char symbol, int to) {
  transitions_[from][symbol] = to;
}

bool DFA::accepts(const std::string &input) const {
  int state = startState_;

  for (char c : input) {
    auto fromIt = transitions_.find(state);
    if (fromIt == transitions_.end())
      return false;

    auto toIt = fromIt->second.find(c);
    if (toIt == fromIt->second.end())
      return false;

    state = toIt->second;
  }

  return acceptStates_.find(state) != acceptStates_.end();
}

void NFA::addTransition(int from, char symbol, int to) {
  transitions_[from][symbol].insert(to);
}

void NFA::addEpsilonTransition(int from, int to) {
  epsilonTransitions_[from].insert(to);
}

std::unordered_set<int>
NFA::epsilonClosure(const std::unordered_set<int> &states) const {
  std::unordered_set<int> closure = states;
  std::queue<int> q;

  for (int s : states)
    q.push(s);

  while (!q.empty()) {
    int cur = q.front();
    q.pop();
    auto epsIt = epsilonTransitions_.find(cur);
    if (epsIt == epsilonTransitions_.end())
      continue;

    for (int nxt : epsIt->second) {
      if (closure.insert(nxt).second)
        q.push(nxt);
    }
  }
  return closure;
}

bool NFA::accepts(const std::string &input) const {
  std::unordered_set<int> current = epsilonClosure({startState_});

  for (char c : input) {
    std::unordered_set<int> nextStates;

    for (int s : current) {
      auto fromIt = transitions_.find(s);
      if (fromIt == transitions_.end())
        continue;
      auto symIt = fromIt->second.find(c);
      if (symIt == fromIt->second.end())
        continue;
      for (int nxt : symIt->second)
        nextStates.insert(nxt);
    }

    current = epsilonClosure(nextStates);
    if (current.empty())
      return false;
  }

  for (int s : current) {
    if (acceptStates_.find(s) != acceptStates_.end())
      return true;
  }
  return false;
}

std::vector<CFGExpressionValidator::Tok>
CFGExpressionValidator::tokenize(const std::string &expr,
                                 std::optional<std::string> &error) const {
  std::vector<Tok> tokens;
  size_t i = 0;

  while (i < expr.size()) {
    unsigned char uc = static_cast<unsigned char>(expr[i]);
    if (std::isspace(uc)) {
      i++;
      continue;
    }

    char c = expr[i];
    if (std::isalpha(uc) || c == '_') {
      size_t start = i++;
      while (i < expr.size()) {
        unsigned char v = static_cast<unsigned char>(expr[i]);
        if (!std::isalnum(v) && expr[i] != '_')
          break;
        i++;
      }
      tokens.push_back({TokKind::IDENT, expr.substr(start, i - start)});
      continue;
    }

    if (std::isdigit(uc)) {
      size_t start = i++;
      while (i < expr.size() &&
             std::isdigit(static_cast<unsigned char>(expr[i]))) {
        i++;
      }
      tokens.push_back({TokKind::NUMBER, expr.substr(start, i - start)});
      continue;
    }

    switch (c) {
    case '+': tokens.push_back({TokKind::PLUS, "+"}); break;
    case '-': tokens.push_back({TokKind::MINUS, "-"}); break;
    case '*': tokens.push_back({TokKind::STAR, "*"}); break;
    case '/': tokens.push_back({TokKind::SLASH, "/"}); break;
    case '(': tokens.push_back({TokKind::LPAREN, "("}); break;
    case ')': tokens.push_back({TokKind::RPAREN, ")"}); break;
    default:
      error = "CFG tokenizer rejected character '" + std::string(1, c) + "'";
      return {};
    }
    i++;
  }

  tokens.push_back({TokKind::END, "<END>"});
  return tokens;
}

bool CFGExpressionValidator::parseExpr(const std::vector<Tok> &tokens,
                                       size_t &pos) const {
  if (!parseTerm(tokens, pos))
    return false;

  while (tokens[pos].kind == TokKind::PLUS || tokens[pos].kind == TokKind::MINUS) {
    pos++;
    if (!parseTerm(tokens, pos))
      return false;
  }
  return true;
}

bool CFGExpressionValidator::parseTerm(const std::vector<Tok> &tokens,
                                       size_t &pos) const {
  if (!parseFactor(tokens, pos))
    return false;

  while (tokens[pos].kind == TokKind::STAR || tokens[pos].kind == TokKind::SLASH) {
    pos++;
    if (!parseFactor(tokens, pos))
      return false;
  }
  return true;
}

bool CFGExpressionValidator::parseFactor(const std::vector<Tok> &tokens,
                                         size_t &pos) const {
  TokKind k = tokens[pos].kind;
  if (k == TokKind::IDENT || k == TokKind::NUMBER) {
    pos++;
    return true;
  }

  if (k == TokKind::LPAREN) {
    pos++;
    if (!parseExpr(tokens, pos))
      return false;
    if (tokens[pos].kind != TokKind::RPAREN)
      return false;
    pos++;
    return true;
  }

  return false;
}

bool CFGExpressionValidator::validate(const std::string &expr,
                                      std::optional<std::string> &error) const {
  error.reset();
  auto tokens = tokenize(expr, error);
  if (tokens.empty())
    return false;

  size_t pos = 0;
  if (!parseExpr(tokens, pos)) {
    error = "CFG parse failed for expression: " + expr;
    return false;
  }
  if (tokens[pos].kind != TokKind::END) {
    error = "CFG parse stopped early near token '" + tokens[pos].lexeme + "'";
    return false;
  }
  return true;
}

DFA buildIdentifierDFA() {
  DFA dfa;
  dfa.setStartState(0);
  dfa.addAcceptState(1);

  for (char c = 'a'; c <= 'z'; ++c) {
    dfa.addTransition(0, c, 1);
    dfa.addTransition(1, c, 1);
  }
  for (char c = 'A'; c <= 'Z'; ++c) {
    dfa.addTransition(0, c, 1);
    dfa.addTransition(1, c, 1);
  }
  dfa.addTransition(0, '_', 1);
  dfa.addTransition(1, '_', 1);
  for (char c = '0'; c <= '9'; ++c) {
    dfa.addTransition(1, c, 1);
  }

  return dfa;
}

NFA buildNumberNFA() {
  // Accepts: digit+ | digit+ '.' digit+
  NFA nfa;
  nfa.setStartState(0);
  nfa.addAcceptState(1);
  nfa.addAcceptState(3);

  for (char c = '0'; c <= '9'; ++c) {
    nfa.addTransition(0, c, 1);
    nfa.addTransition(1, c, 1);
    nfa.addTransition(2, c, 3);
    nfa.addTransition(3, c, 3);
  }
  nfa.addTransition(1, '.', 2);

  return nfa;
}
