#include "core/distances.hpp"
#include "core/version.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace kleiotide {
namespace {
void check(const DistanceCancel& stop){if(stop&&stop())throw std::runtime_error("Calculation cancelled.");}
char state(char c,RowKind kind){if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');if(kind==RowKind::Nucleotide){if(c=='U')c='T';return std::string_view("ACGT").find(c)!=std::string_view::npos?c:0;}return std::string_view("ACDEFGHIKLMNPQRSTVWY").find(c)!=std::string_view::npos?c:0;}
std::size_t triangle(std::size_t i,std::size_t j){if(i<j)std::swap(i,j);return i*(i+1)/2+j;}
void validate(const DistanceOptions& o){
    if(o.scope.molecule!=RowKind::Nucleotide&&o.scope.molecule!=RowKind::Protein)throw std::invalid_argument("Choose nucleotide or protein sequences.");
    if(!o.minimumSites)throw std::invalid_argument("Minimum comparable sites must be at least 1.");
    if(o.scope.selectedRows&&o.scope.ids.empty())throw std::invalid_argument("No sequence names selected. Choose all rows explicitly or select names first.");
    if((o.method==DistanceMethod::JukesCantor||o.method==DistanceMethod::Kimura2)&&o.scope.molecule!=RowKind::Nucleotide)throw std::invalid_argument("This model requires nucleotide sequences.");
    if(o.method==DistanceMethod::Poisson&&o.scope.molecule!=RowKind::Protein)throw std::invalid_argument("Poisson correction requires protein sequences.");
}
struct Input {
    DistanceOptions options;std::vector<const Sequence*> rows;std::vector<DistanceGroup> groups;std::vector<std::size_t> group;
    std::size_t begin=0,end=0;std::vector<unsigned char> complete;std::size_t completeSites=0;
    Input(const Document& d,DistanceOptions o,const DistanceCancel& stop={},bool prepare=true):options(std::move(o)){
        validate(options);check(stop);std::unordered_set<SequenceId> chosen(options.scope.ids.begin(),options.scope.ids.end());
        if(options.scope.selectedRows)for(auto id:chosen)(void)d.sequence(id);
        std::set<GroupId> present;std::size_t width=0;
        for(const auto& r:d.rows()){check(stop);if(r.kind!=options.scope.molecule||(options.scope.selectedRows&&!chosen.contains(r.id))||(!options.includeUngrouped&&!r.groupId))continue;rows.push_back(&r);width=std::max(width,r.symbols.size());present.insert(r.groupId);}
        if(rows.size()<2)throw std::invalid_argument("At least two sequences of the chosen molecule type are required in this scope. Masks and the other molecule type are excluded.");
        begin=options.scope.selectedColumns?options.scope.begin:0;end=options.scope.selectedColumns?options.scope.end:width;
        if(begin>=end||end>d.columns())throw std::invalid_argument("Choose a nonempty column range within the alignment.");
        for(const auto& g:d.groups())if(present.contains(g.id))groups.push_back({g.id,g.name,0});
        if(present.contains(0))groups.push_back({0,"Ungrouped",0});
        std::unordered_map<GroupId,std::size_t> indices;for(std::size_t i=0;i<groups.size();++i)indices[groups[i].id]=i;
        for(auto r:rows){const auto i=indices.at(r->groupId);group.push_back(i);++groups[i].rows;}
        if(prepare&&options.deletion==DistanceDeletion::Complete){
            if(end-begin>distance_memory_limit)throw std::length_error("Complete-deletion site filter exceeds 128 MiB. Select fewer columns or use pairwise deletion.");
            check(stop);complete.assign(end-begin,1);
            for(auto r:rows){const auto& text=r->symbols.text();for(auto c=begin;c<end;++c){if((c&4095)==0)check(stop);if(c>=text.size()||!state(text[c],options.scope.molecule))complete[c-begin]=0;}}
            completeSites=static_cast<std::size_t>(std::count(complete.begin(),complete.end(),1));
        }
    }
    PairDistance pair(std::size_t i,std::size_t j,const DistanceCancel& stop)const{
        check(stop);const auto& a=rows[i]->symbols.text();const auto& b=rows[j]->symbols.text();std::size_t n=0,diff=0,ts=0;
        const auto last=std::min({end,a.size(),b.size()});
        for(auto c=begin;c<last;++c){if((c&4095)==0)check(stop);if(options.deletion==DistanceDeletion::Complete&&!complete[c-begin])continue;const char x=state(a[c],options.scope.molecule),y=state(b[c],options.scope.molecule);if(!x||!y)continue;++n;if(x!=y){++diff;ts+=options.scope.molecule==RowKind::Nucleotide&&((x=='A'&&y=='G')||(x=='G'&&y=='A')||(x=='C'&&y=='T')||(x=='T'&&y=='C'));}}
        return correct_distance(n,diff,ts,options);
    }
};
void add(MeanDistance& mean,const PairDistance& pair){
    switch(pair.failure){case DistanceFailure::NoOverlap:++mean.noOverlap;return;case DistanceFailure::BelowMinimum:++mean.belowMinimum;return;case DistanceFailure::Saturated:++mean.saturated;return;default:break;}
    const auto next=static_cast<long double>(mean.valid)+1;
    const auto delta=static_cast<long double>(pair.value)-mean.varianceMean;
    mean.varianceMean+=delta/next;
    mean.squaredDeviations+=delta*(static_cast<long double>(pair.value)-mean.varianceMean);
    const long double y=static_cast<long double>(pair.value)-mean.compensation,t=mean.sum+y;mean.compensation=(t-mean.sum)-y;mean.sum=t;
    mean.minimumComparable=mean.valid?std::min(mean.minimumComparable,pair.comparable):pair.comparable;mean.maximumComparable=std::max(mean.maximumComparable,pair.comparable);mean.comparableSum+=static_cast<long double>(pair.comparable);++mean.valid;
}
std::string safe(std::string s){for(auto& c:s)if(c=='\t'||c=='\n'||c=='\r')c=' ';return s;}
}
std::optional<double> MeanDistance::sample_sd()const{
    if(valid<2)return std::nullopt;
    return static_cast<double>(std::sqrt(std::max(0.L,squaredDeviations)/static_cast<long double>(valid-1)));
}
std::optional<double> MeanDistance::naive_se()const{
    const auto sd=sample_sd();if(!sd)return std::nullopt;
    return *sd/std::sqrt(static_cast<double>(valid));
}
std::uint64_t distance_pair_count(std::size_t count){const auto n=static_cast<std::uint64_t>(count);if(n<2)return 0;auto a=n,b=n-1;if(a%2==0)a/=2;else b/=2;if(a>std::numeric_limits<std::uint64_t>::max()/b)throw std::length_error("Too many sequence pairs.");return a*b;}
DistanceEstimate estimate_distances(const Document& d,const DistanceOptions& o,DistanceReport report,const DistanceCancel& stop){
    Input in(d,o,stop,false);DistanceEstimate e{in.rows.size(),in.groups.size(),in.begin,in.end,distance_pair_count(in.rows.size()),0};
    const auto cells=report==DistanceReport::Matrix?e.pairs+e.rows:report==DistanceReport::BetweenGroups?distance_pair_count(e.groups):report==DistanceReport::WithinGroups?e.groups:1;
    const auto cellBytes=report==DistanceReport::Matrix?sizeof(PairDistance):sizeof(MeanDistance);
    if(cells>std::numeric_limits<std::uint64_t>::max()/cellBytes)throw std::length_error("Distance output is too large.");
    e.bytes=cells*cellBytes;
    if(e.bytes>distance_memory_limit)throw std::length_error("Output exceeds 128 MiB. Select fewer sequence names or use overall/within-group means instead of a matrix.");
    return e;
}
PairDistance correct_distance(std::size_t n,std::size_t diff,std::size_t ts,const DistanceOptions& o){
    if(diff>n||ts>diff)throw std::invalid_argument("Invalid difference counts.");
    PairDistance r{n,diff,ts,0,DistanceFailure::None};
    if(!n){r.failure=DistanceFailure::NoOverlap;return r;}if(n<o.minimumSites){r.failure=DistanceFailure::BelowMinimum;return r;}
    const double p=static_cast<double>(diff)/static_cast<double>(n);
    switch(o.method){
    case DistanceMethod::Differences:r.value=static_cast<double>(diff);break;
    case DistanceMethod::PDistance:r.value=p;break;
    case DistanceMethod::JukesCantor:if(p>=0.75)r.failure=DistanceFailure::Saturated;else r.value=-0.75*std::log1p(-4*p/3);break;
    case DistanceMethod::Kimura2:{const double P=static_cast<double>(ts)/static_cast<double>(n),Q=static_cast<double>(diff-ts)/static_cast<double>(n);if(2*P+Q>=1||2*Q>=1)r.failure=DistanceFailure::Saturated;else r.value=-0.5*std::log1p(-2*P-Q)-0.25*std::log1p(-2*Q);break;}
    case DistanceMethod::Poisson:if(diff==n)r.failure=DistanceFailure::Saturated;else r.value=-std::log1p(-p);break;
    }
    if(r.value==0)r.value=0;
    return r;
}
const PairDistance& DistanceResult::pair(std::size_t i,std::size_t j)const{if(report!=DistanceReport::Matrix||i>=rows.size()||j>=rows.size())throw std::out_of_range("Distance matrix index.");return matrix.at(triangle(i,j));}
DistanceResult calculate_distances(const Document& d,DistanceOptions options,DistanceReport report,const DistanceCancel& stop,const DistanceProgress& progress){
    check(stop);const auto estimate=estimate_distances(d,options,report,stop);Input in(d,options,stop);DistanceResult out;out.options=std::move(options);out.report=report;out.begin=in.begin;out.end=in.end;out.groups=in.groups;out.completeSites=in.completeSites;
    for(auto r:in.rows)out.rows.push_back({r->id,r->name,r->groupId});
    if(report==DistanceReport::Matrix)out.matrix.resize(static_cast<std::size_t>(estimate.pairs+estimate.rows));
    else if(report==DistanceReport::Overall)out.means.resize(1);
    else if(report==DistanceReport::WithinGroups){out.means.resize(in.groups.size());for(std::size_t i=0;i<out.means.size();++i)out.means[i].first=out.means[i].second=i;}
    else {for(std::size_t i=0;i<in.groups.size();++i)for(std::size_t j=0;j<i;++j){MeanDistance m;m.first=j;m.second=i;out.means.push_back(m);}}
    std::uint64_t done=0;if(progress)progress(done,estimate.pairs);
    for(std::size_t i=0;i<in.rows.size();++i){check(stop);if(report==DistanceReport::Matrix)out.matrix[triangle(i,i)]=in.pair(i,i,stop);
        for(std::size_t j=0;j<i;++j){check(stop);const auto a=in.group[i],b=in.group[j];const bool relevant=report==DistanceReport::Matrix||report==DistanceReport::Overall||(report==DistanceReport::WithinGroups?a==b:a!=b);
            if(relevant){const auto pair=in.pair(i,j,stop);if(report==DistanceReport::Matrix)out.matrix[triangle(i,j)]=pair;else if(report==DistanceReport::Overall)add(out.means[0],pair);else if(report==DistanceReport::WithinGroups)add(out.means[a],pair);else{const auto hi=std::max(a,b),lo=std::min(a,b);add(out.means[hi*(hi-1)/2+lo],pair);}}
            ++done;if(progress)progress(done,estimate.pairs);
        }
    }return out;
}
SimilarityOrder similarity_order(const Document& d,DistanceOptions options,const DistanceCancel& stop,const DistanceProgress& progress){
    options.method=DistanceMethod::Differences;Input in(d,options,stop);const auto n=in.rows.size(),pairs=distance_pair_count(n);if(pairs>std::numeric_limits<std::uint64_t>::max()/2)throw std::length_error("Too many similarity comparisons.");std::uint64_t done=0;const auto total=2*pairs;std::vector<unsigned char> partner(n),used(n);if(progress)progress(0,total);
    // A first streaming pass identifies rows with no valid comparison anywhere.
    // This keeps them last without retaining a quadratic matrix.
    for(std::size_t i=0;i<n;++i)for(std::size_t j=0;j<i;++j){if(in.pair(i,j,stop).valid())partner[i]=partner[j]=1;++done;if(progress)progress(done,total);}
    std::vector<std::size_t> chain{0};used[0]=1;
    while(chain.size()<n){check(stop);std::optional<std::size_t> best;PairDistance bestPair;const auto current=chain.back();
        for(std::size_t j=1;j<n;++j)if(!used[j]&&partner[j]){const auto p=in.pair(current,j,stop);++done;if(progress)progress(std::min(done,total),total);if(p.valid()&&(!best||p.differences<bestPair.differences||(p.differences==bestPair.differences&&p.comparable>bestPair.comparable))){best=j;bestPair=p;}}
        if(!best){for(std::size_t j=1;j<n;++j)if(!used[j]&&partner[j]){best=j;break;}}
        if(!best){for(std::size_t j=1;j<n;++j)if(!used[j]){chain.push_back(j);used[j]=1;}break;}chain.push_back(*best);used[*best]=1;
    }
    SimilarityOrder out;out.eligible=n;out.uncomparable=static_cast<std::size_t>(std::count(partner.begin(),partner.end(),0));std::unordered_set<SequenceId> eligible;for(auto r:in.rows)eligible.insert(r->id);std::size_t at=0;for(const auto& row:d.rows())out.order.push_back(eligible.contains(row.id)?in.rows[chain[at++]]->id:row.id);if(progress)progress(total,total);return out;
}
std::string distance_method_name(DistanceMethod m){switch(m){case DistanceMethod::Differences:return "Observed differences";case DistanceMethod::PDistance:return "p-distance";case DistanceMethod::JukesCantor:return "Jukes-Cantor";case DistanceMethod::Kimura2:return "Kimura 2-parameter";case DistanceMethod::Poisson:return "Poisson correction";}return {};}
std::string distance_method_description(DistanceMethod m){
    switch(m){
    case DistanceMethod::Differences:return "Observed differences = D, the number of unequal definite residues among n comparable sites. No length normalization or multiple-hit correction. Short overlaps can give smaller counts; use Minimum comparable sites to require sufficient overlap.";
    case DistanceMethod::PDistance:return "p-distance = D/n, where D is the observed difference count and n is the number of comparable sites. Units: fraction of differing sites (0 to 1), not a percentage. No correction for multiple substitutions at the same site.";
    case DistanceMethod::JukesCantor:return "Jukes-Cantor distance = -3/4 ln(1 - 4p/3), where p = D/n. Assumes equal base frequencies, equal rates for all nucleotide substitutions and uniform rates among sites. Units: estimated substitutions per site. N/A when p >= 0.75 (model saturation).";
    case DistanceMethod::Kimura2:return "Kimura 2-parameter distance = -1/2 ln(1 - 2P - Q) - 1/4 ln(1 - 2Q). P is the fraction of transitions (A-G or C-T); Q is the fraction of transversions among comparable sites. Assumes equal base frequencies and uniform rates among sites, allowing different transition/transversion rates. Units: estimated substitutions per site. N/A when either logarithm argument is <= 0.";
    case DistanceMethod::Poisson:return "Poisson correction = -ln(1 - p), where p = D/n for definite amino acids. Assumes uniform substitution rates among sites and a Poisson process. Units: estimated substitutions per site. N/A at p = 1 (model saturation). No amino-acid exchangeability matrix or composition correction is used.";
    }return {};
}
std::string distance_failure_name(DistanceFailure f){switch(f){case DistanceFailure::None:return {};case DistanceFailure::NoOverlap:return "No comparable sites";case DistanceFailure::BelowMinimum:return "Below minimum comparable sites";case DistanceFailure::Saturated:return "Model saturation (logarithm outside domain)";}return {};}
std::string distance_number(double n){std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(12)<<n;return out.str();}
void write_distance_tsv(const DistanceResult& r,const std::function<void(std::string_view)>& sink,const DistanceCancel& stop){
    auto emit=[&](const std::string& line){check(stop);sink(line);};auto line=[&](const std::vector<std::string>& cells){std::string text;for(std::size_t i=0;i<cells.size();++i){if(i)text+='\t';text+=safe(cells[i]);}text+='\n';emit(text);};
    emit(std::string("# Kleiotide distances\n# Kleiotide version: ")+version+"\n# Molecule: "+std::string(r.options.scope.molecule==RowKind::Nucleotide?"Nucleotide":"Protein")+"; model: "+distance_method_name(r.options.method)+"; units: "+(r.options.method==DistanceMethod::Differences?"differences":"substitutions per site (p-distance is observed fraction)")+"\n");
    emit("# Rows: "+std::string(r.options.scope.selectedRows?"selected-name snapshot":"all rows of chosen molecule")+"; sequences: "+std::to_string(r.rows.size())+"; columns (1-based inclusive): "+std::to_string(r.begin+1)+"-"+std::to_string(r.end)+"\n# Deletion: "+(r.options.deletion==DistanceDeletion::Pairwise?"pairwise":"complete across every included row")+"; minimum comparable sites: "+std::to_string(r.options.minimumSites)+"; ungrouped: "+(r.options.includeUngrouped?"included":"excluded")+"\n");
    emit("# Only definite A/C/G/T (U=T), or the 20 standard amino acids. All ambiguities, gaps, missing cells, protein U/O and stops are excluded. Masks/other molecule type are excluded.\n# Means are unweighted arithmetic means of valid pair distances. N/A pairs are excluded and counted. No bootstrap or rate variation.\n");
    if(r.options.deletion==DistanceDeletion::Complete)emit("# Complete-deletion columns retained: "+std::to_string(r.completeSites)+"\n");
    emit("# Groups in analysis (0 = no assigned group)\n");line({"Group ID","Group name","Eligible rows"});std::unordered_map<GroupId,std::string> names;for(const auto& g:r.groups){names[g.id]=g.name;line({std::to_string(g.id),g.name,std::to_string(g.rows)});}
    line({"Sequence index","Sequence ID","Sequence name","Group ID","Group name"});for(std::size_t i=0;i<r.rows.size();++i)line({std::to_string(i+1),std::to_string(r.rows[i].id),r.rows[i].name,std::to_string(r.rows[i].group),names.at(r.rows[i].group)});
    if(r.report==DistanceReport::Matrix){
        for(int part=0;part<2;++part){emit(part?"# Comparable-site matrix\n":"# Distance matrix\n");std::vector<std::string> header{"Sequence index"};for(std::size_t j=0;j<r.rows.size();++j)header.push_back(std::to_string(j+1));line(header);
            for(std::size_t i=0;i<r.rows.size();++i){std::vector<std::string> cells{std::to_string(i+1)};for(std::size_t j=0;j<r.rows.size();++j){if((j&1023)==0)check(stop);const auto& p=r.pair(i,j);cells.push_back(part?std::to_string(p.comparable):p.valid()?distance_number(p.value):"N/A");}line(cells);}}
        emit("# Unavailable estimates (including unavailable diagonal entries)\n");line({"Sequence 1 index","Sequence 2 index","Reason"});for(std::size_t i=0;i<r.rows.size();++i)for(std::size_t j=0;j<=i;++j){check(stop);const auto& p=r.pair(i,j);if(!p.valid())line({std::to_string(j+1),std::to_string(i+1),distance_failure_name(p.failure)});}
    }else{
        emit("# m = valid pair distances; exclusions do not enter the mean, SD or SE. Sample SD = sqrt(sum((d_i - mean)^2)/(m-1)); SE (naive) = SD/sqrt(m). SD and SE are N/A for m < 2.\n# SE (naive) assumes independent pairs. Shared sequences make pair distances dependent; this is not a bootstrap or dependence-adjusted uncertainty estimate. SD describes dispersion among pair distances.\n");
        line({"Group 1","Group 2","Mean distance","SD (pairs)","SE (naive)","Valid pairs","Excluded pairs","No overlap","Below minimum","Saturated","Mean comparable sites (valid pairs)","Min comparable","Max comparable"});
        for(const auto& m:r.means){const bool overall=r.report==DistanceReport::Overall;line({overall?"Overall":r.groups[m.first].name,overall?"Overall":r.groups[m.second].name,m.valid?distance_number(m.value()):"N/A",m.sample_sd()?distance_number(*m.sample_sd()):"N/A",m.naive_se()?distance_number(*m.naive_se()):"N/A",std::to_string(m.valid),std::to_string(m.excluded()),std::to_string(m.noOverlap),std::to_string(m.belowMinimum),std::to_string(m.saturated),m.valid?distance_number(static_cast<double>(m.comparableSum/static_cast<long double>(m.valid))):"N/A",m.valid?std::to_string(m.minimumComparable):"N/A",m.valid?std::to_string(m.maximumComparable):"N/A"});}
    }
}
std::string distance_tsv(const DistanceResult& result,std::size_t limit,const DistanceCancel& stop){std::string text;write_distance_tsv(result,[&](std::string_view chunk){if(chunk.size()>limit-text.size())throw std::length_error("Copy exceeds the 64 MiB clipboard limit. Export TSV to a file instead.");text.append(chunk);},stop);return text;}
}
