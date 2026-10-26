# Kleiotide 0.26.3 — wrapped TSV cells

The group-table importer now decodes surrounding wrappers before looking up
sequence names and group names. The shared parser is used for both loaded and
pasted tables, including the background preview.

- `(sequence_name)` matches `sequence_name`.
- `"sequence_name"` matches `sequence_name` using standard TSV quoting.
- `"(sequence_name)"` also matches `sequence_name`.
- `(Group A)` and `"Group A"` both identify `Group A`.
- A quoted field can contain a tab or an escaped double quote: doubled quotes
  decode to a single literal quote. Quoted fields occupy one physical line.
- Only a balanced enclosing parenthesis pair spanning the whole cell is removed.
  Names such as `sequence_(copy_2)` and `(a)_(b)` keep their internal parentheses.
- An empty group, `""`, or `()` means Ungrouped. An empty sequence title is an error.
- Malformed quote syntax and invalid decoded group names block Apply. Extra
  columns still produce a format error. Two-column import semantics and
  duplicate-title, revision, size, cancellation and Undo behavior are retained.

## Acceptance

Load or paste a two-column TSV containing parenthesized or double-quoted names,
then check that the preview shows the decoded names with matching assignments.
Try both wrappers together, literal internal parentheses and embedded escaped
quotes. Apply should assign rows once; one Undo must restore previous membership.
A saved clustering preview still has too many columns to import directly.

## Validation

- All 14 core group test groups pass, including a new wrapped-cell regression
  covering parentheses, standard TSV quotes, tabs, escaped quotes, blank groups,
  malformed syntax, normalized group membership and Undo.
- The Qt group workflows pass with parenthesized and quoted corrected import
  data, alongside the 5,555-row malformed-input responsiveness and active-worker
  cancellation/replacement checks. New parsing is shared by file loading and paste.
- The updated importer passes AddressSanitizer/UndefinedBehaviorSanitizer checks
  with leak detection disabled in this runner.
- Qt validation uses the available Qt 6.4.2 in a validation-only copy; distributed
  sources retain Qt >=6.5. Native Windows/MSVC validation remains pending.
