# Next steps

Build 0.28.1 test 1 with the same Qt 6.5.3/MSVC2022 x64 Release kit and a fresh
build directory. Run release-check (41 tests), then test-installer with revision 1
and the matching new Source_For_Installer ZIP. Follow RELEASE_0.28.1_TESTING.md.

Prioritize the supplied trimmed N-rich trace, BLAST Settings persistence, 21-column
results/ranking and real blastp/advanced-client searches on small public queries.
If a search waits, keep its RID/job and diagnostic report; inspect the same RID
at NCBI. Local fixtures cannot establish the cause of the previously reported
no-result searches. Avoid submitting duplicate jobs while investigating.

Clean-machine TLS and ARM MAFFT Automatic/fallback still need native acceptance.
This is a testing release, not RC1. No Git push or Windows installer build was
performed in this environment.
