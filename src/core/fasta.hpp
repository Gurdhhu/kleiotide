#pragma once
#include "core/alignment.hpp"
#include <functional>
#include <iosfwd>
namespace kleiotide {
enum class FastaImport { NucleotideAndMasks, ProteinAndMasks, Masks };
std::vector<Record> read_fasta(std::istream& input,const std::function<bool()>& cancelled = {},FastaImport type=FastaImport::NucleotideAndMasks);
// Zero (default) writes each complete sequence on one line.
void write_fasta_record(std::ostream&,std::string_view name,std::string_view symbols,std::size_t lineWidth=0);
void write_fasta(std::ostream& output, const Document& document, std::size_t lineWidth = 0);
}

