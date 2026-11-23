# Trace import, linking and quality

Open AB1 files directly, or link them to nucleotide rows already in an alignment. Original files are never edited.

Select a residue in a linked row to view its chromatogram. The view clears
when the residue selection is empty or belongs to an unlinked row. To jump
to a traced sequence, choose it in the trace dropdown. Drag the grips above
the chromatogram and alignment overview to resize each panel.

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

The viewer and AB1 exporter use these positions consistently. Unsupported terminal runs use local spacing. Undo, edits, reverse complement and native sessions preserve the distinction between original-call anchors and inferred sample positions. Relink the original AB1 to recalculate an older mapping.

## Manually positioning calls

In Edit mode, drag a letter in the **Current** line to position it between its
neighboring calls. Sample spacing shows the adjusted position directly over the
original trace. The alignment letters and gaps stay in place. Select a range
in the alignment to highlight it in the chromatogram.

Click a deleted letter in **Imported** to place an insertion caret at its
original sample. For another location, click the signal plot or an open space
in **Current**, including between calls and in either flank. Type a nucleotide
to insert it at the indicated sample, before the next current base or after the
last one. The alignment caret shows where the base will be inserted. You can
drag the new call afterwards. Edited AB1 exports use its selected sample and
Q0. The original calls and their measured Q remain available in the trace.
Undo/Redo and format-6 sessions retain the edited positions.

## Double peak calling

Open **Sequence → Double peak calling** or use the sequence-name context menu.
Choose selected sequences or all sequences with chromatograms. The preview
examines non-gap residues in the **Current** row with an imported, inferred
or manually placed sample anchor. Positions with only an interpolated
location are counted in the status line.

**Exclude hand-edited bases** is selected by default. It protects substitutions, case changes and inserted calls. A manual correction back to the imported letter remains protected after saving and reopening. Automatic substitutions made by this version remain eligible on the next scan. Older sessions have no edit history, so Kleiotide protects differences from the original call (including case) and added calls. Clear the checkbox to review these sites too.

The default secondary peak threshold is **60%** of the major peak height.
Sites with a qualifying third peak at **30%** of the major peak height or
higher are excluded. Peak tops may be separated by at most **20%** of the
local base spacing by default. Displacement in samples counts the recorded
signal points between the two tops.

Kleiotide measures the peaks from the same zero line you see in the trace.
It uses the surrounding signal to check whether a local maximum has a clear
peak shape. If the tallest signal near the residue has no clear peak, the
site is skipped. Phred scores do not enter the calculation.

Select a proposed substitution to inspect its chromatogram. The table shows
the one-based alignment column, current and proposed symbols, the two bases,
the peak-height ratios, and the displacement. A proposal starts unchecked
if the current symbol differs from the detected pair, including when it is
a different ambiguity code. Review each such site before checking it.
**Apply** makes the checked substitutions in one undoable edit and preserves
the original imported evidence.

**Copy table** copies the calculation rules, Kleiotide version and preview
as TSV. **Save table** offers XLSX, TSV and CSV with the same information,
including which proposals are checked. The preview is a record of proposed
changes, and its checked column indicates what would be substituted on Apply.

## Quality editing

Edit → Trim low-quality ends and Low-quality calls to N provide automatic previews with collapsed rules. Only usable original quality is evidence; manual or inserted residues do not receive fabricated scores. Review the proposed edits and confirm Trim ends or Convert calls to N. Cancel leaves the alignment unchanged. Linked signal evidence remains available after edits and alignment.

See [edited AB1 export](AB1_EXPORT.md), [merging](MERGING.md) and [session format](../development/SESSION_FORMAT.md).
