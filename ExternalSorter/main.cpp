#include "buffer.h"
#include "paged_io.h"
#include "record.h"
#include "sorter.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

static void setup(const std::string& filename) {
    std::fstream f1a{"1a", std::ios::out | std::ios::trunc};
    std::fstream f1b{"1b", std::ios::out | std::ios::trunc};
    std::fstream f2a{"2a", std::ios::out | std::ios::trunc};
    std::fstream f2b{"2b", std::ios::out | std::ios::trunc};
    std::fstream f3a{"3a", std::ios::out | std::ios::trunc};
    std::fstream f3b{"3b", std::ios::out | std::ios::trunc};
    std::fstream f4a{"4a", std::ios::out | std::ios::trunc};
    std::fstream f4b{"4b", std::ios::out | std::ios::trunc};
    std::fstream f5a{"5a", std::ios::out | std::ios::trunc};
    std::fstream f5b{"5b", std::ios::out | std::ios::trunc};
    std::fstream f6a{"6a", std::ios::out | std::ios::trunc};
    std::fstream f6b{"6b", std::ios::out | std::ios::trunc};
    std::fstream f7a{"7a", std::ios::out | std::ios::trunc};
    std::fstream f7b{"7b", std::ios::out | std::ios::trunc};
    std::fstream f8a{"8a", std::ios::out | std::ios::trunc};
    std::fstream f8b{"8b", std::ios::out | std::ios::trunc};
    std::fstream finput{filename, std::ios::out | std::ios::trunc};
    std::fstream foutput{"output", std::ios::out | std::ios::trunc};
}

static void generateInput(const std::string& filename, std::size_t size) {
    PagedIO input{filename};
    std::random_device seed;
    std::mt19937 generator{seed()};
    std::uniform_int_distribution<std::int32_t> distribution{
        0, std::numeric_limits<std::int32_t>::max()};
    std::vector<Record> vec;
    for (std::size_t i{}; i < size; i++) {
        std::int32_t recordSize = distribution(generator) % 16;
        if (recordSize < 0) {
            recordSize = -recordSize;
        }
        else if (recordSize == 0) {
            recordSize++;
        }
        std::vector<std::int32_t> set;
        set.reserve(recordSize);
        for (std::int32_t i{}; i < recordSize; i++) {
            set.emplace_back(distribution(generator));
        }
        vec.emplace_back(std::move(set));
        input.save({vec.begin(), 1});
        vec.pop_back();
    }
}

static void checkOrder(const char* filename) {
    Buffer output{PagedIO(filename)};
    std::size_t position{};
    for (auto previous = output.getNext(), current = output.getNext(); current.has_value();
         previous = current, current = output.getNext(), position++) {
        if (*previous > *current) {
            std::cerr << "Output file " << filename << " is not sorted at: " << position << '\n';
        }
    }
}

static void check() {
    checkOrder("output");
}

static void clean(const std::string& filename) {
    std::filesystem::remove("1a");
    std::filesystem::remove("1b");
    std::filesystem::remove("2a");
    std::filesystem::remove("2b");
    std::filesystem::remove("3a");
    std::filesystem::remove("3b");
    std::filesystem::remove("4a");
    std::filesystem::remove("4b");
    std::filesystem::remove("5a");
    std::filesystem::remove("5b");
    std::filesystem::remove("6a");
    std::filesystem::remove("6b");
    std::filesystem::remove("7a");
    std::filesystem::remove("7b");
    std::filesystem::remove("8a");
    std::filesystem::remove("8b");
    std::filesystem::remove("output");
    std::filesystem::remove(filename);
}

int main() {
    std::string filename = "input";
    bool printBetweenPhases = false;
    std::size_t count{};
    std::cout << std::boolalpha;
    setup(filename);
    while (true) {
        std::cout << "1. Generuj losowe rekordy\n";
        std::cout << "2. Wprowadz rekordy z klawiatury\n";
        std::cout << "3. Zaladuj rekordy z pliku (" << filename << ")\n";
        std::cout << "4. Wyswietl plik po kazdej fazie sortowania (" << printBetweenPhases << ")\n";
        std::cout << "5. Sortuj plik\n";
        std::cout << "6. Wyjscie\n";
        std::cout << "Wybierz opcje: ";
        int choice{};
        std::cin >> choice;
        if (choice == 1) {
            setup(filename);
            std::cout << "Podaj liczbe rekordow: ";
            std::cin >> count;
            generateInput(filename, count);
        }
        else if (choice == 2) {
            setup(filename);
            std::cout << "Podaj liczbe rekordow: ";
            std::cin >> count;
            std::vector<Record> records;
            records.reserve(count);
            for (std::size_t i{}; i < count; i++) {
                std::size_t setSize{};
                std::vector<std::int32_t> set;
                do {
                    std::cout << i + 1 << ") Podaj rozmiar zbioru: ";
                    std::cin >> setSize;
                    if (setSize == 0 || setSize > 15) {
                        std::cout
                            << "Rozmiar zbioru musi byc liczba dodatnia od 1 do 15 wlacznie.\n";
                    }
                } while (setSize == 0 || setSize > 15);
                set.reserve(setSize);
                for (std::size_t j{}; j < setSize; j++) {
                    std::cout << j + 1 << ") Wprowadz liczbe: ";
                    std::int32_t number{};
                    std::cin >> number;
                    set.emplace_back(number);
                }
                records.emplace_back(std::move(set));
            }
            auto input = PagedIO{filename};
            for (auto it = records.begin(); it != records.end(); it++) {
                input.save({it, 1});
            }
        }
        else if (choice == 3) {
            std::cout << "Podaj nazwe pliku: ";
            std::cin >> filename;
            setup(filename);
        }
        else if (choice == 4) {
            printBetweenPhases = !printBetweenPhases;
        }
        else if (choice == 5) {
            std::size_t reads{};
            std::size_t writes{};
            std::size_t phases{};
            std::cout << "====PRZED====\n";
            {
                Buffer buffer{PagedIO(filename)};
                for (auto record = buffer.getNext(); record.has_value();
                     record = buffer.getNext()) {
                    std::cout << record->toString() << '\n';
                }
            }
            {
                Sorter sorter{printBetweenPhases};
                sorter.sort(filename, "output");
                reads = sorter.reads();
                writes = sorter.writes();
                phases = sorter.phases();
            }
            std::cout << "====PO====\n";
            {
                Buffer buffer{PagedIO("output")};
                for (auto record = buffer.getNext(); record.has_value();
                     record = buffer.getNext()) {
                    std::cout << record->toString() << '\n';
                }
            }
            std::cout << "====KONIEC====\n";
            std::cout << "Liczba faz sortowania: " << phases << '\n';
            std::cout << "Liczba odczytow: " << reads << '\n';
            std::cout << "Liczba zapisow: " << writes << '\n';
            std::cout << "Laczna liczba operacji: " << reads + writes << '\n';
            auto ratio = static_cast<double>(count) / PagedIO::PageSize;
            std::cout << "Oczekiwana licza operacji: "
                      << static_cast<std::size_t>(
                             2 * std::ceil(ratio / std::log(Sorter::BufferCount)) *
                             std::ceil(std::log(ratio)))
                      << '\n';
            std::cout << "========\n";
        }
        else if (choice == 6) {
            break;
        }
        else {
            std::cout << "Nieprawidlowa opcja. Sprobuj ponownie.\n";
        }
    }
    clean(filename);
    return 0;
}
