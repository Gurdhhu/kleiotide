# Version 0.17.4 — small layout and wording refinements

Extract into a fresh Kleiotide_0.17.4 directory and build `kleiotide` with the
usual Qt 6.8.1/MSVC kit (minimum Qt 6.5). Native session format remains 3.

1. Confirm a visible gap between the joined Edit/View control and the editing
   dropdown at 100% and 150% scaling. Both mode labels remain visible; View still
   disables the editing-mode and right-click dropdowns.
2. Edit groups Cut selected sequences, Copy selected sequences, Paste sequences,
   Delete selected sequences and Copy sequence titles in one subsection. Fragment
   actions have their own subsection. Ctrl+Delete still deletes selected rows.
3. In the sequence-name context menu, Delete selected sequences is beside the
   sequence Cut/Copy/Paste actions, before the separator and selection commands.
   Selecting and deleting names, then Undo, should behave as before.
4. Open Sequence → Groups → Group identical sequences. Confirm the explanation
   states that full sequences must match after removing gaps; a shorter sequence
   matching only a substring of a longer sequence is not grouped with it.
   For example, A-CGT and ac~gt match, whereas ACG does not match ACGT. This is a
   wording clarification; grouping behavior, type separation and Undo are unchanged.

The next planned milestone remains trace-quality trimming and low-quality calls
to N. This source package preserves the preceding mode and gap-locking updates.
