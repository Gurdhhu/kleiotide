#include "core/abif.hpp"
#include "core/session.hpp"
#include "abif_fixture.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <functional>
#include <random>
#include <stdexcept>
using namespace kleiotide;
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
Document example(){std::istringstream input(test::synthetic_abif());auto record=read_abif(input,"trace Я","moved/source.ab1");Document d;d.load({record,{"mask","12~4",RowKind::Mask},{"plain","aN-~C"},record});d.replace(1,1,0,"W");d.replace(1,3,1,"a");d.replace(1,5,1,"");d.transform({4},Document::Transform::ReverseComplement);d.rename({{1,"edited trace"}});d.reorder({4,1,3,2});return d;}
std::string bytes(const Document& d){std::ostringstream out(std::ios::binary);write_session(out,d,"{\"version\":1}","report\nline two");return out.str();}
Session parse(const std::string& text){std::istringstream in(text,std::ios::binary);return read_session(in);}
void rejects(const std::string& text){try{(void)parse(text);}catch(const std::exception&){return;}throw std::runtime_error("Malformed session accepted");}
int main(int argc,char** argv){int passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Session retains edits, row kind/case/gaps/IDs and embedded shared evidence",[]{const auto d=example();const auto serialized=bytes(d);const auto saved=parse(serialized);const auto& rows=saved.document.rows();require(rows.size()==4,"row count");
    for(std::size_t i=0;i<rows.size();++i){const auto& a=d.rows()[i];const auto& b=rows[i];require(a.id==b.id&&a.name==b.name&&a.kind==b.kind&&a.symbols.text()==b.symbols.text(),"exact row roundtrip");if(a.trace){require(b.trace&&a.trace->originalCalls==b.trace->originalCalls&&a.trace->reversed==b.trace->reversed,"mapping roundtrip");const auto& x=*a.trace->original;const auto& y=*b.trace->original;require(x.calls==y.calls&&x.channels==y.channels&&x.peaks==y.peaks&&x.quality==y.quality&&x.sourcePath==y.sourcePath&&x.sampleName==y.sampleName&&x.callSet==y.callSet,"original evidence");}}
    require(rows[0].trace->original==rows[1].trace->original,"original evidence deduplicated");require(saved.viewState=="{\"version\":1}"&&saved.alignmentReport=="report\nline two","metadata");require(!saved.document.can_undo(),"reopened baseline has fresh history");require(bytes(saved.document)==serialized,"deterministic serialization");
});
run("Sessions reopen and remain editable without original AB1 files",[]{auto d=parse(bytes(example())).document;auto old=d.sequence(1).trace;d.replace(1,2,0,"NN");validate_trace(d.sequence(1));d.undo();require(d.sequence(1).trace==old,"fresh undo after reopening");d.append({{"new","ACGT"}});require(d.rows().back().id==5,"stable next ID");});
run("Empty documents and forced nucleotide-looking masks roundtrip",[]{Document empty;require(parse(bytes(empty)).document.rows().empty(),"empty session");Document d;d.load({{"mask","ACGT",RowKind::Mask},{"empty",""}});require(parse(bytes(d)).document.rows()[0].kind==RowKind::Mask,"explicit mask retained");});
run("Bad IDs reject before document mutation",[]{auto d=example();const auto before=bytes(d);for(const auto& ids:std::vector<std::vector<SequenceId>>{{0},{1,1},{7}}){try{std::vector<Record> r(ids.size(),{"new","A"});d.restore_session(std::move(r),ids,6);throw std::logic_error("bad ID accepted");}catch(const std::invalid_argument&){}require(bytes(d)==before,"atomic restore");}});
run("Version, truncation, checksum, trailing bytes and huge fake lengths rejected",[]{const auto good=bytes(example());for(std::size_t i=0;i<good.size();i+=17)rejects(good.substr(0,i));auto bad=good;bad[8]=6;rejects(bad);bad=good;bad.back()^=1;rejects(bad);rejects(good+"x");bad=good;for(unsigned i=0;i<8;++i)bad[20+i]=static_cast<char>(255);rejects(bad);});
run("2000 malformed session mutations fail safely",[]{std::mt19937 random(909);const auto good=bytes(example());for(unsigned i=0;i<2000;++i){auto bad=good;bad[random()%bad.size()]^=static_cast<char>(1+random()%255);rejects(bad);}});

