#pragma once
#include "core/alignment.hpp"
#include <functional>
namespace kleiotide {
struct TraceMapping {
    std::shared_ptr<const TraceLink> link;
    std::string orientation="No match",quality="Failed",reason;
    double identity=0;std::size_t matches=0,aligned=0;
};
// Local affine alignment, free terminal sequence, immutable original-call indices.
// Ambiguous orientations/repeat placements fail rather than guessing peak indices.
TraceMapping map_trace(std::string_view,std::shared_ptr<const Chromatogram>,
    const std::function<bool()>& cancelled={},std::size_t maxCells=8000000);
int suggest_trace(std::string_view name,const std::vector<std::string>& fileStems,
    const std::vector<std::string>& internalNames); // -1: none or ambiguous best tier
void apply_trace_links(Document&,const std::vector<std::pair<SequenceId,std::shared_ptr<const TraceLink>>>&);
}
