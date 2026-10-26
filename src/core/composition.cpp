#include "core/composition.hpp"
#include "core/version.hpp"
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
namespace kleiotide {
namespace {
std::string number(double n){std::ostringstream s;s.imbue(std::locale::classic());s<<std::fixed<<std::setprecision(3)<<n;return s.str();}
std::string safe(std::string s){for(auto& c:s)if(c=='\t'||c=='\r'||c=='\n')c=' ';return s;}
}
NucleotideComposition nucleotide_composition(const Document& d,AnalysisScope scope,bool fractional,const std::function<bool()>& cancelled){
    auto check=[&]{if(cancelled&&cancelled())throw std::runtime_error("Composition cancelled.");};check();
    NucleotideComposition result;scope.molecule=RowKind::Nucleotide;result.scope=scope;result.fractional=fractional;result.pooled.name="Pooled";
    const std::unordered_set<SequenceId> ids(scope.ids.begin(),scope.ids.end());std::size_t width=0;
    for(const auto& row:d.rows())if(row.kind==RowKind::Nucleotide&&(!scope.selectedRows||ids.contains(row.id)))width=std::max(width,row.symbols.size());
    // Explicit column scope may extend beyond nucleotide rows; report those
    // positions as trailing missing cells instead of silently shortening it.
    result.begin=scope.selectedColumns?std::min(scope.begin,d.columns()):0;
    result.end=scope.selectedColumns?std::clamp(scope.end,result.begin,d.columns()):width;
    for(const auto& row:d.rows()){
        check();if(row.kind!=RowKind::Nucleotide||(scope.selectedRows&&!ids.contains(row.id)))continue;
        CompositionRow counts;counts.id=row.id;counts.name=row.name;const auto& text=row.symbols.text();
        const auto last=std::min(result.end,text.size());
        counts.trailing=result.end-std::max(result.begin,last);
        for(auto c=result.begin;c<last;++c){
            if((c&4095)==0)check();
            const auto raw=text[c];
            if(raw=='-'){++counts.lockedGaps;continue;}if(is_gap(raw)){++counts.openGaps;continue;}
            const auto state=nucleotide_state(raw);if(state==15){++counts.unknown;continue;}
            const bool definite=std::has_single_bit(state);if(definite)++counts.definite;else ++counts.ambiguous;
            if(definite||fractional){const auto votes=nucleotide_votes(raw);for(std::size_t i=0;i<4;++i)counts.votes[i]+=votes[i];}
        }
        auto& p=result.pooled;for(std::size_t i=0;i<4;++i)p.votes[i]+=counts.votes[i];
        p.definite+=counts.definite;p.ambiguous+=counts.ambiguous;p.unknown+=counts.unknown;p.openGaps+=counts.openGaps;p.lockedGaps+=counts.lockedGaps;p.trailing+=counts.trailing;
        result.rows.push_back(std::move(counts));
    }
    return result;
}
std::vector<std::string> composition_headers(){return {"Sequence","A count","C count","G count","T/U count","A %","C %","G %","T/U %","GC %","Scored calls","Definite calls","Ambiguous calls","N/?","Open gaps","Locked gaps","Trailing missing"};}
std::vector<std::string> composition_cells(const CompositionRow& r){
    std::vector<std::string> cells{r.name};const auto total=r.scored_units();
    for(auto n:r.votes)cells.push_back(number(n/6.0));
    for(auto n:r.votes)cells.push_back(total?number(100.0*n/total):"N/A");
    cells.push_back(total?number(100.0*(r.votes[1]+r.votes[2])/total):"N/A");cells.push_back(number(total/6.0));
    for(auto n:{r.definite,r.ambiguous,r.unknown,r.openGaps,r.lockedGaps,r.trailing})cells.push_back(std::to_string(n));
    return cells;
}
std::string composition_tsv(const NucleotideComposition& s,const std::function<bool()>& cancelled){
    std::ostringstream out;out.imbue(std::locale::classic());
    out<<"# Nucleotide composition\n# Kleiotide version: "<<version<<"\n# Rows: "<<(s.scope.selectedRows?"explicit selected-name snapshot":"all nucleotide rows")<<"; sequences: "<<s.rows.size()<<"; columns (1-based inclusive): ";
    if(s.begin==s.end)out<<"empty";else out<<s.begin+1<<'-'<<s.end;
    out<<"\n# Partial IUPAC: "<<(s.fractional?"fractional equal votes":"excluded from scored calls")<<"; U = T. N/?, gaps and trailing cells excluded from frequency denominator.\n# A/C/G/T and GC percentages divide by scored calls. Pooled counts sum rows (not mean row percentages). Counts rounded to 3 decimals.\n";
    auto line=[&](const auto& cells){for(std::size_t i=0;i<cells.size();++i){if(i)out<<'\t';out<<safe(cells[i]);}out<<'\n';};
    line(composition_headers());for(const auto& r:s.rows){if(cancelled&&cancelled())throw std::runtime_error("Composition cancelled.");line(composition_cells(r));}line(composition_cells(s.pooled));return out.str();
}
}
