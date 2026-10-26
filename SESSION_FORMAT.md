# Kleiotide native session format, versions 1–4 (.kleio)

This documented application format preserves an alignment and its imported trace
provenance. It is independent of Qt's binary serialization and native CPU byte
order. Original **analyzed** A/C/G/T channels, imported base calls, peak positions
and qualities are embedded. Source filenames are provenance labels; the original
AB1 files are not needed to reopen the session. Other instrument tags/raw channels
from the source AB1 are not copied; the source AB1 is never modified.

Current symbols, nucleotide/protein/mask types, names, row order, stable row IDs, the next
row ID, group catalog/memberships, original-call mappings (including inserted/deleted/substituted symbols),
strand, optional view settings and last MAFFT report are retained. Reopening
creates a new baseline: edits are retained, while undo/redo history starts afresh.

## Encoding

All unsigned/signed integers use little-endian byte order. Signed 32-bit integers
use two's complement. A text/blob field `T` is an unsigned 64-bit byte length,
followed by those bytes without a terminator. Names/paths/metadata are UTF-8;
sequence/call symbols use ASCII. There is no padding or implicit compression.

| Order | Field | Encoding |
| --- | --- | --- |
| 1 | Signature | 8 bytes: `KLEIO` + CR + LF + `0x1A` |
| 2 | Format version | u32 = 4; reader also accepts versions 1–3 |
| 3 | Next sequence ID | u64 |
| 4 | View state | T: optional JSON object, view schema version 1 |
| 5 | Last alignment report | T |
| 6 | Group count (versions 3+) | u64 |
| 7 | Next group ID (versions 3+) | u64 |
| 8 | Groups (versions 3+) | Repeated: stable ID u64, name T, RGB color u32 |
| 9 | Translation settings (version 4+) | Settings record below |
| 10 | Original trace count | u64 |
| 11 | Original traces | Repeated trace records below |
| 12 | Sequence row count | u64 |
| 13 | Sequence rows | Repeated row records below |
| 14 | Checksum | u32: standard CRC-32/ISO-HDLC of all preceding bytes |

A trace record contains, in order: source path T, sample name T, imported ABIF call
set u32 (1 or 2), imported calls T, peak count u64 plus that many u32 positions,
quality count u64 plus that many u8 qualities, then four channel arrays in A,C,G,T
order. Each channel array is a u64 sample count followed by signed i32 amplitudes.
Qualities may be absent (count 0); their meaning remains imported quality, not
new confidence assigned to edited calls. Original evidence shared by several
rows is written once, in first-reference order.

A version-1 row record contains: stable ID u64, kind u8 (0 nucleotide, 1 mask),
name T, current symbols T, original trace reference u64 (0 none, otherwise
one-based). A version-2 row uses kind 0 nucleotide, 1 mask, or 2 protein and
inserts a mandatory representation u8 immediately after kind. Representation 0
is aligned symbols: one ASCII symbol per alignment column; protein symbols are
canonical one-letter amino-acid codes. Other encodings reject as unsupported;
three-letter labels must never be consumed as three ordinary amino acids.
Version 3 additionally inserts group ID u64 immediately after representation:
0 means Ungrouped; other IDs must resolve to the group catalog. All remaining
row fields are unchanged. Versions 1/2 migrate with no groups and group ID 0.
For a nonzero reference, it continues with strand u8 (0 forward, 1 reverse),
anchor count u64, and that many signed i32 original-call indices. There is one
anchor per current non-gap symbol. Index -1 means an inserted symbol; other
indices are zero-based imported calls. Surviving indices must be strictly
increasing on the forward strand, decreasing on the reverse strand. Deletion
removes a current anchor without removing original evidence.

View JSON stores editing mode, font, name-area width, alignment scroll position,
row selection/fragment/caret, active trace, focused trace column, zoom/gain and
panel visibility. Version 0.10 adds optional `spacing` (0 original sample axis,
1 alignment column axis), without changing binary format version 1. The display
transform is reconstructed from preserved row/provenance data; it is not stored
as changed signal samples. IDs and column coordinates are decimal strings to retain full
64-bit precision. Unknown/invalid optional view metadata falls back to defaults;
the binary alignment and evidence still undergo all mandatory validations.

Version 0.11 adds optional `identityDots` (boolean, default false). It controls only
alignment rendering against the current first row; stored row symbols stay real
letters. Independent name and residue selections use the existing `selected` and
`fragment` fields, which can refer to different rows without affecting each other.

Version 0.14 adds optional `residuePalette` and `residueDisplay` strings to the
version-1 view object. Palettes are `classic`, `high-contrast`, `okabe-ito` and
`tol-bright`; styles are `letters`, `monochrome` and `backgrounds`. Missing or
unknown values default to `classic` and `letters`. Old readers can ignore these
fields. Binary format version 1 and all stored symbols/evidence are unchanged.
Version 0.14.1 renames Classic to Kleio while preserving its `classic` ID, and
adds `chromas` as a palette ID. Earlier readers fall back to their default when
encountering the new ID.

