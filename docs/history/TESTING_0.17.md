# Version 0.17 — groups, cut/paste and horizontal scrolling

Extract into a fresh Kleiotide_0.17 folder and build with the usual Desktop Qt/MSVC
kit. Existing .kleio formats 1/2 open; newly saved format 3 requires 0.17+.

1. Sequence → Groups → Manage groups: create, rename, recolor and delete groups.
   Check individual sequence members or add selected names. Each row belongs to
   one group or Ungrouped. Arrow buttons change group sorting order. Apply commits
   all staged changes in one undo step; Cancel discards them. Masks and proteins
   may be grouped. Row numbers identify duplicate titles in the member list.
2. A group swatch is visible beside each grouped name, including selected names.
   Name hover shows the group. Right-click a name for Add to group or Remove from
   group. Right-clicking an unselected name targets that row; clicking a selected
   name preserves the multi-sequence selection for the operation.
3. Sequence → Select → Groups selects multiple groups and/or Ungrouped. Residue
   selection stays independent. Sort → By group follows manager order, preserves
   within-group order, and puts Ungrouped last. Undo sorting and group edits.
4. Import groups from table: paste/load a UTF-8 two-column TSV with complete
   title<TAB>group name. Set the header checkbox explicitly. Check the preview:
   unmatched titles are skipped/reported; duplicate alignment titles block Apply
   unless assigning all matches is enabled. Conflicting repeated assignments
   block Apply. Empty group means Ungrouped. Import applies as one undo step.
5. Rename, drag/reorder, delete/undo rows and run nucleotide MAFFT. Membership and
   group colors/order must follow stable row IDs and survive result creation.
   Save/reopen .kleio; old sessions open with Ungrouped rows. Plain FASTA exports
   keep sequence text, with group metadata retained in .kleio only.
6. Select several sequence names in any editing mode. Edit → Cut (Ctrl+X) removes
   those rows and puts their FASTA text plus native metadata on the clipboard.
   Select a destination name and Paste (Ctrl+V): rows appear after the last selected
   name, preserving clipboard order. With no selected names they append. Existing
   destination rows remain. Undo Paste, then Undo Cut, restoring both operations.
7. Repeat between two Kleiotide windows, including a protein, a forced mask and a
   trace-linked row. Types, group membership/colors and embedded traces must
   survive. Existing matching groups are reused; same-name/different-color groups
   receive numeric suffixes. Paste creates fresh destination row IDs. Copy selected
   sequences also supplies native metadata while remaining ordinary FASTA text
   to external programs. File → Add from clipboard appends to the alignment.
8. With no names selected, select a substring in Insert mode and Cut. Paste at
   the resulting bare caret: the text returns at the deletion boundary. Insert
   paste at a selected single symbol still inserts after that symbol. Overwrite
   paste substitutes the selected stretch, or the incoming length at a bare caret.
9. Cut a residue block across several rows. Its plain clipboard text has one line
   per row. Paste at an anchor with enough following rows, or select a block with
   the same number of destination rows. Insert accepts a single-column destination
   or bare caret; Overwrite substitutes the selected width. Ragged source rows
   retain their actual copied spans, and virtual destination ends pad with open
   gaps. Invalid symbols for any destination reject the entire edit. One undo
   restores a block Cut or Paste. Outside Insert/Overwrite, residue Cut is inactive.
10. Trace signals/qualities stay immutable. Whole-sequence paste retains mappings;
    pasted residue text is edited/inserted data, with no new measured quality.
11. Ctrl+wheel horizontal movement is doubled: six columns per wheel notch instead
    of three. Pixel-wheel Ctrl steps also double. Normal vertical wheel and native
    horizontal wheel behavior keep their prior speed. Check at 100% and 150% DPI.

The native clipboard payload budget is 256 MiB. An over-limit sequence Cut fails
before row deletion. Group-table file loading is limited to 16 MiB. Plain text
round trips through other programs may discard the native clipboard payload;
ordinary FASTA/text import rules then apply. Ctrl+C retains its existing
single-row residue-fragment behavior.

Model tests: CTest includes the groups suite, old/new format migration fixtures
and semantic-validation checks. Optional GUI validation target:
kleiotide_stage017_check (KLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON).
