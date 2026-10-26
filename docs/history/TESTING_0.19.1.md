# Windows acceptance — Kleiotide 0.19.1

For revised toolbar priority, drag undo and sequence shortcuts, see TESTING_0.19.2.md.

Extract into a fresh `Kleiotide_0.19.1` folder and build with the usual Qt Creator /
Qt 6.8.1 MSVC 2022 64-bit kit. The shipped Qt minimum remains 6.5. This release
refines the confirmed 0.19 editor; protein presentation/translation remains next.

## Mode switches and sliding

- The top panel shows **Edit | View**, then spacing, **Rewrite | Insert**, then
  spacing and the Right click behavior dropdown. Both labels of each switch stay
  visible, with the active segment colored. New windows start in Edit / Rewrite.
- View disables the Rewrite/Insert switch and Right click dropdown. Edit restores
  the previous edit submode and right-click behavior. Clicking an active segment
  leaves it selected. There is no separate Slide mode.
- In **both** Rewrite and Insert, select a stretch or rectangular block, then drag
  inside the selection to slide it. A drag preview does not change data until
  release. Solid-block pushing, open-gap consumption, locked-gap/residue pushing,
  alignment extension and one-step undo are retained. Start a new selection outside
  the existing selection, or Shift-click to extend it.
- Plain Left/Right/Up/Down moves the caret using the previous Insert/Rewrite
  navigation behavior. A rectangular selection collapses to a single caret.
  **Shift+Left/Right** slides the selected residues/block one column.
  **Ctrl+Left/Right** retains movement across the available adjacent open gaps.
  View never changes sequence data with these gestures.
- In each edit mode, Right click: menu includes a separate group of four movement
  commands, with Shift/Ctrl arrow hints. Copy/Cut/Paste rules remain: Copy is
  single-row; Cut is single-row in Edit; Rewrite can paste over a single-row span;
  Insert pastes only after one selected symbol or at an unambiguous empty caret.
  Block Cut/Paste remains disabled. Name selection does not affect fragment keys.
- Rewrite substitutes the selected single-row span and advances the caret. Insert
  inserts after the selected symbol. Deletion, Backspace, gap locking, residue
  colors and undo/redo retain their existing behavior. Trace Current keyboard
  editing uses the same mode and Shift/Ctrl movement rules.

## Highlight scope

Open `examples/site_highlight_demo.fasta`. Select row1 and row4, then choose
Highlight → Analysis scope → Current selected sequence names and Apply. Enable
Variable sites. Only row1 and row4 receive site shading. Other nucleotide rows
and the mask remain unshaded. The ruler still marks the analyzed columns.

The explicit snapshot behavior remains: later selecting another name does not
change the analysis inputs or shaded row set. Reapply Analysis scope to capture
different names. All-row scope shades all rows of the chosen molecule type.
Masks and other molecule types never contribute or receive this shading.

Save/reopen a native session: highlight row scope and color/rules return. Old
sessions storing Slide reopen in Rewrite. Old View sessions that remembered Slide
retain View, with Rewrite as their edit submode. Binary session format remains 3.

## Menu shortcuts and alignment dimensions

- File/Edit/Sequence menu commands retain their assigned shortcuts. Context menus
  show native keyboard hints for fragment Copy/Cut/Paste, selection clearing,
  Select all, Delete selected sequences and movement. Whole-sequence Copy/Cut/
  Paste intentionally have no Ctrl+C/X/V hints; these keys act only on fragments.
  Esc clears both name and residue selections when used in the alignment.
- The **upper-right** panel always shows total sequence and column counts, even
  during residue/column/name selection. Totals include proteins and masks, as
  before. Check a narrow window, font/DPI scaling, insertion, deletion, row paste,
  alignment extension and undo/redo. Selected-range information remains below.

## Automated coverage

The 0.19.1 GUI check exercises both edit modes, dragging and Shift/Ctrl sliding,
caret movement, pushing/extension, undo/redo, context groups/hints, scoped-row
painting, mode switches/spacing, narrow-window dimensions and old/new sessions.
Run it in Release and sanitizer builds; run high-DPI and viewport regressions.
The existing composition, quality, clipboard/group, trace and core suites remain
applicable. Linux checks do not replace acceptance on the Windows kit.
