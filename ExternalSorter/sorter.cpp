#include "sorter.h"
#include "buffer.h"
#include "paged_io.h"
#include "record.h"
#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>

struct MinHeapComparator
{
    constexpr bool operator()(
        const std::pair<Record, std::size_t>& lhs,
        const std::pair<Record, std::size_t>& rhs) const {
        return std::greater<Record>()(lhs.first, rhs.first);
    }
};

void Sorter::sort(const std::string& inputFilename, const std::string& outputFilename) {
    createInitialRuns(inputFilename);
    merge();
    std::string resultTape = _phaseCount % 2 == 0 ? "1a" : "1b";
    Buffer transport{PagedIO(resultTape), PagedIO(outputFilename)};
    do {
        transport.load();
        if (transport.empty()) {
            break;
        }
        transport.save();
    } while (true);
}

void Sorter::createInitialRuns(const std::string& filename) {
    Buffer input{PagedIO(filename)};
    std::array<PagedIO, BufferCount> tapes = {
        PagedIO("1a"),
        PagedIO("2a"),
        PagedIO("3a"),
        PagedIO("4a"),
        PagedIO("5a"),
        PagedIO("6a"),
        PagedIO("7a"),
        PagedIO("8a"),
    };
    for (std::size_t i{};; i = (i + 1) % BufferCount) {
        input.load();
        if (input.empty()) {
            break;
        }
        input.sort();
        input.save(tapes[i]);
    }
    for (auto& tape : tapes) {
        _totalReads += tape.reads();
        _totalWrites += tape.writes();
    }
}

static void mergeOnce(
    std::array<Buffer, Sorter::BufferCount>& buffers, const std::shared_ptr<PagedIO>& destination) {
    std::priority_queue<
        std::pair<Record, std::size_t>,
        std::vector<std::pair<Record, std::size_t>>,
        MinHeapComparator>
        minHeap{};
    Buffer outputBuffer{destination};
    for (std::size_t i{}; i < Sorter::BufferCount; i++) {
        auto record = buffers[i].getNext();
        if (record.has_value()) {
            minHeap.emplace(std::make_pair(std::move(*record), i));
        }
    }
    while (!minHeap.empty()) {
        auto [record, source] = minHeap.top();
        minHeap.pop();
        auto nextRecord = buffers[source].peek();
        if (nextRecord.has_value() && *nextRecord >= record) {
            minHeap.emplace(std::make_pair(std::move(*buffers[source].getNext()), source));
        }
        outputBuffer.put(std::move(record));
        if (outputBuffer.full()) {
            outputBuffer.save(destination);
        }
    }
    if (!outputBuffer.empty()) {
        outputBuffer.save(destination);
    }
}

static void printTape(const std::shared_ptr<PagedIO>& tape) {
    tape->reset();
    Buffer buffer{tape};
    for (auto record = buffer.getNext(); record.has_value(); record = buffer.getNext()) {
        std::cout << record->toString() << '\n';
    }
    tape->reset();
}

void Sorter::merge() {
    std::array<std::shared_ptr<PagedIO>, BufferCount> sources = {
        std::make_shared<PagedIO>(PagedIO("1a")),
        std::make_shared<PagedIO>(PagedIO("2a")),
        std::make_shared<PagedIO>(PagedIO("3a")),
        std::make_shared<PagedIO>(PagedIO("4a")),
        std::make_shared<PagedIO>(PagedIO("5a")),
        std::make_shared<PagedIO>(PagedIO("6a")),
        std::make_shared<PagedIO>(PagedIO("7a")),
        std::make_shared<PagedIO>(PagedIO("8a")),
    };
    std::array<std::shared_ptr<PagedIO>, BufferCount> destinations = {
        std::make_shared<PagedIO>(PagedIO("1b")),
        std::make_shared<PagedIO>(PagedIO("2b")),
        std::make_shared<PagedIO>(PagedIO("3b")),
        std::make_shared<PagedIO>(PagedIO("4b")),
        std::make_shared<PagedIO>(PagedIO("5b")),
        std::make_shared<PagedIO>(PagedIO("6b")),
        std::make_shared<PagedIO>(PagedIO("7b")),
        std::make_shared<PagedIO>(PagedIO("8b")),
    };
    std::array<Buffer, BufferCount> buffers = {
        Buffer(sources[0]),
        Buffer(sources[1]),
        Buffer(sources[2]),
        Buffer(sources[3]),
        Buffer(sources[4]),
        Buffer(sources[5]),
        Buffer(sources[6]),
        Buffer(sources[7]),
    };
    auto sorting = [](std::array<Buffer, BufferCount>& buffers) {
        std::size_t nonEmptyCount{};
        for (auto& buffer : buffers) {
            if (buffer.peek().has_value()) {
                nonEmptyCount++;
            }
        }
        return nonEmptyCount > 1;
    };
    do {
        auto finished = [](std::array<Buffer, BufferCount>& buffers) {
            for (auto& buffer : buffers) {
                if (buffer.peek().has_value()) {
                    return false;
                }
            }
            return true;
        };
        do {
            for (auto& destination : destinations) {
                mergeOnce(buffers, destination);
            }
        } while (!finished(buffers));
        for (auto& source : sources) {
            _totalReads += source->reads();
            _totalWrites += source->writes();
        }
        for (auto& destination : destinations) {
            _totalReads += destination->reads();
            _totalWrites += destination->writes();
        }
        for (std::size_t i{}; i < buffers.size(); i++) {
            sources[i]->hardReset();
            destinations[i]->reset();
            std::swap(sources[i], destinations[i]);
            buffers[i] = Buffer(sources[i]);
        }
        if (_printing) {
            std::cout << "====FAZA " << _phaseCount + 1 << "====\n";
            for (const auto& tape : sources) {
                printTape(tape);
            }
        }
        _phaseCount++;
    } while (sorting(buffers));
}
