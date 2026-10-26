# Version 0.12 — toolbar views and sequence selection

Extract into a fresh Kleiotide_0.12 folder and build `kleiotide` using the existing
Qt/MSVC kit. Dependencies and the version-1 native session format are unchanged.

1. Three small buttons appear after the two toolbar dropdowns: identity dots,
   chromatogram peaks and overview bars. Hover to read the descriptions. Click
   each and verify its checked state and the matching View menu item. Toggle
   from the menu as well. Close the chromatogram panel with its close control:
   its button becomes unchecked; click the button to reopen it. Without linked
   traces the chromatogram toggle is disabled. Add/open AB1 to enable it.
2. Save/reopen a .kleio session with a mixture of view choices. Menu, toolbar and
   actual panel visibility return consistently; the identity toggle remains a
   render-only setting. No selection operation below should change sequence data
   or create an undo entry. Test with a residue block selected throughout.
3. Under Sequence → Select, try **Names starting with**, **Names containing** and
   **Names from exact list**. Enter several lines, including Cyrillic names,
   spaces, punctuation, duplicate entries and blank lines. Match case is on by
   default; disable it for case-insensitive matching. These are literal filters,
   so characters such as '.', '*' and '[' are not regular expressions. Ctrl+F
   still opens the existing name-regex selector. Cancel preserves selection;
   an accepted list with no matches clears names but preserves residues.
4. **Duplicate names** compares complete titles or titles before the first '(' or
   '|'. Prefix comparison trims outer whitespace. Test duplicate full names,
   annotated variants and mixed-case Unicode names. Select all members or only
   the additional copies; "first" means the current row order, including after
   reordering. No names are renamed or deleted by this action.
5. **Duplicate sequences** ignores case. By default it also ignores open/locked
   gaps; turn Ignore gaps off to require identical aligned symbols and gap types.
   Masks group separately from nucleotide rows. Test all members versus only
   additional copies. Empty and all-gap rows compare normally under the chosen
   gap rule; no actual gaps/symbols are changed.
6. Select columns, then **First non-gap symbol in selected columns**. This selects
   sequences that start in that range, not every sequence having any residue in
   it. Test staggered starts, ragged rows, masks, leading N and empty/all-gap rows.
   Previously selected names do not restrict the search.
7. **Sequences containing a motif** searches nucleotide rows case-insensitively.
   Ignore gaps is on; reverse complement and compatible IUPAC options start off.
   For example AAG also matches a CTT sequence with reverse complement enabled.
   Exact R matches literal R; compatibility allows A/G and any partial call sharing
   a possible base. N/? then match any nucleotide and T/U are compatible. With
   Ignore gaps off, gap type and position remain exact. Mask rows are excluded.
   Enter whitespace, an invalid symbol and an all-gap query to check validation;
   invalid input does not change the current selection. Motifs support up to 4096
   searchable symbols. This action selects names; it does not navigate all hits.
8. **Random subset** selects the requested count from all rows without replacement.
   Check zero, one, all rows and an intermediate count. Repeating draws can give
   different subsets; row order and sequence contents stay unchanged.
9. Recheck Backspace/Delete, clipboard priority, selected-block sliding, trace
   editing and scrolling. The selection dialogs must preserve the residue block.

## Automated checks

The new nine-group `kleiotide_selection_tests` covers duplicate rules, start ranges,
exact/compatible/reverse motifs, original-column mapping across gaps, overlaps,
64/128-bit word boundaries, the 4096-symbol limit, 2000 independent randomized
comparisons, invalid input and seeded random samples.

The optional `kleiotide_stage012_check` exercises actual menu dialogs, buttons,
panel closure, Unicode title matching, duplicate options, motif/start/random
selection, cancel/zero matches, residue independence, unchanged evidence/data,
and native restoration. Build with `-DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON`.
Run all core suites using `ctest --test-dir BUILD_FOLDER --output-on-failure`.
Linux/offscreen checks complement the Windows interaction checklist above.
