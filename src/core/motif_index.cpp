#include "core/motif_index.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace kleiotide {
namespace {
using Bounds=std::pair<std::uint64_t,std::uint64_t>; // half-open
Bounds bounds(std::int64_t first,std::int64_t step,std::uint64_t count,std::int64_t low,std::int64_t high){
    if(!count||low>=high)return {};
    if(!step)return first>=low&&first<high?Bounds{0,count}:Bounds{};
    if(step<0){auto result=bounds(first+static_cast<std::int64_t>(count-1)*step,-step,count,low,high);return {count-result.second,count-result.first};}
    auto lower=[&](std::int64_t value){std::uint64_t a=0,b=count;while(a<b){const auto m=a+(b-a)/2;if(first+static_cast<std::int64_t>(m)*step<value)a=m+1;else b=m;}return a;};
    return {lower(low),lower(high)};
}
}
std::uint64_t MotifIndex::end_at(const Run& run,std::uint64_t i){return static_cast<std::uint64_t>(static_cast<std::int64_t>(run.end)+static_cast<std::int64_t>(i)*run.endStep);}
void MotifIndex::encode(Row& row,std::uint64_t n){do{if(++bytes_>limit_)throw std::runtime_error("Search index exceeds its 512 MiB budget. Refine the motif or selected scope; no partial results were applied.");row.data.push_back(static_cast<unsigned char>((n&127)|(n>127?128:0)));n>>=7;}while(n);}
void MotifIndex::flush(Row& row){
    auto& run=row.pending;if(!run.count)return;
    if(row.runs%128==0){bytes_+=sizeof(Block);if(bytes_>limit_)throw std::runtime_error("Search index budget exceeded. Refine the search scope.");row.blocks.push_back({row.data.size(),row.count-run.count,run.start,row.maxEnd});}
    encode(row,run.start);encode(row,run.end-run.start);encode(row,run.step);
    encode(row,run.endStep>=0?static_cast<std::uint64_t>(run.endStep)*2:static_cast<std::uint64_t>(-(run.endStep+1))*2+1);
    encode(row,run.count);encode(row,run.flags);
    row.maxEnd=std::max({row.maxEnd,run.end,end_at(run,run.count-1)});row.blocks.back().maxEnd=row.maxEnd;++row.runs;run={};
}
void MotifIndex::add(MotifHit hit){
    if(finished_)throw std::logic_error("Cannot append to a finished motif index.");
    const auto& m=hit.match;if(!m.width||m.width>static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())||m.start>static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())-m.width)throw std::invalid_argument("Invalid motif interval.");
    if(rows_.empty()||rows_.back().id!=hit.id){if(lookup_.contains(hit.id))throw std::invalid_argument("Motif rows must be contiguous.");if(!rows_.empty())flush(rows_.back());
        lookup_[hit.id]=rows_.size();Row row;row.id=hit.id;row.first=total;rows_.push_back(std::move(row));}
    auto& row=rows_.back();auto& run=row.pending;const std::uint64_t start=m.start,end=m.start+m.width;const unsigned flags=m.bothStrands?3:m.reversed?2:1;
    if(run.count){const auto last=run.start+(run.count-1)*run.step,lastEnd=end_at(run,run.count-1);
        if(start<last||(start==last&&end<=lastEnd))throw std::invalid_argument("Motif hits must be ordered and unique.");
        const auto step=start-last;const auto endStep=static_cast<std::int64_t>(end)-static_cast<std::int64_t>(lastEnd);
        if(flags==run.flags&&(run.count==1||(step==run.step&&endStep==run.endStep))){run.step=step;run.endStep=endStep;++run.count;}
        else{flush(row);run={start,end,0,1,0,flags};}
    }else run={start,end,0,1,0,flags};
    ++row.count;++total;forward+=(flags&1)!=0;reverse+=(flags&2)!=0;both+=flags==3;
}
void MotifIndex::finish(){if(finished_)return;if(!rows_.empty())flush(rows_.back());finished_=true;}
MotifIndex::Run MotifIndex::decode(const Row& row,std::size_t& offset){
    auto read=[&]{std::uint64_t n=0;unsigned shift=0;while(true){const auto c=row.data.at(offset++);n|=static_cast<std::uint64_t>(c&127)<<shift;if(!(c&128))return n;shift+=7;}};
    Run run;run.start=read();run.end=run.start+read();run.step=read();const auto zig=read();run.endStep=zig&1?-static_cast<std::int64_t>(zig/2)-1:static_cast<std::int64_t>(zig/2);run.count=read();run.flags=static_cast<unsigned>(read());return run;
}
MotifHit MotifIndex::at(std::uint64_t ordinal)const{
    if(!finished_||ordinal>=total)throw std::out_of_range("Motif hit ordinal.");
    auto row=std::upper_bound(rows_.begin(),rows_.end(),ordinal,[](auto n,const Row& r){return n<r.first;});--row;ordinal-=row->first;
    auto block=std::upper_bound(row->blocks.begin(),row->blocks.end(),ordinal,[](auto n,const Block& b){return n<b.ordinal;});--block;
    auto offset=block->offset;auto base=block->ordinal;
    while(true){const auto run=decode(*row,offset);if(ordinal<base+run.count){const auto i=ordinal-base,start=run.start+i*run.step;
            return {row->id,{static_cast<std::size_t>(start),static_cast<std::size_t>(end_at(run,i)-start),run.flags==2,run.flags==3}};}base+=run.count;}
}
std::vector<unsigned char> MotifIndex::coverage(SequenceId id,std::size_t begin,std::size_t end)const{
    if(end<begin)throw std::invalid_argument("Invalid coverage range.");
    std::vector<unsigned char> cells(end-begin,0);
    if(!finished_||cells.empty())return cells;
    const auto found=lookup_.find(id);if(found==lookup_.end())return cells;const auto& row=rows_[found->second];
    auto block=std::upper_bound(row.blocks.begin(),row.blocks.end(),begin,[](auto n,const Block& b){return n<b.maxEnd;});
    for(;block!=row.blocks.end()&&block->firstStart<end;++block){auto offset=block->offset;const auto next=block+1;const auto limit=next==row.blocks.end()?row.data.size():next->offset;
        while(offset<limit){const auto run=decode(row,offset);if(run.start>=end)break;
            auto a=bounds(static_cast<std::int64_t>(run.start),static_cast<std::int64_t>(run.step),run.count,0,static_cast<std::int64_t>(end));
            auto b=bounds(static_cast<std::int64_t>(run.end),run.endStep,run.count,static_cast<std::int64_t>(begin)+1,std::numeric_limits<std::int64_t>::max());
            const auto lo=std::max(a.first,b.first),hi=std::min(a.second,b.second);if(lo>=hi)continue;
            auto shade=[&](std::uint64_t s,std::uint64_t e){for(auto c=std::max<std::uint64_t>(begin,s);c<std::min<std::uint64_t>(end,e);++c)cells[static_cast<std::size_t>(c-begin)]|=1;};
            const auto firstStart=run.start+lo*run.step,lastStart=run.start+(hi-1)*run.step,firstEnd=end_at(run,lo),lastEnd=end_at(run,hi-1);
            if(std::min(firstEnd-firstStart,lastEnd-lastStart)>=run.step)shade(firstStart,std::max(firstEnd,lastEnd));
            else if(hi-lo<=2*cells.size())for(auto i=lo;i<hi;++i)shade(run.start+i*run.step,end_at(run,i));
            else for(std::size_t c=begin;c<end;++c){a=bounds(static_cast<std::int64_t>(run.start),static_cast<std::int64_t>(run.step),run.count,0,static_cast<std::int64_t>(c)+1);b=bounds(static_cast<std::int64_t>(run.end),run.endStep,run.count,static_cast<std::int64_t>(c)+1,std::numeric_limits<std::int64_t>::max());if(std::max(a.first,b.first)<std::min(a.second,b.second))cells[c-begin]|=1;}
            auto edges=[&](std::int64_t first,std::int64_t step,unsigned char flag){const auto range=bounds(first,step,run.count,static_cast<std::int64_t>(begin),static_cast<std::int64_t>(end));
                if(!step){if(range.first<range.second)cells[static_cast<std::size_t>(first)-begin]|=flag;}
                else for(auto i=range.first;i<range.second;++i)cells[static_cast<std::size_t>(first+static_cast<std::int64_t>(i)*step)-begin]|=flag;};
            edges(static_cast<std::int64_t>(run.start),static_cast<std::int64_t>(run.step),2);edges(static_cast<std::int64_t>(run.end)-1,run.endStep,4);
        }
    }
    return cells;
}
}
