#include "core/alignment.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace kleiotide {
namespace {
unsigned char folded(char c){return static_cast<unsigned char>(c>='A'&&c<='Z'?c-'A'+'a':c);}
bool alphabetic(std::string_view a,std::string_view b){return std::lexicographical_compare(a.begin(),a.end(),b.begin(),b.end(),[](char x,char y){return folded(x)<folded(y);});}
struct Key {std::string_view text;RowKind kind;};
struct Hash {std::size_t operator()(Key k)const noexcept {std::size_t h=static_cast<std::size_t>(k.kind);for(char c:k.text)h=(h^folded(c))*16777619U;return h;}};
struct Equal {bool operator()(Key a,Key b)const noexcept {return a.kind==b.kind&&a.text.size()==b.text.size()&&std::equal(a.text.begin(),a.text.end(),b.text.begin(),[](char x,char y){return folded(x)==folded(y);});}};
}
std::vector<SequenceId> Document::nucleotide_ids()const {
    std::vector<SequenceId> ids;for(const auto& row:rows_)if(row.kind==RowKind::Nucleotide)ids.push_back(row.id);return ids;
}
void Document::reorder(const std::vector<SequenceId>& ids){
    if(ids.size()!=rows_.size())throw std::invalid_argument("Row order must contain every row exactly once.");
    std::unordered_map<SequenceId,std::size_t> positions;positions.reserve(rows_.size());
    for(std::size_t i=0;i<rows_.size();++i)positions.emplace(rows_[i].id,i);
    std::vector<std::size_t> desired;desired.reserve(ids.size());std::unordered_set<SequenceId> seen;
    for(auto id:ids){auto it=positions.find(id);if(it==positions.end()||!seen.insert(id).second)throw std::invalid_argument("Unknown or repeated row in ordering.");desired.push_back(it->second);}
    std::vector<std::size_t> current(rows_.size()),where(rows_.size());std::iota(current.begin(),current.end(),0);where=current;
    RowOrder order{rows_.size(),{}};order.swaps.reserve(rows_.size());
    for(std::size_t i=0;i<desired.size();++i){const auto j=where[desired[i]];if(i==j)continue;order.swaps.emplace_back(i,j);std::swap(current[i],current[j]);where[current[i]]=i;where[current[j]]=j;}
    if(order.swaps.empty())return;
    Command command;command.cost=sizeof(Command)+sizeof(RowOrder)+order.swaps.size()*sizeof(order.swaps.front());command.edits.emplace_back(std::move(order));execute(std::move(command));
}
void Document::move_rows(const std::vector<SequenceId>& ids,std::size_t insertionIndex){
    if(insertionIndex>rows_.size())throw std::out_of_range("Drop position outside row list.");
    std::unordered_set<SequenceId> selected(ids.begin(),ids.end());for(auto id:selected)(void)sequence(id);
    std::vector<SequenceId> moving,remaining;std::size_t removedBefore=0;
    for(std::size_t i=0;i<rows_.size();++i){const auto id=rows_[i].id;if(selected.contains(id)){moving.push_back(id);removedBefore+=i<insertionIndex;}else remaining.push_back(id);}
    remaining.insert(remaining.begin()+static_cast<std::ptrdiff_t>(insertionIndex-removedBefore),moving.begin(),moving.end());reorder(remaining);
}
std::vector<SequenceId> Document::sorted_order(Sort method,std::optional<ColumnRange> range)const {
    std::vector<std::size_t> order(rows_.size());std::iota(order.begin(),order.end(),0);
    std::vector<double> scores(rows_.size());std::vector<std::size_t> groups(rows_.size());
    if(method==Sort::ResidueFrequency){
        if(!range||!range->width||range->start>columns()||range->width>columns()-range->start)throw std::invalid_argument("Select a nonempty column range first.");
        std::vector<std::array<double,4>> frequencies(range->width);
        for(const auto& row:rows_)if(row.kind==RowKind::Nucleotide){const auto& s=row.symbols.text();const auto end=std::min(s.size(),range->start+range->width);
            for(auto c=range->start;c<end;++c){const auto v=nucleotide_votes(s[c]);for(std::size_t b=0;b<4;++b)frequencies[c-range->start][b]+=v[b];}}
        for(auto& f:frequencies){const auto total=std::accumulate(f.begin(),f.end(),0.0);if(total)for(auto& v:f)v/=total;}
        for(std::size_t i=0;i<rows_.size();++i)if(rows_[i].kind==RowKind::Nucleotide){const auto& s=rows_[i].symbols.text();const auto end=std::min(s.size(),range->start+range->width);
            for(auto c=range->start;c<end;++c){const auto v=nucleotide_votes(s[c]);for(std::size_t b=0;b<4;++b)scores[i]+=v[b]*frequencies[c-range->start][b]/6.0;}}
    }else if(method==Sort::Identical){
        std::unordered_map<Key,std::size_t,Hash,Equal> group;group.reserve(rows_.size());
        for(std::size_t i=0;i<rows_.size();++i)groups[i]=group.emplace(Key{rows_[i].symbols.text(),rows_[i].kind},i).first->second;
    }
    if(method==Sort::Group){std::unordered_map<GroupId,std::size_t> rank;for(std::size_t i=0;i<groups_.size();++i)rank[groups_[i].id]=i;
        std::stable_sort(order.begin(),order.end(),[&](auto a,auto b){const auto x=rows_[a].groupId,y=rows_[b].groupId;return (x?rank.at(x):groups_.size())<(y?rank.at(y):groups_.size());});
    }else if(method==Sort::Reverse)std::reverse(order.begin(),order.end());
    else std::stable_sort(order.begin(),order.end(),[&](auto a,auto b){const auto& x=rows_[a];const auto& y=rows_[b];switch(method){
        case Sort::Type:{auto rank=[](RowKind k){return k==RowKind::Mask?0:k==RowKind::Protein?1:2;};return rank(x.kind)<rank(y.kind);}
        case Sort::Title:return alphabetic(x.name,y.name);
        case Sort::LengthDescending:return x.symbols.size()>y.symbols.size();case Sort::LengthAscending:return x.symbols.size()<y.symbols.size();
        case Sort::TrueLengthDescending:return x.symbols.residue_count()>y.symbols.residue_count();case Sort::TrueLengthAscending:return x.symbols.residue_count()<y.symbols.residue_count();
        case Sort::ResidueFrequency:if(x.kind!=y.kind)return x.kind==RowKind::Nucleotide;return scores[a]>scores[b];
        case Sort::Identical:return groups[a]<groups[b];default:return false;
    }});
    std::vector<SequenceId> result;result.reserve(order.size());for(auto i:order)result.push_back(rows_[i].id);return result;
}
}
