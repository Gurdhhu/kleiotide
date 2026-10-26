# Kleiotide 0.26.6 — member color markers and checkbox labels

In the group manager, Select all / Unselect all are now **Check all** /
**Uncheck all**. Their existing filtered-membership behavior is unchanged.

Each grouped sequence in the right-hand list now has a colored square matching
its actual staged group. Hovering over the row shows the group name. Ungrouped
rows have no marker and show an Ungrouped tooltip. Markers describe membership
independently of which group is currently selected on the left.

Markers update after individual, filtered-bulk, highlighted-subset and alignment-
selected membership changes; deleting/clearing groups removes their markers.
Changing a group color refreshes all of that group's member markers. Colored
squares have an outline and retain their actual color under row selection, while
name text uses the normal theme colors. Icons are cached per group for reuse.
Apply, Cancel, Undo/Redo, live counts and Delete empty groups remain unchanged.

## Acceptance and validation

- Open a manager containing members of differently colored groups and Ungrouped
  rows. Check that every marker matches actual membership and tooltips identify it.
- Select another group, apply a name filter, move/check/uncheck members and change
  a group color. Markers should update immediately without changing hidden rows.
- Highlight rows: marker colors should remain recognizable. Ungrouped rows should
  remain easy to identify from the absence of a marker.
- Cancel should discard staged color/membership changes. Apply and one Undo
  should restore the expected group colors and memberships.

The application builds without new warnings. Expanded GUI workflows pass at
100% and 150% scaling. Checks verify button labels, exact marker colors in normal
and selected states, grouped versus Ungrouped rows, tooltips, individual/bulk/
subset changes, live recoloring, group deletion, Cancel, Apply and Undo. Existing
counts, cleanup, import and selection workflows also pass. No core or file-format
change was introduced. Qt validation uses the available Qt 6.4.2 in a validation-
only copy; distributed sources retain Qt >=6.5. Native Windows/MSVC validation
remains pending.
