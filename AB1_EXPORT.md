# Edited AB1 export — 0.24.3

File → Export edited trace (AB1)… creates a standalone ABIF 1.01 .ab1/.abi copy
from one trace-linked sequence. The dialog lists trace-linked nucleotide rows;
a single selected trace row is preferred, otherwise the active trace is used.
Choose the desired row and review its calls, edits, quality and orientation.

Export uses embedded evidence, so it also works after reopening a .kleio session
when the original AB1 file is missing. The source document, its Undo history,
saved/dirty state and imported trace files remain unchanged. Export refuses to
overwrite any existing imported source trace in the current document, including
an alias/symlink/hard link to it. Other destination files use the save dialog's
normal overwrite confirmation. QSaveFile commits a completed file atomically.
Cancellation before commit discards the temporary output. A completed commit is
reported as success even if a late cancellation arrives afterward.

## Calls and quality

- Every current non-gap symbol is stored, with case preserved. Alignment open
  and locked gaps are omitted. Deletions and low-quality trimming represented
  by gaps therefore remove calls from the exported sequence. A gap-only/empty
  row cannot export a useful AB1 and shows an explanatory error.
- PBAS1/PLOC1/PCON1 and PBAS2/PLOC2/PCON2 both describe the current edited sequence.
  They have identical lengths and values. Some readers choose the user-edited
  set, others only the basecaller set; mirroring avoids silently showing the
  pre-edit sequence. This is an edited export baseline, not a new instrument
  basecalling run. The CMNT tags document quality/metadata policies and the Kleiotide version.
- Unchanged, anchored calls retain their original measured quality. Case-only
  changes retain Q. Substitutions, insertions and calls without measured Q get
  Q0; no new confidence estimate is inferred from the peak signal. Restoring an
  original symbol restores its eligibility for the original Q.
- Original pre-edit calls/evidence and Kleiotide-specific metadata remain in the
  .kleio session; they are not a second conflicting call set in the AB1 export.

## Peaks and signal

Surviving original calls retain their original signal-sample positions. Inserted
runs receive linearly interpolated positions between surviving anchors, with
signal edges used at the ends or when no original anchor remains. These are
synthetic display positions, not newly measured peaks. Rounding or crowded
regions may produce coincident positions; all calls are still stored. Original
coincident positions remain coincident. Channels are never resampled to invent
extra signal or artificially separate inserted calls.

A reverse-complemented row exports in its current left-to-right sequence order.
Each channel is reversed in sample order and complementary channels are swapped
(A/T, C/G). Peak coordinates become last_sample - original_peak; qualities follow
their calls. Channel samples remain exact. Alignment-space stretching and display
zoom/palette settings have no effect on the exported signal.

DATA9–12 contain the analyzed channels in FWO_ = GATC order. Signals use signed
16-bit shorts, or unsigned 16-bit words when needed for nonnegative amplitudes
above 32767. PLOC uses 16-bit positions. Values requiring wider signals or positions
above 65535 are rejected explicitly, with no clipping/downsampling; keep those
traces in a .kleio session. A 64 MiB export limit bounds memory and matches the
AB1 reader's limit. Normal Sanger traces are well within these limits.

## Metadata and scope

This reconstructs a viewing/editing AB1 from the evidence retained by Kleiotide.
It is not a byte-preserving copy of the instrument file. Raw instrument/run fields,
raw pre-analysis channels and vendor-specific analysis arrays are not copied or
fabricated. Analyzed signals, edited calls, positions, conservative qualities,
dye order, sample label, spacing and exporter/version comment are written.

SMPL1 uses the current sequence title in a Pascal string, limited to 127 bytes
and truncated at a UTF-8 boundary when necessary. The sequence itself is never
truncated. The full title, groups and other session metadata stay in .kleio.
An exported file reopened in Kleiotide uses its edited sequence as the imported
baseline, as other AB1 readers do; the original pre-edit baseline is available
in the source session.

## Batch export

File → Batch export edited traces (AB1)… offers All traces and Selected sequence
names only. Selections refer to the name area; a residue block does not choose
batch rows. Non-trace rows are ignored. Selected scope with no linked rows never
falls back to All. Each linked row exports separately, even if two rows share
one original trace and have different edits.

Choose an existing output folder and review sequence/filename pairs. Drag the
column divider to resize the preview. Narrow windows prioritize output filenames;
widening restores the preferred source-name width. Filenames
use current titles plus the editable Filename suffix and .ab1. The default is
_edited; use your own separator, for example _trimmed, or leave empty for no
suffix. The preview updates immediately. Suffix input accepts up to 80 characters
and is disabled during export. Invalid filename characters are replaced,
long names bounded and reserved device names protected. Numeric suffixes avoid
case-insensitive name collisions and existing files (including directories and
symlinks). Existing source traces and previous exports are never overwritten.
If a path appears after preview, that file fails with a clear report.

