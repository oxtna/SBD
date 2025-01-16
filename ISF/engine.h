#pragma once
#include "statements.h"
#include <exception>
#include <iostream>
#include <string>

class Engine
{
  public:
    Engine() = delete;
    explicit Engine(std::ostream& output)
        : _output(output),
          _index_reads{},
          _index_writes{},
          _data_reads{},
          _data_writes{},
          _overflow_reads{},
          _overflow_writes{} {}
    ~Engine() = default;

    void execute(const Statement* statement);

    void sort();
    void merge();
    void rebuild_index();

    std::size_t total_operations() const { return total_reads() + total_writes(); }
    std::size_t total_reads() const { return _index_reads + _data_reads + _overflow_reads; }
    std::size_t total_writes() const { return _index_writes + _data_writes + _overflow_writes; }
    std::size_t total_index_operations() const { return _index_reads + _index_writes; }
    std::size_t total_data_operations() const { return _data_reads + _data_writes; }
    std::size_t total_overflow_operations() const { return _overflow_reads + _overflow_writes; }
    std::size_t index_reads() const { return _index_reads; }
    std::size_t index_writes() const { return _index_writes; }
    std::size_t data_reads() const { return _data_reads; }
    std::size_t data_writes() const { return _data_writes; }
    std::size_t overflow_reads() const { return _overflow_reads; }
    std::size_t overflow_writes() const { return _overflow_writes; }

  private:
    std::ostream& _output;
    std::size_t _index_reads;
    std::size_t _index_writes;
    std::size_t _data_reads;
    std::size_t _data_writes;
    std::size_t _overflow_reads;
    std::size_t _overflow_writes;

    void execute_raw(const Statement* statement);
};

class KeyError : public std::exception
{
  public:
    explicit KeyError(const std::string& message) : std::exception(message.c_str()) {}
    explicit KeyError(const char* message) : std::exception(message) {}
};

class NotInitializedError : public std::exception
{
  public:
    explicit NotInitializedError(const std::string& message) : std::exception(message.c_str()) {}
    explicit NotInitializedError(const char* message) : std::exception(message) {}
};

class InvalidStatementError : public std::exception
{
  public:
    explicit InvalidStatementError(const std::string& message) : std::exception(message.c_str()) {}
    explicit InvalidStatementError(const char* message) : std::exception(message) {}
};

class NotImplementedError : public std::exception
{
  public:
    explicit NotImplementedError(const std::string& message) : std::exception(message.c_str()) {}
    explicit NotImplementedError(const char* message) : std::exception(message) {}
};
