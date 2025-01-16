#pragma once
#include "record.h"
#include <utility>
#include <vector>

enum class StatementType {
    Empty,
    Invalid,
    Show,
    Insert,
    Update,
    Remove,
    List,
    Sort,
    Status,
};

struct Statement
{
    StatementType type;

    Statement() = default;
    Statement(StatementType type) : type(type) {}
};

struct EmptyStatement : Statement
{
    EmptyStatement() : Statement(StatementType::Empty) {}
};

struct InvalidStatement : Statement
{
    InvalidStatement() : Statement(StatementType::Invalid) {}
};

struct ShowStatement : Statement
{
    Record::key_type key;

    ShowStatement() : Statement(StatementType::Show), key{} {}
    ShowStatement(Record::key_type key) : Statement(StatementType::Show), key(key) {}
};

struct InsertStatement : Statement
{
    Record::key_type key;
    std::vector<Record::data_type> data;

    InsertStatement() : Statement(StatementType::Insert), key{}, data{} {}
    InsertStatement(Record::key_type key, const std::vector<Record::data_type>& data)
        : Statement(StatementType::Insert), key(key), data(data) {}
    InsertStatement(Record::key_type key, std::vector<Record::data_type>&& data)
        : Statement(StatementType::Insert), key(key), data(std::move(data)) {}
};

struct UpdateStatement : Statement
{
    Record::key_type key;
    std::vector<Record::data_type> data;

    UpdateStatement() : Statement(StatementType::Update), key{}, data{} {}
    UpdateStatement(Record::key_type key, const std::vector<Record::data_type>& data)
        : Statement(StatementType::Update), key(key), data(data) {}
    UpdateStatement(Record::key_type key, std::vector<Record::data_type>&& data)
        : Statement(StatementType::Update), key(key), data(std::move(data)) {}
};

struct RemoveStatement : Statement
{
    Record::key_type key;

    RemoveStatement() : Statement(StatementType::Remove), key{} {}
    RemoveStatement(Record::key_type key) : Statement(StatementType::Remove), key(key) {}
};

struct ListStatement : Statement
{
    ListStatement() : Statement(StatementType::List) {}
};

struct SortStatement : Statement
{
    SortStatement() : Statement(StatementType::Sort) {}
};

struct StatusStatement : Statement
{
    StatusStatement() : Statement(StatementType::Status) {}
};
