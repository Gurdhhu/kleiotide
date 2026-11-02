# Tables, group import and batch renaming

Sequence → Groups → Import groups from table, Edit → Rename → Using old name / new name table and Append taxonomy / text from table accept UTF-8 TSV, CSV and XLSX. Load table opens a file and offers worksheet selection for XLSX. Choose whether the first row is a header, then choose the name and group/replacement columns. Quoted fields and balanced outer parentheses are decoded consistently. XLSX uses stored values; formulas are not evaluated and need cached values.

Matching uses complete sequence names. Previews identify unmatched entries, duplicates and conflicting mappings; no partial or silently selected ambiguous match is applied. Preview updates after a short typing delay. Rename commits the reviewed name changes; group import retains its Apply action. Cancel leaves the document unchanged. The preview tables have resizable columns, horizontal scrolling and Copy table.

## Appending numeric values

Append true lengths, Append aligned lengths and Append row numbers retain the complete existing name and append the chosen Separator followed by the value. The separator defaults to a space and can be empty. True length excludes both open and locked gaps; aligned length counts every stored cell. Row number is the row's current one-based position in the whole alignment, even when only selected names are renamed. There is no optional replacement-name prefix in these operations.

Append filename has the same separator convention and lets you edit the filename text. Other rename operations include literal prepend/append/replace, whitespace conversion, unique-name suffixes, group names and sanitation. [SPART sanitation](SPART.md) has its own lowercase conversion rules.

## Saving analysis tables

Distance, BLAST, identity-grouping and nucleotide-composition reports use Save table with XLSX/TSV/CSV choices. The selected extension is added when absent. Settings precede results; XLSX gives each block a worksheet. CSV/TSV retain the same blocks in a single plain table. Analysis exports protect formula-like text; XLSX stores names as literal strings and metrics as numeric cells. GenBank accession hyperlinks remain blue and underlined in XLSX. Copy table copies only the currently displayed results where that command is offered.

## Exporting groups

File → Export sequence groups offers SPART, SPART XML, XLSX, CSV and TSV. Ordinary table formats contain Sequence name and Group name, preserve original names exactly and can be reimported. SPART export sanitizes names only in its output and rejects sanitation collisions. Group files describe assignments, not sequence letters or trace evidence.
