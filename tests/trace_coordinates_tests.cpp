#include "core/abif.hpp"
#include "core/trace_coordinates.hpp"
#include "core/session.hpp"
#include "abif_fixture.hpp"
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <cmath>
using namespace kleiotide;
void require(bool v,const char* msg){if(!v)throw std::runtime_error(msg);}
Document example(){std::istringstream in(test::synthetic_abif());Document d;d.load({read_abif(in,"trace")});return d;}
void anchors(const Sequence& row){TraceCoordinates coordinates(row);std::size_t ordinal=0;for(std::size_t c=0;c<row.symbols.size();++c)if(!is_gap(row.symbols.text()[c])){auto call=row.trace->originalCalls[ordinal++];if(call>=0){const double sample=row.trace->original->peaks[static_cast<std::size_t>(call)];require(std::abs(coordinates.column_for_sample(sample)-(c+.5))<1e-8,"Surviving peaks align with columns");require(std::abs(coordinates.sample_for_column(c+.5)-sample)<1e-8,"Inverse anchor");}}}
int main(){int passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Gap insertion stretches signal intervals without changing evidence",[]{auto d=example();auto evidence=d.rows()[0].trace->original;d.replace(1,2,0,"---~~");anchors(d.rows()[0]);TraceCoordinates map(d.rows()[0]);require(std::abs(map.column_for_sample(60)-4.5)<1e-8,"Expanded midpoint spans gap columns");require(d.rows()[0].trace->original==evidence,"Original evidence immutable");d.undo();anchors(d.rows()[0]);});
run("Inserted and deleted bases, compression and strand keep invertible coordinates",[]{auto d=example();d.replace(1,2,0,"WR");d.replace(1,5,1,"");anchors(d.rows()[0]);TraceCoordinates map(d.rows()[0]);require(map.column_for_sample(90)>map.column_for_sample(70)&&map.column_for_sample(90)<map.column_for_sample(110),"Deleted original peak remains between survivors");d.transform({1},Document::Transform::ReverseComplement);anchors(d.rows()[0]);TraceCoordinates reverse(d.rows()[0]);require(reverse.column_for_sample(30)>reverse.column_for_sample(130),"Reversed direction");});
run("All-deleted/all-synthetic and coincident peaks remain finite",[]{auto d=example();d.replace(1,0,d.rows()[0].symbols.size(),"");TraceCoordinates empty(d.rows()[0]);require(std::isfinite(empty.sample_for_column(0)),"Empty current row");d.replace(1,0,0,"AAA");TraceCoordinates synthetic(d.rows()[0]);require(synthetic.column_for_sample(30)<synthetic.column_for_sample(130),"Synthetic-only finite order");
 auto record=d.rows()[0];auto data=std::make_shared<Chromatogram>(*record.trace->original);data->peaks={30,30,70,90,110,130};auto link=std::make_shared<TraceLink>();link->original=data;link->originalCalls={0,1,2,3,4,5};Document duplicate;duplicate.load({{"duplicate","ACGTRN",RowKind::Nucleotide,link}});TraceCoordinates shared(duplicate.rows()[0]);require(std::abs(shared.column_for_sample(30)-1.)<1e-8,"Shared peak uses average surviving column");});
run("Random edits/strand and session restore preserve coordinate inverses",[]{auto d=example();std::mt19937 random(1010);const auto evidence=d.rows()[0].trace->original;for(unsigned i=0;i<200;++i){const auto size=d.rows()[0].symbols.size();if(random()%7==0)d.transform({1},Document::Transform::ReverseComplement);else{const auto at=random()%(size+1);const auto count=random()%(std::min<std::size_t>(size-at,3)+1);std::string text;for(unsigned j=0,n=random()%4;j<n;++j)text+="ACGTN-~"[random()%7];d.replace(1,at,count,text);}anchors(d.rows()[0]);TraceCoordinates map(d.rows()[0]);for(unsigned sample=0;sample<evidence->channels[0].size();sample+=13)require(std::abs(map.sample_for_column(map.column_for_sample(sample))-sample)<1e-7,"Piecewise inverse");}
 std::ostringstream out;write_session(out,d);std::istringstream in(out.str());auto restored=read_session(in);anchors(restored.document.rows()[0]);require(restored.document.rows()[0].trace->original->channels==evidence->channels,"Session original samples exact");});
std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;}
