#pragma once
#include "core/alignment.hpp"
#include <functional>
namespace kleiotide {
struct MotifOptions { bool ignoreGaps=true, reverseComplement=false, compatibleIupac=false; };
struct MotifMatch { std::size_t start=0, width=0; bool reversed=false,bothStrands=false; };
struct MotifLocation {SequenceId id=0;std::size_t start=0;};
struct MotifHit {SequenceId id=0;MotifMatch match;};
struct MotifNavigation {std::optional<MotifHit> hit;bool wrapped=false;};
void scan_motifs(const Document&,std::string_view,MotifOptions,
    const std::function<void(MotifHit)>&,
    const std::optional<std::vector<SequenceId>>& scope={},const std::function<bool()>& cancelled={});
// Navigate row order, then increasing start columns, including overlaps. Null
// scope means all nucleotide rows; an explicit empty scope means no rows.
// Only one hit and O(motif length) matcher storage are retained, not a hit list.
MotifNavigation navigate_motif(const Document&,std::string_view,MotifOptions={},
    std::optional<MotifLocation> after={},bool backwards=false,bool wrap=true,
    const std::optional<std::vector<SequenceId>>& scope={},
    const std::function<bool()>& cancelled={});
// Motifs are case-insensitive. Coordinates are always actual alignment columns.
// Exact mode matches literal IUPAC codes; compatibility mode intersects base sets.
std::optional<MotifMatch> find_motif(const Sequence&,std::string_view,MotifOptions={});
std::vector<SequenceId> select_motif(const Document&,std::string_view,MotifOptions={});
std::vector<SequenceId> select_duplicate_sequences(const Document&,bool ignoreGaps,bool additionalOnly);
std::vector<SequenceId> select_random_rows(const Document&,std::size_t count,std::uint64_t seed);
}
