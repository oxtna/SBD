#pragma once
#include "paged_io.h"
#include "record.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

class Buffer
{
  public:
    static const std::size_t MaxSize = 4096;
    Buffer() = delete;
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = default;
    explicit Buffer(PagedIO&& pagedIO);
    explicit Buffer(const std::shared_ptr<PagedIO>& pagedIO);
    Buffer(PagedIO&& pagedInput, PagedIO&& pagedOutput);
    Buffer(const std::shared_ptr<PagedIO>& pagedInput, const std::shared_ptr<PagedIO>& pagedOutput);
    ~Buffer() = default;

    bool empty() const;
    bool full() const;
    std::optional<Record> getNext();
    std::optional<Record> peek();
    void put(Record&& record);
    void load();
    void save();
    void save(PagedIO& destination);
    void save(const std::shared_ptr<PagedIO>& destination);
    void sort();
    void clear();

    constexpr Buffer& operator=(const Buffer&) = delete;
    constexpr Buffer& operator=(Buffer&&) = default;

  private:
    std::vector<Record> _buffer;
    std::vector<Record>::iterator _iterator;
    std::shared_ptr<PagedIO> _pagedInput;
    std::shared_ptr<PagedIO> _pagedOutput;
};
