# Validation — 0.29.1

## Build fix 1 — MSVC declaration correction

The supplied Windows build stopped in dialog_policy.cpp. Distinct widget
variables and explicit branch bodies replace the repeated c declarations.
The corrected file passes a GCC C++20 syntax check with -Wshadow -Werror=shadow,
and the rebuilt release suite passes 45/45 tests (17 GUI workflows included).
The supplied log was reviewed in full; its only failing compilation unit was
this file. No new native MSVC/Windows result is claimed. The scaling and actual
BLAST+ checks described below were performed for the original 0.29.1 update;
they were not repeated for this declaration-only build fix.

30 October 2026. Linux Release / GCC 13 / Qt 6.4.2. The validation copy alone
lowers the Qt minimum to 6.4; shipped source requires Qt >=6.5 as before.
No native MSVC/Windows installer or ARM result is claimed.

The release suite registers 45 tests: 20 core, eight Qt unit and 17 GUI workflows.
The new automatic-preview workflow edits fields without clicking Preview, checks
rename/merge/quality/collapse/trace mapping, and confirms explicitly. It checks
that unassigned traces stay Not attempted and that formatting changes retain a
chosen collapse representative. Existing source-protection tests reflect suffix
normalization while still checking that the original source is unchanged.

Exchange tests cover suffix insertion and alternative suffixes, SPART whitespace
sanitation/export, and sanitation-aware import without modifying alignment names.
BLAST metadata tests put a journal reference before the direct-submission
reference and verify creation date and direct-submission authors. Independent
openpyxl checks verify real hyperlinks, blue underlined font and workbook styles.

Final runs pass 45/45 tests, all 17 GUI workflows at 150% and nine focused
workflows at 200% with the Qt Windows style (Linux offscreen rendering).
Final run counts and scaling results are in the accompanying logs. Actual
installed BLAST+ database creation and all five local searches were also rerun.
No supplied research query sequences were submitted or redistributed. The attached
RID was checked read-only, with no duplicate submission; it still returned WAITING.

The user's 0.29.0 manual acceptance is retained as prior-version evidence only.
Its report mentions 41 automatic tests; the new source registers 45, so use a fresh
CMake build directory and check that complete count. Native cursor behavior,
Cancel placement, compact dialog geometry, popup width and file dialogs need
confirmation with the user's Windows kit and actual display scaling.

See RELEASE_0.29.1_TESTING.md for focused manual checks and metadata definitions.
