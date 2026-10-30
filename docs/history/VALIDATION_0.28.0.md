# Validation — Kleiotide 0.28.0 testing release

28 October 2026. This follows native Windows 0.27.2 acceptance. No new native
Windows, real ARM, real NCBI service or installed BLAST+ run is claimed here.

## Final local checks

- Incremental Release build using GCC 13.3 / Qt 6.4.2 on Linux. The validation
  copy alone lowers Qt minimum 6.5 to 6.4; delivered CMake requires Qt >=6.5
  Widgets, Concurrent and Network for the usual Qt 6.5.3/MSVC2022 x64 kit.
- All 44 application/test/fixture executable images verified nonempty ELF before
  execution. Invalid cached outputs were relinked rather than counted as tests.
- CTest: 41/41 passed, no disabled/skipped entries: 20 core suites, seven Qt unit
  suites and fourteen GUI workflows. Every entry captured actual output.
- GUI at 150%: 14/14 passed. Five focused Windows-style workflows at 200% passed
  (merge, Settings/recovery, modes, release fixes and BLAST/trace linking).
  Offscreen captures inspected; Search/Results tabs and fixed footer rows keep
  controls reachable, and narrow trace tables retain horizontal scrolling.
- Normal native Kleiotide preferences unchanged across isolated test runs.
- Targeted AddressSanitizer + UndefinedBehaviorSanitizer: trace-mapping tests
  passed, including seeded long flanks, ambiguous homopolymer indels, repeat/
  orientation rejection, immutable original indices and atomic Undo/session
  behavior. Only trace_linking.cpp and its tests were instrumented; other linked
  core objects were not. Leak detection disabled; no whole-program/GUI claim.
- Four previously supplied AB1 files (580, 580, 1,622 and 566 calls): exact forward,
  reverse complement, terminal trim and constructed substitution/insertion cases
  passed original-call-index ground truth. Ambiguous positions may remain -1.
  This is not broad empirical calibration of biological sample identity. Those
  private research files are not included in the distributable source bundle.
- XLSX: ZIP integrity and independent openpyxl loading passed for an 18-column
  table, duplicate-name expansion, Unicode, numeric metric cells and a literal
  formula-like sequence name. Native Excel/LibreOffice still needs manual checks.

## Added correctness coverage

Trace core checks include exact/reverse original-index truth, terminal deletions/
large additions, substitutions/internal indels, multiple optimal homopolymer paths,
repeat/palindrome refusal, cancellation, name tiers, invalid atomic batches, shared
original evidence, one Undo/Redo, unlinking and session deduplication/orientation.
GUI checks add file loading/suggestions, manual many-to-one assignment, async
Preview, category/orientation display, Apply and one Undo.

BLAST data checks cover requested defaults, documented HTTP/toolkit routing,
synthetic IDs, gap/case/query deduplication, nucleotide/protein scope, HSP union
coverage, reverse coordinates, metadata versus record-update dates, no hits,
local filtering, malformed JSON and TSV/XLSX safety.

The new GUI integration test uses an actual loopback HTTP server: nucleotide and
protein submission, waiting/polling, duplicate expansion and record enrichment;
cancel after an RID and resume without another POST; and a controlled external
BLAST+ fixture receiving the real argument list/temp FASTA. Neither fixture
contacts NCBI. The process fixture establishes launcher plumbing, not a real
NCBI task or installed BLAST+ result. Existing editing/save/group/MAFFT/recovery
regressions remain included. The compact merge preview was retested too.

## Remaining native/live gates

Follow RELEASE_0.28.0_TESTING.md. Verify Windows TLS deployment and a small public
NCBI nucleotide/protein query, compare metrics/metadata, test a real optional
BLAST+ remote task/cancellation and inspect trace mapping on edited research
reads. Mapping quality labels are heuristics, not Phred scores or probabilities.
GenBank references often supply a year only; unavailable dates remain blank.
Qt Network/Schannel are new deployment checks. No certificate bypass is used.
ARM MAFFT Automatic/fallback remains pending and can use a later build.

Final logs/XML, executable hashes, supplied-read aggregate output, sanitizer output
and independent XLSX validation are included in the outer bundle. Source ZIP
entries and the patch against 0.27.2 are byte-verified separately. Historical
0.27.2 validation remains under docs/history/.
