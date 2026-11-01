#include "core/session.hpp"
#include "core/translation.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>
#include <stdexcept>
namespace kleiotide {
namespace {
constexpr std::uint64_t maxBytes=8ULL*1024*1024*1024;
constexpr std::uint64_t maxCalls=16*1024*1024;
std::uint32_t checksum(std::uint32_t crc,const char* bytes,std::size_t count){
    static const auto table=[]{std::array<std::uint32_t,256> t{};for(std::uint32_t i=0;i<256;++i){auto c=i;for(int j=0;j<8;++j)c=(c>>1)^((c&1)?0xedb88320U:0U);t[i]=c;}return t;}();
    for(std::size_t i=0;i<count;++i)crc=table[(crc^static_cast<unsigned char>(bytes[i]))&255]^(crc>>8);
    return crc;
}
class Writer {
public:
    Writer(std::ostream& output,const std::function<bool()>& cancelled):out(output),cancelled_(cancelled){}
    void bytes(const char* data,std::size_t n){
        if(n>maxBytes-total)throw std::runtime_error("Session exceeds the 8 GiB limit.");
        while(n){
            if(cancelled_&&cancelled_())throw std::runtime_error("Session write cancelled.");
            const auto take=std::min(n,buffer.size());
            crc=checksum(crc,data,take);total+=take;
            if(take>buffer.size()-used)flush();
            std::copy_n(data,take,buffer.data()+used);used+=take;data+=take;n-=take;
            if(!out)throw std::runtime_error("Could not write session.");
        }
    }
    void number(std::uint64_t value,unsigned width=8){char b[8];for(unsigned i=0;i<width;++i)b[i]=static_cast<char>(value>>(i*8));bytes(b,width);}
    void text(std::string_view s,std::uint64_t limit){if(s.size()>limit)throw std::runtime_error("Session text exceeds its field limit.");number(s.size());bytes(s.data(),s.size());}
    void finish(){flush();const auto c=crc^0xffffffffU;char b[4];for(unsigned i=0;i<4;++i)b[i]=static_cast<char>(c>>(i*8));out.write(b,4);if(!out)throw std::runtime_error("Could not finish session.");}
private:
    void flush(){if(used){out.write(buffer.data(),static_cast<std::streamsize>(used));used=0;}}
    std::array<char,65536> buffer{};std::size_t used=0;
    std::ostream& out;const std::function<bool()>& cancelled_;std::uint32_t crc=0xffffffffU;std::uint64_t total=0;
};
class Reader {
public:
    explicit Reader(std::istream& input):in(input){}
    void bytes(char* data,std::size_t n){if(n>maxBytes-total)throw std::runtime_error("Session exceeds the 8 GiB limit.");in.read(data,static_cast<std::streamsize>(n));if(!in)throw std::runtime_error("Truncated session.");crc=checksum(crc,data,n);total+=n;}
    std::uint64_t number(unsigned width=8){char b[8];bytes(b,width);std::uint64_t value=0;for(unsigned i=0;i<width;++i)value|=static_cast<std::uint64_t>(static_cast<unsigned char>(b[i]))<<(i*8);return value;}
    std::size_t count(std::uint64_t limit){const auto n=number();if(n>limit||n>std::numeric_limits<std::size_t>::max())throw std::runtime_error("Invalid session field length.");return static_cast<std::size_t>(n);}
    std::string text(std::uint64_t limit){const auto n=count(limit);std::string result;std::array<char,65536> block{};
        for(std::size_t i=0;i<n;){const auto take=std::min(block.size(),n-i);bytes(block.data(),take);result.append(block.data(),take);i+=take;}return result;}
    void finish(){char b[4];in.read(b,4);if(!in)throw std::runtime_error("Missing session checksum.");std::uint32_t c=0;for(unsigned i=0;i<4;++i)c|=static_cast<std::uint32_t>(static_cast<unsigned char>(b[i]))<<(i*8);
        if(c!=(crc^0xffffffffU))throw std::runtime_error("Session checksum mismatch.");
        if(in.get()!=std::char_traits<char>::eof()||in.bad())throw std::runtime_error("Unexpected bytes after session.");}
private:
    std::istream& in;std::uint32_t crc=0xffffffffU;std::uint64_t total=0;
};
constexpr auto small=1024*1024;
void write_settings(Writer& w,const TranslationSettings& s){validate_translation_settings(s);w.number(s.code,4);w.number(s.frame,1);w.number(s.custom?1:0,1);if(s.custom){w.text(s.custom->name,1024);w.text(s.custom->aminoAcids,64);w.number(s.custom->contextStops);}w.number(s.regions.size());for(auto r:s.regions){w.number(r.start);w.number(r.width);w.number(r.phase,1);}}
TranslationSettings read_settings(Reader& r){TranslationSettings s;s.code=static_cast<unsigned>(r.number(4));s.frame=static_cast<unsigned>(r.number(1));auto custom=r.number(1);if(custom>1)throw std::runtime_error("Invalid custom code flag.");if(custom){auto c=std::make_shared<GeneticCode>();c->id=0;c->name=r.text(1024);c->aminoAcids=r.text(64);c->contextStops=r.number();validate_genetic_code(*c);s.custom=c;}auto n=r.count(100000);for(std::size_t i=0;i<n;++i){auto start=r.count(1073741824),width=r.count(1073741824);auto p=static_cast<unsigned>(r.number(1));s.regions.push_back({start,width,p});}validate_translation_settings(s);return s;}
void write_protein(Writer& w,const std::shared_ptr<const ProteinLayout>& p){w.number(p?1:0,1);if(!p)return;if(p->spans.size()>maxCalls)throw std::runtime_error("Too many protein spans in one sequence.");w.number(0,1); // Reserved legacy display flag.
    w.number(p->sourceId);w.number(p->sourceFingerprint);w.number(p->sourceState);w.text(p->sourceName,small);write_settings(w,p->settings);w.number(p->spans.size());for(auto span:p->spans){w.number(span.cells,1);for(auto c:span.columns)w.number(c);}}
std::shared_ptr<const ProteinLayout> read_protein(Reader& r){auto present=r.number(1);if(present>1)throw std::runtime_error("Invalid protein metadata flag.");if(!present)return {};auto p=std::make_shared<ProteinLayout>();auto three=r.number(1);if(three>1)throw std::runtime_error("Invalid amino-acid label style.");p->sourceId=r.number();p->sourceFingerprint=r.number();p->sourceState=r.number();p->sourceName=r.text(small);p->settings=read_settings(r);auto n=r.count(maxCalls);for(std::size_t i=0;i<n;++i){ProteinSpan s;s.cells=static_cast<unsigned>(r.number(1));for(auto& c:s.columns)c=r.count(1073741824);p->spans.push_back(s);}return p;}

}
void write_session(std::ostream& output,const Document& d,std::string_view view,std::string_view report,const std::function<bool()>& cancelled){
    if(cancelled&&cancelled())throw std::runtime_error("Session write cancelled.");
    std::map<const Chromatogram*,std::size_t> indices;std::vector<std::shared_ptr<const Chromatogram>> traces;
    for(const auto& row:d.rows()){
        if(row.kind!=RowKind::Nucleotide&&row.kind!=RowKind::Mask&&row.kind!=RowKind::Protein)throw std::runtime_error("Invalid session row type.");
        validate_protein_layout(row);
        validate_trace(row);if(row.trace&&!indices.contains(row.trace->original.get())){indices[row.trace->original.get()]=traces.size();traces.push_back(row.trace->original);}}
    if(d.rows().size()>1000000||traces.size()>1000000)throw std::runtime_error("Too many session rows or traces.");
    Writer w(output,cancelled);w.bytes(session_magic.data(),session_magic.size());w.number(session_version,4);w.number(d.next_sequence_id());w.text(view,small);w.text(report,16*small);w.number(d.groups().size());w.number(d.next_group_id());
    for(const auto& g:d.groups()){w.number(g.id);w.text(g.name,small);w.number(g.color,4);}write_settings(w,d.translation_settings());w.number(traces.size());
    for(const auto& ptr:traces){const auto& t=*ptr;w.text(t.sourcePath,small);w.text(t.sampleName,small);w.number(t.callSet,4);w.text(t.calls,maxCalls);
        w.number(t.peaks.size());for(auto p:t.peaks)w.number(p,4);w.number(t.quality.size());w.bytes(reinterpret_cast<const char*>(t.quality.data()),t.quality.size());
        for(const auto& channel:t.channels){if(channel.size()>maxCalls)throw std::runtime_error("Too many trace samples.");w.number(channel.size());for(auto v:channel)w.number(std::bit_cast<std::uint32_t>(v),4);}
    }
    w.number(d.rows().size());for(const auto& row:d.rows()){
        w.number(row.id);w.number(static_cast<unsigned>(row.kind),1);w.number(static_cast<unsigned>(row.representation),1);w.number(row.groupId);w.text(row.name,small);w.text(row.symbols.text(),1024ULL*1024*1024);
        w.number(row.trace?indices.at(row.trace->original.get())+1:0);
        if(row.trace){w.number(row.trace->reversed?1:0,1);w.number(row.trace->originalCalls.size());for(auto a:row.trace->originalCalls)w.number(std::bit_cast<std::uint32_t>(a),4);w.number(row.trace->inferredPeaks.size());for(auto peak:row.trace->inferredPeaks)w.number(std::bit_cast<std::uint32_t>(peak),4);}
    write_protein(w,row.protein);
    }w.finish();
}
Session read_session(std::istream& input){
    Reader r(input);std::array<char,8> magic{};r.bytes(magic.data(),magic.size());if(std::string_view(magic.data(),8)!=session_magic)throw std::runtime_error("Not a Kleiotide session.");
    const auto version=r.number(4);if(version<1||version>session_version)throw std::runtime_error("Unsupported Kleiotide session version "+std::to_string(version)+". This application reads versions 1–5.");
    const auto next=r.number();Session result;result.formatVersion=static_cast<unsigned>(version);result.viewState=r.text(small);result.alignmentReport=r.text(16*small);
    std::vector<SequenceGroup> groups;GroupId nextGroup=1;
    if(version>=3){const auto count=r.count(100000);nextGroup=r.number();for(std::size_t i=0;i<count;++i){SequenceGroup g;g.id=r.number();g.name=r.text(small);g.color=static_cast<std::uint32_t>(r.number(4));groups.push_back(std::move(g));}}
    const auto settings=version>=4?read_settings(r):TranslationSettings{};
    const auto traces=r.count(1000000);std::vector<std::shared_ptr<const Chromatogram>> originals;
    for(std::size_t i=0;i<traces;++i){auto t=std::make_shared<Chromatogram>();t->sourcePath=r.text(small);t->sampleName=r.text(small);t->callSet=static_cast<unsigned>(r.number(4));if(t->callSet!=1&&t->callSet!=2)throw std::runtime_error("Invalid imported call set.");t->calls=r.text(maxCalls);
        if(t->calls.empty()||std::any_of(t->calls.begin(),t->calls.end(),[](char c){return !is_nucleotide_symbol(c)||is_gap(c);}))throw std::runtime_error("Invalid original base calls.");
        auto n=r.count(maxCalls);if(n!=t->calls.size())throw std::runtime_error("Session call/peak mismatch.");for(std::size_t j=0;j<n;++j)t->peaks.push_back(static_cast<std::uint32_t>(r.number(4)));
        n=r.count(maxCalls);if(n&&n!=t->calls.size())throw std::runtime_error("Session call/quality mismatch.");
        for(std::size_t j=0;j<n;++j)t->quality.push_back(static_cast<std::uint8_t>(r.number(1)));
        for(auto& channel:t->channels){n=r.count(maxCalls);for(std::size_t j=0;j<n;++j)channel.push_back(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(r.number(4))));}
        // Validate even unreferenced original evidence.
        auto link=std::make_shared<TraceLink>();link->original=t;for(std::size_t j=0;j<t->calls.size();++j)link->originalCalls.push_back(static_cast<std::int32_t>(j));
        Sequence check{0,"trace",SequenceBuffer(t->calls),RowKind::Nucleotide,link};validate_trace(check);originals.push_back(t);
    }
    const auto rows=r.count(1000000);std::vector<Record> records;std::vector<SequenceId> ids;
    for(std::size_t i=0;i<rows;++i){ids.push_back(r.number());const auto kind=r.number(1);if(kind>(version==1?1:2))throw std::runtime_error("Invalid session row type.");
        Record row;row.kind=static_cast<RowKind>(kind);
        const auto representation=version==1?0:r.number(1);if(representation>(version>=4?1:0))throw std::runtime_error("Unsupported sequence representation. A newer Kleiotide version may be required.");
        row.representation=static_cast<RowRepresentation>(representation);row.groupId=version>=3?r.number():0;row.name=r.text(small);row.symbols=r.text(1024ULL*1024*1024);
        if(std::any_of(row.symbols.begin(),row.symbols.end(),[&](char c){return c=='.'||!is_row_symbol(c,row.kind);}))throw std::runtime_error("Invalid stored sequence symbols.");
        const auto index=r.number();if(index>originals.size())throw std::runtime_error("Invalid session trace reference.");
        if(index){auto link=std::make_shared<TraceLink>();link->original=originals[static_cast<std::size_t>(index-1)];const auto reverse=r.number(1);if(reverse>1)throw std::runtime_error("Invalid trace orientation.");link->reversed=reverse==1;
            auto n=r.count(row.symbols.size());const auto expected=static_cast<std::size_t>(std::count_if(row.symbols.begin(),row.symbols.end(),[](char c){return !is_gap(c);}));if(n!=expected)throw std::runtime_error("Invalid session residue mapping length.");
            for(std::size_t j=0;j<n;++j)link->originalCalls.push_back(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(r.number(4))));
            if(version>=5){const auto hints=r.count(expected);if(hints&&hints!=expected)throw std::runtime_error("Invalid inferred peak count.");for(std::size_t j=0;j<hints;++j)link->inferredPeaks.push_back(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(r.number(4))));}
            row.trace=link;
        }if(version>=4)row.protein=read_protein(r);records.push_back(std::move(row));
    }r.finish();result.document.restore_session(std::move(records),ids,next,std::move(groups),nextGroup,settings);return result;
}
}