Version 0.15 adds no native fields. Motif query/cursor/search outlines are
transient. Selecting a hit explicitly uses the existing residue-selection field;
searching alone preserves the saved editing selection and sequence data.

Version 0.15.1 still adds no native fields. Regex/result indexes and all-hit
overlays are transient. Visiting a result now automatically updates the ordinary
residue selection, which is retained through the existing view-selection fields.
This does not modify stored sequence symbols or original trace evidence.

## Version 0.16 migration (historical)

The 0.16 writer emitted format 2. Its reader accepted 1 and 2 and validated
mandatory type/encoding/alphabet combinations, and never guesses new types from
saved symbols. Old forced masks (including ACGT or MEF) remain masks. Missing
version-1 representation metadata migrates to aligned-symbol encoding 0.
Trace links remain nucleotide-only. Samples, qualities, current anchors, IDs,
order and view data retain the original preservation contract. An independent
version-1 fixture is in tests/fixtures/session_v1.kleio; it includes a missing
source path, inserted-call anchors and nucleotide/protein-looking forced masks.

The input-only `RowKind::Automatic` default is resolved by the core's legacy
nucleotide/mask classification before document insertion; it is never a stored
row type or serialized type value. Explicit type declarations validate strictly.
Proteins accept one-letter amino-acid codes plus B/Z/J/X/U/O, *, ?, and gaps.
Native storage uses - and ~ for gaps; dot input is normalized before storage.
Plain FASTA remains canonical text without semantic type metadata. Normal Save
prefers native sessions when explicit protein or forced-mask typing would be lost.

Old Kleiotide versions that support only format 1 reject new saves at the version
check. Groups were not present in 0.16; version 0.17 adds them below. Version 0.20 adds translated protein layouts with mandatory format-4 metadata.

## Version 0.17 groups and clipboard

The writer emits format 3, with retained format-1/2 readers. The catalog is ordered
for By group sorting and contains at most 100000 groups. Group IDs are nonzero,
unique and below the next group ID; names are nonblank, unique, single-line and
contain no tab/null byte. Colors are unsigned RGB values 0x000000–0xFFFFFF. The
next group ID remains monotonic across undo/redo; retired IDs are not reassigned.
Membership is one group per row, independent of title and molecule type. Removing
a group clears its memberships; the combined catalog/member change is undoable.
Unknown catalog references and malformed semantic metadata reject before install.
Group metadata is mandatory binary data, not optional view JSON.

Copy/Cut selected sequences provides `application/x-kleiotide-sequences` containing
a native session of the selected rows and their used groups, plus normal FASTA
text. Paste validates the complete payload before adding rows and remaps row and
group IDs, retaining immutable evidence. Group name/color collisions are resolved
without overwriting existing groups. Newly imported groups and rows form one undo
command. Clipboard payloads are limited to 256 MiB.

Residue Cut provides `application/x-kleiotide-residues`: UTF-8 JSON with version 1
and a rows array of ASCII symbol strings, plus plain text joined by newlines. It
is transient, not a saved-session field. Paste validates the target row alphabets
and applies the entire block atomically; original trace evidence is unchanged.

## Reader limits and validation

All supported formats accept at most 8 GiB before the checksum, one million rows/traces,
1 MiB for names/paths/view JSON, 16 MiB for reports, 1 GiB for an individual row,
and 16,777,216 calls or samples per channel. Dimensions, call symbols, row kinds,
IDs/references, residue counts, orientation, anchor order, peak order/range,
channel lengths, checksum and end-of-file are checked before installing a document.
Declared string lengths do not cause allocation until actual bytes have been read.
Future unsupported versions are rejected. The checksum detects accidental damage;
it is not an authenticity signature.

File saves use Qt's atomic QSaveFile replacement. FASTA is a separate sequence-only
export and cannot overwrite a session or an AB1 file. External alignment software
continues to receive FASTA; .kleio is not an interchange format for other editors.

## View controls in 0.17.2

The optional version-1 view JSON keeps the original numeric `mode` enum:
0 = View, 1 = Insert, 2 = Overwrite, 3 = Slide. An optional `editMode` (1–3)
remembers the last edit submode while View is active; older sessions default this
to Overwrite. The toolbar dropdown order is independent of these stored values.
The session binary format remains version 3. New windows use Edit/Overwrite;
reopening a saved session restores its saved mode. Unknown view keys are ignored.

## Site highlighting in 0.19

