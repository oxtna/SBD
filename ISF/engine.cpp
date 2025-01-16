#include "engine.h"
#include "record.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

void Engine::execute(const Statement* statement) {
    execute_raw(statement);
    std::fstream overflow_file("overflow.bin", std::ios::binary | std::ios::in);
    overflow_file.seekg(0, std::ios::end);
    auto overflow_size = overflow_file.tellg();
    overflow_file.close();
    if (overflow_size >= 5376) {
        _output << "Overflow is too big. Sorting all data.\n";
        auto reads_before = total_reads();
        auto writes_before = total_writes();
        sort();
        overflow_file.open("overflow.bin", std::ios::binary | std::ios::out | std::ios::trunc);
        overflow_file.flush();
        overflow_file.close();
        merge();
        std::ofstream index_file("index.bin", std::ios::binary | std::ios::out | std::ios::trunc);
        index_file.flush();
        index_file.close();
        rebuild_index();
        _output << "Reads during sorting: " << total_reads() - reads_before << '\n';
        _output << "Writes during sorting: " << total_writes() - writes_before << '\n';
    }
}

void Engine::sort() {
    const std::int64_t overflow_offset_null = -1;
    std::fstream overflow_file("overflow.bin", std::ios::binary | std::ios::in);
    std::array<char, 336> overflow_buffer{};
    std::vector<Record> records;
    records.reserve(4);
    int chunk_index = 0;
    std::ostringstream chunk_name;
    while (overflow_file) {
        records.clear();
        chunk_name.clear();
        chunk_name.str("");
        chunk_name << "chunk_" << chunk_index << ".bin";
        overflow_file.read(overflow_buffer.data(), overflow_buffer.size());
        auto bytes_read = overflow_file.gcount();
        if (bytes_read == 0) {
            return;
        }
        _overflow_reads++;
        char* pos = overflow_buffer.data();
        while (pos < overflow_buffer.data() + bytes_read) {
            auto [_, record] = Record::deserialize(pos);
            if (!record.has_value()) {
                throw std::runtime_error("Overflow file has been corrupted");
            }
            records.emplace_back(std::move(*record));
            pos += Record::total_size_in_bytes;
            pos += sizeof(overflow_offset_null);
        }
        std::sort(records.begin(), records.end());
        pos = overflow_buffer.data();
        for (const auto& record : records) {
            pos = record.serialize(pos, overflow_buffer.data() - pos + overflow_buffer.size());
            std::memcpy(pos, &overflow_offset_null, sizeof(overflow_offset_null));
            pos += sizeof(overflow_offset_null);
        }
        std::fstream chunk_file(
            chunk_name.str(), std::ios::binary | std::ios::out | std::ios::trunc);
        chunk_file.write(overflow_buffer.data(), bytes_read);
        chunk_file.flush();
        chunk_index++;
        _overflow_writes++;
    }
    int chunk_count = chunk_index;
    std::ostringstream first_input_chunk_name;
    std::array<char, 336> first_buffer{};
    std::vector<Record> first_vector;
    first_vector.reserve(4);
    std::ostringstream second_input_chunk_name;
    std::array<char, 336> second_buffer{};
    std::vector<Record> second_vector;
    second_vector.reserve(4);
    std::ostringstream output_chunk_name;
    std::array<char, 336> output_buffer{};
    std::vector<Record> output_vector;
    output_vector.reserve(4);
    while (chunk_count > 1) {
        for (int i = 0; i < chunk_count / 2; i++) {
            first_vector.clear();
            second_vector.clear();
            output_vector.clear();
            first_input_chunk_name.clear();
            first_input_chunk_name.str("");
            first_input_chunk_name << "chunk_" << i << ".bin";
            std::fstream first_input_chunk_file(
                first_input_chunk_name.str(), std::ios::binary | std::ios::in);
            second_input_chunk_name.clear();
            second_input_chunk_name.str("");
            second_input_chunk_name << "chunk_" << i + chunk_count / 2 << ".bin";
            std::fstream second_input_chunk_file(
                second_input_chunk_name.str(), std::ios::binary | std::ios::in);
            first_input_chunk_file.read(first_buffer.data(), first_buffer.size());
            auto bytes_read_from_first = first_input_chunk_file.gcount();
            second_input_chunk_file.read(second_buffer.data(), second_buffer.size());
            auto bytes_read_from_second = second_input_chunk_file.gcount();
            while (bytes_read_from_first > 0) {
                _overflow_reads++;
                for (char* pos = first_buffer.data();
                     pos < first_buffer.data() + bytes_read_from_first;
                     pos += Record::total_size_in_bytes + sizeof(overflow_offset_null)) {
                    auto [_, record] = Record::deserialize(pos);
                    if (!record.has_value()) {
                        throw std::runtime_error("Overflow file has been corrupted");
                    }
                    first_vector.emplace_back(std::move(*record));
                }
                first_input_chunk_file.read(first_buffer.data(), first_buffer.size());
                bytes_read_from_first = first_input_chunk_file.gcount();
            }
            first_input_chunk_file.close();
            while (bytes_read_from_second > 0) {
                _overflow_reads++;
                for (char* pos = second_buffer.data();
                     pos < second_buffer.data() + bytes_read_from_second;
                     pos += Record::total_size_in_bytes + sizeof(overflow_offset_null)) {
                    auto [_, record] = Record::deserialize(pos);
                    if (!record.has_value()) {
                        throw std::runtime_error("Overflow file has been corrupted");
                    }
                    second_vector.emplace_back(std::move(*record));
                }
                second_input_chunk_file.read(second_buffer.data(), second_buffer.size());
                bytes_read_from_second = second_input_chunk_file.gcount();
            }
            second_input_chunk_file.close();
            auto first_it = first_vector.begin();
            auto second_it = second_vector.begin();
            while (first_it != first_vector.end() && second_it != second_vector.end()) {
                if (*first_it < *second_it) {
                    output_vector.emplace_back(std::move(*first_it));
                    first_it++;
                }
                else {
                    output_vector.emplace_back(std::move(*second_it));
                    second_it++;
                }
            }
            while (first_it != first_vector.end()) {
                output_vector.emplace_back(std::move(*first_it));
                first_it++;
            }
            while (second_it != second_vector.end()) {
                output_vector.emplace_back(std::move(*second_it));
                second_it++;
            }
            std::fstream output_chunk_file(
                first_input_chunk_name.str(), std::ios::binary | std::ios::out | std::ios::trunc);
            char* pos = output_buffer.data();
            for (const auto& record : output_vector) {
                if (pos == output_buffer.data() + output_buffer.size()) {
                    output_chunk_file.write(output_buffer.data(), output_buffer.size());
                    output_chunk_file.flush();
                    _overflow_writes++;
                    pos = output_buffer.data();
                }
                pos = record.serialize(pos, output_buffer.size());
                std::memcpy(pos, &overflow_offset_null, sizeof(overflow_offset_null));
                pos += sizeof(overflow_offset_null);
            }
            output_chunk_file.write(output_buffer.data(), pos - output_buffer.data());
            output_chunk_file.flush();
            _overflow_writes++;
        }
        if (chunk_count % 2 == 1) {
            output_chunk_name.clear();
            output_chunk_name.str("");
            output_chunk_name << "chunk_" << chunk_count - 1 << ".bin";
            std::fstream in_chunk_file(output_chunk_name.str(), std::ios::binary | std::ios::in);
            output_chunk_name.clear();
            output_chunk_name.str("");
            output_chunk_name << "chunk_" << chunk_count / 2 << ".bin";
            std::fstream out_chunk_file(
                output_chunk_name.str(), std::ios::binary | std::ios::out | std::ios::trunc);
            while (in_chunk_file) {
                in_chunk_file.read(output_buffer.data(), output_buffer.size());
                auto bytes_read = in_chunk_file.gcount();
                if (bytes_read == 0) {
                    break;
                }
                out_chunk_file.write(output_buffer.data(), bytes_read);
                out_chunk_file.flush();
                _overflow_reads++;
                _overflow_writes++;
            }
            chunk_count = chunk_count / 2;
            chunk_count++;
        }
        else {
            chunk_count = chunk_count / 2;
        }
    }
}

