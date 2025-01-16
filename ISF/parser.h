#pragma once
#include "statements.h"
#include "tokens.h"
#include <memory>
#include <string>
#include <vector>

class Parser
{
  public:
    Parser() = default;
    Parser(const Parser&) = delete;
    Parser(Parser&&) = delete;
    ~Parser() = default;

    std::vector<std::unique_ptr<Statement>> parse(const std::vector<Token>& tokens) const;
};
