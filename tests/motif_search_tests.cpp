#include "gui/motif_search.hpp"
#include <QCoreApplication>
#include <algorithm>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
using namespace kleiotide;
namespace {void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}require(threw,"Expected failure");}}
int main(int argc,char** argv){QCoreApplication app(argc,argv);int passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Literal complete index counts strands and maps original gap spans",[]{Document d;d.load({{"f","~AA-G"},{"r","C-TT"},{"mask","AAG",RowKind::Mask}});auto result=search_motifs(d,"AAG",{true,true,false},false);const auto& i=*result.index;
    require(i.total==2&&i.forward==1&&i.reverse==1&&!i.both,"Forward/reverse counts");require(i.at(0).id==1&&i.at(0).match.start==1&&i.at(0).match.width==4&&i.at(1).match.reversed,"Mapped hit coordinates/order");
    Document p;p.load({{"palindrome","ATAT"}});auto both=search_motifs(p,"AT",{true,true,false},false).index;require(both->total==2&&both->forward==2&&both->reverse==2&&both->both==2,"Both orientations contribute to strand counts, total intervals once");
});
run("Regular expression alternation, classes, bounded repeats, anchors and captures",[]{Document d;d.load({{"one","AAAACACACG"}});
    auto i=search_motifs(d,"A{2,3}",{},true).index;require(i->total==3&&i->at(0).match.width==3&&i->at(2).match.width==2,"Greedy overlapping quantifier");
    i=search_motifs(d,"(AC)\\1",{},true).index;require(i->total==2&&i->at(0).match.start==3&&i->at(1).match.start==5,"Backreferences and overlaps");
    i=search_motifs(d,"^A{4}C|[CG]$",{},true).index;require(i->total==2&&i->at(0).match.width==5&&i->at(1).match.start==9,"Alternation/class/anchors");
    i=search_motifs(d,"A.G",{},true).index;require(i->total==1&&i->at(0).match.start==7,"Regex dot is a wildcard");
});
run("Regex reverse complements preserve anchors and variable aligned intervals",[]{Document d;d.load({{"one","C-TT"},{"two","TTT"}});
    auto i=search_motifs(d,"^AAG$",{true,true,false},true).index;require(i->total==1&&i->at(0).id==1&&i->at(0).match.reversed&&i->at(0).match.width==4,"Regex runs on reverse-complemented subject and maps back");
    i=search_motifs(d,"A+",{true,true,false},true,std::vector<SequenceId>{2}).index;require(i->total==3&&i->at(0).match.start==0&&i->at(0).match.width==1&&i->at(2).match.width==3,"Same-start variable-width reverse hits remain distinct");
    Document p;p.load({{"both","AT"}});i=search_motifs(p,"AT",{true,true,false},true).index;require(i->total==1&&i->both==1,"Same interval merges both orientations");
});
run("Regex scopes, gaps, zero-length matches and bounded engine failures",[]{Document d;d.load({{"one","A~G"},{"two","AG"},{"mask","AG",RowKind::Mask}});
    auto i=search_motifs(d,"A~?G",{false,false,false},true).index;require(i->total==2&&i->at(0).match.width==3,"Literal gap quantification");
    require(search_motifs(d,"AG",{},true,std::vector<SequenceId>{}).index->total==0,"Explicit empty scope");
    auto zero=search_motifs(d,"^",{},true);require(!zero.index->total&&zero.emptyMatches==2,"Ignore zero-width matches without looping");
    rejects([&]{search_motifs(d,"[",{},true);});Document bad;bad.load({{"bad",std::string(10000,'A')+"C"}});rejects([&]{search_motifs(bad,"(A+)+$",{},true);});
    require(!bad.can_undo(),"Regex failures leave data/history intact");
});
run("Dense arithmetic hits compress and ordinal/coverage queries do not expand them",[]{MotifIndex i;for(std::size_t n=0;n<1000000;++n)i.add({1,{n,3,false}});i.finish();
    require(i.total==1000000&&i.encoded_bytes()<256&&i.at(999999).match.start==999999,"Million hits in one compact run");
    auto cells=i.coverage(1,999900,1000002);require(cells.size()==102&&std::all_of(cells.begin(),cells.end(),[](auto c){return c&1;}),"Dense/overlapping visible coverage");
    rejects([&]{i.at(1000000);});MotifIndex limited(1);limited.add({1,{0,1,false}});rejects([&]{limited.finish();});
});
run("Random variable runs, checkpoints and clipped coverage match brute-force intervals",[]{std::mt19937 rng(15151);
    for(int trial=0;trial<40;++trial){MotifIndex index;std::vector<MotifHit> expected;
        for(SequenceId row=1;row<=3;++row){std::size_t start=0;for(int n=0;n<350;++n){start+=1+rng()%5;const auto flags=rng()%3;MotifHit h{row,{start,1+rng()%100,flags==1,flags==2}};expected.push_back(h);index.add(h);}}
        index.finish();for(std::size_t n=0;n<expected.size();++n){const auto a=index.at(n),b=expected[n];require(a.id==b.id&&a.match.start==b.match.start&&a.match.width==b.match.width&&a.match.reversed==b.match.reversed&&a.match.bothStrands==b.match.bothStrands,"Random ordinal round trip across blocks/rows");}
        for(int n=0;n<30;++n){const SequenceId row=1+rng()%4;const std::size_t begin=rng()%1900,end=begin+60;std::vector<unsigned char> expectedCells(end-begin,0);
            for(const auto& h:expected)if(h.id==row){const auto a=h.match.start,b=a+h.match.width;for(auto c=std::max(begin,a);c<std::min(end,b);++c)expectedCells[c-begin]|=1;if(a>=begin&&a<end)expectedCells[a-begin]|=2;if(b>begin&&b<=end)expectedCells[b-1-begin]|=4;}
            require(index.coverage(row,begin,end)==expectedCells,"All shading and outline edges agree with brute force");}
    }
});
run("Literal and regex full scans cancel without modifying document state",[]{Document d;d.load({{"long",std::string(1000000,'A')}});const auto state=d.state_id();for(bool regex:{false,true}){int checks=0;rejects([&]{search_motifs(d,"AAA",{},regex,{},[&]{return ++checks>3;});});}require(d.state_id()==state&&!d.can_undo(),"Cancellation is read-only");});

