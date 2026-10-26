#include "core/abif.hpp"
#include "core/chromatogram.hpp"
#include <algorithm>
#include <cstring>
#include <set>
#include <optional>
#include <map>
#include <span>
#include <stdexcept>

namespace kleiotide {
namespace {
using Bytes=std::span<const unsigned char>;
std::uint32_t number(Bytes bytes,std::size_t at,std::size_t width) {
    if(at>bytes.size()||width>bytes.size()-at)throw std::runtime_error("Truncated ABIF field.");
    std::uint32_t result=0;for(std::size_t i=0;i<width;++i)result=(result<<8)|bytes[at+i];return result;
}
struct Entry { unsigned type,elementSize;std::size_t count;Bytes bytes; };
class Reader {
public:
    explicit Reader(std::istream& input) {
        constexpr std::size_t limit=64*1024*1024;
        std::array<char,16384> block{};
        while(input){input.read(block.data(),static_cast<std::streamsize>(block.size()));const auto count=input.gcount();
            if(storage.size()+static_cast<std::size_t>(count)>limit)throw std::runtime_error("ABIF file exceeds the 64 MiB safety limit.");
            storage.insert(storage.end(),block.data(),block.data()+count);
        }
        if(input.bad())throw std::runtime_error("Could not read ABIF file.");
        const Bytes all(storage);
        if(all.size()<34||std::memcmp(all.data(),"ABIF",4)!=0)throw std::runtime_error("Not an ABIF chromatogram.");
        const auto version=number(all,4,2);if(version<100||version>=200)throw std::runtime_error("Unsupported ABIF format version.");
        if(number(all,14,2)!=1023||number(all,16,2)!=28)throw std::runtime_error("Invalid ABIF root directory.");
        const auto count=number(all,18,4),size=number(all,22,4),offset=number(all,26,4);
        if(!count||count>100000||size<static_cast<std::uint64_t>(count)*28)
            throw std::runtime_error("Invalid ABIF directory bounds.");
        try{if(offset>all.size()||size>all.size()-offset)throw std::runtime_error("Invalid ABIF directory bounds.");entries=parse_directory(all,offset,count);}
        catch(const std::runtime_error&){
            if(!recover(all,offset,count,size))throw;
        }
    }
    std::string recoveryNote;
private:
    struct Raw {std::string name;unsigned n,type,element;std::size_t count,size,pointer,at;};
    using Entries=std::map<std::pair<std::string,unsigned>,Entry>;
    static std::vector<Raw> directory(Bytes all,std::size_t offset,std::size_t count){
        if(offset>all.size()||count>(all.size()-offset)/28)throw std::runtime_error("Invalid ABIF directory bounds.");
        std::vector<Raw> raw;std::set<std::pair<std::string,unsigned>> keys;
        for(std::size_t i=0;i<count;++i){
            const auto at=offset+i*28;std::string name(reinterpret_cast<const char*>(all.data()+at),4);
            if(std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32||c>126;}))throw std::runtime_error("Invalid ABIF directory tag name.");
            const auto n=number(all,at+4,4),type=number(all,at+8,2),element=number(all,at+10,2),elements=number(all,at+12,4),bytes=number(all,at+16,4);
            if(!keys.emplace(name,n).second)throw std::runtime_error("Duplicate ABIF directory tag: "+name);
            raw.push_back({name,n,type,element,elements,bytes,number(all,at+20,4),at});
        }
        return raw;
    }
    static Entries materialize(Bytes all,const std::vector<Raw>& raw,std::size_t boundary=0,std::size_t oldSize=0,std::int64_t delta=0){
        Entries result;
        for(const auto& tag:raw){
            auto bytes=tag.size,count=tag.count;
            std::int64_t payload=bytes<=4?static_cast<std::int64_t>(tag.at+20):static_cast<std::int64_t>(tag.pointer);
            if(bytes>4&&delta){
                if(tag.pointer>=boundary+oldSize)payload+=delta;
                else if(tag.pointer==boundary&&tag.size==oldSize&&tag.name=="SMPL"&&tag.type==18){bytes=static_cast<std::size_t>(static_cast<std::int64_t>(bytes)+delta);count=bytes;}
                else if(tag.pointer+bytes>boundary)throw std::runtime_error("ABIF recovery overlaps a non-name payload.");
            }
            if(payload<0||static_cast<std::size_t>(payload)>all.size()||bytes>all.size()-static_cast<std::size_t>(payload))throw std::runtime_error("ABIF tag payload is outside the file: "+tag.name);
            result.emplace(std::make_pair(tag.name,tag.n),Entry{tag.type,tag.element,count,all.subspan(static_cast<std::size_t>(payload),bytes)});
        }
        return result;
    }
    static Entries parse_directory(Bytes all,std::size_t offset,std::size_t count){return materialize(all,directory(all,offset,count));}
    bool recover(Bytes all,std::size_t declared,std::size_t count,std::size_t size){
        // Some name-replacement scripts expand a Pascal string in place without
        // fixing later offsets. Accept one uniquely identifiable expansion only:
        // nearby complete directory, one exact-size printable Pascal replacement,
        // constant offset shift, and valid dimensions/bounds for every payload.
        if(count>4096)return false;
        std::optional<Entries> recovered;std::int64_t recoveredDelta=0;
        const auto begin=declared>4096?declared-4096:34,end=std::min(all.size(),declared+4096);
        for(std::size_t actual=begin;actual<=end;++actual){
            if(actual==declared||actual>all.size()||size>all.size()-actual)continue;
            const auto delta=static_cast<std::int64_t>(actual)-static_cast<std::int64_t>(declared);
            std::vector<Raw> raw;try{raw=directory(all,actual,count);}catch(const std::runtime_error&){continue;}
            std::vector<std::pair<std::size_t,std::size_t>> ranges,candidates;
            bool valid=true;
            for(const auto& tag:raw){
                if(!tag.element||static_cast<std::uint64_t>(tag.count)*tag.element!=tag.size){valid=false;break;}
                if(tag.size>4){if(tag.pointer>declared||tag.size>declared-tag.pointer){valid=false;break;}ranges.emplace_back(tag.pointer,tag.pointer+tag.size);if(tag.name=="SMPL"&&tag.type==18)candidates.emplace_back(tag.pointer,tag.size);}
            }
            if(!valid)continue;
            std::sort(ranges.begin(),ranges.end());std::size_t previous=34;
            for(const auto& range:ranges){if(range.first>previous&&range.first-previous<=256)candidates.emplace_back(previous,range.first-previous);previous=std::max(previous,range.second);}
            std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
            for(const auto& [boundary,oldSize]:candidates){
                const auto expanded=static_cast<std::int64_t>(oldSize)+delta;
                if(expanded<1||expanded>256||boundary>=all.size()||static_cast<std::size_t>(expanded)>all.size()-boundary||all[boundary]+1!=expanded)continue;
                const auto name=all.subspan(boundary+1,static_cast<std::size_t>(expanded-1));
                if(std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32||c==127;}))continue;
                try{
                    auto candidate=materialize(all,raw,boundary,oldSize,delta);
                    bool strings=true;
                    for(const auto& [key,entry]:candidate){(void)key;if(entry.type==18&&!entry.bytes.empty()&&entry.bytes[0]>entry.bytes.size()-1){strings=false;break;}}
                    if(!strings)continue;
                    if(recovered)return false; // More than one plausible repair is unsafe.
                    recovered=std::move(candidate);recoveredDelta=delta;
                }catch(const std::runtime_error&){continue;}
            }
        }
        if(!recovered)return false;
        entries=std::move(*recovered);
        recoveryNote="Recovered stale ABIF offsets after an in-place sample-name edit ("+std::to_string(recoveredDelta)+" bytes). Calls, peaks, quality and signals passed normal validation; the source file was not modified.";
        return true;
    }
