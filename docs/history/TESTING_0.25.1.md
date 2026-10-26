# Kleiotide 0.25.1 — acceptance checks

Extract into a fresh Kleiotide_0.25.1 folder. Open CMakeLists.txt with the normal
Qt Creator / Qt 6.8.1 / MSVC 2022 kit and build kleiotide. Dependencies and native
session format are unchanged. Earlier checks remain in TESTING_0.25.md, with the
case, partial-result and mismatch behavior superseded by this document.

1. **Sequence menu:** Set sequence type is first; a separator encloses Merge
   selected sequences, Find overlap and merge, and Collapse identical sequences.
2. **Result emphasis:** the merge dialog's result panel has a bold heading and
   colored border/background. Complete results are green, partial results amber,
   failures red. Rules are collapsed by default. Expand them if needed.
3. **Lowercase:** use these nucleotide rows, select both names, and try both
   current-alignment merging and overlap search (minimum 8, differences 0):

   ```text
   acgtACGTTAGC----
   ----ACGTTAGCttaa
   ```

   Expect acgtACGTTAGCttaa. Single-source informative positions retain case;
   overlapping informative calls use uppercase consensus. Unknown N/? calls do
   not force a lowercase flank to uppercase.
4. **Conflict highlights:** select conflict_A and conflict_G in
   examples/merge_demo.fasta. Expect red backgrounds at the two conflict columns.
   Previous/Next conflict moves through them, including after line wrapping.
   Q-resolved conflicts have green backgrounds. Unresolved indels are highlighted.
5. **Substitutions:** load examples/merge_refinements.fasta. Select left and
   substitution. Find overlap and merge, minimum 7, maximum differences 0 gives
   no contig. Set differences to 1 and Preview: acgtACGYTAGCttaa, one residue
   conflict, one edit event. Original rows remain unchanged after Apply.
6. **Indels:** select left and insertion, minimum 8, differences 1. Expect
   acgtACGATTAGCttaa, one indel conflict and one edit event. The retained inserted
   base is highlighted. Each inserted/deleted base costs one, so a two-base gap
   consumes two differences. Terminal overhangs do not consume the budget.
7. **Partial contigs:** select left, substitution and isolated, minimum 7,
   differences 1. The pair should merge even though the isolated read is longer.
   The banner says 2 of 3; the table identifies the excluded read. Apply and Save
   FASTA remain disabled until Use this partial contig is checked. Save FASTA
   writes exactly the preview and custom name, without modifying the document.
   Apply appends one consensus; one Undo restores the original row set.
8. **Separate contigs:** select left, exact_right, other_left and other_right,
   minimum 8, differences 0. The Result contig list has two choices. Switch it;
   preview, membership and exclusions change. Partial opt-in resets for each
   component. Apply appends only the chosen component.
9. **Failed searches:** repeat_long plus repeat_short from merge_demo, minimum 4,
   differences 0, remains ambiguous. The failure is prominent; all source rows
   stay listed, and no single-read or empty result can be added/saved.
10. **Renamed AB1:** open MYX15849_PHYkar_cf_i2469_S2.ab1 supplied with the issue.
    It should display normally, with an import notice reporting a recovered
    23-byte stale-offset shift. Expect 580 calls and 6,998 samples per channel.
    The row title is the filename. The active SMPL tag in this file still says
    7337; the renamed text was in an older unused sample-name area. The source
    file is not changed. Try editing, Undo/Redo, reverse complement, session
    saving/reopening and edited AB1 export.
11. **Layout:** inspect 100% and 150% scaling, long titles, expanded rules and
    both no-conflict and conflict results. The result banner should remain easy
    to distinguish from explanations. Scroll/wrap the preview and use conflict
    navigation; highlighted sites should stay at their actual sequence columns.

Automated checks include 32 merge groups, a new ABIF recovery suite, the existing
core/Qt suites and extended kleiotide_merge_check GUI workflows. The latter
accepts an optional screenshot directory and optional real renamed AB1 path.
Linux Qt checks do not replace native Windows/MSVC acceptance.
