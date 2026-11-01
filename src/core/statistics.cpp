#include "core/statistics.hpp"
#include "core/site_summary.hpp"
#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace kleiotide {
namespace {
constexpr std::uint64_t seed=14695981039346656037ULL, multiplier=1099511628211ULL;
unsigned state(char c,RowKind molecule){if(molecule==RowKind::Nucleotide)return nucleotide_state(c);constexpr unsigned all=(1U<<20)-1;const auto value=protein_state(c)&all;return value?value:all;}
unsigned at(const Sequence& row,std::size_t column) {
    return state(column<row.symbols.size()?row.symbols.text()[column]:'~',row.kind);
}
}
AlignmentStatistics alignment_statistics(const Document& document,const std::function<bool()>& cancelled,RowKind molecule) {
    auto check=[&] { if (cancelled && cancelled()) throw std::runtime_error("Statistics cancelled."); };
    check(); AlignmentStatistics result;result.molecule=molecule;
    if(molecule!=RowKind::Nucleotide&&molecule!=RowKind::Protein)throw std::invalid_argument("Choose nucleotide or protein statistics.");
    std::vector<const Sequence*> rows;
    for (const auto& row:document.rows()) if (row.kind==molecule) {
        rows.push_back(&row); result.columns=std::max(result.columns,row.symbols.size());
    }
    if(rows.size()>std::numeric_limits<std::uint32_t>::max()/6)throw std::length_error("Too many rows for nucleotide statistics.");
    result.sequences=rows.size();
    if (rows.empty()) return result;
    struct Column {
        std::uint64_t hash=seed;
        unsigned intersection=0,seen=0,repeated=0;
        bool residues=false,information=false;
        bool informative()const{return std::popcount(repeated)>=2;}
    };
    const unsigned all=molecule==RowKind::Nucleotide?15:(1U<<20)-1;
    std::vector<Column> columns(result.columns);for(auto& column:columns)column.intersection=all;
    std::unordered_map<std::uint64_t,std::vector<std::size_t>> uniqueRows;
    uniqueRows.reserve(rows.size());
    std::array<unsigned,256> states{};
    for (std::size_t c=0;c<states.size();++c) states[c]=state(static_cast<char>(c),molecule);
    // One row-major scan, with only O(columns + sequences) additional storage.
    // Hash buckets retain representatives; collisions are verified exactly.
    for (std::size_t r=0;r<rows.size();++r) {
        check(); const auto& text=rows[r]->symbols.text(); std::uint64_t hash=seed;
        for (std::size_t c=0;c<result.columns;++c) {
            if ((c & 4095)==0) check();
            const char raw=c<text.size()?text[c]:'~';
            const auto value=states[static_cast<unsigned char>(raw)];
            hash=(hash^value)*multiplier;
            auto& column=columns[c]; column.hash=(column.hash^value)*multiplier;
            column.intersection &= value;
            column.residues |= !is_gap(raw);
            column.information |= value!=all;
            if(std::has_single_bit(value)){column.repeated |= column.seen&value;column.seen|=value;}
        }
        auto& representatives=uniqueRows[hash]; bool duplicate=false;
        for (auto other:representatives) {
            bool equal=true;
            for (std::size_t c=0;c<result.columns;++c) {
                if ((c & 4095)==0) check();
                if (at(*rows[r],c)!=at(*rows[other],c)) { equal=false; break; }
            }
            if (equal) { duplicate=true; break; }
        }
        if (!duplicate) { representatives.push_back(r); ++result.uniqueSequences; }
    }
    std::unordered_map<std::uint64_t,std::vector<std::size_t>> patterns;
    patterns.reserve(result.columns);
    for (std::size_t c=0;c<columns.size();++c) {
        check(); const auto& column=columns[c];
        const bool informative=column.informative();
        result.informative += informative;
        result.constant += column.intersection!=0;
        result.singleton += !informative && column.intersection==0;
        result.gapOnly += !column.residues;
        result.nOnly += column.residues && !column.information;
        auto& representatives=patterns[column.hash]; bool duplicate=false;
        for (auto other:representatives) {
            bool equal=true;
            for (std::size_t r=0;r<rows.size();++r) {
                if ((r & 4095)==0) check();
                if (at(*rows[r],c)!=at(*rows[r],other)) { equal=false; break; }
            }
            if (equal) { duplicate=true; break; }
        }
        if (!duplicate) { representatives.push_back(c); ++result.patterns; }
    }
    return result;
}
std::string statistics_text(const AlignmentStatistics& s) {
    std::ostringstream text;
    text << "Molecule: " << (s.molecule==RowKind::Protein?"Protein":"Nucleotide") << "; Total sequences: " << s.sequences << "; Unique sequences: " << s.uniqueSequences
         << "; Columns: " << s.columns << "; Distinct patterns: " << s.patterns
         << "; Parsimony-informative sites: " << s.informative << "; Singleton sites: " << s.singleton
         << "; Constant sites: " << s.constant << "; Gap-only sites: " << s.gapOnly << (s.molecule==RowKind::Protein?"; Unknown-only sites: ":"; N-only sites: ") << s.nOnly;
    return text.str();
}
}
