#pragma once
#include "core/alignment.hpp"
#include <functional>
namespace kleiotide {
using CollapseCancel=std::function<bool()>;
using CollapseProgress=std::function<void(std::size_t,std::size_t)>;
struct DuplicateCluster {std::vector<SequenceId> members;};
struct CollapsePlan {
    std::uint64_t revision=0;
    std::vector<DuplicateCluster> clusters;
    std::size_t scopedRows=0,eligibleRows=0,masksSkipped=0,removedRows=0;
};
enum class CollapseNaming { Concatenate, CountSuffix };
bool identical_ignoring_gaps(const Sequence&,const Sequence&,CollapseCancel={});
// All rows, or an explicit selected-name snapshot. Hash collisions are checked by exact comparison.
CollapsePlan plan_identical_collapse(const Document&,bool selectedOnly=false,
    const std::vector<SequenceId>& selected={},bool includeMasks=false,CollapseCancel={},CollapseProgress={});
std::string collapsed_title(const Document&,const DuplicateCluster&,SequenceId representative,
    CollapseNaming,std::string_view separator=";");
std::vector<Document::CollapseGroup> collapse_choices(const Document&,const CollapsePlan&,
    const std::vector<SequenceId>& representatives,CollapseNaming,std::string_view separator=";",CollapseCancel={});
void apply_identical_collapse(Document&,const CollapsePlan&,const std::vector<Document::CollapseGroup>&);
}
