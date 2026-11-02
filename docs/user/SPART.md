# SPART sequence groups

SPART export automatically converts each character outside ASCII letters,
digits and underscores to an underscore, including leading/trailing whitespace.
Names are then lowercased. This changes only the exported names. A separate Rename → Sanitize names for SPART
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


Sequence → Groups → Import groups from SPART loads matricial or XML partitions; choose the partition in the dropdown. File → Export sequence groups saves .spart or .spart.xml as well as ordinary [tables](TABLES_AND_RENAMING.md). The current alignment names are not changed by export or import comparison.
