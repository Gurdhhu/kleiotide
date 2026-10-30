# Implementation status — 0.29.0 testing release

Implements the 20 follow-up items from the user's 0.28.1 acceptance report:
trace import guidance and per-row single-file selection; inline executable
browsing; compact parameter controls and grouped BLAST settings; local databases
and all five BLAST programs; owner-window modality/new blank windows; required
email indicator; single merge preview, colors/name divider and post-placement Q;
shorter tooltips and expandable rules; SPART matrix/XML import/export; shared
TSV/CSV/XLSX table loading; XLSX accession hyperlinks; reported warning fixes.

The requested fallback is applied: NCBI protein search was removed after another
small valid blastp request stayed pending beyond twelve minutes. No routing or
performance fix is claimed. NCBI nucleotide remains HTTPS; BLAST+ is local only.

The intermittent invisible scrollbar handles could not be reproduced and are
not declared fixed. Native MSVC/Windows DPI/Excel and clean-machine TLS checks,
and ARM MAFFT Automatic/fallback remain manual gates. See VALIDATION.md and
RELEASE_0.29.0_TESTING.md. Session format 4 and existing license/artwork policies
are unchanged. No Windows installer EXE or Git push is produced here.
