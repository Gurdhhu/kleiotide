# Kleiotide 0.26 — acceptance checks

Extract into a fresh Kleiotide_0.26 folder. Open CMakeLists.txt with Qt Creator
15.0.1 and the usual Qt 6.8.1 / MSVC 2022 x64 kit; build kleiotide. No new
dependency is needed. Native Windows build/interaction must be checked here;
local validation used GCC 13.3 and a Qt 6.4.2 validation-only copy. The shipped
minimum remains Qt 6.5. See IDENTITY_GROUPS.md for the complete contract.

1. Open examples/identity_groups.fasta. Sequence → Groups contains
   Group by % identity…; Sequence → Select contains One sequence per group….
   The upper-right totals remain. The idle bottom-left corner is empty.
   Select a residue rectangle: its columns and row count appear at bottom left.
   Clear the rectangle: the idle bottom-left area becomes empty again.
2. Group by % identity: leave All sequences and Use existing alignment; use
   minimum identity 90%, minimum overlap 80%, prefix `Family_`. Click Preview
   groups. Expect three new groups: short_first, long_representative and
   variant_90 together (long_representative is the representative); shifted
   separately; different separately. missing_only is skipped. There are six
   scoped rows, five assigned and one skipped. Neither existing names nor source
   sequences are changed by preview.
3. Apply, then Sort → By group or open Manage groups to inspect memberships.
   Undo once restores the previous catalog/memberships; Redo restores the groups.
   Save a .kleio session, reopen, and inspect names, colors and memberships.
   Apply a second grouping using the same prefix: new names avoid collisions;
   old catalog entries remain, even if their membership becomes empty.
4. Undo to the ungrouped baseline. Repeat with Align each query–representative
   pair (internal), identity 90%, overlap 80%. Expect shifted to join
   long_representative: two groups total, with different separate. No MAFFT or
   VSEARCH path is requested. Apply preserves all original gaps and symbols.
5. Select only short_first and shifted in the name pane. Choose selected scope
   and pairwise alignment, 100% identity, 80% overlap. The longer shifted row is
   representative; only these two rows are assigned. Others retain memberships.
   Singletons are assigned their own groups, not silently dropped.
6. Change identity, method, overlap, scope or prefix after a preview. Apply must
   disable until a new preview is calculated. Empty prefix gives a clear error.
   Cancel after a valid preview keeps groups unchanged. Open Calculation details
   to inspect identity/missing-call and greedy-centroid rules.
7. For sequences sharing only a short high-identity end, compare overlap 0% and
   80%: the higher overlap setting should keep insufficiently overlapping rows
   apart. At 0%, a one-residue overlap can qualify by design. Internal indels count
   per residue; terminal mixed N/?/gap regions are excluded. N–N is not a match;
   other ambiguity symbols are literal. U and T are equivalent nucleotides.
8. Try pairwise mode on long divergent sequences. The window must stay responsive
   and Stop calculation must discard the unfinished preview. Cancel/Escape or
   closing the dialog while running must interrupt and exit without group edits.
   A workspace-limit error must leave the document unchanged.
9. With the two-group pairwise result, choose One sequence per group…. First
   selects short_first and different. Longest selects long_representative and
   different (the equal-length shifted row comes later). Random previews one
   member per group; Draw again produces another uniform draw, which may repeat.
   Select sequences must select exactly the displayed draw. Cancel keeps the
   previous selection. The alignment and Undo history are unchanged.
10. Include one Ungrouped representative to include missing_only as well. Empty
    named groups produce no selected row. Re-sort the alignment and verify First
    and longest-length ties use the new current order. Groups containing masks
    are included in representative selection. Check both dialogs at Windows
    display scaling 100%, 150% and your usual setting; columns can be resized.

For automated core checks, configure with KLEIOTIDE_BUILD_GUI=OFF and run CTest.
The optional GUI workflow executable is built with
KLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON, target kleiotide_identity_group_check. It
accepts an existing screenshot directory as its optional first argument.
