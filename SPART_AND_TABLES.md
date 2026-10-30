# Sequence groups and table exchange

Sequence → Groups → Import groups from SPART… reads matricial .spart and SPART
XML (.spart.xml), following the supplied SPART format specification dated
2 August 2021. If several partitions exist, choose one from the dropdown and
review its assignments before Apply. Missing assignments (?) become Ungrouped
for matching sequence names; unmatched titles are skipped. Duplicate titles
require explicitly enabling assignment to every matching row. Import changes
group membership, not sequences or names. Scores and analysis annotations are
not stored as Kleiotide group data.

File → Export sequence groups… offers .spart, .spart.xml or a TSV table of
sequence name and group name. SPART exports one partition representing the
current memberships; choose its name when saving. SPART requires unique sample
names containing ASCII letters, digits or underscores. Invalid names are reported
without changing the document; use Rename → Sanitize names / Make names unique,
or export TSV. Ungrouped rows use ? in matricial output and remain unassigned in
XML.

Standard SPART subset identifiers are numbers. Optional Kleiotide_group_names
(matricial, Base64 UTF-8) and kleiotideName attributes (XML) preserve named groups
on a Kleiotide round trip. Other readers may display their numeric subset labels.

## Shared table loading

Load table… is available in:

- Sequence → Groups → Import groups from table…
- Edit → Rename → Using old name / new name table…
- Edit → Rename → Append taxonomy / text from table…

All accept UTF-8 TSV/TXT, comma-separated CSV (including quoted commas/doubled
quotes; semicolon CSV is also accepted), and XLSX. Select a worksheet if there
is more than one, indicate whether the first row is a header, and choose source
and value columns for tables with more than two columns. Two-column tables use
the first and second columns. Column selections invalidate the old preview.

Matching uses complete, case-sensitive sequence names. Quotes are decoded and
one balanced outer pair of parentheses is removed, as in the existing group
importer. Conflicting mappings are rejected. Preview before Apply.

XLSX imports use stored cell values, including shared and inline strings. They
never execute formulas, macros or external links. Formula cells need saved cached
values; recalculate and save the workbook first if these are missing. Text cells
preserve identifiers exactly; numeric formatting is not applied. Cells containing
line breaks or null characters are rejected. Limits: 16 MiB decoded table,
100,000 data rows, 4,096 columns, 64 MiB XLSX file/individual decompressed entry.
The bundled miniz inflate component requires no additional runtime installation.

examples/table_import_demo.xlsx provides two worksheets for manual testing.
