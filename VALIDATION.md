# Validation — Kleiotide 0.29.0 testing release

29 October 2026. This is a testing source release, not a Windows installer or RC1.
The user's 0.28.1 automatic/manual acceptance is recorded in the release guide.

## Completed

- Linux Release build: GCC 13.3, Qt 6.4.2 Widgets/Concurrent/Network. Only the
  validation copy lowers the Qt minimum to 6.4; shipped sources still require
  Qt >=6.5 for the usual Qt 6.5.3/MSVC2022 x64 kit. The standalone miniz inflater
  is bundled; no added DLL or separately installed build dependency is needed.
- 47 application/test/fixture images verified as nonempty ELF before the final
  run. CTest: 44/44, no disabled/skipped cases (20 core, eight Qt unit,
  sixteen GUI workflows). Sixteen GUI workflows pass at 150% scale with the
  Qt Windows style; seven focused workflows pass at 200%. These are Linux
  offscreen checks, not native Windows results.
- Actual installed NCBI BLAST+ 2.12.0: database creation through the UI from
  degapped nucleotide FASTA and a mixed .kleio session; actual GUI searches
  with blastn/blastp/blastx/tblastn/tblastx all produce hits. Typed recent lists,
  applicable controls, query aliases, 15-column local metrics and complete
  original subject names (spaces/Unicode) are checked. No supplied/private
  research sequences are used in network tests or redistributed fixtures.
- Remote nucleotide loopback HTTP: synthetic query submission/deduplication,
  WAITING/poll/metadata, missing-email warning, cancel/RID resume without another
  submission, FAILED RID, bounded monitoring, countdown and global/per-job
  preference isolation. No fixture success is claimed as a new live NCBI
  nucleotide or native TLS pass.
- Trace UI: first batch action, dropdowns/reasons/unattempted states and actual
  single-file row chooser; async Preview, shared original evidence, Apply and
  history. Existing original-coordinate/trimmed-call mapping tests remain.
- Merge: aligned/tolerant/exact placement, missing/edited Q fallback, per-column
  conflict provenance, source retention, append/history and cancellation. A
  three-read case proves that Q does not change the reference used to accept a
  later overlap or its edit count. A real mouse drag moves the name divider;
  the preview and Copy button do not overlap. UI captures inspected at base
  and scaled sizes show only the reads-to-contig preview and red/amber colors.
- SPART: matrix/XML grouped/ungrouped name round trips, multiple partitions,
  scores/comments/unknown optional fields, Windows/Unix/Mac line endings,
  integer subset labels, dimensions, invalid names and duplicate assignments.
  Scores are ignored, not claimed as preserved group metadata.
- Tables: CSV quoting/empty cells, shared normalization, UTF-8 TSV, independent
  compressed XLSX/shared strings and worksheet choice. UI tests load CSV/XLSX
  into groups and XLSX into both rename operations with header/column selection;
  multi-partition SPART selection/preview/Apply is exercised through dialogs.
- Independent openpyxl reads the exported remote 21-column workbook and verifies
  numeric metrics, literal formula-like names and a real accession hyperlink.
  XLSX ZIP entry bounds and checksums are checked on import.
- Source correction of the reported MSVC warnings: unsigned CRC negation,
  checked ZIP size narrowing, overview fragment/font shadowing, translation
  changed/member shadowing and BLAST response/data shadowing. The final Linux
  incremental build reports no warnings. Native MSVC confirmation is pending.

## Live protein fallback

One public 76-amino-acid ubiquitin query was submitted once to nr using HTTPS,
PROGRAM=blastp, FILTER=F and three targets. NCBI's submission page explicitly
acknowledged blastp/Proteins. The same RID remained WAITING at >12 minutes and
at the final check after approximately 34 minutes. No duplicate job was sent.
The Open RID browser link now supplies program context; a blastn breadcrumb on
the former link did not prove the query was executed by blastn.

This establishes no client-side cause or service-completion guarantee. Following
the requested fallback, NCBI protein search is removed. NCBI nucleotide remains
HTTPS; all BLAST+ remote options are removed and BLAST+ is local only. Sanitized
RID/timing evidence is included separately without user paths or sequence text.

## Remaining manual gates

- Build/test the exact shipped source with Qt 6.5.3/MSVC2022 x64 and verify the
  warning corrections; create installer test revision 1 with its matching ZIP.
- Native DPI and file-dialog behavior, actual independent windows/Open with
  during a pending search, local cancellation/time limits and moved databases.
- Excel/LibreOffice hyperlink clicking and the installed Windows BLAST+ tools.
- Clean-machine HTTPS/TLS deployment and ARM MAFFT Automatic/fallback.
- Intermittently missing scrollbar handles: not reproduced and not declared fixed.

Validation environment: some generated executables initially had missing execute
bits or incomplete image content. The affected image was relinked, all 47 were
verified, and all final test runs completed successfully; launch failures are
not counted as application passes. Package logs contain the final complete runs.