void Engine::merge() {
    const std::int64_t overflow_offset_null = -1;
    std::fstream chunk_file("chunk_0.bin", std::ios::binary | std::ios::in);
    std::array<char, 336> buffer{};
    std::vector<Record> chunk_vector;
    chunk_vector.reserve(4);
    chunk_file.read(buffer.data(), buffer.size());
    auto bytes_read_from_chunk = chunk_file.gcount();
    while (bytes_read_from_chunk > 0) {
        _overflow_reads++;
        for (char* pos = buffer.data(); pos < buffer.data() + bytes_read_from_chunk;
             pos += Record::total_size_in_bytes + sizeof(overflow_offset_null)) {
            auto [_, record] = Record::deserialize(pos);
            if (!record.has_value()) {
                throw std::runtime_error("Overflow file has been corrupted");
            }
            chunk_vector.emplace_back(std::move(*record));
        }
        chunk_file.read(buffer.data(), buffer.size());
        bytes_read_from_chunk = chunk_file.gcount();
    }
    chunk_file.close();
    std::fstream data_file("data.bin", std::ios::binary | std::ios::in);
    std::vector<Record> data_vector;
    data_vector.reserve(4);
    data_file.read(buffer.data(), buffer.size());
    auto bytes_read_from_data = data_file.gcount();
    while (bytes_read_from_data > 0) {
        _data_reads++;
        for (char* pos = buffer.data(); pos < buffer.data() + bytes_read_from_data;
             pos += Record::total_size_in_bytes + sizeof(overflow_offset_null)) {
            auto [_, record] = Record::deserialize(pos);
            if (!record.has_value()) {
                throw std::runtime_error("Data file has been corrupted");
            }
            data_vector.emplace_back(std::move(*record));
        }
        data_file.read(buffer.data(), buffer.size());
        bytes_read_from_data = data_file.gcount();
    }
    data_file.close();
    std::vector<Record> output_vector;
    output_vector.reserve(4);
    auto chunk_it = chunk_vector.begin();
    auto data_it = data_vector.begin();
    while (chunk_it != chunk_vector.end() && data_it != data_vector.end()) {
        if (*chunk_it < *data_it) {
            output_vector.emplace_back(std::move(*chunk_it));
            chunk_it++;
        }
        else {
            output_vector.emplace_back(std::move(*data_it));
            data_it++;
        }
    }
    while (chunk_it != chunk_vector.end()) {
        output_vector.emplace_back(std::move(*chunk_it));
        chunk_it++;
    }
    while (data_it != data_vector.end()) {
        output_vector.emplace_back(std::move(*data_it));
        data_it++;
    }
    data_file.open("data.bin", std::ios::binary | std::ios::out | std::ios::trunc);
    char* pos = buffer.data();
    for (const auto& record : output_vector) {
        if (pos == buffer.data() + buffer.size()) {
            data_file.write(buffer.data(), buffer.size());
            data_file.flush();
            _data_writes++;
            pos = buffer.data();
        }
        pos = record.serialize(pos, buffer.size());
        std::memcpy(pos, &overflow_offset_null, sizeof(overflow_offset_null));
        pos += sizeof(overflow_offset_null);
    }
    data_file.write(buffer.data(), pos - buffer.data());
    data_file.flush();
    data_file.close();
    _data_writes++;
}

