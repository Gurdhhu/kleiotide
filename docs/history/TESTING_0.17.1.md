# Version 0.17.1 — stage-3 refinements

Extract into a fresh Kleiotide_0.17.1 folder. Build the `kleiotide` target with
Qt >=6.5 / the usual Desktop Qt 6.8.1 MSVC kit. Native format remains 3; formats
1/2/3 open. MAFFT is still installed separately.

1. In an alignment containing nucleotide, protein, mask and trace-linked rows,
   try Sequence → Select → Nucleotide sequences / Protein sequences / Masks /
   Sequences with traces. Only matching names are selected. No matches clears
   name selection; an independent residue selection stays unchanged.
2. Assign a group color. Its wider swatch remains visible for selected names and
   leaves space before the title. Check 100% and 150% display scaling.
3. The identical sorting option is absent. Sequence → Groups → Group identical
   sequences shows the number of duplicate clusters. Apply replaces the previous
   grouping, without sorting rows. Case differences match; gap types/positions and
   row types must match exactly. Only clusters of two or more receive groups;
   singletons become Ungrouped. One Undo restores all previous groups/memberships;
   Redo restores the generated groups. Cancel leaves everything unchanged. Save
   and reopen a .kleio session to retain this grouping.
4. Select several nucleotide names (leave other nucleotide/protein/mask rows
   unselected), then Alignment → MAFFT → Align selected sequences. The settings
   show the selected-only scope. The new result window contains only selected
   rows and retains their names, groups and trace links. Source stays unchanged.
   A selected mask stays excluded from MAFFT and is retained in the result. A
   selected protein is rejected with an explanatory status message. The normal
   requirement of at least two nucleotide rows containing residues still applies.
5. For both Align selected sequences to templates and Align selected fragments to
   templates, choose fast then accurate Adjust strand on a reverse-oriented
   target. Templates must stay on their original strand with original column
   relationships; target orientation changes are reported under Reversed row IDs.
   Linked trace direction must follow the result; original calls, qualities and
   signals remain immutable. Keep each input strand remains the default. Column
   realignment still disables direction changes. Test with MAFFT 7.526 or newer.
6. With no names selected, Cut/Paste a one-row substring in Insert and Overwrite.
   Cut leaves the deletion boundary; Paste inserts/substitutes as before. Undo each.
   Select two or more residue rows. Ctrl+X and Edit → Cut must leave clipboard,
   alignment and selection unchanged. Ctrl+V and Edit → Paste must leave the
   alignment/selection unchanged, for raw text, native residue data and FASTA.
   An old 0.17 multi-row residue payload also cannot paste at a single-row caret.
   Whole-sequence Cut/Paste still works when sequence names are selected, even
   with an independent residue block. Copy remains single-row for fragments.
7. Recheck ordinary block selection/sliding, Delete/Backspace, group import and
   manager, whole-sequence clipboard metadata, motif search and smooth scrolling.

MAFFT references: [strand adjustment](https://mafft.cbrc.jp/alignment/software/adjustdirection.html)
and [template additions](https://mafft.cbrc.jp/alignment/software/addsequences.html).
Fast adjustment uses 6-mer counting; accurate uses dynamic programming. Native
launcher support for additions was checked with MAFFT 7.526, including both flags
and both addition methods. Kleiotide additionally rejects any template reversal.

This package contains source, not an installer. The next planned milestone remains
trace-quality trimming and low-quality calls to N.
