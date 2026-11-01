# SPART and table exchange — 0.29.1

## Current 0.29.2 behavior

SPART sanitation now lowercases names, replaces every disallowed character with an underscore, and preserves boundary whitespace as underscores. Sanitation-aware import compares canonical names without editing the alignment. SPART export sanitizes only exported names; ordinary XLSX/CSV/TSV group tables retain original names. File > Export sequence groups offers all five formats.

See RELEASE_0.29.2_TESTING.md; it supersedes older UI instructions below.

SPART export automatically converts each character outside ASCII letters,
digits and underscores to an underscore, including leading/trailing whitespace.
It changes only the exported names. A separate Rename → Sanitize names for SPART
operation applies that rule to alignment names after reviewing the automatic
preview. It does not silently invent suffixes for collisions; export rejects
colliding sanitized names so they can be resolved explicitly.

SPART import offers Assume SPART name sanitation, checked by default. It sanitizes
an in-memory name snapshot for comparison, retaining original IDs and names in
the alignment. Turning the checkbox off restores literal matching. Ambiguous
matching titles use the same duplicate-title validation as table import.

Both matrix and XML variants support partition selection, grouped/ungrouped
assignments and group-name round trips. Standard numeric subset labels are
retained; optional Kleiotide group-name metadata preserves descriptive labels.
Scores and analysis annotations are not stored as group data.

All three table loaders accept UTF-8 TSV/CSV/XLSX, worksheets, headers and column
selection. Quotes and balanced outer parentheses are decoded consistently.
XLSX uses stored cell values; formulas are not executed and need cached values.
Rename previews update automatically; the Rename button commits the result.
Group import previews retain their existing automatic behavior and Apply action.
