#pragma once
// Entirely synthetic evidence; never derived from a research trace.
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
namespace kleiotide::test {
inline void put(std::string& out,std::size_t at,std::uint32_t value,unsigned width){for(unsigned i=0;i<width;++i)out[at+i]=static_cast<char>(value>>(8*(width-1-i)));}
inline std::string synthetic_abif(std::string calls="ACGTRN",unsigned set=1){
    struct Tag{std::string name;unsigned n,type,width,count;std::string payload;};std::vector<Tag> tags;
    auto add=[&](std::string name,unsigned n,unsigned type,unsigned width,unsigned count,std::string payload){tags.push_back({name,n,type,width,count,std::move(payload)});};
    add("PBAS",set,2,1,static_cast<unsigned>(calls.size()),calls);
    std::string peaks(calls.size()*2,'\0'),quality(calls.size(),'\0');
    for(std::size_t i=0;i<calls.size();++i){put(peaks,i*2,static_cast<unsigned>(30+i*20),2);quality[i]=static_cast<char>(20+i);}
    add("PLOC",set,4,2,static_cast<unsigned>(calls.size()),peaks);add("PCON",set,2,1,static_cast<unsigned>(calls.size()),quality);
    add("FWO_",1,2,1,4,"GATC");
    const auto samples=static_cast<unsigned>(calls.size()*20+60);
    const std::string dye="GATC";
    for(unsigned channel=0;channel<4;++channel){std::string signal(samples*2,'\0');
        for(unsigned s=0;s<samples;++s){double value=-1.;for(std::size_t c=0;c<calls.size();++c){
            const double distance=static_cast<double>(s)-static_cast<double>(30+c*20);
            if(calls[c]==dye[channel]||calls[c]=='R'||calls[c]=='N')value+=(50.+channel*70.)*std::exp(-distance*distance/20.);
        }put(signal,s*2,static_cast<std::uint16_t>(static_cast<std::int16_t>(value)),2);}
        add("DATA",9+channel,4,2,samples,signal);
    }
    add("SMPL",1,18,1,6,std::string("\5demo1",6));
    constexpr std::size_t directory=128;std::string out(directory+tags.size()*28+28,'\0');out.replace(0,4,"ABIF");put(out,4,101,2);
    out.replace(6,4,"tdir");put(out,10,1,4);put(out,14,1023,2);put(out,16,28,2);put(out,18,static_cast<unsigned>(tags.size()),4);put(out,22,static_cast<unsigned>(tags.size()*28+28),4);put(out,26,directory,4);
    for(std::size_t i=0;i<tags.size();++i){const auto& t=tags[i];const auto at=directory+i*28;out.replace(at,4,t.name);put(out,at+4,t.n,4);put(out,at+8,t.type,2);put(out,at+10,t.width,2);put(out,at+12,t.count,4);put(out,at+16,static_cast<unsigned>(t.payload.size()),4);
        if(t.payload.size()<=4)out.replace(at+20,t.payload.size(),t.payload);
        else{put(out,at+20,static_cast<unsigned>(out.size()),4);out+=t.payload;}
    }return out;
}
}
