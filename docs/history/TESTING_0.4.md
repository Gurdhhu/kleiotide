# Windows check — Kleiotide 0.4

Extract into a fresh folder, open CMakeLists.txt in Qt Creator and build Release
with the existing Qt 6.5.3/MSVC2022 kit. No new dependencies are required.
Open examples/masks_sorting_demo.fasta and use a disposable copy when saving.

1. Ctrl-select two nonadjacent names and drag either selected name above the
   first row, between rows, then below the last row. Both rows should move
   together, retaining their order and selection. Undo/redo each drop. Check
   normal single clicks, Shift selection, double-click rename, dragging near
   the viewport edges, and Escape cancellation.
2. Try all nine Sequence → Sort methods; undo each to restore the original.
   Title is case-insensitive; Alphabetically uses aligned sequence text.
   Physical length counts gaps, true length does not. Identical grouping
   ignores letter case but distinguishes gap types. Ties keep current order.
3. In Select / Slide select columns in a row, across a block, or via the ruler.
   Try summed-residue-frequency sorting: highest scores first, masks last.
   Without a selected range, this action should leave the alignment unchanged.
4. The digits, punctuation and E-containing example rows should load as gray
   masks. A/C/G/T inside those rows must also stay gray. Select a mixed mask/DNA
   block, slide it, undo, then type/paste digits and punctuation at a mask caret
   in Insert/Overwrite. Compare with ordinary nucleotide editing.
5. Select all names and reverse complement. Only nucleotide rows should change.
   Undo. Save/reopen and confirm mask text survives. FASTA has no type metadata:
   a mask edited into nucleotide-only text will reopen as a nucleotide row.
6. Overview: masks must affect neither residue percentage nor conservation.
   R contributes half A and half G; B contributes thirds C/G/T. A/R is 75%,
   all-R 50%, and A/N 100%. N-only columns are full-height gray; gap-only columns
   have no residue bar. Masks-only columns also have no residue bar. Use View
   to hide/show the overview, and check updates after edits and undo.
7. Confirm the previous smooth scrolling, divider adjustment, font-6 ruler,
   clipboard functions and rigid-block pushing still work with this fixture.

If an interaction differs, report the mode, selection, sort/drop destination
and a tiny before/after example. Automated validation is recorded in VALIDATION.md.
