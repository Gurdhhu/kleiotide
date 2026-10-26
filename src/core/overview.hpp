#pragma once
#include "core/site_summary.hpp"
#include <array>
namespace kleiotide {
struct OverviewBin {
    std::size_t begin=0,end=0; // Half-open column range.
    double residues=0,conservation=0;
    bool hasInformation=false;
};
// Cooperative scan on the GUI thread: no document copying or concurrent reads.
class OverviewScan {
public:
    explicit OverviewScan(const Document& document);
    bool step(const Document& document,std::size_t budget=16384);
    bool complete() const noexcept { return row_==rows_; }
    double progress() const noexcept;
    std::vector<OverviewBin> bins(std::size_t count) const;
    std::uint64_t revision() const noexcept { return revision_; }
    std::size_t columns() const noexcept { return columns_.size(); }
private:
    const Document* source_;
    using Column=NucleotideEvidence;
    std::vector<Column> columns_;
    std::size_t rows_=0,nucleotideRows_=0,row_=0,offset_=0,total_=0,processed_=0;
    std::uint64_t revision_=0;
};
}
