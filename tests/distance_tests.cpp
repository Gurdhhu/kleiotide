#include "core/distances.hpp"
#include "core/session.hpp"
#include "core/version.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <random>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void close(double a,double b){need(std::abs(a-b)<1e-11,"Known-answer distance");}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}need(failed,"Expected rejection");}
}
int main(){int passed=0,failed=0;auto run=[&](const char* name,std::function<void()> test){try{test();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Independent numeric answers for observed, p, JC, K2P and Poisson",[]{
    DistanceOptions o;Document d;d.load({{"a","AAAAAAAAAA"},{"b","GGCAAAAAAA"}});
    o.method=DistanceMethod::Differences;auto x=calculate_distances(d,o,DistanceReport::Matrix);need(x.pair(1,0).comparable==10&&x.pair(1,0).differences==3&&x.pair(1,0).transitions==2,"Transition/transversion counts");close(x.pair(0,1).value,3);close(x.pair(0,0).value,0);
    o.method=DistanceMethod::PDistance;close(calculate_distances(d,o,DistanceReport::Matrix).pair(0,1).value,.3);
    o.method=DistanceMethod::JukesCantor;close(calculate_distances(d,o,DistanceReport::Matrix).pair(0,1).value,.383119217824493);
    o.method=DistanceMethod::Kimura2;close(calculate_distances(d,o,DistanceReport::Matrix).pair(0,1).value,.402359478108525);
    d.load({{"a","ACDE",RowKind::Protein},{"b","ACDF",RowKind::Protein}});o.scope.molecule=RowKind::Protein;o.method=DistanceMethod::Poisson;close(calculate_distances(d,o,DistanceReport::Matrix).pair(0,1).value,.287682072451781);
});
run("Symmetry, case/U equivalence and strict ambiguity/missing rules",[]{
    Document d;d.load({{"a","acguRYSWKMBDHVN?~-"},{"b","ACGTAAAAAAAAAAAAAA"},{"protein","ACDE",RowKind::Protein},{"mask","123456789012345678901234567890",RowKind::Mask}});auto x=calculate_distances(d,{},DistanceReport::Matrix);need(x.rows.size()==2&&x.end==18&&x.pair(0,1).comparable==4&&x.pair(0,1).value==0,"Ambiguities excluded and masks do not extend scope");
    DistanceOptions o;o.scope.molecule=RowKind::Protein;Document p;p.load({{"a","AcDBZJXUO*?~-",RowKind::Protein},{"b","AAAAAAAAAAAAA",RowKind::Protein}});auto r=calculate_distances(p,o,DistanceReport::Matrix);need(r.pair(0,1).comparable==3&&r.pair(0,1).differences==2,"Twenty standard amino acids only");
});
run("Pairwise/complete deletion, ragged rows and selected column scope",[]{
    Document d;d.load({{"a","ACGN-"},{"b","ATGT?"},{"c","ACGTT"}});DistanceOptions o;auto x=calculate_distances(d,o,DistanceReport::Matrix);need(x.pair(1,2).comparable==4&&x.pair(0,2).comparable==3,"Pair-specific overlap");o.deletion=DistanceDeletion::Complete;x=calculate_distances(d,o,DistanceReport::Matrix);need(x.completeSites==3&&x.pair(1,2).comparable==3,"Complete deletion uses all included rows");
    o.scope.selectedRows=true;o.scope.ids={2,3};x=calculate_distances(d,o,DistanceReport::Matrix);need(x.completeSites==4,"Complete deletion respects selected-name snapshot");o.scope.selectedColumns=true;o.scope.begin=1;o.scope.end=3;x=calculate_distances(d,o,DistanceReport::Matrix);need(x.begin==1&&x.end==3&&x.pair(0,1).comparable==2,"Selected columns");
    Document rag;rag.load({{"a","A"},{"b","ACGT"}});o={};need(calculate_distances(rag,o,DistanceReport::Matrix).pair(0,1).comparable==1,"Trailing missing never a mismatch");
});
run("No overlap, minimum sites, model boundaries and unavailable diagonal",[]{
    Document d;d.load({{"a","AAAA"},{"b","NNNN"},{"c","TTTT"}});auto x=calculate_distances(d,{},DistanceReport::Matrix);need(x.pair(0,1).failure==DistanceFailure::NoOverlap&&x.pair(1,1).failure==DistanceFailure::NoOverlap,"No false zero, even on diagonal");DistanceOptions o;o.minimumSites=5;need(calculate_distances(d,o,DistanceReport::Matrix).pair(0,2).failure==DistanceFailure::BelowMinimum,"Overlap threshold");o.minimumSites=1;o.method=DistanceMethod::JukesCantor;need(correct_distance(4,3,0,o).failure==DistanceFailure::Saturated,"JC boundary");o.method=DistanceMethod::Kimura2;need(correct_distance(10,5,5,o).failure==DistanceFailure::Saturated&&correct_distance(10,5,0,o).failure==DistanceFailure::Saturated,"Both K2P domains");o.method=DistanceMethod::Poisson;o.scope.molecule=RowKind::Protein;need(correct_distance(4,4,0,o).failure==DistanceFailure::Saturated,"Poisson boundary");o.method=DistanceMethod::PDistance;need(correct_distance(4,4,0,o).value==1,"p=1 remains valid observed fraction");
});
run("Overall, within and between means use unique valid pairs with exclusion counts",[]{
    Document d;d.load({{"a","AAAA"},{"b","AAAT"},{"c","AATT"},{"d","TTTT"},{"missing","NNNN"}});d.apply_groups({{1,"North",0x123456},{2,"South",0x654321}},{{1,1},{2,1},{3,2},{4,2}});DistanceOptions o;auto r=calculate_distances(d,o,DistanceReport::Overall);need(r.means[0].valid==6&&r.means[0].noOverlap==4,"Each unordered pair once");close(r.means[0].value(),3.25/6);r=calculate_distances(d,o,DistanceReport::WithinGroups);need(r.means.size()==3&&r.means[2].valid==0,"Singleton ungrouped is N/A");close(r.means[0].value(),.25);close(r.means[1].value(),.5);r=calculate_distances(d,o,DistanceReport::BetweenGroups);need(r.means.size()==3&&r.means[0].valid==4&&r.means[1].noOverlap==2&&r.means[2].noOverlap==2,"Cross-group pairs");close(r.means[0].value(),.625);o.includeUngrouped=false;r=calculate_distances(d,o,DistanceReport::Overall);need(r.rows.size()==4&&r.means[0].excluded()==0,"Explicit ungrouped exclusion");
    o.method=DistanceMethod::JukesCantor;r=calculate_distances(d,o,DistanceReport::Overall);need(r.means[0].saturated==2&&r.means[0].valid==4,"Saturated pairs omitted from mean, counted separately");
});
run("Means average pair estimates equally rather than pooling sites",[]{
    Document d;d.load({{"a","AAAA"},{"b","T"},{"c","AAAT"}});auto r=calculate_distances(d,{},DistanceReport::Overall);close(r.means[0].value(),.75);need(r.means[0].minimumComparable==1&&r.means[0].maximumComparable==4,"Comparable range retained");
});
run("Nearest-neighbor chain differs from fixed-reference ranking and preserves slots",[]{
    Document d;d.load({{"mask","1234",RowKind::Mask},{"a","AAAA"},{"protein","ACDE",RowKind::Protein},{"b","AAAT"},{"c","AATA"},{"d","AATT"},{"unknown","NNNN"}});const auto content=d.content_id();auto r=similarity_order(d,{});need(r.order==std::vector<SequenceId>({1,2,3,4,6,5,7})&&r.uncomparable==1,"Expected chain and fixed noneligible slots");need(d.content_id()==content&&!d.can_undo(),"Calculation is read-only");d.reorder(r.order);d.undo();need(d.rows()[4].id==5,"Atomic undo restores source order");d.redo();need(d.rows()[4].id==6,"Redo restores chain");
});
run("Similarity ties prefer overlap, then stable input order; disconnected rows last",[]{
    Document d;d.load({{"a","AAAA"},{"short","AAAN"},{"long1","AAAA"},{"long2","AAAA"},{"zero","NNNN"}});auto r=similarity_order(d,{});need(r.order==std::vector<SequenceId>({1,3,4,2,5}),"Overlap and stable ties");DistanceOptions o;o.minimumSites=4;r=similarity_order(d,o);need(r.uncomparable==2&&r.order==std::vector<SequenceId>({1,3,4,2,5}),"No valid overlap rows at end");o.scope.selectedRows=true;o.scope.ids={2,3,4};r=similarity_order(d,o);need(r.order.front()==1&&r.order.back()==5&&r.order[1]==2,"Only eligible selected slots reordered, first eligible retained");
});
run("Disconnected overlap components restart stably before no-comparison rows",[]{Document d;d.load({{"first","AA??"},{"second","AT??"},{"unknown","NNNN"},{"third","??GG"},{"fourth","??GA"}});DistanceOptions o;o.minimumSites=2;const auto order=similarity_order(d,o);need(order.order==std::vector<SequenceId>({1,2,4,5,3})&&order.uncomparable==1,"Stable component restart");});
run("Cancellation, invalid inputs and quadratic memory bounds",[]{
    Document d;d.load({{"a","AAAA"},{"b","AAAT"}});const auto state=d.state_id();rejects([&]{calculate_distances(d,{},DistanceReport::Matrix,[]{return true;});});rejects([&]{similarity_order(d,{},[]{return true;});});need(d.state_id()==state,"Cancellation no mutation");DistanceOptions o;o.minimumSites=0;rejects([&]{calculate_distances(d,o,DistanceReport::Matrix);});o={};o.scope.selectedRows=true;rejects([&]{calculate_distances(d,o,DistanceReport::Matrix);});o.scope.ids={99};rejects([&]{calculate_distances(d,o,DistanceReport::Matrix);});o={};o.method=DistanceMethod::Poisson;rejects([&]{calculate_distances(d,o,DistanceReport::Matrix);});
    std::vector<Record> rows;for(int i=0;i<3000;++i)rows.push_back({std::to_string(i),"A"});d.load(std::move(rows));rejects([&]{estimate_distances(d,{},DistanceReport::Matrix);});need(estimate_distances(d,{},DistanceReport::Overall).bytes<1024,"Streaming means do not allocate matrix");
});
run("TSV retains duplicate labels via indices, settings, comparable sites and reasons",[]{
    Document d;d.load({{"same","ACGT"},{"same","NNNN"}});auto r=calculate_distances(d,{},DistanceReport::Matrix);const auto text=distance_tsv(r);need(text.find("# Comparable-site matrix")!=std::string::npos&&text.find("No comparable sites")!=std::string::npos&&text.find("1\t1\tsame")!=std::string::npos&&text.find("2\t2\tsame")!=std::string::npos,"Labels and unavailable reasons");rejects([&]{distance_tsv(r,10);});std::string streamed;write_distance_tsv(r,[&](std::string_view part){streamed+=part;});need(streamed==text,"Streaming file export matches Copy");rejects([&]{write_distance_tsv(r,[](std::string_view){},[]{return true;});});
});
run("Randomized pair counts agree with an independent column oracle",[]{
    std::mt19937 random(20261019);const std::string alphabet="ACGTUNR?~-acgt";
    for(unsigned trial=0;trial<100;++trial){std::vector<Record> records;for(unsigned row=0;row<5;++row){std::string text;for(unsigned c=0,n=1+random()%40;c<n;++c)text+=alphabet[random()%alphabet.size()];records.push_back({std::to_string(row),text});}Document d;d.load(records);
        for(bool complete:{false,true}){DistanceOptions o;o.deletion=complete?DistanceDeletion::Complete:DistanceDeletion::Pairwise;o.minimumSites=2;auto result=calculate_distances(d,o,DistanceReport::Matrix);
            auto base=[](char c){switch(c){case 'a':case 'A':return 1;case 'c':case 'C':return 2;case 'g':case 'G':return 3;case 't':case 'T':case 'U':return 4;default:return 0;}};
            for(std::size_t i=0;i<records.size();++i)for(std::size_t j=0;j<records.size();++j){std::size_t sites=0,differences=0;for(std::size_t c=0;c<d.columns();++c){bool retained=true;if(complete)for(const auto& row:records)if(c>=row.symbols.size()||!base(row.symbols[c]))retained=false;if(!retained||c>=records[i].symbols.size()||c>=records[j].symbols.size())continue;const int a=base(records[i].symbols[c]),b=base(records[j].symbols[c]);if(a&&b){++sites;differences+=a!=b;}}const auto& got=result.pair(i,j);need(got.comparable==sites&&got.differences==differences,"Independent count oracle");need(got.valid()==(sites>=2),"Independent minimum-site oracle");if(got.valid())close(got.value,static_cast<double>(differences)/sites);}
        }
    }
});
run("Cancellation is checked inside long comparisons and streaming work",[]{
    Document d;d.load({{"a",std::string(50000,'A')},{"b",std::string(50000,'T')}});unsigned polls=0;rejects([&]{calculate_distances(d,{},DistanceReport::Matrix,[&]{return ++polls>20;});});need(polls>20&&!d.can_undo(),"Mid-comparison cancellation leaves source intact");DistanceOptions o;o.deletion=DistanceDeletion::Complete;polls=0;rejects([&]{calculate_distances(d,o,DistanceReport::Overall,[&]{return ++polls>20;});});
});
run("SD and naive SE use valid pair distances, sample divisor and group scope",[]{
    Document d;d.load({{"a","AAAA"},{"b","AAAT"},{"c","AATT"},{"d","TTTT"},{"missing","NNNN"}});d.apply_groups({{1,"North",0x123456},{2,"South",0x654321}},{{1,1},{2,1},{3,2},{4,2}});
    auto overall=calculate_distances(d,{},DistanceReport::Overall);const auto& m=overall.means[0];need(m.valid==6&&m.noOverlap==4,"Missing pairs never enter dispersion denominator");close(*m.sample_sd(),0.292261298612503);close(*m.naive_se(),0.119315175527303);
    auto within=calculate_distances(d,{},DistanceReport::WithinGroups);need(within.means[0].valid==1&&!within.means[0].sample_sd()&&!within.means[0].naive_se(),"One pair has a mean but no sample SD/SE");need(!within.means[2].sample_sd()&&!within.means[2].naive_se(),"Zero pairs are unavailable");
    auto between=calculate_distances(d,{},DistanceReport::BetweenGroups);close(*between.means[0].sample_sd(),.322748612183951);close(*between.means[0].naive_se(),.161374306091976);
    const auto text=distance_tsv(overall);need(text.find("SD (pairs)\tSE (naive)")!=std::string::npos&&text.find("m-1")!=std::string::npos&&text.find("assumes independent pairs")!=std::string::npos,"Exports include columns, formulas and dependence limitation");need(text.find(std::string("# Kleiotide version: ")+version+"\n")!=std::string::npos,"TSV has exact build version");
});
run("Two observations and equal distances have correct dispersion boundaries",[]{Document d;d.load({{"a","A?"},{"b","?T"},{"c","AC"}});auto m=calculate_distances(d,{},DistanceReport::Overall).means[0];need(m.valid==2,"Two valid pairs");close(*m.sample_sd(),.707106781186548);close(*m.naive_se(),.5);d.load({{"a","A"},{"b","C"},{"c","G"}});m=calculate_distances(d,{},DistanceReport::Overall).means[0];close(*m.sample_sd(),0);close(*m.naive_se(),0);});
run("Dispersion agrees with independent two-pass oracle for every distance model",[]{
    for(auto method:{DistanceMethod::Differences,DistanceMethod::PDistance,DistanceMethod::JukesCantor,DistanceMethod::Kimura2,DistanceMethod::Poisson}){Document d;const bool protein=method==DistanceMethod::Poisson;const auto kind=protein?RowKind::Protein:RowKind::Nucleotide;d.load({{"a",protein?"AAAAAAAAAA":"AAAAAAAAAA",kind},{"b",protein?"ACAAAAAAAA":"AGAAAAAAAA",kind},{"c",protein?"ACDAAAAAAA":"AGCAAAAAAA",kind},{"distant","TTTTTTTTTT",kind},{"missing",protein?"XXXXXXXXXX":"NNNNNNNNNN",kind}});DistanceOptions o;o.method=method;o.scope.molecule=kind;const auto matrix=calculate_distances(d,o,DistanceReport::Matrix);std::vector<double> values;for(std::size_t i=0;i<matrix.rows.size();++i)for(std::size_t j=0;j<i;++j)if(matrix.pair(i,j).valid())values.push_back(matrix.pair(i,j).value);double mean=0;for(auto x:values)mean+=x;mean/=static_cast<double>(values.size());double squares=0;for(auto x:values)squares+=(x-mean)*(x-mean);const auto m=calculate_distances(d,o,DistanceReport::Overall).means[0];close(*m.sample_sd(),std::sqrt(squares/static_cast<double>(values.size()-1)));close(*m.naive_se(),std::sqrt(squares/static_cast<double>((values.size()-1)*values.size())));}
});
std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;
}
