#pragma once
#include "core/alignment.hpp"
#include <functional>
#include <iosfwd>

namespace kleiotide {
enum class MafftAlgorithm { Auto, Fast1, Fast2, Iterative, Local, Global, LargeGaps, PartTree };
enum class MafftDirection { Keep, Adjust, Accurate };
enum class ProteinScoring { Blosum, Jtt, Transmembrane };
// Explicit row types decide the alphabet, never the frequency of A/C/G/T. Masks are skipped.
RowKind mafft_molecule(const Document&,const std::vector<SequenceId>& ids={});
struct MafftOptions {
    MafftAlgorithm algorithm=MafftAlgorithm::Auto;
    MafftDirection direction=MafftDirection::Keep;
    int threads=-1,iterations=1000,retree=2,kimura=200;
    double gapOpen=1.53,offset=.123,kappa=2;
    bool composition=false,memorySaving=false,deterministic=true;
    RowKind molecule=RowKind::Nucleotide;
    ProteinScoring proteinScoring=ProteinScoring::Blosum;
    int blosum=62,proteinPam=200;
    std::vector<std::string> arguments() const;
};
struct MafftResult {
    std::vector<Record> records;
    std::vector<SequenceId> reversed;
    std::size_t fragmentWidth=0;
};
// The source is immutable throughout a job. No alignment-sized snapshot is made.
class MafftAlignment {
public:
    using Cancel=std::function<bool()>;
    MafftAlignment(const Document&,std::optional<Document::ColumnRange> = {},Cancel = {});
    void write_input(std::ostream&,Cancel={}) const;
    MafftResult merge(std::vector<Record>,bool allowReverse=false,Cancel={}) const;
    RowKind molecule() const noexcept {return molecule_;}
    std::size_t input_count() const noexcept {return inputs_.size();}
    std::size_t mask_count() const noexcept {return masks_;}
    std::optional<Document::ColumnRange> range() const noexcept {return range_;}
    std::uint64_t revision() const noexcept {return revision_;}
private:
    void check_source() const;
    std::string_view region(const Sequence&) const;
    RowKind molecule_;
    const Document* source_;
    std::uint64_t revision_;
    std::optional<Document::ColumnRange> range_;
    std::vector<std::size_t> inputs_;
    std::size_t masks_=0,minimumWidth_=0;
};
}
