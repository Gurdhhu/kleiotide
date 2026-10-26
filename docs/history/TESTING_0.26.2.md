# Kleiotide 0.26.2 acceptance checks

Extract into a fresh folder and build with your usual Qt 6.8.1 / MSVC 2022
64-bit kit. C++20 and Qt Widgets/Concurrent are required as before. The session
format and fast identity-clustering algorithm are unchanged from 0.26.1.
Native Windows/MSVC testing remains pending.

## One sequence per group

- Named groups still contribute First, Longest or Random as chosen. First and
  length ties use current alignment row order. Empty named groups are skipped.
- The checkbox now reads **Also select from Ungrouped…**. Its dropdown offers
  **One sequence** or **All sequences**, and is disabled until the box is checked.
- One sequence uses the same First/Longest/Random rule as the named groups.
  All sequences includes every Ungrouped row, regardless of that rule.
- The preview lists groups in the exact group-manager order, independently of
  sequence row order. Ungrouped appears last; when all its rows are included,
  they retain current alignment order within that section.
- Apply uses the displayed preview. It replaces sequence-name selection without
  reordering rows, editing group membership or adding an Undo entry. Cancel
  preserves the previous selection. Turning the checkbox off excludes Ungrouped.

## Large/malformed table import

- Paste a saved identity-grouping preview for 5,555 sequences into Import groups
  from table. Its extra columns are deliberately invalid for the two-field
  import format. The dialog should report the errors and remain responsive.
- The preview is now a virtual model/view table with fixed initial column widths;
  it no longer creates/resizes a widget item for every cell while typing.
- Validation is delayed 200 ms after the latest edit and runs in the background
  against a title/ID/catalog snapshot, without copying the sequence alignment.
  Editing cancels the previous job and invalidates Apply immediately. A late
  result cannot replace the latest input's preview.
- Try toggling Header and duplicate-title policy, correcting the input, loading
  a table, and Cancel/closing while validation is pending or active. No partial
  result should be applied. Closing does not wait for a worker to finish.
- Correct two-column data still applies in one Undo step. Duplicate-title,
  unmatched-title, conflicting-assignment, BOM/CRLF and stale-revision behavior
  is retained. Invalid previews cannot be applied.
- Input is bounded to 16 MiB and 100,000 nonempty data rows, excluding an optional
  header. Oversized input gets an explanatory error. Importing cannot create a
  catalog exceeding 100,000 groups. These guards apply to pasted and loaded text.
- A clustering preview export is not directly importable: the importer expects
  exactly complete sequence title<TAB>group name, with no additional fields.

## Group manager

- Drag across several sequence names to select a range; Ctrl/Shift selection
  also works. The highlighted selection is separate from the membership checks.
- **Check selected** / **Uncheck selected** changes membership for highlighted,
  visible names only. The existing Select all / Unselect all still acts on all
  visible filtered names. Hidden rows retain membership. The new subset controls
  are disabled for Ungrouped and when nothing is selected.
- **Clear and delete all groups** opens a warning with **No** as the default.
  No leaves the staged catalog unchanged. Yes stages deletion of every named
  group and sets all rows to Ungrouped, including names hidden by a filter.
- Changes remain staged until Apply. Cancel after Yes discards the clearing.
  After Apply, one Undo restores the exact catalog/order/colors and memberships;
  Redo clears them again. Clearing an empty catalog is disabled.

## Validation performed

- All 17 core test executables and 3 Qt test executables pass (20 suites).
- The identity suite has 20 test groups, including None/One/All Ungrouped behavior
  under First/Longest/Random. The group suite has 13 test groups, including
  5,555 malformed rows, snapshot independence, cancellation, bounds and stale
  import rejection.
- Qt workflows pass at 100% and 150% scaling, including real mouse-drag range
  selection, subset membership checks, clear confirmation No/Yes, staged Cancel,
  Apply/Undo/Redo, manager-order preview, and Ungrouped one/all selection.
- Pasting the 5,555-row clustering export, toggling its header and replacing it
  with valid input completed in 624–709 ms across runs, including
  debounce delays. The longest measured gap between UI heartbeat callbacks was
  65 ms (earlier runs: 57 ms at 100% and 51 ms at 150% scaling).
- Additional GUI checks cancel an active 100,000-row validation job and replace
  another active job's input, confirming safe close and rejection of late results.
- AddressSanitizer and UndefinedBehaviorSanitizer checks pass for the changed
  group-import and representative-selection engines and their suites. Leak
  detection is disabled in this runner. The changed core importer builds with
  -Wall -Wextra -Wpedantic -Werror; the Qt build has no new warnings.
- GUI checks use the available Qt 6.4.2 with only the validation copy's minimum
  lowered. Distributed sources retain Qt >=6.5. No native Windows result is claimed.