void Engine::rebuild_index() {
    std::fstream data_file("data.bin", std::ios::binary | std::ios::in);
    std::array<char, 336> data_buffer{};
    std::array<char, 336> index_buffer{};
    char* index_pos = index_buffer.data();
    bool updated_index_buffer = false;
    std::uint64_t page_index{};
    while (data_file) {
        data_file.read(data_buffer.data(), data_buffer.size());
        auto bytes_read = data_file.gcount();
        if (bytes_read == 0) {
            break;
        }
        _data_reads++;
        std::memcpy(index_pos, data_buffer.data(), sizeof(Record::key_type));
        index_pos += sizeof(Record::key_type);
        std::memcpy(index_pos, &page_index, sizeof(page_index));
        index_pos += sizeof(page_index);
        updated_index_buffer = true;
        page_index++;
        if (index_pos == index_buffer.data() + index_buffer.size()) {
            std::fstream index_file("index.bin", std::ios::binary | std::ios::out);
            index_file.seekp(0, std::ios::end);
            index_file.write(index_buffer.data(), index_buffer.size());
            index_file.flush();
            index_pos = index_buffer.data();
            updated_index_buffer = false;
            _index_writes++;
        }
    }
    if (updated_index_buffer) {
        std::fstream index_file("index.bin", std::ios::binary | std::ios::out);
        index_file.seekp(0, std::ios::end);
        index_file.write(index_buffer.data(), index_pos - index_buffer.data());
        index_file.flush();
        _index_writes++;
    }
}

