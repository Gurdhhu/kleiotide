# Windows check for Kleiotide 0.3.1

Extract to a new folder and open CMakeLists.txt with the existing
Qt 6.5.3/MSVC2022 kit. Build and run in Release / Выпуск. No new dependencies.

1. Double-click a long sequence name, or use Edit → Rename → Rename selected
   sequence. The dialog should be substantially wider. Rename, then undo.
2. Hover over the vertical border between names and residues. The cursor changes
   to horizontal resize. Drag it right and left, including from the header.
   Long names should fit better in a wider area. Check that existing name and
   fragment selections stay intact and no sequence edit is added to undo history.
3. After resizing, click residues in different rows, select/slide a block, paste
   at an Insert caret, undo and scroll. Cells should follow the moved border.
4. Set the alignment font to size 6. Ruler numbers should be complete at their
   upper edge, while residue rows stay compact. Check larger fonts again, and
   scroll to high column numbers. Tick lengths and labels remain aligned.
5. Open examples/overview_demo.fasta. Its five equal regions should show:
   conserved A: full-height blue; half gaps: half-height blue; varied A/C/G/T:
   full-height bright orange; all N: full-height gray; all gaps: gray baseline.
6. In Overwrite mode, replace one N in the all-N region with A. That column
   becomes blue, because the remaining N calls do not provide evidence of
   variation. Undo restores gray. Try lowercase and U; case is ignored and U
   counts as T for conservation.
7. Open examples/conservation_steps.fasta to see the intermediate colors. The
   seven regions have 100%, 95%, 80%, 70%, 50%, 25% and 100% called-base
   conservation. The last region has one A and nineteen N calls per column;
   it should be just as blue as the first region.
8. Recheck overview navigation and View-menu hide/show. Clipboard shortcuts,
   gap tools and usual scrolling should retain the confirmed 0.3 behavior.

Height includes every nongap symbol. Conservation uses only A/C/G/T/U calls.
N and other ambiguous calls are excluded; bins containing only ambiguity or gaps
are gray. When columns share a pixel, color averages only informative columns,
so no-information columns cannot make their neighbors appear more variable.
