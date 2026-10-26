# Version 0.17.3 — visible Edit/View states and selection gap locking

Extract into a fresh Kleiotide_0.17.3 folder and build the `kleiotide` target with
the usual Qt 6.8.1/MSVC kit. Minimum Qt remains 6.5. Sessions remain format 3,
with readers for formats 1/2/3. This is a source package rather than an installer.

1. A new window shows joined Edit and View segments simultaneously, with Edit
   highlighted blue and Overwrite selected immediately to the right. Click View:
   View becomes blue, Edit becomes neutral, and both the editing-mode and
   right-click dropdowns are disabled. The latter shows Right click: menu.
   Click Edit to restore the remembered Overwrite/Insert/Slide submode and
   right-click choice. Clicking the active segment leaves it selected. Confirm
   both segments are keyboard-focusable and work with Space. Check 100%/150% DPI.
2. In any Edit submode, select a rectangle containing letters, open gaps (~) and
   locked gaps (-), including several rows and a short/ragged row. Run Sequence →
   Lock gaps in selection. Only '~' inside that rectangle becomes '-'. Letters,
   outside cells and row lengths stay intact; short rows are not padded. Select
   unrelated sequence names as well: they must not change the affected rows.
3. With Right click: menu selected, repeat via Lock gaps in selection in the
   fragment context menu. Unlock gaps in selection is alongside it. Both actions
   work for single-row and multi-row selections in Overwrite, Insert and Slide.
   In View, these editing options are unavailable. Undo/Redo the whole rectangle
   in one step; applying Lock to an already locked region adds no history.
4. Repeat with nucleotide, protein, mask and trace-linked rows. Original trace
   calls, qualities, signal samples and residue mapping must remain intact.
   Save/reopen .kleio and verify gap states. Save while in View and reopen:
   View remains selected and Edit returns to the remembered editing submode.
5. Recheck fragment Ctrl+C/X/V, explicit sequence clipboard menus, block sliding,
   right-click gap actions after returning to Edit, undo and smooth scrolling.

The next planned milestone remains trace-quality end trimming and low-quality
calls to N. The implementation roadmap is unchanged apart from these refinements.
