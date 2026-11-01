#pragma once
#include "core/alignment.hpp"
#include <functional>

namespace kleiotide {
struct AlignmentStatistics {
    RowKind molecule=RowKind::Nucleotide;
    std::size_t sequences=0, uniqueSequences=0, columns=0, patterns=0;
    std::size_t informative=0, singleton=0, constant=0, gapOnly=0, nOnly=0;
};
// IQ-TREE nucleotide state/counting conventions, excluding masks. Ragged ends
// are missing data. Gap-only and N-only sites are subsets of constant sites.
AlignmentStatistics alignment_statistics(const Document& document,
    const std::function<bool()>& cancelled = {}, RowKind molecule = RowKind::Nucleotide);
std::string statistics_text(const AlignmentStatistics& statistics);
}
