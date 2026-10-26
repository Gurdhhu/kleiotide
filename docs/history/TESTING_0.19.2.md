# Windows acceptance — Kleiotide 0.19.2

Extract into a fresh `Kleiotide_0.19.2` folder and build with the usual Qt Creator /
MSVC kit. Qt 6.5 and newer remain supported, including the reported Qt 6.5.3 kit.
No session-format change or installer is introduced by this maintenance release.

## Toolbar priority

1. In a wide window, the sequence/column counts remain at the upper right, even
   when residues or names are selected.
2. Narrow the window: counts disappear **before** they take space needed by the
   mode switches, Right click selector and display controls. Widen it again and
   the current counts return automatically.
3. At very small widths, ordinary toolbar overflow can still be necessary for the
   controls themselves. The counts do not reserve space ahead of those controls.
   Check both Edit and View, and Windows DPI scaling.

## Mouse-block undo

- In Rewrite and Insert, select a multi-row residue block, drag it right or left,
  then release. Undo from Edit or Ctrl+Z should restore **both the symbols and the
  complete selected rectangle**: row set, starting column and width. Redo should
  restore the moved block and selection together.
- Repeat with a single-row stretch and a block that pushes locked gaps/residues,
  consumes open gaps or extends the alignment. Keyboard Shift/Ctrl sliding keeps
  its existing behavior.
- Display/palette/font changes must not suppress selection restoration. The drag
  interaction now ends before edit notifications; selection history is tracked
  independently of drawing-cache state.

## Whole-sequence shortcuts

| Shortcut | Command | Target |
| --- | --- | --- |
| Ctrl+Shift+C | Copy selected sequences | Selected sequence names |
| Ctrl+Shift+X | Cut selected sequences | Selected sequence names |
| Ctrl+Shift+V | Paste sequences | Whole-sequence clipboard payload or FASTA |
| Ctrl+C / Ctrl+X / Ctrl+V | Existing fragment commands | Residue selection/caret, independent of selected names |

The new bindings appear in Edit and the sequence-name context menu. Whole-sequence
copy/cut retains types, groups and trace links through the native clipboard
payload, with normal FASTA text alongside it. Copy/Cut with no selected names does
nothing. Paste uses the existing sequence insertion rules. Each Cut/Paste remains
one undo step. Multi-row fragment Cut/Paste stays disabled.

## Reported compiler warnings

The supplied warnings did not indicate a data-loss bug at these locations:

- `statistics.cpp` C4244: the nucleotide state was returned through an unsigned
  byte even though the helper's return type is unsigned. Its states are only
  1–15. The comparison helper now returns unsigned too, removing the narrowing.
- C4458: local/parameter names `data` hid Qt's inherited `QWidget::data`. They
  intentionally represented clipboard or chromatogram values. Those names are
  now `residueBytes`, `residueRows` or `traceData`.
- `main_window.cpp` C4456: a second `fragment` declaration unnecessarily hid the
  existing local reference. The existing reference is reused.

These are source corrections; no warning pragmas or disabled warning levels were
added. Linux builds and extra GCC shadow/conversion checks on the affected files
pass. A native MSVC build is not available in this environment, so check the
reported locations again with the Windows kit.

Microsoft diagnostic references:
[C4244](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-levels-3-and-4-c4244),
[C4458](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4458),
[C4456](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4456).

## Automated verification

All 12 CTest suites pass. New mouse-history and full-window tests pass in Release
and ASan+UBSan, including a display-cache invalidation that reproduced the old
selection failure. Actual Ctrl+Shift+C/X/V, typed/grouped clipboard roundtrips,
ordinary fragment-key independence, toolbar priority and 150% scaling are covered.
Previous editing/highlight, quality and trace GUI checks pass. Raster scrolling,
undo/redo and fresh repaint comparisons pass with backgrounds, identity dots,
site highlights and groups. Leak detection is disabled for this runner.

The next planned feature milestone remains protein coloring and translation.
