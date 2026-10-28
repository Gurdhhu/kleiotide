# Validation — Kleiotide 0.27.2

28 October 2026. Local Release build uses GCC 13.3 / Qt 6.4.2 on Linux; the
validation copy alone lowers the Qt minimum to 6.4, while delivered source keeps
Qt >=6.5 for Qt 6.5.3/MSVC2022 x64. An existing local build was incrementally
rebuilt; no clean/native Windows build is claimed. All 40 executable images were
checked for nonempty ELF content before testing; one invalid cached output was
removed and successfully relinked before execution.

Final registered suite: 38/38 passed. GUI at 150%: 13/13 passed. Focused merge,
Settings, modes and release-fixes workflows at Windows-style 200%: 4/4 passed.
Normal native preferences remained unchanged. These are offscreen checks;
native Windows hover/dwell rendering and Explorer drag/drop need manual retesting.

New regressions cover consensus/source clipboard FASTA headers and equal-width
gap padding; session drop with linked original AB1 calls, masks and proteins;
metadata discard/current-code retention; one Undo; import notice; no-height
trace title; motif icon/merge label; and repeated native overview tooltip events
retaining column conservation details. Existing tests cover atomic save/export,
original evidence, translation, groups, MAFFT fixture/cancellation and recovery.

Native 0.27.1 acceptance is recorded in WINDOWS_ACCEPTANCE.md. Real ARM MAFFT
Automatic/fallback remains pending. No new live MAFFT/Windows installer build,
registry execution, sanitizer or BLAST submission is claimed for this version.
The outer bundle retains final logs/XML and executable hashes. Source ZIP entries
are byte-checked; the patch against 0.27.1 is applied and compared to delivery.
