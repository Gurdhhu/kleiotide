# Version 0.14 — residue palettes and display modes

Extract into a fresh Kleiotide_0.14 folder and build with the existing Qt/MSVC kit.
No new dependency or native binary format version is required.

1. Open a mixed alignment with lowercase, IUPAC symbols, N, both gap types and a
   mask row. Default Classic colored letters should look as before.
2. Try each palette through View → Residue palette and the toolbar swatch dropdown.
   Check that both menus share their checked state, icons update, and hover help
   describes the controls. DISPLAY_COLORS.md explains the color choices.
3. Toggle monochrome and colored backgrounds from both View and the toolbar.
   Only one style is active; clicking the active toolbar toggle restores colored
   letters. Background lettering should be readable in black or white. Masks
   remain gray on white. Switch fonts, including size 6 and larger sizes.
4. Combine every style with identity dots. Matching cells should show dots with
   their real residue background where enabled. The top row still shows letters.
   Copy a substring or selected sequences: the clipboard must contain real bases.
5. In Insert, check a blue single cell and yellow multi-cell selection. In
   Overwrite, check a blue single-row stretch and yellow multiple-row block.
   Selections must remain readable over colored cells. Edit, undo and redo, then
   slide a block; the new styles must preserve the established editing behavior.
6. Scroll in both directions with a large block selected, including a block that
   extends past the viewport. Check ruler, selection borders and short row ends
   for stale pixels. Repeat at your normal Windows display scaling.
7. Open AB1 traces and switch palettes. Trace legend and channel curves should
   match. Monochrome/background toggles affect the grid, while the trace stays
   colored. The overview keeps its independent conservation colors.
8. Save and reopen .kleio with a nondefault palette, background mode and identity
   dots. All three choices should return along with selections and trace links.
   Open an older session: it should default to Classic colored letters. Toggling
   views should not change sequence data or add an undo operation.

Linux/offscreen validation covers actual rendered cells for all 12 palette/style
combinations, toolbar/menu synchronization, clipboard, trace legend, native-session
round trips and unknown preference fallback. Four style test groups cover exact
Classic inks, alternative ink contrast, automatic black/white foregrounds and
stable preference IDs. Windows interaction is checked with this list.
