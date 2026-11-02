# Native session format (.kleio)

The current writer emits **format 5**. The reader accepts **formats 1–5**. Kleiotide 0.29.2 introduced format 5; versions through 0.29.1 cannot open these new saves. Loading older formats preserves their recorded mappings and migrates missing metadata; it does not automatically relink traces. Undo/Redo history is not serialized: reopening creates a new editing baseline.

Sessions embed names, typed rows, aligned symbols, stable IDs/order, groups, translation metadata, analyzed chromatogram channels, original calls/positions/quality and current sequence-to-trace mappings. Original AB1 paths are provenance labels: the files are not required on reopening. Raw instrument channels and unrelated vendor tags are not embedded. Source AB1 files are never modified.

## Primitive encoding

Integers use little-endian byte order; signed i32 values use two's complement. `T` is a u64 byte length followed by exactly that many bytes, without a terminator. Names, paths and text metadata are UTF-8; sequence/call symbols are ASCII. There is no implicit padding or compression. This encoding is independent of Qt serialization and host byte order.

## Format-5 file layout

| Order | Field | Encoding |
| --- | --- | --- |
| 1 | Signature | Eight bytes: `KLEIO` + CR + LF + `0x1A` |
| 2 | Format version | u32 = 5 |
| 3 | Next sequence ID | u64 |
| 4 | View state | T: optional JSON object, view schema version 1 |
| 5 | Last alignment report | T |
| 6 | Group count | u64 |
| 7 | Next group ID | u64 |
| 8 | Groups | Repeated: ID u64, name T, RGB color u32 |
| 9 | Translation settings | Record below |
| 10 | Original trace count | u64 |
| 11 | Original traces | Repeated trace records below |
| 12 | Row count | u64 |
| 13 | Rows | Repeated row records below |
| 14 | Checksum | u32 CRC-32/ISO-HDLC of all preceding bytes |

The checksum detects accidental corruption, not authenticity. Unexpected trailing bytes are rejected.

## Original trace record

In order: source path T, sample name T, imported ABIF call set u32 (1 or 2), imported calls T, peak count u64 followed by that many u32 positions, quality count u64 followed by that many u8 values, then four channel arrays in A/C/G/T order. Each channel is a u64 sample count followed by signed i32 amplitudes.

Peak count equals call count. Quality count is zero or equals call count. Channels have compatible dimensions and peaks are ordered and in bounds. Evidence shared by multiple linked rows is written once in first-reference order. Quality describes the original imported calls; it is not recomputed confidence for edited calls.

## Row record

In order: stable ID u64, kind u8 (0 nucleotide, 1 mask, 2 protein), representation u8, group ID u64, name T, aligned symbols T, original trace reference u64 (0 none, otherwise one-based index into the trace array).

If the trace reference is nonzero, continue with:

1. Strand u8: 0 forward, 1 reverse.
2. Original-call anchor count u64 and that many signed i32 call indices. Count equals current non-gap residue count. -1 denotes a current residue without an original call; other values are zero-based imported call indices. Surviving indices increase strictly on the forward strand and decrease strictly on the reverse strand.
3. Inferred-peak count u64 and that many signed i32 raw sample positions. Count is zero or equals current non-gap residue count. -1 means no inferred sample anchor. Nonnegative values are positions in the immutable original trace's sample coordinates, also on reversed rows. Anchors must satisfy range and oriented-order validation. Original-call anchors remain authoritative.

Every row then has a protein-presence u8 and, if present, the protein metadata record below. Trace links are nucleotide-only; protein metadata requires a protein row without a trace.

An inferred peak does not change an original-call index of -1 or create measured quality. This distinction is retained through edits, reverse complement, Undo/Redo and serialization. Unsupported original/inferred anchors remain absent. Ordinary import preserves older exported AB1 PLOC values; explicit relinking performs a new mapping.

Representation 0 means ordinary aligned symbols: one ASCII symbol per alignment column. Open gaps are `~`, locked gaps are `-`; input dots are normalized before storage. Protein symbols are canonical one-letter amino-acid codes, including B/Z/J/X/U/O, `*`, `?` and gaps. Automatic input classification is resolved before storage and is never a stored row kind. Invalid kind/representation/alphabet combinations reject.

Representation 1 is a legacy codon-spaced protein layout. The reader validates its spans, recenters residues and converts it to representation 0; new translated sequences use ordinary independently editable protein symbols.

## Groups and identifiers

