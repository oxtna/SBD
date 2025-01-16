#pragma once
#include "tokens.h"
#include <regex>
#include <string>
#include <utility>
#include <vector>

class Lexer
{
  public:
    Lexer() = default;
    Lexer(const Lexer&) = delete;
    Lexer(Lexer&&) = delete;
    explicit Lexer(const std::regex& pattern) : _pattern(pattern) {}
    explicit Lexer(std::regex&& pattern) : _pattern(std::move(pattern)) {}
    ~Lexer() = default;

    std::regex pattern() const { return _pattern; }

    void pattern(const std::regex& pattern) { _pattern = pattern; }
    void pattern(std::regex&& pattern) { _pattern = std::move(pattern); }

    std::vector<Token> tokenize(const std::string& string) const;

    Lexer& operator=(const Lexer&) = delete;
    Lexer& operator=(Lexer&&) = delete;

  private:
    std::regex _pattern;
};
