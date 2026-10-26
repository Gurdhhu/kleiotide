# Kleiotide 0.25 — Windows acceptance

Extract into a fresh `Kleiotide_0.25` directory. Open CMakeLists.txt with the usual
Qt Creator Desktop Qt 6.8.1 / MSVC 2022 kit and build `kleiotide`. C++20, Qt Widgets
and Concurrent remain the dependencies; the shipped minimum remains Qt 6.5.

1. Open `examples/merge_demo.fasta`. Select only **aligned_left** and
   **aligned_right**, then Sequence → Merge selected sequences….
   Expect `AGATACACAGATATAG`, 5 overlapping informative positions, no conflicts
   and default name `aligned_left_merged`. Rename and Apply. The row appears at
   the bottom; both original gap layouts remain. Undo once and Redo once.
2. Select **chain_left**, **chain_middle**, **chain_right**. Use Find overlap and
   merge…. The initial minimum 20 is too high; set it to 4 and click Preview.
   Expect the same contig, source starts 1/5/8 and ends 9/12/16. Apply; the new
   contig starts at column 1. Existing rows stay in place. Undo once.
3. Select **full_length** and **contained**, exact minimum 4. Expect the same
   full contig. Deselect, reverse their row order and repeat. Results agree.
4. Select **conflict_A**, **conflict_G**. Aligned merge gives `RCGY`, 4 overlap
   positions and 2 residue conflicts. Cancel leaves the document unchanged.
5. Select **gap_read**, **gap_fill**. Aligned preview gives `ACGT` and one
   internal gap conflict. An internal gap is not treated as a terminal gap.
6. Select **repeat_long**, **repeat_short**; exact minimum 4. Expect an ambiguous
   placement message and disabled Apply. A mismatch or a disconnected selected
   row likewise prevents Apply; no partial contig is added.
7. Open two AB1 files and align their current nucleotide rows. Select their names,
   use Merge selected sequences…, inspect ordinary IUPAC conflicts, then enable
   original trace quality and Preview. Check original Q in both chromatograms.
   A concrete highest-Q call must beat different observations by the margin.
   Ties/missing Q/insufficient margin retain IUPAC. Try margin 0, 5 and 10.
   Q controls are unavailable if one selected row has no trace, and always
   unavailable in Find overlap and merge….
8. Change one conflicting call manually or insert a new call; its original Q
   must not resolve that position. Reverse-complement a linked source and insert
   gaps before another conflicting position; the correct original Q still maps
   to unchanged calls. Originals and their traces stay intact after Apply.
9. Clear the result name: Apply becomes disabled. Enter a custom name: Apply
   returns without another Preview. Changing the Q margin or minimum overlap
   disables Apply until Preview finishes. Cancel during a long overlap search;
   the dialog remains responsive and no row is added.
10. Save/reopen a native session after each kind of merge; original traces/groups
    and the new consensus remain. Undo/Redo the merge before saving. FASTA export
    contains the new sequence without synthetic trace data.

Also check layout at 100% and 150% scaling, including long source titles. The
source placement table should scroll; result/summary and action buttons remain
readable. The implementation has been built and exercised offscreen under Linux
Qt 6.4.2 in a validation-only copy; native Windows acceptance is still required.

Automated validation: `ctest` includes the new `merge` suite. Configure
`KLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON`, build `kleiotide_merge_check`, and run it with
an optional screenshot-output directory for the dialog workflow checks.
