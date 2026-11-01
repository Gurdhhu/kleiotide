#include "core/chromatogram.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace kleiotide {
namespace {
char upper(char c) { return c >= 'a' && c <= 'z' ? char(c - 'a' + 'A') : c; }
double spacing(const TraceLink &link, std::size_t ordinal) {
  const auto &trace = *link.original;
  std::size_t nearest = trace.peaks.size() / 2;
  if (!link.originalCalls.empty()) {
    auto left = std::min(ordinal, link.originalCalls.size() - 1), right = left;
    while (link.originalCalls[left] < 0 && left)
      --left;
    while (link.originalCalls[right] < 0 &&
           right + 1 < link.originalCalls.size())
      ++right;
    if (link.originalCalls[left] >= 0)
      nearest = std::size_t(link.originalCalls[left]);
    else if (link.originalCalls[right] >= 0)
      nearest = std::size_t(link.originalCalls[right]);
  }
  std::vector<double> differences;
  const auto begin = nearest > 8 ? nearest - 8 : 1,
             end = std::min(trace.peaks.size(), nearest + 9);
  for (auto i = std::max(std::size_t(1), begin); i < end; ++i)
    if (trace.peaks[i] > trace.peaks[i - 1])
      differences.push_back(trace.peaks[i] - trace.peaks[i - 1]);
  if (differences.empty())
    return 12.;
  std::sort(differences.begin(), differences.end());
  return std::clamp(differences[differences.size() / 2], 2., 100.);
}
} // namespace
std::optional<std::uint32_t> trace_sample_anchor(const TraceLink &link,
                                                 std::size_t ordinal) {
  const auto call = link.originalCalls[ordinal];
  if (call >= 0)
    return link.original->peaks[std::size_t(call)];
  if (!link.inferredPeaks.empty() && link.inferredPeaks[ordinal] >= 0)
    return std::uint32_t(link.inferredPeaks[ordinal]);
  return {};
}
std::vector<double> trace_sample_positions(const TraceLink &link) {
  std::vector<double> positions(link.originalCalls.size());
  const double last = double(link.original->channels[0].size() - 1),
               direction = link.reversed ? -1. : 1.;
  for (std::size_t i = 0; i < positions.size(); ++i)
    if (const auto sample = trace_sample_anchor(link, i))
      positions[i] = *sample;
  for (std::size_t begin = 0; begin < positions.size();) {
    if (trace_sample_anchor(link, begin)) {
      ++begin;
      continue;
    }
    auto end = begin + 1;
    while (end < positions.size() && !trace_sample_anchor(link, end))
      ++end;
    const auto count = end - begin;
    const double step = spacing(link, begin);
    const double lo =
        begin ? positions[begin - 1]
        : end < positions.size()
            ? positions[end] - direction * step * double(count + 1)
            : (direction > 0 ? 0 : last);
    const double hi = end < positions.size()
                          ? positions[end]
                          : lo + direction * step * double(count + 1);
    for (auto i = begin; i < end; ++i)
      positions[i] = std::clamp(
          lo + (hi - lo) * double(i - begin + 1) / double(count + 1), 0., last);
    begin = end;
  }
  return positions;
}
void refine_trace_flanks(TraceLink &link, std::string_view residues,
                         const std::function<bool()> &cancelled,
                         std::size_t maxCells) {
  if (residues.size() != link.originalCalls.size() || residues.empty())
    return;
  const auto &trace = *link.original;
  if (trace.channels[0].empty() || trace.peaks.size() != trace.calls.size())
    return;
  auto check = [&] {
    if (cancelled && cancelled())
      throw std::runtime_error("Trace preview cancelled.");
  };
  for (const int side : {-1, 1}) {
    check();
    auto first =
        std::find_if(link.originalCalls.begin(), link.originalCalls.end(),
                     [](auto value) { return value >= 0; });
    auto last =
        std::find_if(link.originalCalls.rbegin(), link.originalCalls.rend(),
                     [](auto value) { return value >= 0; });
    if (first == link.originalCalls.end())
      return;
    auto anchor = side > 0 ? std::size_t(link.originalCalls.rend() - last - 1)
                           : std::size_t(first - link.originalCalls.begin());
    const auto direction = (link.reversed ? -1 : 1) * side;
    // Extend over literal recorded calls first. Original-call identities remain
    // exact.
    while ((side > 0 ? anchor + 1 < residues.size() : anchor > 0)) {
      const auto next = side > 0 ? anchor + 1 : anchor - 1;
      const auto call = std::int64_t(link.originalCalls[anchor]) + direction;
      if (call < 0 || call >= std::int64_t(trace.calls.size()))
        break;
      const auto expected =
          upper(link.reversed ? trace_complement(trace.calls[std::size_t(call)])
                              : trace.calls[std::size_t(call)]);
      if (upper(residues[next]) != expected)
        break;
      link.originalCalls[next] = std::int32_t(call);
      anchor = next;
    }
    std::vector<std::size_t> indices;
    for (auto at = std::int64_t(anchor) + side;
         at >= 0 && at < std::int64_t(residues.size()) && indices.size() < 128;
         at += side) {
      const auto base = upper(residues[std::size_t(at)]);
      if (std::string_view("ACGT").find(base) == std::string_view::npos)
        break;
      indices.push_back(std::size_t(at));
    }
    if (indices.empty())
      continue;
    const auto origin = trace.peaks[std::size_t(link.originalCalls[anchor])];
    const double step = spacing(link, anchor);
    const int minimum = std::max(1, int(std::floor(step * .45))),
              maximum = std::max(minimum + 1, int(std::ceil(step * 1.8)));
    const auto available =
        direction > 0 ? trace.channels[0].size() - 1 - origin : origin;
    const int span = int(std::min<std::size_t>(
        available, indices.size() * std::size_t(maximum)));
    if (span < minimum * int(indices.size()))
      continue;
    double amplitude = 1;
    for (const auto &channel : trace.channels)
      amplitude = std::max(amplitude, double(channel[origin]));
    constexpr double invalid = -1e100;
    std::vector<double> previous(std::size_t(span) + 1, invalid),
        current(previous);
    if (indices.size() > maxCells / (std::size_t(span) + 1))
      continue;
    std::vector<int> back(indices.size() * (std::size_t(span) + 1), -1);
    previous[0] = 0;
    std::size_t supported = 0;
    int best = 0;
    for (std::size_t k = 0; k < indices.size(); ++k) {
      check();
      std::fill(current.begin(), current.end(), invalid);
      char base = upper(residues[indices[k]]);
      if (link.reversed)
        base = trace_complement(base);
      const auto channel = std::string_view("ACGT").find(base);
      double bestScore = invalid;
      int bestAt = -1;
      for (int distance = minimum * int(k + 1);
           distance <= std::min(span, maximum * int(k + 1)); ++distance) {
        const auto raw = std::size_t(std::int64_t(origin) +
                                     std::int64_t(direction) * distance);
        const double signal = trace.channels[channel][raw];
        double other = 1;
        for (std::size_t c = 0; c < 4; ++c)
          if (c != channel)
            other = std::max(other, double(trace.channels[c][raw]));
        if (signal < std::max(5., amplitude * .04) || signal < other * 1.1)
          continue;
        double local = std::max(1., signal);
        const int radius = std::max(2, int(step * .35));
        for (int delta = -radius; delta <= radius; ++delta) {
          const auto pos = std::int64_t(raw) + delta;
          if (pos >= 0 && pos < std::int64_t(trace.channels[0].size()))
            local = std::max(local,
                             double(trace.channels[channel][std::size_t(pos)]));
        }
        const double evidence =
            std::min(2.5, std::log((signal + 1) / (other + 1))) +
            .6 * std::log((signal + 1) / (local + 1));
        for (int gap = minimum; gap <= maximum && gap <= distance; ++gap) {
          const auto prior = distance - gap;
          if (previous[std::size_t(prior)] == invalid)
            continue;
          const auto deviation = (gap - step) / step;
          const double score = previous[std::size_t(prior)] + evidence -
                               3. * deviation * deviation;
          if (score > current[std::size_t(distance)]) {
            current[std::size_t(distance)] = score;
            back[k * (std::size_t(span) + 1) + std::size_t(distance)] = prior;
          }
        }
        if (current[std::size_t(distance)] > bestScore) {
          bestScore = current[std::size_t(distance)];
          bestAt = distance;
        }
      }
      if (bestAt < 0)
        break;
      supported = k + 1;
      best = bestAt;
      previous.swap(current);
    }
    // At least three successive sequence-supported calls are needed; a single
    // noisy peak or unsupported homopolymer tail does not establish a
    // placement.
    if (supported < 3)
      continue;
    std::vector<std::int32_t> path(supported);
    int at = best;
    for (auto k = supported; k > 0; --k) {
      path[k - 1] =
          std::int32_t(std::int64_t(origin) + std::int64_t(direction) * at);
      at = back[(k - 1) * (std::size_t(span) + 1) + std::size_t(at)];
    }
    // Where a distinct nearby maximum exists, center the inferred call on it.
    // Broad shoulders retain the sequence/spacing estimate and never gain Q.
    const auto radius = std::max<std::int64_t>(2, std::lround(step * .4));
    auto distance = [&](std::int64_t sample) {
      return (sample - std::int64_t(origin)) * direction;
    };
    for (std::size_t k = 0; k < supported; ++k) {
      char base = upper(residues[indices[k]]);
      if (link.reversed)
        base = trace_complement(base);
      const auto channel = std::string_view("ACGT").find(base);
      const auto &signal = trace.channels[channel];
      const auto center = std::int64_t(path[k]);
      const auto before = k ? distance(path[k - 1]) : 0;
      const auto after = k + 1 < supported
                             ? distance(path[k + 1])
                             : std::numeric_limits<std::int64_t>::max() / 2;
      auto bestOffset = radius + 1;
      std::int64_t maximumSample = center;
      for (auto sample = std::max<std::int64_t>(1, center - radius);
           sample <= std::min<std::int64_t>(std::int64_t(signal.size()) - 2,
                                            center + radius);
           ++sample) {
        const auto progress = distance(sample);
        if (progress < before + minimum || progress > after - minimum)
          continue;
        const auto peak = signal[std::size_t(sample)];
        if (peak < std::max(5., amplitude * .04) ||
            peak <= signal[std::size_t(sample - 1)] ||
            peak < signal[std::size_t(sample + 1)])
          continue;
        double other = 1;
        for (std::size_t c = 0; c < 4; ++c)
          if (c != channel)
            other =
                std::max(other, double(trace.channels[c][std::size_t(sample)]));
        if (peak < other * 1.1)
          continue;
        const auto offset = std::abs(sample - center);
        if (offset < bestOffset ||
            (offset == bestOffset &&
             peak > signal[std::size_t(maximumSample)])) {
          bestOffset = offset;
          maximumSample = sample;
        }
      }
      path[k] = std::int32_t(maximumSample);
    }
    // Long homopolymers need multiple signal maxima. Constant plateaus and a
    // single broad noisy bump cannot justify several successive new calls.
    for (std::size_t begin = 0; begin < supported;) {
      auto end = begin + 1;
      while (end < supported &&
             upper(residues[indices[end]]) == upper(residues[indices[begin]]))
        ++end;
      if (end - begin >= 3) {
        char base = upper(residues[indices[begin]]);
        if (link.reversed)
          base = trace_complement(base);
        const auto &signal =
            trace.channels[std::string_view("ACGT").find(base)];
        const auto lo = std::max<std::int64_t>(
                       1, std::min(path[begin], path[end - 1]) -
                              std::int64_t(step * .4)),
                   hi = std::min<std::int64_t>(
                       std::int64_t(signal.size()) - 2,
                       std::max(path[begin], path[end - 1]) +
                           std::int64_t(step * .4));
        std::size_t peaks = 0;
        std::int64_t lastMaximum = -1000000;
        for (auto sample = lo; sample <= hi; ++sample)
          if (signal[std::size_t(sample)] > std::max(5., amplitude * .04) &&
              signal[std::size_t(sample)] > signal[std::size_t(sample - 1)] &&
              signal[std::size_t(sample)] >= signal[std::size_t(sample + 1)] &&
              sample - lastMaximum >= minimum) {
            ++peaks;
            lastMaximum = sample;
          }
        if (peaks < end - begin - 1) {
          supported = begin;
          break;
        }
      }
      begin = end;
    }
    if (supported < 3)
      continue;
    if (link.inferredPeaks.empty())
      link.inferredPeaks.assign(residues.size(), -1);
    for (std::size_t k = 0; k < supported; ++k)
      link.inferredPeaks[indices[k]] = path[k];
  }
}
} // namespace kleiotide
