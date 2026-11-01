#include "core/composition.hpp"
#include "core/version.hpp"
#include "core/overview.hpp"
#include "core/statistics.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}require(caught,"Expected rejection");}
void near(double a,double b){require(std::abs(a-b)<1e-10,"Incorrect fractional value");}
Document fixture(){
    const std::vector<std::string> columns={"AAAA","AACC","AAAC","RARA","~-~-","N?N~","RYRY","AN~~","AG~~","UTTT"};
    std::vector<Record> rows(4);for(std::size_t r=0;r<4;++r){rows[r].name="row"+std::to_string(r+1);for(const auto& c:columns)rows[r].symbols+=c[r];}
    rows.push_back({"mask",std::string(15,'1'),RowKind::Mask});rows.push_back({"protein","DEFGHIKLMNPQ",RowKind::Protein});Document d;d.load(rows);return d;
}
std::vector<unsigned char> scan(const Document& d,const HighlightRules& rules,AnalysisScope scope={},std::size_t budget=1){SiteScan s(d,scope);std::size_t steps=0;while(!s.step(d,budget))require(++steps<10000,"Bounded scan must advance");return s.highlights(rules);}
void expected(const std::vector<unsigned char>& flags,std::initializer_list<std::size_t> columns){std::set<std::size_t> got;for(std::size_t c=0;c<flags.size();++c)if(flags[c])got.insert(c);require(got==std::set<std::size_t>(columns),"Incorrect highlighted columns");}
}
int main(){try{
    auto d=fixture();const auto revision=d.state_id();auto stats=alignment_statistics(d);require(stats.sequences==4&&stats.uniqueSequences==4&&stats.columns==10&&stats.patterns==9&&stats.constant==6&&stats.informative==1&&stats.singleton==3&&stats.gapOnly==1&&stats.nOnly==1,"Legacy nucleotide classifications retained");
    HighlightRules r;r.conserved=true;expected(scan(d,r),{0,3,7,9});r={};r.variable=true;expected(scan(d,r),{1,2,6,8});r.informative=true;r.variable=false;expected(scan(d,r),{1});r.singleton=true;expected(scan(d,r),{1,2,6,8});
    r={};r.conservation=true;r.conservationMin=100;expected(scan(d,r),{0,7,9});r.conservationMin=75;r.conservationMax=75;expected(scan(d,r),{2,3});
    r={};r.coverage=true;r.coverageMax=50;expected(scan(d,r),{4,7,8,10,11,12,13,14});r.informativeCoverage=true;r.coverageMax=0;expected(scan(d,r),{4,5,10,11,12,13,14});
    AnalysisScope scope;scope.selectedRows=true;scope.ids={1,4,5,6};scope.selectedColumns=true;scope.begin=1;scope.end=4;r={};r.variable=true;expected(scan(d,r,scope),{1,2});scope.ids={};expected(scan(d,r,scope),{});
    require(d.state_id()==revision&&!d.can_undo(),"Analyses are read-only");
    SiteScan stale(d,{});d.replace(1,0,1,"C");rejects([&]{stale.step(d);});d.undo();SiteScan wrong(d,{});auto other=fixture();rejects([&]{wrong.step(other);});
    // Compatibility and fractional conservation deliberately differ.
    NucleotideColumn c;c.add('R');c.add('a');c.add('n');c.add('-');require(c.constant()&&c.information==2&&c.residues==3,"Ambiguous/unknown states");near(c.conservation(),.75);
    for(char ch:std::string("ACGTURYSWKMBDHVN?-~")){NucleotideColumn column;column.add(ch);NucleotideEvidence e;e.add(ch);require(column.votes==e.bases&&column.residues==e.residues,"Shared overview and site votes");}
    Document proteins;proteins.load({{"p1","BDJZUOX*",RowKind::Protein},{"p2","NDLEUOX*",RowKind::Protein},{"mask","AAAAAAAA",RowKind::Mask},{"dna","CCCCCCCC",RowKind::Nucleotide}});scope={};scope.molecule=RowKind::Protein;r={};r.conserved=true;expected(scan(proteins,r,scope),{0,1,2,3,4,5,7});r={};r.conservation=true;r.conservationMin=100;expected(scan(proteins,r,scope),{1,4,5,7});
    require(protein_state('B')==(protein_state('D')|protein_state('N'))&&protein_state('Z')==(protein_state('E')|protein_state('Q'))&&protein_state('J')==(protein_state('I')|protein_state('L')),"Protein ambiguity definitions");
    Document composition;composition.load({{"fractional","aCrYbN?u-~"},{"short","A"},{"protein","AAAAAA",RowKind::Protein},{"mask","999999999999999",RowKind::Mask}});
    auto comp=nucleotide_composition(composition,{});require(comp.rows.size()==2&&comp.begin==0&&comp.end==10,"Composition excludes masks/proteins and their extensions");const auto& row=comp.rows[0];require(row.votes==std::array<std::uint64_t,4>{9,11,5,11}&&row.definite==3&&row.ambiguous==3&&row.unknown==2&&row.openGaps==1&&row.lockedGaps==1&&comp.rows[1].trailing==9,"Fractional counts, missing and gap accounting");require(comp.pooled.scored_units()==42,"Pooled counts sum rows");
    auto cells=composition_cells(comp.pooled);require(cells[10]=="7.000"&&cells[9]=="38.095","Pooled frequency uses pooled denominator");auto text=composition_tsv(comp);require(text.find(std::string("# Kleiotide version: ")+version+"\n")!=std::string::npos,"Composition TSV build version");require(text.find("fractional equal votes")!=std::string::npos&&text.find("Trailing missing")!=std::string::npos,"Export carries methods and missing counts");
    auto definite=nucleotide_composition(composition,{},false);require(definite.rows[0].votes==std::array<std::uint64_t,4>{6,6,0,6}&&definite.pooled.scored_units()==24,"Explicit exclusion of partial ambiguities");
    scope={};scope.selectedRows=true;scope.ids={1};scope.selectedColumns=true;scope.begin=1;scope.end=4;auto subset=nucleotide_composition(composition,scope);require(subset.rows.size()==1&&subset.pooled.scored_units()==18&&subset.begin==1&&subset.end==4,"Independent row and column scope");scope.ids={3,4};require(nucleotide_composition(composition,scope).rows.empty(),"No eligible selected rows never expands to all");
    scope.ids={2};scope.begin=12;scope.end=15;auto trailing=nucleotide_composition(composition,scope);require(trailing.rows[0].trailing==3&&trailing.pooled.scored_units()==0&&composition_cells(trailing.pooled)[5]=="N/A","Explicit columns beyond nucleotide lengths are missing, not zeros in denominator");
    rejects([&]{nucleotide_composition(composition,{},true,[]{return true;});});rejects([&]{composition_tsv(comp,[]{return true;});});
    // Independent small-alignment oracle, including ragged input and every IUPAC symbol.
    std::mt19937 random(19019);constexpr std::string_view alphabet="ACGTURYSWKMBDHVN?-~acgtryn";
    {Document proteins;proteins.load({{"a","AABX~DNJU",RowKind::Protein},{"b","AAC?~DNIO",RowKind::Protein},{"c","CCC*~NELO",RowKind::Protein},{"d","CCD?~NEJX",RowKind::Protein},{"nt",std::string(100,'A'),RowKind::Nucleotide}});
     const auto p=alignment_statistics(proteins,{},RowKind::Protein);
     require(p.sequences==4&&p.uniqueSequences==4&&p.columns==9&&p.patterns==6&&p.informative==4&&p.singleton==2&&p.constant==3&&p.gapOnly==1&&p.nOnly==2,"Protein IQ-TREE states, B/Z/J ambiguity, unknown U/O/stops, row filtering and distinct patterns");
     require(statistics_text(p).find("Protein")!=std::string::npos&&statistics_text(p).find("Unknown-only")!=std::string::npos,"Protein statistics labels");}
    for(int trial=0;trial<200;++trial){std::vector<Record> rows;const auto nr=1+random()%12,nc=1+random()%35;for(unsigned i=0;i<nr;++i){Record x{std::to_string(i),{}};const auto len=random()%(nc+1);for(unsigned j=0;j<len;++j)x.symbols+=alphabet[random()%alphabet.size()];rows.push_back(x);}Document input;input.load(rows);std::size_t width=input.columns();AlignmentStatistics oracle;oracle.sequences=nr;oracle.columns=width;std::set<std::vector<unsigned>> unique,patterns;for(const auto& x:rows){std::vector<unsigned> states;for(std::size_t col=0;col<width;++col)states.push_back(col<x.symbols.size()?nucleotide_state(x.symbols[col]):15);unique.insert(states);}oracle.uniqueSequences=unique.size();
        std::vector<unsigned char> expectedVariable(width),expectedConserved(width);for(std::size_t col=0;col<width;++col){unsigned intersection=15;unsigned counts[4]={};std::size_t nongap=0,information=0;std::vector<unsigned> pattern;for(const auto& x:rows){char raw=col<x.symbols.size()?x.symbols[col]:'~';const auto state=nucleotide_state(raw);pattern.push_back(state);intersection&=state;nongap+=!is_gap(raw);information+=state!=15;for(int base=0;base<4;++base)counts[base]+=state==(1U<<base);}patterns.insert(pattern);int repeated=0;for(auto n:counts)repeated+=n>=2;oracle.constant+=intersection!=0;oracle.informative+=repeated>=2;oracle.singleton+=!intersection&&repeated<2;oracle.gapOnly+=nongap==0;oracle.nOnly+=nongap&&!information;expectedVariable[col]=!intersection;expectedConserved[col]=intersection&&information;}
        oracle.patterns=patterns.size();const auto actual=alignment_statistics(input);require(statistics_text(actual)==statistics_text(oracle),"Independent statistics oracle");r={};r.variable=true;require(scan(input,r,{},7)==expectedVariable,"Random variable sites");r={};r.conserved=true;require(scan(input,r,{},11)==expectedConserved,"Random conserved sites");}
    std::cout<<"PASS site analysis: known-answer classes, scope, IUPAC/protein evidence, overview parity, composition, missing values, cancellation and 200 independent randomized statistics fixtures\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
