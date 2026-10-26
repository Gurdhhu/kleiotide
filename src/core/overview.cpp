#include "core/overview.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace kleiotide {
OverviewScan::OverviewScan(const Document& document):source_(&document),columns_(document.columns()),rows_(document.rows().size()),revision_(document.content_id()) {
    if(rows_>std::numeric_limits<std::uint32_t>::max()/6)throw std::length_error("Too many rows for overview.");
    for(const auto& row:document.rows())if(row.kind==RowKind::Nucleotide){++nucleotideRows_;total_+=row.symbols.size();}
}
bool OverviewScan::step(const Document& document,std::size_t budget) {
    if(&document!=source_ || document.content_id()!=revision_ || document.rows().size()!=rows_)throw std::logic_error("Overview input changed.");
    while(row_<rows_ && budget) {
        if(document.rows()[row_].kind!=RowKind::Nucleotide){++row_;offset_=0;--budget;continue;}
        const auto& text=document.rows()[row_].symbols.text();
        if(offset_==text.size()){++row_;offset_=0;--budget;continue;}
        const auto end=offset_+std::min(budget,text.size()-offset_);
        while(offset_<end) {
            auto c=text[offset_];auto& column=columns_[offset_];++offset_;--budget;++processed_;
            column.add(c);
        }
    }
    return complete();
}
double OverviewScan::progress() const noexcept { return complete()?1.0:total_?static_cast<double>(processed_)/total_:0.0; }
std::vector<OverviewBin> OverviewScan::bins(std::size_t count) const {
    if(!complete())throw std::logic_error("Overview is not complete.");
    count=std::min(count,columns_.size());std::vector<OverviewBin> bins;bins.reserve(count);
    for(std::size_t i=0;i<count;++i){
        const auto begin=i*columns_.size()/count,end=(i+1)*columns_.size()/count;
        OverviewBin bin;bin.begin=begin;bin.end=end;std::size_t informative=0;
        for(auto c=begin;c<end;++c){
            const auto& column=columns_[c];
            bin.residues+=nucleotideRows_?static_cast<double>(column.residues)/nucleotideRows_:0;
            const auto calls=column.bases[0]+column.bases[1]+column.bases[2]+column.bases[3];
            if(calls){bin.hasInformation=true;++informative;bin.conservation+=static_cast<double>(*std::max_element(column.bases.begin(),column.bases.end()))/calls;}
        }
        bin.residues/=end-begin;if(informative)bin.conservation/=informative;bins.push_back(bin);
    }
    return bins;
}
}
