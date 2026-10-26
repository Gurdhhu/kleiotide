# Version 0.11 — independent selection and identity dots

Extract into a fresh Kleiotide_0.11 folder and use the existing Qt/MSVC kit.
Build `kleiotide` as before. Dependencies and native session format are unchanged.

1. In Insert and Overwrite, select one symbol or a single-row stretch in the
   middle of a sequence. Backspace deletes exactly the selection and selects the
   preceding symbol. Delete deletes exactly the selection and selects the
   following symbol, which has shifted left. Repeat on gaps, at the beginning,
   at the last symbol and on the whole row; empty rows retain an end caret.
   Check undo/redo, and repeat in the chromatogram Current line.
2. Select several names, including nonadjacent rows. Click a residue or drag
   horizontally: only the clicked row's cells are selected. Drag vertically or
   Shift-click: the actual rectangle is selected, including intermediate rows
   whose names were not selected. Test all four modes and mixed DNA/mask blocks.
   Delete a block in an editing mode: only its rows change, with one undo.
3. Click/drag the ruler with one name selected: columns cover all rows. Change
   the selected names, or click blank space beneath the names: residue selection
   remains. Clicking/selecting Current leaves the selected names intact. Esc
   clears both selections. Copy priority remains as previously agreed: selected
   names copy FASTA; deselecting names permits single-row substring copying;
   multiple-row residue blocks alone leave the clipboard unchanged.
4. Enable **View → Identity dots to top sequence**. The top row keeps its letters;
   exact matching residues below it become gray dots, ignoring letter case.
   Differences, gaps and masks remain visible. The same IUPAC code matches itself,
   but a partial ambiguity does not match its individual bases; T/U are distinct.
   Short references do not hide trailing symbols. A mask in the top row disables
   comparisons until a nucleotide row is moved to the top.
5. Reorder rows, type/edit the reference and undo while dots are on. Comparison
   follows the current top row immediately. Select a dotted substring and copy
   with no names selected: clipboard text contains actual letters. Export FASTA
   and confirm actual letters; ordinary editing/MAFFT/statistics stay unchanged.
   Scroll a large alignment and toggle dots off to restore the normal display.
6. Save/reopen a .kleio session with dots on and independent name/residue selections.
   Both selections and the display setting return; actual sequence text is saved.
   Existing sessions lacking the optional setting reopen with letters.

## Automated checks

The optional `kleiotide_stage011_check` exercises directional deletion in both
editing modes, row boundaries, gaps, linked-call provenance, undo and end carets;
independent mouse/name/ruler/trace selection in every mode; actual dot raster
rendering, case/ambiguities/masks/ragged rows, cache refresh on editing/reordering/
undo, real-text copying and native persistence. `kleiotide_stage010_check` includes
direct Current-line Backspace and independent name selection regressions.

Build checks with `-DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON`; run core checks with
`ctest --test-dir BUILD_FOLDER --output-on-failure`. Optional GUI executables are
manual validation tools; Linux offscreen results do not replace Windows interaction
checks on the existing Qt/MSVC kit.
