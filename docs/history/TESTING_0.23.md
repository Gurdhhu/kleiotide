# Kleiotide 0.23 — Windows acceptance

Extract into a fresh Kleiotide_0.23 folder and build with the existing Qt >=6.5
Desktop/MSVC kit. Native sessions remain format 4. Acceptance uses copies of
research data; representative traces/groups are chosen explicitly in preview.

1. Open examples/collapse_demo.fasta with the default nucleotide import type.
   Sequence → Collapse identical sequences… should find first/second/third as a
   set of three despite case and gaps. The shorter substring, different ambiguity
   and different literal sequence stay separate. No changes occur while previewing.
2. The default result title is first;second;third. Select the third member and
   Use selected member as representative. Its title should move first and its
   original case/gaps remain the displayed retained data. Change the separator.
3. Choose Representative title + count. Three members should give third_x3;
   the separator becomes inactive. A set of 13 should give name_x13, not x12.
   Apply, inspect retained rows/names and Undo once. All original rows/names/gaps
   return. Redo once should repeat the exact collapse.
4. Select only two of a three-member set by sequence names. Choose Selected
   sequence names and Preview. Count should be x2; the unselected copy remains.
5. Add duplicates with different group memberships, different linked AB1 traces
   or translated protein provenance. The member table should identify each group
   and trace source; choose the desired representative. After Apply its original
   symbols, group, trace and provenance must remain. Undo restores all alternatives.
6. Protein and nucleotide rows with the same letters must never be collapsed
   together. Translated snapshots are ordinary protein rows; independent gaps
   are ignored. N and other ambiguities match literally. U and T are distinct.
7. Masks remain untouched by default. Include mask rows opts in to a separate
   mask comparison domain. Empty/gap-only rows match other empty/gap-only rows of
   their type; shorter matching substrings do not match longer complete strings.
8. Cancel with or without changing settings/representatives: data stays unchanged.
   A no-duplicate scope should disable Apply. Cancel a large preview and verify
   the dialog stays responsive. After Apply, save/reopen .kleio and export FASTA:
   surviving IDs/groups/evidence and complete result titles must round-trip.
9. Recheck grouping (which retains all rows), rename commands, ordinary row deletion,
   Undo/Redo, motif/translation/trace views and scrolling after a collapse/undo.

COLLAPSE.md describes exact identity, naming and evidence rules. VALIDATION.md
records local automated results and platform limits. Free-residue compression
has been removed from the remaining plan.