run("N-only exclusion handles gaps, case, mixed ambiguity and both strands",[]{Document d;d.load({{"unknown","N-~nN"},{"mixed","NAN"},{"ambiguous","NNR"},{"question","???"},{"gaps","-~~-"},{"mask","NNN",RowKind::Mask}});
    auto i=search_motifs(d,"NNN",{true,true,true},false,{}, {},true).index;
    require(i->total==3&&i->both==3&&i->forward==3&&i->reverse==3,"Only actual N-only intervals excluded; mixed/other ambiguities retained on both strands");
    for(bool ignore:{false,true}){auto result=search_motifs(d,ignore?"N{3}":"N[-~]*N",{ignore,true,false},true,{}, {},true);require(!result.index->total,"Regex exclusion maps reverse/gapped intervals to actual symbols");}
    i=search_motifs(d,"[-~]+",{false,false,false},true,{}, {},true).index;require(i->total==6,"Gap-only hits contain no N and remain searchable");
    require(search_motifs(d,"NNN",{true,true,true},false).index->total==4,"Default preserves N-only matches");
});
run("N-only rank filter agrees with brute force across block boundaries",[]{std::mt19937 rng(15252);Document d;std::vector<Record> rows;
    for(int r=0;r<8;++r){std::string text;for(int c=0;c<289+r;++c)text+="NNNNNnAG-R~?"[rng()%12];rows.push_back({std::to_string(r),text});}d.load(rows);
    for(bool regex:{false,true})for(bool ignore:{false,true})for(bool reverse:{false,true}){
        const std::string query=regex?"[NAG?~-]{1,100}":std::string(65,'N');const MotifOptions options{ignore,reverse,true};
        const auto all=search_motifs(d,query,options,regex).index,filtered=search_motifs(d,query,options,regex,{}, {},true).index;
        std::vector<MotifHit> expected;std::uint64_t forward=0,backward=0,both=0;
        for(std::uint64_t n=0;n<all->total;++n){const auto hit=all->at(n);const auto span=d.sequence(hit.id).symbols.text().substr(hit.match.start,hit.match.width);bool hasN=false,hasOther=false;
            for(char c:span){hasN|=c=='N'||c=='n';hasOther|=c!='N'&&c!='n'&&!is_gap(c);}if(hasN&&!hasOther)continue;
            expected.push_back(hit);forward+=!hit.match.reversed||hit.match.bothStrands;backward+=hit.match.reversed||hit.match.bothStrands;both+=hit.match.bothStrands;}
        require(filtered->total==expected.size()&&filtered->forward==forward&&filtered->reverse==backward&&filtered->both==both,"Filtered totals and orientation counts agree with actual content");
        for(std::size_t n=0;n<expected.size();++n){const auto a=filtered->at(n),b=expected[n];require(a.id==b.id&&a.match.start==b.match.start&&a.match.width==b.match.width&&a.match.reversed==b.match.reversed&&a.match.bothStrands==b.match.bothStrands,"Retained intervals/order/orientations unchanged");}
    }
});
run("Dense N-only filtering stays compact and cancellation remains read-only",[]{Document d;d.load({{"long",std::string(100032,'N')}});const auto state=d.state_id();
    require(search_motifs(d,"N{3}",{},true).index->total==100030,"Dense baseline includes every overlap");
    for(bool regex:{false,true})require(search_motifs(d,regex?"N{3}":"NNN",{true,true,true},regex,{}, {},true).index->total==0,"Dense filtered intervals never enter the result index");
    int checks=0;rejects([&]{search_motifs(d,"NNN",{},false,{},[&]{return ++checks>8;},true);});require(d.state_id()==state&&!d.can_undo(),"Filter cancellation preserves document/history");
});

std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;}
