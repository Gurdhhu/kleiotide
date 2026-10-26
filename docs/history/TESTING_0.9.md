# Version 0.9 — complete residue labels and portable trace sessions

Extract to a fresh Kleiotide_0.9 folder and build `kleiotide` with the same
Qt 6.5.3/MSVC2022 kit. No new dependency is required.

1. Open the trace from your missing-residue screenshot. At 100% and lower zoom,
   every visible imported/current base must be drawn, including overlapping letters.
   Quality numbers can still be spaced apart to stay readable.
2. In Insert, add two or more bases. Move the caret elsewhere, change rows/zoom,
   reverse complement and undo/redo. Insertions remain in Current; they do not
   appear in Imported and have no original peak or quality. Clicking an inserted
   symbol in Current selects its alignment column.
3. File → Open now permits multiple selections. Select several AB1 files with
   Ctrl/Shift: they open as rows of one new document, with one trace per row.
   FASTA files can also be opened together or mixed with AB1; their row lists are
   concatenated without aligning/reordering residues. A single FASTA opens as before.
   Session files must be opened individually; Add continues to accept FASTA/AB1.
4. Look for the tiny waveform beside linked sequence names. Verify it survives
   rename/reorder and appears on reopened/MAFFT-derived linked rows. Hover on the
   sequence name to see the trace source. Name colors remain easy to read.
5. Edit residues/gaps, rename/reorder and reverse complement a trace row. Save
   with Ctrl+S: trace-containing documents default to `.kleio`. Add masks/ordinary
   FASTA sequences and save again. File → Export FASTA remains available for other
   tools; exporting does not mark an unsaved native session as saved.
6. Close/reopen the session. Check current sequence text, gap types, case, row
   types/order, edited/inserted/deleted original-call mappings, signals and qualities.
   Move/copy the source AB1 files out of their original folder first: the session
   must still reopen with its embedded trace data. The source AB1 is never changed.
7. Editing mode, font/name width, selection/caret, trace choice, zoom/gain and panel
   visibility resume. Undo history begins afresh after reopening; subsequent edits
   and undo/redo work normally. Save again without a new filename prompt.
8. Trace information now shows base calls and signal samples/channel. Confirm that
   internal PBAS/PLOC tag names are absent from the main information line.
9. Test Windows opening below; confirm a previous alignment stays in its own window.

## Opening selected files from Windows Explorer

The application now accepts all filename arguments and forwards nearby individual
shell launches to one process, collecting each burst into one new document. It
waits for 500 ms of quiet before opening the group. A later independent request
creates a separate window, leaving existing work intact.

Choose Open with → Choose another app → browse to the **built kleiotide.exe**.
The optional script `tools/windows/Register-Kleiotide.ps1` registers Kleiotide in
Open With for the current user and adds the multi-selection declaration, without
changing existing default applications. Example from PowerShell:

```powershell
.\tools\windows\Register-Kleiotide.ps1 -Executable 'C:\path\to\build\Release\kleiotide.exe'
```

Move/rebuild the executable elsewhere: rerun the script with its new path. Qt
runtime DLLs must remain available, just as when running the built app normally.
On Windows 11 the legacy menu may be under Show more options. Legacy shell verbs
can have a 100-item limit even with the multi-selection declaration; for larger
sets, use the unrestricted File → Open dialog. The script itself and Explorer
interaction require a Windows check; Linux validation tests the request queue and
batch-opening implementation, not Windows registry/shell execution.

## Automated checks

CTest runs 63 core cases, 12 trace groups and 6 session groups. The session groups
include exact evidence/ID/type/mapping round trips, missing-source independence,
new editing/undo, forced masks, bad IDs, truncation, version/checksum errors and
2,000 mutated inputs. `kleiotide_session_tests FIRST.ab1 SECOND.ab1` adds real-file
round trips. The optional `kleiotide_stage09_check DENSE.png SESSION.png` checks
actual multi-file Open/Save, continued editing after deleting source fixtures,
render label completeness, restored view state and external request coalescing.

Direct trace editing and alignment-coordinate stretching/compression remain the
next M7 substage; this release completes persistence and the requested corrections.
