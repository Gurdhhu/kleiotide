#pragma once
#include "core/mafft_alignment.hpp"

namespace kleiotide {
struct MafftTemplateRequest {
    std::vector<SequenceId> targets,templates;
    bool fragments=false;
};
struct MafftTemplateResult {
    MafftResult alignment;
    // Exact projection of the complete original axis into the result.
    std::vector<std::size_t> originalColumns;
    std::size_t insertedColumns=0;
};
class MafftTemplateAlignment {
public:
    using Cancel=MafftAlignment::Cancel;
    MafftTemplateAlignment(const Document&,MafftTemplateRequest,Cancel={});
    void write_templates(std::ostream&,Cancel={}) const;
    void write_targets(std::ostream&,Cancel={}) const;
    MafftTemplateResult merge(std::vector<Record>,Cancel={},bool allowReverse=false) const;
    RowKind molecule() const noexcept {return molecule_;}
    std::size_t target_count() const noexcept {return targets_.size();}
    std::size_t template_count() const noexcept {return templates_.size();}
    std::size_t restored_columns() const noexcept {return originalWidth_-anchors_.size();}
    std::uint64_t revision() const noexcept {return revision_;}
private:
    void check_source() const;
    RowKind molecule_=RowKind::Nucleotide;
    const Document* source_;
    std::uint64_t revision_;
    std::size_t originalWidth_;
    std::vector<std::size_t> targets_,templates_,anchors_;
};
}
