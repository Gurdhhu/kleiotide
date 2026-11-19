#pragma once

#include "core/alignment.hpp"

#include <functional>
#include <span>

namespace kleiotide {
struct DoublePeakSettings {
    bool allTraces = false;
    unsigned secondaryPercent = 50;
    unsigned thirdPercent = 30;
    unsigned displacementPercent = 20;
};

struct DoublePeakCall {
    SequenceId id;
    std::size_t column;
    char current, proposed, major, minor;
    double secondaryPercent, thirdPercent, displacementPercent;
    std::size_t displacementSamples;
    bool needsReview;
};

struct DoublePeakPlan {
    const Document* source = nullptr;
    std::uint64_t revision = 0;
    DoublePeakSettings settings;
    std::vector<DoublePeakCall> calls;
    std::size_t rows = 0, sites = 0, unanchored = 0, thirdPeakExcluded = 0;
};

DoublePeakPlan plan_double_peaks(const Document&, DoublePeakSettings,
                               const std::vector<SequenceId>& selected,
                               const std::function<bool()>& cancelled = {});
void apply_double_peaks(Document&, const DoublePeakPlan&, std::span<const unsigned char> included);
}
