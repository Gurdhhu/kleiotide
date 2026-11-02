# Edited AB1 export

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

Surviving original calls retain their recorded sample positions. Terminal additions with signal-supported inferred anchors use those positions; they remain synthetic calls with Q0. Internal inserted runs interpolate between surviving anchors. Unsupported terminal runs extrapolate local spacing instead of stretching across the entire remaining trace. Rounding or crowded regions may give coincident positions. No channel is resampled to invent signal or separate calls artificially. See [trace linking](TRACES.md).

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

## Reader compatibility

Exports are reconstructed ABIF 1.01 files with matching edited PBAS/PLOC/PCON sets. They are intended for viewing edited calls in external readers such as BioEdit and Chromas; they are not raw instrument archives. Verify important output with the reader/version used in your workflow. Synthetic interoperability tests do not establish every vendor reader's behavior. The analyzed evidence and the original pre-edit call set remain preserved in the native session.
