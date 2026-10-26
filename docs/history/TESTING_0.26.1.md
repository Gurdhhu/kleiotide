# Kleiotide 0.26.1 acceptance and validation

Build into a fresh directory with C++20 and your usual Qt 6.8.1 / MSVC 2022
64-bit kit. Do not reuse an older release's generated build directory.
No additional external program is required for identity clustering.
Native Windows/MSVC testing remains pending; the checks below were run on Linux.

## Pairwise speed and sensitivity

1. Open a nucleotide FASTA, then Sequence → Groups → Group by % identity….
2. Choose internal pairwise calculation. The maximum centroid candidate count
   becomes editable; its default is 16. Minimum identity and shorter-sequence
   overlap remain 97% / 80% by default.
3. Preview groups. Check the centroid/member names, identities, overlap and
   summary. Every assigned row must pass both thresholds. Current row order,
   sequences and traces remain unchanged. Stop/Cancel must leave groups unchanged.
4. Choose maximum candidates 0 (All / exhaustive), refresh, and compare.
   This restores the old centroid creation-order search, using the faster DP.
   Fast mode can choose a different qualifying centroid or miss one outside
   the shortlist. Increasing the candidate limit is a sensitivity/time tradeoff,
   and even an untruncated word-ranked search can differ in which centroid wins.
5. Existing-alignment mode should disable the candidate control and retain
   its original column-based calculations.

### Supplied dataset benchmark

The uploaded `#all_alignment_SerNr_re_mafft_5555_edit.fas` was read locally,
unchanged: 5,555 rows, at most 734 alignment columns, 302–648 ungapped calls.
Linux x86-64, GCC 13.3.0, Release `-O3`, single-threaded grouping; file loading
is excluded from clustering timings. Concurrent runner activity affects timing.

| Dataset / mode | Time | Centroid comparisons | Groups |
| --- | ---: | ---: | ---: |
| First 500 source rows, 0.26 exhaustive | 60.82 s | 1,710 | 15 |
| Same 500 rows, 0.26.1 exhaustive | 6.44 s | 1,710 | 15 |
| Same 500 rows, 0.26.1 fast, 16 candidates | 2.52 s | 591 | 15 |
| Full 5,555 rows, 0.26.1 fast, 16 candidates | 19.13 s | 6,939 | 101 |

A separate full fast run took 22.57 s with the same 6,939 comparisons and
101 groups. The final benchmark reported 19.13 CPU seconds and 5,471 queries
whose compatible centroid list exceeded 16. All 5,555 exported rows satisfy
the 97% identity / 80% overlap settings. These are performance measurements,
not a claim of biological correctness or equivalence to exhaustive clustering.
For the 500-row subset both searches produced 15 groups, but 119 rows chose
different centroids in fast mode. The chosen centroid order is documented.
The user's FASTA is not redistributed in this source archive.

To repeat on your machine, build `kleiotide_identity_group_benchmark` and run:

```text
kleiotide_identity_group_benchmark INPUT.fas 16 0 preview.tsv
kleiotide_identity_group_benchmark INPUT.fas 0 500 exhaustive_500.tsv
```

Arguments are FASTA, maximum candidates (0 = exhaustive), source-row limit
(0 = all), and optional preview TSV. It reports wall and CPU time, cluster count,
comparison count and shortlisted-query count. Defaults: 16 candidates, all rows,
97% identity, 80% overlap, current orientation.

## Table wording and export

- Result labels must say “New group centroid” and “Group member”; the centroid
  name column says “Group centroid”. Skipped rows keep their explanatory label.
- Save preview… is disabled before preview, during work, after changing settings,
  and after a rejected/failed preview. A stale document revision is rejected.
- Save UTF-8 TSV, open it in a spreadsheet or text editor, and compare every
  preview row. It includes the six displayed columns plus sequence/centroid IDs.
  Unicode, quotes, duplicate names and tabs are retained through quoted cells.
  Save does not apply the groups or create an Undo entry. Canceling the file
  chooser leaves the preview available. An unwritable path must show an error.

## Group catalog replacement

- With all ordinary rows scoped, existing groups that become empty (including
  previously empty entries) disappear after Apply. Removed names may be reused
  by the prefix numbering; group IDs must remain fresh.
- Undo restores the entire old catalog, colors, names and memberships. Redo
  restores the new catalog in the same single step.
- Selected-only scope preserves a group still used by an unselected or skipped
  row. Mask/no-information rows keep their old membership; only unused old
  groups are deleted. Changing settings or Cancel does not delete anything.

## Group manager bulk membership

1. In Manage groups choose a named group and filter the sequence names.
2. Select all checks membership for visible names only, moving them from other
   groups when needed. Unselect all removes only visible members of this group.
3. Clear the filter: hidden memberships should remain as they were.
4. A filter with no matches makes both actions harmless. For Ungrouped the bulk
   buttons are disabled. Cancel discards all staged changes; Apply and one Undo
   restore the expected memberships.

## Automated validation performed

- All 17 core CTest suites and 3 Qt suites pass (20 total).
- The identity suite has 19 test groups: affine scoring/oracle checks, composite
  missing ends, gaps/unknowns, protein layouts, deterministic length ordering,
  candidate filtering, short/seedless exact strings, thresholds, catalog
  replacement, TSV quoting, stale previews, cancellation and Undo/Redo.
- The optimized aligner matched the previous 0.26 implementation for all five
  returned counters on 4,000 randomized nucleotide/protein pairs, including
  internal unknown calls, and on both sides of the packed-DP length boundary
  (combined lengths 8,190 and 8,191).
- Synthetic separated clusters agree with exhaustive assignments; fast search
  substantially reduces alignment count and every accepted pair meets thresholds.
- Qt workflow checks pass at 100% and 150% scaling for preview/save, both methods,
  candidate controls, updated labels, catalog replacement, representative
  selection, stale plans, Stop/Cancel, filtered bulk checks and history.
- AddressSanitizer/UndefinedBehaviorSanitizer checks cover the updated identity
  engine and its tests. Leak detection is disabled in this runner.
- Qt checks use the locally available Qt 6.4.2 with only the validation copy's
  minimum version lowered; distributed sources retain Qt >=6.5. No native
  Windows/MSVC result is claimed.