Work runs in the background, one trace at a time. Each complete file is written
to a temporary file in the destination folder, flushed and renamed without
replacing an existing path. Cancellation discards the unfinished temporary file
and leaves completed outputs intact. One failure does not block later traces.
The final report lists successful paths, individual failures and unprocessed
counts. There is no claim that a whole batch commits as a single transaction.
Neither single nor batch export changes the document, saved state or Undo.

## BioEdit compatibility

BioEdit 7.7.1 scans every byte from the root directory offset to EOF for tag names,
rather than iterating only directory entries. The 0.24 comment contained PBAS/PLOC
text after the directory; the scanner interpreted it as entries and used text
bytes as lengths/offsets, reproducing an invalid allocation. Version 0.24.1 puts
all external payloads before the final directory.

The reader also identifies DATA9–12 by ordinal occurrence, and accesses multiple
old metadata fields unconditionally. DATA1–8 are present as empty arrays; they
contain no invented raw or instrument measurements. Required text fields say
Unavailable, model says UNKN, and unknown lane/SNR/analysis bounds are zero.
Valid date/time markers 1970-01-01 00:00:00 support readers rejecting zero dates;
CMNT2 explicitly labels these as compatibility placeholders. They are not the
original acquisition date. Full instrument metadata is unavailable in sessions.

Pascal metadata strings are limited to 127 UTF-8 bytes because BioEdit treats
lengths as signed bytes. SPAC1 stays a positive float; SPAC2 identifies edited
calls. Both sequence call sets continue to mirror the complete current sequence.

## Compatibility validation and limits

Biopython reads 24 generated exports, including edits of its 3730, 310 and A6_1-DB3
fixtures. Checks compare both call sets, exact edited calls, positions, Q and every
signal sample, with reverse orientation, crowded insertion, entirely new calls
and 1–4-call inline tags. Kleiotide round-trip tests also cover all supported Q
and 16-bit signal ranges. The prior 0.24 Staden checks remain documented in
docs/history/TESTING_0.24.md; current independent-reader results are in VALIDATION.md.

The unmodified x86 parser from the official BioEdit 7.7.1 executable is exercised
in a CPU emulator with file/allocation/string runtime stubs, stopping before GUI
formatting. Old exports reproduce an invalid allocation; revised exports pass
20 normal-length fixtures with exact calls, positions and all analyzed samples.
The harness fingerprints the parser and is shipped as a developer-only tool.
It does not execute the Windows UI or validate rendering, installation or every
BioEdit version. Native Windows testing in docs/history/TESTING_0.24.1.md remains necessary.

BioEdit and Staden assume PBAS is an external array, so standard ABIF exports
with only 1–4 calls are unsupported by those readers; Kleiotide and Biopython
read them. BioEdit also interprets signal amplitudes as signed shorts even when
ABIF specifies unsigned words: values above 32767 can display incorrectly there.
The exporter preserves actual values rather than silently clipping/scaling them.
Full ABIF Q bytes 0–255 round-trip in Kleiotide; Biopython has an upstream UTF-8
PCON decoding limitation at >=128. Independent fixtures use normal Sanger Q.

No instrument re-basecalling compatibility is promised. Export reconstructs a
viewing/editing trace from retained evidence, not a vendor acquisition file.

## Primary references and reproduction

- Applied Biosystems, ABIF File Format Specification and Sample File Schema:
  [vendor-authored specification mirror](https://github.com/BGI-flexlab/SOAPTyping/blob/master/doc/ABIF_File_Format.pdf).
- [Biopython's ABI reader](https://github.com/biopython/biopython/blob/master/Bio/SeqIO/AbiIO.py).
- [Staden io_lib ABI reader](https://github.com/jkbonfield/io_lib/blob/master/io_lib/seqIOABI.c).

Developer tools (not required for the application): build kleiotide_abif_export_probe,
run it with an output directory and optional real AB1 input paths, then run
`python tools/check_abif_compatibility.py OUTPUT_DIRECTORY` with Biopython installed.
The optional second argument is a Staden-reader JSON probe executable. No external
reader library is linked into Kleiotide and no new application dependency is added.
For BioEdit parser emulation, install developer-only unicorn, pefile and Biopython,
then run `python tools/check_bioedit_reader.py PATH/TO/BioEdit.exe OUTPUT_DIRECTORY`
with the official 7.7.1 executable from https://thalljiscience.github.io/.
The third-party executable is not redistributed in this source package.
