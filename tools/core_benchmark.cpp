#include "core/alignment.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
int main(int argc, char** argv) {
    try {
        const std::size_t rows = argc > 1 ? std::stoull(argv[1]) : 500;
        const std::size_t columns = argc > 2 ? std::stoull(argv[2]) : 100000;
        if (rows == 0 || columns == 0) throw std::runtime_error("Dimensions must be positive.");
        using Clock = std::chrono::steady_clock;
        auto start = Clock::now();
        std::vector<Record> records;
        records.reserve(rows);
        for (std::size_t r = 0; r < rows; ++r) {
            std::string text(columns, 'A');
            for (std::size_t c = r % 17; c < columns; c += 17) text[c] = '~';
            records.push_back({"row_" + std::to_string(r + 1), std::move(text)});
        }
        Document d; d.load(std::move(records));
        const auto loaded = Clock::now();
        const auto id = d.rows()[rows / 2].id;
        for (int i = 0; i < 1000; ++i) {
            d.replace(id, columns / 2, 0, "N");
            d.undo();
        }
        const auto edited = Clock::now();
        std::size_t checksum = 0;
        for (const auto& row : d.rows()) checksum += row.symbols.residue_rank(columns / 2);
        const auto ranked = Clock::now();
        auto ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b-a).count(); };
        std::cout << "Rows: " << rows << ", columns: " << columns << '\n'
                  << "Stored symbol bytes: " << rows * columns << '\n'
                  << "Generate + load ms: " << ms(start, loaded) << '\n'
                  << "1000 insert/undo pairs ms: " << ms(loaded, edited) << '\n'
                  << "Average insert/undo pair ms: " << ms(loaded, edited) / 1000 << '\n'
                  << "First rank query for all rows ms: " << ms(edited, ranked) << '\n'
                  << "Logical history bytes: " << d.history_bytes() << '\n'
                  << "Checksum: " << checksum << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

