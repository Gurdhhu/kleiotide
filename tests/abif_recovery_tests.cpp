#include "core/abif.hpp"
#include "core/abif_export.hpp"
#include "core/session.hpp"
#include "abif_fixture.hpp"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
unsigned number(const std::string& b,std::size_t at,unsigned width=4){unsigned v=0;for(unsigned i=0;i<width;++i)v=(v<<8)|static_cast<unsigned char>(b.at(at+i));return v;}
struct Fixture {std::string bytes;std::size_t name=0,oldSize=6,directory=0;};
Fixture fixture(bool retired){
    const auto original=test::synthetic_abif("ACGTRNACGT");const auto count=number(original,18),directory=number(original,26);
    Fixture f;f.bytes=original.substr(0,128);std::vector<std::string> tags;
    const auto sample=std::string("\5demo1",6);if(retired){const auto first=original.substr(directory,28);f.bytes+=original.substr(number(first,20),number(first,16));}f.name=f.bytes.size();f.bytes+=sample;
    for(unsigned i=0;i<count;++i){auto entry=original.substr(directory+28*i,28);const auto size=number(entry,16);
        if(size>4){const auto payload=original.substr(number(entry,20),size);const auto offset=entry.substr(0,4)=="SMPL"&&!retired?f.name:retired&&entry.substr(0,4)=="PBAS"?128:f.bytes.size();test::put(entry,20,static_cast<unsigned>(offset),4);if((entry.substr(0,4)!="SMPL"||retired)&&(!retired||entry.substr(0,4)!="PBAS"))f.bytes+=payload;}
        tags.push_back(std::move(entry));
    }
    f.directory=f.bytes.size();test::put(f.bytes,26,static_cast<unsigned>(f.directory),4);
    for(const auto& entry:tags)f.bytes+=entry;
    f.bytes.append(28,'\0');return f;
}
std::pair<std::string,std::string> rename(const Fixture& f,const std::string& name){
    const auto replacement=std::string(1,static_cast<char>(name.size()))+name;
    auto damaged=f.bytes;damaged.replace(f.name,f.oldSize,replacement);auto repaired=damaged;
    const auto delta=static_cast<std::int64_t>(replacement.size())-static_cast<std::int64_t>(f.oldSize);
    const auto actual=static_cast<std::size_t>(static_cast<std::int64_t>(f.directory)+delta);test::put(repaired,26,static_cast<unsigned>(actual),4);
    for(unsigned i=0;i<number(f.bytes,18);++i){const auto at=actual+28*i;const auto old=f.directory+28*i;const auto bytes=number(f.bytes,old+16),pointer=number(f.bytes,old+20);
        if(bytes<=4)continue;
        if(pointer==f.name&&bytes==f.oldSize){test::put(repaired,at+12,static_cast<unsigned>(replacement.size()),4);test::put(repaired,at+16,static_cast<unsigned>(replacement.size()),4);if(replacement.size()<=4){test::put(repaired,at+20,0,4);repaired.replace(at+20,replacement.size(),replacement);}}
        else if(pointer>=f.name+f.oldSize)test::put(repaired,at+20,static_cast<unsigned>(static_cast<std::int64_t>(pointer)+delta),4);
    }
    return {damaged,repaired};
}
Record parse(const std::string& bytes,std::string* note=nullptr){std::istringstream in(bytes);return read_abif(in,"trace","source.ab1",note);}
std::string with_calls(std::string bytes,const std::string& calls){const auto directory=number(bytes,26);for(unsigned i=0;i<number(bytes,18);++i){const auto at=directory+28*i;if(bytes.substr(at,4)=="PBAS"){need(number(bytes,at+12)==calls.size(),"Same call count");bytes.replace(calls.size()<=4?at+20:number(bytes,at+20),calls.size(),calls);return bytes;}}throw std::runtime_error("Missing PBAS fixture");}
std::string with_unedited_set(std::string bytes){
    const auto old=number(bytes,26),count=number(bytes,18);const auto directory=bytes.size();bytes+=bytes.substr(old,count*28);
    for(const auto& name:std::vector<std::string>{"PBAS","PLOC","PCON"}){std::string entry(28,'\0');entry.replace(0,4,name);test::put(entry,4,2,4);test::put(entry,8,name=="PLOC"?4:2,2);test::put(entry,10,name=="PLOC"?2:1,2);test::put(entry,12,2,4);test::put(entry,16,name=="PLOC"?4:2,4);if(name=="PBAS")entry.replace(20,2,"TT");else if(name=="PLOC"){test::put(entry,20,10,2);test::put(entry,22,20,2);}else entry.replace(20,2,"!!");bytes+=entry;}
    bytes.append(28,'\0');test::put(bytes,18,count+3,4);test::put(bytes,22,(count+3)*28+28,4);test::put(bytes,26,static_cast<unsigned>(directory),4);return bytes;
}
void same(const Record& a,const Record& b){const auto& x=*a.trace->original;const auto& y=*b.trace->original;need(x.calls==y.calls&&x.peaks==y.peaks&&x.quality==y.quality&&x.channels==y.channels&&x.sampleName==y.sampleName,"Exact calls, peaks, Q, all channel samples and sample metadata");}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}need(threw,"Expected malformed recovery to reject");}
}
int main(int argc,char** argv){unsigned passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Renamed live and retired Pascal payloads recover without evidence changes",[]{for(bool retired:{false,true})for(const auto& name:std::vector<std::string>{"renamed_long_internal_name_123","a","",std::string(255,'Z')}){const auto f=fixture(retired);const auto [damaged,reference]=rename(f,name);std::string note;const auto recovered=parse(damaged,&note);same(recovered,parse(reference));need(!note.empty()&&note.find("source file was not modified")!=std::string::npos,"Recovery is reported after full trace validation");}});
run("Clean files do not invoke recovery and clear old notes",[]{std::string note="old warning";parse(fixture(false).bytes,&note);need(note.empty(),"No false recovery note");});
run("X/x becomes N/n without changing evidence or choosing the unedited set",[]{for(unsigned set:{1U,2U}){
    const auto reference=test::synthetic_abif("ACNnTG",set);auto input=with_calls(reference,"ACXxTG");if(set==1)input=with_unedited_set(input);
    std::string note;const auto r=parse(input,&note);same(r,parse(reference));need(r.symbols=="ACNnTG"&&r.trace->original->callSet==set&&r.trace->originalCalls==std::vector<std::int32_t>({0,1,2,3,4,5}),"Case/count/identity/selected set retained");
    need(note.find("2 X/x")!=std::string::npos&&note.find("3, 4")!=std::string::npos&&note.find("PBAS"+std::to_string(set))!=std::string::npos,"Explicit normalization note");
    Document d;d.load({r});std::ostringstream saved(std::ios::binary);write_session(saved,d);std::istringstream session(saved.str(),std::ios::binary);const auto restored=read_session(session);const auto& trace=*restored.document.sequence(1).trace->original;need(trace.calls==r.symbols&&trace.peaks==r.trace->original->peaks&&trace.quality==r.trace->original->quality&&trace.channels==r.trace->original->channels,"Portable session retains normalized evidence");
    std::ostringstream exported(std::ios::binary);write_abif_export(exported,prepare_abif_export(d.sequence(1)));const auto reread=parse(exported.str());need(reread.symbols==r.symbols&&reread.trace->original->peaks==trace.peaks&&reread.trace->original->quality==trace.quality&&reread.trace->original->channels==trace.channels,"AB1 re-export keeps calls and measured evidence");
}});
run("Normalization notes combine with recovery, stay bounded and follow validation",[]{
    const auto [damaged,reference]=rename(fixture(false),"long_renamed_sample");(void)reference;auto b=damaged;const auto at=b.find("ACGTRNACGT");need(at!=std::string::npos,"Fixture calls");b[at]='x';std::string note;parse(b,&note);need(note.find("Recovered stale")!=std::string::npos&&note.find("1 X/x")!=std::string::npos,"Both import notes retained");
    parse(test::synthetic_abif(std::string(1000,'x')),&note);need(note.find("1000 X/x")!=std::string::npos&&note.find("...")!=std::string::npos&&note.size()<400,"Large call lists do not flood the dialog");
    b=test::synthetic_abif("ACXxTG");const auto directory=number(b,26);test::put(b,directory+28+12,1,4);rejects([&]{parse(b,&note);});need(note.empty(),"No success note for invalid trace");
});
run("Other illegal call bytes reject with a precise position",[]{for(char c:std::string("Zz*-~\x01",7)){auto b=test::synthetic_abif("ACGTTG");std::string calls="ACGTTG";calls[2]=c;b=with_calls(b,calls);bool rejected=false;try{parse(b);}catch(const std::runtime_error& e){const std::string message=e.what();rejected=message.find("PBAS1 at call 3")!=std::string::npos;}need(rejected,"Unrelated corruption is not normalized");}});
run("Invalid metadata, arbitrary offsets and non-name corruption still reject",[]{const auto f=fixture(true);auto [damaged,reference]=rename(f,"long_internal_name");(void)reference;auto b=damaged;const auto delta=b.size()-f.bytes.size(),directory=f.directory+delta;test::put(b,directory+20,0xfffffff0,4);rejects([&]{parse(b);});b=damaged;b[f.name+2]='\1';rejects([&]{parse(b);});b=damaged;test::put(b,26,static_cast<unsigned>(f.directory-5000),4);rejects([&]{parse(b);});b=damaged;b.resize(b.size()-200);rejects([&]{parse(b);});});
run("Recovered trace remains editable, exportable and independently rereadable",[]{const auto [damaged,reference]=rename(fixture(false),"long_renamed_sample");(void)reference;Document d;d.load({parse(damaged)});const auto original=d.sequence(1).trace->original;d.replace(1,2,1,"T");d.transform({1},Document::Transform::ReverseComplement);std::ostringstream out(std::ios::binary);write_abif_export(out,prepare_abif_export(d.sequence(1)));auto exported=parse(out.str());need(exported.symbols==d.sequence(1).symbols.text()&&d.sequence(1).trace->original==original,"Recovery feeds normal immutable editing/export model");});
for(int i=1;i<argc;++i)run(argv[i],[&,i]{std::ifstream in(argv[i],std::ios::binary);need(bool(in),"Open real input");std::string note;const auto r=read_abif(in,"real",argv[i],&note);need(!note.empty()&&r.symbols.size()==580&&r.trace->original->channels[0].size()==6998,"User file recovered with expected dimensions");std::cout<<"  "<<note<<'\n';});
std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;}
