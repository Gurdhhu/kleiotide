#include "core/mafft_template.hpp"
#include "core/fasta.hpp"
#include "core/translation.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}need(threw,"Expected rejection");}
std::string ungap(std::string_view text){std::string s;for(char c:text)if(!is_gap(c))s+=c;return s;}
std::string fasta(const Document& d){std::ostringstream out;write_fasta(out,d);return out.str();}
}
int main(){int passed=0,failed=0;auto test=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
test("Explicit protein flags and scoring models never emit nucleotide options",[]{
    MafftOptions o;o.molecule=RowKind::Protein;
    for(int method=0;method<8;++method)for(int model=0;model<3;++model){o.algorithm=static_cast<MafftAlgorithm>(method);o.proteinScoring=static_cast<ProteinScoring>(model);const auto args=o.arguments();auto has=[&](const char* s){return std::find(args.begin(),args.end(),s)!=args.end();};need(has("--amino")&&has("--anysymbol")&&!has("--nuc")&&!has("--kimura")&&!has("--kappa")&&!has("--adjustdirection"),"Molecule-specific flags");need(has(model==0?"--bl":model==1?"--jtt":"--tm"),"Scoring flag");}
    o.proteinScoring=ProteinScoring::Blosum;for(int b:{30,45,62,80}){o.blosum=b;(void)o.arguments();}o.blosum=63;rejects([&]{o.arguments();});o.blosum=62;o.direction=MafftDirection::Adjust;rejects([&]{o.arguments();});o.direction=MafftDirection::Keep;o.proteinScoring=ProteinScoring::Jtt;o.proteinPam=0;rejects([&]{o.arguments();});o.proteinPam=1001;rejects([&]{o.arguments();});o.molecule=RowKind::Mask;rejects([&]{o.arguments();});
});
test("Whole protein alignment preserves overlapping alphabets, all symbols, case, IDs and groups",[]{
    Document d;const std::string symbols="MeFYWVILACGTNDBZJXUO*?";d.load({{"same",symbols,RowKind::Protein},{"same","ACGT",RowKind::Protein},{"mask","12#45",RowKind::Mask},{"empty","",RowKind::Protein}});d.apply_groups({{1,"Family",0x0072b2}},{{1,1},{2,1}});const auto before=fasta(d);MafftAlignment job(d);need(job.molecule()==RowKind::Protein&&job.input_count()==2&&job.mask_count()==1,"Protein/mask/empty scope");std::ostringstream input;job.write_input(input);need(input.str().find(symbols)!=std::string::npos,"All legal protein symbols retained in input");
    auto second=std::string("A~C~G~T");second.resize(symbols.size(),'~');auto result=d.derive(job.merge({{"kleio_2",second},{"kleio_1",symbols}}).records);need(result.sequence(1).kind==RowKind::Protein&&result.sequence(1).groupId==1&&result.sequence(1).name=="same"&&result.sequence(1).symbols.text()==symbols,"Protein identity/case/groups");need(result.sequence(2).symbols.text()==second&&result.sequence(3).symbols.text()=="12#45"&&result.sequence(4).symbols.size()==symbols.size(),"Masks exact and empty proteins padded");need(fasta(d)==before,"Source unchanged");
});
test("Protein column realignment retains outside context, masks and locked gap states",[]{
    Document d;d.load({{"one","m-ME~~F*w~",RowKind::Protein},{"two","W~M~~EF*Y-",RowKind::Protein},{"mask","0123456789",RowKind::Mask}});MafftAlignment job(d,Document::ColumnRange{2,6});auto merged=job.merge({{"kleio_1","M~EF*"},{"kleio_2","ME~F*"}});need(merged.fragmentWidth==6,"Mask fragment prevents shortening");need(merged.records[0].symbols=="m-M~EF*~w~"&&merged.records[1].symbols=="W~ME~F*~Y-"&&merged.records[2].symbols=="0123456789","Outside region stays byte exact");
});
test("Corrupt or mismatched protein output is rejected losslessly",[]{
    Document d;d.load({{"one","MUO*?",RowKind::Protein},{"two","MTX*?",RowKind::Protein}});MafftAlignment j(d);auto good=std::vector<Record>{{"kleio_1","MUO*?"},{"kleio_2","MTX*?"}};auto bad=good;bad[0].symbols="MTO*?";rejects([&]{j.merge(bad);});bad=good;bad[0].symbols="M1O*?";rejects([&]{j.merge(bad);});bad=good;bad[0].name="_R_kleio_1";rejects([&]{j.merge(bad);});rejects([&]{j.merge(good,true);});bad=good;bad[1].name="kleio_1";rejects([&]{j.merge(bad);});bad=good;bad[0].symbols="MUO*";rejects([&]{j.merge(bad);});bad=good;bad[0].kind=RowKind::Mask;rejects([&]{j.merge(bad);});rejects([&]{j.write_input(std::cout,[]{return true;});});d.rename({{1,"changed"}});rejects([&]{j.merge(good);});
});
test("Mixed participants reject; selected-only subsets remain allowed",[]{
    Document d;d.load({{"dna","ACGT"},{"aa1","ACGT",RowKind::Protein},{"aa2","MEF",RowKind::Protein},{"mask","123",RowKind::Mask}});rejects([&]{MafftAlignment j(d);});rejects([&]{MafftTemplateAlignment j(d,{{2},{1},false});});rejects([&]{MafftTemplateAlignment j(d,{{2},{4},false});});auto subset=d.subset({2,3});MafftAlignment j(subset);need(j.molecule()==RowKind::Protein&&subset.rows()[0].id==2&&subset.rows()[1].id==3,"Homogeneous subset keeps stable IDs");
});
test("Protein template projection preserves all nonparticipants, masks, group and protein provenance",[]{
    auto provenance=std::make_shared<ProteinLayout>();provenance->sourceId=42;provenance->sourceName="source";Document d;d.load({{"template","M-~EF",RowKind::Protein},{"target","MkEF",RowKind::Protein,{},RowRepresentation::AlignedSymbols,0,provenance},{"dna","AcG-T"},{"mask","12345",RowKind::Mask},{"other protein","w~X?-",RowKind::Protein}});d.apply_groups({{1,"A",0x0072b2}},{{1,1},{2,1}});const auto before=fasta(d);
    for(bool fragments:{false,true}){MafftTemplateAlignment j(d,{{2},{1},fragments});need(j.molecule()==RowKind::Protein&&j.restored_columns()==2,"Protein anchors");auto r=j.merge({{"kleio_2","MkEF"},{"kleio_1","M~EF"}});need(r.insertedColumns==1&&r.originalColumns==std::vector<std::size_t>({0,1,2,4,5}),"Exact original-column projection");auto result=d.derive(r.alignment.records);need(result.sequence(1).symbols.text()=="M-~~EF"&&result.sequence(2).symbols.text()=="M~~kEF","Templates stay fixed, target gains insertion");for(auto id:{3,4,5}){const auto& old=d.sequence(id).symbols.text();const auto& now=result.sequence(id).symbols.text();for(std::size_t c=0;c<old.size();++c)need(now[r.originalColumns[c]]==old[c],"Non-target byte preserved");}need(result.sequence(2).protein==provenance&&result.sequence(2).groupId==1&&result.sequence(3).kind==RowKind::Nucleotide,"Protein provenance, groups, mixed nonparticipant type");rejects([&]{j.merge({{"_R_kleio_2","MkEF"},{"kleio_1","M~EF"}},{},true);});}
    need(fasta(d)==before,"Source remains exact");
});
test("Whole translated protein rows retain provenance without structural padding",[]{
    Document d;d.load({{"dna","ATGAAATAG"}});TranslationScan scan(d,{});while(!scan.step(d)){}auto translated=translated_records(d,scan.result(),false);auto ids=d.insert_translations(translated.records,translated.sources);d.erase_rows({1});d.append({{"aa","MK*",RowKind::Protein}});MafftAlignment job(d);const auto info=d.sequence(ids[0]).protein;auto result=d.derive(job.merge({{"kleio_2","MK*"},{"kleio_3","MK*"}}).records);need(result.sequence(2).symbols.text()=="MK*"&&result.sequence(2).protein==info&&info&&info->spans.empty(),"Ordinary gaps removed; translated provenance retained");
});
std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;}
