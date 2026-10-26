#include "core/chromatogram.hpp"
#include "core/alignment.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>

namespace kleiotide {
char trace_complement(char c) noexcept {
    const bool lower=c>='a'&&c<='z';
    const auto upper=lower?static_cast<char>(c-'a'+'A'):c;
    constexpr std::string_view bases="ACGTURYSWKMBDHVN", complements="TGCAAYRSWMKVHDBN";
    const auto i=bases.find(upper);
    if(i==std::string_view::npos)return c;
    const char result=complements[i];return lower?static_cast<char>(result-'A'+'a'):result;
}
std::shared_ptr<const TraceLink> reversed_trace(const std::shared_ptr<const TraceLink>& link) {
    if(!link)return {};
    auto result=std::make_shared<TraceLink>(*link);
    std::reverse(result->originalCalls.begin(),result->originalCalls.end());
    result->reversed=!result->reversed;return result;
}
std::size_t trace_storage_bytes(const std::shared_ptr<const TraceLink>& link) noexcept {
    if(!link)return 0;
    const auto& data=*link->original;
    auto size=sizeof(TraceLink)+link->originalCalls.size()*sizeof(std::int32_t)+sizeof(Chromatogram)+data.sourcePath.size()+data.sampleName.size()+data.calls.size()+data.peaks.size()*sizeof(std::uint32_t)+data.quality.size();
    for(const auto& channel:data.channels)size+=channel.size()*sizeof(std::int32_t);
    return size;
}
void validate_trace(const Sequence& row) {
    if(!row.trace)return;
    const auto& link=*row.trace;
    if(row.kind!=RowKind::Nucleotide||!link.original||link.originalCalls.size()!=row.symbols.residue_count())
        throw std::invalid_argument("Invalid trace-to-sequence mapping.");
    const auto& data=*link.original;
    if(data.calls.empty()||data.peaks.size()!=data.calls.size()||(!data.quality.empty()&&data.quality.size()!=data.calls.size()))
        throw std::invalid_argument("Inconsistent original trace calls.");
    const auto size=data.channels[0].size();
    if(!size)throw std::invalid_argument("Empty chromatogram signal.");
    for(const auto& channel:data.channels)if(channel.size()!=size)throw std::invalid_argument("Unequal trace channels.");
    for(std::size_t i=0;i<data.peaks.size();++i)if(data.peaks[i]>=size||(i&&data.peaks[i]<data.peaks[i-1]))
        throw std::invalid_argument("Invalid original trace peak positions.");
    std::int32_t previous=-1;
    for(auto call:link.originalCalls){
        if(call < -1||call>=static_cast<std::int64_t>(data.calls.size()))throw std::invalid_argument("Invalid original call index.");
        if(call<0)continue;
        if(previous>=0&&(!link.reversed?call<=previous:call>=previous))throw std::invalid_argument("Trace anchors must retain strand order.");
        previous=call;
    }
}
std::shared_ptr<const TraceLink> edited_trace(const Sequence& row,std::size_t start,std::size_t count,const std::string& after) {
    if(!row.trace)return {};
    const auto before=row.symbols.text().substr(start,count);
    auto residues=[](std::string_view text){std::string out;for(char c:text)if(!is_gap(c))out+=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));return out;};
    if(residues(before)==residues(after))return row.trace;
    const auto rank=row.symbols.residue_rank(start),end=row.symbols.residue_rank(start+count);
    auto result=std::make_shared<TraceLink>();result->original=row.trace->original;result->reversed=row.trace->reversed;
    const auto& old=row.trace->originalCalls;
    result->originalCalls.insert(result->originalCalls.end(),old.begin(),old.begin()+static_cast<std::ptrdiff_t>(rank));
    auto ordinal=rank;
    for(std::size_t c=0;c<std::max(before.size(),after.size());++c){
        const bool was=c<before.size()&&!is_gap(before[c]);
        if(c<after.size()&&!is_gap(after[c]))result->originalCalls.push_back(was?old[ordinal]:-1);
        if(was)++ordinal;
    }
    result->originalCalls.insert(result->originalCalls.end(),old.begin()+static_cast<std::ptrdiff_t>(end),old.end());return result;
}
TracePosition trace_position(const Sequence& row,std::size_t column) {
    TracePosition result;if(!row.trace)return result;
    const auto& link=*row.trace;const auto& data=*link.original;
    column=std::min(column,row.symbols.size());
    const auto ordinal=row.symbols.residue_rank(column);
    result.current=column<row.symbols.size()?row.symbols.text()[column]:'~';
    if(!is_gap(result.current)&&ordinal<link.originalCalls.size()&&link.originalCalls[ordinal]>=0){
        const auto call=static_cast<std::size_t>(link.originalCalls[ordinal]);result.originalCall=call;result.sample=data.peaks[call];
        result.imported=link.reversed?trace_complement(data.calls[call]):data.calls[call];
        result.edited=std::toupper(static_cast<unsigned char>(result.current))!=std::toupper(static_cast<unsigned char>(result.imported));
        if(!result.edited&&!data.quality.empty())result.quality=data.quality[call];
        return result;
    }
    // Interpolate between surviving original calls, including runs of inserted bases.
    const auto& anchors=link.originalCalls;std::size_t left=ordinal,right=ordinal;
    while(left&&anchors[left-1]<0)--left;
    while(right<anchors.size()&&anchors[right]<0)++right;
    const double direction=link.reversed?-1.:1.;
    const double lo=left?data.peaks[static_cast<std::size_t>(anchors[left-1])]:
        (right<anchors.size()?data.peaks[static_cast<std::size_t>(anchors[right])]-direction*12.*static_cast<double>(right-left+1):0.);
    const double hi=right<anchors.size()?data.peaks[static_cast<std::size_t>(anchors[right])]:lo+direction*12.*static_cast<double>(right-left+2);
    const double fraction=is_gap(result.current)?0.5:static_cast<double>(ordinal-left+1)/static_cast<double>(right-left+1);
    result.sample=std::clamp(lo+(hi-lo)*fraction,0.,static_cast<double>(data.channels[0].size()-1));
    return result;
}
std::optional<std::size_t> trace_column(const Sequence& row,std::size_t originalCall) {
    if(!row.trace)return {};
    std::size_t ordinal=0;
    for(std::size_t c=0;c<row.symbols.size();++c)if(!is_gap(row.symbols.text()[c])){
        if(row.trace->originalCalls[ordinal++]==static_cast<std::int64_t>(originalCall))return c;
    }
    return {};
}
}
