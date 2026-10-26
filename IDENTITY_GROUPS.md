# Identity grouping and representatives — 0.26.6

Sequence → Groups → Group by % identity… creates previewed groups from all
sequences or the selected sequence names. Column/residue selections do not
restrict this operation. The defaults are existing alignment, 97% identity,
80% minimum shorter-sequence overlap and prefix `Identity_`. Pairwise mode defaults
to at most 16 word-ranked centroid candidates per query; set this to 0 for
exhaustive centroid search.

## Greedy clustering

1. Trim terminal runs of gaps and missing calls from both ends. Missing means
   nucleotide N/n or ?, and protein X/x or ?. A composite N/?/gap run is terminal
   until the first informative residue; internal missing calls are retained.
2. Sort the eligible rows internally by decreasing trimmed ungapped length.
   Internal missing residues count toward length. Equal lengths keep the
   current alignment order. The visible alignment is never reordered.
3. Compare each row with centroid candidates. Existing-alignment mode and
   exhaustive pairwise mode use centroid creation order. Fast pairwise mode
   uses the word-ranked shortlist described below. It joins the first
   candidate that meets both identity and overlap thresholds. Otherwise it becomes a new representative, including singletons.
   Representatives are fixed; this is centroid clustering, not transitive
   single-linkage clustering. Only the necessary query–representative pairs
   are calculated; there is no retained all-pairs matrix.

Nucleotide and protein rows are clustered separately. Masks and rows with no
informative residues are reported as skipped and retain their old membership.
With existing alignment, compact and codon-spanned protein rows are kept
separate; pairwise alignment can compare their biological residue strings.

## Identity and overlap

- Identity is 100 × exact matching symbols / compared alignment columns.
  Case is ignored and nucleotide U is normalized to T for comparison only.
- Leading/trailing overhangs and terminal missing regions do not contribute.
  The existing-alignment mode compares the intersection of the two trimmed
  coordinate spans. Ragged ends are absent evidence.
- An internal residue-versus-gap column is a difference. Open and locked gaps
  have the same comparison role. Double-gap columns are omitted.
- Internal columns with N/? (protein X/?) are omitted from both numerator and
  denominator. Other ambiguity codes are literal: R–R matches; A–R differs.
  N–N is not evidence of identity. Protein N is an ordinary amino acid.
- At least one informative residue-to-residue column is required. A pair with
  no such column cannot join a group even at a zero identity threshold.
- Overlap is the covered ungapped length of the shorter trimmed sequence,
  divided by its full trimmed ungapped length. Internal missing residues count
  toward covered length but not identity. At equal lengths, use the smaller
  covered count. In existing alignment, an internal gap in either row counts
  as an aligned indel, not a terminal overhang.
- Both thresholds are inclusive. Minimum overlap 0% permits even a one-residue
  overlap to qualify. Increase it, for example to 80%, when grouping sequences
  intended to overlap over most of the shorter row. An exact contained fragment
  has 100% overlap relative to the shorter row.

The preview shows original sequence names, proposed groups, representatives,
identity, overlap and skipped reasons. Identity/overlap display to two decimal
places; thresholds use the underlying counts, not rounded table text. Hover over
those columns for exact match/comparison counts.

## Internal pairwise alignment

Existing gaps are removed without changing the document. The aligner uses
affine-gap semiglobal dynamic programming: +2 exact match, −4 substitution,
−20 to open a gap and −2 for each additional gap residue. Missing calls score
zero. End gaps in both sequences are free. Alignment starts on the top or left
edge and ends on the bottom or right edge; there is no local score reset.
The highest scoring alignment determines identity. Score ties favor more
matches, then fewer compared columns, then more paired residues, with fixed
iteration order for remaining ties. The identity threshold does not change
alignment scoring. Current sequence orientations are used; reverse complements
are not searched automatically.

