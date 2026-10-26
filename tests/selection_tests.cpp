#include "core/row_selection.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_set>
using namespace kleiotide;
namespace {
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::invalid_argument&){failed=true;}require(failed,"Expected invalid input rejection");}
unsigned reference_bases(char c){const std::string codes="ACGTURYSWKMBDHVN?";const unsigned bits[]={1,2,4,8,8,5,10,6,9,12,3,14,13,11,7,15,15};const auto i=codes.find(c);return i==std::string::npos?0:bits[i];}
}
int main(){int passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& test){try{test();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Case-insensitive duplicates respect gaps, row kinds and first copies",[]{Document d;d.load({{"1","aa~-C"},{"2","A-Ac"},{"3","AAC"},{"4","AAC",RowKind::Mask},{"5","aac",RowKind::Mask},{"6",""},{"7","~~~"}});
    require(select_duplicate_sequences(d,true,false)==std::vector<SequenceId>({1,2,3,4,5,6,7}),"Ungapped groups");require(select_duplicate_sequences(d,true,true)==std::vector<SequenceId>({2,3,5,7}),"Additional copies");require(select_duplicate_sequences(d,false,false)==std::vector<SequenceId>({4,5}),"Aligned gap types/positions retained");});
run("Motif exact matching and original alignment-coordinate span",[]{Document d;d.load({{"one","~~aC-G~T"},{"mask","ACGT",RowKind::Mask},{"short","AC"}});auto hit=find_motif(d.sequence(1)," a c g t ");require(hit&&hit->start==2&&hit->width==6&&!hit->reversed,"Ungapped coordinate span");require(!find_motif(d.sequence(1),"ACGT",{false,false,false}),"Aligned exact query differs");hit=find_motif(d.sequence(1),"AC-G~T",{false,false,false});require(hit&&hit->start==2&&hit->width==6,"Aligned exact gap span");require(select_motif(d,"ACGT")==std::vector<SequenceId>({1}),"Mask/short excluded");});
run("Reverse complement chooses earliest match and preserves orientation",[]{Document d;d.load({{"one","GGCTT"},{"two","AAG~~CTT"},{"three","cTt~AAG"},{"rna","UUG"}});auto hit=find_motif(d.sequence(1),"AAG",{true,true,false});require(hit&&hit->start==2&&hit->reversed,"Reverse-only motif");require(!find_motif(d.sequence(1),"AAG"),"Forward-only excludes reverse");hit=find_motif(d.sequence(2),"AAG",{true,true,false});require(hit&&hit->start==0&&!hit->reversed,"Earliest forward hit");hit=find_motif(d.sequence(3),"AAG",{true,true,false});require(hit&&hit->start==0&&hit->reversed,"Earliest reverse hit");hit=find_motif(d.sequence(4),"CAA",{true,true,true});require(hit&&hit->reversed,"RNA/DNA compatible reverse");});
run("IUPAC compatibility is optional with explicit unknown/gap semantics",[]{Document d;d.load({{"one","GATUN?R"}});require(!find_motif(d.sequence(1),"RAT"),"Exact R remains literal");auto hit=find_motif(d.sequence(1),"RAT",{true,false,true});require(hit&&hit->start==0,"R includes G");require(!find_motif(d.sequence(1),"Y",{false,false,false}),"Exact ambiguity remains literal");require(find_motif(d.sequence(1),"AAA",{true,false,true}).has_value(),"N/? compatible sets");Document gaps;gaps.load({{"gaps","-~"}});hit=find_motif(gaps.sequence(1),"~",{false,false,true});require(hit&&hit->start==1,"Compatibility retains gap types");});
run("Compatible motifs cross 64-bit word boundaries and find overlaps",[]{for(std::size_t n:{1,63,64,65,127,128,129,4096}){Document d;d.load({{"long",std::string(7,'T')+std::string(n,'A')+"C"}});auto hit=find_motif(d.sequence(1),std::string(n,'R'),{true,false,true});require(hit&&hit->start==7&&hit->width==n,"Multiword compatible matching");}Document d;d.load({{"overlap","AAAAC"}});auto hit=find_motif(d.sequence(1),"AAC",{true,false,true});require(hit&&hit->start==2,"Overlapping candidate hit");});
run("2000 randomized motif matches agree with independent brute force",[]{std::mt19937 random(1200);const std::string alphabet="ACGTRYN?-~";for(unsigned trial=0;trial<2000;++trial){std::string row,query;const auto rowLength=random()%200+1,queryLength=random()%90+1;for(unsigned i=0;i<rowLength;++i)row+=alphabet[random()%alphabet.size()];for(unsigned i=0;i<queryLength;++i)query+=alphabet[random()%alphabet.size()];const bool gaps=random()%2;std::string text,pattern;std::vector<std::size_t> columns;for(std::size_t i=0;i<row.size();++i)if(!gaps||!is_gap(row[i])){text+=row[i];columns.push_back(i);}for(auto c:query)if(!gaps||!is_gap(c))pattern+=c;if(pattern.empty())continue;
    std::optional<std::size_t> expected;for(std::size_t p=0;p+pattern.size()<=text.size();++p){bool match=true;for(std::size_t j=0;j<pattern.size();++j){const bool compatible=is_gap(pattern[j])?pattern[j]==text[p+j]:(reference_bases(pattern[j])&reference_bases(text[p+j]))!=0;if(!compatible){match=false;break;}}if(match){expected=p;break;}}
    Document d;d.load({{"random",row}});const auto hit=find_motif(d.sequence(1),query,{gaps,false,true});require(hit.has_value()==expected.has_value(),"Random motif presence");if(expected)require(hit->start==columns[*expected]&&hit->width==columns[*expected+pattern.size()-1]-columns[*expected]+1,"Random motif coordinates");}});
