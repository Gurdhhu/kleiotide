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
unsigned at(const Sequence& row,std::size_t column) {
    return column<row.symbols.size() ? nucleotide_state(row.symbols.text()[column]) : 15;
}
}
AlignmentStatistics alignment_statistics(const Document& document,const std::function<bool()>& cancelled) {
    auto check=[&] { if (cancelled && cancelled()) throw std::runtime_error("Statistics cancelled."); };
    check(); AlignmentStatistics result;
    std::vector<const Sequence*> rows;
    for (const auto& row:document.rows()) if (row.kind==RowKind::Nucleotide) {
        rows.push_back(&row); result.columns=std::max(result.columns,row.symbols.size());
    }
    if(rows.size()>std::numeric_limits<std::uint32_t>::max()/6)throw std::length_error("Too many rows for nucleotide statistics.");
    result.sequences=rows.size();
    if (rows.empty()) return result;
    struct Column : NucleotideColumn {
        std::uint64_t hash=seed;
    };
    std::vector<Column> columns(result.columns);
    std::unordered_map<std::uint64_t,std::vector<std::size_t>> uniqueRows;
    uniqueRows.reserve(rows.size());
    std::array<unsigned char,128> states{};
    for (std::size_t c=0;c<states.size();++c) states[c]=static_cast<unsigned char>(nucleotide_state(static_cast<char>(c)));
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
            column.add(raw);
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
    text << "Total sequences: " << s.sequences << "; Unique sequences: " << s.uniqueSequences
         << "; Columns: " << s.columns << "; Distinct patterns: " << s.patterns
         << "; Parsimony-informative sites: " << s.informative << "; Singleton sites: " << s.singleton
         << "; Constant sites: " << s.constant << "; Gap-only sites: " << s.gapOnly << "; N-only sites: " << s.nOnly;
    return text.str();
}
}