Groups have nonzero unique IDs below the next group ID, unique nonblank single-line names with no tab/null, and RGB values 0x000000–0xFFFFFF. Group ID 0 means Ungrouped. All nonzero row memberships must resolve to the catalog. Row IDs likewise remain stable and unique; next-ID counters remain monotonic through editing. Groups are mandatory semantic metadata, not optional view JSON.

## Translation settings record

In order: code ID u32 (0 for custom), frame u8 (1–3), custom-presence u8, optional custom name T (maximum 1024 bytes), 64 amino-acid assignments T and context-stop bitmap u64. Then region count u64 and each region's start u64, width u64 and phase u8 (0 noncoding; 1/2/3 coding phase).

Regions are ordered, nonoverlapping and nonempty. Codon assignment order is NCBI TCAG; bit 0 of the context-stop bitmap corresponds to TTT. Missing translation metadata in old formats uses standard code 1, frame 1, without manual overrides.

## Protein provenance record

After protein-presence=1: reserved legacy label-style u8 (writer uses 0; reader accepts 0/1), source sequence ID u64, source-symbol fingerprint u64 (FNV-1a), source document state u64, source name T, complete translation settings record, span count u64 and each span as cell count u8 (1–3) plus three u64 alignment-column coordinates.

Legacy span coordinates are ordered, bounded and nonoverlapping; unrepresented cells must be gaps. The reader validates before converting bound layouts to ordinary symbols. Current translated proteins retain code/frame/annotation and source provenance without active spans. Noncompact additions place each amino acid at the middle coding cell, with ordinary gaps around it; compact translations use one amino acid per column. Provenance does not make a derived protein regenerate after source edits.

## Optional view JSON

View schema version 1 stores font, name-area width, scroll positions, name/residue selections, caret, trace visibility/active row/focused column, zoom/gain/spacing, residue palette/display, identity dots, highlighting scope/rules and translation-display preferences. IDs and column coordinates are decimal strings to retain 64-bit precision. Unknown or invalid optional view fields fall back to defaults without bypassing mandatory binary validation.

Editing mode values are 0 View, 1 Insert, 2 Overwrite (previously named Rewrite); a legacy 3 Slide maps to Overwrite. `editMode` remembers the editing submode while viewing. Highlight scopes use stable row-ID snapshots and fixed selected-column coordinates. Genetic codes, coding annotations and protein provenance belong to binary semantic metadata, not only display JSON.

## Migration by format

| Format | Added mandatory data | Reader behavior |
| --- | --- | --- |
| 1 | Original nucleotide/mask rows and trace mappings | Supplies representation 0, no groups, default translation settings |
| 2 | Explicit protein kind and representation | Retains declared row types; no alphabet guessing |
| 3 | Group catalog, next group ID and row memberships | Validates catalog and references |
| 4 | Translation settings and protein provenance/layout | Validates and normalizes legacy translated layouts |
| 5 | Separate inferred sample anchors on trace-linked rows | Preserves inferred positions without original-call/Q fabrication |

Old formats have no inferred-peak field and load with an empty inference array. Unknown future format numbers reject before interpreting the remainder. Saving always writes current format 5; keep an earlier copy when compatibility with an old application is needed.

## Limits and validation

Maximum file bytes before checksum: 8 GiB; rows/traces: one million each; groups/translation regions: 100,000; names, paths and view JSON: 1 MiB each; alignment report: 16 MiB; individual row: 1 GiB; calls, samples per channel and protein spans: 16,777,216. Declared lengths are checked and payloads read in bounded chunks before document installation. IDs/references, types, alphabets, call/peak/Q counts, channels, orientation/order, residue mappings, protein metadata, checksum and end-of-file all validate.

Normal session saves use atomic replacement. FASTA is separate sequence-only exchange; it cannot retain groups, translation provenance or trace evidence. External alignment programs receive FASTA and do not interpret .kleio.

## Clipboard formats

Whole-sequence clipboard MIME `application/x-kleiotide-sequences` contains a native session of selected rows and used groups plus normal FASTA text. Paste validates and remaps IDs/group collisions without overwriting existing groups. Immutable evidence is retained. Clipboard payloads are limited to 256 MiB.

Residue MIME `application/x-kleiotide-residues` is UTF-8 JSON version 2 with physical alignment `rows`; plain text joins those rows with newlines. The reader accepts versions 1/2. Legacy `proteinLogical` hints are ignored: fragments are ordinary aligned symbols, including independent gaps. Cut/Paste validates row alphabets before committing. Clipboard indexes and motif/highlight caches are not saved-session fields.
