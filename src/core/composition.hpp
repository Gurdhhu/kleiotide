#pragma once
#include "core/site_summary.hpp"
#include <functional>
namespace kleiotide {
struct CompositionRow {
    SequenceId id=0;
    std::string name;
    std::array<std::uint64_t,4> votes{}; // Six units per scored call.
    std::size_t definite=0,ambiguous=0,unknown=0,openGaps=0,lockedGaps=0,trailing=0;
    std::uint64_t scored_units()const noexcept{return votes[0]+votes[1]+votes[2]+votes[3];}
};
struct NucleotideComposition {
    AnalysisScope scope;
    bool fractional=true;
    std::size_t begin=0,end=0;
    std::vector<CompositionRow> rows;
    CompositionRow pooled;
};
NucleotideComposition nucleotide_composition(const Document&,AnalysisScope,bool fractional=true,const std::function<bool()>& cancelled={});
std::vector<std::string> composition_headers();
std::vector<std::string> composition_cells(const CompositionRow&);
std::string composition_tsv(const NucleotideComposition&,const std::function<bool()>& cancelled={});
}
