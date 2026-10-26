#include "core/mafft_template.hpp"
#include <algorithm>
#include <charconv>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace kleiotide {
namespace {
constexpr auto absent=std::numeric_limits<std::size_t>::max();
void check(const MafftAlignment::Cancel& cancel){if(cancel&&cancel())throw std::runtime_error("Alignment cancelled.");}
void skip_gaps(const std::string& text,std::size_t& position,const MafftAlignment::Cancel& cancel){
    while(position<text.size()&&is_gap(text[position])){if((position&4095)==0)check(cancel);++position;}
}
char comparable(char c,RowKind molecule){if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');return molecule==RowKind::Nucleotide&&c=='U'?'T':c;}
std::string reverse_symbols(const std::string& text){
    const bool rna=text.find_first_of("Uu")!=std::string::npos&&text.find_first_of("Tt")==std::string::npos;
    std::string result;result.reserve(text.size());
    for(auto it=text.rbegin();it!=text.rend();++it){char c=*it;const bool lower=c>='a'&&c<='z';const char upper=lower?static_cast<char>(c-'a'+'A'):c;
        const auto at=std::string_view("ACGTURYSWKMBDHVN").find(upper);
        if(at!=std::string_view::npos){c=rna&&upper=='A'?'U':std::string_view("TGCAAYRSWMKVHDBN")[at];if(lower)c=static_cast<char>(c-'A'+'a');}
        result.push_back(c);
    }return result;
}
}
MafftTemplateAlignment::MafftTemplateAlignment(const Document& source,MafftTemplateRequest request,Cancel cancel)
    :source_(&source),revision_(source.state_id()),originalWidth_(source.columns()){
    auto participants=request.targets;participants.insert(participants.end(),request.templates.begin(),request.templates.end());
    if(!participants.empty())molecule_=mafft_molecule(source,participants);
    std::unordered_map<SequenceId,std::size_t> indices;
    for(std::size_t i=0;i<source.rows().size();++i)indices.emplace(source.rows()[i].id,i);
    std::unordered_set<SequenceId> seen;
    auto collect=[&](const auto& ids,auto& rows){for(auto id:ids){check(cancel);
        const auto found=indices.find(id);if(found==indices.end()||!seen.insert(id).second)throw std::invalid_argument("Targets and templates must be distinct existing rows, without duplicates.");
        const auto& row=source.rows()[found->second];
        if(row.kind!=molecule_||!row.symbols.residue_count())throw std::invalid_argument("Choose targets and templates of one molecule type containing residues. Masks and empty rows cannot be included.");
        rows.push_back(found->second);
    }};
    collect(request.targets,targets_);collect(request.templates,templates_);
    if(targets_.empty()||templates_.empty())throw std::invalid_argument("Choose at least one target and one separate template.");
    std::vector<bool> occupied(originalWidth_,false);
    for(auto i:templates_){const auto& text=source.rows()[i].symbols.text();for(std::size_t c=0;c<text.size();++c){if((c&4095)==0)check(cancel);if(!is_gap(text[c]))occupied[c]=true;}}
    for(std::size_t c=0;c<occupied.size();++c){if((c&4095)==0)check(cancel);if(occupied[c])anchors_.push_back(c);}
}
void MafftTemplateAlignment::check_source()const{if(source_->state_id()!=revision_)throw std::logic_error("The source alignment changed during this job.");}
void MafftTemplateAlignment::write_templates(std::ostream& out,Cancel cancel)const{
    check_source();for(auto i:templates_){check(cancel);const auto& row=source_->rows()[i];const auto& text=row.symbols.text();out<<">kleio_"<<row.id<<'\n';std::size_t line=0;
        for(auto c:anchors_){if((line&4095)==0)check(cancel);out.put(c<text.size()&&!is_gap(text[c])?text[c]:'-');if(++line%80==0)out.put('\n');}if(line%80)out.put('\n');
    }if(!out)throw std::runtime_error("Cannot write MAFFT templates.");check_source();
}
void MafftTemplateAlignment::write_targets(std::ostream& out,Cancel cancel)const{
    check_source();for(auto i:targets_){check(cancel);const auto& row=source_->rows()[i];out<<">kleio_"<<row.id<<'\n';std::size_t line=0,steps=0;
        for(char c:row.symbols.text()){if((++steps&4095)==0)check(cancel);if(is_gap(c))continue;out.put(c);if(++line%80==0)out.put('\n');}if(line%80)out.put('\n');
    }if(!out)throw std::runtime_error("Cannot write MAFFT targets.");check_source();
}
MafftTemplateResult MafftTemplateAlignment::merge(std::vector<Record> output,Cancel cancel,bool allowReverse)const{
    check_source();if(molecule_==RowKind::Protein&&allowReverse)throw std::invalid_argument("Proteins cannot be reverse-complemented.");if(output.size()!=targets_.size()+templates_.size())throw std::runtime_error("MAFFT output has missing or extra target/template rows.");
    std::unordered_map<SequenceId,std::size_t> indices;for(auto i:targets_)indices.emplace(source_->rows()[i].id,i);for(auto i:templates_)indices.emplace(source_->rows()[i].id,i);
    std::vector<std::optional<std::string>> aligned(source_->rows().size());std::optional<std::size_t> width;
    std::unordered_set<std::size_t> reversed;
    for(auto& record:output){check(cancel);auto name=std::string_view(record.name);const bool reverse=name.starts_with("_R_");if(reverse)name.remove_prefix(3);if(!name.starts_with("kleio_"))throw std::runtime_error("Unknown row identifier in MAFFT template output.");name.remove_prefix(6);
        SequenceId id=0;const auto parsed=std::from_chars(name.data(),name.data()+name.size(),id);const auto found=indices.find(id);
        if(parsed.ec!=std::errc{}||parsed.ptr!=name.data()+name.size()||found==indices.end())throw std::runtime_error("Unknown row identifier in MAFFT template output.");
        const auto i=found->second;if(aligned[i]||(record.kind!=RowKind::Automatic&&record.kind!=molecule_)||!std::all_of(record.symbols.begin(),record.symbols.end(),[&](char c){return is_row_symbol(c,molecule_);}))throw std::runtime_error("Repeated row or unexpected symbols in MAFFT template output.");
        if(reverse){if(!allowReverse||std::find(targets_.begin(),targets_.end(),i)==targets_.end())throw std::runtime_error("MAFFT reversed a template or a target without permission; source unchanged.");reversed.insert(i);}
        if(width&&*width!=record.symbols.size())throw std::runtime_error("MAFFT output rows have unequal aligned lengths.");
        width=record.symbols.size();
        const auto original=reverse?reverse_symbols(source_->rows()[i].symbols.text()):source_->rows()[i].symbols.text();std::size_t ordinal=0,steps=0;
        for(auto& c:record.symbols){if((++steps&4095)==0)check(cancel);if(is_gap(c)){c='~';continue;}
            skip_gaps(original,ordinal,cancel);
            if(ordinal==original.size()||comparable(c,molecule_)!=comparable(original[ordinal],molecule_))throw std::runtime_error("MAFFT changed, lost or duplicated residues in row "+std::to_string(id)+".");
            c=original[ordinal++];
        }skip_gaps(original,ordinal,cancel);if(ordinal!=original.size())throw std::runtime_error("MAFFT truncated a target or template.");aligned[i]=std::move(record.symbols);
    }
    // Residue ordinals identify anchors even for repeated motifs and duplicate titles.
    std::vector<std::size_t> outputAnchor(*width,absent),anchorOutput(originalWidth_,absent);
    for(auto i:templates_){const auto& original=source_->rows()[i].symbols.text();std::size_t ordinal=0;
        for(std::size_t c=0;c<*width;++c){if((c&4095)==0)check(cancel);if(is_gap((*aligned[i])[c]))continue;skip_gaps(original,ordinal,cancel);
            if(outputAnchor[c]!=absent&&outputAnchor[c]!=ordinal)throw std::runtime_error("MAFFT merged distinct template columns; result cannot preserve the source alignment.");
            if(anchorOutput[ordinal]!=absent&&anchorOutput[ordinal]!=c)throw std::runtime_error("MAFFT split a template column; result cannot preserve the source alignment.");
            outputAnchor[c]=ordinal;anchorOutput[ordinal]=c;++ordinal;
        }
    }
    std::size_t last=0;bool first=true;for(auto a:anchors_){check(cancel);if(anchorOutput[a]==absent||(!first&&anchorOutput[a]<=last))throw std::runtime_error("MAFFT changed template column order; result cannot preserve the source alignment.");first=false;last=anchorOutput[a];}
    std::vector<bool> targetOccupied(*width,false);for(auto i:targets_)for(std::size_t c=0;c<*width;++c){if((c&4095)==0)check(cancel);if(!is_gap((*aligned[i])[c]))targetOccupied[c]=true;}
    MafftTemplateResult result;result.originalColumns.resize(originalWidth_);std::vector<std::size_t> outputColumns(*width,absent);std::size_t old=0,newColumn=0,interval=0;
    auto originals=[&](std::size_t end){while(old<end){if((old&4095)==0)check(cancel);result.originalColumns[old++]=newColumn++;}};
    auto insertions=[&](std::size_t end){for(auto c=interval;c<end;++c){if((c&4095)==0)check(cancel);if(targetOccupied[c]){outputColumns[c]=newColumn++;++result.insertedColumns;}}};
    for(std::size_t c=0;c<*width;++c){if((c&4095)==0)check(cancel);if(outputAnchor[c]==absent)continue;
        // Restore original gap intervals first; insertions sit immediately before
        // the next anchored column. Trailing insertions follow the full old axis.
        originals(outputAnchor[c]);insertions(c);outputColumns[c]=newColumn;originals(outputAnchor[c]+1);interval=c+1;
    }originals(originalWidth_);insertions(*width);
    std::unordered_set<std::size_t> targets(targets_.begin(),targets_.end());result.alignment.fragmentWidth=newColumn;result.alignment.records.reserve(source_->rows().size());
    for(std::size_t i=0;i<source_->rows().size();++i){check(cancel);const auto& row=source_->rows()[i];std::string symbols(newColumn,'~');
        if(targets.contains(i)){for(std::size_t c=0;c<*width;++c){if((c&4095)==0)check(cancel);if(outputColumns[c]!=absent)symbols[outputColumns[c]]=(*aligned[i])[c];}}
        else{const auto& text=row.symbols.text();for(std::size_t c=0;c<text.size();++c){if((c&4095)==0)check(cancel);symbols[result.originalColumns[c]]=text[c];}}
        if(reversed.contains(i))result.alignment.reversed.push_back(row.id);
        result.alignment.records.push_back({row.name,std::move(symbols),row.kind,reversed.contains(i)?reversed_trace(row.trace):row.trace,row.representation,row.groupId,row.protein});
    }check_source();return result;
}
}