run("Invalid, empty and oversized motifs fail before selection",[]{Document d;d.load({{"one","ACGT"}});for(const auto& query:{std::string{},std::string("  "),std::string("~~--"),std::string("AZ"),std::string(4097,'A')})rejects([&]{select_motif(d,query);});require(!find_motif(d.sequence(1),std::string(4096,'A')),"Long valid motif no match");});
run("Random selection is exact, reproducible and never reorders or edits rows",[]{Document d;std::vector<Record> rows;for(unsigned i=0;i<200;++i)rows.push_back({std::to_string(i),"ACGT"});d.load(rows);const auto state=d.state_id();for(std::size_t count:{0,1,10,199,200}){const auto ids=select_random_rows(d,count,42);require(ids.size()==count&&std::is_sorted(ids.begin(),ids.end())&&std::unordered_set<SequenceId>(ids.begin(),ids.end()).size()==count,"Unique stable-order sample");require(ids==select_random_rows(d,count,42),"Seed reproducibility");}require(select_random_rows(d,10,42)!=select_random_rows(d,10,43),"Fresh seed changes sample");require(d.state_id()==state&&!d.can_undo(),"Selection never edits data");rejects([&]{select_random_rows(d,201,42);});});
run("Navigator includes overlaps, ignores masks, and wraps in both directions",[]{Document d;d.load({{"same","AAAA"},{"mask","AAA",RowKind::Mask},{"same","~~AA"}});
    auto hit=navigate_motif(d,"AA");require(hit.hit&&hit.hit->id==1&&hit.hit->match.start==0&&!hit.wrapped,"First match");
    for(auto expected:std::vector<std::pair<SequenceId,std::size_t>>{{1,1},{1,2},{3,2},{1,0}}){hit=navigate_motif(d,"AA",{},{MotifLocation{hit.hit->id,hit.hit->match.start}});require(hit.hit&&hit.hit->id==expected.first&&hit.hit->match.start==expected.second,"Ordered overlapping navigation");}
    require(hit.wrapped,"Wrap reports boundary crossing");hit=navigate_motif(d,"AA",{},MotifLocation{1,0},true,true);require(hit.wrapped&&hit.hit->id==3&&hit.hit->match.start==2,"Previous wraps to last");
    require(!navigate_motif(d,"AA",{},MotifLocation{1,0},true,false).hit,"Previous stops at beginning without wrap");require(!navigate_motif(d,"AA",{},MotifLocation{3,2},false,false).hit,"Next stops at end without wrap");
});
run("Navigator maps gap spans, exact gaps and reverse hits without strand duplicates",[]{Document d;d.load({{"one","~a-~A~G~CT-T"},{"palindrome","ATAT"},{"ambiguous","N"}});
    auto hit=navigate_motif(d,"AAG",{true,true,false});require(hit.hit&&hit.hit->match.start==1&&hit.hit->match.width==6&&!hit.hit->match.reversed,"Gap-aware forward span");
    hit=navigate_motif(d,"AAG",{true,true,false},MotifLocation{1,1});require(hit.hit&&hit.hit->id==1&&hit.hit->match.start==8&&hit.hit->match.width==4&&hit.hit->match.reversed,"Gap-aware reverse span");
    hit=navigate_motif(d,"A-~A",{false,false,false});require(hit.hit&&hit.hit->match.start==1&&hit.hit->match.width==4,"Exact aligned gaps");
    hit=navigate_motif(d,"AT",{true,true,false},{},false,false,std::vector<SequenceId>{2});require(hit.hit&&hit.hit->match.bothStrands&&!hit.hit->match.reversed,"Palindromic hits coalesce");
    hit=navigate_motif(d,"A",{true,true,true},{},false,false,std::vector<SequenceId>{3});require(hit.hit&&hit.hit->match.bothStrands,"Compatible forward and reverse coalesce");
});
run("Navigator explicit empty/selected scopes and missing cursor validation",[]{Document d;d.load({{"one","AA"},{"two","AA"},{"mask","AA",RowKind::Mask}});
    require(!navigate_motif(d,"A",{},{},false,true,std::vector<SequenceId>{}).hit,"Empty selected scope is not all rows");
    auto hit=navigate_motif(d,"A",{},{},false,true,std::vector<SequenceId>{2,3});require(hit.hit&&hit.hit->id==2,"Only selected biological rows");
    rejects([&]{navigate_motif(d,"A",{},MotifLocation{999,0});});
});
run("Navigator cancels during a long row and never changes data/history",[]{Document d;d.load({{"long",std::string(200000,'A')}});const auto state=d.state_id();int checks=0;bool cancelled=false;
    try{navigate_motif(d,"CCCC",{},{},false,true,{},[&]{return ++checks>=4;});}catch(const std::runtime_error&){cancelled=true;}
    require(cancelled&&checks==4&&d.state_id()==state&&!d.can_undo(),"Cooperative cancellation within a row");
    auto hit=navigate_motif(d,std::string(129,'R'),{true,false,true});require(hit.hit&&hit.hit->match.width==129,"Navigator compatible matching crosses word boundaries");
});
run("Navigator all hits and reverse traversal agree with independent randomized brute force",[]{std::mt19937 rng(1515);const std::string alphabet="ACGTRYN?-~";
    for(int trial=0;trial<350;++trial){Document d;std::vector<Record> rows;for(int r=0;r<3;++r){std::string s;for(int c=0;c<45;++c)s+=alphabet[rng()%alphabet.size()];rows.push_back({"duplicate",s});}d.load(rows);
        std::string query;for(unsigned i=0,n=1+rng()%6;i<n;++i)query+="ACGTRYN?"[rng()%8];const bool ignore=rng()%2,compatible=rng()%2;
        std::vector<std::pair<SequenceId,MotifMatch>> expected;
        for(const auto& row:d.rows()){std::string text;std::vector<std::size_t> columns;for(std::size_t c=0;c<row.symbols.size();++c)if(!ignore||!is_gap(row.symbols.text()[c])){text+=row.symbols.text()[c];columns.push_back(c);}
            for(std::size_t p=0;p+query.size()<=text.size();++p){bool match=true;for(std::size_t j=0;j<query.size();++j)if(compatible?(reference_bases(query[j])&reference_bases(text[p+j]))==0:query[j]!=text[p+j]){match=false;break;}
                if(match)expected.push_back({row.id,{columns[p],columns[p+query.size()-1]-columns[p]+1,false}});}}
        for(bool backwards:{false,true}){std::optional<MotifLocation> cursor;for(std::size_t i=0;i<=expected.size();++i){const auto hit=navigate_motif(d,query,{ignore,false,compatible},cursor,backwards,false);
                require(hit.hit.has_value()==(i<expected.size()),"Brute-force hit count");if(!hit.hit)break;const auto& value=expected[backwards?expected.size()-1-i:i];require(hit.hit->id==value.first&&hit.hit->match.start==value.second.start&&hit.hit->match.width==value.second.width,"Brute-force hit coordinates/order");cursor=MotifLocation{hit.hit->id,hit.hit->match.start};}}
    }
});
std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;}
