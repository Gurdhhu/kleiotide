# Windows acceptance — Kleiotide 0.19.4

Extract into a fresh `Kleiotide_0.19.4` folder and build with the usual Qt Creator /
MSVC kit. Use Release for timing comparisons. Qt 6.5 and newer remain supported;
native .kleio format remains 3.

## Selection with chromatograms open

1. Open the alignment/session that showed the lag and leave the chromatogram
   viewer open. Maximize at 3440×1440. Drag to grow/shrink a single-row selection
   and a block across several rows. Repeat dragging both right and left, so the
   selection's starting column sometimes moves and sometimes stays fixed.
2. Repeat in sample spacing and alignment spacing, at 25%, 100% and higher zoom,
   and with Windows display scaling. Selection should track the mouse smoothly;
   the trace still follows the selected sequence and starting column.
3. Switch between traced sequences, select a block containing the displayed trace,
   and select untraced sequences. Mixed selections must retain the established
   trace-following behavior. Closing/reopening the panel must show current data.

## Trace cache correctness

- Rewrite, insert, paste, delete and slide residues/gaps in the alignment and
  directly from Current. Inserted symbols remain visible after moving the caret;
  original calls are still drawn even when they overlap.
- Undo/Redo these edits after scrolling away. The alignment navigates back and the
  Current line, original-call links, selection and caret update correctly.
- Reverse complement, switch traces, change zoom/gain/palette/spacing, scroll,
  resize and move between monitors/DPI scales. Check that signal, imported labels,
  current symbols, edited markers and sticky line labels redraw correctly.
- Imported calls, original quality and original signal remain immutable. Gaps and
  inserted symbols do not acquire invented quality or peaks. Deleted original
  calls remain available as evidence.

## Automated checks

With `KLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON`, build and run
`kleiotide_trace_selection_check` using Qt's offscreen raster platform. It opens a
3440×1440 MainWindow with two synthetic 1,000-call traces plus an untraced row.
It measures open/hidden panel, both spacing modes and both drag directions; then
compares cached/partial pixels with a fresh render, verifies visible labels and
trace following, and compares cached coordinates with the model through random
edits, reverse complements, gaps, coincident peaks and a long inserted run.
Zoom, gain, palette, font, resize and clearing the trace are exercised.

`--benchmark-only` skips correctness assertions after the timing workload for
comparison with older implementations. No timing threshold is imposed. Existing
direct trace-editing/session and 0.19.3 undo/selection GUI checks remain applicable.
Measured Linux results and the Windows verification boundary are in VALIDATION.md.
