#include "parser.h"
#include "record.h"
#include <stdexcept>
#include <utility>

enum class ParserState {
    Invalid,
    Initial,
    ShowKeyword,
    InsertKeyword,
    UpdateKeyword,
    RemoveKeyword,
    ListKeyword,
    SortKeyword,
    StatusKeyword,
    ShowKeyNumber,
    InsertKeyNumber,
    InsertLeftBrace,
    InsertRightBrace,
    InsertSetNumber,
    InsertComma,
    UpdateKeyNumber,
    UpdateLeftBrace,
    UpdateRightBrace,
    UpdateSetNumber,
    UpdateComma,
    RemoveKeyNumber,
};

std::vector<std::unique_ptr<Statement>> Parser::parse(const std::vector<Token>& tokens) const {
    std::vector<std::unique_ptr<Statement>> statements{};
    auto parser_state = ParserState::Initial;
    auto numbers = std::vector<Record::data_type>();
    Record::key_type key{};
    for (const auto& token : tokens) {
        if (parser_state == ParserState::Invalid) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                numbers.clear();
                parser_state = ParserState::Initial;
            }
        }
        else if (parser_state == ParserState::Initial) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<EmptyStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::ShowKeyword) {
                parser_state = ParserState::ShowKeyword;
            }
            else if (token.type == TokenType::InsertKeyword) {
                parser_state = ParserState::InsertKeyword;
            }
            else if (token.type == TokenType::UpdateKeyword) {
                parser_state = ParserState::UpdateKeyword;
            }
            else if (token.type == TokenType::RemoveKeyword) {
                parser_state = ParserState::RemoveKeyword;
            }
            else if (token.type == TokenType::ListKeyword) {
                parser_state = ParserState::ListKeyword;
            }
            else if (token.type == TokenType::SortKeyword) {
                parser_state = ParserState::SortKeyword;
            }
            else if (token.type == TokenType::StatusKeyword) {
                parser_state = ParserState::StatusKeyword;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::ListKeyword) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<ListStatement>());
                parser_state = ParserState::Initial;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::SortKeyword) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<SortStatement>());
                parser_state = ParserState::Initial;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::StatusKeyword) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<StatusStatement>());
                parser_state = ParserState::Initial;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::ShowKeyword) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Number) {
                key = static_cast<Record::key_type>(std::stoull(token.text));
                parser_state = ParserState::ShowKeyNumber;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::ShowKeyNumber) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<ShowStatement>(key));
                parser_state = ParserState::Initial;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::RemoveKeyword) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Number) {
                key = static_cast<Record::key_type>(std::stoull(token.text));
                parser_state = ParserState::RemoveKeyNumber;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::RemoveKeyNumber) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<RemoveStatement>(key));
                parser_state = ParserState::Initial;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::UpdateKeyword) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Number) {
                key = static_cast<Record::key_type>(std::stoull(token.text));
                parser_state = ParserState::UpdateKeyNumber;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::UpdateKeyNumber) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::LeftBrace) {
                parser_state = ParserState::UpdateLeftBrace;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::UpdateLeftBrace) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Number) {
                numbers.emplace_back(static_cast<Record::data_type>(std::stoi(token.text)));
                parser_state = ParserState::UpdateSetNumber;
            }
            else if (token.type == TokenType::RightBrace) {
                parser_state = ParserState::UpdateRightBrace;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::UpdateSetNumber) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                numbers.clear();
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Comma) {
                parser_state = ParserState::UpdateComma;
            }
            else if (token.type == TokenType::RightBrace) {
                parser_state = ParserState::UpdateRightBrace;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::UpdateComma) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                numbers.clear();
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Number) {
                numbers.emplace_back(static_cast<Record::data_type>(std::stoi(token.text)));
                parser_state = ParserState::UpdateSetNumber;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::UpdateRightBrace) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<UpdateStatement>(key, std::move(numbers)));
                numbers.clear();
                parser_state = ParserState::Initial;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::InsertKeyword) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Number) {
                key = static_cast<Record::key_type>(std::stoull(token.text));
                parser_state = ParserState::InsertKeyNumber;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::InsertKeyNumber) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::LeftBrace) {
                parser_state = ParserState::InsertLeftBrace;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::InsertLeftBrace) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Number) {
                numbers.emplace_back(static_cast<Record::data_type>(std::stoi(token.text)));
                parser_state = ParserState::InsertSetNumber;
            }
            else if (token.type == TokenType::RightBrace) {
                parser_state = ParserState::InsertRightBrace;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::InsertSetNumber) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                numbers.clear();
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Comma) {
                parser_state = ParserState::InsertComma;
            }
            else if (token.type == TokenType::RightBrace) {
                parser_state = ParserState::InsertRightBrace;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::InsertComma) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InvalidStatement>());
                numbers.clear();
                parser_state = ParserState::Initial;
            }
            else if (token.type == TokenType::Number) {
                numbers.emplace_back(static_cast<Record::data_type>(std::stoi(token.text)));
                parser_state = ParserState::InsertSetNumber;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else if (parser_state == ParserState::InsertRightBrace) {
            if (token.type == TokenType::Semicolon) {
                statements.emplace_back(std::make_unique<InsertStatement>(key, std::move(numbers)));
                numbers.clear();
                parser_state = ParserState::Initial;
            }
            else {
                parser_state = ParserState::Invalid;
            }
        }
        else {
            throw std::logic_error("Parser state was not recognized");
        }
    }
    if (parser_state != ParserState::Initial) {
        statements.emplace_back(std::make_unique<InvalidStatement>());
    }
    return statements;
}
