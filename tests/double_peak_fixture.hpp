#pragma once

#include "core/alignment.hpp"

namespace kleiotide::test {
inline Record double_peak_read(std::string name, bool withQuality) {
    auto trace = std::make_shared<Chromatogram>();
    trace->calls = "ACACAGT";
    trace->peaks = {20, 40, 60, 80, 100, 120, 140};
    if (withQuality) trace->quality = {0, 4, 72, 54, 12, 38, 0};
    for (std::size_t i = 0; i < trace->channels.size(); ++i)
        trace->channels[i].resize(180, static_cast<int>(10 + i * 10));
    const auto peak = [&](std::size_t channel, std::size_t center, int height) {
        auto& samples = trace->channels[channel];
        samples[center] += height;
        for (std::size_t offset = 1; offset <= 3; ++offset) {
            samples[center - offset] += height * (4 - static_cast<int>(offset)) / 4;
            samples[center + offset] += height * (4 - static_cast<int>(offset)) / 4;
        }
    };
    peak(0, 20, 100); peak(2, 24, 50);                      // Both thresholds exactly.
    peak(1, 40, 100); peak(3, 41, 64); peak(0, 40, 30);     // Third peak exactly 30%.
    peak(0, 60, 100); peak(3, 66, 70);                      // Peaks are too far apart at 20%.
    peak(1, 80, 100); peak(0, 81, 49);                      // Below the secondary threshold.
    peak(0, 100, 100); peak(1, 101, 60);                    // The manually edited g needs review.
    peak(2, 120, 110);
    // A small bump on a broad shoulder has a large amplitude and little prominence.
    for (std::size_t i = 112; i <= 134; ++i) trace->channels[1][i] += 65;
    trace->channels[1][120] += 1;
    peak(3, 140, 110); peak(1, 142, 72);                   // Imported call was deleted.
    peak(1, 157, 100); peak(3, 158, 70);                   // A manually placed or inferred anchor.
    auto link = std::make_shared<TraceLink>();
    link->original = trace;
    link->originalCalls = {0, 1, 2, 3, 4, 5};
    return {std::move(name), "A~CACgG", RowKind::Nucleotide, link};
}
}
