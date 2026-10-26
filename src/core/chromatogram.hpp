#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace kleiotide {
struct Sequence;
// Original evidence is immutable. Channels are always stored in A,C,G,T order.
struct Chromatogram {
    std::string sourcePath, sampleName, calls;
    std::vector<std::uint32_t> peaks;
    std::vector<std::uint8_t> quality;
    std::array<std::vector<std::int32_t>,4> channels;
    unsigned callSet = 1;
};
struct TraceLink {
    std::shared_ptr<const Chromatogram> original;
    // One entry per current non-gap symbol; -1 denotes an inserted symbol.
    std::vector<std::int32_t> originalCalls;
    bool reversed = false;
};
char trace_complement(char symbol) noexcept;
std::shared_ptr<const TraceLink> reversed_trace(const std::shared_ptr<const TraceLink>& link);
void validate_trace(const Sequence& row);
std::size_t trace_storage_bytes(const std::shared_ptr<const TraceLink>& link) noexcept;
std::shared_ptr<const TraceLink> edited_trace(const Sequence& row,std::size_t start,
                                             std::size_t count,const std::string& after);
struct TracePosition {
    double sample = 0;
    std::optional<std::size_t> originalCall;
    char current = '~', imported = '?';
    bool edited = false;
    std::optional<unsigned> quality;
};
// Synthetic positions are display anchors only: no original peak or quality is invented.
TracePosition trace_position(const Sequence& row,std::size_t column);
std::optional<std::size_t> trace_column(const Sequence& row,std::size_t originalCall);
}
