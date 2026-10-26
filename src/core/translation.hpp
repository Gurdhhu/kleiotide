#pragma once
#include "core/alignment.hpp"
#include <functional>
#include <map>
namespace kleiotide {
enum CodonFlag : unsigned char { CodonGap=1,PartialGap=2,UnknownAA=4,DefiniteStop=8,PossibleStop=16,ContextStop=32 };
struct CodonCall {char amino='X';unsigned char flags=UnknownAA;};
class CodonLookup {
public:
    explicit CodonLookup(const GeneticCode&);
    CodonCall translate(char a,char b,char c)const;
private:std::array<CodonCall,4096> calls_{};
};
struct CodonMap {
    std::vector<ProteinSpan> codons;
    std::vector<std::int32_t> at; // -1 for noncoding or unresolved leading/invalid-phase positions.
    std::vector<unsigned char> phases,partial;
    std::size_t boundaryFragments=0,phaseErrors=0;
};
class CodonMapBuilder {
public:
    CodonMapBuilder(std::size_t columns,TranslationSettings);
    bool step(std::size_t budget=4096);
    const CodonMap& result()const{return map_;}
private:
    void finish_pending(bool boundary);
    TranslationSettings settings_;CodonMap map_;std::vector<std::size_t> pending_;
    std::size_t next_=0;bool started_=false,finished_=false,leading_=false;
};
struct AAProfile {
    std::array<std::uint64_t,27> counts{};std::uint64_t total=0;
    void add(char);
};
// Clustal-style residue colors, independent of column conservation/coverage.
// Zero is reserved for gaps/non-residues; ambiguous calls use neutral gray.
std::uint32_t amino_acid_color(char);
struct TranslationRow {SequenceId id=0;std::vector<CodonCall> calls;};
struct TranslationResult {
    std::uint64_t content=0;TranslationSettings settings;CodonMap map;
    std::vector<TranslationRow> rows;std::vector<AAProfile> profiles;
    std::size_t stops=0,possible=0,context=0,unknown=0,partialGaps=0;
    const TranslationRow* row(SequenceId)const;
};
class TranslationScan {
public:
    TranslationScan(const Document&,TranslationSettings,std::vector<SequenceId> ids={});
    bool step(const Document&,std::size_t budget=4096);
    const TranslationResult& result()const{return result_;}
private:
    CodonMapBuilder builder_;CodonLookup lookup_;TranslationResult result_;
    std::vector<SequenceId> ids_;std::size_t row_=0,codon_=0;bool built_=false,complete_=false;
};
struct ProteinProfiles {
    std::vector<AAProfile> ordinary;
    std::map<std::array<std::size_t,3>,AAProfile> spanned;
    std::size_t ordinaryRows=0,spannedRows=0;
    const AAProfile* profile(const Sequence&,std::size_t column)const;
};
class ProteinProfileScan {
public:
    explicit ProteinProfileScan(const Document&);
    bool step(const Document&,std::size_t budget=4096);
    const ProteinProfiles& result()const{return profiles_;}
private:ProteinProfiles profiles_;std::uint64_t content_=0;std::size_t row_=0,column_=0;bool complete_=false;
};
const ProteinSpan* protein_span(const Sequence&,std::size_t column);
bool complete_protein_selection(const Sequence&,std::size_t start,std::size_t count);
void validate_protein_layout(const Sequence&);
void normalize_legacy_protein(Record&);
std::string logical_protein(const Sequence&);
std::uint64_t sequence_fingerprint(const Sequence&);
struct TranslatedRecords {std::vector<Record> records;std::vector<SequenceId> sources;};
TranslatedRecords translated_records(const Document&,const TranslationResult&,bool compact);
}
