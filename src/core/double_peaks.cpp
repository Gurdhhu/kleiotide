#include "core/double_peaks.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace kleiotide {
namespace {
constexpr std::string_view bases = "ACGT";
constexpr double siteRadius = 0.4;
constexpr double flankRadius = 0.5;
constexpr double minimumProminence = 0.2;

struct Site {
    std::size_t column;
    std::uint32_t sample;
};

struct Peak {
    std::size_t sample;
    std::int32_t height;
    std::size_t channel;
};

double local_spacing(const Chromatogram& trace, std::span<const Site> sites, std::size_t index) {
    const auto at = static_cast<std::size_t>(std::lower_bound(trace.peaks.begin(), trace.peaks.end(),
        sites[index].sample) - trace.peaks.begin());
    std::array<std::uint32_t, 12> differences{};
    std::size_t count = 0;
    const auto first = at > 6 ? at - 6 : 1;
    const auto end = std::min(trace.peaks.size(), at + 6);
    for (auto i = first; i < end; ++i) {
        const auto difference = trace.peaks[i] - trace.peaks[i - 1];
        if (difference) differences[count++] = difference;
    }
    if (count) {
        auto middle = differences.begin() + static_cast<std::ptrdiff_t>(count / 2);
        std::nth_element(differences.begin(), middle,
                         differences.begin() + static_cast<std::ptrdiff_t>(count));
        return *middle;
    }
    double nearest = static_cast<double>(trace.channels.front().size());
    if (index) nearest = sites[index].sample - sites[index - 1].sample;
    if (index + 1 < sites.size()) nearest = std::min(nearest, double(sites[index + 1].sample - sites[index].sample));
    return sites.size() > 1 ? nearest : 0;
}

std::size_t distance(std::size_t a, std::size_t b) {
    return a > b ? a - b : b - a;
}

std::optional<Peak> best_peak(const std::vector<std::int32_t>& signal, std::size_t channel,
                             std::size_t first, std::size_t last, std::size_t target,
                             std::size_t flank, std::size_t maxShift) {
    std::optional<Peak> best;
    for (auto start = first; start <= last;) {
        auto end = start;
        while (end < last && signal[end] == signal[end + 1]) ++end;
        if (signal[start] > signal[start - 1] && signal[end] > signal[end + 1]) {
            const auto apex = start + (end - start) / 2;
            const auto height = signal[apex];
            if (height <= 0 || distance(apex, target) > maxShift) {
                start = end + 1;
                continue;
            }
            const auto left = std::min_element(signal.begin() + start - std::min(start, flank),
                signal.begin() + start);
            const auto right = std::min_element(signal.begin() + end + 1,
                signal.begin() + end + 1 + std::min(flank, signal.size() - end - 1));
            // The local baseline is only used to judge the peak shape.
            const double rise = double(height) - std::min(*left, *right);
            const double prominence = double(height) - std::max(*left, *right);
            if (prominence >= std::max(1.0, rise * minimumProminence) &&
                (!best || height > best->height ||
                 (height == best->height && distance(apex, target) < distance(best->sample, target))))
                best = Peak{apex, height, channel};
        }
        start = end + 1;
    }
    return best;
}

void examine_site(DoublePeakPlan& plan, const Sequence& row, std::span<const Site> sites, std::size_t index) {
    const auto center = sites[index].sample;
    if ((index && sites[index - 1].sample == center) ||
        (index + 1 < sites.size() && sites[index + 1].sample == center))
        return;
    const auto& trace = *row.trace->original;
    if (trace.channels.front().size() < 3) return;
    const double spacing = local_spacing(trace, sites, index);
    if (spacing <= 0) return;

    double lower = std::max(1.0, center - spacing * siteRadius);
    double upper = std::min(double(trace.channels.front().size() - 2), center + spacing * siteRadius);
    // Assign each apex to at most one current anchor. Omit exact midpoints.
    if (index) lower = std::max(lower, std::floor((double(center) + sites[index - 1].sample) / 2) + 1);
    if (index + 1 < sites.size())
        upper = std::min(upper, std::ceil((double(center) + sites[index + 1].sample) / 2) - 1);
    if (std::ceil(lower) > std::floor(upper)) return;
    const auto first = static_cast<std::size_t>(std::ceil(lower));
    const auto last = static_cast<std::size_t>(std::floor(upper));
    const auto flank = std::max<std::size_t>(1, static_cast<std::size_t>(std::round(spacing * flankRadius)));
    const auto siteShift = std::max(distance(center, first), distance(center, last));

    // Establish the tallest displayed signal before rejecting shoulders and
    // unresolved maxima. A shorter channel cannot stand in for it.
    std::int32_t tallest = 0;
    for (const auto& channel : trace.channels)
        tallest = std::max(tallest, *std::max_element(channel.begin() + first, channel.begin() + last + 1));
    if (!tallest) return;

    std::optional<Peak> major;
    for (std::size_t channel = 0; channel < trace.channels.size(); ++channel) {
        const auto peak = best_peak(trace.channels[channel], channel, first, last, center, flank, siteShift);
        if (peak && peak->height == tallest &&
            (!major || distance(peak->sample, center) < distance(major->sample, center)))
            major = peak;
    }
    if (!major) {
        ++plan.unresolvedMajor;
        return;
    }
    const auto maxShift = static_cast<std::size_t>(std::floor(spacing * plan.settings.displacementPercent / 100));
    std::optional<Peak> secondary, third;
    for (std::size_t channel = 0; channel < trace.channels.size(); ++channel) {
        if (channel == major->channel) continue;
        // Keep the site window intact so an apex plateau is measured the same
        // way for the major peak and its neighbors.
        const auto peak = best_peak(trace.channels[channel], channel, first, last, major->sample, flank, maxShift);
        if (!peak) continue;
        if (!secondary || peak->height > secondary->height) {
            third = secondary;
            secondary = peak;
        } else if (!third || peak->height > third->height) {
            third = peak;
        }
    }
    if (!secondary || 100LL * secondary->height < std::int64_t(major->height) * plan.settings.secondaryPercent) return;
    if (third && 100LL * third->height >= std::int64_t(major->height) * plan.settings.thirdPercent) {
        ++plan.thirdPeakExcluded;
        return;
    }

    const auto column = sites[index].column;
    const char current = row.symbols.text()[column];
    const bool lowercase = current >= 'a' && current <= 'z';
    const char upperCurrent = lowercase ? static_cast<char>(current - 'a' + 'A') : current;
    const auto oriented = [&](std::size_t channel) { return row.trace->reversed ? 3 - channel : channel; };
    const char primary = bases[oriented(major->channel)], minor = bases[oriented(secondary->channel)];
    constexpr std::string_view iupac = "~ACMGRSVTWYHKDBN";
    const char proposed = iupac[(1u << oriented(major->channel)) | (1u << oriented(secondary->channel))];
    if (proposed == upperCurrent) return;
    const char currentBase = upperCurrent == 'U' ? 'T' : upperCurrent;
    const auto shift = distance(major->sample, secondary->sample);
    plan.calls.push_back({row.id, column, current,
        lowercase ? static_cast<char>(proposed - 'A' + 'a') : proposed, primary, minor,
        major->height, secondary->height, 100.0 * secondary->height / major->height,
        third ? 100.0 * third->height / major->height : 0,
        100 * double(shift) / spacing, shift, currentBase != primary && currentBase != minor});
}
}

