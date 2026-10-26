#include "core/mafft_alignment.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace kleiotide {
namespace {
void check_cancel(const MafftAlignment::Cancel& cancelled){if(cancelled&&cancelled())throw std::runtime_error("Alignment cancelled.");}
char upper(char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;}
char comparable(char c,RowKind molecule){c=upper(c);return molecule==RowKind::Nucleotide&&c=='U'?'T':c;}
char complement(char c,bool rna){
    const auto at=std::string_view("ACGTURYSWKMBDHVN").find(upper(c));
    if(at==std::string_view::npos)return c;
    char result=rna&&upper(c)=='A'?'U':std::string_view("TGCAAYRSWMKVHDBN")[at];
    return c>='a'&&c<='z'?static_cast<char>(result-'A'+'a'):result;
}
std::string number(double n){std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(10)<<n;return out.str();}
}
RowKind mafft_molecule(const Document& source,const std::vector<SequenceId>& ids){
    std::optional<RowKind> molecule;
    auto include=[&](const Sequence& row){if(row.kind==RowKind::Mask)return;
        if(molecule&&*molecule!=row.kind)throw std::invalid_argument("MAFFT cannot mix nucleotide and protein participants. Select rows of one type, or choose templates of the same type.");
        molecule=row.kind;
    };
    if(ids.empty()){for(const auto& row:source.rows())include(row);}else for(auto id:ids)include(source.sequence(id));
    if(!molecule)throw std::invalid_argument("Choose nucleotide or protein sequences; masks cannot be aligned.");
    return *molecule;
}
std::vector<std::string> MafftOptions::arguments() const {
    if(molecule!=RowKind::Nucleotide&&molecule!=RowKind::Protein)throw std::invalid_argument("MAFFT requires a nucleotide or protein molecule type.");
    if(molecule==RowKind::Protein&&direction!=MafftDirection::Keep)throw std::invalid_argument("Protein MAFFT cannot adjust nucleotide strands.");
    if(threads==0||threads<-1||threads>256||iterations<0||iterations>1000||retree<1||retree>10||kimura<1||kimura>1000||!std::isfinite(gapOpen)||gapOpen<0||gapOpen>100||!std::isfinite(offset)||offset<0||offset>100||!std::isfinite(kappa)||kappa<=0||kappa>100)throw std::invalid_argument("Invalid MAFFT parameter.");
    std::vector<std::string> args={molecule==RowKind::Protein?"--amino":"--nuc","--preservecase","--inputorder","--thread",std::to_string(threads),"--op",number(gapOpen),"--ep",number(offset)};
    if(molecule==RowKind::Nucleotide)args.insert(args.end(),{"--kimura",std::to_string(kimura),"--kappa",number(kappa)});
    else{
        // Retain U/O, stops and unknown calls; MAFFT scores these as unknown.
        args.push_back("--anysymbol");
        switch(proteinScoring){
        case ProteinScoring::Blosum:if(blosum!=30&&blosum!=45&&blosum!=62&&blosum!=80)throw std::invalid_argument("Choose BLOSUM 30, 45, 62 or 80.");args.insert(args.end(),{"--bl",std::to_string(blosum)});break;
        case ProteinScoring::Jtt:case ProteinScoring::Transmembrane:if(proteinPam<1||proteinPam>1000)throw std::invalid_argument("Protein PAM distance must be 1–1000.");args.insert(args.end(),{proteinScoring==ProteinScoring::Jtt?"--jtt":"--tm",std::to_string(proteinPam)});break;
        default:throw std::invalid_argument("Unknown protein scoring model.");
        }
    }
    switch(algorithm){
    case MafftAlgorithm::Auto:args.push_back("--auto");break;
    case MafftAlgorithm::Fast1:args.insert(args.end(),{"--retree","1","--maxiterate","0"});break;
    case MafftAlgorithm::Fast2:args.insert(args.end(),{"--retree","2","--maxiterate","0"});break;
    case MafftAlgorithm::PartTree:args.insert(args.end(),{"--parttree","--retree","1","--maxiterate","0"});break;
    case MafftAlgorithm::Iterative:args.insert(args.end(),{"--retree",std::to_string(retree),"--maxiterate",std::to_string(iterations)});break;
    case MafftAlgorithm::Local:args.insert(args.end(),{"--localpair","--maxiterate",std::to_string(iterations)});break;
    case MafftAlgorithm::Global:args.insert(args.end(),{"--globalpair","--maxiterate",std::to_string(iterations)});break;
    case MafftAlgorithm::LargeGaps:args.insert(args.end(),{"--genafpair","--maxiterate",std::to_string(iterations)});break;
    default:throw std::invalid_argument("Unknown MAFFT algorithm.");
    }
    switch(direction){case MafftDirection::Keep:break;case MafftDirection::Adjust:args.push_back("--adjustdirection");break;case MafftDirection::Accurate:args.push_back("--adjustdirectionaccurately");break;default:throw std::invalid_argument("Unknown strand setting.");}
    if(composition)args.push_back("--fmodel");
    if(memorySaving)args.push_back("--memsave");
    if(deterministic)args.insert(args.end(),{"--threadit","0"});
    return args;
}
MafftAlignment::MafftAlignment(const Document& source,std::optional<Document::ColumnRange> range,Cancel cancelled):molecule_(mafft_molecule(source)),source_(&source),revision_(source.state_id()),range_(range){
    if(range&&(!range->width||range->start>source.columns()||range->width>source.columns()-range->start))throw std::invalid_argument("Select a nonempty alignment column range.");
    for(std::size_t i=0;i<source.rows().size();++i){check_cancel(cancelled);const auto& row=source.rows()[i];
        if(row.kind==RowKind::Mask){++masks_;if(range_)minimumWidth_=std::max(minimumWidth_,region(row).size());continue;}
        const auto text=region(row);if(std::any_of(text.begin(),text.end(),[](char c){return !is_gap(c);}))inputs_.push_back(i);
    }
    if(inputs_.size()<2)throw std::invalid_argument("MAFFT needs at least two rows of the same molecule type containing residues in this scope. Masks and empty rows are excluded.");
}
void MafftAlignment::check_source()const {if(source_->state_id()!=revision_)throw std::logic_error("The source alignment changed during this job.");}
std::string_view MafftAlignment::region(const Sequence& row)const {
    const auto text=std::string_view(row.symbols.text());if(!range_)return text;
    const auto begin=std::min(range_->start,text.size());return text.substr(begin,std::min(range_->width,text.size()-begin));
}
void MafftAlignment::write_input(std::ostream& out,Cancel cancelled)const {
    check_source();for(auto i:inputs_){const auto& row=source_->rows()[i];out<<">kleio_"<<row.id<<'\n';std::size_t line=0,steps=0;
        for(char c:region(row)){if((++steps&4095)==0)check_cancel(cancelled);if(is_gap(c))continue;out.put(c);if(++line==80){out.put('\n');line=0;}}
        if(line)out.put('\n');
        check_cancel(cancelled);
    }if(!out)throw std::runtime_error("Cannot write MAFFT input.");
}
MafftResult MafftAlignment::merge(std::vector<Record> output,bool allowReverse,Cancel cancelled)const {
    check_source();if(molecule_==RowKind::Protein&&allowReverse)throw std::invalid_argument("Proteins cannot be reverse-complemented.");if(range_&&allowReverse)throw std::invalid_argument("Column realignment must keep the original strand.");
    if(output.size()!=inputs_.size())throw std::runtime_error("MAFFT output has missing or extra rows.");
    std::unordered_map<SequenceId,std::size_t> index;for(auto i:inputs_)index.emplace(source_->rows()[i].id,i);
    std::vector<std::optional<std::string>> aligned(source_->rows().size());MafftResult result;std::optional<std::size_t> width;
    for(auto& record:output){check_cancel(cancelled);auto name=std::string_view(record.name);bool reversed=name.starts_with("_R_");if(reversed)name.remove_prefix(3);
        if(reversed&&!allowReverse)throw std::runtime_error("MAFFT reversed a row without permission.");
        if(!name.starts_with("kleio_"))throw std::runtime_error("Unrecognized row identifier in MAFFT output.");
        name.remove_prefix(6);
        SequenceId id=0;const auto parsed=std::from_chars(name.data(),name.data()+name.size(),id);
        const auto found=index.find(id);if(parsed.ec!=std::errc{}||parsed.ptr!=name.data()+name.size()||found==index.end())throw std::runtime_error("Unknown row identifier in MAFFT output.");
        const auto i=found->second;if(aligned[i])throw std::runtime_error("Repeated row in MAFFT output.");
        if((record.kind!=RowKind::Automatic&&record.kind!=molecule_)||!std::all_of(record.symbols.begin(),record.symbols.end(),[&](char c){return is_row_symbol(c,molecule_);}))throw std::runtime_error("Unexpected symbols in MAFFT output.");
        if(width&&*width!=record.symbols.size())throw std::runtime_error("MAFFT output rows have unequal aligned lengths.");
        width=record.symbols.size();
        const auto original=region(source_->rows()[i]);std::size_t ordinal=reversed?original.size():0,steps=0;
        const bool rna=original.find_first_of("Uu")!=std::string_view::npos&&original.find_first_of("Tt")==std::string_view::npos;
        for(auto& c:record.symbols){if((++steps&4095)==0)check_cancel(cancelled);if(is_gap(c)){c='~';continue;}char expected=0;
            if(reversed){while(ordinal&&is_gap(original[ordinal-1]))--ordinal;if(ordinal)expected=complement(original[--ordinal],rna);}
            else{while(ordinal<original.size()&&is_gap(original[ordinal]))++ordinal;if(ordinal<original.size())expected=original[ordinal++];}
            if(!expected||comparable(c,molecule_)!=comparable(expected,molecule_))throw std::runtime_error("MAFFT changed, lost or duplicated residues in row "+std::to_string(id)+".");
            c=expected;
        }
        if(reversed){while(ordinal&&is_gap(original[ordinal-1]))--ordinal;if(ordinal)throw std::runtime_error("MAFFT truncated a row.");result.reversed.push_back(id);}
        else{while(ordinal<original.size()&&is_gap(original[ordinal]))++ordinal;if(ordinal!=original.size())throw std::runtime_error("MAFFT truncated a row.");}
        aligned[i]=std::move(record.symbols);
    }
    const auto newWidth=std::max(width.value_or(0),minimumWidth_);result.fragmentWidth=newWidth;result.records.reserve(source_->rows().size());
    for(std::size_t i=0;i<source_->rows().size();++i){check_cancel(cancelled);const auto& row=source_->rows()[i];std::string symbols;
        if(!range_){if(row.kind==RowKind::Mask)symbols=row.symbols.text();else if(aligned[i])symbols=std::move(*aligned[i]);else symbols.assign(newWidth,'~');}
        else{const auto& text=row.symbols.text();const auto begin=range_->start,end=begin+range_->width;
            if(row.kind==RowKind::Mask&&text.size()<=begin)symbols=text;
            else{symbols=text.substr(0,begin);if(symbols.size()<begin)symbols.append(begin-symbols.size(),'~');
                auto fragment=row.kind==RowKind::Mask?std::string(region(row)):aligned[i]?std::move(*aligned[i]):std::string{};fragment.resize(newWidth,'~');symbols+=fragment;
                if(text.size()>end)symbols.append(text,end,std::string::npos);
            }
        }const bool reversed=std::find(result.reversed.begin(),result.reversed.end(),row.id)!=result.reversed.end();
        result.records.push_back({row.name,std::move(symbols),row.kind,reversed?reversed_trace(row.trace):row.trace,row.representation,row.groupId,row.protein});
    }check_source();return result;
}
}
