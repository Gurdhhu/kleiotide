#include "core/abif_export.hpp"
#include "core/version.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace kleiotide {
namespace {
void check(const AbifCancel& cancel){if(cancel&&cancel())throw std::runtime_error("AB1 export cancelled.");}
char upper(char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;}
// Pascal lengths are signed bytes in some legacy readers. Keep metadata
// within 127 UTF-8 bytes; full sequence titles remain in the session.
std::string pstring(std::string_view text){auto n=std::min<std::size_t>(127,text.size());if(n<text.size())while(n&&(static_cast<unsigned char>(text[n])&0xc0)==0x80)--n;return std::string(1,static_cast<char>(n))+std::string(text.substr(0,n));}
void put(std::string& bytes,std::size_t at,std::uint32_t value,unsigned width){for(unsigned i=0;i<width;++i)bytes.at(at+i)=static_cast<char>(value>>(8*(width-1-i)));}
struct Tag {std::string name;unsigned number,type,width;std::size_t count;std::string bytes;};
}
AbifExport prepare_abif_export(const Sequence& row,AbifCancel cancel){
    check(cancel);if(!row.trace||row.kind!=RowKind::Nucleotide)throw std::invalid_argument("Choose a nucleotide sequence linked to a chromatogram.");validate_trace(row);
    AbifExport result;result.signal=row.trace->original;result.reversed=row.trace->reversed;result.sampleName=row.name;
    const auto& original=*result.signal;const auto samples=original.channels[0].size();
    constexpr std::size_t exportLimit=64*1024*1024;
    if(samples>(exportLimit-2048)/8||row.symbols.residue_count()>(exportLimit-2048-samples*8)/8)throw std::invalid_argument("Trace exceeds the 64 MiB AB1 compatibility-export limit.");
    for(std::size_t channel=0;channel<4;++channel){bool negative=false;std::int32_t high=0;std::size_t step=0;
        for(auto v:original.channels[channel]){if((++step&4095)==0)check(cancel);negative|=v<0;high=std::max(high,v);if(v<std::numeric_limits<std::int16_t>::min()||v>std::numeric_limits<std::uint16_t>::max())throw std::invalid_argument("AB1 compatibility export requires 16-bit signal amplitudes. This trace has wider values; retain it in a .kleio session.");}
        if(negative&&high>std::numeric_limits<std::int16_t>::max())throw std::invalid_argument("This signal mixes negative samples and amplitudes above 32767, which cannot be preserved in a common 16-bit AB1 channel.");
        result.channelTypes[channel]=high>std::numeric_limits<std::int16_t>::max()?3U:4U;
    }
    std::vector<std::optional<std::uint32_t>> positions;const auto& anchors=row.trace->originalCalls;std::size_t ordinal=0;
    result.calls.reserve(anchors.size());result.quality.reserve(anchors.size());positions.reserve(anchors.size());
    for(char c:row.symbols.text()){if((ordinal&4095)==0)check(cancel);if(is_gap(c)){++result.omittedGaps;continue;}result.calls+=c;const auto originalCall=anchors.at(ordinal++);std::uint8_t q=0;
        if(originalCall<0){++result.inserted;positions.push_back({});}
        else{const auto call=static_cast<std::size_t>(originalCall);const auto peak=original.peaks[call];positions.push_back(static_cast<std::uint32_t>(result.reversed?samples-1-peak:peak));
            const auto expected=result.reversed?trace_complement(original.calls[call]):original.calls[call];
            if(upper(c)!=upper(expected))++result.substituted;
            else if(!original.quality.empty()){q=original.quality[call];++result.measuredQuality;}
            else ++result.missingQuality;
        }result.quality.push_back(q);
    }
    if(result.calls.empty())throw std::invalid_argument("The current sequence has no non-gap calls to export.");
    // Inserted calls have only display positions, never measured peaks or new signal.
    // Interpolate runs in signal coordinates, with the signal edges as virtual anchors.
    for(std::size_t begin=0;begin<positions.size();){check(cancel);if(positions[begin]){++begin;continue;}auto end=begin;while(end<positions.size()&&!positions[end]){if((end&4095)==0)check(cancel);++end;}
        const double lo=begin?static_cast<double>(*positions[begin-1]):0.;const double hi=end<positions.size()?static_cast<double>(*positions[end]):static_cast<double>(samples-1);
        const auto count=end-begin;for(std::size_t i=0;i<count;++i){if((i&4095)==0)check(cancel);positions[begin+i]=static_cast<std::uint32_t>(std::llround(lo+(hi-lo)*static_cast<double>(i+1)/static_cast<double>(count+1)));}begin=end;
    }
    for(const auto& position:positions){if(!position||*position>65535)throw std::invalid_argument("Current peak positions exceed the 16-bit AB1 compatibility range (65535 samples).");
        if(!result.peaks.empty()){if(*position<result.peaks.back())throw std::invalid_argument("Trace call positions are out of order.");if(*position==result.peaks.back())++result.coincident;}
        result.peaks.push_back(static_cast<std::uint16_t>(*position));
    }
    check(cancel);return result;
}
void write_abif_export(std::ostream& output,const AbifExport& plan,AbifCancel cancel){
    check(cancel);if(!plan.signal||plan.calls.empty()||plan.peaks.size()!=plan.calls.size()||plan.quality.size()!=plan.calls.size())throw std::invalid_argument("Invalid AB1 export plan.");
    if(!std::all_of(plan.calls.begin(),plan.calls.end(),[](char c){return is_nucleotide_symbol(c)&&!is_gap(c);}))throw std::invalid_argument("Invalid exported AB1 calls.");
    const auto samples=plan.signal->channels[0].size();if(!samples)throw std::invalid_argument("Empty AB1 signal.");
    std::vector<Tag> tags;auto add=[&](std::string name,unsigned n,unsigned type,unsigned width,std::size_t count,std::string bytes){if(bytes.size()!=count*width)throw std::invalid_argument("Inconsistent AB1 export tag.");tags.push_back({std::move(name),n,type,width,count,std::move(bytes)});};
    for(unsigned set:{1U,2U}){add("PBAS",set,2,1,plan.calls.size(),plan.calls);std::string peaks(plan.peaks.size()*2,'\0');
        for(std::size_t i=0;i<plan.peaks.size();++i){if((i&4095)==0)check(cancel);if(plan.peaks[i]>=samples||(i&&plan.peaks[i]<plan.peaks[i-1]))throw std::invalid_argument("Invalid exported AB1 peak positions.");put(peaks,i*2,plan.peaks[i],2);}
        const auto peakType=*std::max_element(plan.peaks.begin(),plan.peaks.end())>32767?3U:4U;add("PLOC",set,peakType,2,plan.peaks.size(),std::move(peaks));
        add("PCON",set,2,1,plan.quality.size(),std::string(reinterpret_cast<const char*>(plan.quality.data()),plan.quality.size()));
    }
    add("FWO_",1,2,1,4,"GATC");constexpr std::array<std::size_t,4> order={2,0,3,1};
    // BioEdit counts DATA occurrences instead of reading their tag numbers.
    // Empty DATA1-8 keep ordinal positions without inventing acquisition data.
    for(unsigned i=1;i<=8;++i)add("DATA",i,4,2,0,{});
    for(std::size_t i=0;i<4;++i){check(cancel);const auto channel=plan.reversed?3-order[i]:order[i];const auto& values=plan.signal->channels[channel];if(values.size()!=samples)throw std::invalid_argument("Unequal exported AB1 channels.");const auto type=plan.channelTypes[channel];if(type!=3&&type!=4)throw std::invalid_argument("Invalid exported AB1 signal type.");std::string bytes(samples*2,'\0');
        for(std::size_t j=0;j<samples;++j){if((j&4095)==0)check(cancel);const auto value=values[plan.reversed?samples-1-j:j];if(value<(type==3?0:-32768)||value>(type==3?65535:32767))throw std::invalid_argument("Signal value exceeds the AB1 export type.");put(bytes,j*2,static_cast<std::uint16_t>(value),2);}add("DATA",static_cast<unsigned>(9+i),type,2,samples,std::move(bytes));
    }
    auto sample=pstring(plan.sampleName);add("SMPL",1,18,1,sample.size(),sample);
    auto comment=pstring(std::string("Kleiotide ")+version+" edited calls. Q0 = changed/inserted/no-Q. Raw/auxiliary DATA1-8 empty. Instrument metadata unavailable.");add("CMNT",1,18,1,comment.size(),comment);
    const float spacing=plan.peaks.size()>1?std::max(1.f,static_cast<float>(plan.peaks.back()-plan.peaks.front())/static_cast<float>(plan.peaks.size()-1)):1.f;std::string space(4,'\0');put(space,0,std::bit_cast<std::uint32_t>(spacing),4);add("SPAC",1,7,4,1,std::move(space));
    auto stringTag=[&](std::string tag,unsigned number,std::string_view value){auto payload=pstring(value);add(std::move(tag),number,18,1,payload.size(),payload);};
    stringTag("SPAC",2,"Kleiotide edited calls");stringTag("SVER",1,"Unavailable");stringTag("SVER",2,std::string("Kleiotide ")+version);
    stringTag("GELN",1,"Unavailable");for(unsigned set:{1U,2U})stringTag("PDMF",set,"Unavailable");
    // Legacy readers access these fields unconditionally. Unknown instrument
    // metadata is explicitly marked. A valid epoch-date placeholder is needed
    // for readers rejecting zero dates; CMNT2 identifies it as a placeholder.
    add("MODL",1,2,1,4,"UNKN");add("LANE",1,4,2,1,std::string(2,'\0'));add("S/N%",1,4,2,4,std::string(8,'\0'));
    std::string unknownDate(4,'\0');put(unknownDate,0,1970,2);unknownDate[2]=unknownDate[3]=1;
    for(unsigned set:{1U,2U}){add("RUND",set,10,4,1,unknownDate);add("RUNT",set,11,4,1,std::string(4,'\0'));}
    stringTag("CMNT",2,"Compatibility placeholders: run date/time 1970-01-01 00:00:00; lane, SNR and analysis bounds 0 mean unknown.");
    add("ASPt",1,4,2,1,std::string(2,'\0'));add("AEPt",1,4,2,1,std::string(2,'\0'));
    std::sort(tags.begin(),tags.end(),[](const auto& a,const auto& b){return a.name<b.name||(a.name==b.name&&a.number<b.number);});
    constexpr std::size_t header=128,limit=64*1024*1024;const auto directorySize=tags.size()*28;std::size_t total=header+directorySize+3;
    for(const auto& t:tags)if(t.bytes.size()>4){total=(total+3)&~std::size_t(3);if(total>limit||t.bytes.size()>limit-total)throw std::invalid_argument("AB1 export exceeds the 64 MiB compatibility limit.");total+=t.bytes.size();}
    // BioEdit scans byte-by-byte from the root directory offset to EOF. Put
    // the directory last so sample names, comments and signal bytes cannot be
    // mistaken for tags. Standard ABIF readers follow the explicit offsets.
    std::string file(header,'\0');file.reserve(total);std::vector<std::uint32_t> offsets(tags.size());
    for(std::size_t i=0;i<tags.size();++i)if(tags[i].bytes.size()>4){check(cancel);file.resize((file.size()+3)&~std::size_t(3),'\0');offsets[i]=static_cast<std::uint32_t>(file.size());file+=tags[i].bytes;}
    file.resize((file.size()+3)&~std::size_t(3),'\0');const auto directory=file.size();file.resize(directory+directorySize,'\0');file.replace(0,4,"ABIF");put(file,4,101,2);file.replace(6,4,"tdir");put(file,10,1,4);put(file,14,1023,2);put(file,16,28,2);put(file,18,static_cast<std::uint32_t>(tags.size()),4);put(file,22,static_cast<std::uint32_t>(directorySize),4);put(file,26,static_cast<std::uint32_t>(directory),4);
    for(std::size_t i=0;i<tags.size();++i){check(cancel);const auto& t=tags[i];const auto at=directory+i*28;file.replace(at,4,t.name);put(file,at+4,t.number,4);put(file,at+8,t.type,2);put(file,at+10,t.width,2);put(file,at+12,static_cast<std::uint32_t>(t.count),4);put(file,at+16,static_cast<std::uint32_t>(t.bytes.size()),4);
        if(t.bytes.size()<=4)file.replace(at+20,t.bytes.size(),t.bytes);else put(file,at+20,offsets[i],4);
    }
    check(cancel);output.write(file.data(),static_cast<std::streamsize>(file.size()));if(!output)throw std::runtime_error("Could not write AB1 export.");
}
}