void Engine::execute_raw(const Statement* statement) {
    constexpr std::int64_t overflow_offset_null = -1;
    switch (statement->type) {
    case StatementType::Empty:
        return;
    case StatementType::Show: {
        auto show_statement = static_cast<const ShowStatement*>(statement);
        if (show_statement->key == 0) {
            throw KeyError("Key cannot be equal to 0");
        }
        auto reads_before = total_reads();
        auto writes_before = total_writes();
        std::array<char, 336> buffer{};
        std::ifstream index_file("index.bin", std::ios::binary | std::ios::ate);
        if (!index_file) {
            throw std::runtime_error("Could not open file");
        }
        if (index_file.tellg() == 0) {
            throw NotInitializedError("Indexed sequential file was not initialized");
        }
        index_file.seekg(0);
        std::uint64_t page_index{};
        bool exists = false;
        bool found_page = false;
        while (!found_page) {
            _index_reads++;
            index_file.read(buffer.data(), buffer.size());
            auto bytes_read = index_file.gcount();
            if (index_file.bad()) {
                throw std::runtime_error("Fatal error encountered during reading");
            }
            if (bytes_read > 0) {
                for (char* current_pos = buffer.data();
                     current_pos + 16 <= buffer.data() + bytes_read;) {
                    Record::key_type next_key{};
                    std::memcpy(&next_key, current_pos, sizeof(next_key));
                    current_pos += sizeof(next_key);
                    if (next_key > show_statement->key) {
                        found_page = true;
                        break;
                    }
                    std::memcpy(&page_index, current_pos, sizeof(page_index));
                    current_pos += sizeof(page_index);
                    if (next_key == show_statement->key) {
                        exists = true;
                        found_page = true;
                        break;
                    }
                }
            }
            if (index_file.eof()) {
                found_page = true;
            }
        }
        index_file.close();
        std::optional<Record> record = std::nullopt;
        bool in_data = true;
        std::int64_t offset = 0;
        std::int64_t next_offset = overflow_offset_null;
        std::ifstream data_file("data.bin");
        _data_reads++;
        data_file.seekg(page_index * buffer.size());
        data_file.read(buffer.data(), buffer.size());
        auto bytes_read = data_file.gcount();
        if (exists) {
            if (bytes_read < Record::total_size_in_bytes + sizeof(overflow_offset_null)) {
                throw std::runtime_error("Indexed sequential file has been corrupted");
            }
            std::memcpy(
                &next_offset, buffer.data() + Record::total_size_in_bytes, sizeof(next_offset));
            record = std::move(Record::deserialize(buffer.data()).second);
        }
        else {
            std::int64_t overflow_offset = overflow_offset_null;
            for (char* current_pos = buffer.data();
                 current_pos + Record::total_size_in_bytes + sizeof(overflow_offset) <=
                 buffer.data() + bytes_read;) {
                Record::key_type next_key{};
                std::memcpy(&next_key, current_pos, sizeof(next_key));
                if (next_key == show_statement->key) {
                    offset = current_pos - buffer.data();
                    std::memcpy(
                        &next_offset,
                        current_pos + Record::total_size_in_bytes,
                        sizeof(next_offset));
                    record = std::move(Record::deserialize(current_pos).second);
                    break;
                }
                current_pos += Record::total_size_in_bytes;
                if (next_key > show_statement->key) {
                    in_data = false;
                    std::ifstream overflow_file("overflow.bin");
                    std::array<char, Record::total_size_in_bytes> overflow_buffer{};
                    while (overflow_offset != overflow_offset_null) {
                        _overflow_reads++;
                        offset = overflow_offset;
                        overflow_file.seekg(overflow_offset);
                        overflow_file.read(overflow_buffer.data(), overflow_buffer.size());
                        overflow_file.read(
                            reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                        auto next_key =
                            *reinterpret_cast<Record::key_type*>(overflow_buffer.data());
                        if (next_key == show_statement->key) {
                            next_offset = overflow_offset;
                            record = std::move(Record::deserialize(overflow_buffer.data()).second);
                            break;
                        }
                        if (next_key > show_statement->key) {
                            throw KeyError("No record with given key exists");
                        }
                    }
                }
                std::memcpy(&overflow_offset, current_pos, sizeof(overflow_offset));
                current_pos += sizeof(overflow_offset);
            }
        }
        if (record.has_value()) {
            _output << record->to_string();
            if (in_data) {
                _output << " (Data: [" << page_index << ":" << offset << "] -> [" << next_offset
                        << "])\n";
            }
            else {
                _output << " (Overflow: [" << offset << "] -> [" << next_offset << "])\n";
            }
        }
        else {
            throw KeyError("No record with given key exists");
        }
        _output << "Reads: " << total_reads() - reads_before << '\n';
        _output << "Writes: " << total_writes() - writes_before << '\n';
        break;
    }
    case StatementType::Insert: {
        auto insert_statement = static_cast<const InsertStatement*>(statement);
        if (insert_statement->key == 0) {
            throw KeyError("Key cannot be equal to 0");
        }
        auto reads_before = total_reads();
        auto writes_before = total_writes();
        std::array<char, 336> buffer{};
        std::ifstream index_file("index.bin", std::ios::binary | std::ios::ate);
        if (!index_file) {
            throw std::runtime_error("Could not open file");
        }
        if (index_file.tellg() == 0) {
            index_file.close();
            std::ofstream index_file_init("index.bin", std::ios::binary);
            _index_writes++;
            index_file_init.write(
                reinterpret_cast<const char*>(&insert_statement->key),
                sizeof(insert_statement->key));
            std::uint64_t page_index{};
            index_file_init.write(reinterpret_cast<char*>(&page_index), sizeof(page_index));
            index_file_init.flush();
            std::ofstream data_file_init("data.bin", std::ios::binary);
            std::array<char, Record::total_size_in_bytes> record_buffer{};
            Record(insert_statement->key, insert_statement->data)
                .serialize(record_buffer.data(), record_buffer.size());
            _data_writes++;
            data_file_init.write(record_buffer.data(), record_buffer.size());
            data_file_init.write(
                reinterpret_cast<const char*>(&overflow_offset_null), sizeof(overflow_offset_null));
            data_file_init.flush();
            _output << "Reads: " << total_reads() - reads_before << '\n';
            _output << "Writes: " << total_writes() - writes_before << '\n';
            return;
        }
        index_file.seekg(0);
        std::uint64_t page_index{};
        bool exists = false;
        bool found_page = false;
        bool first = true;
        while (!found_page) {
            _index_reads++;
            index_file.read(buffer.data(), buffer.size());
            auto bytes_read = index_file.gcount();
            if (index_file.bad()) {
                throw std::runtime_error("Fatal error encountered during reading");
            }
            if (bytes_read > 0) {
                for (char* current_pos = buffer.data();
                     current_pos + 16 <= buffer.data() + bytes_read;) {
                    Record::key_type next_key{};
                    std::memcpy(&next_key, current_pos, sizeof(next_key));
                    current_pos += sizeof(next_key);
                    if (next_key > insert_statement->key) {
                        found_page = true;
                        break;
                    }
                    std::memcpy(&page_index, current_pos, sizeof(page_index));
                    current_pos += sizeof(page_index);
                    if (next_key == insert_statement->key) {
                        exists = true;
                        found_page = true;
                        break;
                    }
                    first = false;
                }
            }
            if (index_file.eof()) {
                found_page = true;
            }
        }
        index_file.close();
        if (exists) {
            throw KeyError("Key already exists");
        }
        std::array<char, Record::total_size_in_bytes> serialized_record{};
        Record(insert_statement->key, insert_statement->data)
            .serialize(serialized_record.data(), serialized_record.size());
        auto overflow_offset = overflow_offset_null;
        std::fstream data_file("data.bin", std::ios::binary | std::ios::in | std::ios::out);
        data_file.seekg(page_index * buffer.size());
        data_file.read(buffer.data(), buffer.size());
        _data_reads++;
        auto bytes_read = data_file.gcount();
        if (data_file.bad()) {
            throw std::runtime_error("Fatal error encountered during reading");
        }
        if (bytes_read == 0) {
            throw std::runtime_error("Indexed sequential file has been corrupted");
        }
        if (first) {
            std::fstream overflow_file(
                "overflow.bin", std::ios::binary | std::ios::in | std::ios::out);
            overflow_file.seekp(0, std::ios::end);
            std::int64_t offset = overflow_file.tellp();
            _overflow_writes++;
            overflow_file.write(
                buffer.data(), Record::total_size_in_bytes + sizeof(overflow_offset_null));
            overflow_file.flush();
            char* pos = buffer.data();
            std::memcpy(pos, serialized_record.data(), serialized_record.size());
            pos += serialized_record.size();
            std::memcpy(pos, &offset, sizeof(offset));
            data_file.clear();
            data_file.seekp(page_index * buffer.size());
            _data_writes++;
            data_file.write(buffer.data(), bytes_read);
            data_file.flush();
            std::fstream updated_index_file(
                "index.bin", std::ios::binary | std::ios::in | std::ios::out);
            updated_index_file.seekp(0);
            _index_writes++;
            updated_index_file.write(
                reinterpret_cast<const char*>(&insert_statement->key),
                sizeof(insert_statement->key));
            updated_index_file.flush();
            _output << "Reads: " << total_reads() - reads_before << '\n';
            _output << "Writes: " << total_writes() - writes_before << '\n';
            return;
        }
        for (char* current_pos = buffer.data();
             current_pos + Record::total_size_in_bytes + sizeof(overflow_offset) <=
             buffer.data() + bytes_read;) {
            Record::key_type next_key{};
            std::memcpy(&next_key, current_pos, sizeof(next_key));
            if (next_key == insert_statement->key) {
                throw KeyError("Key already exists");
            }
            current_pos += Record::total_size_in_bytes;
            if (next_key > insert_statement->key) {
                if (overflow_offset != overflow_offset_null) {
                    std::fstream overflow_file(
                        "overflow.bin", std::ios::binary | std::ios::in | std::ios::out);
                    _overflow_reads++;
                    overflow_file.seekg(overflow_offset);
                    overflow_file.read(reinterpret_cast<char*>(&next_key), sizeof(next_key));
                    if (next_key == insert_statement->key) {
                        throw KeyError("Key already exists");
                    }
                    if (next_key > insert_statement->key) {
                        _overflow_writes++;
                        overflow_file.clear();
                        overflow_file.seekp(0, std::ios::end);
                        std::int64_t record_offset = overflow_file.tellp();
                        overflow_file.write(serialized_record.data(), serialized_record.size());
                        overflow_file.write(
                            reinterpret_cast<const char*>(&overflow_offset),
                            sizeof(overflow_offset));
                        overflow_file.flush();
                        std::memcpy(
                            current_pos - Record::total_size_in_bytes - sizeof(overflow_offset),
                            &record_offset,
                            sizeof(record_offset));
                        _data_writes++;
                        data_file.clear();
                        data_file.seekp(page_index * buffer.size());
                        data_file.write(buffer.data(), bytes_read);
                        data_file.flush();
                        _output << "Reads: " << total_reads() - reads_before << '\n';
                        _output << "Writes: " << total_writes() - writes_before << '\n';
                        return;
                    }
                    auto previous_overflow_offset = overflow_offset;
                    _overflow_reads++;
                    overflow_file.clear();
                    overflow_file.seekg(
                        Record::total_size_in_bytes - sizeof(next_key), std::ios::cur);
                    overflow_file.read(
                        reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                    while (overflow_offset != overflow_offset_null) {
                        overflow_file.clear();
                        overflow_file.seekg(overflow_offset);
                        overflow_file.read(reinterpret_cast<char*>(&next_key), sizeof(next_key));
                        overflow_file.seekg(
                            Record::total_size_in_bytes - sizeof(next_key), std::ios::cur);
                        if (next_key == insert_statement->key) {
                            throw KeyError("Key already exists");
                        }
                        if (next_key > insert_statement->key) {
                            break;
                        }
                        previous_overflow_offset = overflow_offset;
                        _overflow_reads++;
                        overflow_file.read(
                            reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                    }
                    _overflow_writes++;
                    overflow_file.clear();
                    overflow_file.seekp(0, std::ios::end);
                    std::int64_t record_offset = overflow_file.tellp();
                    overflow_file.write(serialized_record.data(), serialized_record.size());
                    overflow_file.write(
                        reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                    overflow_file.flush();
                    _overflow_writes++;
                    overflow_file.seekp(previous_overflow_offset + Record::total_size_in_bytes);
                    overflow_file.write(
                        reinterpret_cast<char*>(&record_offset), sizeof(record_offset));
                    overflow_file.flush();
                }
                else {
                    std::fstream overflow_file(
                        "overflow.bin", std::ios::in | std::ios::out | std::ios::binary);
                    overflow_file.seekp(0, std::ios::end);
                    std::int64_t record_offset = overflow_file.tellp();
                    _overflow_writes++;
                    overflow_file.write(serialized_record.data(), serialized_record.size());
                    overflow_file.write(
                        reinterpret_cast<const char*>(&overflow_offset_null),
                        sizeof(overflow_offset_null));
                    overflow_file.flush();
                    std::memcpy(
                        current_pos - Record::total_size_in_bytes - sizeof(overflow_offset),
                        &record_offset,
                        sizeof(record_offset));
                    _data_writes++;
                    data_file.clear();
                    data_file.seekp(page_index * buffer.size());
                    data_file.write(buffer.data(), bytes_read);
                    data_file.flush();
                }
                _output << "Reads: " << total_reads() - reads_before << '\n';
                _output << "Writes: " << total_writes() - writes_before << '\n';
                return;
            }
            std::memcpy(&overflow_offset, current_pos, sizeof(overflow_offset));
            current_pos += sizeof(overflow_offset);
        }
        if (overflow_offset != overflow_offset_null) {
            std::fstream overflow_file(
                "overflow.bin", std::ios::binary | std::ios::in | std::ios::out);
            overflow_file.seekg(overflow_offset);
            Record::key_type overflow_key{};
            _overflow_reads++;
            overflow_file.read(reinterpret_cast<char*>(&overflow_key), sizeof(overflow_key));
            if (overflow_key == insert_statement->key) {
                throw KeyError("Key already exists");
            }
            if (overflow_key > insert_statement->key) {
                overflow_file.clear();
                overflow_file.seekp(0, std::ios::end);
                std::int64_t record_offset = overflow_file.tellp();
                _overflow_writes++;
                overflow_file.write(serialized_record.data(), serialized_record.size());
                overflow_file.write(
                    reinterpret_cast<const char*>(&overflow_offset), sizeof(overflow_offset));
                overflow_file.flush();
                char* pos = buffer.data();
                while (pos + Record::total_size_in_bytes + sizeof(overflow_offset_null) <=
                       buffer.data() + bytes_read) {
                    pos += Record::total_size_in_bytes + sizeof(overflow_offset_null);
                }
                pos -= sizeof(overflow_offset_null);
                std::memcpy(pos, &record_offset, sizeof(record_offset));
                _data_writes++;
                data_file.clear();
                data_file.seekp(page_index * buffer.size());
                data_file.write(buffer.data(), bytes_read);
                data_file.flush();
                _output << "Reads: " << total_reads() - reads_before << '\n';
                _output << "Writes: " << total_writes() - writes_before << '\n';
                return;
            }
            auto overflow_offset_prev = overflow_offset;
            while (overflow_key < insert_statement->key) {
                overflow_offset_prev = overflow_offset;
                overflow_file.clear();
                _overflow_reads++;
                overflow_file.seekg(overflow_offset + Record::total_size_in_bytes);
                overflow_file.read(
                    reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                if (overflow_offset == overflow_offset_null) {
                    break;
                }
                overflow_file.seekg(overflow_offset);
                overflow_file.read(reinterpret_cast<char*>(&overflow_key), sizeof(overflow_key));
            }
            if (overflow_key == insert_statement->key) {
                throw KeyError("Key already exists");
            }
            if (overflow_offset != overflow_offset_null ||
                (overflow_offset == overflow_offset_null && buffer.size() == bytes_read)) {
                overflow_file.clear();
                overflow_file.seekp(0, std::ios::end);
                std::int64_t record_offset = overflow_file.tellp();
                _overflow_writes++;
                overflow_file.write(serialized_record.data(), serialized_record.size());
                overflow_file.write(
                    reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                overflow_file.flush();
                _overflow_writes++;
                overflow_file.seekp(overflow_offset_prev + Record::total_size_in_bytes);
                overflow_file.write(reinterpret_cast<char*>(&record_offset), sizeof(record_offset));
                overflow_file.flush();
                _output << "Reads: " << total_reads() - reads_before << '\n';
                _output << "Writes: " << total_writes() - writes_before << '\n';
                return;
            }
        }
        if (buffer.size() - bytes_read > 0) {
            char* pos = buffer.data() + bytes_read;
            std::memcpy(pos, serialized_record.data(), serialized_record.size());
            pos += serialized_record.size();
            std::memcpy(pos, &overflow_offset_null, sizeof(overflow_offset_null));
            _data_writes++;
            data_file.clear();
            data_file.seekp(page_index * buffer.size());
            data_file.write(
                buffer.data(),
                bytes_read + serialized_record.size() + sizeof(overflow_offset_null));
            data_file.flush();
        }
        else {
            std::fstream overflow_file(
                "overflow.bin", std::ios::binary | std::ios::in | std::ios::out);
            overflow_file.seekp(0, std::ios::end);
            _overflow_writes++;
            std::int64_t record_offset = overflow_file.tellp();
            overflow_file.write(serialized_record.data(), serialized_record.size());
            overflow_file.flush();
            if (overflow_offset == overflow_offset_null) {
                overflow_file.clear();
                overflow_file.seekp(0, std::ios::end);
                overflow_file.write(
                    reinterpret_cast<const char*>(&overflow_offset_null),
                    sizeof(overflow_offset_null));
                overflow_file.flush();
                char* pos = buffer.data();
                while (pos + Record::total_size_in_bytes + sizeof(overflow_offset_null) <=
                       buffer.data() + bytes_read) {
                    pos += Record::total_size_in_bytes + sizeof(overflow_offset_null);
                }
                pos -= sizeof(overflow_offset_null);
                std::memcpy(pos, &record_offset, sizeof(record_offset));
                _data_writes++;
                data_file.clear();
                data_file.seekp(page_index * buffer.size());
                data_file.write(buffer.data(), buffer.size());
                data_file.flush();
            }
            else {
                Record::key_type overflow_next_key{};
                _overflow_reads++;
                overflow_file.clear();
                overflow_file.seekg(overflow_offset);
                overflow_file.read(
                    reinterpret_cast<char*>(&overflow_next_key), sizeof(overflow_next_key));
                if (overflow_next_key == insert_statement->key) {
                    throw KeyError("Key already exists");
                }
                if (overflow_next_key > insert_statement->key) {
                    _overflow_writes++;
                    overflow_file.clear();
                    overflow_file.seekp(0, std::ios::end);
                    overflow_file.write(
                        reinterpret_cast<const char*>(&overflow_offset), sizeof(overflow_offset));
                    overflow_file.flush();
                    char* pos = buffer.data();
                    while (pos + Record::total_size_in_bytes + sizeof(overflow_offset_null) <=
                           buffer.data() + bytes_read) {
                        pos += Record::total_size_in_bytes + sizeof(overflow_offset_null);
                    }
                    pos -= sizeof(overflow_offset_null);
                    std::memcpy(pos, &record_offset, sizeof(record_offset));
                    _data_writes++;
                    data_file.clear();
                    data_file.seekp(page_index * buffer.size());
                    data_file.write(buffer.data(), buffer.size());
                    data_file.flush();
                    return;
                }
                auto overflow_offset_prev = overflow_offset;
                while (overflow_next_key < insert_statement->key) {
                    overflow_offset_prev = overflow_offset;
                    overflow_file.clear();
                    _overflow_reads++;
                    overflow_file.seekg(overflow_offset + Record::total_size_in_bytes);
                    overflow_file.read(
                        reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                    if (overflow_offset == overflow_offset_null) {
                        break;
                    }
                    overflow_file.clear();
                    overflow_file.seekg(overflow_offset);
                    overflow_file.read(
                        reinterpret_cast<char*>(&overflow_next_key), sizeof(overflow_next_key));
                }
                if (overflow_next_key == insert_statement->key) {
                    throw KeyError("Key already exists");
                }
                overflow_file.clear();
                overflow_file.seekp(0, std::ios::end);
                overflow_file.write(
                    reinterpret_cast<const char*>(&overflow_offset), sizeof(overflow_offset));
                overflow_file.seekp(overflow_offset_prev + Record::total_size_in_bytes);
                overflow_file.write(
                    reinterpret_cast<const char*>(&record_offset), sizeof(record_offset));
                overflow_file.flush();
                _overflow_writes++;
            }
        }
        _output << "Reads: " << total_reads() - reads_before << '\n';
        _output << "Writes: " << total_writes() - writes_before << '\n';
        break;
    }
    case StatementType::Update: {
        auto update_statement = static_cast<const UpdateStatement*>(statement);
        if (update_statement->key == 0) {
            throw KeyError("Key cannot be equal to 0");
        }
        auto reads_before = total_reads();
        auto writes_before = total_writes();
        std::array<char, 336> buffer{};
        std::ifstream index_file("index.bin", std::ios::binary | std::ios::ate);
        if (!index_file) {
            throw std::runtime_error("Could not open file");
        }
        if (index_file.tellg() == 0) {
            throw NotInitializedError("Indexed sequential file was not initialized");
        }
        index_file.seekg(0);
        std::uint64_t page_index{};
        bool exists = false;
        bool found_page = false;
        while (!found_page) {
            _index_reads++;
            index_file.read(buffer.data(), buffer.size());
            auto bytes_read = index_file.gcount();
            if (index_file.bad()) {
                throw std::runtime_error("Fatal error encountered during reading");
            }
            if (bytes_read > 0) {
                for (char* current_pos = buffer.data();
                     current_pos + 16 <= buffer.data() + bytes_read;) {
                    Record::key_type next_key{};
                    std::memcpy(&next_key, current_pos, sizeof(next_key));
                    current_pos += sizeof(next_key);
                    if (next_key > update_statement->key) {
                        found_page = true;
                        break;
                    }
                    std::memcpy(&page_index, current_pos, sizeof(page_index));
                    current_pos += sizeof(page_index);
                    if (next_key == update_statement->key) {
                        exists = true;
                        found_page = true;
                        break;
                    }
                }
            }
            if (index_file.eof()) {
                found_page = true;
            }
        }
        index_file.close();
        auto updated_record = Record(update_statement->key, update_statement->data);
        std::array<char, Record::total_size_in_bytes> updated_record_buffer{};
        updated_record.serialize(updated_record_buffer.data(), updated_record_buffer.size());
        std::fstream data_file("data.bin", std::ios::binary | std::ios::in | std::ios::out);
        data_file.seekg(page_index * buffer.size());
        _data_reads++;
        data_file.read(buffer.data(), buffer.size());
        auto bytes_read = data_file.gcount();
        if (data_file.bad()) {
            throw std::runtime_error("Fatal error encountered during reading");
        }
        if (exists) {
            if (bytes_read < Record::total_size_in_bytes + sizeof(overflow_offset_null)) {
                throw std::runtime_error("Indexed sequential file has been corrupted");
            }
            std::memcpy(buffer.data(), updated_record_buffer.data(), Record::total_size_in_bytes);
            data_file.clear();
            data_file.seekp(page_index * buffer.size());
            data_file.write(buffer.data(), bytes_read);
            data_file.flush();
            _data_writes++;
        }
        else {
            std::int64_t overflow_offset = overflow_offset_null;
            for (char* current_pos = buffer.data();
                 current_pos + Record::total_size_in_bytes + sizeof(overflow_offset) <=
                 buffer.data() + bytes_read;) {
                Record::key_type next_key{};
                std::memcpy(&next_key, current_pos, sizeof(next_key));
                if (next_key == update_statement->key) {
                    std::memcpy(
                        current_pos, updated_record_buffer.data(), Record::total_size_in_bytes);
                    data_file.clear();
                    data_file.seekp(page_index * buffer.size());
                    data_file.write(buffer.data(), bytes_read);
                    data_file.flush();
                    _data_writes++;
                    break;
                }
                current_pos += Record::total_size_in_bytes;
                if (next_key > update_statement->key) {
                    std::fstream overflow_file(
                        "overflow.bin", std::ios::binary | std::ios::in | std::ios::out);
                    std::array<char, Record::total_size_in_bytes> overflow_buffer{};
                    while (overflow_offset != overflow_offset_null) {
                        _overflow_reads++;
                        overflow_file.clear();
                        overflow_file.seekg(overflow_offset);
                        overflow_file.read(overflow_buffer.data(), overflow_buffer.size());
                        auto next_key =
                            *reinterpret_cast<Record::key_type*>(overflow_buffer.data());
                        if (next_key == update_statement->key) {
                            _overflow_writes++;
                            overflow_file.clear();
                            overflow_file.seekp(overflow_offset);
                            overflow_file.write(
                                updated_record_buffer.data(), updated_record_buffer.size());
                            overflow_file.flush();
                            break;
                        }
                        if (next_key > update_statement->key) {
                            throw KeyError("No record with given key exist");
                        }
                        overflow_file.read(
                            reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                    }
                    break;
                }
                std::memcpy(&overflow_offset, current_pos, sizeof(overflow_offset));
                current_pos += sizeof(overflow_offset);
            }
        }
        _output << "Reads: " << total_reads() - reads_before << '\n';
        _output << "Writes: " << total_writes() - writes_before << '\n';
        break;
    }
    case StatementType::Remove: {
        throw NotImplementedError("Remove was not implemented");
        break;
    }
    case StatementType::List: {
        auto reads_before = total_reads();
        auto writes_before = total_writes();
        std::ifstream data_file("data.bin", std::ios::binary);
        std::array<char, 336> buffer{};
        std::ifstream overflow_file("overflow.bin", std::ios::binary);
        std::array<char, Record::total_size_in_bytes> overflow_buffer{};
        std::uint64_t page_index{};
        while (data_file) {
            data_file.read(buffer.data(), buffer.size());
            auto bytes_read = data_file.gcount();
            if (bytes_read == 0) {
                continue;
            }
            _data_reads++;
            for (char* current_pos = buffer.data();
                 current_pos + Record::total_size_in_bytes + sizeof(overflow_offset_null) <=
                 buffer.data() + bytes_read;) {
                auto overflow_offset = overflow_offset_null;
                auto record_pos = current_pos - buffer.data();
                auto [_, record] = Record::deserialize(current_pos);
                current_pos += Record::total_size_in_bytes;
                std::memcpy(&overflow_offset, current_pos, sizeof(overflow_offset));
                current_pos += sizeof(overflow_offset);
                if (record.has_value()) {
                    _output << record->to_string() << " (Data: [" << page_index << ":" << record_pos
                            << "] -> [" << overflow_offset << "])\n";
                }
                while (overflow_offset != overflow_offset_null) {
                    _overflow_reads++;
                    overflow_file.seekg(overflow_offset);
                    std::uint64_t overflow_pos = overflow_offset;
                    overflow_file.read(overflow_buffer.data(), overflow_buffer.size());
                    overflow_file.read(
                        reinterpret_cast<char*>(&overflow_offset), sizeof(overflow_offset));
                    auto [_, record] = Record::deserialize(overflow_buffer.data());
                    if (record.has_value()) {
                        _output << record->to_string() << " (Overflow: [" << overflow_pos
                                << "] -> [" << overflow_offset << "])\n";
                    }
                }
            }
            page_index++;
        }
        _output << "Reads: " << total_reads() - reads_before << '\n';
        _output << "Writes: " << total_writes() - writes_before << '\n';
        break;
    }
    case StatementType::Sort: {
        auto reads_before = total_reads();
        auto writes_before = total_writes();
        sort();
        std::ofstream overflow_file(
            "overflow.bin", std::ios::binary | std::ios::out | std::ios::trunc);
        overflow_file.flush();
        overflow_file.close();
        merge();
        std::ofstream index_file("index.bin", std::ios::binary | std::ios::out | std::ios::trunc);
        index_file.flush();
        index_file.close();
        rebuild_index();
        _output << "Reads: " << total_reads() - reads_before << '\n';
        _output << "Writes: " << total_writes() - writes_before << '\n';
        break;
    }
    case StatementType::Status:
        _output << "Total Reads: " << total_reads() << '\n';
        _output << "Total Writes: " << total_writes() << '\n';
        _output << "Total Operations: " << total_operations() << '\n';
        _output << "Index Reads: " << index_reads() << '\n';
        _output << "Index Writes: " << index_writes() << '\n';
        _output << "Index Operations: " << total_index_operations() << '\n';
        _output << "Data Reads: " << data_reads() << '\n';
        _output << "Data Writes: " << data_writes() << '\n';
        _output << "Data Operations: " << total_data_operations() << '\n';
        _output << "Overflow Reads: " << overflow_reads() << '\n';
        _output << "Overflow Writes: " << overflow_writes() << '\n';
        _output << "Overflow Operations: " << total_overflow_operations() << '\n';
        break;
    case StatementType::Invalid:
    default:
        throw InvalidStatementError("Syntax error");
    }
}
