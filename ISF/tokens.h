#pragma once
#include <string>
#include <string_view>

enum class TokenType {
    Invalid,
    ShowKeyword,
    InsertKeyword,
    UpdateKeyword,
    RemoveKeyword,
    ListKeyword,
    SortKeyword,
    StatusKeyword,
    Number,
    Semicolon,
    Comma,
    LeftBrace,
    RightBrace,
};

struct Token
{
    TokenType type;
    std::string text;
};

TokenType string_to_token_type(std::string_view string);
