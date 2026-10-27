#include "core/overview.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace kleiotide {
namespace {
std::size_t boundary(std::size_t length,std::size_t i,std::size_t count){
    return i*(length/count)+(i*(length%count))/count;
}
}
OverviewScan::OverviewScan(const Document& d,std::size_t workspace,std::size_t maxBins)
    :source_(&d),rows_(d.rows().size()),columns_(d.columns()),revision_(d.content_id()) {
    if(!maxBins||maxBins>65536)throw std::invalid_argument("Overview resolution must be 1–65536 bins.");
    std::size_t eligible=0;
    for(const auto& row:d.rows())if(row.kind==RowKind::Nucleotide){++eligible;total_+=row.symbols.size();}
    if(eligible>std::numeric_limits<std::uint32_t>::max()/6)throw std::length_error("Too many rows for overview.");
    const auto count=std::min(columns_,maxBins);
    if(count>workspace/sizeof(Summary))throw std::length_error("Overview workspace limit is too small for display bins.");
    auto remaining=workspace-count*sizeof(Summary);
    if(eligible>remaining/sizeof(std::size_t))throw std::length_error("Overview workspace limit is too small for the row index.");
    remaining-=eligible*sizeof(std::size_t);
    const auto tileCount=std::min({columns_,std::size_t(65536),remaining/sizeof(NucleotideEvidence)});
    if(columns_&&eligible&&!tileCount)throw std::length_error("Overview workspace limit is too small for one column.");
    summaries_.resize(count);
    for(std::size_t i=0;i<count;++i){summaries_[i].begin=boundary(columns_,i,count);summaries_[i].end=boundary(columns_,i+1,count);}
    rowsByLength_.reserve(eligible);
    for(std::size_t i=0;i<rows_;++i)if(d.rows()[i].kind==RowKind::Nucleotide)rowsByLength_.push_back(i);
    std::sort(rowsByLength_.begin(),rowsByLength_.end(),[&](auto a,auto b){return d.rows()[a].symbols.size()>d.rows()[b].symbols.size();});
    complete_=!columns_||!eligible||!total_;
    if(!complete_)tile_.resize(tileCount);
    tileEnd_=tileCount;
}
std::size_t OverviewScan::workspace_bytes()const noexcept {
    return tile_.capacity()*sizeof(NucleotideEvidence)+summaries_.capacity()*sizeof(Summary)+rowsByLength_.capacity()*sizeof(std::size_t);
}
bool OverviewScan::step(const Document& d,std::size_t budget){
    if(&d!=source_||d.content_id()!=revision_||d.rows().size()!=rows_)throw std::logic_error("Overview input changed.");
    while(!complete_&&budget){
        if(folding_){
            const auto columnIndex=tileBegin_+fold_;
            while(columnIndex>=summaries_[bin_].end)++bin_;
            auto& summary=summaries_[bin_];auto& column=tile_[fold_];
            summary.residues+=static_cast<double>(column.residues)/rowsByLength_.size();
            const auto votes=std::uint64_t(column.bases[0])+column.bases[1]+column.bases[2]+column.bases[3];
            if(votes){++summary.informative;summary.conservation+=static_cast<double>(*std::max_element(column.bases.begin(),column.bases.end()))/votes;}
            column={};++fold_;--budget;
            if(tileBegin_+fold_==tileEnd_){
                tileBegin_=tileEnd_;complete_=tileBegin_==columns_;
                tileEnd_=tileBegin_+std::min(tile_.size(),columns_-tileBegin_);
                row_=offset_=fold_=0;folding_=false;
            }
            continue;
        }
        if(!row_&&d.rows()[rowsByLength_.front()].symbols.size()<=tileBegin_){complete_=true;break;}
        if(row_==rowsByLength_.size()||d.rows()[rowsByLength_[row_]].symbols.size()<=tileBegin_){folding_=true;continue;}
        const auto& text=d.rows()[rowsByLength_[row_]].symbols.text();
        const auto limit=std::min(tileEnd_,text.size());
        if(!offset_)offset_=tileBegin_;
        const auto end=offset_+std::min(budget,limit-offset_);
        while(offset_<end){tile_[offset_-tileBegin_].add(text[offset_]);++offset_;++processed_;--budget;}
        if(offset_==limit){++row_;offset_=0;if(budget)--budget;}
    }
    return complete_;
}
double OverviewScan::progress()const noexcept{return complete_?1.0:total_?static_cast<double>(processed_/total_):0.0;}
std::vector<OverviewBin> OverviewScan::bins(std::size_t count)const{
    if(!complete_)throw std::logic_error("Overview is not complete.");
    count=std::min(count,summaries_.size());std::vector<OverviewBin> result;result.reserve(count);
    for(std::size_t i=0;i<count;++i){
        const auto first=boundary(summaries_.size(),i,count),last=boundary(summaries_.size(),i+1,count);
        OverviewBin bin;bin.begin=summaries_[first].begin;bin.end=summaries_[last-1].end;std::size_t informative=0;
        for(auto j=first;j<last;++j){bin.residues+=summaries_[j].residues;bin.conservation+=summaries_[j].conservation;informative+=summaries_[j].informative;}
        bin.residues/=bin.end-bin.begin;bin.hasInformation=informative!=0;
        if(informative)bin.conservation/=informative;
        result.push_back(bin);
    }
    return result;
}
}
