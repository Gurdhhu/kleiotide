#include "core/site_summary.hpp"
#include <stdexcept>
#include <unordered_set>
namespace kleiotide {
namespace {char upper(char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;}}
unsigned nucleotide_state(char raw)noexcept{
    switch(upper(raw)){
    case 'A':return 1;case 'C':return 2;case 'G':return 4;case 'T':case 'U':return 8;
    case 'R':return 5;case 'Y':return 10;case 'W':return 9;case 'S':return 6;
    case 'M':return 3;case 'K':return 12;case 'B':return 14;case 'H':return 11;
    case 'D':return 13;case 'V':return 7;default:return 15;
    }
}
unsigned protein_state(char raw)noexcept{
    constexpr std::string_view alphabet="ACDEFGHIKLMNPQRSTVWYUO*";
    const auto c=upper(raw);const auto index=alphabet.find(c);
    if(index!=std::string_view::npos)return 1U<<index;
    switch(c){
    case 'B':return (1U<<2)|(1U<<11); // D or N
    case 'Z':return (1U<<3)|(1U<<13); // E or Q
    case 'J':return (1U<<7)|(1U<<9);  // I or L
    default:return ProteinColumn::all;
    }
}
void NucleotideEvidence::add(char c)noexcept{
    if(is_gap(c))return;
    ++residues;const auto votes=nucleotide_votes(c);
    for(std::size_t i=0;i<4;++i)bases[i]+=votes[i];
}
SiteScan::SiteScan(const Document& d,AnalysisScope scope):source_(&d),revision_(d.content_id()),scope_(std::move(scope)),width_(d.columns()){
    if(scope_.molecule!=RowKind::Nucleotide&&scope_.molecule!=RowKind::Protein)throw std::invalid_argument("Choose nucleotide or protein rows.");
    const std::unordered_set<SequenceId> ids(scope_.ids.begin(),scope_.ids.end());
    for(std::size_t i=0;i<d.rows().size();++i)if(d.rows()[i].kind==scope_.molecule&&(!scope_.selectedRows||ids.contains(d.rows()[i].id)))rows_.push_back(i);
    if(rows_.size()>std::numeric_limits<std::uint32_t>::max()/6)throw std::length_error("Too many rows for column analysis.");
    scope_.begin=scope_.selectedColumns?std::min(scope_.begin,width_):0;
    scope_.end=scope_.selectedColumns?std::clamp(scope_.end,scope_.begin,width_):width_;
    const auto count=scope_.end-scope_.begin;
    if(scope_.molecule==RowKind::Protein)columns_=std::vector<ProteinColumn>(count);
    else columns_=std::vector<NucleotideColumn>(count);
    column_=scope_.begin;
}
bool SiteScan::step(const Document& d,std::size_t budget){
    if(source_!=&d||revision_!=d.content_id())throw std::logic_error("Column analysis input changed.");
    std::visit([&](auto& columns){while(row_<rows_.size()&&budget){
        const auto& text=d.rows()[rows_[row_]].symbols.text();
        const auto end=std::min(scope_.end,text.size());
        if(column_>=end){++row_;column_=scope_.begin;--budget;continue;}
        const auto finish=column_+std::min(end-column_,budget);
        while(column_<finish){columns[column_-scope_.begin].add(text[column_]);++column_;--budget;}
    }},columns_);
    return complete();
}
std::vector<unsigned char> SiteScan::highlights(const HighlightRules& rules)const{
    if(!complete())throw std::logic_error("Column analysis incomplete.");
    std::vector<unsigned char> result(width_);
    std::visit([&](const auto& columns){for(std::size_t c=0;c<columns.size();++c)result[c+scope_.begin]=highlighted(columns[c],rows_.size(),rules);},columns_);
    return result;
}
}
