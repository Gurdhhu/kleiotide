# Windows acceptance — Kleiotide 0.19.3

Extract into a fresh `Kleiotide_0.19.3` folder and build with the usual Qt Creator /
MSVC kit. Qt 6.5 and newer remain supported. Use Release when comparing scrolling
and selection performance. Native .kleio format remains 3.

## Undo navigation

1. In a large alignment, edit residues well away from the top-left: rewrite or
   paste a substring, insert symbols, delete a stretch/block, lock/unlock gaps, or
   slide a block. Scroll far away in both directions and select another area.
2. Use Edit → Undo or Ctrl+Z. The original row and affected columns should come
   into view automatically. The actual edit and existing selection restoration
   still behave as before. Mouse-slide Undo restores the complete original block
   selection, including all selected rows; navigation must not collapse it.
3. Scroll away again and Redo: its affected area should also be revealed.
4. Repeat on ragged rows, a terminal insertion/deletion, masks and traced rows.
   Whole-row transformations/quality edits reveal the first actual changed area
   when a patch contains an unchanged prefix. If an affected range is larger than
   the viewport, its beginning is shown.
5. Undo/Redo renaming, groups or row reordering: these metadata operations must
   not cause a residue-navigation jump. Selected sequence names remain selected.

## Selection responsiveness

- Maximize on the 3440×1440 screen. Grow and shrink a rectangle by dragging in
  View, Rewrite and Insert. Repeat at font sizes 6 and 11, at the usual Windows
  DPI scale, with letters and residue-colored backgrounds.
- Check single-row and multi-row selection, Shift extension, ruler column
  selection, offscreen auto-scrolling, and dragging an existing block to slide.
  The selection should follow the mouse without accumulating a visible delay.
- Check that old outlines disappear, symbols do not darken, and changing between
  blue single-row/caret and yellow block selections leaves no stale pixels.
- Check clipboard action availability, selected-name independence, motif scope,
  trace following, toolbar counts and mode switches after selection changes.

The optimization repaints changed cells and row outlines, skips identical
selections, validates IDs through the view's live-row cache, and refreshes only
selection-dependent window state. A tile touching several dirty strips is drawn
once to avoid compositing translucent glyphs repeatedly.

## Optional automated checks

Enable `KLEIOTIDE_BUILD_GUI_SMOKE_CHECK` and build
`kleiotide_stage0193_check`. It opens a real MainWindow at 3440×1440 with 2,000 rows
× 4,000 columns, times growing/shrinking mouse selections in letters/background
styles, compares the actual raster backing store with full repaints, checks
font-6 blue/yellow transitions, and exercises undo/redo navigation and metadata
non-navigation. It requires Qt's offscreen raster platform for pixel checks:

```text
QT_QPA_PLATFORM=offscreen kleiotide_stage0193_check
```

The `--benchmark-only` argument skips navigation assertions for comparison with
the previous implementation. There is no timing pass/fail threshold: timings
depend on the machine, build, platform and scaling. See VALIDATION.md for measured
Linux results and the remaining Windows verification boundary.