Optional view JSON adds a `highlight` object with molecule (nucleotide/protein),
selectedRows/selectedColumns flags, stable row IDs and begin/end column snapshots,
color, category/range enable flags, coverage basis and inclusive range bounds.
IDs and coordinates are decimal strings, avoiding JSON double precision loss.
The reader clamps ranges/percentages and supplies defaults for absent fields.
Masks are never analysis inputs; selected IDs of other types are ignored.
No sequence semantics depend on these fields. Binary format remains 3 and older
versions can ignore the object. Composition reports remain transient exports.

## Editing controls in 0.19.1

The optional view enum now writes only 0 = View, 1 = Insert, 2 = Rewrite. Numeric 2
was previously named Overwrite. A legacy numeric 3 (Slide) maps to Rewrite; a
legacy editMode=3 remembered while in View also maps to Rewrite. New View saves
remember either Insert (1) or Rewrite (2). The binary session format remains 3.
The highlight selected-ID snapshot now controls both analysis inputs and which
rows receive shading; no new stored field is needed for this display correction.


## Protein translation in 0.20: format 4

The writer now emits format 4, retaining strict format-1/2/3 readers. A settings
record follows the group catalog: code ID u32 (0 selects a custom code), frame
u8 (1–3), custom-presence u8, optional custom name T (maximum 1024 bytes), 64
amino-acid assignments T and context-stop bitmap u64. Then region count u64,
and each region's start u64, width u64 and phase u8 (0 noncoding; 1/2/3 cyclic).
Regions are ordered, nonoverlapping, nonempty and limited to 100000. Absent
metadata in old sessions uses standard code 1, frame 1, no manual overrides.
Bit 0 of a code's context bitmap denotes TTT; codons use NCBI TCAG order.

Each version-4 row appends a protein-presence u8 after the original trace fields
(or directly after a zero trace reference). If present, the payload is:
three-letter-label preference u8; source sequence ID u64; source symbol
fingerprint u64 (FNV-1a); source document state u64; source name T; the complete
translation settings record; span count u64; then each span as cell count u8
(1–3) and three u64 column coordinates. The first coordinate stores the canonical
one-letter AA token; other cells are structural padding. Span coordinates are
ordered and bounded by the row, with no overlapping token intervals. Unmapped
cells must be gaps. Original codon coordinates/settings also provide provenance;
edited snapshots do not regenerate from sources.

Representation 1 means codon-spaced protein tokens and requires a valid protein
layout. Representation 0 remains ordinary aligned symbols; compact translated
proteins can retain provenance and a label preference with no spans. Metadata
requires a protein row without a trace. Protein spans are limited to 16,777,216
per row. Earlier applications reject format 4 at the version check.

View JSON adds an optional `translation` object: enabled, letters, three, ruler
and rememberedMode. These are presentation preferences; genetic codes, frames,
annotations, spans and provenance are mandatory binary semantics. The fragment
view object adds a `columns` boolean, distinguishing ruler column selection from
a row-local rectangle. Column deletion/minimization moves semantic annotations
with retained columns; row-local editing leaves them fixed.

Native fragment clipboard JSON now writes version 2 with the existing physical
`rows` strings and optional `proteinLogical` canonical one-letter text. When the
target is protein, Paste uses the canonical text to avoid interpreting structural
padding as additional gap residues. The plain text still retains alignment
coordinates. The reader accepts versions 1 and 2. Fragment Cut/Paste is single-row
only; partial-token cutting is rejected before publishing or deleting anything.


## Translation simplification in 0.20.2 (format remains 4)

Only single-letter presentation is supported. The existing label-style byte in
format-4 protein metadata is reserved and written as 0. Values 0/1 remain accepted
from earlier files, then ignored. Optional old view JSON fields for three-letter
presentation are ignored. No version bump is required.

All newly created translated rows use representation 0 (ordinary aligned protein
symbols). Noncompact additions place the canonical AA at the second coding cell
of each codon and open gaps in other cells: ~M~~K~~*~ for ATGAAATAA. Gap and AA
cells are independent. Protein metadata retains source ID/name/fingerprint/state
and code/frame/annotation provenance, but contains no active span map.

Legacy representation-1 rows are validated before conversion. Each non-gap AA
moves from its old anchor to its middle mapped coding cell. Surrounding gap
states are retained, the active spans are discarded, and representation becomes
0. An empty legacy row likewise becomes an ordinary empty/gap-only protein.
Provenance survives. Ordinary imported protein rows are not recentered. Both old
single-/three-letter preference files migrate to this one editable form. Source
files are not rewritten until the user saves.

Native fragment clipboard writes physical alignment text. Historical
proteinLogical hints in older version-2 fragment payloads are ignored: a pasted
fragment has the same ordinary letters and gaps as its text, without implicit
span expansion or gap stripping. Native whole-sequence clipboard uses the same
validated migration and ordinary-row rules as sessions. Readers of versions
1–3 keep their previous preservation behavior.
