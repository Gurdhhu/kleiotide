# Kleiotide 0.26.5 — group counts and empty-group cleanup

The group manager now shows live totals beside every group, including Ungrouped:
`Group name (12)`. These are staged membership totals across the full alignment;
filtering the sequence names does not change them. Display counts do not become
part of the actual group names or saved metadata.

**Delete empty groups** removes all named groups with zero staged members,
including groups made empty by edits in the current dialog. It leaves Ungrouped
and nonempty groups intact, regardless of filters. The button is disabled when
there are no empty named groups. Focus stays on the current group if it survives;
otherwise the manager selects Ungrouped.

Edits remain staged until Apply. Cancel discards the changes. One Undo after
Apply restores the previous catalog, order, colors and memberships; Redo repeats
all changes in that operation.

## Acceptance

- Check counts on opening, then move/check/uncheck names individually, through
  the filtered bulk buttons, through highlighted subsets, and using alignment-
  selected names. Both the source and destination totals should update immediately.
- Filter out all names. Counts should retain their full totals, and nonempty
  groups must survive Delete empty groups.
- Empty a group, then delete empties. Cancel should restore the original dialog
  state in the document. Repeat with Apply, Undo and Redo.
- Delete a nonempty group or clear all groups: Ungrouped should receive the
  corresponding total. Newly created groups should display 0 until populated.
- Rename/reorder/color changes must preserve counts and actual group names.

## Validation

The application builds, and expanded GUI workflows pass at 100% and 150%
scaling. Checks cover initial/full/empty counts, filters, individual and bulk
moves, subset checks, alignment-selected additions, newly empty groups, cleanup,
focus fallback, untouched hidden members, Cancel, Apply, Undo/Redo and group names
without count suffixes. Existing import, clustering, selection and clear-all
workflows also pass. Counts use incremental staged membership updates, with
one label refresh after bulk actions. No core data-format change is needed.

Qt validation uses the available Qt 6.4.2 in a validation-only copy; distributed
sources retain Qt >=6.5. Native Qt 6.8.1 / MSVC 2022 validation remains pending.
