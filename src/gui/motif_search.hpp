#pragma once
#include "core/motif_index.hpp"
#include <memory>
namespace kleiotide {
struct MotifSearchResult {std::shared_ptr<const MotifIndex> index;std::uint64_t emptyMatches=0;};
MotifSearchResult search_motifs(const Document&,std::string_view,MotifOptions,bool regex,
    const std::optional<std::vector<SequenceId>>& scope={},const std::function<bool()>& cancelled={},bool excludeNOnly=false);
}
