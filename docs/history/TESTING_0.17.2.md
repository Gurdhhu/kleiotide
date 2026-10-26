# Version 0.17.2 — controls, context menus and fragment shortcuts

Extract into a fresh Kleiotide_0.17.2 directory. Build the `kleiotide` target with
the usual Qt 6.8.1/MSVC kit (minimum Qt 6.5). Native format remains 3 and readers
for 1/2/3 remain available. Existing sessions restore their saved editing mode.

1. A new window starts with the left button showing Edit and Overwrite selected
   immediately to its right. The dropdown contains Overwrite, Insert, Slide.
   Choose Insert or Slide, click Edit to enter View, then click View to return:
   the previous editing mode is restored. In View, the editing dropdown is
   disabled and right-click behavior has only Menu. Returning to Edit restores
   the prior right-click choice. Save a session while viewing; reopen it and
   confirm View and the remembered editing mode return correctly.
2. Right-click a selected fragment while Right click: menu is chosen. Check the
   following rules; multi-row fragment Copy/Cut/Paste is always disabled.

| Mode | Copy | Cut | Paste |
| --- | --- | --- | --- |
| View | Any length on one row | Unavailable | Unavailable |
| Overwrite | Any length on one row | Any length on one row | Any length on one row, substitutes it |
| Insert | Any length on one row | Any length on one row | One symbol on one row, inserts after it |
| Slide | Any length on one row | Unavailable | Unavailable |

3. Repeat the same operations through Edit → Copy fragment / Cut fragment /
   Paste fragment, and with Ctrl+C/X/V. Select several sequence names, including
   different rows from the residue selection. The shortcuts must behave exactly
   the same: they act only on the fragment and never remove or append sequences.
   Ctrl+C copies plain residue text with original case and gaps in every mode.
   Ctrl+X/V modify only one row in Insert/Overwrite; Insert also accepts an
   unambiguous bare caret, such as the boundary after cutting a stretch. With only
   names selected and no residue selection/caret, clipboard shortcuts do nothing.
4. Clipboard FASTA/native whole sequences cannot be imported by Ctrl+V or Paste
   fragment. Use Edit → Paste sequences or the name context menu. The name context
   also provides Cut selected sequences and Copy selected sequences. These whole
   row actions have no Ctrl+C/X/V shortcuts. Check same-window and cross-window
   copy/cut/paste of nucleotides, proteins, masks and trace rows: native metadata
   survives, the destination follows the last selected name (or appends with no
   names), and each Cut/Paste is undoable. Copy sequence titles remains available.
5. In View, select a fragment and right-click. Copy and Clear fragment selection
   are available; editing options are absent. Choosing a gap action in Edit then
   switching to View must never insert/delete a gap when right-clicking.
6. Select a residue rectangle and use Sequence → Unlock gaps in selection, then
   repeat from its context menu. Only '-' cells inside the selected rectangle
   become '~'; residue letters, gaps outside selection, row lengths and column
   positions stay intact. Works on mixed nucleotide/protein/mask rows and trace
   rows. Name selection is independent. Undo restores the whole change in one
   step. This action is available in Edit modes, including Slide.
7. Group identical sequences now compares sequences after removing every '-' and
   '~', ignoring case. For example, A-C~GT, acgt and AC--G~T must share a group
   when all are the same row type. A protein with the same letters is compared
   only with proteins; forced masks likewise. Names are irrelevant. Check that
   the grouping preview explains the rule, previous grouping is replaced,
   singletons become Ungrouped, row order stays the same, and Undo restores the
   old groups/memberships.
8. Recheck trace editing, independent name/residue selections, motif navigation,
   block sliding, Delete/Backspace, menu undo, scrolling and session save/reopen.
   At 100% and 150% display scaling, confirm the toolbar fits and group swatches
   and fragment outlines redraw correctly while scrolling.

The next planned milestone remains trace-quality end trimming and low-quality
calls to N. This refinement package contains source rather than an installer.
