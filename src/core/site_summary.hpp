#pragma once
#include "core/alignment.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <variant>

namespace kleiotide {
// Missing calls have every possible state, but cast no conservation votes.
unsigned nucleotide_state(char c) noexcept;
unsigned protein_state(char c) noexcept;
struct NucleotideEvidence {
    std::uint32_t residues=0;
    std::array<std::uint32_t,4> bases{};
    void add(char c) noexcept;
};
template<std::size_t N> struct SiteColumn {
    static constexpr unsigned all=(1U<<N)-1;
    std::array<std::uint32_t,N> votes{};
    std::uint32_t residues=0,information=0;
    unsigned intersection=all,seen=0,repeated=0;
    void add(char c) noexcept {
        if(is_gap(c))return;
        ++residues;
        const auto state=N==4?nucleotide_state(c):protein_state(c);
        intersection &= state;
        if(state==all)return;
        ++information;
        const auto count=std::popcount(state);
        if(count==1){repeated |= seen & state;seen |= state;}
        // All supported ambiguities contain 2 or 3 states: six units per call.
        for(auto bits=state;bits;bits &= bits-1)votes[std::countr_zero(bits)]+=6/count;
    }
    bool constant()const noexcept{return intersection!=0;}
    bool informative()const noexcept{return std::popcount(repeated)>=2;}
    bool singleton()const noexcept{return !constant()&&!informative();}
    double conservation()const noexcept{
        return information?static_cast<double>(*std::max_element(votes.begin(),votes.end()))/(6.0*information):0;
    }
};
using NucleotideColumn=SiteColumn<4>;
using ProteinColumn=SiteColumn<23>; // 20 standard amino acids, U, O and stop.
struct AnalysisScope {
    RowKind molecule=RowKind::Nucleotide;
    bool selectedRows=false;
    std::vector<SequenceId> ids; // Explicit snapshot, independent of later name selection.
    bool selectedColumns=false;
    std::size_t begin=0,end=0; // Half-open range, fixed alignment coordinates.
};
struct HighlightRules {
    bool conserved=false,variable=false,informative=false,singleton=false;
    bool coverage=false,conservation=false,informativeCoverage=false;
    double coverageMin=0,coverageMax=100,conservationMin=80,conservationMax=100;
    bool enabled()const noexcept{return conserved||variable||informative||singleton||coverage||conservation;}
};
template<std::size_t N>bool highlighted(const SiteColumn<N>& c,std::size_t rows,const HighlightRules& r){
    if(!rows)return false;
    // No-information sites are not called conserved, even though compatibility
    // statistics count them as constant. Threshold coverage can still select them.
    const auto cover=100.0*(r.informativeCoverage?c.information:c.residues)/rows;
    const auto conservation=100*c.conservation();
    return (r.conserved&&c.information&&c.constant())||(r.variable&&!c.constant())
        ||(r.informative&&c.informative())||(r.singleton&&c.singleton())
        ||(r.coverage&&cover+1e-9>=r.coverageMin&&cover<=r.coverageMax+1e-9)
        ||(r.conservation&&c.information&&conservation+1e-9>=r.conservationMin&&conservation<=r.conservationMax+1e-9);
}
// Bounded cooperative scan. No SequenceBuffer rank caches or concurrent paints.
class SiteScan {
public:
    SiteScan(const Document&,AnalysisScope);
    bool step(const Document&,std::size_t budget=4096);
    bool complete()const noexcept{return row_==rows_.size();}
    std::vector<unsigned char> highlights(const HighlightRules&)const;
    std::size_t row_count()const noexcept{return rows_.size();}
    std::uint64_t revision()const noexcept{return revision_;}
    const AnalysisScope& scope()const noexcept{return scope_;}
private:
    const Document* source_;
    std::uint64_t revision_;
    AnalysisScope scope_;
    std::vector<std::size_t> rows_;
    std::variant<std::vector<NucleotideColumn>,std::vector<ProteinColumn>> columns_;
    std::size_t row_=0,column_=0,width_=0;
};
}
