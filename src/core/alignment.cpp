#include "core/alignment.hpp"
#include "core/translation.hpp"
#include "core/collapse.hpp"
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>

namespace kleiotide {
namespace {
std::size_t residues(std::string_view text) {
    return static_cast<std::size_t>(std::count_if(text.begin(), text.end(),
        [](char c) { return !is_gap(c); }));
}
void validate_name(const std::string& name) {
    const bool blank = std::all_of(name.begin(), name.end(), [](char c) {
        return c == ' ' || c == '\t';
    });
    if (blank || name.find_first_of("\r\n") != std::string::npos || name.find('\0') != std::string::npos)
        throw std::invalid_argument("A sequence name must be nonblank and contain no newline or null byte.");
}
constexpr std::size_t rankBlockSize = 1024;
std::string normalize_symbols(std::string_view text,RowKind kind) {
    if(kind==RowKind::Nucleotide)return normalize_nucleotides(text);
    if(kind!=RowKind::Mask&&kind!=RowKind::Protein)throw std::invalid_argument("Unknown sequence type.");
    std::string result;result.reserve(text.size());
    for(char c:text){if(!is_row_symbol(c,kind))throw std::invalid_argument(kind==RowKind::Protein?"Unsupported amino-acid symbol. Use one-letter amino-acid codes, B/Z/J/X/U/O, *, ? or gaps.":"Mask symbols must be printable ASCII without spaces or '>'.");result+=c=='.'?'~':c;}
    return result;
}
}
bool is_mask_symbol(char c) noexcept {return c>='!'&&c<='~'&&c!='>';}
std::array<unsigned,4> nucleotide_votes(char c) noexcept {
    if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');
    switch(c){
    case 'A':return {6,0,0,0};case 'C':return {0,6,0,0};case 'G':return {0,0,6,0};case 'T':case 'U':return {0,0,0,6};
    case 'R':return {3,0,3,0};case 'Y':return {0,3,0,3};case 'S':return {0,3,3,0};case 'W':return {3,0,0,3};case 'K':return {0,0,3,3};case 'M':return {3,3,0,0};
    case 'B':return {0,2,2,2};case 'D':return {2,0,2,2};case 'H':return {2,2,0,2};case 'V':return {2,2,2,0};default:return {};
    }
}
bool is_gap(char symbol) noexcept {
    return symbol == '-' || symbol == '~' || symbol == '.';
}
bool is_nucleotide_symbol(char symbol) noexcept {
    if (symbol >= 'a' && symbol <= 'z') symbol = static_cast<char>(symbol - 'a' + 'A');
    return std::string_view("ACGTURYSWKMBDHVN-~.?").find(symbol) != std::string_view::npos;
}
bool is_protein_symbol(char symbol) noexcept {
    if(symbol>='a'&&symbol<='z')symbol=static_cast<char>(symbol-'a'+'A');
    return std::string_view("ACDEFGHIKLMNPQRSTVWYBZJXUO*?-~.").find(symbol)!=std::string_view::npos;
}
bool is_row_symbol(char symbol,RowKind kind) noexcept {
    switch(kind){case RowKind::Nucleotide:return is_nucleotide_symbol(symbol);case RowKind::Protein:return is_protein_symbol(symbol);case RowKind::Mask:return is_mask_symbol(symbol);default:return false;}
}
RowKind inferred_row_kind(std::string_view text) noexcept {
    return std::all_of(text.begin(),text.end(),is_nucleotide_symbol)?RowKind::Nucleotide:RowKind::Mask;
}
std::string_view row_kind_name(RowKind kind) noexcept {
    switch(kind){case RowKind::Nucleotide:return "Nucleotide";case RowKind::Protein:return "Protein";case RowKind::Mask:return "Mask";default:return "Unknown";}
}
std::string normalize_nucleotides(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        if (!is_nucleotide_symbol(c))
            throw std::invalid_argument("Unsupported nucleotide symbol.");
        result.push_back(c == '.' ? '~' : c);
    }
    return result;
}
SequenceBuffer::SequenceBuffer(std::string text,RowKind kind)
    : text_(normalize_symbols(text,kind)), residueCount_(residues(text_)) {}
