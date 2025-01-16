#include "paged_io.h"
#include <limits>
#include <stdexcept>
#include <utility>

PagedIO::PagedIO(const std::string& filename)
    : _file(filename, std::ios::in | std::ios::out | std::ios::binary),
      _filename{filename},
      _readCount{},
      _writeCount{} {
    if (!_file.is_open()) {
        throw std::runtime_error("File could not be opened");
    }
}

PagedIO::PagedIO(const char* filename)
    : _file(filename, std::ios::in | std::ios::out | std::ios::binary),
      _filename{filename},
      _readCount{},
      _writeCount{} {
    if (!_file.is_open()) {
        throw std::runtime_error("File could not be opened");
    }
}

std::vector<Record> PagedIO::loadNextPage() {
    std::vector<Record> page;
    page.reserve(PageSize);
    for (std::size_t i{}; i < PageSize; i++) {
        std::size_t header{};
        _file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (_file.eof()) {
            break;
        }
        if (header == 0 || header == std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("Header was equal to an invalid value");
        }
        std::vector<std::int32_t> vec;
        vec.reserve(header);
        for (std::size_t j{}; j < header; j++) {
            std::int32_t buffer{};
            _file.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
            vec.emplace_back(buffer);
        }
        page.emplace_back(std::move(vec));
    }
    if (page.size() > 0) {
        _readCount++;
    }
    return page;
}

void PagedIO::save(std::span<const Record> records) {
    if (records.size() == 0) {
        return;
    }
    if (records.size() > PageSize) {
        throw std::logic_error("Span is bigger than page size");
    }
    for (auto& record : records) {
        std::size_t header = record.size();
        if (header == 0) {
            continue;
        }
        if (header == std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("Record has invalid size");
        }
        _file.write(reinterpret_cast<char*>(&header), sizeof(header));
        for (auto n : record.data()) {
            _file.write(reinterpret_cast<char*>(&n), sizeof(n));
        }
    }
    _file.flush();
    _writeCount++;
}

void PagedIO::reset() {
    _file.clear();
    if (_file.is_open()) {
        _file.close();
    }
    _file.open(_filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!_file.is_open()) {
        throw std::runtime_error("File could not be reopened");
    }
    _readCount = 0;
    _writeCount = 0;
}

void PagedIO::hardReset() {
    _file.clear();
    if (_file.is_open()) {
        _file.close();
    }
    _file.open(_filename, std::ios::out | std::ios::trunc);
    if (!_file.is_open()) {
        throw std::runtime_error("File could not be reopened");
    }
    _file.close();
    _file.open(_filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!_file.is_open()) {
        throw std::runtime_error("File could not be reopened");
    }
    _readCount = 0;
    _writeCount = 0;
}

bool PagedIO::finished() {
    _file.peek();
    return _file.eof();
}
