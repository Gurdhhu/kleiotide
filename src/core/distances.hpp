#pragma once
#include "core/site_summary.hpp"
#include <functional>
#include <ostream>
#include <optional>
#include <string_view>
namespace kleiotide {
enum class DistanceMethod { Differences, PDistance, JukesCantor, Kimura2, Poisson };
enum class DistanceDeletion { Pairwise, Complete };
enum class DistanceReport { Matrix, Overall, WithinGroups, BetweenGroups };
enum class DistanceFailure { None, NoOverlap, BelowMinimum, Saturated };
struct DistanceOptions {
    AnalysisScope scope;
    DistanceMethod method=DistanceMethod::PDistance;
    DistanceDeletion deletion=DistanceDeletion::Pairwise;
    std::size_t minimumSites=1;
    bool includeUngrouped=true;
};
using DistanceCancel=std::function<bool()>;
using DistanceProgress=std::function<void(std::uint64_t,std::uint64_t)>;
struct PairDistance {
    std::size_t comparable=0,differences=0,transitions=0;
    double value=0;
    DistanceFailure failure=DistanceFailure::NoOverlap;
    bool valid()const{return failure==DistanceFailure::None;}
};
struct DistanceSequence {SequenceId id=0;std::string name;GroupId group=0;};
struct DistanceGroup {GroupId id=0;std::string name;std::size_t rows=0;};
struct MeanDistance {
    std::size_t first=0,second=0;
    std::uint64_t valid=0,noOverlap=0,belowMinimum=0,saturated=0;
    long double sum=0,compensation=0,comparableSum=0;
    long double varianceMean=0,squaredDeviations=0; // Welford accumulator, valid pairs only.
    std::size_t minimumComparable=0,maximumComparable=0;
    std::uint64_t excluded()const{return noOverlap+belowMinimum+saturated;}
    double value()const{return valid?static_cast<double>(sum/static_cast<long double>(valid)):0;}
    std::optional<double> sample_sd()const;
    std::optional<double> naive_se()const;
};
struct DistanceResult {
    DistanceOptions options;DistanceReport report=DistanceReport::Matrix;
    std::vector<DistanceSequence> rows;std::vector<DistanceGroup> groups;
    std::size_t begin=0,end=0,completeSites=0;
    std::vector<PairDistance> matrix; // Lower triangle including diagonal.
    std::vector<MeanDistance> means;
    const PairDistance& pair(std::size_t,std::size_t)const;
};
struct DistanceEstimate {std::size_t rows=0,groups=0,begin=0,end=0;std::uint64_t pairs=0,bytes=0;};
inline constexpr std::uint64_t distance_memory_limit=128ULL*1024*1024;
std::uint64_t distance_pair_count(std::size_t);
DistanceEstimate estimate_distances(const Document&,const DistanceOptions&,DistanceReport,const DistanceCancel& = {});
PairDistance correct_distance(std::size_t comparable,std::size_t differences,std::size_t transitions,const DistanceOptions&);
DistanceResult calculate_distances(const Document&,DistanceOptions,DistanceReport,const DistanceCancel& = {},const DistanceProgress& = {});
struct SimilarityOrder {std::vector<SequenceId> order;std::size_t eligible=0,uncomparable=0;};
SimilarityOrder similarity_order(const Document&,DistanceOptions,const DistanceCancel& = {},const DistanceProgress& = {});
std::string distance_method_name(DistanceMethod);
std::string distance_failure_name(DistanceFailure);
std::string distance_number(double);
void write_distance_tsv(const DistanceResult&,const std::function<void(std::string_view)>&,const DistanceCancel& = {});
std::string distance_tsv(const DistanceResult&,std::size_t maximumBytes=64*1024*1024,const DistanceCancel& = {});
}
