#include "core/collapse.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "abif_fixture.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}need(threw,"Expected rejection");}
std::string saved(const Document& d){std::ostringstream out(std::ios::binary);write_session(out,d);return out.str();}
std::vector<SequenceId> first(const CollapsePlan& p){std::vector<SequenceId> result;for(const auto& c:p.clusters)result.push_back(c.members.front());return result;}
}
int main(){int passed=0,failed=0;auto test=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
test("Gap/case insensitive full strings, literal ambiguities, kind separation and mask opt-in",[]{
    Document d;d.load({{"nt1","a-C~GN?"},{"nt2","ACG~N?"},{"substring","CGN?"},{"ambiguous","ACGR?"},{"n-is-not-wildcard","ACGA?"},{"protein1","ACGN?",RowKind::Protein},{"protein2","a~cgn?",RowKind::Protein},{"mask1","ACGN?",RowKind::Mask},{"mask2","ACG~N?",RowKind::Mask},{"rna","ACGU"},{"dna","ACGT"}});
    auto p=plan_identical_collapse(d);need(p.clusters.size()==2&&p.clusters[0].members==std::vector<SequenceId>({1,2})&&p.clusters[1].members==std::vector<SequenceId>({6,7})&&p.masksSkipped==2&&p.removedRows==2,"Strict complete strings and type domains");
    p=plan_identical_collapse(d,false,{},true);need(p.clusters.size()==3&&p.clusters[2].members==std::vector<SequenceId>({8,9}),"Masks compare only with masks on opt-in");
});
test("Concatenation is representative-first, stable, complete UTF-8 and configurable",[]{
    Document d;d.load({{"one Я","AC"},{"two with spaces","aC"},{"one Я","A~C"},{"unique","G"}});const auto p=plan_identical_collapse(d);auto choices=collapse_choices(d,p,{2},CollapseNaming::Concatenate," | ");need(choices[0].name=="two with spaces | one Я | one Я","All titles retained, including repeated titles");need(collapsed_title(d,p.clusters[0],2,CollapseNaming::Concatenate," | ")==choices[0].name,"Single and bulk formatting agree");
    auto before=saved(d);apply_identical_collapse(d,p,choices);need(d.rows().size()==2&&d.rows()[0].id==2&&d.rows()[1].id==4&&d.sequence(2).symbols.text()=="aC"&&d.sequence(2).name==choices[0].name,"Selected representative retains exact symbols and surviving order");d.undo();need(saved(d)==before&&!d.can_undo(),"Names, rows, gaps, case, IDs return in one undo");d.redo();need(d.rows()[0].name==choices[0].name,"Redo restores collapse");
});
test("Count suffix counts all 13 current rows and leaves singletons unchanged",[]{
    Document d;std::vector<Record> rows;for(int i=0;i<13;++i)rows.push_back({i==7?"representative":"sample"+std::to_string(i),i%2?"A-C":"a~C"});rows.push_back({"singleton","G"});d.load(rows);auto p=plan_identical_collapse(d);auto c=collapse_choices(d,p,{8},CollapseNaming::CountSuffix);need(c[0].name=="representative_x13"&&p.removedRows==12,"Count includes representative");need(collapsed_title(d,p.clusters[0],8,CollapseNaming::CountSuffix)==c[0].name,"Count helpers agree");apply_identical_collapse(d,p,c);need(d.sequence(8).name=="representative_x13"&&d.sequence(14).name=="singleton","Singleton name unchanged");auto noDuplicates=plan_identical_collapse(d);need(noDuplicates.clusters.empty(),"Repeated operation creates no _x1 suffix");
});
test("Selected-only scope is explicit, stable and independent of unselected duplicates",[]{
    Document d;d.load({{"a","A"},{"b","a"},{"c","A"},{"other","C"},{"mask","A",RowKind::Mask}});auto p=plan_identical_collapse(d,true,{3,2});need(p.clusters.size()==1&&p.clusters[0].members==std::vector<SequenceId>({2,3})&&p.scopedRows==2,"Input order of selection does not change alignment order");apply_identical_collapse(d,p,collapse_choices(d,p,{3},CollapseNaming::CountSuffix));need(d.rows().size()==4&&d.sequence(1).name=="a"&&d.sequence(3).name=="c_x2"&&d.sequence(5).name=="mask","Unselected rows untouched");need(plan_identical_collapse(d,true,{}).clusters.empty(),"Empty selection means no rows");rejects([&]{plan_identical_collapse(d,true,{1,1});});rejects([&]{plan_identical_collapse(d,true,{999});});
});
test("Empty/gap-only rows match by type and all-N rows use literal identity",[]{
    Document d;d.load({{"empty",""},{"gaps","-~~"},{"protein empty","",RowKind::Protein},{"protein gaps","~~",RowKind::Protein},{"n1","NN"},{"n2","n~n"},{"single N","N"}});auto p=plan_identical_collapse(d);need(p.clusters.size()==3&&p.clusters[0].members==std::vector<SequenceId>({1,2})&&p.clusters[1].members==std::vector<SequenceId>({3,4})&&p.clusters[2].members==std::vector<SequenceId>({5,6}),"Empty biological strings and literal all-N strings");
});
test("Chosen group, complete trace link and protein provenance survive collapse/history/session",[]{
    std::istringstream a(test::synthetic_abif()),b(test::synthetic_abif());auto one=read_abif(a,"trace 1","one.ab1"),two=read_abif(b,"trace 2","two.ab1");two.symbols="~"+two.symbols;auto provenance=std::make_shared<ProteinLayout>();provenance->sourceName="translated source";provenance->sourceId=1;
    Document d;d.load({one,two,{"protein1","M~EF*",RowKind::Protein},{"protein2","MEF*",RowKind::Protein,{},RowRepresentation::AlignedSymbols,0,provenance},{"mask","123",RowKind::Mask}});d.apply_groups({{1,"A",0x0072b2},{2,"B",0xe69f00}},{{1,1},{2,2},{3,1},{4,2}});d.set_translation_settings({1,2,{},{}});const auto before=saved(d);const auto history=d.history_bytes();const auto trace=d.sequence(2).trace;const auto protein=d.sequence(4).protein;auto p=plan_identical_collapse(d);apply_identical_collapse(d,p,collapse_choices(d,p,{2,4},CollapseNaming::CountSuffix));need(d.rows().size()==3&&d.sequence(2).trace==trace&&d.sequence(2).groupId==2&&d.sequence(4).protein==protein&&d.sequence(4).groupId==2&&d.groups().size()==2&&d.translation_settings().frame==2,"Chosen evidence/group/settings retained");const auto after=saved(d);d.undo();need(saved(d)==before&&d.history_bytes()>=history,"One undo restores every discarded trace and group assignment");d.redo();need(saved(d)==after&&d.sequence(2).trace==trace,"Redo retains exact immutable evidence");std::istringstream session(after,std::ios::binary);auto restored=read_session(session);need(saved(restored.document)==after&&restored.document.sequence(2).trace->original->sourcePath=="two.ab1"&&restored.document.sequence(4).protein->sourceName=="translated source","Native session retains chosen metadata");
});
test("Invalid and stale choices reject atomically with history and IDs intact",[]{
    Document d;d.load({{"a","A"},{"b","A"},{"not equal","G"},{"protein","A",RowKind::Protein}});auto p=plan_identical_collapse(d);auto choices=collapse_choices(d,p,{1},CollapseNaming::Concatenate);const auto before=saved(d);const auto state=d.state_id(),history=d.history_bytes();
    auto invalid=[&](auto choice){rejects([&]{d.collapse_identical(choice);});need(saved(d)==before&&d.state_id()==state&&d.history_bytes()==history&&!d.can_undo(),"Invalid collapse changes nothing");};
    invalid(std::vector<Document::CollapseGroup>{{1,{1,3},"bad"}});invalid(std::vector<Document::CollapseGroup>{{1,{1,4},"bad"}});invalid(std::vector<Document::CollapseGroup>{{3,{1,2},"bad"}});invalid(std::vector<Document::CollapseGroup>{{1,{1,2,2},"bad"}});invalid(std::vector<Document::CollapseGroup>{{1,{1,2},"bad"},{2,{1,2},"bad"}});invalid(std::vector<Document::CollapseGroup>{{1,{1,2},"\n"}});invalid(std::vector<Document::CollapseGroup>{{1,{1,999},"bad"}});
    rejects([&]{collapse_choices(d,p,{},CollapseNaming::CountSuffix);});rejects([&]{collapse_choices(d,p,{3},CollapseNaming::CountSuffix);});rejects([&]{collapse_choices(d,p,{1},CollapseNaming::Concatenate,"\n");});rejects([&]{apply_identical_collapse(d,p,{});});auto forged=choices;forged[0].members={1,3};rejects([&]{apply_identical_collapse(d,p,forged);});
    d.rename({{3,"changed"}});rejects([&]{apply_identical_collapse(d,p,choices);});rejects([&]{collapse_choices(d,p,{1},CollapseNaming::CountSuffix);});
});
test("Cancelled matching/naming and no-op plans preserve source and history",[]{
    Document d;d.load({{"a",std::string(50000,'A')},{"b",std::string(50000,'a')}});const auto before=saved(d);int checks=0;rejects([&]{plan_identical_collapse(d,false,{},false,[&]{return ++checks>3;});});need(saved(d)==before&&!d.can_undo(),"Cancelled scan is read-only");auto p=plan_identical_collapse(d);rejects([&]{collapse_choices(d,p,{1},CollapseNaming::CountSuffix,";",[]{return true;});});d.collapse_identical({});need(!d.can_undo()&&saved(d)==before,"No-op has no history");
});
test("Randomized matching and transaction agree with an independent ungapped oracle",[]{
    std::mt19937 rng(271828);for(int trial=0;trial<200;++trial){std::vector<Record> records;std::map<std::string,std::vector<SequenceId>> expected;for(int i=0;i<35;++i){auto kind=rng()%3==0?RowKind::Protein:RowKind::Nucleotide;const std::string base=std::vector<std::string>{"ACGN","ACGT","AC","NN","T",""}[rng()%6];std::string s;for(char c:base){s.append(rng()%3,rng()%2?'~':'-');s+=rng()%2?c:static_cast<char>(c-'A'+'a');}s.append(rng()%4,'~');records.push_back({"name"+std::to_string(i),s,kind});expected[std::to_string(static_cast<int>(kind))+base].push_back(static_cast<SequenceId>(i+1));}
        Document d;d.load(records);auto before=saved(d);auto p=plan_identical_collapse(d);std::vector<std::vector<SequenceId>> oracle;for(auto& [key,ids]:expected)if(ids.size()>1)oracle.push_back(ids);std::sort(oracle.begin(),oracle.end(),[](const auto& x,const auto& y){return x.front()<y.front();});need(p.clusters.size()==oracle.size(),"Cluster count oracle");std::vector<SequenceId> reps;for(std::size_t i=0;i<oracle.size();++i){need(p.clusters[i].members==oracle[i],"Exact cluster oracle");reps.push_back(oracle[i][rng()%oracle[i].size()]);}auto c=collapse_choices(d,p,reps,CollapseNaming::CountSuffix);apply_identical_collapse(d,p,c);for(const auto& choice:c)need(d.sequence(choice.representative).symbols.text()==records[choice.representative-1].symbols&&d.sequence(choice.representative).name==choice.name,"Representative exact bytes");for(std::size_t i=1;i<d.rows().size();++i)need(d.rows()[i-1].id<d.rows()[i].id,"Survivor order stable");const auto after=saved(d);d.undo();need(saved(d)==before&&!d.can_undo(),"Atomic random undo");d.redo();need(saved(d)==after,"Atomic random redo");}
});
std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;}
