#pragma once
#include "core/alignment.hpp"
#include <functional>
namespace kleiotide {
enum class IdentityAlignment { Existing, Pairwise };
using IdentityCancel=std::function<bool()>;
using IdentityProgress=std::function<void(std::size_t done,std::size_t total,std::uint64_t comparisons)>;
struct IdentityGroupOptions {
    IdentityAlignment alignment=IdentityAlignment::Existing;
    double minimumIdentity=97,minimumOverlap=80; // Percent; overlap covers the shorter trimmed sequence.
    // Pairwise only: shortlist this many centroids by shared unique words.
    // Zero preserves the exhaustive 0.26 centroid search. Fast search is heuristic.
    std::size_t maximumCandidates=16;
    bool selectedOnly=false;
    std::vector<SequenceId> selected;
    std::string prefix="Identity_";
};
struct IdentityPair {
    std::size_t matches=0,columns=0,paired=0,coveredFirst=0,coveredSecond=0;
    double identity()const noexcept{return columns?100.0*matches/columns:0;}
    bool valid()const noexcept{return paired!=0;}
};
// Exact case-insensitive symbols, U=T for nucleotides. N/? (protein X/?)
// are missing. End runs of missing calls/gaps are excluded; internal single
// gaps are differences, double gaps and columns with missing calls are omitted.
IdentityPair sequence_identity(const Sequence&,const Sequence&,IdentityAlignment,IdentityCancel={});
struct IdentityMember {
    SequenceId id=0,centroid=0;
    GroupId group=0; // Zero means skipped, leaving the old membership intact.
    IdentityPair pair;
    double overlap=0;
    std::string skipped;
};
struct IdentityGroupPlan {
    const Document* source=nullptr;
    std::uint64_t revision=0,comparisons=0,shortlistedQueries=0;
    std::vector<SequenceGroup> groups; // Full catalog: retained nonempty old entries plus new clusters.
    std::vector<IdentityMember> members; // Current alignment order, including skipped scoped rows.
    std::size_t clusters=0,assigned=0;
};
// Stable length-descending greedy centroid clustering. The first qualifying
// centroid wins (word-ranked in fast mode); only centroids are compared.
IdentityGroupPlan plan_identity_groups(const Document&,const IdentityGroupOptions&,IdentityCancel={},IdentityProgress={});
// UTF-8 TSV with quoted cells, including every preview row and stable IDs.
std::string identity_groups_tsv(const Document&,const IdentityGroupPlan&);
void apply_identity_groups(Document&,const IdentityGroupPlan&);
enum class GroupRepresentative { First, Longest, Random };
enum class UngroupedSelection { None, One, All };
std::vector<SequenceId> select_group_representatives(const Document&,GroupRepresentative,UngroupedSelection,std::uint64_t seed=0,IdentityCancel={});
// Returns IDs in current row order. Longest uses ungapped residue count,
// including unknown calls; equal lengths keep the first row. Empty groups skip.
std::vector<SequenceId> select_group_representatives(const Document&,GroupRepresentative,bool includeUngrouped=false,std::uint64_t seed=0,IdentityCancel={});
}
