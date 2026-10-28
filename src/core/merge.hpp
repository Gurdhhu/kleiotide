#pragma once
#include "core/alignment.hpp"
#include <functional>
namespace kleiotide {
enum class MergeMode { Aligned, ExactOverlap };
struct MergeSettings {
    MergeMode mode=MergeMode::Aligned;
    std::size_t minimumOverlap=20; // Informative matching positions, not unknowns.
    bool resolveQuality=false;
    unsigned qualityMargin=10;
    unsigned maximumEdits=0; // Per added read: substitutions + inserted/deleted bases.
};
struct MergePlacement {SequenceId id=0;std::size_t start=0,length=0;};
struct MergeConflict {std::size_t column=0;bool resolvedByQuality=false;};
struct MergeAlignedRow {SequenceId id=0;std::size_t begin=0,end=0;std::string symbols;};
struct MergeContig {
    std::vector<SequenceId> ids;
    std::string symbols;
    std::size_t overlappingPositions=0,conflictingPositions=0;
    std::size_t residueConflicts=0,gapConflicts=0,qualityResolved=0;
    std::vector<MergePlacement> placements;
    std::vector<MergeConflict> conflicts;
    // Original reads in final contig coordinates; exterior padding is no evidence.
    std::vector<MergeAlignedRow> alignment;
    std::vector<unsigned char> differences;
    std::size_t editsUsed=0;
    // Search events across read additions, distinct from final conflict columns.
    std::size_t substitutionsUsed=0,indelBasesUsed=0,compatibleDifferencesUsed=0;
};
struct MergeOmission {SequenceId id=0;std::string reason;};
struct MergePlan:MergeContig {
    const Document* source=nullptr;
    std::uint64_t revision=0;
    MergeSettings settings;
    std::vector<SequenceId> selectedIds;
    std::vector<MergeContig> contigs;
    std::vector<MergeOmission> omitted;
    std::size_t chosenContig=0;
};
using MergeCancel=std::function<bool()>;
// Selected nucleotide rows only, ordered as in the document. Preview is read-only.
MergePlan plan_sequence_merge(const Document&,const std::vector<SequenceId>&,
    MergeSettings={},MergeCancel={});
// Appends one ungrouped consensus without a fabricated single-read trace.
SequenceId apply_sequence_merge(Document&,const MergePlan&,std::string name);
// Select one reviewed component; omitted rows never contribute to this result.
void choose_merge_contig(MergePlan&,std::size_t index);
}