An exact-fragment KMP search handles identical/contained normalized strings in
linear time. Otherwise the full alignment uses three rolling rows and stores
only scores and counts. Time per comparison is O(nm), workspace O(min(n,m)),
with a 128 MiB guard for that workspace. For combined read lengths up to 8,190,
score and deterministic tie-breaks are packed into one ordered 64-bit integer;
coverage travels with the winning path. This accelerates the same full affine
recurrence. Longer pairs use the original rolling-cell implementation.
No banding or edit-distance approximation changes the selected alignment.

### Fast centroid candidates

Pairwise mode defaults to **16 maximum candidates**. An inverted index records
unique 8-base words for nucleotide centroids and 3-residue words for protein
centroids. Nucleotide ambiguous words are omitted; protein words containing X/?
are omitted. Repeated words count once per sequence. Query/centroid candidates
are ranked by number of shared words, with centroid creation order breaking
ties. An exact normalized centroid string always leads the shortlist, including
short and ambiguity-rich strings. At most the chosen number receive a full
affine alignment; the first qualifying candidate wins. If no candidate passes,
the query starts a new cluster. Short/seedless nonexact queries fall back to
the earliest compatible centroids up to the same limit. Current orientations
are used, and nucleotide/protein indices are separate.

This is a heuristic search, **not guaranteed to reproduce exhaustive groups**.
It can choose a different qualifying centroid even when all centroids fit in
the shortlist, because their order is based on shared words. With a limit, it
can also miss a qualifying centroid outside the shortlist and form an extra
group. Every accepted member must still pass the full identity/overlap test.
Increase the limit for sensitivity; choose **0 (All / exhaustive)** to restore
0.26's centroid search order and coverage. This is especially useful for low
identity thresholds, short overlaps and ambiguity-rich sequences. The dialog
shows how many queries actually had their candidate list truncated.

Normalized input strings, the membership plan, centroid strings and inverted
word postings use O(total residues + rows + indexed centroid words) storage.
There is no all-pairs matrix or external executable. With a fixed candidate
limit, full alignments are bounded by that limit per eligible query, rather
than quadratically many. Word counting/ranking can still inspect postings
across many centroids; exhaustive mode retains the quadratic worst case.
Stop calculation and Cancel interrupt either search without applying a partial
plan. Preview export is available only after a complete calculation.

