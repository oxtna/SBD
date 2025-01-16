#pragma once
#include "record.h"
#include <cstddef>
#include <fstream>
#include <span>
#include <string>
#include <vector>

class PagedIO
{
  public:
    static const std::size_t PageSize = 4096;
    PagedIO() = delete;
    PagedIO(const PagedIO&) = delete;
    PagedIO(PagedIO&&) = default;
    explicit PagedIO(const std::string& filename);
    explicit PagedIO(const char* filename);
    ~PagedIO() = default;

    constexpr std::size_t reads() const { return _readCount; }
    constexpr std::size_t writes() const { return _writeCount; }

    std::vector<Record> loadNextPage();
    void save(std::span<const Record> records);
    void reset();
    void hardReset();
    bool finished();

    constexpr PagedIO& operator=(const PagedIO&) = delete;
    constexpr PagedIO& operator=(PagedIO&&) = default;

  private:
    std::fstream _file;
    std::string _filename;
    std::size_t _readCount;
    std::size_t _writeCount;
};
