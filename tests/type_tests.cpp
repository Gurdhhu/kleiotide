#include "core/alignment.hpp"
#include "core/fasta.hpp"
#include "core/mafft_template.hpp"
#include "core/overview.hpp"
#include "core/row_selection.hpp"
#include "core/statistics.hpp"
#include "core/abif.hpp"
#include "abif_fixture.hpp"
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}require(threw,"Expected rejection");}
std::vector<Record> parse(const std::string& text,FastaImport type=FastaImport::NucleotideAndMasks){std::istringstream in(text);return read_fasta(in,{},type);}
std::string ungapped(std::string_view s){std::string r;for(char c:s)if(!is_gap(c))r+=c;return r;}
}
int main(){int passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Explicit alphabets preserve legacy inference and reject invalid declarations atomically",[]{
    Document d;d.load({{"legacy","MEF"},{"protein","acgtBZJXUO*?-.EF",RowKind::Protein},{"forced","ACGT",RowKind::Mask}});
    require(d.sequence(1).kind==RowKind::Mask&&d.sequence(2).kind==RowKind::Protein&&d.sequence(3).kind==RowKind::Mask,"No automatic protein guessing or mask reclassification");
    require(d.sequence(2).symbols.text()=="acgtBZJXUO*?-~EF","Protein case/unknowns/stops/gap normalization");
    const auto state=d.state_id(),next=d.next_sequence_id();rejects([&]{d.append({{"valid","A"},{"bad","MEF",RowKind::Nucleotide}});});
    require(d.rows().size()==3&&d.state_id()==state&&d.next_sequence_id()==next&&!d.can_undo(),"Failed batch consumes no IDs or history");
    rejects([&]{d.append({{"bad protein","M1",RowKind::Protein}});});
    rejects([&]{d.append({{"bad representation","A",RowKind::Protein,{},static_cast<RowRepresentation>(1)}});});
});
run("FASTA choices distinguish overlapping alphabets, retain masks and accept arbitrary wrapping",[]{
    const std::string text=">overlap\nAC\nGT\n>protein\nMeF*\n>mask\n12AC\n>empty\n>gaps\n.-~\n";
    auto legacy=parse(text),protein=parse(text,FastaImport::ProteinAndMasks),masks=parse(text,FastaImport::Masks);
    require(legacy[0].kind==RowKind::Nucleotide&&legacy[1].kind==RowKind::Mask,"Default retains nucleotide/mask interpretation");
    require(protein[0].kind==RowKind::Protein&&protein[1].kind==RowKind::Protein&&protein[2].kind==RowKind::Mask&&protein[3].kind==RowKind::Protein&&protein[4].symbols=="~-~","Explicit protein scope includes overlapping/empty/gap-only rows");
    for(const auto& row:masks)require(row.kind==RowKind::Mask,"Forced masks remain masks");
    Document d;d.load(protein);std::ostringstream out;write_fasta(out,d);require(out.str().find(">overlap\nACGT\n")!=std::string::npos,"Canonical unwrapped FASTA output");
    auto again=parse(out.str(),FastaImport::ProteinAndMasks);for(std::size_t n=0;n<protein.size();++n)require(again[n].kind==protein[n].kind&&again[n].symbols==protein[n].symbols,"Typed import/export round trip");
});
run("Type assignment changes semantics only and has one exact undo/redo step",[]{
    Document d;d.load({{"one","ACGT"},{"two","ACGT",RowKind::Mask}});const auto content=d.content_id();
    d.set_kind({1,2},RowKind::Protein);require(d.content_id()!=content&&d.sequence(1).kind==RowKind::Protein&&d.sequence(2).kind==RowKind::Protein,"Type edits invalidate biological calculations");
    require(d.sequence(1).symbols.text()=="ACGT"&&d.sequence(2).symbols.text()=="ACGT","Type assignment does not translate or alter symbols");
    d.undo();require(d.sequence(1).kind==RowKind::Nucleotide&&d.sequence(2).kind==RowKind::Mask&&!d.can_undo(),"Original types return in one undo");d.redo();
    const auto state=d.state_id();d.set_kind({1,2},RowKind::Protein);require(d.state_id()==state,"No-op type assignment has no history step");
    d.rename({{1,"renamed"}});d.reorder({2,1});d.undo();d.undo();d.undo();require(d.sequence(1).name=="one"&&d.sequence(1).kind==RowKind::Nucleotide,"Names/order/history retain stable row typing");
});
run("Incompatible type batches and trace retyping leave all rows/evidence untouched",[]{
    std::istringstream input(test::synthetic_abif());Document d;d.load({{"valid","ACGT"},{"protein","MEF",RowKind::Protein},read_abif(input,"trace","missing.ab1")});
    const auto state=d.state_id();const auto original=d.sequence(3).trace;
    rejects([&]{d.set_kind({1,2},RowKind::Nucleotide);});rejects([&]{d.set_kind({1,3},RowKind::Mask);});rejects([&]{d.set_kind({1,1},RowKind::Protein);});rejects([&]{d.set_kind({1},RowKind::Automatic);});
    require(d.state_id()==state&&!d.can_undo()&&d.sequence(1).kind==RowKind::Nucleotide&&d.sequence(3).trace==original,"Failed batch changes neither types nor history/evidence");
});
run("Protein edits and mixed-row block operations retain strict types and atomic history",[]{
    Document d;d.load({{"dna","AC~~GT"},{"protein","ME~~F*",RowKind::Protein},{"mask","12~~34",RowKind::Mask}});
    d.slide({1,2,3},0,2,1);require(d.sequence(1).symbols.text()=="~AC~GT"&&d.sequence(2).symbols.text()=="~ME~F*"&&d.sequence(3).symbols.text()=="~12~34","Shared block sliding honors each row's content");d.undo();
    d.replace(2,1,1,"wX");require(d.sequence(2).symbols.text()=="MwX~~F*","Protein insertion/substitution preserves case");d.undo();
    rejects([&]{d.replace_many({{1,0,1,"T"},{2,0,1,"1"}});});require(d.sequence(1).symbols.text()=="AC~~GT"&&!d.can_undo(),"Invalid protein edit cannot partly rewrite mixed block");
    d.transform({1,2,3},Document::Transform::Lowercase);require(d.sequence(2).symbols.text()=="me~~f*"&&d.sequence(2).kind==RowKind::Protein,"General transforms preserve protein identity");d.undo();
});
run("Nucleotide calculations, motif searches and reverse complements exclude proteins",[]{
    Document base;base.load({{"a","ACGT"},{"b","ACGT"}});Document mixed;mixed.load({{"a","ACGT"},{"b","ACGT"},{"protein","TTTT",RowKind::Protein},{"mask","AAAA",RowKind::Mask}});
    OverviewScan a(base),b(mixed);while(!a.complete())a.step(base,3);while(!b.complete())b.step(mixed,3);const auto x=a.bins(4),y=b.bins(4);
    for(std::size_t n=0;n<x.size();++n)require(x[n].residues==y[n].residues&&x[n].conservation==y[n].conservation,"Protein symbols never vote in nucleotide overview");
    require(alignment_statistics(mixed).sequences==2,"Statistics include only nucleotide rows");require(select_motif(mixed,"TTTT").empty(),"Nucleotide motif does not match a protein");
    mixed.transform({1,3,4},Document::Transform::ReverseComplement);require(mixed.sequence(3).symbols.text()=="TTTT"&&mixed.sequence(4).symbols.text()=="AAAA","Protein/mask reverse complement skipped");
    Document duplicates;duplicates.load({{"dna","ACGT"},{"protein","ACGT",RowKind::Protein},{"mask","ACGT",RowKind::Mask}});require(select_duplicate_sequences(duplicates,false,false).empty(),"Different molecule types are not sequence duplicates");
});
run("Nucleotide MAFFT rejects protein inputs and preserves non-target protein metadata",[]{
    Document d;d.load({{"template","A~C"},{"target","ATC"},{"protein","MeF",RowKind::Protein}});
    rejects([&]{MafftAlignment job(d);});rejects([&]{MafftTemplateAlignment job(d,{{3},{1},false});});
    MafftTemplateAlignment job(d,{{2},{1},false});auto result=job.merge({{"kleio_1","A-C"},{"kleio_2","ATC"}});auto derived=d.derive(result.alignment.records);
    require(derived.sequence(3).kind==RowKind::Protein&&ungapped(derived.sequence(3).symbols.text())=="MeF"&&derived.sequence(3).representation==RowRepresentation::AlignedSymbols,"Non-target protein retained on projected original axis");
    result.alignment.records[2].kind=RowKind::Nucleotide;rejects([&]{d.derive(result.alignment.records);});
});
std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;}
