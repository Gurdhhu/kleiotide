#include "core/identity_groups.hpp"
#include "core/group_import.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <iomanip>
#include <locale>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace kleiotide {
namespace {
void check(const IdentityCancel& stop){if(stop&&stop())throw std::runtime_error("Grouping cancelled.");}
char canonical(char c,RowKind kind){if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');if(kind==RowKind::Nucleotide&&c=='U')c='T';return c;}
bool missing(char c,RowKind kind){c=canonical(c,kind);return c=='?'||c==(kind==RowKind::Protein?'X':'N');}
bool terminal(char c,RowKind kind){return is_gap(c)||missing(c,kind);}
struct Input {
    const Sequence* row;
    std::size_t begin=0,end=0,length=0;
    std::string ungapped;
    Input(const Sequence& r,IdentityAlignment mode,const IdentityCancel& stop):row(&r),end(r.symbols.size()){
        const auto& s=r.symbols.text();
        while(begin<end&&terminal(s[begin],r.kind)){if((begin&4095)==0)check(stop);++begin;}
        while(end>begin&&terminal(s[end-1],r.kind)){if((end&4095)==0)check(stop);--end;}
        for(auto c=begin;c<end;++c){if((c&4095)==0)check(stop);length+=!is_gap(s[c]);}
        if(mode==IdentityAlignment::Pairwise){ungapped.reserve(length);for(auto c=begin;c<end;++c){if((c&4095)==0)check(stop);if(!is_gap(s[c]))ungapped+=canonical(s[c],r.kind);}}
    }
};
void column(IdentityPair& p,char a,char b,RowKind kind){
    p.coveredFirst+=!is_gap(a);p.coveredSecond+=!is_gap(b);
    if(missing(a,kind)||missing(b,kind)||(is_gap(a)&&is_gap(b)))return;
    ++p.columns;
    if(!is_gap(a)&&!is_gap(b)){++p.paired;p.matches+=canonical(a,kind)==canonical(b,kind);}
}
IdentityPair aligned(const Input& a,const Input& b,const IdentityCancel& stop){
    IdentityPair p;const auto end=std::min(a.end,b.end);const auto& x=a.row->symbols.text();const auto& y=b.row->symbols.text();
    for(auto c=std::max(a.begin,b.begin);c<end;++c){if((c&4095)==0)check(stop);column(p,x[c],y[c],a.row->kind);}return p;
}
// A linear-time contained-string fast path avoids DP for identical sequences
// and exact fragments. All other pairs use the full affine semiglobal DP.
std::optional<std::size_t> contained(std::string_view hay,std::string_view needle,const IdentityCancel& stop){
    std::vector<std::size_t> prefix(needle.size());
    for(std::size_t i=1,j=0;i<needle.size();++i){if((i&4095)==0)check(stop);while(j&&needle[i]!=needle[j])j=prefix[j-1];if(needle[i]==needle[j])++j;prefix[i]=j;}
    for(std::size_t i=0,j=0;i<hay.size();++i){if((i&4095)==0)check(stop);while(j&&hay[i]!=needle[j])j=prefix[j-1];if(hay[i]==needle[j])++j;if(j==needle.size())return i+1-j;}return {};
}
constexpr auto invalidScore=std::numeric_limits<std::int64_t>::min()/4;
struct Cell {std::int64_t score=invalidScore;IdentityPair pair;};
bool better(const Cell& a,const Cell& b){
    if(a.score!=b.score)return a.score>b.score;
    // Fixed deterministic tie-breaking: more matches, then fewer differences,
    // then more paired residues. No traceback or alignment matrix is retained.
    if(a.pair.matches!=b.pair.matches)return a.pair.matches>b.pair.matches;
    if(a.pair.columns!=b.pair.columns)return a.pair.columns<b.pair.columns;
    return a.pair.paired>b.pair.paired;
}
Cell choose(Cell a,const Cell& b){return better(b,a)?b:a;}
Cell extend(Cell a,char x,char y,RowKind kind,std::int64_t score){if(a.score==invalidScore)return a;a.score+=score;column(a.pair,x,y,kind);return a;}
// For ordinary read lengths, encode the existing score and deterministic
// tie-breaks into one ordered integer. Coverage travels with the winning path.
// This is the same full affine recurrence, not a band or an edit approximation.
IdentityPair packed_pairwise(std::string_view x,std::string_view y,RowKind kind,const IdentityCancel& stop){
    const auto maxColumns=x.size()+y.size();unsigned bits=1;while((std::size_t(1)<<bits)<=maxColumns)++bits;
    const std::int64_t pairedUnit=1,columnUnit=std::int64_t(1)<<bits,matchUnit=std::int64_t(1)<<(2*bits),scoreUnit=std::int64_t(1)<<(3*bits);
    const auto initial=static_cast<std::int64_t>(maxColumns)*columnUnit;
    const auto mask=(std::uint64_t(1)<<bits)-1;
    struct Packed {std::int64_t rank=invalidScore;std::uint32_t coverage=0;};
    auto choosePacked=[](Packed a,Packed b){return b.rank>a.rank?b:a;};
    auto add=[](Packed a,std::int64_t delta,std::uint32_t coverage){if(a.rank!=invalidScore){a.rank+=delta;a.coverage+=coverage;}return a;};
    std::vector<Packed> previous(y.size()+1),current(y.size()+1),vertical(y.size()+1);for(auto& c:previous)c.rank=initial;
    std::vector<bool> absent(y.size());for(std::size_t j=0;j<y.size();++j)absent[j]=missing(y[j],kind);
    Packed best;
    for(std::size_t i=1;i<=x.size();++i){check(stop);current[0]=Packed{initial,0};Packed horizontal;const auto u=x[i-1];const bool absentU=missing(u,kind);
        for(std::size_t j=1;j<=y.size();++j){
            if((j&4095)==0)check(stop);
            const bool absentV=absent[j-1];
            const auto columnV=absentV?0:columnUnit,columnU=absentU?0:columnUnit;
            horizontal=choosePacked(add(current[j-1],-20*scoreUnit-columnV,1u<<16),add(horizontal,-2*scoreUnit-columnV,1u<<16));
            vertical[j]=choosePacked(add(previous[j],-20*scoreUnit-columnU,1),add(vertical[j],-2*scoreUnit-columnU,1));
            const bool omitted=absentU||absentV,equal=u==y[j-1];
            const auto delta=omitted?0:(equal?2:-4)*scoreUnit+(equal?matchUnit:0)-columnUnit+pairedUnit;
            current[j]=choosePacked(choosePacked(add(previous[j-1],delta,(1u<<16)+1),horizontal),vertical[j]);
            if((i==x.size()||j==y.size())&&(static_cast<std::uint64_t>(current[j].rank)&mask)&&current[j].rank>best.rank)best=current[j];
        }previous.swap(current);
    }
    if(best.rank==invalidScore)return {};
    const auto value=static_cast<std::uint64_t>(best.rank);IdentityPair result;
    result.paired=value&mask;result.columns=maxColumns-((value>>bits)&mask);result.matches=(value>>(2*bits))&mask;
    result.coveredFirst=best.coverage&0xffff;result.coveredSecond=best.coverage>>16;return result;
}
IdentityPair pairwise(const Input& first,const Input& second,const IdentityCancel& stop){
    if(!first.length||!second.length)return {};
    // Put the shorter sequence in the rolling rows: O(min(n,m)) memory.
    const bool swap=first.length<second.length;const auto& a=swap?second:first;const auto& b=swap?first:second;
    const auto& x=a.ungapped;const auto& y=b.ungapped;const auto kind=a.row->kind;
    constexpr std::size_t memoryLimit=128*1024*1024;
    if(y.size()>memoryLimit/sizeof(std::size_t))throw std::length_error("Exact-fragment workspace exceeds 128 MiB. Use the existing alignment or shorter sequences.");
    if(const auto at=contained(x,y,stop)){IdentityPair p;for(std::size_t j=0;j<y.size();++j){if((j&4095)==0)check(stop);column(p,x[*at+j],y[j],kind);}if(swap)std::swap(p.coveredFirst,p.coveredSecond);return p;}
    if(x.size()+y.size()<=8190){auto p=packed_pairwise(x,y,kind,stop);if(swap)std::swap(p.coveredFirst,p.coveredSecond);return p;}
    if(y.size()>=memoryLimit/(3*sizeof(Cell)))throw std::length_error("Pairwise alignment workspace exceeds 128 MiB. Use the existing alignment or shorter sequences.");
    std::vector<Cell> previous(y.size()+1),current(y.size()+1),vertical(y.size()+1);
    for(auto& c:previous)c.score=0; // Free leading overhangs in either sequence.
    Cell best;
    for(std::size_t i=1;i<=x.size();++i){
        check(stop);current[0]=Cell{0,{}};Cell horizontal;
        for(std::size_t j=1;j<=y.size();++j){
            if((j&4095)==0)check(stop);
            const char u=x[i-1],v=y[j-1];
            horizontal=choose(extend(current[j-1],'~',v,kind,-20),extend(horizontal,'~',v,kind,-2));
            vertical[j]=choose(extend(previous[j],u,'~',kind,-20),extend(vertical[j],u,'~',kind,-2));
            const auto score=missing(u,kind)||missing(v,kind)?0:u==v?2:-4;
            current[j]=choose(choose(extend(previous[j-1],u,v,kind,score),horizontal),vertical[j]);
            // End only on the last row/column. Unlike local alignment, there
            // is no score reset inside the matrix; only terminal gaps are free.
            if((i==x.size()||j==y.size())&&current[j].pair.valid()&&better(current[j],best))best=current[j];
        }
        previous.swap(current);
    }
    auto p=best.pair;if(swap)std::swap(p.coveredFirst,p.coveredSecond);return p;
}
IdentityPair compare(const Input& a,const Input& b,IdentityAlignment mode,const IdentityCancel& stop){
    check(stop);if(a.row->kind!=b.row->kind)return {};
    if(mode==IdentityAlignment::Existing&&a.row->representation!=b.row->representation)return {};
    return mode==IdentityAlignment::Existing?aligned(a,b,stop):pairwise(a,b,stop);
}
double overlap(const Input& a,const Input& b,const IdentityPair& p){
    if(!p.valid())return 0;
    if(a.length<b.length)return 100.0*p.coveredFirst/a.length;
    if(b.length<a.length)return 100.0*p.coveredSecond/b.length;
    return 100.0*std::min(p.coveredFirst,p.coveredSecond)/a.length;
}
// An inverted index stores unique k-words for centroids only. Hash collisions
// can affect ranking, never identity acceptance: shortlisted pairs still use the
// full unbanded affine DP above. No external dependency or n-by-n matrix.
std::vector<std::uint64_t> words(const Input& in,const IdentityCancel& stop){
    const std::size_t k=in.row->kind==RowKind::Protein?3:8;
    const auto& s=in.ungapped;std::vector<std::uint64_t> result;
    if(s.size()<k)return result;
    result.reserve(s.size()-k+1);
    for(std::size_t i=0;i+k<=s.size();++i){
        if((i&4095)==0)check(stop);
        std::uint64_t h=14695981039346656037ULL;bool valid=true;
        for(std::size_t j=0;j<k;++j){const auto c=s[i+j];
            if(missing(c,in.row->kind)||(in.row->kind==RowKind::Nucleotide&&c!='A'&&c!='C'&&c!='G'&&c!='T')){valid=false;break;}
            h=(h^static_cast<unsigned char>(c))*1099511628211ULL;
        }
        if(valid)result.push_back(h);
    }
    std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
}
class CandidateIndex {
    std::unordered_map<std::uint64_t,std::vector<std::size_t>> nucleotide_,protein_;
    std::unordered_map<std::string,std::size_t> exact_;
    static std::string key(const Input& in){return std::string(1,static_cast<char>(in.row->kind))+in.ungapped;}
public:
    void add(const Input& in,std::size_t c,const IdentityCancel& stop){exact_.emplace(key(in),c);auto& index=in.row->kind==RowKind::Protein?protein_:nucleotide_;for(auto word:words(in,stop))index[word].push_back(c);}
    std::vector<std::size_t> find(const Input& in,const std::vector<std::size_t>& centroids,const std::vector<Input>& input,std::size_t limit,const IdentityCancel& stop)const{
        std::vector<std::size_t> result;result.reserve(centroids.size());
        for(std::size_t c=0;c<centroids.size();++c){if((c&4095)==0)check(stop);if(in.row->kind==input[centroids[c]].row->kind)result.push_back(c);}
        if(!limit||result.empty())return result;
        limit=std::min(limit,result.size());
        std::vector<std::size_t> counts(centroids.size());const auto& index=in.row->kind==RowKind::Protein?protein_:nucleotide_;
        for(auto word:words(in,stop)){check(stop);if(auto it=index.find(word);it!=index.end())for(auto c:it->second){if((c&4095)==0)check(stop);++counts[c];}}
        std::partial_sort(result.begin(),result.begin()+static_cast<std::ptrdiff_t>(limit),result.end(),[&](auto a,auto b){return counts[a]!=counts[b]?counts[a]>counts[b]:a<b;});result.resize(limit);
        // Exact centroid strings always lead the shortlist, including seedless
        // ambiguity-rich or shorter-than-k queries.
        if(auto it=exact_.find(key(in));it!=exact_.end()){
            std::erase(result,it->second);result.insert(result.begin(),it->second);if(result.size()>limit)result.resize(limit);
        }
        // Short/seedless nonexact rows fall back to the first N compatible centroids.
        return result;
    }
};
void validate(const IdentityGroupOptions& o){
    if(o.maximumCandidates>4096)throw std::invalid_argument("Maximum candidates must be 0 (exhaustive) or between 1 and 4096.");
    if(o.alignment!=IdentityAlignment::Existing&&o.alignment!=IdentityAlignment::Pairwise)throw std::invalid_argument("Unknown identity alignment method.");
    if(!std::isfinite(o.minimumIdentity)||o.minimumIdentity<0||o.minimumIdentity>100||!std::isfinite(o.minimumOverlap)||o.minimumOverlap<0||o.minimumOverlap>100)throw std::invalid_argument("Identity and minimum overlap must be between 0 and 100%.");
    if(o.prefix.empty()||o.prefix.size()>256||o.prefix.find_first_not_of(' ')==std::string::npos||o.prefix.find_first_of("\t\r\n")!=std::string::npos||o.prefix.find('\0')!=std::string::npos)throw std::invalid_argument("Enter a nonblank, single-line group prefix (up to 256 UTF-8 bytes).");
    if(o.selectedOnly&&o.selected.empty())throw std::invalid_argument("Select sequence names first, or choose All sequences.");
}
}
IdentityPair sequence_identity(const Sequence& a,const Sequence& b,IdentityAlignment mode,IdentityCancel stop){
    if(mode!=IdentityAlignment::Existing&&mode!=IdentityAlignment::Pairwise)throw std::invalid_argument("Unknown identity alignment method.");
    if(a.kind==RowKind::Mask||b.kind==RowKind::Mask)return {};
    return compare(Input(a,mode,stop),Input(b,mode,stop),mode,stop);
}
IdentityGroupPlan plan_identity_groups(const Document& d,const IdentityGroupOptions& o,IdentityCancel stop,IdentityProgress progress){
    validate(o);check(stop);IdentityGroupPlan plan;plan.source=&d;plan.revision=d.state_id();
    std::unordered_set<SequenceId> selected(o.selected.begin(),o.selected.end());
    std::vector<Input> input;std::vector<std::size_t> memberIndex;
    for(const auto& row:d.rows()){
        check(stop);if(o.selectedOnly&&!selected.contains(row.id))continue;
        IdentityMember member;member.id=row.id;
        if(row.kind==RowKind::Mask)member.skipped="Mask row; unchanged";
        else {Input in(row,o.alignment,stop);if(!in.length)member.skipped="No informative residues; unchanged";else{memberIndex.push_back(plan.members.size());input.push_back(std::move(in));}}
        plan.members.push_back(std::move(member));
    }
    if(o.selectedOnly&&plan.members.size()!=selected.size())throw std::invalid_argument("Unknown sequence ID in grouping scope.");
    std::vector<std::size_t> order(input.size());std::iota(order.begin(),order.end(),0);
    std::stable_sort(order.begin(),order.end(),[&](auto a,auto b){return input[a].length>input[b].length;});
    // Retain old groups only if an untouched/skipped row still needs them.
    std::unordered_set<SequenceId> reassigned;for(const auto& in:input)reassigned.insert(in.row->id);
    std::unordered_set<GroupId> retained;for(const auto& row:d.rows())if(!reassigned.contains(row.id))retained.insert(row.groupId);
    for(const auto& g:d.groups())if(retained.contains(g.id))plan.groups.push_back(g);
    std::vector<std::size_t> centroids;std::vector<GroupId> clusterIds;CandidateIndex index;
    std::unordered_set<std::string> names;for(const auto& g:plan.groups)names.insert(g.name);
    auto nextId=d.next_group_id();std::size_t nextName=1,done=0;if(progress)progress(0,input.size(),0);
    for(auto i:order){
        check(stop);auto& member=plan.members[memberIndex[i]];const auto& query=input[i];
        std::vector<std::size_t> candidates;
        if(o.alignment==IdentityAlignment::Pairwise){
            candidates=index.find(query,centroids,input,o.maximumCandidates,stop);
            if(o.maximumCandidates&&static_cast<std::size_t>(std::count_if(centroids.begin(),centroids.end(),[&](auto c){return input[c].row->kind==query.row->kind;}))>o.maximumCandidates)++plan.shortlistedQueries;
        }else{candidates.resize(centroids.size());std::iota(candidates.begin(),candidates.end(),0);}
        for(auto c:candidates){
            const auto& target=input[centroids[c]];
            if(query.row->kind!=target.row->kind||(o.alignment==IdentityAlignment::Existing&&query.row->representation!=target.row->representation))continue;
            auto p=compare(query,target,o.alignment,stop);++plan.comparisons;if(progress)progress(done,input.size(),plan.comparisons);const auto cover=overlap(query,target,p);
            if(p.valid()&&p.identity()+1e-9>=o.minimumIdentity&&cover+1e-9>=o.minimumOverlap){member.group=clusterIds[c];member.centroid=target.row->id;member.pair=p;member.overlap=cover;break;}
        }
        if(!member.group){
            if(plan.groups.size()>=100000)throw std::length_error("The resulting catalog would exceed 100,000 groups. Remove unused groups or choose a smaller scope.");
            if(nextId==std::numeric_limits<GroupId>::max())throw std::overflow_error("Group ID space exhausted.");
            std::string name;do{name=o.prefix+std::to_string(nextName++);}while(names.contains(name));names.insert(name);
            member.group=nextId++;member.centroid=member.id;member.pair=compare(query,query,o.alignment,stop);member.overlap=overlap(query,query,member.pair);
            plan.groups.push_back({member.group,std::move(name),default_group_color(plan.groups.size())});centroids.push_back(i);clusterIds.push_back(member.group);if(o.alignment==IdentityAlignment::Pairwise&&o.maximumCandidates)index.add(query,centroids.size()-1,stop);++plan.clusters;
        }
        ++plan.assigned;++done;if(progress)progress(done,input.size(),plan.comparisons);
    }
    check(stop);return plan;
}
std::string identity_groups_tsv(const Document& d,const IdentityGroupPlan& p){
    if(p.source!=&d||p.revision!=d.state_id())throw std::invalid_argument("The alignment changed; refresh the grouping preview.");
    std::unordered_map<SequenceId,const Sequence*> rows;for(const auto& r:d.rows())rows[r.id]=&r;
    std::unordered_map<GroupId,std::string> names;for(const auto& g:p.groups)names[g.id]=g.name;
    auto quote=[](std::string_view value){std::string out="\"";for(char c:value){if(c=='\"')out+='\"';out+=c;}return out+'\"';};
    std::ostringstream out;out.imbue(std::locale::classic());out<<"Sequence\tProposed group\tGroup centroid\tIdentity (%)\tOverlap (%)\tResult\tSequence ID\tCentroid ID\n";
    for(const auto& m:p.members){out<<quote(rows.at(m.id)->name)<<'\t'<<quote(m.group?names.at(m.group):"Unchanged")<<'\t'<<quote(m.centroid?rows.at(m.centroid)->name:"")<<'\t';
        if(m.group){out<<std::fixed<<std::setprecision(2)<<m.pair.identity();}
        out<<'\t';if(m.group){out<<m.overlap;}
        out<<'\t'<<quote(!m.skipped.empty()?m.skipped:m.id==m.centroid?"New group centroid":"Group member")<<'\t'<<m.id<<'\t';if(m.centroid)out<<m.centroid;out<<'\n';
    }return out.str();
}
void apply_identity_groups(Document& d,const IdentityGroupPlan& p){
    if(p.source!=&d||p.revision!=d.state_id())throw std::invalid_argument("The alignment changed; refresh the grouping preview.");
    if(!p.assigned)throw std::invalid_argument("No eligible sequences to group.");
    std::vector<std::pair<SequenceId,GroupId>> assignments;assignments.reserve(p.assigned);for(const auto& member:p.members)if(member.group)assignments.emplace_back(member.id,member.group);d.apply_groups(p.groups,assignments);
}
std::vector<SequenceId> select_group_representatives(const Document& d,GroupRepresentative method,UngroupedSelection ungrouped,std::uint64_t seed,IdentityCancel stop){
    if(method!=GroupRepresentative::First&&method!=GroupRepresentative::Longest&&method!=GroupRepresentative::Random)throw std::invalid_argument("Unknown representative selection method.");
    if(ungrouped!=UngroupedSelection::None&&ungrouped!=UngroupedSelection::One&&ungrouped!=UngroupedSelection::All)throw std::invalid_argument("Unknown Ungrouped selection mode.");
    struct Choice {const Sequence* row=nullptr;std::uint64_t count=0;};std::unordered_map<GroupId,Choice> choices;std::mt19937_64 rng(seed);
    for(const auto& row:d.rows()){
        check(stop);if(!row.groupId&&ungrouped!=UngroupedSelection::One)continue;auto& choice=choices[row.groupId];++choice.count;
        const bool pick=!choice.row||(method==GroupRepresentative::Longest&&row.symbols.residue_count()>choice.row->symbols.residue_count())||(method==GroupRepresentative::Random&&std::uniform_int_distribution<std::uint64_t>(1,choice.count)(rng)==1);
        if(pick)choice.row=&row;
    }
    std::unordered_set<SequenceId> chosen;for(const auto& [id,choice]:choices){(void)id;chosen.insert(choice.row->id);}
    std::vector<SequenceId> result;for(const auto& row:d.rows()){check(stop);if(chosen.contains(row.id)||(!row.groupId&&ungrouped==UngroupedSelection::All))result.push_back(row.id);}return result;
}
std::vector<SequenceId> select_group_representatives(const Document& d,GroupRepresentative method,bool includeUngrouped,std::uint64_t seed,IdentityCancel stop){return select_group_representatives(d,method,includeUngrouped?UngroupedSelection::One:UngroupedSelection::None,seed,std::move(stop));}

}
