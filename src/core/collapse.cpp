#include "core/collapse.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace kleiotide {
namespace {
void check(const CollapseCancel& cancel){if(cancel&&cancel())throw std::runtime_error("Duplicate preview cancelled.");}
char upper(char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;}
}
bool identical_ignoring_gaps(const Sequence& a,const Sequence& b,CollapseCancel cancel){
    check(cancel);
    if(a.kind!=b.kind||a.representation!=b.representation||a.symbols.residue_count()!=b.symbols.residue_count())return false;
    const auto& x=a.symbols.text();const auto& y=b.symbols.text();std::size_t i=0,j=0,steps=0;
    for(;;){while(i<x.size()&&is_gap(x[i])){++i;if((++steps&4095)==0)check(cancel);}while(j<y.size()&&is_gap(y[j])){++j;if((++steps&4095)==0)check(cancel);}
        if(i==x.size()||j==y.size())return i==x.size()&&j==y.size();
        if(upper(x[i++])!=upper(y[j++]))return false;
        if((++steps&4095)==0)check(cancel);
    }
}
CollapsePlan plan_identical_collapse(const Document& d,bool selectedOnly,const std::vector<SequenceId>& selected,bool includeMasks,CollapseCancel cancel,CollapseProgress progress){
    CollapsePlan result;result.revision=d.state_id();check(cancel);
    std::unordered_set<SequenceId> chosen;
    if(selectedOnly){for(auto id:selected)if(!chosen.insert(id).second)throw std::invalid_argument("Repeated selected sequence ID.");}
    struct Bucket {std::size_t first;DuplicateCluster cluster;};
    std::vector<Bucket> buckets;std::unordered_map<std::uint64_t,std::vector<std::size_t>> lookup;std::size_t visited=0;
    for(std::size_t rowIndex=0;rowIndex<d.rows().size();++rowIndex){check(cancel);const auto& row=d.rows()[rowIndex];if(progress)progress(rowIndex,d.rows().size());
        if(selectedOnly&&!chosen.contains(row.id))continue;
        ++visited;++result.scopedRows;
        if(row.kind==RowKind::Mask&&!includeMasks){++result.masksSkipped;continue;}++result.eligibleRows;
        std::uint64_t hash=14695981039346656037ULL;auto add=[&](unsigned char c){hash^=c;hash*=1099511628211ULL;};
        add(static_cast<unsigned char>(row.kind));add(static_cast<unsigned char>(row.representation));std::size_t steps=0;
        for(char c:row.symbols.text()){if((++steps&4095)==0)check(cancel);if(!is_gap(c))add(static_cast<unsigned char>(upper(c)));}
        auto& candidates=lookup[hash];std::optional<std::size_t> matching;
        for(auto at:candidates)if(identical_ignoring_gaps(d.rows()[buckets[at].first],row,cancel)){matching=at;break;}
        if(!matching){matching=buckets.size();candidates.push_back(*matching);buckets.push_back({rowIndex,{}});}
        buckets[*matching].cluster.members.push_back(row.id);
    }
    if(selectedOnly&&visited!=chosen.size())throw std::invalid_argument("A selected sequence is no longer present.");
    for(auto& bucket:buckets){check(cancel);if(bucket.cluster.members.size()>1){result.removedRows+=bucket.cluster.members.size()-1;result.clusters.push_back(std::move(bucket.cluster));}}
    if(progress)progress(d.rows().size(),d.rows().size());
    check(cancel);
    return result;
}
std::string collapsed_title(const Document& d,const DuplicateCluster& cluster,SequenceId representative,CollapseNaming naming,std::string_view separator){
    if(cluster.members.size()<2||std::find(cluster.members.begin(),cluster.members.end(),representative)==cluster.members.end())throw std::invalid_argument("Choose a representative from this duplicate set.");
    const auto& name=d.sequence(representative).name;
    if(naming==CollapseNaming::CountSuffix)return name+"_x"+std::to_string(cluster.members.size());
    if(naming!=CollapseNaming::Concatenate)throw std::invalid_argument("Unknown collapsed-title format.");
    if(separator.find_first_of("\r\n")!=std::string_view::npos||separator.find('\0')!=std::string_view::npos)throw std::invalid_argument("The title separator must be a single-line string.");
    std::string result=name;for(auto id:cluster.members)if(id!=representative){result+=separator;result+=d.sequence(id).name;}return result;
}
std::vector<Document::CollapseGroup> collapse_choices(const Document& d,const CollapsePlan& plan,const std::vector<SequenceId>& representatives,CollapseNaming naming,std::string_view separator,CollapseCancel cancel){
    if(d.state_id()!=plan.revision)throw std::invalid_argument("The alignment changed. Preview again before applying.");
    if(representatives.size()!=plan.clusters.size())throw std::invalid_argument("Choose one representative per duplicate set.");
    if(naming!=CollapseNaming::Concatenate&&naming!=CollapseNaming::CountSuffix)throw std::invalid_argument("Unknown collapsed-title format.");
    if(naming==CollapseNaming::Concatenate&&(separator.find_first_of("\r\n")!=std::string_view::npos||separator.find('\0')!=std::string_view::npos))throw std::invalid_argument("The title separator must be a single-line string.");
    std::unordered_map<SequenceId,const Sequence*> rows;for(const auto& row:d.rows())rows.emplace(row.id,&row);
    std::vector<Document::CollapseGroup> result;result.reserve(plan.clusters.size());
    for(std::size_t i=0;i<plan.clusters.size();++i){check(cancel);const auto& cluster=plan.clusters[i];const auto id=representatives[i];
        if(cluster.members.size()<2||std::find(cluster.members.begin(),cluster.members.end(),id)==cluster.members.end())throw std::invalid_argument("Choose a representative from this duplicate set.");
        auto title=rows.at(id)->name;
        if(naming==CollapseNaming::CountSuffix)title+="_x"+std::to_string(cluster.members.size());
        else for(auto member:cluster.members)if(member!=id){check(cancel);title+=separator;title+=rows.at(member)->name;}
        result.push_back({id,cluster.members,std::move(title)});
    }
    return result;
}
void apply_identical_collapse(Document& d,const CollapsePlan& plan,const std::vector<Document::CollapseGroup>& choices){
    if(d.state_id()!=plan.revision)throw std::invalid_argument("The alignment changed. Preview again before applying.");
    if(choices.size()!=plan.clusters.size())throw std::invalid_argument("Choose a representative for every duplicate set.");
    for(std::size_t i=0;i<choices.size();++i)if(choices[i].members!=plan.clusters[i].members)throw std::invalid_argument("Duplicate membership differs from the preview.");
    d.collapse_identical(choices);
}
}