This is inspired by VSEARCH's length-ordered centroid workflow and its identity
denominator excluding terminal gaps. It is not a VSEARCH compatibility mode:
ambiguity handling, end-gap scoring, candidate ordering, masking and search
heuristics differ. No VSEARCH source code or dependency is included. Primary
references for the comparison are the [VSEARCH clustering documentation](https://github.com/torognes/vsearch/wiki/Clustering),
[identity/search manual](https://torognes.github.io/vsearch/dev/commands/vsearch-usearch_global.1.html)
and [pairwise alignment parameter manual](https://torognes.github.io/vsearch/dev/misc/vsearch-pairwise_alignment_parameters.7.html).

## Applying groups

Names are prefix + an integer starting at 1; retained existing names are skipped. UTF-8
prefixes are supported (nonblank, single line, at most 256 bytes). Each new group
gets a fresh stable ID and a palette color. Eligible scoped rows are reassigned. Old groups that become empty are removed
from the catalog, including groups that were already empty. Groups still used
by rows outside the scope or skipped rows are retained with their names and
colors. Undo restores the entire previous catalog and memberships; new names
can reuse the names of removed groups, while IDs are always fresh.

Preview is read-only. Changing settings invalidates it. Apply checks source
identity/revision, then saves all assignments and new groups in one Undo step.
Undo restores memberships and the previous catalog; Redo restores the same
group IDs. Retired group IDs remain reserved even after Undo, as elsewhere in
Kleiotide. Sequence IDs, names, symbols, row order, trace/protein evidence and
native session format 4 are preserved. Save a .kleio session to retain groups.
The group catalog retains its existing 100,000-group limit.

Group validation and membership application now use linear passes and hashed
lookups, so applying a large grouping does not perform a row search for each
member. Group assignments are stored in source order; history restores that
order before undoing/repeating the group edit, including compound insert edits.

## Saving the preview and managing members

Save preview… writes the displayed result to UTF-8 TSV without applying it.
It includes every preview row, sequence name, proposed group, group centroid,
identity, overlap and result, plus stable sequence/centroid IDs to disambiguate
duplicate names. Cells are quoted and embedded quotes doubled, preserving
Unicode, tabs and line breaks in names. Skipped rows have blank numeric cells.
Result labels are **New group centroid** and **Group member**. An incomplete,
invalidated or stale preview cannot be exported. The file is committed atomically.

Grouped rows in the group manager's sequence list show colored markers for their
actual staged membership and group-name tooltips. Ungrouped rows are unmarked.
Markers update after membership and color edits, regardless of the group selected
on the left, and retain their color when rows are highlighted.

In Manage groups, every named group and Ungrouped shows its live staged sequence
count. Counts include all names regardless of the name filter and update after
individual, bulk and highlighted-subset edits. Count suffixes are display-only.
Delete empty groups removes all named groups with zero staged members. It does
not delete Ungrouped or groups whose members are hidden by a filter. Cleanup is
staged with other edits: Cancel discards it, and Apply supports one Undo/Redo.

In Manage groups, **Check all** and **Uncheck all** check/uncheck membership
for the names currently visible under the name filter. Hidden rows keep their
memberships. Checking moves a visible row from its old group to the chosen
group; unchecking removes it from the chosen group only. These buttons are
disabled for Ungrouped. Mouse dragging, Ctrl and Shift can highlight multiple
names; Check selected / Uncheck selected changes only that visible subset.
Highlighting alone does not change membership. Clear and delete all groups
requires a warning confirmation (default No) and stages removal of the entire
catalog and all memberships. Edits remain staged until Apply and use one Undo
step; Cancel discards them, including a confirmed clear.

### Importing group tables

Import accepts a tab-separated table. For more than two columns, choose the
Sequence name column and Group name column using the numbered selectors. When
First row is a header is checked, selectors also show header names. Two-column
tables use column 1 for names and column 2 for groups automatically. Both mapped
columns must exist on each row and must differ; other values are ignored for
assignment. Changing the mapping refreshes the background preview before Apply.
Standard double-quoted fields are decoded (including doubled quotes and embedded
tabs), then a balanced enclosing parenthesis pair is removed from each cell.
The preview and matching use decoded values; internal parentheses remain literal.
A saved clustering preview can be imported directly by selecting Sequence and
Proposed group. Extra centroid/statistic/ID columns do not affect assignments.
Preview validation runs after a 200 ms pause in typing, on a worker using an
independent title/ID/catalog snapshot. The table uses a virtual model without
per-cell allocation or content-based column resizing. Editing cancels old work,
invalidates Apply immediately, and suppresses stale results. Cancel/close is
safe with pending or active validation. Valid assignments still apply atomically.
Input is limited to 16 MiB, 100,000 nonempty data rows (optional header not
counted), and 4,096 columns; errors explain when to split or reduce a larger table. Existing matching and
duplicate-title policies remain unchanged.

## One sequence per group

Sequence → Select → One sequence per group… shows the exact rows to select:

- First: first member in the current alignment sorting order.
- Longest: most ungapped residues, counting unknown calls and excluding both
  gap types/structural padding. Equal lengths keep the first current row.
- Random: a uniform reservoir draw per group. Draw again refreshes the preview;
  Select sequences uses the displayed draw without drawing again.

All nonempty named groups participate, including groups containing masks.
Empty groups are skipped. Ungrouped is excluded by default. Check **Also select
from Ungrouped…** and choose **One sequence** (using the same chosen rule) or
**All sequences** (every Ungrouped row). Preview order follows the group-manager
catalog, with Ungrouped last and its rows in current alignment order. Selecting
the preview never reorders the actual alignment. Core result IDs remain in
current alignment order. The command replaces name selection only;
it does not remove rows, edit groups, alter the residue selection or add history.
Cancel preserves the previous selection.

## Status bar

Without a residue selection, the bottom-left status area is empty during idle
use. A residue selection still shows its column range, row count and applicable
slide hint. Sequence/column totals remain at the upper right; temporary operation
messages can still use the status bar.
