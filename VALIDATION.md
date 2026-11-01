# Validation — 0.29.2

31 October 2026. Linux Release, GCC 13, Qt 6.4.2. Only the validation copy
lowers the Qt minimum to 6.4; shipped source still requires Qt >=6.5.
No native MSVC/Windows installer, clean-machine TLS or ARM result is claimed.

## Automated checks

The registered release suite has 47 tests: 20 core tests, nine Qt unit tests
and 18 GUI workflows. Full release runs pass 47/47. GUI checks also pass at
125% (the new integration workflow), 150% (all 18 workflows) and 200% (nine
focused workflows) with the Qt Windows style and Linux offscreen rendering.
These are software/scaling checks, not a replacement for native Windows display
acceptance on the tester's 2560×1440 screen.

The new integration check drives actual menu actions and loopback HTTP. It checks
modeless search windows and an editable owner during a WAITING request, frozen
query snapshots, status text, screen-bounded search geometry, compact footer
layout, protein statistics, method help with no eligible rows, column resizing,
result-only clipboard copies, trace-view activation after linking and exact
XLSX/CSV/TSV group-name roundtrips. Existing automatic-preview checks also verify
that an unchanged trace assignment never invalidates or recalculates its mapping.

Protein fixtures verify 20-state normalization, B/Z/J ambiguity, U/O/stops and
unknown calls, informative/constant/singleton classes, distinct patterns and row
filtering. Existing nucleotide randomized oracles remain in the suite.
State conventions were checked against the official IQ-TREE FAQ:
https://iqtree.github.io/doc/Frequently-Asked-Questions

Table tests cover multi-sheet XLSX, typed numbers/literal names, CSV quoting,
settings and analysis blocks, links and cancellation. Independent openpyxl checks
confirm the BLAST settings/results sheets, all 13 numeric result columns,
literal formula-like text and blue underlined HTTPS accession links. Grouping
and seven separate distance data blocks also load independently.

Additional C++20 syntax checks use -Wshadow -Werror=shadow for dialog_policy,
table_export, trace_peak_mapping and statistics. Linux builds report no compiler
warnings in the recorded runs; this does not establish an MSVC /W4 result.
Application/test/fixture executables were checked before running the suite.

Earlier scaling runs timed out in the BLAST/trace fixture. The final revision
removes redundant resize-event handling that could conflict with Qt layout
constraints, and updates the fixture to wait for automatic previews instead of
clicking a hidden Preview button. Focused checks pass at 150% and 200%; final
GUI suites run sequentially. Earlier timeout records are retained for transparency;
the precise cause of those stalls was not established.

## Installed tools and supplied trace

Separately installed NCBI BLAST+ 2.12.0 passes actual database creation from FASTA
and mixed .kleio sessions, plus blastn/blastp/blastx/tblastn/tblastx GUI searches.
Fake tools and loopback HTTP additionally cover cancellation, monitoring limits,
errors, filtering, duplicate aliases, saved jobs and metadata. No search query
was submitted to NCBI during this update.

Synthetic trace tests cover forward/reverse terminal inference, no fabricated
call indices or quality, flat/noisy unsupported signal, viewer coordinates,
AB1 peak export, edits, Undo and native session persistence. A separate reference
check uses the supplied already-exported AB1: its 14 terminal Q0 calls are treated
as unassigned additions while all channel samples are retained. All 14 receive
signal-supported anchors; unchanged letters, signal samples, Q0 and matching
exported PLOC positions are verified. This reconstruction is not a comparison
against the unavailable original pre-link AB1. Ordinary import preserves old
PLOC values; relink the original AB1 to recalculate older linked mappings.

New session saves use format 5 for inferred sample anchors; formats 1–4 still
load. Earlier application versions cannot open format-5 saves. Source archives
and the patch are byte-verified before packaging. No supplied AB1, screenshot,
research query sequence or job JSON is included in the update package.

See RELEASE_0.29.2_TESTING.md for native acceptance and remaining manual checks.
