# Windows check — Kleiotide 0.6

Historical checklist: in 0.7, Insert adds after the selected symbol and keeps the
last inserted symbol selected. See TESTING_0.7.md for the current behavior.

## Build

Extract Kleiotide_0.6_Source.zip into a fresh folder. Open its CMakeLists.txt in
Qt Creator, keep Desktop Qt 6.5.3/MSVC2022 64bit and Release, and build Ctrl+B.
Use a new build directory. The archive retains the historical timestamps that
corrected the previous Ninja regeneration loop. No new dependencies are needed.

## Selection and editing

1. Open examples/editing_statistics_demo.fasta. In each of the four modes, drag
   a substring and a rectangle over several rows, including the mask. Shift-click
   to extend/shrink from the initial anchor. Drag the ruler for columns, and
   Shift-click its later columns. Dragging at viewport edges should scroll.
2. With no names selected, Ctrl+C on a single-row substring copies exact text,
   including case and gap symbols. A multiple-row block does nothing. Selecting
   names retains the existing FASTA-copy behavior.
3. Switch modes with a selection active. Its range should remain selected and
   its color should change according to the following table.

| Mode | One cell, one row | Stretch, one row | Block, multiple rows |
|---|---|---|---|
| View | Yellow | Yellow | Yellow |
| Insert | Blue | Yellow | Yellow |
| Overwrite | Blue | Blue | Yellow |
| Select / Slide | Yellow | Yellow | Yellow |

An append caret at a row's end remains blue in editing modes. A cell beyond a
short row's end has no editable caret; select an actual cell or the row-end caret.

4. In Insert, select one symbol and type or Ctrl+V a sequence-like string. It
   inserts before that symbol and advances to the next original symbol. A wider
   stretch or block must reject typed/pasted input. Row-end insertion must work.
5. In Overwrite, select 2–4 symbols in one row. Type one symbol to replace the
   whole selection; the next typed symbol overwrites the following cell. Undo.
   Paste a longer string and then a shorter string; the selected range alone is
   replaced, the suffix shifts, and the caret follows the replacement. A block
   across rows must reject typed/pasted input. Masks accept their usual symbols.
6. In Insert/Overwrite, select a stretch or mixed mask/nucleotide block and press
   Delete or Backspace. It removes selected cells and shifts suffixes left. One
   Undo restores every row, original gap state and selected block; Redo repeats
   the removal. Short rows must not cause an error. View must not type or delete.
7. Double-click a residue: its whole nongap run is selected. Double-click a gap:
   the whole adjacent run is selected even if open and locked gaps are mixed.
   Try both nucleotide and mask rows, in every mode.
8. Select several names and press Esc: all names become unselected. It also clears
   any residue selection and cancels a row drag without moving rows. Sequence
   selection commands should appear only in Sequence → Select. Ctrl+F should open
   the name-regex selector; try `^one$` on the example.

## MAFFT hover help

Open Alignment → MAFFT settings. Hover over Method, Threads, Nucleotide direction
and each advanced parameter. Brief descriptions should explain their effects.
Where a parameter is disabled for the current method, hover over its label.
Whole/column alignment, result windows and cancellation should still work.

## Statistics

Open Alignment → Statistics on the example, with any selection and mode.
Expected values (all nucleotide rows, excluding the longer mask):

| Statistic | Value |
|---|---:|
| Total sequences | 4 |
| Unique sequences | 4 |
| Columns | 9 |
| Distinct patterns | 8 |
| Parsimony-informative sites | 1 |
| Singleton sites | 3 |
| Constant sites | 5 |
| Gap-only sites | 1 |
| N-only sites | 1 |

Click Copy and paste elsewhere: all nine labeled counts should be on one line.
The source must remain unchanged. Close/reopen after editing to recalculate.
Try a larger alignment; the interface should keep repainting while computing,
and closing the statistics window should cancel and re-enable the source.

Counts follow IQ-TREE's nucleotide rules. A site is constant when all calls share
at least one compatible base, even if ambiguities differ. Definite bases alone
contribute to parsimony-informative counts; variable non-informative sites are
singletons. N/? and gaps share its missing state for patterns and uniqueness;
gap-only and N-only are included in constant sites and are separately reported.
N-only allows gaps but requires at least one N/?. Mask-only extra columns are
excluded. Short nucleotide rows have virtual missing ends. No IQ-TREE install
is required by Kleiotide.

Primary reference for the counting convention: IQ-TREE's `buildStateMap`,
`computeConst` and `countConstSite` in
https://github.com/iqtree/iqtree2/blob/v2.4.0/alignment/alignment.cpp .
MAFFT help follows its official manual and multithreading documentation:
https://mafft.cbrc.jp/alignment/software/manual/manual.html and
https://mafft.cbrc.jp/alignment/software/multithreading.html .
