# 0.29.2 implementation status

31 October 2026. This update follows the tester's successful 0.29.1_test2
manual checks. RELEASE_0.29.2_TESTING.md contains the native acceptance checklist.

| Request | Implemented behavior |
| --- | --- |
| 1, 5 | NCBI context command in its own section; main BLAST searches separated from database creation. |
| 2 | Collapsed distance comparison rules; method help placed below the selector and refreshed before validating scope. |
| 3 | Shared XLSX/TSV/CSV table chooser; settings and separate analysis sheets; preserved remote hyperlinks. Composition saving also uses the same formats. |
| 4 | Molecule-selectable nucleotide/protein alignment statistics on an immutable alignment snapshot. |
| 6 | Visible text/color search status for ready, running, metadata, completion, cancellation and failure. |
| 7 | Main-menu BLAST windows are modeless and query a snapshot, allowing edits in the originating session. |
| 8, 9, 11 | Footer layout corrected; compact prompts and search windows; scrolling forms; paired merge controls and shorter previews. |
| 10 | Resizable preview columns, horizontal scrolling and result-only TSV Copy table buttons. |
| 12 | A no-op trace assignment returns without invalidation or remapping. |
| 13 | Supported terminal additions get separate inferred signal anchors; viewer/export/session use them without inventing original calls or Q. |
| 14 | Confirming trace linking activates the current session's chromatogram. |
| 15 | Lowercase SPART sanitation on rename/export and canonical import comparison; alignment names unchanged by import/export. |
| 16 | Group export adds XLSX/CSV; ordinary group tables preserve original names for reimport. |

Native sessions write format 5 and read formats 1–5. Earlier applications cannot
read new format-5 saves. Old evidence/mappings remain intact on import; relink
an original AB1 when recalculating its mapping. Previously exported AB1 PLOC
values are not silently rewritten on ordinary import.

Linux/Qt checks, public synthetic fixtures, installed local BLAST+ checks and a
reference reconstruction using the supplied already-exported trace are recorded
in VALIDATION.md. Native MSVC/Windows installer, display scaling, clean-machine
TLS and ARM results still require their own acceptance evidence. No NCBI query
was submitted during this update; HTTP workflows use a loopback fixture.
