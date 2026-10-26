# Kleiotide 0.26.4 — import column mapping

Import groups from table now accepts TSV data with more than two columns.

- Choose **Sequence name column** and **Group name column** independently.
- Columns are labeled by number. Checking **First row is a header** also shows
  the decoded header names, with column numbers distinguishing duplicate headers.
- Two-column tables automatically use column 1 for sequence names and column 2
  for groups. The selectors are fixed in this case, including after replacing
  a wider table whose previous mapping used other columns.
- Wider tables initially use columns 1 and 2; change them as needed and inspect
  the preview before Apply. Other column values do not affect assignments.
- Mapping changes invalidate Apply immediately and refresh the background
  preview. Old/cancelled worker results cannot replace the current mapping.
- Both selected columns must exist on each data row and must differ. Missing
  columns are errors, while a present but empty group cell means Ungrouped.
- Quote and parenthesis decoding from 0.26.3 still applies, including escaped
  quotes and embedded tabs in quoted fields. TSV syntax is checked in all fields.
- File loading, paste, duplicate-title policy, Cancel, stale-revision checks and
  single-step Undo/Redo retain their previous behavior.

## Importing a clustering preview

1. Load or paste the saved preview TSV.
2. Check First row is a header.
3. Choose **Column 1 — Sequence** and **Column 2 — Proposed group**.
4. Check the matching sequence count and preview, then Apply.

A saved clustering preview is now directly importable. All additional statistic,
centroid and ID columns are ignored for these assignments.

## Validation

- All 15 group test groups pass, including arbitrary/reordered columns, header
  metadata, quoted tabs in unused fields, default columns, missing/equal mappings,
  malformed TSV, bounds, normalized cells, conflicts, cancellation and Undo.
- A 5,555-row grouping-preview table imports through the default mapping in a
  read-only core plan, with all sequence assignments and header labels retained.
- Qt workflows pass at 100% and 150% scaling: 8-column detection, header labels,
  changing sequence/group mappings, an unmatched-column choice, direct export
  import, two-column reset, Apply/Undo, active-worker cancellation and editing.
- The large-table GUI checks took 1.24–1.30 seconds across multiple header and
  mapping changes, including debounce delays. Maximum UI heartbeat gaps were
  22 ms at 100% and 48 ms at 150% in these runs.
- The updated importer passes AddressSanitizer/UndefinedBehaviorSanitizer checks
  with leak detection disabled in this runner.
- Bounds are 16 MiB, 100,000 nonempty data rows and 4,096 columns. The column
  bound prevents an oversized malformed row from creating millions of controls.
- Qt validation uses the available Qt 6.4.2 with the validation copy's minimum
  lowered; distributed sources retain Qt >=6.5. Windows/MSVC validation is pending.
