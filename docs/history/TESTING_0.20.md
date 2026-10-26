# Kleiotide 0.20 acceptance

Build into a fresh folder using the existing Qt >=6.5/MSVC C++20 kit. Runtime
installer work remains the final roadmap stage. Native format 4 reads formats
1–3; old applications cannot read new sessions. Keep original files when trying
a new version. Export FASTA for sequence-text interchange.

1. Open nucleotide alignments with masks, proteins and AB1 links. Toggle
   Translation in the main toolbar. Check codon colors, all 27 table IDs,
   frame 1/2/3, live stops and the detailed count tooltip. Change code/frame,
   Undo/Redo, edit nucleotides, reverse complement and slide blocks. Stops must
   update while masks/proteins do not contribute. Scroll/select: no recount.
2. AA code enters View; one-/three-letter labels retain the same widths. Edit
   returns nucleotide labels and keeps codon colors. Ctrl+C copies underlying
   nucleotides. Copy translated fragment requires complete codons in one row.
3. Select columns and apply 123, 231, 312, noncoding. Explicit phases survive
   frame changes. Reset selected/all annotations. Try 1,NC,2,3 split codons,
   invalid phases, leading/trailing partial codons, GCN, TAR, TGN, NNN, all-gap,
   mixed-gap and ragged rows. Invalid phases block snapshot creation unless
   annotations are explicitly ignored. Tables 27/28/31 retain sense translation
   and indicate context-dependent termination separately.
4. Select nucleotide names and Add translated sequences via Sequence or the
   name context menu. Check live preview names/stops, frame/code, annotation
   choice, one-/three-letter display, group inheritance and _TRANSLATED suffixes
   with numeric collision handling. All additions undo/redo once. Source edits
   leave snapshots independent. Only nucleotide sources keep traces.
5. Click a protein continuation cell: the full AA is selected. Rewrite/Insert
   accept one-letter codes, Delete removes complete tokens, arrows advance AA
   cells. Multi-row partial-token edits must reject without mutation. Expand
   selection to complete amino acids explicitly, then edit. Slide must move
   structural padding with the AA and consume only biological open gaps.
6. Open a compact translated protein window; three-letter display occupies one
   biological AA cell. Normal FASTA keeps alignment coordinates and one-letter
   anchors. Export compact proteins removes only structural padding. Native
   whole-sequence clipboard retains layouts, types and groups.
7. Save/reopen native format 4 with manual phases, a custom genetic code, mixed
   rows, groups, snapshots and traces. Rename/move original trace files: evidence
   must still open. Reopen format-1/2/3 sessions; no type reclassification.
8. At 3440×1440, 100% and 150% scaling, leave traces, overview and highlights
   open. Select large blocks and scroll with NT/AA/three-letter translation.
   Repeat with font size 6, narrow the window, and check ruler numbers/positions
   and essential editing controls. Compare with translation off.

Automated Linux validation: CTest core/GUI suites, translation_check, prior
stage0193_check and trace_selection_check. The translation check covers controls,
code Undo, preview and atomic snapshot actions, canonical protein edits, narrow
controls/font-6 ruler, cached pixels, and wide trace-open selection timing. Qt
6.4.2 is used only in a separate local validation copy; shipped minimum stays 6.5.
Windows/MSVC and real-monitor DPI acceptance remain user-side checks.
