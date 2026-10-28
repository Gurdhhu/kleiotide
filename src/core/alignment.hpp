#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <utility>
#include <vector>
#include <array>
#include <optional>
#include <unordered_map>
#include "core/chromatogram.hpp"
#include "core/translation_types.hpp"

namespace kleiotide {
using SequenceId = std::uint64_t;
using GroupId = std::uint64_t;
struct SequenceGroup {GroupId id=0;std::string name;std::uint32_t color=0;
    bool operator==(const SequenceGroup&)const=default;};
bool is_gap(char symbol) noexcept;
bool is_nucleotide_symbol(char symbol) noexcept;
bool is_protein_symbol(char symbol) noexcept;
std::string normalize_nucleotides(std::string_view text);
bool is_mask_symbol(char symbol) noexcept;
// Six units per informative IUPAC call; zero for N, ?, or gaps.
std::array<unsigned,4> nucleotide_votes(char symbol) noexcept;
enum class RowKind : std::uint8_t { Nucleotide=0, Mask=1, Protein=2, Automatic=255 };
// Canonical one-letter symbols; CodonSpans adds explicit structural display
// cells. Three-letter labels never become three biological residues.
enum class RowRepresentation : std::uint8_t { AlignedSymbols=0, CodonSpans=1 };
bool is_row_symbol(char symbol,RowKind kind) noexcept;
RowKind inferred_row_kind(std::string_view symbols) noexcept; // Legacy nucleotide/mask inference only.
std::string_view row_kind_name(RowKind kind) noexcept;

struct Record {
    std::string name; // Complete UTF-8 FASTA header, without '>'.
    std::string symbols;
    RowKind kind = RowKind::Automatic; // Input-only default; never stored in a Document.
    std::shared_ptr<const TraceLink> trace = {};
    RowRepresentation representation = RowRepresentation::AlignedSymbols;
    GroupId groupId=0;
    std::shared_ptr<const ProteinLayout> protein={};
};

class SequenceBuffer {
public:
    SequenceBuffer() = default;
    explicit SequenceBuffer(std::string text,RowKind kind=RowKind::Nucleotide);
    const std::string& text() const noexcept { return text_; }
    std::size_t size() const noexcept { return text_.size(); }
    std::size_t residue_count() const noexcept { return residueCount_; }
    // Number of residues in [0, end). End must be <= size().
    std::size_t residue_rank(std::size_t end) const;
private:
    friend class Document;
    void reserve(std::size_t length) { text_.reserve(length); }
    void replace(std::size_t start, std::size_t count, std::string_view replacement);
    std::string text_;
    std::size_t residueCount_ = 0;
    mutable bool rankValid_ = false;
    mutable std::vector<std::size_t> rankBlocks_;
};

struct Sequence {
    SequenceId id = 0;
    std::string name;
    SequenceBuffer symbols;
    RowKind kind = RowKind::Nucleotide;
    std::shared_ptr<const TraceLink> trace = {};
    RowRepresentation representation = RowRepresentation::AlignedSymbols;
    GroupId groupId=0;
    std::shared_ptr<const ProteinLayout> protein={};
};

class Document {
public:
    const std::vector<Sequence>& rows() const noexcept { return rows_; }
    const Sequence* find_sequence(SequenceId id) const noexcept;
    const Sequence& sequence(SequenceId id) const;
    std::size_t columns() const noexcept;
    std::uint64_t state_id() const noexcept { return stateId_; }
    std::uint64_t content_id() const noexcept { return contentId_; }
    // Bounded chronological invalidation log, independent of Undo retention.
    // No value means a structural/length change or an expired history window.
    struct ContentRange {std::size_t begin=0,end=0;};
    std::optional<ContentRange> changed_columns_since(std::uint64_t revision) const noexcept;
    void load(std::vector<Record> records,TranslationSettings translation={}); // New baseline; clears history.
    Document derive(std::vector<Record> records) const; // New result baseline; same row IDs.
    void append(std::vector<Record> records);
    const TranslationSettings& translation_settings()const{return translation_;}
    void set_translation_settings(TranslationSettings);
    std::vector<SequenceId> insert_translations(std::vector<Record>,const std::vector<SequenceId>& sources);
    void replace_protein(SequenceId,std::size_t start,std::size_t count,std::string_view aminoAcids);
    void insert_records(std::vector<Record>,std::size_t position,std::optional<std::vector<SequenceGroup>> catalog={});
    void insert_document(const Document&,std::size_t position);
    Document subset(const std::vector<SequenceId>&)const;
    const std::vector<SequenceGroup>& groups()const noexcept{return groups_;}
    GroupId next_group_id()const noexcept{return nextGroupId_;}
    const SequenceGroup* group(GroupId)const noexcept;
    void apply_groups(std::vector<SequenceGroup>,const std::vector<std::pair<SequenceId,GroupId>>& assignments={});
    SequenceId next_sequence_id() const noexcept {return nextSequenceId_;}
    void restore_session(std::vector<Record> records,const std::vector<SequenceId>& ids,SequenceId next,
        std::vector<SequenceGroup> groups={},GroupId nextGroup=1,TranslationSettings translation={});
    void erase_rows(const std::vector<SequenceId>& ids);
    struct CollapseGroup {SequenceId representative=0;std::vector<SequenceId> members;std::string name;};
    // Keep the chosen row and its evidence exactly; rename and remove duplicates in one command.
    void collapse_identical(const std::vector<CollapseGroup>&);
    void replace(SequenceId id, std::size_t start, std::size_t count,
                 std::string_view replacement);
    struct Replacement { SequenceId id; std::size_t start, count; std::string text;
        std::optional<std::shared_ptr<const TraceLink>> trace = {};
        std::optional<std::shared_ptr<const ProteinLayout>> protein={}; };
    enum class Transform { LockGaps, UnlockGaps, Degap, Unalign, Uppercase, Lowercase, ReverseComplement };
    struct ColumnRange { std::size_t start=0,width=0; };
    void replace_many(const std::vector<Replacement>& edits,std::optional<ColumnRange> erasedColumns={});
    void erase_columns(std::size_t start,std::size_t width);
    // Shift a rigid fragment, consuming external open gaps and pushing other content.
    std::int64_t adjacent_gap_distance(const std::vector<SequenceId>& ids, std::size_t start,
                                       std::size_t width, std::int64_t requested) const;
    std::int64_t slide_distance(const std::vector<SequenceId>& ids, std::size_t start,
                                std::size_t width, std::int64_t requested) const;
    std::int64_t slide(const std::vector<SequenceId>& ids, std::size_t start,
                       std::size_t width, std::int64_t requested);
    void fill_fragment(const std::vector<SequenceId>& ids, std::size_t start,
                       std::size_t width, char symbol);
    void edit_gaps(const std::vector<SequenceId>& ids, std::size_t column, bool insert);
    void transform(const std::vector<SequenceId>& ids, Transform operation);
    void lock_fragment(const std::vector<SequenceId>& ids,std::size_t start,std::size_t width);
    void unlock_fragment(const std::vector<SequenceId>& ids,std::size_t start,std::size_t width);
    void minimize();
    void flush(char gap = '~');
    void rename(const std::vector<std::pair<SequenceId, std::string>>& names);
    void set_kind(const std::vector<SequenceId>& ids,RowKind kind);
    void reorder(const std::vector<SequenceId>& order);
    void move_rows(const std::vector<SequenceId>& ids,std::size_t insertionIndex);
    enum class Sort { Title,LengthDescending,LengthAscending,TrueLengthDescending,TrueLengthAscending,ResidueFrequency,Identical,Reverse,Group,Type };
    std::vector<SequenceId> sorted_order(Sort method,std::optional<ColumnRange> columns={}) const;
    std::vector<SequenceId> nucleotide_ids() const;
    bool can_undo() const noexcept { return !undo_.empty(); }
    bool can_redo() const noexcept { return !redo_.empty(); }
    struct ResidueLocation { SequenceId id; std::size_t start, width; };
    // Destination coordinates, before applying Undo/Redo. Metadata-only commands
    // have no residue location and must not move the alignment viewport.
    std::optional<ResidueLocation> undo_residue_location() const;
    std::optional<ResidueLocation> redo_residue_location() const;
    void undo();
    void redo();
    std::size_t history_bytes() const noexcept;
    void set_history_budget(std::size_t bytes);
    // Cap temporary/undo payload planned by bulk edits before allocating it.
    // This is an additional working-memory bound, not a document-size limit.
    void set_bulk_edit_memory_limit(std::size_t bytes) noexcept { bulkEditMemoryLimit_=bytes; }
    std::size_t bulk_edit_memory_limit() const noexcept { return bulkEditMemoryLimit_; }
private:
    void account_bulk_memory(std::size_t& total,std::size_t bytes,std::size_t copies=1) const;
    void set_fragment_gap_lock(const std::vector<SequenceId>& ids,std::size_t start,std::size_t width,bool locked);
    struct TextPatch {
        SequenceId id;
        std::size_t start;
        std::string before;
        std::string after;
        std::shared_ptr<const TraceLink> beforeTrace = {}, afterTrace = {};
        std::shared_ptr<const ProteinLayout> beforeProtein={},afterProtein={};
    };
    // Rows move between the document and parked buffers; history does not
    // duplicate the entire document or duplicate inserted rows.
    struct RowSplice {
        std::size_t index;
        std::size_t removeCount;
        std::size_t insertCount;
        std::vector<Sequence> removed;
        std::vector<Sequence> inserted;
    };
    struct NamePatch { SequenceId id; std::string before; std::string after; };
    struct KindPatch { SequenceId id;RowKind before,after;std::shared_ptr<const ProteinLayout> beforeProtein={},afterProtein={}; };
    struct GroupAssignment {SequenceId id;GroupId before,after;};
    struct GroupPatch {std::vector<SequenceGroup> other;std::vector<GroupAssignment> assignments;GroupId next;};
    struct RowOrder { std::size_t count;std::vector<std::pair<std::size_t,std::size_t>> swaps; };
    struct TranslationPatch {TranslationSettings other;};
    using Edit = std::variant<TranslationPatch, TextPatch, RowSplice, NamePatch, RowOrder, KindPatch, GroupPatch>;
    struct Command {
        std::vector<Edit> edits;
        std::uint64_t before = 0;
        std::uint64_t after = 0;
        std::size_t cost = 0;
        std::uint64_t contentBefore = 0, contentAfter = 0;
    };
    static std::optional<ResidueLocation> residue_location(const Command&,bool forward);
    std::vector<Sequence> make_rows(std::vector<Record> records,const std::vector<SequenceGroup>* catalog=nullptr);
    GroupPatch group_patch(std::vector<SequenceGroup>,const std::vector<std::pair<SequenceId,GroupId>>&)const;
    Sequence& mutable_sequence(SequenceId id);
    void execute(Command command);
    using RowIndex = std::unordered_map<SequenceId,std::size_t>;
    static RowIndex make_index(const std::vector<Sequence>& rows);
    // A structural transaction owns a fully allocated working index before any
    // row changes. Const lookups never rebuild or mutate the published index.
    std::optional<RowIndex> prepare(Command& command, bool forward);
    void apply(Command& command, bool forward, RowIndex* preparedIndex);
    void trim_history();
    struct ContentChange {std::uint64_t before=0,after=0;ContentRange range;bool full=true;};
    std::array<ContentChange,64> contentChanges_{};
    std::size_t contentChangeCount_=0,contentChangeNext_=0;
    void record_content_change(const Command&,std::uint64_t before,std::uint64_t after) noexcept;
    TranslationSettings translation_;
    std::vector<Sequence> rows_;
    RowIndex rowIndex_;
    std::vector<SequenceGroup> groups_;
    GroupId nextGroupId_=1;
    std::vector<Command> undo_, redo_;
    SequenceId nextSequenceId_ = 1;
    std::uint64_t stateId_ = 0, nextStateId_ = 1;
    std::uint64_t contentId_ = 0, nextContentId_ = 1;
    std::size_t historyBudget_ = 64 * 1024 * 1024;
    std::size_t bulkEditMemoryLimit_ = std::size_t{2} * 1024 * 1024 * 1024;
};
} // namespace kleiotide

