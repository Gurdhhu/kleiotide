#pragma once
#include "core/site_summary.hpp"
#include <array>
namespace kleiotide {
struct OverviewBin {
    std::size_t begin=0,end=0;
    double residues=0,conservation=0;
    bool hasInformation=false;
};
// Cooperative, revision-checked scan. Workspace is independent of sequence length.
// Conservation is computed per column before folding into bounded summaries.
class OverviewScan {
public:
    explicit OverviewScan(const Document&,std::size_t workspaceBytes=16*1024*1024,std::size_t maxBins=8192);
    bool step(const Document&,std::size_t budget=16384);
    bool complete() const noexcept {return complete_;}
    double progress() const noexcept;
    std::vector<OverviewBin> bins(std::size_t count) const;
    std::uint64_t revision() const noexcept {return revision_;}
    std::size_t columns() const noexcept {return columns_;}
    std::size_t workspace_bytes() const noexcept;
private:
    struct Summary {std::size_t begin=0,end=0,informative=0;double residues=0,conservation=0;};
    const Document* source_;
    std::vector<NucleotideEvidence> tile_;
    std::vector<Summary> summaries_;
    std::vector<std::size_t> rowsByLength_;
    std::size_t rows_=0,columns_=0,tileBegin_=0,tileEnd_=0,row_=0,offset_=0,fold_=0,bin_=0;
    long double total_=0,processed_=0;
    std::uint64_t revision_=0;
    bool folding_=false,complete_=false;
};
}
