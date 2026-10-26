# Version 0.10 — direct trace editing and alignment-space display

Extract into a fresh Kleiotide_0.10 folder. Use the existing Qt/MSVC kit; build
`kleiotide` as before. No new dependency or session format version is required.

1. Open your scrolled-label example. Current, Imported and Original Q now have
   opaque white backgrounds, drawn above letters, quality values and the caret.
   Scroll at different zooms and confirm that the labels remain readable.
2. Click a symbol in **Current**, then type in Insert or Overwrite. The toolbar
   modes have exactly the existing alignment semantics: Insert puts text after a
   single selected symbol; Overwrite replaces the selected single-row stretch and
   advances. View remains read-only. All keyboard commands use the shared editor.
3. Drag across Current, or Shift-click to extend selection. A multi-cell Insert
   selection remains yellow/non-editable; Overwrite single-row selections are blue.
   Ctrl+C copies the substring; Ctrl+V substitutes/inserts using the current mode.
   Delete/Backspace removes the selection; Ctrl+Z/redo restores it with provenance.
   Clicking an imported mapped peak selects its current symbol. Deleted imported
   calls remain inspectable but cannot be used as current editing targets.
4. Choose **Alignment spacing** beside Zoom/Gain. Current now displays actual
   alignment columns, including open/locked gaps. Surviving original peaks line up
   with their mapped current columns; signal intervals stretch/compress between
   them automatically when gaps/residues are inserted, deleted or slid.
5. Insert gaps in Current. In Select / Slide, select a stretch and use left/right
   arrows or Ctrl+arrows. Existing rigid sliding rules apply. Mouse dragging in
   Current selects a stretch; it does not independently distort the original signal.
6. Choose **Sample spacing** to inspect the original sample-axis spacing and
   deleted peak regions. Display stretching never rewrites/resamples stored signal
   arrays, re-basecalls data, or creates original peaks/quality for inserted bases.
   Imported and Original Q always describe imported evidence.
7. Select the linked sequence name and use Sequence → Reverse complement. Both
   displays reverse consistently with complementary channels. Check undo/redo.
8. Save/reopen .kleio: current edits, original evidence, strand and spacing choice
   are retained. Old 0.9 sessions remain readable; absent spacing settings default
   to Sample spacing. Reopening starts a fresh undo history as before.

## Display policy

Alignment spacing is a monotone piecewise-linear display transform anchored at
surviving imported peaks and current column centers. Inserted/gap cells have their
own column positions. Deleted peaks remain between neighboring surviving anchors.
Original signal ends are displayed at the row boundaries. Coincident imported
peak positions share the mean column of their surviving calls; they are not split
into invented independent signal peaks. All-deleted/synthetic-only rows have a
finite fallback transform. Compression keeps the original extrema within each
screen pixel; expansion interpolates only for rendering. Original stored samples,
peak positions, qualities and provenance remain unchanged by display settings.

Signals use 20 pixels/column at 100% in Alignment spacing; this is an alignment
column axis, not a requirement that the two panes use identical pixel scales.
Manual independent signal warping and free-residue compression/edit semantics
remain separate future refinements; this release implements automatic trace
stretching/compression from the existing alignment edits.

## Automated checks

CTest adds four coordinate groups to the existing 81 core/trace/session groups:
gap stretching, insertion/deletion compression, strand/inverse consistency,
coincident/all-deleted/synthetic cases and 200 randomized edits followed by a
session round trip. `kleiotide_stage010_check [WINDOW.png]` checks actual direct
selection/modes/typing/paste/copy/deletion/gap insertion/sliding/undo/redo,
opaque labels while scrolling, reverse complement, display settings and native
session restore. Earlier stage 0.6/0.8/0.9 GUI workflows remain regression checks.
