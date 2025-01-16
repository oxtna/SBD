#include "tokens.h"
#include <algorithm>
#include <cctype>

TokenType string_to_token_type(std::string_view string) {
    if (string.empty()) {
        return TokenType::Invalid;
    }
    std::string lowered(string);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    if (lowered == "show") {
        return TokenType::ShowKeyword;
    }
    else if (lowered == "insert") {
        return TokenType::InsertKeyword;
    }
    else if (lowered == "update") {
        return TokenType::UpdateKeyword;
    }
    else if (lowered == "remove") {
        return TokenType::RemoveKeyword;
    }
    else if (lowered == "list") {
        return TokenType::ListKeyword;
    }
    else if (lowered == "sort") {
        return TokenType::SortKeyword;
    }
    else if (lowered == "status") {
        return TokenType::StatusKeyword;
    }
    else if (lowered == ";") {
        return TokenType::Semicolon;
    }
    else if (lowered == ",") {
        return TokenType::Comma;
    }
    else if (lowered == "{") {
        return TokenType::LeftBrace;
    }
    else if (lowered == "}") {
        return TokenType::RightBrace;
    }
    else {
        auto begin = lowered.begin();
        if (*begin == '+' || *begin == '-') {
            begin++;
        }
        if (std::find_if(begin, lowered.end(), [](unsigned char c) { return !std::isdigit(c); }) ==
            lowered.end()) {
            return TokenType::Number;
        }
        else {
            return TokenType::Invalid;
        }
    }
}
