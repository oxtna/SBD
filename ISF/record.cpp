#include "record.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>

Record::Record(Record&& other) noexcept : _data{std::move(other._data)}, _key{other._key} {
    other._data.clear();
}

Record::Record(key_type key, std::initializer_list<data_type> data) : _data{}, _key{key} {
    if (data.size() > max_size) {
        throw std::logic_error("Record has exceeded maximum capacity");
    }
    _data = std::vector<data_type>(data);
    std::make_heap(_data.begin(), _data.end());
}

Record::Record(key_type key, const std::vector<data_type>& data) : _data{}, _key{key} {
    if (data.size() > max_size) {
        throw std::logic_error("Record has exceeded maximum capacity");
    }
    _data = data;
    std::make_heap(_data.begin(), _data.end());
}

Record::Record(key_type key, std::vector<data_type>&& data) : _data{}, _key{key} {
    if (data.size() > max_size) {
        throw std::logic_error("Record has exceeded maximum capacity");
    }
    _data = std::move(data);
    std::make_heap(_data.begin(), _data.end());
}

std::string Record::to_string() const {
    std::ostringstream buffer{};
    buffer << "Record(key=" << _key << ", data={";
    if (!_data.empty()) {
        auto it = _data.cbegin();
        buffer << *it;
        it++;
        while (it != _data.cend()) {
            buffer << ", " << *it;
            it++;
        }
    }
    buffer << "})";
    return buffer.str();
}

char* Record::serialize(char* destination, std::size_t capacity) const {
    if (destination == nullptr) {
        throw std::logic_error("Destination cannot be null");
    }
    if (capacity < sizeof(_key) + sizeof(key_type) + sizeof(data_type) * max_size) {
        throw std::logic_error(
            "Capacity of destination cannot be smaller than serialized record size");
    }
    std::memcpy(destination, &_key, sizeof(_key));
    destination += sizeof(_key);
    size_type data_size = _data.size();
    std::memcpy(destination, &data_size, sizeof(data_size));
    destination += sizeof(data_size);
    for (const auto& datum : _data) {
        std::memcpy(destination, &datum, sizeof(datum));
        destination += sizeof(datum);
    }
    std::memset(destination, 0, (max_size - data_size) * sizeof(data_type));
    destination += (max_size - data_size) * sizeof(data_type);
    return destination;
}

std::pair<char*, std::optional<Record>> Record::deserialize(char* source) {
    if (source == nullptr) {
        return std::make_pair(nullptr, std::nullopt);
    }
    key_type key{};
    std::memcpy(&key, source, sizeof(key));
    source += sizeof(key);
    size_type data_size{};
    std::memcpy(&data_size, source, sizeof(data_size));
    source += sizeof(data_size);
    std::vector<data_type> data{};
    data.reserve(data_size);
    for (size_type i{}; i < data_size; i++) {
        data_type datum;
        std::memcpy(&datum, source, sizeof(datum));
        source += sizeof(datum);
        data.emplace_back(datum);
    }
    source += sizeof(data_type) * (max_size - data_size);
    return std::make_pair(source, Record(key, std::move(data)));
}

Record& Record::operator=(Record&& other) noexcept {
    this->_data = std::move(other._data);
    other._data.clear();
    this->_key = other._key;
    return *this;
}
