#include "core/abif.hpp"
#include "core/fasta.hpp"
#include "core/mafft_alignment.hpp"
#include "core/mafft_template.hpp"
#include "abif_fixture.hpp"
#include <fstream>
#include <random>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
Record parse(const std::string& bytes){std::istringstream stream(bytes);return read_abif(stream,"Trace Я","original.ab1");}
void rejects(const std::string& bytes){try{(void)parse(bytes);}catch(const std::exception&){return;}throw std::runtime_error("Malformed ABIF accepted");}
Document example(std::string calls="ACGTRN"){Document d;d.load({parse(test::synthetic_abif(calls))});return d;}
}
int main(int argc,char** argv){int passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
    run("ABIF endian, padded directory, inline dye order and sample name",[]{auto d=example();const auto& data=*d.rows()[0].trace->original;
        require(data.calls=="ACGTRN"&&data.peaks==std::vector<std::uint32_t>({30,50,70,90,110,130}),"calls/peaks");
        require(data.callSet==1&&data.sampleName=="demo1"&&data.quality[3]==23,"quality/sample");
        require(data.channels[0][30]==119&&data.channels[1][50]==259&&data.channels[2][70]==49&&data.channels[3][90]==189,"canonical ACGT mapping");
        require(data.channels[0][0]==-1,"signed signal");
        const auto p=trace_position(d.rows()[0],3);require(p.originalCall==3&&p.sample==90&&p.quality==23&&!p.edited,"original position");
    });
    run("PBAS2 fallback and missing quality",[]{auto bytes=test::synthetic_abif("ACGT",2);bytes.replace(128+2*28,4,"NONE");const auto r=parse(bytes);require(r.trace->original->callSet==2&&r.trace->original->quality.empty(),"coherent fallback");});
    run("Malformed ABIF magic/version/root/offset/dimensions/duplicate/dye/peaks",[]{const auto original=test::synthetic_abif();
        rejects(original.substr(0,20));auto b=original;b[0]='X';rejects(b);b=original;test::put(b,4,200,2);rejects(b);
        b=original;test::put(b,16,27,2);rejects(b);b=original;test::put(b,18,100001,4);rejects(b);
        b=original;test::put(b,26,0xfffffff0,4);rejects(b);b=original;test::put(b,128+20,0xffffff00,4);rejects(b);
        b=original;test::put(b,128+12,100,4);rejects(b);b=original;b.replace(128+28,8,b.substr(128,8));rejects(b);
        b=original;b.replace(128+3*28+20,4,"AATC");rejects(b);b=original;b.replace(128+28,4,"NONE");rejects(b);
        b=original;test::put(b,128+2*28+12,2,4);rejects(b);b=original;test::put(b,128+4*28+12,2,4);rejects(b);
        const auto at=(static_cast<unsigned char>(original[128+28+20])<<24)|(static_cast<unsigned char>(original[128+28+21])<<16)|(static_cast<unsigned char>(original[128+28+22])<<8)|static_cast<unsigned char>(original[128+28+23]);
        b=original;test::put(b,at,5000,2);rejects(b);b=original;test::put(b,at+2,10,2);rejects(b);
    });
    run("Substitutions preserve anchors but do not assign original quality",[]{auto d=example();const auto id=d.rows()[0].id;const auto old=d.rows()[0].trace;
        d.replace(id,1,1,"G");auto p=trace_position(d.rows()[0],1);require(p.originalCall==1&&p.imported=='C'&&p.edited&&!p.quality,"substitution evidence");
        d.undo();require(d.rows()[0].trace==old&&trace_position(d.rows()[0],1).quality==21,"undo trace");d.redo();require(trace_position(d.rows()[0],1).edited,"redo trace");
    });
    run("Insertions have no original call/quality; deletions retain immutable evidence",[]{auto d=example();auto id=d.rows()[0].id;const auto original=d.rows()[0].trace->original;
        d.replace(id,2,0,"tt");require(d.rows()[0].trace->originalCalls==std::vector<std::int32_t>({0,1,-1,-1,2,3,4,5}),"insert mapping");
        auto p=trace_position(d.rows()[0],2);require(!p.originalCall&&!p.quality&&p.sample>50&&p.sample<70,"interpolated only");
        d.replace(id,0,2,"");require(!trace_column(d.rows()[0],0)&&d.rows()[0].trace->original==original&&original->calls=="ACGTRN","deletion retains original");
        d.undo();d.undo();require(d.rows()[0].trace->originalCalls==std::vector<std::int32_t>({0,1,2,3,4,5}),"undo insertion/deletion");
    });
    run("Gap filling, rigid sliding, minimize, flush and case preserve provenance",[]{auto d=example();auto id=d.rows()[0].id;const auto original=d.rows()[0].trace;
        d.replace(id,1,0,"~~~");require(d.rows()[0].trace==original,"gap insertion");
        d.slide({id},4,2,2);require(d.rows()[0].trace==original,"rigid sliding");d.transform({id},Document::Transform::Lowercase);require(d.rows()[0].trace==original,"case");
        d.minimize();d.append({{"long","ACGTACGTACGT"}});d.flush();require(d.rows()[0].trace==original,"minimize/flush");
        d.replace(id,6,1,"A");require(d.rows()[0].trace->originalCalls.back()==-1,"gap to nucleotide creates synthetic anchor");
        validate_trace(d.rows()[0]);
    });
    run("Reverse complement including palindromes reverses mapping and quality",[]{auto d=example("ACGT");auto id=d.rows()[0].id;const auto original=d.rows()[0].trace;
        d.transform({id},Document::Transform::ReverseComplement);require(d.rows()[0].symbols.text()=="ACGT"&&d.rows()[0].trace->reversed,"palindrome orientation still changes");
        require(d.rows()[0].trace->originalCalls==std::vector<std::int32_t>({3,2,1,0}),"reverse mapping");const auto p=trace_position(d.rows()[0],0);require(p.originalCall==3&&p.imported=='A'&&p.quality==23&&!p.edited,"complemented baseline");
        d.undo();require(d.rows()[0].trace==original,"strand undo");d.redo();d.transform({id},Document::Transform::ReverseComplement);require(!d.rows()[0].trace->reversed,"double reverse");
    });
    run("Trace attachment follows row rename/reorder/delete/append history",[]{auto d=example();const auto original=d.rows()[0].trace;d.append({{"other","AT"}});d.rename({{1,"renamed"}});d.move_rows({1},2);require(d.rows()[1].trace==original,"move trace");d.erase_rows({1});d.undo();require(d.sequence(1).trace==original,"row restore trace");d.undo();d.undo();d.undo();require(d.rows().size()==1&&d.rows()[0].trace==original,"all row undo");});
    run("MAFFT whole/columns/reverse and template merge retain original trace",[]{auto source=std::make_shared<Document>(example("ACGT"));source->append({{"template","ACGT"},{"mask","1234"}});const auto trace=source->rows()[0].trace;
        MafftAlignment job(*source,{});auto result=job.merge({{"kleio_1","A-CGT"},{"kleio_2","AC-GT"}},false);auto d=source->derive(std::move(result.records));require(d.rows()[0].trace==trace,"whole provenance");
        auto reverse=job.merge({{"_R_kleio_1","ACGT"},{"kleio_2","ACGT"}},true);d=source->derive(std::move(reverse.records));require(d.rows()[0].trace->reversed&&d.rows()[0].trace->originalCalls[0]==3,"MAFFT orientation");
        MafftAlignment columns(*source,Document::ColumnRange{1,2});auto partial=columns.merge({{"kleio_1","C-G"},{"kleio_2","CG-"}},false);d=source->derive(std::move(partial.records));require(d.rows()[0].trace==trace,"column provenance");
        MafftTemplateAlignment templates(*source,{{1},{2},false});auto merged=templates.merge({{"kleio_1","A-CGT"},{"kleio_2","AC-GT"}});d=source->derive(std::move(merged.alignment.records));require(d.rows()[0].trace==trace,"template provenance");
    });
    run("Invalid trace links reject before document mutation; FASTA export is text only",[]{auto d=example();const auto original=d.rows()[0].trace;auto bad=std::make_shared<TraceLink>(*original);bad->originalCalls[0]=100;
        try{d.append({{"bad","ACGTRN",RowKind::Nucleotide,bad}});throw std::logic_error("Accepted bad anchor");}catch(const std::invalid_argument&){}
        require(d.rows().size()==1,"atomic append");std::ostringstream out;write_fasta(out,d);std::istringstream in(out.str());auto records=read_fasta(in);require(records[0].symbols=="ACGTRN"&&!records[0].trace,"sequence-only FASTA");
    });
    run("Random edits preserve trace invariants and exact undo/redo",[]{auto d=example("ACGTRNACGTRN");std::mt19937 random(808);std::vector<std::string> states;
        auto snapshot=[&]{std::ostringstream out;const auto& row=d.rows()[0];out<<row.symbols.text()<<':'<<row.trace->reversed<<':';for(auto a:row.trace->originalCalls)out<<a<<',';return out.str();};
        states.push_back(snapshot());const auto evidence=d.rows()[0].trace->original;
        for(unsigned i=0;i<500;++i){const auto state=d.state_id();const auto size=d.rows()[0].symbols.size();
            if(random()%9==0)d.transform({1},Document::Transform::ReverseComplement);
            else{auto start=random()%(size+1);auto count=random()%(std::min<std::size_t>(3,size-start)+1);std::string text;const std::string symbols="ACGTN-~";for(unsigned j=0,n=random()%4;j<n;++j)text+=symbols[random()%symbols.size()];d.replace(1,start,count,text);}
            validate_trace(d.rows()[0]);require(d.rows()[0].trace->original==evidence,"immutable original after random edits");
            for(std::size_t c=0;c<d.rows()[0].symbols.size();++c){const auto p=trace_position(d.rows()[0],c);require(!p.quality||(p.originalCall&&!p.edited),"no invented quality");}
            if(d.state_id()!=state)states.push_back(snapshot());
        }
        for(std::size_t i=states.size()-1;i>0;--i){d.undo();require(snapshot()==states[i-1],"random exact undo");}
        for(std::size_t i=1;i<states.size();++i){d.redo();require(snapshot()==states[i],"random exact redo");}
    });
    run("Malformed-byte ABIF fuzzing remains bounded",[]{std::mt19937 random(809);const auto original=test::synthetic_abif();for(unsigned i=0;i<2000;++i){auto bytes=original;
        for(unsigned j=0,n=1+random()%4;j<n;++j)bytes[random()%bytes.size()]=static_cast<char>(random());
        if(i%5==0)bytes.resize(random()%bytes.size());
        try{auto record=parse(bytes);Document d;d.load({std::move(record)});validate_trace(d.rows()[0]);}catch(const std::exception&){}
    }});
    for(int i=1;i<argc;++i)run(argv[i],[&,i]{std::ifstream in(argv[i],std::ios::binary);require(bool(in),"open real trace");const auto record=read_abif(in,"real",argv[i]);Document d;d.load({record});const auto& data=*record.trace->original;
        std::cout<<"  "<<data.calls.size()<<" calls / "<<data.channels[0].size()<<" samples / sample "<<data.sampleName<<'\n';
        require(data.calls.size()==580||data.calls.size()==1622,"supplied sample calls");require(data.channels[0].size()==6998||data.channels[0].size()==17242,"supplied samples");
        d.transform({d.rows()[0].id},Document::Transform::ReverseComplement);validate_trace(d.rows()[0]);d.undo();require(d.rows()[0].trace==record.trace,"real trace undo");
    });
    std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;
}
