#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace kleiotide {
struct GeneticCode {
    unsigned id=1;std::string name,aminoAcids;std::uint64_t contextStops=0;
    bool operator==(const GeneticCode&)const=default;
};
struct CodingRegion {
    std::size_t start=0,width=0;unsigned phase=1; // 0 = noncoding, 1/2/3 cyclic.
    bool operator==(const CodingRegion&)const=default;
};
struct TranslationSettings {
    unsigned code=1,frame=1;std::shared_ptr<const GeneticCode> custom;
    std::vector<CodingRegion> regions;
    bool operator==(const TranslationSettings& b)const {
        return code==b.code&&frame==b.frame&&regions==b.regions&&
            ((!custom&&!b.custom)||(custom&&b.custom&&*custom==*b.custom));
    }
};
struct ProteinSpan {
    std::array<std::size_t,3> columns{};unsigned cells=3;
    std::size_t first()const{return columns[0];}
    std::size_t last()const{return columns[cells-1];}
    bool operator==(const ProteinSpan&)const=default;
};
struct ProteinLayout {
    std::vector<ProteinSpan> spans; // Legacy input only; cleared when a record enters a Document.
    std::uint64_t sourceId=0,sourceFingerprint=0,sourceState=0;
    std::string sourceName;
    TranslationSettings settings;
    bool operator==(const ProteinLayout&)const=default;
};
std::size_t protein_storage_bytes(const std::shared_ptr<const ProteinLayout>&);
unsigned coding_phase(const TranslationSettings&,std::size_t column);
std::string translation_mask(const TranslationSettings&,std::size_t columns);
TranslationSettings annotate_from_mask(TranslationSettings,std::string_view mask,std::size_t columns);
TranslationSettings annotate_coding(TranslationSettings,std::size_t start,std::size_t width,int phase);
void validate_translation_settings(const TranslationSettings&);
const std::vector<GeneticCode>& genetic_codes();
const GeneticCode& genetic_code(unsigned id);
GeneticCode selected_genetic_code(const TranslationSettings&);
void validate_genetic_code(const GeneticCode&);
}
