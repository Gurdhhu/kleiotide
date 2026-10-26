#include "core/row_selection.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace kleiotide {
namespace {
char upper(char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;}
unsigned bases(char c){
    if(c=='N'||c=='?')return 15;
    const auto votes=nucleotide_votes(c);unsigned result=0;
    for(unsigned i=0;i<4;++i)if(votes[i])result|=1u<<i;
    return result;
}
struct Pattern {
    std::string text;
    std::vector<std::size_t> prefix;
    std::array<std::vector<std::uint64_t>,128> masks;
    explicit Pattern(std::string symbols,bool compatible):text(std::move(symbols)){
        if(!compatible){prefix.assign(text.size(),0);for(std::size_t i=1,j=0;i<text.size();++i){while(j&&text[i]!=text[j])j=prefix[j-1];if(text[i]==text[j])++j;prefix[i]=j;}return;}
        const auto words=(text.size()+63)/64;
        for(auto& mask:masks)mask.assign(words,0);
        for(std::size_t i=0;i<text.size();++i)for(unsigned c=0;c<128;++c){
            const bool match=is_gap(text[i])?text[i]==static_cast<char>(c):
                (bases(text[i])&bases(static_cast<char>(c)))!=0;
            if(match)masks[c][i/64]|=std::uint64_t{1}<<(i%64);
        }
    }
    struct State {std::size_t matched=0;std::vector<std::uint64_t> bits;};
    bool advance(unsigned char c,State& state,bool compatible)const{
        if(!compatible){while(state.matched&&c!=static_cast<unsigned char>(text[state.matched]))state.matched=prefix[state.matched-1];
            if(c==static_cast<unsigned char>(text[state.matched]))++state.matched;
            if(state.matched!=text.size())return false;
            state.matched=prefix.back();return true;}
        if(state.bits.empty())state.bits.assign((text.size()+63)/64,0);
        std::uint64_t carry=1;
        for(std::size_t j=0;j<state.bits.size();++j){const auto next=state.bits[j]>>63;state.bits[j]=((state.bits[j]<<1)|carry)&(c<128?masks[c][j]:0);carry=next;}
        return (state.bits.back()&(std::uint64_t{1}<<((text.size()-1)%64)))!=0;
    }
};
struct Query {
    MotifOptions options;
    Pattern forward;
    std::optional<Pattern> reverse;
    static std::string prepare(std::string_view text,bool ignoreGaps){
        std::string result;
        for(unsigned char c:text){
            if(std::isspace(c))continue;
            if(!is_nucleotide_symbol(static_cast<char>(c)))throw std::invalid_argument("Motifs must contain nucleotide/IUPAC symbols or gaps.");
            const auto symbol=upper(c=='.'?'~':static_cast<char>(c));if(!ignoreGaps||!is_gap(symbol))result+=symbol;
            if(result.size()>4096)throw std::invalid_argument("Motifs are limited to 4096 symbols.");
        }
        if(result.empty())throw std::invalid_argument("Enter a motif containing at least one searchable symbol.");
        return result;
    }
    Query(std::string_view text,MotifOptions opts):options(opts),forward(prepare(text,opts.ignoreGaps),opts.compatibleIupac){
        if(!options.reverseComplement)return;
        auto rc=forward.text;std::reverse(rc.begin(),rc.end());
        const bool rna=rc.find('U')!=std::string::npos&&rc.find('T')==std::string::npos;
        for(auto& c:rc)c=rna&&c=='A'?'U':trace_complement(c);
        if(rc!=forward.text)reverse.emplace(std::move(rc),opts.compatibleIupac);
    }
    void scan(const Sequence& row,const std::function<bool(MotifMatch)>& visit,const std::function<bool()>& cancelled={},std::size_t firstColumn=0)const{
        if(row.kind!=RowKind::Nucleotide)return;
        Pattern::State f,r;std::vector<std::size_t> columns(forward.text.size());std::size_t ordinal=0;
        for(std::size_t column=firstColumn;column<row.symbols.size();++column){
            if((column&4095)==0&&cancelled&&cancelled())throw std::runtime_error("Motif search cancelled.");
            const auto c=upper(row.symbols.text()[column]);if(options.ignoreGaps&&is_gap(c))continue;
            columns[ordinal%columns.size()]=column;
            const bool positive=forward.advance(static_cast<unsigned char>(c),f,options.compatibleIupac);
            const bool negative=reverse?reverse->advance(static_cast<unsigned char>(c),r,options.compatibleIupac):positive&&options.reverseComplement;
            ++ordinal;
            if(positive||negative){const auto start=columns[(ordinal-columns.size())%columns.size()];
                if(!visit({start,column-start+1,!positive&&negative,positive&&negative}))return;}
        }
    }
    std::optional<MotifMatch> find(const Sequence& row)const{
        std::optional<MotifMatch> hit;scan(row,[&](auto match){hit=match;return false;});return hit;
    }
};
}
std::optional<MotifMatch> find_motif(const Sequence& row,std::string_view text,MotifOptions options){return Query(text,options).find(row);}
void scan_motifs(const Document& document,std::string_view text,MotifOptions options,const std::function<void(MotifHit)>& visit,
    const std::optional<std::vector<SequenceId>>& scope,const std::function<bool()>& cancelled){
    Query query(text,options);std::unordered_set<SequenceId> included;
    if(scope)for(auto id:*scope){(void)document.sequence(id);included.insert(id);}
    for(const auto& row:document.rows()){
        if(cancelled&&cancelled())throw std::runtime_error("Motif search cancelled.");
        if(scope&&!included.contains(row.id))continue;
        query.scan(row,[&](auto match){visit({row.id,match});return true;},cancelled);
    }
}
std::vector<SequenceId> select_motif(const Document& document,std::string_view text,MotifOptions options){
    Query query(text,options);std::vector<SequenceId> result;
    for(const auto& row:document.rows())if(query.find(row))result.push_back(row.id);
    return result;
}
MotifNavigation navigate_motif(const Document& document,std::string_view text,MotifOptions options,
    std::optional<MotifLocation> after,bool backwards,bool wrap,const std::optional<std::vector<SequenceId>>& scope,
    const std::function<bool()>& cancelled){
    Query query(text,options);std::unordered_set<SequenceId> included;
    if(scope)for(auto id:*scope){(void)document.sequence(id);included.insert(id);}
    std::optional<std::size_t> anchor;
    if(after){for(std::size_t i=0;i<document.rows().size();++i)if(document.rows()[i].id==after->id){anchor=i;break;}
        if(!anchor)throw std::invalid_argument("The previous motif hit is no longer in this document.");}
    auto search=[&](bool bounded)->std::optional<MotifHit>{
        for(std::size_t step=0;step<document.rows().size();++step){
            if(cancelled&&cancelled())throw std::runtime_error("Motif search cancelled.");
            const auto index=backwards?document.rows().size()-1-step:step;
            if(bounded&&anchor&&(backwards?index>*anchor:index<*anchor))continue;
            const auto& row=document.rows()[index];if(scope&&!included.contains(row.id))continue;
            const auto firstColumn=bounded&&anchor&&index==*anchor&&!backwards?
                (after->start<row.symbols.size()?after->start+1:row.symbols.size()):0;
            std::optional<MotifHit> hit;
            query.scan(row,[&](auto match){
                if(bounded&&anchor&&index==*anchor){
                    if(backwards&&match.start>=after->start)return false;
                    if(!backwards&&match.start<=after->start)return true;
                }
                hit=MotifHit{row.id,match};return backwards;
            },cancelled,firstColumn);
            if(hit)return hit;
        }
        return {};
    };
    if(auto hit=search(true))return {hit,false};
    if(after&&wrap)if(auto hit=search(false))return {hit,true};
    return {};
}
std::vector<SequenceId> select_duplicate_sequences(const Document& document,bool ignoreGaps,bool additionalOnly){
    std::unordered_map<std::string,std::vector<SequenceId>> groups;
    for(const auto& row:document.rows()){
        std::string key(1,static_cast<char>(row.kind));key+=static_cast<char>(row.representation);key.reserve(row.symbols.size()+2);
        for(auto c:row.symbols.text())if(!ignoreGaps||!is_gap(c))key+=upper(c);
        groups[std::move(key)].push_back(row.id);
    }
    std::unordered_set<SequenceId> matches;
    for(const auto& [key,ids]:groups)if(ids.size()>1)for(std::size_t i=additionalOnly?1:0;i<ids.size();++i)matches.insert(ids[i]);
    std::vector<SequenceId> result;for(const auto& row:document.rows())if(matches.contains(row.id))result.push_back(row.id);
    return result;
}
std::vector<SequenceId> select_random_rows(const Document& document,std::size_t count,std::uint64_t seed){
    if(count>document.rows().size())throw std::invalid_argument("The requested count exceeds the number of sequences.");
    std::vector<SequenceId> reservoir;reservoir.reserve(count);std::mt19937_64 generator(seed);
    for(std::size_t i=0;i<document.rows().size();++i){if(i<count)reservoir.push_back(document.rows()[i].id);
        else if(count){const auto slot=std::uniform_int_distribution<std::size_t>(0,i)(generator);if(slot<count)reservoir[slot]=document.rows()[i].id;}}
    const std::unordered_set<SequenceId> chosen(reservoir.begin(),reservoir.end());reservoir.clear();
    for(const auto& row:document.rows())if(chosen.contains(row.id))reservoir.push_back(row.id);
    return reservoir;
}
}
