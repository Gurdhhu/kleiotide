# Focused Windows checks — 0.27.2 test 1

Build with the same Qt 6.5.3/MSVC2022 x64 Release kit in a fresh build directory.
Enable release checks; expected CTest result remains 38 passing entries.
Use the matching inner Source_For_Installer ZIP for test-installer, revision 1.

1. Find overlap and merge: Preview substitution/indel and partial components.
   Copy reads-to-contig alignment as FASTA, paste into a second Kleiotide window
   or BioEdit. Consensus is first, contributing originals follow, padded with
   ordinary '-' gaps to the same width. Excluded reads are omitted. Switching
   components changes copied data; invalidating Preview disables Copy. Source
   data and Undo remain unchanged. Test the aligned-merge dialog too.
2. Sequence menu and dialog use “Merge aligned sequences”. Motif search in
   View → Toolbars has a magnifier over nucleotides at all display scales.
3. Trace Current tooltip says Ctrl+Left/Right moves one column and Shift+Left/
   Right moves as far as possible under gap/lock rules. Confirm actual movement.
4. Hover a variable/conserved overview column, stop moving for several seconds,
   then move to another. Column details stay visible rather than being replaced
   by the general legend. Outside the plot, the general explanation is available.
5. Chromatogram has no title/close strip. Main toolbar and View → Toolbars still
   toggle it; inspect narrow windows and 100/125/150/200% scaling.
6. Add a .kleio via File → Add sequence from file; also drop one onto an existing
   alignment, and batch sessions/FASTA/AB1 files. Names, text, explicit row types,
   masks and linked original trace evidence remain, with new row IDs and one Undo
   for the whole batch. Imported rows are ungrouped. The current session's groups,
   code/frame/annotations and appearance remain. Imported session groups, code,
   annotations, protein snapshot provenance, view state, selection and reports are
   discarded, with a notice after success. Original source files are unchanged.
   A malformed file in a mixed batch must prevent the whole append and show no
   success notice. File → Open still opens a single complete session normally.
7. Save/reopen the combined .kleio and inspect linked traces, edits, Undo/Redo and
   source AB1 protection. No original AB1 files need remain present to view traces.

ARM real MAFFT Automatic/fallback testing remains pending for a later version.
