#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Record
{
  public:
    using data_type = std::int32_t;
    using key_type = std::uint64_t;
    using size_type = std::uint64_t;

    static constexpr std::size_t max_size = 15;
    static constexpr std::size_t total_size_in_bytes =
        sizeof(key_type) + sizeof(size_type) + sizeof(data_type) * max_size;

    Record() = default;
    Record(const Record&) = default;
    Record(Record&& other) noexcept;
    Record(key_type key, std::initializer_list<data_type> data);
    Record(key_type key, const std::vector<data_type>& data);
    Record(key_type key, std::vector<data_type>&& data);
    ~Record() = default;

    constexpr size_type size() const { return static_cast<size_type>(_data.size()); }

    constexpr const std::vector<data_type>& data() const { return _data; }

    constexpr key_type key() const { return _key; }

    std::string to_string() const;

    char* serialize(char* destination, std::size_t capacity) const;

    static std::pair<char*, std::optional<Record>> deserialize(char* source);

    constexpr Record& operator=(const Record&) = default;

    Record& operator=(Record&& other) noexcept;

    constexpr bool operator==(const Record& other) const { return this->_key == other._key; }

    constexpr bool operator!=(const Record& other) const { return !(*this == other); }

    constexpr bool operator<(const Record& other) const { return this->_key < other._key; }

    constexpr bool operator<=(const Record& other) const {
        return *this == other ? true : *this < other;
    }

    constexpr bool operator>(const Record& other) const { return other < *this; }

    constexpr bool operator>=(const Record& other) const {
        return *this == other ? true : other < *this;
    }

  private:
    std::vector<data_type> _data;
    std::size_t _key;
};