public:
    const Entry* find(const std::string& name,unsigned n)const {const auto i=entries.find({name,n});return i==entries.end()?nullptr:&i->second;}
    const Entry& get(const std::string& name,unsigned n)const {const auto* e=find(name,n);if(!e)throw std::runtime_error("Missing ABIF tag: "+name+std::to_string(n));return *e;}
private:
    std::vector<unsigned char> storage;
    std::map<std::pair<std::string,unsigned>,Entry> entries;
};
void shape(const Entry& entry) {
    if(!entry.elementSize||static_cast<std::uint64_t>(entry.count)*entry.elementSize!=entry.bytes.size())
        throw std::runtime_error("Inconsistent ABIF element dimensions.");
}
void chars(const Entry& entry) {shape(entry);if((entry.type!=1&&entry.type!=2)||entry.elementSize!=1)throw std::runtime_error("Invalid ABIF character tag.");}
void integers(const Entry& entry) {shape(entry);if(!((entry.type==3||entry.type==4)&&entry.elementSize==2)&&!(entry.type==5&&entry.elementSize==4))throw std::runtime_error("Unsupported ABIF integer tag.");}
std::int64_t integer(const Entry& entry,std::size_t i) {
    const auto value=number(entry.bytes,i*entry.elementSize,entry.elementSize);
    if(entry.type==4)return value>=0x8000?static_cast<std::int64_t>(value)-0x10000:value;
    if(entry.type==5)return value>=0x80000000U?static_cast<std::int64_t>(value)-0x100000000LL:value;
    return value;
}
}
Record read_abif(std::istream& input,std::string title,std::string sourcePath,std::string* recoveryNote) {
    if(recoveryNote)recoveryNote->clear();
    Reader reader(input);auto data=std::make_shared<Chromatogram>();data->sourcePath=std::move(sourcePath);
    // An edited call set must have its own positions. Never mix call/peak/quality sets.
    unsigned set=reader.find("PBAS",1)?1:2;
    const auto& calls=reader.get("PBAS",set);chars(calls);
    if(!calls.count)throw std::runtime_error("ABIF contains no base calls.");
    data->calls.assign(reinterpret_cast<const char*>(calls.bytes.data()),calls.bytes.size());data->callSet=set;
    // Some edited traces use X for an unknown nucleotide. Normalize only this
    // alias at the import boundary; never drop a call or switch to unedited PBAS2.
    std::size_t unknownCount=0;std::string unknownPositions;
    for(std::size_t i=0;i<data->calls.size();++i){auto& c=data->calls[i];
        if(c=='X'||c=='x'){
            c=c=='X'?'N':'n';++unknownCount;
            if(unknownCount<=10){if(!unknownPositions.empty())unknownPositions+=", ";unknownPositions+=std::to_string(i+1);}
        }else if(!is_nucleotide_symbol(c)||is_gap(c))
            throw std::runtime_error("Unexpected ABIF base-call symbol (byte "+std::to_string(static_cast<unsigned char>(c))+") in PBAS"+std::to_string(set)+" at call "+std::to_string(i+1)+".");
    }
    const auto& peaks=reader.get("PLOC",set);integers(peaks);
    if(peaks.count!=calls.count)throw std::runtime_error("ABIF base-call and peak counts differ.");
    for(std::size_t i=0;i<peaks.count;++i){const auto value=integer(peaks,i);if(value<0)throw std::runtime_error("Negative ABIF peak position.");data->peaks.push_back(static_cast<std::uint32_t>(value));}
    if(const auto* q=reader.find("PCON",set)){
        chars(*q);if(q->count!=calls.count)throw std::runtime_error("ABIF quality and call counts differ.");
        data->quality.assign(q->bytes.begin(),q->bytes.end());
    }
    const auto& order=reader.get("FWO_",1);chars(order);
    if(order.count!=4)throw std::runtime_error("ABIF dye order must contain four channels.");
    std::array<bool,4> seen{};constexpr std::string_view bases="ACGT";
    for(unsigned i=0;i<4;++i){
        const auto index=bases.find(static_cast<char>(order.bytes[i]));
        if(index==std::string_view::npos||seen[index])throw std::runtime_error("Invalid ABIF dye order.");
        seen[index]=true;
        const auto& channel=reader.get("DATA",9+i);integers(channel);auto& output=data->channels[index];output.reserve(channel.count);
        for(std::size_t j=0;j<channel.count;++j)output.push_back(static_cast<std::int32_t>(integer(channel,j)));
    }
    if(const auto* sample=reader.find("SMPL",1)){
        const auto bytes=sample->bytes;
        if(sample->type==18&&!bytes.empty()){
            if(bytes[0]>bytes.size()-1)throw std::runtime_error("Truncated ABIF sample name.");
            data->sampleName.assign(reinterpret_cast<const char*>(bytes.data()+1),bytes[0]);
        }else if(sample->type==19){auto end=std::find(bytes.begin(),bytes.end(),0);data->sampleName.assign(bytes.begin(),end);}
    }
    auto link=std::make_shared<TraceLink>();link->original=data;link->originalCalls.reserve(calls.count);
    for(std::size_t i=0;i<calls.count;++i)link->originalCalls.push_back(static_cast<std::int32_t>(i));
    Record record{std::move(title),data->calls,RowKind::Nucleotide,link};
    Sequence row{0,record.name,SequenceBuffer(record.symbols),record.kind,link};validate_trace(row);
    if(recoveryNote){
        *recoveryNote=reader.recoveryNote;
        if(unknownCount){
            if(!recoveryNote->empty())*recoveryNote+="\n";
            *recoveryNote+="Imported "+std::to_string(unknownCount)+" X/x unknown base call(s) as N/n from PBAS"+std::to_string(set)+" at one-based position(s): "+unknownPositions+(unknownCount>10?", ...":"")+". Case, call positions, quality and signals were retained; the source file was not modified.";
        }
    }
    return record;
}
}
