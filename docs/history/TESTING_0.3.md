# Windows check for Kleiotide 0.3

Extract to a new folder. Open CMakeLists.txt with the same Qt 6.5.3/MSVC2022 kit,
choose Release / Выпуск, build and run. No additional installation is needed.

1. Open examples/pushing_demo.fasta. Clear selected names, choose Select / Slide,
   select CG in push_right, and press Ctrl+C. Paste into a text editor: plain CG.
   Include gaps or lowercase in another selection and check they are retained.
2. Select a rectangle over two or more rows without selected names. Put a known
   word in the clipboard first; Ctrl+C should leave that word unchanged.
3. Select two sequence names and press Ctrl+C, with or without a residue block.
   Pasting into a text editor should give those two full FASTA records.
4. Choose Insert and click inside a sequence. Copy the following raw text from
   a text editor, then Ctrl+V. Line breaks and spaces should disappear, the string
   should be inserted before the caret, dots should become ~, and following
   symbols should shift right. Type one more base: it belongs after the paste.

       aCGU Nry
       ~-.?

5. Ctrl+Z should undo the whole paste in one step (undo any extra typed base first).
   Ctrl+Y should restore it. Try pasting at the row end and near its beginning.
6. Copy AC12GT or arbitrary prose: Ctrl+V should change nothing. Raw nucleotide
   paste in View, Overwrite or Select / Slide should also change nothing.
7. Copy a complete FASTA record beginning with > and Ctrl+V: it still adds a row,
   including in Insert mode. Undo. File → Add from clipboard and New alignment
   from clipboard remain available. Copied FASTA files retain their old behavior.
8. Check ruler ticks at columns 1–10: each column has a tick, 5 is longer,
   10 is longest and labeled. Scroll far right and change font size; labels and
   ticks should stay aligned with residues.
9. Open examples/overview_demo.fasta. The overview's five successive regions
   show: conserved nongap bases; half gaps with conserved bases; varying bases;
   ambiguous N; and all gaps. Conserved regions are blue, ambiguity orange,
   all gaps gray; the half-gap region has half-height bars.
10. Click the middle or right end of the overview and drag back left. The view
    should follow and the narrow outlined frame should track its visible range.
    Hover over regions to see column ranges, gap density and conservation.
11. Paste or slide past the current end, transform a row, add/delete rows and
    undo/redo: the overview should recalculate and its extent should follow.
    Rename, change selection or scroll: the existing plot should remain ready.
12. Hide/show the overview from View → Alignment overview. Check your usual
    scrolling with it visible and hidden, including on a large real alignment.

The overview includes all rows. Missing cells in shorter rows count as gaps.
Case is ignored, U votes as T; ambiguity symbols are nongaps but do not vote for
one nucleotide. Hover values are averages when several columns occupy one pixel.
Large alignments briefly show calculation progress. Input remains available;
an edit restarts the calculation for the changed alignment.
