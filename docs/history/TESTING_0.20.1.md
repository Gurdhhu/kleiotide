# Kleiotide 0.20.1 acceptance

Build into a fresh Kleiotide_0.20.1 folder using your usual Qt >=6.5/MSVC kit.
Native session format remains 4, compatible with 0.20. All earlier translation,
editing, snapshot, native-session and trace checks in TESTING_0.20.md still apply.

1. Open both genetic-code selectors: the main translation toolbar and Add
   translated sequences. Labels must be non-editable. Verify IDs and concise
   names; hover popup items for full names, especially codes 4, 6, 9 and 11.
2. Select Custom code… at the end of either list. Cancel must keep the previous
   code and make no document edit. Accept must create a selectable custom entry
   before the final command. From the main toolbar, code creation is undoable.
   From a snapshot preview, the custom code is local to translated snapshots;
   alignment defaults do not change. No separate Custom code toolbar button.
3. Verify the one-/three-letter switch shows a blue L cell above Leu. Its tooltip
   still describes switching code lengths. No change in alignment cell widths.
4. Set selection coding appears as a rectangular, two-line button; coding menu
   commands and annotation semantics are unchanged.
5. With translation active, inspect all 20 standard amino acids in gap-rich and
   variable regions, with NT letters, one-letter AA and three-letter AA labels.
   Every AA must retain its own color, regardless of coverage or conservation.
   Repeat with imported protein rows, including leading/gappy fragments. Verify
   both Letters and Backgrounds; Monochrome deliberately remains monochrome.
   Unknown/ambiguous AA symbols are neutral gray; actual gaps have no color fill.
6. Copy translated fragment is disabled in Alignment → Translation and the
   fragment context menu when Translation is off, no fragment is selected,
   a protein/mask or multiple rows are selected, or a codon is cut by selection.
   With active, completed translation and complete codons in one NT row, it is
   enabled and copies canonical AA text. Ctrl+C still copies underlying data.
7. Repeat at 100% and 150% scaling and narrow window widths. Keep traces open,
   drag selections and scroll; old responsiveness and cache correctness remain.

Optional Linux UI check target: kleiotide_translation_controls_check. It tests
both custom-code paths, menu availability, >98%-gap color fixtures, all standard
amino acids in NT/AA/three-letter views and imported proteins. The original
kleiotide_translation_check retains wide trace-open and cached-pixel checks.