DoublePeakPlan plan_double_peaks(const Document& document, DoublePeakSettings settings,
                               const std::vector<SequenceId>& selected,
                               const std::function<bool()>& cancelled) {
    if (settings.secondaryPercent > 100 || settings.thirdPercent > 100 || settings.displacementPercent > 100)
        throw std::invalid_argument("Double peak thresholds must be between 0 and 100%.");
    DoublePeakPlan plan;
    plan.source = &document;
    plan.revision = document.state_id();
    plan.settings = settings;
    const std::unordered_set<SequenceId> selectedIds(selected.begin(), selected.end());
    const auto check = [&] {
        if (cancelled && cancelled()) throw std::runtime_error("Double peak calculation cancelled.");
    };
    for (const auto& row : document.rows()) {
        check();
        if (!row.trace || (!settings.allTraces && !selectedIds.contains(row.id))) continue;
        ++plan.rows;
        std::vector<Site> sites;
        sites.reserve(row.symbols.residue_count());
        std::size_t ordinal = 0;
        for (std::size_t column = 0; column < row.symbols.size(); ++column) {
            if ((column & 1023) == 0) check();
            if (is_gap(row.symbols.text()[column])) continue;
            ++plan.sites;
            if (const auto anchor = trace_sample_anchor(*row.trace, ordinal++))
                sites.push_back({column, *anchor});
            else
                ++plan.unanchored;
        }
        if (row.trace->reversed) std::reverse(sites.begin(), sites.end());
        const auto firstCall = plan.calls.size();
        for (std::size_t i = 0; i < sites.size(); ++i) {
            if ((i & 255) == 0) check();
            examine_site(plan, row, sites, i);
        }
        if (row.trace->reversed)
            std::reverse(plan.calls.begin() + static_cast<std::ptrdiff_t>(firstCall), plan.calls.end());
    }
    check();
    return plan;
}

void apply_double_peaks(Document& document, const DoublePeakPlan& plan, std::span<const unsigned char> included) {
    if (plan.source != &document || plan.revision != document.state_id())
        throw std::invalid_argument("The alignment changed. Recalculate the double peak preview.");
    if (included.size() != plan.calls.size()) throw std::invalid_argument("Invalid double peak selection.");
    std::vector<Document::Replacement> edits;
    for (std::size_t first = 0; first < plan.calls.size();) {
        const auto id = plan.calls[first].id;
        auto end = first + 1;
        while (end < plan.calls.size() && plan.calls[end].id == id) ++end;
        const auto& row = document.sequence(id);
        while (first < end && !included[first]) ++first;
        if (first < end) {
            auto last = end - 1;
            while (!included[last]) --last;
            const auto start = plan.calls[first].column;
            const auto length = plan.calls[last].column - start + 1;
            auto updated = row.symbols.text().substr(start, length);
            for (auto i = first; i <= last; ++i)
                if (included[i]) updated.at(plan.calls[i].column - start) = plan.calls[i].proposed;
            edits.push_back({id, start, length, std::move(updated), row.trace});
        }
        first = end;
    }
    document.replace_many(edits);
}
}
