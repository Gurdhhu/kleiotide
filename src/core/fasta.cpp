#include "core/fasta.hpp"
#include <algorithm>
#include <istream>
#include <ostream>
#include <stdexcept>
namespace kleiotide {
namespace {
bool whitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
}
std::vector<Record> read_fasta(std::istream& input,const std::function<bool()>& cancelled,FastaImport type) {
    if(type!=FastaImport::NucleotideAndMasks&&type!=FastaImport::ProteinAndMasks&&type!=FastaImport::Masks)throw std::invalid_argument("Unknown FASTA import type.");
    std::vector<Record> result;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        if(cancelled&&cancelled())throw std::runtime_error("FASTA reading cancelled.");
        ++lineNumber;
        if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) line.erase(0, 3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || std::all_of(line.begin(), line.end(), whitespace)) continue;
        if (line.front() == '>') {
            auto header = line.substr(1);
            if (header.empty() || header.find('\0') != std::string::npos ||
                std::all_of(header.begin(), header.end(), whitespace))
                throw std::runtime_error("Empty FASTA header on line " + std::to_string(lineNumber) + ".");
            result.push_back({std::move(header), {},type==FastaImport::ProteinAndMasks?RowKind::Protein:type==FastaImport::Masks?RowKind::Mask:RowKind::Nucleotide});
            continue;
        }
        if (result.empty())
            throw std::runtime_error("Expected a FASTA header on line " + std::to_string(lineNumber) + ".");
        auto& symbols = result.back().symbols;
        for (std::size_t column = 0; column < line.size(); ++column) {
            const char c = line[column];
            if (whitespace(c)) continue;
            if (!is_mask_symbol(c))
                throw std::runtime_error("Invalid FASTA/mask symbol at line " +
                    std::to_string(lineNumber) + ", column " + std::to_string(column + 1) + ".");
            if(!is_row_symbol(c,result.back().kind))result.back().kind=RowKind::Mask;
            symbols.push_back(c == '.' ? '~' : c);
        }
    }
    if (input.bad() || (!input.eof() && input.fail())) throw std::runtime_error("Error reading FASTA.");
    if (result.empty()) throw std::runtime_error("The input contains no FASTA records.");
    return result;
}
void write_fasta_record(std::ostream& output,std::string_view name,std::string_view symbols,std::size_t lineWidth) {
    output << '>' << name << '\n';
    if(!lineWidth||symbols.empty())output << symbols << '\n';
    else for(std::size_t offset=0;offset<symbols.size();offset+=std::min(lineWidth,symbols.size()-offset))
        output << symbols.substr(offset,lineWidth) << '\n';
    if (!output) throw std::runtime_error("Error writing FASTA.");
}
void write_fasta(std::ostream& output, const Document& document, std::size_t lineWidth) {
    for(const auto& row:document.rows())write_fasta_record(output,row.name,row.symbols.text(),lineWidth);
    if (!output) throw std::runtime_error("Error writing FASTA.");
}
}

