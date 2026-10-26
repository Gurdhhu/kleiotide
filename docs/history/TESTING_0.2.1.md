# Windows check for Kleiotide 0.2.1

Build/run with the same Qt kit in Release. Open examples/pushing_demo.fasta.

1. Double-click any name, enter a new name, then Undo. The clicked row alone
   should be renamed, even if several names were selected previously.
2. Select some names, then click below the last row in the name panel: clear selection.
3. Choose Select / Slide. Click row 1 column 3, then Shift+click row 2 column 4.
   A 2×2 rectangle should be selected. Shift+click row 3 column 5 extends it from
   the original cell; Shift+click a nearer corner shrinks it. Also test drag selection.
4. Escape. In push_right select columns 3–4 (CG). Press Right three times.
   A~CG-T~G~~A becomes A~~~~CG-TGA. The internal block stays CG; locked dash and
   residues are pushed, downstream open gaps consumed. Undo all three steps.
5. In push_left select columns 9–10 (TG). Press Left three times.
   A~B-~C~~TG-T becomes A~B-CTG~~~-T. The dash and C are pushed; upstream gaps
   consumed. Ctrl+Z should restore each step.
6. In right_extension select columns 2–3 (C-). Move right six steps (or drag).
   AC-GT becomes A~~~~~~C-GT. Scroll to see the extended alignment, then Undo.
7. Select a rectangle across multiple rows and slide both ways. Rows should move
   equally; residues and locked gaps must be preserved. Compare with your screenshots.
8. Select two names. Copy sequence titles, paste into a text editor. Then Copy
   to clipboard (FASTA format), paste into a text editor or a new Kleiotide alignment.
   Only those rows should appear, in original order, with full headers/case/gaps.
9. Open View → Alignment font. Only family, size, style and preview should appear.
10. Recheck your usual scrolling and the already-working gap/sequence menu actions.

Normal arrows/drag now push. Ctrl+arrows and maximum-movement context commands
still jump only across adjacent open gaps, preserving the working 0.2 behavior.