run("Independent version-1 fixture migrates without reclassifying old masks or losing evidence",[]{
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/session_v1.kleio",std::ios::binary);require(bool(file),"Legacy fixture is available");const auto old=read_session(file);
    require(old.formatVersion==1&&old.document.rows().size()==4&&old.document.next_sequence_id()==20,"Legacy version/count/next-ID decoded");
    require(old.document.sequence(15).kind==RowKind::Mask&&old.document.sequence(16).kind==RowKind::Mask,"Nucleotide/protein-looking legacy masks stay masks");
    const auto trace=old.document.sequence(9).trace;require(trace&&trace->original->calls=="AC"&&trace->originalCalls==std::vector<std::int32_t>({0,-1,1})&&trace->original->quality==std::vector<std::uint8_t>({30,35}),"Old embedded signal/inserted-call provenance retained");
    const auto upgraded=parse(bytes(old.document));require(upgraded.formatVersion==session_version&&upgraded.document.sequence(15).kind==RowKind::Mask&&upgraded.document.sequence(16).kind==RowKind::Mask,"New save explicitly upgrades format without guessing types");
    require(upgraded.document.sequence(9).trace->original->channels==trace->original->channels&&upgraded.document.sequence(9).trace->originalCalls==trace->originalCalls,"Migration preserves exact samples/mapping");
});
run("Version-2 protein type, canonical representation, stops and ambiguities roundtrip",[]{
    auto d=example();d.append({{"overlapping alphabet","ACGT",RowKind::Protein},{"protein","MeFbzjxUO*?-~",RowKind::Protein}});const auto saved=parse(bytes(d));
    require(saved.formatVersion==session_version&&saved.document.sequence(5).kind==RowKind::Protein&&saved.document.sequence(5).symbols.text()=="ACGT","Nucleotide-looking protein retains declared type");
    require(saved.document.sequence(6).representation==RowRepresentation::AlignedSymbols&&saved.document.sequence(6).symbols.text()=="MeFbzjxUO*?-~","One-letter canonical storage/case/gaps/stop retained");
    require(saved.document.sequence(1).trace->original->channels==d.sequence(1).trace->original->channels,"Mixed document preserves original trace samples");
});
run("Checksummed unsupported types/representations and invalid declared alphabets reject",[]{
    Document d;d.load({{"P","MEF",RowKind::Protein}});std::ostringstream out(std::ios::binary);write_session(out,d);const auto good=out.str();
    auto withCRC=[](std::string data){std::uint32_t crc=0xffffffffU;for(std::size_t i=0;i<data.size()-4;++i){crc^=static_cast<unsigned char>(data[i]);for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320U:0U);}crc^=0xffffffffU;for(unsigned i=0;i<4;++i)data[data.size()-4+i]=static_cast<char>(crc>>(8*i));return data;};
    // Empty view/report, no traces: first row starts at byte 82; kind/encoding
    // follow its u64 ID. Checksums are recomputed so semantic validation is tested.
    require(good.size()==133&&static_cast<unsigned char>(good[90])==2,"Known portable single-row layout");
    auto bad=good;bad[90]=3;rejects(withCRC(bad));bad=good;bad[91]=1;rejects(withCRC(bad));
    bad=good;bad[90]=0;rejects(withCRC(bad));bad=good;bad[117]='1';rejects(withCRC(bad));bad=good;bad[92]=1;rejects(withCRC(bad));bad=good;bad[44]=0;rejects(withCRC(bad));
    bad=good;bad[8]=6;rejects(withCRC(bad));
});

run("Independent format-3 group/protein fixture migrates to current format",[]{std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/session_v3.kleio",std::ios::binary);const auto old=read_session(file);require(old.formatVersion==3&&old.document.groups().size()==1&&old.document.sequence(1).kind==RowKind::Protein&&old.document.sequence(2).kind==RowKind::Mask&&old.document.sequence(1).groupId==1,"Format 3 semantic migration");const auto upgraded=parse(bytes(old.document));require(upgraded.formatVersion==session_version&&upgraded.document.groups()==old.document.groups(),"Upgrade preserves group catalog");});

for(int i=1;i<argc;++i)run(argv[i],[&,i]{std::ifstream input(argv[i],std::ios::binary);require(bool(input),"Open supplied AB1");Document d;d.load({read_abif(input,"supplied",argv[i])});const auto original=d.rows()[0].trace->original;d.replace(1,20,0,"WR");d.replace(1,35,1,"");d.transform({1},Document::Transform::ReverseComplement);const auto session=parse(bytes(d));const auto restored=session.document.rows()[0].trace;require(restored->original->channels==original->channels&&restored->original->quality==original->quality&&restored->original->peaks==original->peaks&&restored->originalCalls==d.rows()[0].trace->originalCalls&&restored->reversed,"Real evidence/mapping roundtrip");});
std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;}
