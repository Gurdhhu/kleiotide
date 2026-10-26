# Version 0.13 — batch renaming with preview

Extract into a fresh Kleiotide_0.13 folder and build with the existing Qt/MSVC kit.
No new dependency or binary session version is required.

1. Sequence → Select no longer contains "First non-gap symbol in selected columns".
   The identity-view button now shows four tiny colored A/C/G/T letters above dots.
   Check it at the usual Windows display scale; menu/button states still agree.
2. Select a few names and leave a residue block selected. Choose one of the new
   direct Edit → Rename entries. Scope defaults to the selected names; it can be
   changed to all sequences. With no names selected, scope starts at all rows.
   Click Preview: inspect Current name/New name and the summary. Nothing changes
   before Apply. Change a field or scope: Apply is disabled until previewed again.
3. Test spaces/underscores, prepend/append literal text, case-sensitive Replace
   (no regex), editable filename text, and Reverse names with Cyrillic/emoji/
   combining marks. The filename stem is prefilled when a current filename exists.
   Text and separators are literal; include desired spaces yourself.
4. Test Make names unique with names such as alpha, alpha, alpha_2. First alpha
   stays unchanged; the next becomes alpha_3. Selected-only operations also avoid
   titles outside their scope. Matching is exact and case-sensitive. Strip numeric
   suffixes removes the chosen separator plus trailing digits; preview any genuine
   identifiers before applying. Duplicate results are amber, reported and allowed.
5. Row-number names reflect current alignment positions (1-based), even with a
   sparse selection. Reorder first and verify the result. Length names include
   gaps; true-length names exclude them. Both accept an optional prefix and work
   on masks. Zero-length sequences get a valid "0" title.
6. In Using old name / new name table, paste complete titles separated from new
   titles by one TAB. Matching uses original names simultaneously: A→B, B→C
   renames A to B and B to C, without chaining A to C. Repeated source names all
   receive the mapping. Unmatched keys/multiple row matches are reported within
   the chosen scope. Repeated identical mappings are accepted; conflicts, missing
   fields, extra TABs or blank results prevent Apply. Append taxonomy / text from
   table uses the same key rule and appends the supplied value with a separator.
7. Cancel a valid preview, try an invalid replacement producing a blank name, and
   preview a no-op. Data and undo history remain unchanged. Apply a batch: one
   Undo restores every changed name, and Redo restores the batch. Sequence symbols,
   kinds, row order/IDs, original signals and trace provenance must stay unchanged.
   A changed alignment invalidates a previously prepared plan.
8. Save/reopen .kleio and export FASTA after renaming. Titles are retained, traces
   still work, and name/residue selections and view settings persist as before.
   Recheck double-click individual rename, table rename, clipboard priority,
   deletion, trace editing, view toggles and smooth scrolling.

## Automated validation

Eight new Qt Core rename test groups cover all rules, Unicode graphemes, occupied
suffixes/outside-scope names, exact table matching/reports, invalid/conflicting
input, no-op/stale plans, atomic undo and native trace/name persistence. Together
with the retained core/selection/trace/session/coordinate tests, 101 groups run
through six CTest suites when the GUI is enabled.

The optional `kleiotide_stage013_check` checks actual menu dialogs, scope, preview
invalidation/cancel/Apply, table reports, Unicode/numeric/length names, one-step
undo/redo, selector removal, icon colors/dots and native selection/evidence round
trips. Build tools with `-DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON`; run CTest normally.
Linux/offscreen checks complement this Windows interaction checklist.