void SequenceBuffer::replace(std::size_t start, std::size_t count,
                             std::string_view replacement) {
    const auto oldCount = residues(std::string_view(text_).substr(start, count));
    text_.replace(start, count, replacement);
    residueCount_ = residueCount_ - oldCount + residues(replacement);
    rankValid_ = false;
}
std::size_t SequenceBuffer::residue_rank(std::size_t end) const {
    if (end > text_.size()) throw std::out_of_range("Residue coordinate outside sequence.");
    if (!rankValid_) {
        rankBlocks_.clear();
        rankBlocks_.reserve(text_.size() / rankBlockSize + 1);
        rankBlocks_.push_back(0);
        std::size_t count = 0;
        for (std::size_t i = 0; i < text_.size(); ++i) {
            count += !is_gap(text_[i]);
            if ((i + 1) % rankBlockSize == 0) rankBlocks_.push_back(count);
        }
        rankValid_ = true;
    }
    const auto block = end / rankBlockSize;
    return rankBlocks_[block] + residues(std::string_view(text_).substr(
        block * rankBlockSize, end % rankBlockSize));
}
const Sequence& Document::sequence(SequenceId id) const {
    const auto it = std::find_if(rows_.begin(), rows_.end(), [id](const auto& r) { return r.id == id; });
    if (it == rows_.end()) throw std::out_of_range("Unknown sequence ID.");
    return *it;
}
Sequence& Document::mutable_sequence(SequenceId id) {
    const auto it = std::find_if(rows_.begin(), rows_.end(), [id](const auto& r) { return r.id == id; });
    if (it == rows_.end()) throw std::out_of_range("Unknown sequence ID.");
    return *it;
}
std::size_t Document::columns() const noexcept {
    std::size_t result = 0;
    for (const auto& row : rows_) result = std::max(result, row.symbols.size());
    return result;
}
std::vector<Sequence> Document::make_rows(std::vector<Record> records,const std::vector<SequenceGroup>* catalog) {
    if(!catalog)catalog=&groups_;
    std::unordered_set<GroupId> groupIds;for(const auto& g:*catalog)groupIds.insert(g.id);
    std::vector<Sequence> result;
    result.reserve(records.size());
    auto next=nextSequenceId_;
    for (auto& r : records) {
        validate_name(r.name);
        if(r.groupId&&!groupIds.contains(r.groupId))throw std::invalid_argument("Unknown sequence group.");
        if(r.kind==RowKind::Automatic)r.kind=inferred_row_kind(r.symbols);
        if(r.representation!=RowRepresentation::AlignedSymbols&&r.representation!=RowRepresentation::CodonSpans)throw std::invalid_argument("Unsupported sequence representation.");
        normalize_legacy_protein(r);
        SequenceBuffer buffer(std::move(r.symbols),r.kind);
        if(next==std::numeric_limits<SequenceId>::max())throw std::overflow_error("Sequence ID space exhausted.");
        result.push_back({next++, std::move(r.name), std::move(buffer),r.kind,std::move(r.trace),r.representation,r.groupId,std::move(r.protein)});
        validate_trace(result.back());validate_protein_layout(result.back());
    }
    nextSequenceId_=next;return result;
}
void Document::load(std::vector<Record> records) {
    const std::vector<SequenceGroup> empty;auto rows = make_rows(std::move(records),&empty); // Validate before mutation.
    groups_.clear();nextGroupId_=1;translation_={};
    rows_ = std::move(rows);
    undo_.clear();
    redo_.clear();
    stateId_ = nextStateId_++; contentId_ = nextContentId_++;
}
void Document::restore_session(std::vector<Record> records,const std::vector<SequenceId>& ids,SequenceId next,std::vector<SequenceGroup> groups,GroupId nextGroup,TranslationSettings translation) {
    validate_translation_settings(translation);
    if(records.size()!=ids.size()||!next||!nextGroup)throw std::invalid_argument("Invalid session IDs.");
    std::unordered_set<SequenceId> unique;for(auto id:ids)if(!id||id>=next||!unique.insert(id).second)throw std::invalid_argument("Invalid or repeated session row ID.");
    Document result;result.translation_=std::move(translation);auto patch=result.group_patch(std::move(groups),{});for(const auto& g:patch.other)if(g.id>=nextGroup)throw std::invalid_argument("Invalid next group ID.");
    result.groups_=std::move(patch.other);result.nextGroupId_=nextGroup;result.rows_=result.make_rows(std::move(records));
    for(std::size_t i=0;i<ids.size();++i)result.rows_[i].id=ids[i];
    result.nextSequenceId_=next;result.stateId_=result.nextStateId_++;result.contentId_=result.nextContentId_++;*this=std::move(result);
}
void Document::append(std::vector<Record> records){insert_records(std::move(records),rows_.size());}
void Document::insert_records(std::vector<Record> records,std::size_t position,std::optional<std::vector<SequenceGroup>> catalog){
    if(position>rows_.size())throw std::out_of_range("Paste position outside alignment.");
    if(records.empty())return;
    Command command;command.cost=sizeof(Command)+sizeof(RowSplice);
    if(catalog){auto patch=group_patch(std::move(*catalog),{});command.cost+=sizeof(GroupPatch)+patch.assignments.size()*sizeof(GroupAssignment);for(const auto& g:groups_)command.cost+=sizeof(SequenceGroup)+g.name.size();for(const auto& g:patch.other)command.cost+=sizeof(SequenceGroup)+g.name.size();command.edits.emplace_back(std::move(patch));}
    const auto* incoming=catalog?&std::get<GroupPatch>(command.edits.front()).other:&groups_;
    auto rows=make_rows(std::move(records),incoming);
    for(const auto& r:rows)command.cost+=r.name.size()+r.symbols.size()+sizeof(Sequence)+trace_storage_bytes(r.trace)+protein_storage_bytes(r.protein);
    command.edits.emplace_back(RowSplice{position,0,rows.size(),{},std::move(rows)});execute(std::move(command));
}
Document Document::derive(std::vector<Record> records)const {
    if(records.size()!=rows_.size())throw std::invalid_argument("Derived alignment must retain every source row.");
    for(std::size_t i=0;i<rows_.size();++i){if(records[i].kind==RowKind::Automatic)records[i].kind=inferred_row_kind(records[i].symbols);
        if(records[i].name!=rows_[i].name||records[i].kind!=rows_[i].kind||records[i].representation!=rows_[i].representation||records[i].groupId!=rows_[i].groupId)throw std::invalid_argument("Derived alignment must preserve names, row types and representations.");}
    std::vector<SequenceId> ids;for(const auto& row:rows_)ids.push_back(row.id);
    Document result;result.restore_session(std::move(records),ids,nextSequenceId_,groups_,nextGroupId_,translation_);return result;
}
void Document::erase_rows(const std::vector<SequenceId>& ids) {
    if (ids.empty()) return;
    std::unordered_set<SequenceId> selected(ids.begin(), ids.end());
    for (const auto id : selected) (void)sequence(id); // Reject stale IDs before mutation.
    Command command;
    command.cost = sizeof(Command);
    std::size_t end = rows_.size();
    while (end != 0) {
        if (!selected.contains(rows_[end - 1].id)) { --end; continue; }
        const auto finish = end;
        while (end && selected.contains(rows_[end - 1].id)) --end;
        const auto count = finish - end;
        command.cost += sizeof(RowSplice);
        for (std::size_t i = end; i < finish; ++i)
            command.cost += sizeof(Sequence) + rows_[i].name.size() + rows_[i].symbols.size() + trace_storage_bytes(rows_[i].trace)+protein_storage_bytes(rows_[i].protein);
        command.edits.emplace_back(RowSplice{end, count, 0, {}, {}});
    }
    execute(std::move(command));
}
void Document::collapse_identical(const std::vector<CollapseGroup>& clusters){
    if(clusters.empty())return;
    Command command;command.cost=sizeof(Command);
    std::unordered_map<SequenceId,const Sequence*> byId;for(const auto& row:rows_)byId.emplace(row.id,&row);
    std::unordered_set<SequenceId> seen,removed;
    for(const auto& cluster:clusters){
        if(cluster.members.size()<2||std::find(cluster.members.begin(),cluster.members.end(),cluster.representative)==cluster.members.end())throw std::invalid_argument("A duplicate set needs at least two rows and a representative from that set.");
        const auto found=byId.find(cluster.representative);if(found==byId.end())throw std::invalid_argument("Unknown representative sequence.");const auto& representative=*found->second;
        validate_name(cluster.name);
        for(auto id:cluster.members){const auto it=byId.find(id);if(it==byId.end()||!seen.insert(id).second)throw std::invalid_argument("Duplicate sets must contain distinct existing sequences.");
            if(!identical_ignoring_gaps(representative,*it->second))throw std::invalid_argument("Collapsed sequences must have the same type and identical complete ungapped symbols, ignoring case.");
            if(id!=cluster.representative)removed.insert(id);
        }
        if(representative.name!=cluster.name){command.cost+=sizeof(NamePatch)+representative.name.size()+cluster.name.size();command.edits.emplace_back(NamePatch{cluster.representative,representative.name,cluster.name});}
    }
    std::size_t end=rows_.size();
    while(end){if(!removed.contains(rows_[end-1].id)){--end;continue;}const auto finish=end;while(end&&removed.contains(rows_[end-1].id))--end;
        command.cost+=sizeof(RowSplice);for(std::size_t i=end;i<finish;++i)command.cost+=sizeof(Sequence)+rows_[i].name.size()+rows_[i].symbols.size()+trace_storage_bytes(rows_[i].trace)+protein_storage_bytes(rows_[i].protein);
        command.edits.emplace_back(RowSplice{end,finish-end,0,{},{}});
    }
    execute(std::move(command));
}
void Document::replace(SequenceId id, std::size_t start, std::size_t count,
                       std::string_view replacement) {
    replace_many({{id,start,count,std::string(replacement),{}}});
}
void Document::replace_many(const std::vector<Replacement>& edits,std::optional<ColumnRange> erasedColumns) {
    Command command; command.cost = sizeof(Command);
    std::size_t planned=0;
    for(const auto& edit:edits){
        const auto& row=sequence(edit.id);
        if(edit.start>row.symbols.size()||edit.count>row.symbols.size()-edit.start)throw std::out_of_range("Edit range outside sequence.");
        account_bulk_memory(planned,edit.count,2);account_bulk_memory(planned,edit.text.size(),2);
        if(edit.text.size()>edit.count){
            const auto retained=row.symbols.size()-edit.count;
            if(edit.text.size()>std::numeric_limits<std::size_t>::max()-retained)throw std::length_error("Edited row exceeds storage limits.");
            account_bulk_memory(planned,retained+edit.text.size(),2);
        }
        if(row.trace||row.protein||edit.trace||edit.protein){account_bulk_memory(planned,row.symbols.size(),4);account_bulk_memory(planned,edit.text.size(),4);}
    }
    std::unordered_set<SequenceId> seen;
    for (const auto& edit : edits) {
        const auto& row = sequence(edit.id);
        if (!seen.insert(edit.id).second) throw std::invalid_argument("A row occurs twice in this edit.");
        if (edit.start > row.symbols.size() || edit.count > row.symbols.size() - edit.start)
            throw std::out_of_range("Edit range outside sequence.");
        auto after = normalize_symbols(edit.text,row.kind);
        auto before = row.symbols.text().substr(edit.start, edit.count);
        auto trace=edit.trace ? *edit.trace : edited_trace(row,edit.start,edit.count,after);
        auto protein=edit.protein?*edit.protein:row.protein;
        if(!protein&&row.representation!=RowRepresentation::AlignedSymbols)throw std::invalid_argument("Missing protein layout.");
        // Ordinary rows have no structural layout to validate. Avoid building
        // full-row temporary copies for every small edit in a genomic sequence.
        if(edit.trace||protein){
            Sequence check{row.id,row.name,SequenceBuffer(row.symbols.text().substr(0,edit.start)+after+row.symbols.text().substr(edit.start+edit.count),row.kind),row.kind,trace,row.representation,row.groupId,protein};
            if(edit.trace)validate_trace(check);
            if(protein)validate_protein_layout(check);
        }
        if(protein&&row.protein&&*protein==*row.protein)protein=row.protein;
        if (before == after && trace==row.trace && protein==row.protein) continue;
        command.cost += sizeof(TextPatch) + before.size() + after.size();
        if(protein!=row.protein)command.cost+=protein_storage_bytes(protein)+protein_storage_bytes(row.protein);
        if(trace!=row.trace){if(trace)command.cost+=sizeof(TraceLink)+trace->originalCalls.size()*sizeof(std::int32_t);if(row.trace)command.cost+=sizeof(TraceLink)+row.trace->originalCalls.size()*sizeof(std::int32_t);}
        command.edits.emplace_back(TextPatch{edit.id, edit.start, std::move(before), std::move(after),row.trace,std::move(trace),row.protein,std::move(protein)});
    }
    if(erasedColumns&&!translation_.regions.empty()){
        const auto start=erasedColumns->start,width=erasedColumns->width;
        if(!width||start>columns()||width>columns()-start)throw std::invalid_argument("Invalid column deletion.");
        for(const auto& row:rows_)if(row.symbols.size()>start){auto it=std::find_if(edits.begin(),edits.end(),[&](const auto& e){return e.id==row.id;});if(it==edits.end()||it->start!=start||it->count!=std::min(width,row.symbols.size()-start)||!it->text.empty())throw std::invalid_argument("Column deletion must cover every row.");}
        auto settings=translation_;settings.regions.clear();const auto end=start+width;
        for(const auto& r:translation_.regions){const auto finish=r.start+r.width;
            if(r.start<start){const auto length=std::min(finish,start)-r.start;if(length)settings.regions.push_back({r.start,length,r.phase});}
            if(finish>end){const auto begin=std::max(r.start,end);const auto phase=r.phase?static_cast<unsigned>((r.phase-1+begin-r.start)%3+1):0;settings.regions.push_back({begin-width,finish-begin,phase});}}
        validate_translation_settings(settings);if(settings!=translation_){command.cost+=sizeof(TranslationPatch)+(settings.regions.size()+translation_.regions.size())*sizeof(CodingRegion);command.edits.emplace_back(TranslationPatch{std::move(settings)});}
    }
    if (!command.edits.empty()) execute(std::move(command));
}
void Document::erase_columns(std::size_t start,std::size_t width){if(!width||start>columns()||width>columns()-start)throw std::out_of_range("Columns outside alignment.");std::vector<Replacement> edits;for(const auto& row:rows_)if(start<row.symbols.size())edits.push_back({row.id,start,std::min(width,row.symbols.size()-start),""});replace_many(edits,ColumnRange{start,width});}
std::int64_t Document::adjacent_gap_distance(const std::vector<SequenceId>& ids, std::size_t start,
                            std::size_t width, std::int64_t requested) const {
    const auto columnsCount = columns();
    if (!width || start > columnsCount || width > columnsCount - start)
        throw std::out_of_range("Fragment outside alignment.");
    if (ids.empty() || requested == 0) return 0;
    const bool left = requested < 0;
    const auto magnitude = left ? static_cast<std::uint64_t>(-(requested + 1)) + 1
                                : static_cast<std::uint64_t>(requested);
    auto distance = std::min<std::size_t>(left ? start : columnsCount - start - width, magnitude);
    for (const auto id : ids) {
        const auto& text = sequence(id).symbols.text();
        std::size_t available = 0;
        while (available < distance) {
            const auto c = left ? start - available - 1 : start + width + available;
            if(c<text.size()&&text[c]!='~')break;
            ++available;
        }
        distance = available;
    }
    return left ? -static_cast<std::int64_t>(distance) : static_cast<std::int64_t>(distance);
}
std::int64_t Document::slide_distance(const std::vector<SequenceId>& ids, std::size_t start,
                                    std::size_t width, std::int64_t requested) const {
    const auto span = columns();
    if (!width || start > span || width > span - start) throw std::out_of_range("Fragment outside alignment.");
    std::unordered_set<SequenceId> seen;
    for (auto id : ids) {
        (void)sequence(id);
        if (!seen.insert(id).second) throw std::invalid_argument("Duplicate row in fragment.");
    }
    if (ids.empty() || !requested) return 0;
    if (requested > 0) {
        const auto distance = static_cast<std::uint64_t>(requested);
        if (distance > std::string{}.max_size() - span) throw std::length_error("Slide exceeds sequence storage limit.");
        return requested; // No right boundary: the alignment grows if open gaps run out.
    }
    const auto magnitude = static_cast<std::uint64_t>(-(requested + 1)) + 1;
    auto distance = std::min<std::size_t>(start, magnitude);
    for (auto id : ids) {
        const auto& text = sequence(id).symbols.text();
        std::size_t available = start > text.size() ? start - text.size() : 0;
        for (auto c = std::min(start, text.size()); c && available < distance; --c)
            if (text[c-1] == '~') ++available;
        distance = std::min(distance, available);
    }
    return -static_cast<std::int64_t>(distance);
}
std::int64_t Document::slide(const std::vector<SequenceId>& ids, std::size_t start,
                            std::size_t width, std::int64_t requested) {
    const auto moved = slide_distance(ids, start, width, requested);
    if (!moved) return 0;
    const auto distance = static_cast<std::size_t>(moved < 0 ? -moved : moved);
    std::vector<Replacement> patches;
    for (auto id : ids) {
        const auto& row=sequence(id);
        const auto& text = row.symbols.text();
        std::string block = start < text.size() ? text.substr(start, width) : std::string{};
        block.resize(width, '~');
        if (moved > 0) {
            const auto begin = std::min(start, text.size());
            auto end = std::min(start + width, text.size());
            std::string pushed; std::size_t remaining = distance;
            while (end < text.size() && remaining) {
                const auto c = text[end++];
                if (c == '~') --remaining; else pushed += c;
            }
            std::string after(start - begin + distance, '~');
            after += block; after += pushed;
            patches.push_back({id, begin, end - begin, std::move(after)});
        } else {
            // Delete the nearest external open gaps from the prefix, pushing
            // every intervening residue/dash left. Internal block gaps stay intact.
            auto begin = start; std::size_t remaining = distance;
            while (remaining) { --begin; if (begin >= text.size() || text[begin] == '~') --remaining; }
            std::string after;
            for (auto c = begin; c < start; ++c) {
                const auto symbol = c < text.size() ? text[c] : '~';
                if (symbol != '~') after += symbol;
            }
            after += block; after.append(distance, '~');
            const auto existingStart = std::min(begin, text.size());
            const auto existingEnd = std::min(start + width, text.size());
            if (begin > existingStart) after.insert(0, begin - existingStart, '~');
            patches.push_back({id, existingStart, existingEnd - existingStart, std::move(after)});
        }
    }
    replace_many(patches); return moved;
}
void Document::fill_fragment(const std::vector<SequenceId>& ids, std::size_t start,
                            std::size_t width, char symbol) {
    if (!width || start > columns() || width > columns() - start)
        throw std::out_of_range("Fragment outside alignment.");
    if (symbol != 'N' && symbol != '~' && symbol != '-') throw std::invalid_argument("Invalid fragment fill.");
    std::size_t planned=0;
    for(const auto id:ids){const auto length=sequence(id).symbols.size(),begin=std::min(start,length);
        account_bulk_memory(planned,start-begin,2);account_bulk_memory(planned,width,2);account_bulk_memory(planned,std::min(start+width,length)-begin,2);}
    std::vector<Replacement> patches;
    for (const auto id : ids) {
        const auto length = sequence(id).symbols.size();
        const auto begin = std::min(start, length);
        std::string after(start - begin, '~'); after.append(width, symbol);
        patches.push_back({id, begin, std::min(start + width, length) - begin, std::move(after)});
    }
    replace_many(patches);
}
void Document::lock_fragment(const std::vector<SequenceId>& ids,std::size_t start,std::size_t width){set_fragment_gap_lock(ids,start,width,true);}
void Document::unlock_fragment(const std::vector<SequenceId>& ids,std::size_t start,std::size_t width){set_fragment_gap_lock(ids,start,width,false);}
void Document::set_fragment_gap_lock(const std::vector<SequenceId>& ids,std::size_t start,std::size_t width,bool locked){
    if(!width||start>columns()||width>columns()-start)throw std::out_of_range("Fragment outside alignment.");
    std::size_t planned=0;
    for(auto id:ids){const auto& text=sequence(id).symbols.text();if(start<text.size()){
        const auto part=std::string_view(text).substr(start,width);if(part.find(locked?'~':'-')!=std::string_view::npos)account_bulk_memory(planned,part.size(),4);}}
    std::vector<Replacement> edits;
    for(auto id:ids){const auto& text=sequence(id).symbols.text();if(start>=text.size())continue;
        if(std::string_view(text).substr(start,width).find(locked?'~':'-')==std::string_view::npos)continue;
        auto after=text.substr(start,width);const auto count=after.size();for(char& symbol:after)if(symbol==(locked?'~':'-'))symbol=locked?'-':'~';
        edits.push_back({id,start,count,std::move(after)});
    }replace_many(edits);
}
void Document::edit_gaps(const std::vector<SequenceId>& ids, std::size_t column, bool insert) {
    if (column > columns()) throw std::out_of_range("Gap position outside alignment.");
    std::size_t planned=0;
    for(auto id:ids){const auto& row=sequence(id);if(insert){account_bulk_memory(planned,column-std::min(column,row.symbols.size())+1,2);account_bulk_memory(planned,row.symbols.size(),2);}}
    std::vector<Replacement> patches;
    for (const auto id : ids) {
        const auto& row=sequence(id);const auto& text = row.symbols.text();
        if (insert) {
            const auto begin = std::min(column, text.size());
            patches.push_back({id, begin, 0, std::string(column - begin + 1, '~')});
        } else if (column < text.size() && text[column] == '~') {
            patches.push_back({id, column, 1, ""});
        }
    }
    replace_many(patches);
}
void Document::transform(const std::vector<SequenceId>& ids, Transform operation) {
    std::unordered_set<SequenceId> selected(ids.begin(),ids.end());
    if(selected.size()!=ids.size())throw std::invalid_argument("A row occurs twice in this edit.");
    std::vector<const Sequence*> targets;targets.reserve(ids.size());std::size_t planned=0,found=0;
    for(const auto& row:rows_)if(selected.contains(row.id)){
        ++found;const auto& text=row.symbols.text();bool changes=true;
        switch(operation){
        case Transform::LockGaps:changes=text.find('~')!=std::string::npos;break;
        case Transform::UnlockGaps:changes=text.find('-')!=std::string::npos;break;
        case Transform::Degap:changes=text.find('~')!=std::string::npos;break;
        case Transform::Unalign:changes=text.find_first_of("~-")!=std::string::npos;break;
        case Transform::Uppercase:changes=std::any_of(text.begin(),text.end(),[](char c){return c>='a'&&c<='z';});break;
        case Transform::Lowercase:changes=std::any_of(text.begin(),text.end(),[](char c){return c>='A'&&c<='Z';});break;
        case Transform::ReverseComplement:changes=row.kind==RowKind::Nucleotide;break;
        }
        if(changes){account_bulk_memory(planned,text.size(),row.trace||row.protein?12:4);targets.push_back(&row);}
    }
    if(found!=selected.size())throw std::out_of_range("Unknown sequence ID.");
    std::vector<Replacement> patches;
    for (const auto* target : targets) {
        const auto& row=*target;const auto id=row.id;
        if(operation==Transform::ReverseComplement&&row.kind!=RowKind::Nucleotide)continue;
        const auto& text = row.symbols.text();
        auto after = text;
        const bool rna = text.find_first_of("Uu") != std::string::npos && text.find_first_of("Tt") == std::string::npos;
        if (operation == Transform::Degap || operation == Transform::Unalign)
            std::erase_if(after, [&](char c) { return operation == Transform::Degap ? c == '~' : is_gap(c); });
        else {
            for (auto& c : after) {
                const bool lower = c >= 'a' && c <= 'z';
                const auto upper = lower ? static_cast<char>(c - 'a' + 'A') : c;
                switch (operation) {
                case Transform::LockGaps: if (c == '~') c = '-'; break;
                case Transform::UnlockGaps: if (c == '-') c = '~'; break;
                case Transform::Uppercase: c = upper; break;
                case Transform::Lowercase: if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a'); break;
                case Transform::ReverseComplement: {
                    const std::string_view bases = "ACGTURYSWKMBDHVN";
                    const std::string_view complement = "TGCAAYRSWMKVHDBN";
                    const auto index = bases.find(upper);
                    if (index != std::string_view::npos) {
                        c = rna && upper == 'A' ? 'U' : complement[index];
                        if (lower) c = static_cast<char>(c - 'A' + 'a');
                    }
                    break;
                }
                default: break;
                }
            }
            if (operation == Transform::ReverseComplement) std::reverse(after.begin(), after.end());
        }
        Replacement patch{id,0,text.size(),std::move(after),{}};
        if(operation==Transform::ReverseComplement&&row.trace)patch.trace=reversed_trace(row.trace);
        patches.push_back(std::move(patch));
    }
    replace_many(patches);
}
void Document::minimize() {
    std::size_t planned=0;account_bulk_memory(planned,columns()/8+1);
    const auto width=columns();std::vector<bool> keep(width,false);
    for(const auto& row:rows_){for(std::size_t c=0;c<row.symbols.size();++c)if(!is_gap(row.symbols.text()[c]))keep[c]=true;
        // Structural codon cells travel with their amino-acid tokens.
        if(row.protein)for(const auto& span:row.protein->spans)for(auto c=span.first();c<=span.last();++c)keep[c]=true;}
    const auto retained=static_cast<std::size_t>(std::count(keep.begin(),keep.end(),true));if(retained==width)return;
    account_bulk_memory(planned,width,sizeof(std::size_t));for(const auto& row:rows_)account_bulk_memory(planned,row.symbols.size(),3);
    std::vector<std::size_t> mapped(width);std::size_t next=0;for(std::size_t c=0;c<width;++c){mapped[c]=next;if(keep[c])++next;}
    auto settings=translation_;settings.regions.clear();
    for(const auto& r:translation_.regions)for(auto c=r.start;c<std::min(width,r.start+r.width);++c)if(keep[c]){
        const auto phase=r.phase?static_cast<unsigned>((r.phase-1+c-r.start)%3+1):0;
        if(!settings.regions.empty()){auto& last=settings.regions.back();const auto expected=last.phase?static_cast<unsigned>((last.phase-1+last.width)%3+1):0;if(last.start+last.width==mapped[c]&&expected==phase){++last.width;continue;}}
        settings.regions.push_back({mapped[c],1,phase});}
    validate_translation_settings(settings);Command command;command.cost=sizeof(Command);
    for(const auto& row:rows_){std::string after;for(std::size_t c=0;c<row.symbols.size();++c)if(keep[c])after+=row.symbols.text()[c];auto layout=row.protein;
        if(after!=row.symbols.text()){command.cost+=sizeof(TextPatch)+after.size()+row.symbols.size();command.edits.emplace_back(TextPatch{row.id,0,row.symbols.text(),std::move(after),row.trace,row.trace,row.protein,layout});}}
    if(settings!=translation_){command.cost+=sizeof(TranslationPatch)+(settings.regions.size()+translation_.regions.size())*sizeof(CodingRegion);command.edits.emplace_back(TranslationPatch{std::move(settings)});}
    if(!command.edits.empty())execute(std::move(command));
}
void Document::flush(char gap) {
    if (gap != '-' && gap != '~') throw std::invalid_argument("Flush requires '-' or '~'.");
    const auto width = columns();
    std::size_t planned=0;
    for(const auto& row:rows_)if(row.symbols.size()<width){account_bulk_memory(planned,width,2);account_bulk_memory(planned,width-row.symbols.size());}
    Command command;
    command.cost = sizeof(Command);
    for (const auto& row : rows_) {
        if (row.symbols.size() == width) continue;
        std::string padding(width - row.symbols.size(), gap);
        command.cost += sizeof(TextPatch) + padding.size();
        command.edits.emplace_back(TextPatch{row.id, row.symbols.size(), {}, std::move(padding),row.trace,row.trace,row.protein,row.protein});
    }
    if (!command.edits.empty()) execute(std::move(command));
}
void Document::account_bulk_memory(std::size_t& total,std::size_t bytes,std::size_t copies) const {
    if(total>bulkEditMemoryLimit_||(copies&&(bytes>(bulkEditMemoryLimit_-total)/copies)))
        throw std::length_error("This edit exceeds the "+std::to_string(bulkEditMemoryLimit_/(1024*1024))+" MiB additional working-memory limit. Select fewer sequences or use a shorter alignment. No sequence changes were made.");
    total+=bytes*copies;
}
void Document::set_kind(const std::vector<SequenceId>& ids,RowKind kind){
    if(kind!=RowKind::Nucleotide&&kind!=RowKind::Protein&&kind!=RowKind::Mask)throw std::invalid_argument("Choose nucleotide, protein or mask.");
    Command command;command.cost=sizeof(Command);std::unordered_set<SequenceId> seen;
    for(auto id:ids){const auto& row=sequence(id);if(!seen.insert(id).second)throw std::invalid_argument("Repeated row in type change.");
        if(row.kind==kind)continue;
        if(row.trace&&kind!=RowKind::Nucleotide)throw std::invalid_argument("Trace-linked sequences must remain nucleotide sequences.");
        if(!std::all_of(row.symbols.text().begin(),row.symbols.text().end(),[&](char c){return is_row_symbol(c,kind);}))
            throw std::invalid_argument("Sequence '"+row.name+"' contains symbols incompatible with the chosen type.");
        command.edits.emplace_back(KindPatch{id,row.kind,kind,row.protein,kind==RowKind::Protein?row.protein:nullptr});command.cost+=sizeof(KindPatch);
    }
    if(!command.edits.empty())execute(std::move(command));
}
void Document::rename(const std::vector<std::pair<SequenceId, std::string>>& names) {
    Command command;
    command.cost = sizeof(Command);
    std::unordered_set<SequenceId> seen;
    for (const auto& [id, name] : names) {
        const auto& row = sequence(id);
        validate_name(name);
        if (!seen.insert(id).second) throw std::invalid_argument("A sequence occurs twice in this rename operation.");
        if (row.name == name) continue;
        command.cost += sizeof(NamePatch) + row.name.size() + name.size();
        command.edits.emplace_back(NamePatch{id, row.name, name});
    }
    if (!command.edits.empty()) execute(std::move(command));
}
void Document::prepare(Command& command, bool forward) {
    // All allocations that can affect row/sequence storage are attempted before
    // applying the command. Reserve may change capacity, not document content.
    std::size_t requiredRows = rows_.size();
    for (auto& edit : command.edits) {
        if (std::holds_alternative<TranslationPatch>(edit)) {
        } else if (auto* p = std::get_if<TextPatch>(&edit)) {
            auto& text = mutable_sequence(p->id).symbols;
            const auto& outgoing = forward ? p->before : p->after;
            const auto& incoming = forward ? p->after : p->before;
            if (outgoing.size() > text.size()) throw std::logic_error("Invalid history patch.");
            text.reserve(text.size() - outgoing.size() + incoming.size());
        } else if (auto* name = std::get_if<NamePatch>(&edit)) {
            mutable_sequence(name->id).name.reserve((forward ? name->after : name->before).size());
        } else if (auto* kind=std::get_if<KindPatch>(&edit)) {
            (void)sequence(kind->id);
        } else if (auto* groups=std::get_if<GroupPatch>(&edit)) {
            for(const auto& a:groups->assignments)(void)sequence(a.id);
        } else if (auto* order=std::get_if<RowOrder>(&edit)) {
            if(order->count!=rows_.size())throw std::logic_error("Invalid row order history.");
        } else {
            auto& splice = std::get<RowSplice>(edit);
            auto& incoming = forward ? splice.inserted : splice.removed;
            auto& parked = forward ? splice.removed : splice.inserted;
            const auto take = forward ? splice.removeCount : splice.insertCount;
            requiredRows += incoming.size(); // Conservative reserve for a multi-splice transaction.
            parked.reserve(take);
        }
    }
    rows_.reserve(requiredRows);
}
void Document::apply(Command& command, bool forward) {
    auto applyEdit = [&](Edit& edit) {
        if(auto* t=std::get_if<TranslationPatch>(&edit)){std::swap(translation_,t->other);
        } else if (auto* p = std::get_if<TextPatch>(&edit)) {
            mutable_sequence(p->id).symbols.replace(p->start,
                forward ? p->before.size() : p->after.size(),
                forward ? std::string_view(p->after) : std::string_view(p->before));
            mutable_sequence(p->id).trace=forward?p->afterTrace:p->beforeTrace;
            mutable_sequence(p->id).protein=forward?p->afterProtein:p->beforeProtein;
        } else if (auto* name = std::get_if<NamePatch>(&edit)) {
            mutable_sequence(name->id).name.assign(forward ? name->after : name->before);
        } else if (auto* kind=std::get_if<KindPatch>(&edit)) {
            mutable_sequence(kind->id).kind=forward?kind->after:kind->before;
            mutable_sequence(kind->id).protein=forward?kind->afterProtein:kind->beforeProtein;
        } else if (auto* groups=std::get_if<GroupPatch>(&edit)) {
            groups_.swap(groups->other);nextGroupId_=std::max(nextGroupId_,groups->next);
            // group_patch emits assignments in source row order. History restores
            // that order before this edit, including composite insert commands.
            auto at=groups->assignments.begin();
            for(auto& row:rows_)if(at!=groups->assignments.end()&&at->id==row.id){row.groupId=forward?at->after:at->before;++at;}
        } else if (auto* order=std::get_if<RowOrder>(&edit)) {
            if(forward){for(const auto& [a,b]:order->swaps)std::swap(rows_[a],rows_[b]);}
            else{for(auto i=order->swaps.rbegin();i!=order->swaps.rend();++i)std::swap(rows_[i->first],rows_[i->second]);}
        } else {
            auto& s = std::get<RowSplice>(edit);
            auto& incoming = forward ? s.inserted : s.removed;
            auto& parked = forward ? s.removed : s.inserted;
            const auto take = forward ? s.removeCount : s.insertCount;
            parked.clear();
            for (std::size_t i = 0; i < take; ++i)
                parked.push_back(std::move(rows_[s.index + i]));
            auto first = rows_.begin() + static_cast<std::ptrdiff_t>(s.index);
            rows_.erase(first, first + static_cast<std::ptrdiff_t>(take));
            rows_.insert(rows_.begin() + static_cast<std::ptrdiff_t>(s.index),
                std::make_move_iterator(incoming.begin()), std::make_move_iterator(incoming.end()));
            incoming.clear();
        }
    };
    if (forward) {
        for (auto& edit : command.edits) applyEdit(edit);
    } else {
        for (auto it = command.edits.rbegin(); it != command.edits.rend(); ++it) applyEdit(*it);
    }
    stateId_ = forward ? command.after : command.before;
    contentId_ = forward ? command.contentAfter : command.contentBefore;
}
void Document::execute(Command command) {
    command.before = stateId_;
    command.after = nextStateId_++;
    command.contentBefore = contentId_;
    const bool contentChanged=std::any_of(command.edits.begin(),command.edits.end(),[](const auto& edit) {return !std::holds_alternative<NamePatch>(edit)&&!std::holds_alternative<GroupPatch>(edit)&&!std::holds_alternative<TranslationPatch>(edit);});
    command.contentAfter = contentChanged ? nextContentId_++ : contentId_;
    undo_.reserve(undo_.size() + 1);
    prepare(command, true);
    apply(command, true);
    redo_.clear();
    undo_.push_back(std::move(command));
    trim_history();
}
std::optional<Document::ResidueLocation> Document::residue_location(const Command& command,bool forward) {
    for(const auto& edit:command.edits)if(const auto* patch=std::get_if<TextPatch>(&edit)){
        const auto& destination=forward?patch->after:patch->before;
        const auto& other=forward?patch->before:patch->after;
        std::size_t prefix=0,suffix=0;
        while(prefix<std::min(destination.size(),other.size())&&destination[prefix]==other[prefix])++prefix;
        while(suffix<std::min(destination.size(),other.size())-prefix&&destination[destination.size()-1-suffix]==other[other.size()-1-suffix])++suffix;
        // Trace-only edits can keep the same base text.
        if(destination==other)return ResidueLocation{patch->id,patch->start,std::max<std::size_t>(1,destination.size())};
        return ResidueLocation{patch->id,patch->start+prefix,std::max<std::size_t>(1,destination.size()-prefix-suffix)};
    }
    return {};
}
std::optional<Document::ResidueLocation> Document::undo_residue_location() const {
    return undo_.empty()?std::nullopt:residue_location(undo_.back(),false);
}
std::optional<Document::ResidueLocation> Document::redo_residue_location() const {
    return redo_.empty()?std::nullopt:residue_location(redo_.back(),true);
}
void Document::undo() {
    if (!can_undo()) return;
    redo_.reserve(redo_.size() + 1);
    prepare(undo_.back(), false);
    apply(undo_.back(), false);
    redo_.push_back(std::move(undo_.back()));
    undo_.pop_back();
}
void Document::redo() {
    if (!can_redo()) return;
    undo_.reserve(undo_.size() + 1);
    prepare(redo_.back(), true);
    apply(redo_.back(), true);
    undo_.push_back(std::move(redo_.back()));
    redo_.pop_back();
}
std::size_t Document::history_bytes() const noexcept {
    std::size_t bytes = 0;
    for (const auto& c : undo_) bytes += c.cost;
    for (const auto& c : redo_) bytes += c.cost;
    return bytes;
}
void Document::trim_history() {
    // Retain the newest undo even when one operation exceeds the nominal budget.
    // Budget measures logical payload, not allocator overhead or retained capacity.
    while (undo_.size() > 1 && history_bytes() > historyBudget_) undo_.erase(undo_.begin());
}
void Document::set_history_budget(std::size_t bytes) {
    historyBudget_ = bytes;
    trim_history();
}
} // namespace kleiotide

