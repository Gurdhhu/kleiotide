# Editing and selection

Sequence-name selections and residue selections are independent. Name selections choose rows for operations; a residue selection chooses a single cell, a row fragment, a rectangular block or ruler-selected columns. Changing a name selection does not discard the residue selection.

## Extend a residue selection

Edit contains Select to beginning, Select to end and Clear fragment selection in a section immediately above Rename. The same section appears at the bottom of the context menu on selected residues.

- Select to beginning keeps the selected rows and right boundary and extends the left boundary to alignment column 1.
- Select to end keeps the selected rows and left boundary and extends the right boundary to the final column of the whole alignment.
- Clear fragment selection removes only the residue selection and retains selected sequence names.

These are view operations: letters, groups, traces, dirty state and Undo history are unchanged. Extension scrolls horizontally to show its boundary. The final alignment column can extend beyond an individual ragged row's last stored symbol; ordinary editing/copy validity rules still apply to missing cells. The commands require a current residue selection and work in View as well as editing modes. Existing Escape-key behavior remains unchanged.

## Selecting sequences

Sequence → Select offers groups, representatives, molecule types, traces, all/invert, name/sequence patterns, duplicates, motifs and random subsets. After a completed selection it reveals the uppermost selected row if that row is outside the viewport. Horizontal position and residue selection are retained. No matches, Unselect all or cancelling a chooser do not move the view. Restoring a native session retains its saved scroll position rather than applying this navigation rule.

## Motif navigation

Find motif, Find next motif and Find previous motif remain together in their own Edit section. Motif search uses its existing scope and literal/regular-expression rules; these menu changes do not alter the search algorithm or shortcuts.
