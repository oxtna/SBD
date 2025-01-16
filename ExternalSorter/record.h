#pragma once
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Record
{
  public:
    Record() = default;
    Record(const Record&) = default;
    Record(Record&& other) noexcept;
    Record(std::initializer_list<std::int32_t> data);
    explicit Record(const std::vector<std::int32_t>& data);
    explicit Record(std::vector<std::int32_t>&& data);
    ~Record() = default;

    constexpr std::size_t size() const { return _data.size(); }

    constexpr const std::vector<std::int32_t>& data() const { return _data; }

    std::string toString();

    constexpr Record& operator=(const Record&) = default;

    constexpr Record& operator=(Record&& other) noexcept {
        this->_data = std::move(other._data);
        other._data.clear();
        return *this;
    }

    constexpr bool operator==(const Record& other) const {
        if (this->_data.empty() && other._data.empty()) {
            return true;
        }
        if (this->_data.empty() || other._data.empty()) {
            return false;
        }
        return this->_data.front() == other._data.front();
    }

    constexpr bool operator!=(const Record& other) const { return !(*this == other); }

    constexpr bool operator<(const Record& other) const {
        if (this->_data.empty()) {
            throw std::logic_error("Records cannot be compared: this record is empty");
        }
        if (other._data.empty()) {
            throw std::logic_error("Records cannot be compared: other record is empty");
        }
        return this->_data.front() < other._data.front();
    }

    constexpr bool operator<=(const Record& other) const {
        return *this == other ? true : *this < other;
    }

    constexpr bool operator>(const Record& other) const { return other < *this; }

    constexpr bool operator>=(const Record& other) const {
        return *this == other ? true : other < *this;
    }

  private:
    std::vector<std::int32_t> _data;
};
