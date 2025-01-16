#include "Buffer.h"
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

Buffer::Buffer(PagedIO&& pagedIO)
    : _buffer{},
      _iterator{},
      _pagedInput{std::make_shared<PagedIO>(std::move(pagedIO))},
      _pagedOutput{_pagedInput} {
    if (PagedIO::PageSize > MaxSize) {
        throw std::logic_error("Page size cannot be larger than buffer size");
    }
    _buffer.reserve(MaxSize);
    _iterator = _buffer.begin();
}

Buffer::Buffer(const std::shared_ptr<PagedIO>& pagedIO)
    : _buffer{}, _iterator{}, _pagedInput(pagedIO), _pagedOutput(pagedIO) {
    if (PagedIO::PageSize > MaxSize) {
        throw std::logic_error("Page size cannot be larger than buffer size");
    }
    _buffer.reserve(MaxSize);
    _iterator = _buffer.begin();
}

Buffer::Buffer(PagedIO&& pagedInput, PagedIO&& pagedOutput)
    : _buffer{},
      _iterator{},
      _pagedInput{std::make_shared<PagedIO>(std::move(pagedInput))},
      _pagedOutput{std::make_shared<PagedIO>(std::move(pagedOutput))} {
    if (PagedIO::PageSize > MaxSize) {
        throw std::logic_error("Page size cannot be larger than buffer size");
    }
    _buffer.reserve(MaxSize);
    _iterator = _buffer.begin();
}

Buffer::Buffer(
    const std::shared_ptr<PagedIO>& pagedInput, const std::shared_ptr<PagedIO>& pagedOutput)
    : _buffer{}, _iterator{}, _pagedInput(pagedInput), _pagedOutput(pagedOutput) {
    if (PagedIO::PageSize > MaxSize) {
        throw std::logic_error("Page size cannot be larger than buffer size");
    }
    _buffer.reserve(MaxSize);
    _iterator = _buffer.begin();
}

bool Buffer::empty() const {
    return _buffer.empty();
}

bool Buffer::full() const {
    return _buffer.size() == MaxSize;
}

std::optional<Record> Buffer::getNext() {
    if (_iterator == _buffer.end()) {
        load();
    }
    if (_iterator == _buffer.end()) {
        return std::nullopt;
    }
    return std::move(*_iterator++);
}

std::optional<Record> Buffer::peek() {
    if (_iterator == _buffer.end()) {
        load();
    }
    if (_iterator == _buffer.end()) {
        return std::nullopt;
    }
    return *_iterator;
}

void Buffer::put(Record&& record) {
    _buffer.emplace_back(std::move(record));
    _iterator = _buffer.begin();
}

void Buffer::load() {
    _buffer.clear();
    do {
        if (_pagedInput->finished()) {
            break;
        }
        auto page = _pagedInput->loadNextPage();
        for (auto&& record : page) {
            _buffer.emplace_back(std::move(record));
        }
    } while (MaxSize >= PagedIO::PageSize + _buffer.size());
    _iterator = _buffer.begin();
}

void Buffer::save() {
    if (empty()) {
        return;
    }
    while (std::distance(_iterator, _buffer.end()) > PagedIO::PageSize) {
        _pagedOutput->save({_iterator, PagedIO::PageSize});
        _iterator += PagedIO::PageSize;
    }
    _pagedOutput->save({
        _iterator,
        static_cast<std::size_t>(std::distance(_iterator, _buffer.end())),
    });
    clear();
}

void Buffer::save(PagedIO& destination) {
    if (empty()) {
        return;
    }
    while (std::distance(_iterator, _buffer.end()) > PagedIO::PageSize) {
        destination.save({_iterator, PagedIO::PageSize});
        _iterator += PagedIO::PageSize;
    }
    destination.save({
        _iterator,
        static_cast<std::size_t>(std::distance(_iterator, _buffer.end())),
    });
    clear();
}

void Buffer::save(const std::shared_ptr<PagedIO>& destination) {
    if (empty()) {
        return;
    }
    while (std::distance(_iterator, _buffer.end()) > PagedIO::PageSize) {
        destination->save({_iterator, PagedIO::PageSize});
        _iterator += PagedIO::PageSize;
    }
    destination->save({
        _iterator,
        static_cast<std::size_t>(std::distance(_iterator, _buffer.end())),
    });
    clear();
}

void Buffer::sort() {
    for (auto fromBegin = _buffer.begin(), fromEnd = _buffer.end();
         fromBegin != fromEnd && *fromBegin == Record();
         fromBegin++) {
        fromEnd--;
        std::swap(*fromBegin, *fromEnd);
        _buffer.pop_back();
    }
    std::sort(_buffer.begin(), _buffer.end());
    _iterator = _buffer.begin();
}

void Buffer::clear() {
    _buffer.clear();
    _iterator = _buffer.begin();
}
