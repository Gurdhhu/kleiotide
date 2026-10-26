# Version 0.8 — AB1 import and linked chromatogram navigation

Use a fresh extracted source folder and your existing Qt 6.5.3/MSVC2022 kit.
Build/run the `kleiotide` target as before; no new dependency is required.

1. File → Open: choose either supplied AB1 file. Its filename becomes the sequence
   title. The chromatogram opens below the alignment/overview. Check calls and
   four channels (A yolk yellow, C blue, G black, T red).
2. Click an alignment residue or move the editing caret. The trace centers on its
   linked original peak. Click a trace peak: the corresponding alignment column
   is selected and brought into view. Try this in all four editing modes.
3. Scroll the alignment or navigate with its overview. The trace follows the
   visible region when the selected residue is outside that region. The trace's
   own scrollbar/wheel pans independently; clicking a peak resumes linked selection.
4. Change Zoom and Gain. These affect display only. Previous/next buttons navigate
   imported calls in the displayed strand direction. Original Q values are shown
   where available; numbers are spaced apart when zoomed out.
5. Add the other AB1 with File → Add sequence from file. This dialog accepts several
   AB1/FASTA files together. Switch linked rows with the Trace dropdown or select
   their alignment residues. Dropdown row numbers distinguish duplicate titles.
   Rename/reorder either sequence and confirm its trace follows the row.
6. Insert open/locked gaps and slide residues or blocks. Anchors follow the bases,
   rather than fixed alignment columns. Undo/redo must restore the connection.
7. Overwrite a residue. Its original signal/call remains intact. The current line
   shows the replacement, marked as edited; the position description identifies
   the original call and does not assign its quality to the substitution.
8. Insert new residues. They have interpolated display anchors, with no claimed
   original peak or quality. Delete a residue: its original signal/call stays
   visible, with the deleted imported call struck through. Clicking/navigating
   that original call reports its deletion, rather than selecting a neighboring base.
9. Reverse complement the linked row. Signal direction and A/T, C/G channels
   reverse together. Check undo, and reverse complement twice. A palindromic row
   must still change strand orientation.
10. Use whole/selected-column MAFFT or target/template addition. The result window
    retains the same original trace and its updated column links; the source stays
    unchanged. Whole-row MAFFT strand adjustment also reverses the trace orientation.
11. View → Chromatogram hides/shows the panel. Hiding survives ordinary selections
    and edits. Delete the last trace row: the pane clears/hides; undo restores the
    attachment, which can then be shown again from View.
12. Save: choose a **FASTA** filename. Export contains sequence text only; reopening
    that FASTA does not restore trace attachments or their edit provenance. The
    imported AB1 is never used as the default save destination, and chromatogram
    filenames/ABIF files are rejected as FASTA export destinations.

This is the original plan's first read-only trace stage. Existing alignment editing
works with trace provenance, but direct trace editing, alignment-space stretching/
compression and a native session format are subsequent stages. Original signals
are displayed in their native sample coordinates and are never resampled.

## Automated validation

Core: configure with `-DKLEIOTIDE_BUILD_GUI=OFF`, build, then run CTest. The `traces`
test uses synthetic ABIF evidence and malformed-input fixtures. Optional real-file
validation: `kleiotide_trace_tests FIRST.ab1 SECOND.ab1`.

GUI: configure with `-DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON`. Run
`kleiotide_stage08_check FIRST.ab1 SECOND.ab1 TRACE.png WINDOW.png MAFFT_LAUNCHER`
with an available Qt platform. The MAFFT argument is optional. This checks paired
navigation, zoom/decimation, edited/inserted/deleted anchors, strand/undo, both real
imports, hide/show and real whole/column/template jobs. User research files are
not included in the source ZIP; all bundled binary test fixtures are synthetic.
