#include "gui/motif_search.hpp"
#include <QRegularExpression>
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <unordered_set>
namespace kleiotide {
namespace {
// Two rank bitmaps classify arbitrary overlapping intervals in constant time.
// Prefix totals per 64 symbols avoid rescanning long N stretches for every hit.
class NOnlyFilter {
    struct Block {std::uint64_t n=0,other=0;std::size_t beforeN=0,beforeOther=0;};
    std::vector<Block> blocks_;
    std::size_t size_=0,totalN_=0,totalOther_=0;
    std::pair<std::size_t,std::size_t> rank(std::size_t end)const{
        if(end==size_)return {totalN_,totalOther_};
        const auto& b=blocks_[end/64];const auto mask=(std::uint64_t{1}<<(end%64))-1;
        return {b.beforeN+std::popcount(b.n&mask),b.beforeOther+std::popcount(b.other&mask)};
    }
public:
    void build(std::string_view symbols,const std::function<void()>& check){
        blocks_.clear();size_=symbols.size();totalN_=totalOther_=0;
        if((size_/64+(size_%64!=0))>128*1024*1024/sizeof(Block))
            throw std::runtime_error("N-only filter exceeds its 128 MiB working budget. Use a smaller scope or disable the filter.");
        blocks_.reserve(size_/64+(size_%64!=0));
        for(std::size_t c=0;c<size_;++c){if((c&4095)==0)check();
            if(c%64==0)blocks_.push_back({0,0,totalN_,totalOther_});
            const char symbol=symbols[c];const auto bit=std::uint64_t{1}<<(c%64);
            if(symbol=='N'||symbol=='n'){blocks_.back().n|=bit;++totalN_;}
            else if(!is_gap(symbol)){blocks_.back().other|=bit;++totalOther_;}
        }
    }
    bool excludes(std::size_t start,std::size_t width)const{
        const auto a=rank(start),b=rank(start+width);
        return b.first>a.first&&b.second==a.second;
    }
};
}
MotifSearchResult search_motifs(const Document& document,std::string_view text,MotifOptions options,bool regex,
    const std::optional<std::vector<SequenceId>>& scope,const std::function<bool()>& cancelled,bool excludeNOnly){
    auto check=[&]{if(cancelled&&cancelled())throw std::runtime_error("Motif search cancelled.");};check();
    auto index=std::make_shared<MotifIndex>();MotifSearchResult result;
    NOnlyFilter filter;
    if(!regex){std::optional<SequenceId> filteredRow;
        scan_motifs(document,text,options,[&](auto hit){
            if(excludeNOnly){if(filteredRow!=hit.id){filter.build(document.sequence(hit.id).symbols.text(),check);filteredRow=hit.id;}
                if(filter.excludes(hit.match.start,hit.match.width))return;}
            index->add(hit);
        },scope,cancelled);index->finish();result.index=index;return result;}
    if(text.empty()||text.size()>4096)throw std::invalid_argument("Enter a regular expression of 1–4096 characters.");
    // Bound individual PCRE2 calls and distinguish engine errors from no match.
    const QString limits="(*NO_JIT)(*LIMIT_MATCH=1000000)(*LIMIT_DEPTH=1000)(*LIMIT_HEAP=8192)";
    const QRegularExpression expression(limits+QString::fromUtf8(text.data(),static_cast<qsizetype>(text.size())),QRegularExpression::CaseInsensitiveOption);
    if(!expression.isValid())throw std::invalid_argument(("Invalid regular expression: "+expression.errorString()).toStdString());
    std::unordered_set<SequenceId> included;if(scope)for(auto id:*scope){(void)document.sequence(id);included.insert(id);}
    for(const auto& row:document.rows()){
        check();if(row.kind!=RowKind::Nucleotide||(scope&&!included.contains(row.id)))continue;
        if(row.symbols.size()>16000000)throw std::runtime_error("Regex row exceeds the 16-million-column working limit. Use literal search or a smaller scope.");
        if(excludeNOnly)filter.build(row.symbols.text(),check);
        QString subject;subject.reserve(static_cast<qsizetype>(row.symbols.size()));std::vector<std::size_t> columns;columns.reserve(row.symbols.size());
        for(std::size_t c=0;c<row.symbols.size();++c){if((c&4095)==0)check();const auto symbol=row.symbols.text()[c];if(!options.ignoreGaps||!is_gap(symbol)){subject+=QChar::fromLatin1(symbol);columns.push_back(c);}}
        std::vector<MotifMatch> hits;
        auto scan=[&](const QString& input,bool reversed){
            // Subjects are constructed solely from validated ASCII symbols.
            // Avoid revalidating the entire UTF-16 subject for every overlap.
            for(qsizetype offset=0;offset<=input.size();){check();const auto match=expression.match(input,offset,QRegularExpression::NormalMatch,QRegularExpression::DontCheckSubjectStringMatchOption);
                if(!match.isValid())throw std::runtime_error("Regular expression exceeded engine resource limits. Simplify the pattern; no partial results were applied.");
                if(!match.hasMatch())break;
                const auto start=match.capturedStart(),length=match.capturedLength();
                if(length){const auto first=reversed?input.size()-start-length:start;const auto last=first+length-1;
                    const auto begin=columns[static_cast<std::size_t>(first)],width=columns[static_cast<std::size_t>(last)]-begin+1;
                    if(!excludeNOnly||!filter.excludes(begin,width))hits.push_back({begin,width,reversed,false});
                    if(hits.size()>128*1024*1024/sizeof(MotifMatch))throw std::runtime_error("Regex hits exceed the per-row working budget. Refine the expression or scope; no partial results were applied.");
                }else ++result.emptyMatches;
                offset=start+1;
            }
        };
        scan(subject,false);
        if(options.reverseComplement){const bool rna=subject.contains('U',Qt::CaseInsensitive)&&!subject.contains('T',Qt::CaseInsensitive);
            std::reverse(subject.begin(),subject.end());for(qsizetype c=0;c<subject.size();++c){if((c&4095)==0)check();const char base=subject[c].toLatin1();subject[c]=QChar::fromLatin1(rna&&(base=='A'||base=='a')?(base=='a'?'u':'U'):trace_complement(base));}
            scan(subject,true);}
        check();std::sort(hits.begin(),hits.end(),[](const auto& a,const auto& b){return a.start<b.start||(a.start==b.start&&a.width<b.width);});
        for(std::size_t i=0;i<hits.size();){if((i&4095)==0)check();auto hit=hits[i++];
            while(i<hits.size()&&hits[i].start==hit.start&&hits[i].width==hit.width){hit.bothStrands=hit.bothStrands||hits[i].reversed!=hit.reversed;++i;}
            if(hit.bothStrands)hit.reversed=false;
            index->add({row.id,hit});}
    }
    check();index->finish();result.index=index;return result;
}
}
