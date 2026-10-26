# Version 0.13.1 — ASCII-safe unique sequence names

Extract into a fresh Kleiotide_0.13.1 folder and build with the existing Qt/MSVC kit.
No new dependency or native format version is required.

1. Sequence → Sort no longer includes Alphabetically. Recheck title/length/true
   length/frequency/group/reverse sorting, including undo.
2. Edit → Rename → Sanitize names opens the familiar preview dialog. It defaults
   to all rows even with only one name selected. Check the scope, then Preview.
   Cancel must leave names, sequences and history unchanged.
3. Try A B, A_B, A/B, A_B_2, a_b, non-ASCII-only names and punctuation-only names.
   All outputs use ASCII letters, digits and underscores. Safe A_B and A_B_2 are
   reserved and preserved; colliding normalized names receive unused numbers.
   Case-only collisions also receive suffixes. Unsupported runs become '_'; newly
   normalized outer underscores are trimmed. Empty normalized names use sequence
   with numeric suffixes as needed. Safe numeric/underscore-only names stay valid.
4. Switch to selected scope: only those rows change and resulting names avoid
   untouched names, including case-only collisions. Unselected unsafe names remain
   as they were. The default all-row scope is the way to sanitize the whole file.
5. Apply, then Undo once: all original names, including Unicode, return. Redo
   restores the sanitized names. A second Sanitize preview should show no changes.
   Name/residue selections, row order/IDs, symbols, masks and traces stay intact.
   Save/reopen .kleio and export FASTA to check the resulting titles as usual.

The new character/uniqueness rule does not shorten titles or transliterate text.
The next high-contrast palettes and monochrome/colored-background displays are
queued separately and are not part of this maintenance release.

Three added model groups test reserved/case/punctuation/non-ASCII collisions,
selected scope/atomic undo, and 2000 varied names with ASCII/uniqueness/idempotence
checks. Updated actual-dialog tests cover all-row default, Preview/Cancel, Apply,
selection/evidence retention and undo. Sorting and existing GUI workflows remain
part of validation. Linux/offscreen checks complement this Windows checklist.
