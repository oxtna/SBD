#include "record.h"
#include <algorithm>
#include <sstream>

Record::Record(Record&& other) noexcept : _data{std::move(other._data)} {
    other._data.clear();
}

Record::Record(std::initializer_list<std::int32_t> data) : _data{data} {
    std::make_heap(_data.begin(), _data.end());
}

Record::Record(const std::vector<std::int32_t>& data) : _data{data} {
    std::make_heap(_data.begin(), _data.end());
}

Record::Record(std::vector<std::int32_t>&& data) : _data{std::move(data)} {
    std::make_heap(_data.begin(), _data.end());
}

std::string Record::toString() {
    std::stringstream stream{};
    stream << "Record{";
    for (auto it = _data.begin(); it != _data.end(); it++) {
        if (it == --_data.end()) {
            stream << *it;
        }
        else {
            stream << *it << ", ";
        }
    }
    stream << "}";
    return stream.str();
}
