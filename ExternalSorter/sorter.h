#pragma once
#include <cstddef>
#include <string>

class Sorter
{

  public:
    static const std::size_t BufferCount = 8;
    Sorter() = default;
    Sorter(const Sorter&) = delete;
    Sorter(Sorter&&) = delete;
    explicit Sorter(bool printing) : _printing{printing} {}
    ~Sorter() = default;

    void sort(const std::string& inputFilename, const std::string& outputFilename);

    constexpr std::size_t reads() const { return _totalReads; }
    constexpr std::size_t writes() const { return _totalWrites; }
    constexpr std::size_t phases() const { return _phaseCount; }

    constexpr Sorter& operator=(const Sorter&) = delete;
    constexpr Sorter& operator=(Sorter&&) = delete;

  private:
    std::size_t _totalReads = 0;
    std::size_t _totalWrites = 0;
    std::size_t _phaseCount = 0;
    bool _printing = false;

    void createInitialRuns(const std::string& filename);
    void merge();
};
