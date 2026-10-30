# Kleiotide 0.29.0 testing release

Based on the 0.28.1 source whose automatic tests and manual items 1–4/6 passed
on the user's Qt 6.5.3/MSVC2022 x64 installation. This release adds local BLAST
and SPART/table exchange, so it advances to 0.29.0. It is not RC1.

## Build

Extract the matching Source_For_Installer ZIP into a fresh directory. Use the
same Qt 6.5.3/MSVC2022 x64 Release kit and fresh build folder. Qt Widgets,
Concurrent and Network remain required; the XLSX inflater is included in source.
Enable KLEIOTIDE_BUILD_RELEASE_CHECKS and build release-check: expect 44 tests.
Build test-installer with revision 1 and the exact matching inner source ZIP as
KLEIOTIDE_SOURCE_ARCHIVE. Matching Qt source/runtime, NSIS and VC runtime inputs
are unchanged. No Windows setup EXE is built by this package.

## Focused manual acceptance

1. Link trace: the bold first action adds a batch and suggests assignments.
   Row Add AB1 accepts one file. Review inferred matches, long names/search,
   manual assignment and trimmed original-call mapping; Preview/Apply and history.
2. Settings → BLAST: HTTPS and local sections are distinct. All executable Browse
   buttons, including MAFFT, are inline. Apply/Cancel/Restore defaults/restart
   retain their normal semantics. Paths alone never alter HTTPS routing.
3. NCBI nucleotide BLAST: email has a required-field asterisk; missing email
   warns before sending. Controls are compact and grouped. Repeat one small
   public HTTPS search; inspect results, countdown, diagnostics, cancellation and
   saved RID resume. Retired advanced remote controls are absent. Existing
   standard nucleotide HTTPS jobs still load. BLAST+ remote jobs are not resumed.
4. NCBI protein BLAST has been removed after the final small live blastp test
   remained pending. Protein search is available under Local protein BLAST.
5. Install real BLAST+ tools. Create nucleotide/protein databases from FASTA
   and a mixed .kleio session. Inspect degapping, type selection, original names
   (including duplicates), output placement and persistence in the correct lists.
   Restart; Browse a moved .kblast with its .files folder; wrong types are rejected.
6. Search locally with blastn/blastp/blastx/tblastn/tblastx. Check task/scoring,
   strand/genetic-code applicability, CPU threads, filters, results and TSV/XLSX.
   Cancel a longer disposable search and test a short limit; errors are visible.
7. While a search is pending, launch Kleiotide again without a file, then open
   another alignment via Open with. Both new windows should be interactive;
   the search blocks only its originating window. Keep the saved job/RID.
8. Both merging dialogs show only the reads-to-contig preview. Drag the name
   divider; inspect red consensus conflicts and amber Q-resolved conflicts.
   Enable Q in overlap mode: accepted placement/edit counts must stay unchanged,
   while supported final conflicts may resolve. Apply appends at the end.
9. Similarity sorting and collapse rules start collapsed. Collapse preview has
   cluster/member headings. Check compact fields, concise tooltips and buttons
   across narrow windows and 100/125/150/200% scaling.
10. Import the supplied matricial/XML SPART examples, including multiple
    partitions and missing assignments; change the dropdown and inspect the
    preview. Export SPART/XML/TSV and reimport; invalid SPART names give guidance.
11. Load TSV/CSV/XLSX into both rename operations and group import. Test headers,
    worksheet and column choices, quoted fields/parentheses, duplicates,
    conflicting/unmatched mappings, preview invalidation and Apply/history.
12. Export remote BLAST XLSX, click an accession hyperlink in Excel/LibreOffice,
    and inspect numeric metrics/literal formula-like names. Local subjects have
    no GenBank links or fabricated metadata.

The intermittent missing scrollbar handles were not reproduced; no root cause
or native fix is claimed. Recheck after long use and capture a reproducible
sequence if it recurs. Clean-machine TLS and ARM MAFFT Automatic/fallback remain
outstanding acceptance items. MSVC warning corrections require native rebuild.

See VALIDATION.md for actual completed checks and their platform limits.
