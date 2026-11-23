#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string_view>
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
enum class CallEdit : std::uint8_t { FromEvidence, Manual, Automatic };
struct TraceLink {
    std::shared_ptr<const Chromatogram> original;
    // One entry per current non-gap symbol; -1 denotes an inserted symbol.
    std::vector<std::int32_t> originalCalls;
    // Optional original-signal sample anchors for inserted residues. These are
    // inferred positions, not original calls and never supply measured Q.
    std::vector<std::int32_t> inferredPeaks;
    bool reversed = false;
    // User-adjusted positions in original sample coordinates, independent of
    // the imported call indices and inferred anchors. -1 uses the default.
    std::vector<std::int32_t> adjustedPeaks;
    // Only needed when the edit history differs from comparison with the imported call.
    std::vector<CallEdit> callEdits;
};
bool manually_edited_call(const TraceLink&, std::size_t ordinal, char current);
char trace_complement(char symbol) noexcept;
std::optional<std::uint32_t> trace_sample_anchor(const TraceLink&,std::size_t ordinal);
std::vector<double> trace_sample_positions(const TraceLink&);
std::optional<std::uint32_t> bounded_trace_adjustment(const TraceLink&,const std::vector<double>&,
                                                      std::size_t ordinal,double sample);
std::shared_ptr<const TraceLink> adjusted_trace(const Sequence&,std::size_t ordinal,std::uint32_t sample);
std::shared_ptr<const TraceLink> inserted_trace_call(const Sequence&,std::size_t column,std::uint32_t sample);
void refine_trace_flanks(TraceLink&,std::string_view residues,const std::function<bool()>& cancelled={},std::size_t maxCells=1000000);
std::shared_ptr<const TraceLink> reversed_trace(const std::shared_ptr<const TraceLink>& link);
void validate_trace(const Sequence& row);
std::size_t trace_storage_bytes(const std::shared_ptr<const TraceLink>& link) noexcept;
std::shared_ptr<const TraceLink> edited_trace(const Sequence& row,std::size_t start,
                                             std::size_t count,const std::string& after, CallEdit origin = CallEdit::Manual);
struct TracePosition {
    double sample = 0;
    std::optional<std::size_t> originalCall;
    char current = '~', imported = '?';
    bool edited = false, inferred = false, adjusted = false;
    std::optional<unsigned> quality;
};
// Synthetic positions are display anchors only: no original peak or quality is invented.
TracePosition trace_position(const Sequence& row,std::size_t column);
std::optional<std::size_t> trace_column(const Sequence& row,std::size_t originalCall);
}
