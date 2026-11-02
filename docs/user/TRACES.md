# Trace import, linking and quality

Open AB1 files directly, or link them to nucleotide rows already in an alignment. Original files are never edited.

## Trace assignment

Sequence → Link trace (AB1) offers selected nucleotide names or all nucleotide
rows without traces. Selected rows can replace existing links; proteins/masks
are excluded. The selected-name context menu also offers Link trace and Delete
trace. Delete removes session associations only, with confirmation and Undo available;
sequence letters and original files remain. Undo restores the links.

Add a batch of AB1 files. Suggestions use the first matching tier: exact filename
stem, mutual substring of name/stem, exact internal AB1 sample name, then mutual
substring of internal name. Comparisons are case-sensitive. Ambiguous best-tier
matches stay unassigned, even if a weaker tier could pick one. File cells are
searchable dropdowns with visible arrows; single-click to choose, or use the per-row Add AB1
button to load and assign exactly one file. Several rows can use the same evidence;
unassigned rows/unused files are allowed. Imported AB1 recovery notes appear in
file-cell tooltips. Filenames are shown before directories; duplicate basenames
are disambiguated by directory. The popup can be wider than the column. Assignment
reasons record filename/internal-name/manual choices; unassigned rows say Not
attempted and recommend manual assignment. Original calls, peaks, qualities and channels remain immutable.

Preview checks forward and reverse-complement orientations after removing open
and locked alignment gaps and normalizing case/U. A bounded local affine
alignment gives terminal trimming/additions free ends. Large queries use
13-base seeds to select candidate windows before alignment. There is no attempt
to globally align a whole genomic assembly to a short trace: query cap is two
million ungapped residues, and each alignment window is limited to eight million
DP cells. Preview output/workspace also respects the bulk-memory setting and a
256 MiB cap; loaded AB1 evidence has a separate 512 MiB cap (or the lower bulk
limit), with a 64 MiB per-file input bound. Automatic name matching yields to
the UI between small chunks. Reduce the scope/trim flanks when these bounds are exceeded.

Equally strong orientations or distinct repeat placements are refused. Within a
placement, all optimal affine traceback paths are considered: a call retains an
original index only when those paths agree. Insertions and ambiguous near-indel positions have -1, without measured quality. Supported terminal additions can receive separate inferred signal anchors, as described below. Unique exact full or trimmed-call
matches retain the original indices, including N/ambiguous calls, when the
orientation is unambiguous and there is enough concrete evidence. Full-call
palindromes do not establish orientation. This is an overlap heuristic, not a
statistical claim that the biological sample identity has been established.

The table reports Forward/Reverse/No match and Perfect/Good/Terrible/Failed.
Perfect means a unique exact full or trimmed-call match or a fully identical mapped query with
no indels. Good uses at least 90% overlap identity; Terrible is weaker accepted
matching evidence (at least 65%). Short/insufficient evidence, ambiguity or bounds
failure gives Failed. Colors are green/amber/red. Review Terrible assignments
and actual traces carefully. These categories are not Phred scores, probabilities,
or a guarantee of mapping correctness. Review the signal as well as the category, especially for edited or repeated regions.

Changing assignments starts a new automatic preview. Link traces checks the source revision and
commits valid links together. Unassigned/failed rows retain their
current trace associations; remove old links through Delete trace when desired.
Preview/file loading run in workers; Cancel leaves sequence data unchanged.
Linked evidence is embedded and deduplicated in native sessions. Confirming Link traces opens the chromatogram view in the current session.


## Terminal additions and signal anchors

When linking an edited sequence, supported concrete terminal residues can receive inferred signal positions. A bounded sequence/spacing/dominance search preserves monotone peak order; homopolymers require distinct signal maxima. Unsupported or ambiguous additions remain unassigned. This heuristic does not create an original call index or a measured Phred score.

The viewer and AB1 exporter use these positions consistently. Unsupported terminal runs use local spacing rather than being stretched across the entire remaining signal. Undo, edits, reverse complement and native sessions preserve the distinction between original-call anchors and inferred sample positions. New sessions use format 5. Relink the original AB1 to recalculate an older mapping; opening a previously exported AB1 does not silently rewrite its stored peak positions.

## Quality editing

Edit → Trim low-quality ends and Low-quality calls to N provide automatic previews with collapsed rules. Only usable original quality is evidence; manual or inserted residues do not receive fabricated scores. Review the proposed edits and confirm Trim ends or Convert calls to N. Cancel leaves the alignment unchanged. Linked signal evidence remains available after edits and alignment.

See [edited AB1 export](AB1_EXPORT.md), [merging](MERGING.md) and [session format](../development/SESSION_FORMAT.md).
