#include "core/identity_groups.hpp"
#include "core/session.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}need(threw,"Expected rejection");}
IdentityPair pair(std::string a,std::string b,IdentityAlignment mode=IdentityAlignment::Existing,RowKind kind=RowKind::Nucleotide){Document d;d.load({{"a",std::move(a),kind},{"b",std::move(b),kind}});return sequence_identity(d.sequence(1),d.sequence(2),mode);}
// Normalize only the intentionally monotonic retired-group ID counter for Undo comparisons.
std::string session(const Document& d){std::vector<Record> records;std::vector<SequenceId> ids;for(const auto& r:d.rows()){ids.push_back(r.id);records.push_back({r.name,r.symbols.text(),r.kind,r.trace,r.representation,r.groupId,r.protein});}GroupId next=1;for(const auto& g:d.groups())next=std::max(next,g.id+1);Document normalized;normalized.restore_session(std::move(records),ids,d.next_sequence_id(),d.groups(),next,d.translation_settings());std::ostringstream out(std::ios::binary);write_session(out,normalized);return out.str();}
// Independent exhaustive enumeration of every short overlap alignment (including
// indel paths). Used as an oracle rather than another rolling DP implementation.
struct Brute {int score=std::numeric_limits<int>::min();IdentityPair pair;};
bool better(const Brute& a,const Brute& b){if(a.score!=b.score)return a.score>b.score;if(a.pair.matches!=b.pair.matches)return a.pair.matches>b.pair.matches;if(a.pair.columns!=b.pair.columns)return a.pair.columns<b.pair.columns;return a.pair.paired>b.pair.paired;}
Brute brute(std::string_view a,std::string_view b){
    Brute best;
    std::function<void(std::size_t,std::size_t,int,int,IdentityPair)> walk=[&](std::size_t i,std::size_t j,int gap,int score,IdentityPair p){
        if((i==a.size()||j==b.size())&&p.valid()){Brute candidate{score,p};if(better(candidate,best))best=candidate;}
        if(i<a.size()&&j<b.size()){auto n=p;++n.columns;++n.paired;++n.coveredFirst;++n.coveredSecond;n.matches+=a[i]==b[j];walk(i+1,j+1,0,score+(a[i]==b[j]?2:-4),n);}
        if(i<a.size()&&j>0){auto n=p;++n.columns;++n.coveredFirst;walk(i+1,j,1,score-(gap==1?2:20),n);}
        if(j<b.size()&&i>0){auto n=p;++n.columns;++n.coveredSecond;walk(i,j+1,2,score-(gap==2?2:20),n);}
    };
    for(std::size_t i=0;i<a.size();++i)walk(i,0,0,0,{});
    for(std::size_t j=1;j<b.size();++j)walk(0,j,0,0,{});
    return best;
}
}
int main(){int passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& test){try{test();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Existing alignment ignores composite terminal missing regions and ragged ends",[]{
    auto p=pair("NN-~acgTACGTNN---","~~??ACGTACGTN");need(p.valid()&&p.matches==8&&p.columns==8&&p.identity()==100,"Terminal regions do not penalize identity");
    need(!pair("ACGT~~~~","~~~~ACGT").valid(),"No shared aligned coordinates means no comparison");
    need(!pair("N-?N","NNNN").valid(),"All missing rows provide no identity");
});
run("Internal gaps, double gaps, unknowns, literal ambiguities, case and U=T",[]{
    auto p=pair("A-C~GNT","ATC-G?U");need(p.matches==4&&p.columns==5&&p.paired==4,"Single gap is a difference, double gap and N/? columns omitted");
    p=pair("ARNT","AANT");need(p.matches==2&&p.columns==3,"R is literal, N is omitted");
    p=pair("XXN?M-EFXX","??N~MGEF?",IdentityAlignment::Existing,RowKind::Protein);need(p.matches==4&&p.columns==5,"Protein N is real, X/? are missing");
});
run("Pairwise removes old gaps and handles free ends, containment and reverse input order",[]{
    auto p=pair("--TTACGTAA--","NAC-GT??",IdentityAlignment::Pairwise);need(p.identity()==100&&p.matches==4&&p.coveredFirst==4&&p.coveredSecond==4,"Exact internal fragment");
    p=pair("ACGT","TTACGTAA",IdentityAlignment::Pairwise);need(p.matches==4&&p.columns==4,"Swapping shorter row works");
    p=pair("ACGT~~~~","~~~~ACGT",IdentityAlignment::Pairwise);need(p.matches==4&&p.columns==4,"Pairwise mode realigns");
    need(!pair("NN?~~","ACGT",IdentityAlignment::Pairwise).valid(),"No-information row is invalid");
});
run("Affine pairwise counts a supported internal insertion as one difference",[]{
    const std::string left="ACGTTGCAAGCTTAGCGATCGTACGATCGTAC",right="GCTAGCATCGATGCTAGCATCGTACGTAGCTA";
    auto p=pair(left+right,left+"A"+right,IdentityAlignment::Pairwise);need(p.matches==left.size()+right.size()&&p.columns==p.matches+1&&p.paired==p.matches,"Internal indel retained and counted");
    p=pair(left+right,left+"AAA"+right,IdentityAlignment::Pairwise);need(p.matches==left.size()+right.size()&&p.columns==p.matches+3,"Affine gap extension counts each inserted residue");
});
run("Protein codon padding is not treated as biological sequence length",[]{
    auto layout=std::make_shared<ProteinLayout>();layout->spans={{{0,1,2},3},{{3,4,5},3},{{6,7,8},3}};
    Document d;d.load({{"compact","MNT",RowKind::Protein},{"spanned","M~~N~~T~~",RowKind::Protein,{},RowRepresentation::CodonSpans,0,layout}});IdentityGroupOptions o;o.minimumIdentity=100;need(plan_identity_groups(d,o).clusters==2,"Existing alignment separates different protein layouts");o.alignment=IdentityAlignment::Pairwise;const auto plan=plan_identity_groups(d,o);need(plan.clusters==1&&plan.members[1].centroid==1&&plan.members[1].pair.matches==3,"Pairwise compares actual residues and protein N is informative");
});
run("Linear-memory affine results match exhaustive overlap alignments",[]{
    std::mt19937 rng(26);for(int k=0;k<120;++k){std::string a(1+rng()%4,'A'),b(1+rng()%4,'A');for(auto& c:a)c="ACGT"[rng()%4];for(auto& c:b)c="ACGT"[rng()%4];const auto expected=brute(a,b).pair;const auto actual=pair(a,b,IdentityAlignment::Pairwise);need(actual.matches==expected.matches&&actual.columns==expected.columns&&actual.paired==expected.paired,"Exhaustive affine overlap oracle");}
});
run("Greedy clustering is stable longest-first with fixed representatives",[]{
    Document d;d.load({{"short first","ACG"},{"long centroid","ACGT"},{"tie later","ACGA"},{"no overlap","~~~~ACGT"},{"mask","ACGT",RowKind::Mask},{"unknown","N-?N"},{"protein","ACGT",RowKind::Protein}});
    IdentityGroupOptions o;o.minimumIdentity=75;auto p=plan_identity_groups(d,o);need(p.clusters==3&&p.assigned==5&&p.members[0].centroid==2&&p.members[2].centroid==2,"Longest row becomes representative despite display position");need(p.members[4].group==0&&p.members[5].group==0&&p.members[6].centroid==7,"Mask/no-info skipped and types separate");need(d.groups().empty()&&!d.can_undo(),"Read-only plan");
    o.alignment=IdentityAlignment::Pairwise;p=plan_identity_groups(d,o);need(p.clusters==2&&p.members[3].centroid==2,"Pairwise mode resolves displaced sequences");
    Document chain;chain.load({{"a","AAAA"},{"b","AAAC"},{"c","AACC"}});o.alignment=IdentityAlignment::Existing;o.minimumIdentity=75;p=plan_identity_groups(chain,o);need(p.clusters==2&&p.members[1].centroid==1&&p.members[2].centroid==3,"No transitive single-linkage chaining");
});
run("Threshold boundaries and minimum shorter-sequence overlap are enforced",[]{
    Document d;d.load({{"one","ACGT"},{"two","ACGA"}});IdentityGroupOptions o;o.minimumIdentity=75;need(plan_identity_groups(d,o).clusters==1,"Inclusive identity threshold");o.minimumIdentity=75.01;need(plan_identity_groups(d,o).clusters==2,"Above boundary splits");
    d.load({{"left","ACGT~~~~~~"},{"right","~~~TACGTAC"}});o.minimumIdentity=100;o.minimumOverlap=0;need(plan_identity_groups(d,o).clusters==1,"Short aligned overlap allowed at zero minimum");o.minimumOverlap=26;need(plan_identity_groups(d,o).clusters==2,"Short overlap excluded");
    d.load({{"a","ANCT"},{"b","ANCT"}});o.minimumOverlap=100;need(plan_identity_groups(d,o).clusters==1,"Internal unknowns do not reduce overlap extent");
});
run("Prefixes avoid collisions; selected scope and skipped memberships survive one Undo",[]{
    Document d;d.load({{"a","ACGT"},{"b","ACGT"},{"other","GGGG"},{"mask","1234",RowKind::Mask}});d.apply_groups({{1,"Set_1",0x123456}},{{1,1},{3,1},{4,1}});const auto before=session(d);const auto content=d.content_id();IdentityGroupOptions o;o.prefix="Set_";o.selectedOnly=true;o.selected={1,2,4};auto p=plan_identity_groups(d,o);need(p.clusters==1&&p.groups[1].name=="Set_2"&&p.members.size()==3,"Unique numbered prefix and scoped preview");apply_identity_groups(d,p);need(d.sequence(1).groupId==d.sequence(2).groupId&&d.sequence(3).groupId==1&&d.sequence(4).groupId==1,"Only eligible scoped memberships change");need(d.content_id()==content&&d.rows()[0].name=="a"&&d.sequence(1).symbols.text()=="ACGT","Order and biological content preserved");const auto after=session(d);d.undo();need(session(d)==before,"Atomic undo");d.redo();need(session(d)==after,"Atomic redo");
});
run("Stale plans, foreign documents, invalid settings and empty selection reject",[]{
    Document d;d.load({{"a","ACGT"},{"b","ACGT"}});IdentityGroupOptions o;auto p=plan_identity_groups(d,o);Document other;other.load({{"a","ACGT"},{"b","ACGT"}});rejects([&]{apply_identity_groups(other,p);});d.rename({{1,"changed"}});rejects([&]{apply_identity_groups(d,p);});
    for(const auto value:{-1.,101.,std::numeric_limits<double>::quiet_NaN()}){o.minimumIdentity=value;rejects([&]{plan_identity_groups(d,o);});}o.minimumIdentity=97;o.prefix="bad\n";rejects([&]{plan_identity_groups(d,o);});o.prefix="";rejects([&]{plan_identity_groups(d,o);});o.prefix="G";o.selectedOnly=true;rejects([&]{plan_identity_groups(d,o);});o.selected={999};rejects([&]{plan_identity_groups(d,o);});
});
run("Cancellation interrupts preparation, DP and grouping without mutation",[]{
    Document d;d.load({{"a",std::string(5000,'A')},{"b",std::string(5000,'C')}});const auto before=session(d);int calls=0;IdentityGroupOptions o;o.alignment=IdentityAlignment::Pairwise;rejects([&]{plan_identity_groups(d,o,[&]{return ++calls>20;});});need(session(d)==before&&!d.can_undo(),"Cancelled plan is read-only");calls=0;rejects([&]{sequence_identity(d.sequence(1),d.sequence(2),IdentityAlignment::Pairwise,[&]{return ++calls>25;});});need(calls>25,"DP cancellation polls");
});
run("Representatives use current order, ungapped length, ties and optional Ungrouped",[]{
    Document d;d.load({{"first","A-----"},{"longest","NNNN"},{"tie","ACGT"},{"second group","CC"},{"unassigned","GGG"}});d.apply_groups({{1,"one",0},{2,"two",0},{3,"empty",0}},{{1,1},{2,1},{3,1},{4,2}});const auto before=session(d);need(select_group_representatives(d,GroupRepresentative::First)==std::vector<SequenceId>({1,4}),"First per named group");need(select_group_representatives(d,GroupRepresentative::Longest,true)==std::vector<SequenceId>({2,4,5}),"Ungapped unknowns count, ties first, empty group skipped");d.reorder({3,2,1,4,5});need(select_group_representatives(d,GroupRepresentative::First)==std::vector<SequenceId>({3,4}),"First follows reordered rows");need(select_group_representatives(d,GroupRepresentative::Longest)==std::vector<SequenceId>({3,4}),"Longest tie follows reordered rows");d.undo();need(session(d)==before,"Representative selection does not edit data");
});
run("Ungrouped selection can include none, one by the chosen rule, or every row",[]{
    Document d;d.load({{"u first","A"},{"g first","A"},{"u longest","ACGT"},{"g longest","ACG"},{"u last","GG"}});d.apply_groups({{1,"Group",0}},{{2,1},{4,1}});const auto before=session(d);
    for(auto method:{GroupRepresentative::First,GroupRepresentative::Longest,GroupRepresentative::Random}){const auto all=select_group_representatives(d,method,UngroupedSelection::All,17);need(all.size()==4&&std::find(all.begin(),all.end(),1)!=all.end()&&std::find(all.begin(),all.end(),3)!=all.end()&&std::find(all.begin(),all.end(),5)!=all.end(),"All ungrouped survive every rule");need(std::is_sorted(all.begin(),all.end()),"IDs remain in alignment order");const auto one=select_group_representatives(d,method,UngroupedSelection::One,17);need(one.size()==2,"One Ungrouped plus one named-group member");need(select_group_representatives(d,method,UngroupedSelection::None,17).size()==1,"No Ungrouped when disabled");}
    need(select_group_representatives(d,GroupRepresentative::Longest,UngroupedSelection::One)==std::vector<SequenceId>({3,4}),"Longest rule applies to single Ungrouped representative");need(session(d)==before,"All selection modes are read-only");rejects([&]{select_group_representatives(d,GroupRepresentative::First,static_cast<UngroupedSelection>(99));});
});
run("Random selection is reproducible, one per group, and samples every member",[]{
    Document d;d.load({{"a","A"},{"b","C"},{"c","G"},{"d","T"}});d.apply_groups({{1,"one",0},{2,"two",0}},{{1,1},{2,1},{3,1},{4,2}});std::size_t counts[3]{};for(std::uint64_t seed=0;seed<3000;++seed){const auto ids=select_group_representatives(d,GroupRepresentative::Random,false,seed);need(ids.size()==2&&ids[1]==4&&ids[0]>=1&&ids[0]<=3,"Exactly one member per group");++counts[ids[0]-1];need(ids==select_group_representatives(d,GroupRepresentative::Random,false,seed),"Stable seed");}for(auto count:counts)need(count>850&&count<1150,"Uniform reservoir distribution sanity");
});
run("Large group updates and history retain memberships through row reordering",[]{
    Document d;std::vector<Record> records;for(int i=0;i<20000;++i)records.push_back({"row"+std::to_string(i),"ACGT"});d.load(std::move(records));IdentityGroupOptions o;auto plan=plan_identity_groups(d,o);need(plan.clusters==1&&plan.comparisons==19999,"Streaming greedy comparisons, no all-pairs matrix");apply_identity_groups(d,plan);const auto firstGroup=d.rows()[0].groupId;d.reorder(d.sorted_order(Document::Sort::Reverse));auto groups=d.groups();const auto secondGroup=d.next_group_id();groups.push_back({secondGroup,"Other",0});d.apply_groups(groups,{{19999,secondGroup},{2,secondGroup},{1000,secondGroup}});need(d.sequence(19999).groupId==secondGroup&&d.sequence(2).groupId==secondGroup,"Assignments follow current order regardless of request order");d.undo();need(std::all_of(d.rows().begin(),d.rows().end(),[&](const auto& r){return r.groupId==firstGroup;}),"Undo restores reordered memberships");d.undo();need(d.rows()[0].id==1,"Undo restores row order");d.undo();need(d.groups().empty()&&std::all_of(d.rows().begin(),d.rows().end(),[](const auto& r){return !r.groupId;}),"One grouping Undo clears all 20000 memberships");need(d.next_group_id()>secondGroup,"Retired group IDs remain reserved after undo");d.redo();need(d.groups().size()==1&&d.rows().back().groupId==firstGroup,"Redo preserves original group IDs");
});
run("Pairwise memory guard rejects oversized DP while exact fragments stay linear",[]{
    const std::string a(1000000,'A');auto p=pair(a,a,IdentityAlignment::Pairwise);need(p.matches==a.size(),"Million-base exact pair uses contained-string path");rejects([&]{pair(a,std::string(a.size(),'C'),IdentityAlignment::Pairwise);});
});
run("Automatic grouping drops empty old groups, reuses names and restores catalog on Undo",[]{
    Document d;d.load({{"a","ACGT"},{"b","ACGT"}});d.apply_groups({{1,"Identity_1",0x123456},{2,"Empty",0}},{{1,1},{2,1}});const auto before=session(d);IdentityGroupOptions o;auto p=plan_identity_groups(d,o);need(p.groups.size()==1&&p.groups[0].name=="Identity_1"&&p.groups[0].id>2,"Old empty and replaced groups removed; prefix restarts");apply_identity_groups(d,p);const auto after=session(d);need(d.groups().size()==1&&d.groups()[0].id!=1,"Catalog replaced");d.undo();need(session(d)==before,"Undo restores old colors names and memberships");d.redo();need(session(d)==after,"Redo deletes old groups again");
});
run("Fast word candidates find seeded clusters and every accepted pair meets thresholds",[]{
    std::mt19937 rng(261);std::vector<Record> records;
    for(int i=0;i<40;++i){std::string value(100,'A');for(auto& c:value)c="ACGT"[rng()%4];records.push_back({"centroid"+std::to_string(i),value});}
    for(int i=0;i<40;++i){auto value=records[static_cast<std::size_t>(i)].symbols;value[50]=value[50]=='A'?'C':'A';records.push_back({"member"+std::to_string(i),value});}
    Document d;d.load(records);IdentityGroupOptions o;o.alignment=IdentityAlignment::Pairwise;o.minimumOverlap=100;o.maximumCandidates=0;const auto exact=plan_identity_groups(d,o);o.maximumCandidates=4;const auto fast=plan_identity_groups(d,o);need(fast.clusters==40&&exact.clusters==40&&fast.shortlistedQueries>0,"Candidate index is exercised beyond limit");need(fast.comparisons<exact.comparisons/3,"Candidate filtering reduces full alignments");
    for(std::size_t i=0;i<fast.members.size();++i){const auto& m=fast.members[i];need(m.centroid==exact.members[i].centroid,"Separated synthetic clusters agree with exhaustive");need(m.pair.identity()>=o.minimumIdentity&&m.overlap>=o.minimumOverlap,"Accepted identity and coverage validated by full DP");}
    const auto again=plan_identity_groups(d,o);for(std::size_t i=0;i<fast.members.size();++i)need(again.members[i].centroid==fast.members[i].centroid,"Fast ranking is deterministic");
});
run("Candidate search handles seedless, short, protein and ambiguous calls",[]{
    Document d;d.load({{"a","ACG"},{"b","ACG"},{"c","MNT",RowKind::Protein},{"d","MNT",RowKind::Protein},{"unknown","N?~"},{"ambiguity","RRYY"},{"same ambiguity","rryy"}});IdentityGroupOptions o;o.alignment=IdentityAlignment::Pairwise;o.maximumCandidates=1;o.minimumIdentity=100;const auto p=plan_identity_groups(d,o);need(p.clusters==3&&p.members[0].centroid==p.members[1].centroid&&p.members[2].centroid==p.members[3].centroid&&p.members[5].centroid==p.members[6].centroid,"No usable words still compares compatible centroids");o.maximumCandidates=4097;rejects([&]{plan_identity_groups(d,o);});
});
run("Preview TSV preserves UTF-8, quotes, missing rows and stable IDs without edits",[]{
    Document d;d.load({{"name\twith \"quotes\" "+std::string("é"),"ACGT"},{"same","ACGT"},{"mask","123",RowKind::Mask}});IdentityGroupOptions o;const auto p=plan_identity_groups(d,o);const auto before=session(d);const auto table=identity_groups_tsv(d,p);need(table.starts_with("Sequence\tProposed group\tGroup centroid"),"TSV headers");need(table.find("\"name\twith \"\"quotes\"\" é\"")!=std::string::npos,"Quoted cells round-trip tabs quotes and Unicode");need(table.find("New group centroid")!=std::string::npos&&table.find("Group member")!=std::string::npos&&table.find("Mask row; unchanged")!=std::string::npos,"Every preview result exported");need(session(d)==before&&!d.can_undo(),"Export does not apply groups");d.rename({{1,"edited"}});rejects([&]{identity_groups_tsv(d,p);});
});
std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;}
