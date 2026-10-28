# Implementation status — 0.27.2

28 October 2026. All seven tester requests implemented: aligned-read FASTA
clipboard copy, explanatory merge naming, motif icon, corrected trace shortcut
help, unified overview tooltip handling, hidden trace title strip and session
sequence/trace append by Add/drop with explicit metadata-discard notification.

Session format remains 4; imported original evidence is retained by shared trace
links and saved normally. Imports are transactional batches and one Undo. Complete
session Open behavior is unchanged. BLAST/trace linking remain 0.28.0 work.
Native 0.27.1 test 1 is accepted: 38/38 automatic and focused manual checks pass,
including Explorer Open With and installation. The 729 Mbp single-row overview
is reported at about 10 s initially, with near-instant overwrite updates.
Real ARM Automatic/fallback remains pending; earlier explicit Threads=1 works.
See RELEASE_0.27.2_TESTING.md and VALIDATION.md for this version's checks.
