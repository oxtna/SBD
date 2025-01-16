#include "lexer.h"

std::vector<Token> Lexer::tokenize(const std::string& string) const {
    auto lexemes = std::vector<std::string>(
        std::sregex_token_iterator(string.begin(), string.end(), _pattern),
        std::sregex_token_iterator{});
    std::vector<Token> tokens{};
    tokens.reserve(lexemes.size());
    for (auto& lexeme : lexemes) {
        TokenType token_type = string_to_token_type(lexeme);
        tokens.emplace_back(token_type, std::move(lexeme));
    }
    return tokens;
}
