# Validation of Kleiotide 0.27.0 — checkpoint B

Executed 26 October 2026 from a fixed source snapshot. This checkpoint follows
the user's accepted native x64 0.26.12 test-1 build. See
IMPLEMENTATION_STATUS.md for scope and SETTINGS_AND_RECOVERY.md for behavior.
Historical validation records remain under docs/history/.

## Final results

- Clean Release build: the application, fixture and all registered checks built.
  All 39 executable images were checked as nonempty ELF binaries before execution.
- Complete registered CTest suite: **37/37 passed**, no disabled or skipped entries:
  19 core, six Qt unit and twelve GUI workflows. Every entry captured actual output.
- GUI workflows at 150% scaling: **12/12 passed**.
- Settings/recovery workflow with Qt Windows style at 200%: **passed**. Dialog
  captures were inspected; tabs/pages scroll and the fixed action row is reachable.
  This is Linux offscreen rendering, not native Windows DPI acceptance.
- Targeted AddressSanitizer + UndefinedBehaviorSanitizer runs passed for the
  changed overview implementation/independent bounds tests and session reader/
  writer with all ten session regressions. Other linked core objects were not
  instrumented in these targeted runs. No GUI sanitizer or leak-check claim;
  leak detection was disabled.
- Deterministic format-4 session output matches baseline bytes across multiple
  64 KiB chunks, with nucleotide/protein/mask rows and genetic code 2. Both files
  have SHA256 d77efa9e2d559ec9075984dce12f9d4218844a308785e1b8a3a3bda732ef43d1.
  The baseline serializer is unchanged between 0.26.11 and accepted 0.26.12.
- A before/after comparison confirms the normal native Kleiotide preference file
  was unchanged by the isolated final test runs.

## New correctness coverage

Preferences: legacy path/-1 thread migration, persistence/restart, validation and
invalid/tampered values, unavailable-font fallback, full custom genetic codes,
clean initial translation settings, Apply/Cancel/reset, current-appearance/code
drafts, existing-window isolation and saved/legacy-session precedence. Per-job
MAFFT choices do not replace defaults; Settings Test performs a tiny alignment
using the controlled fixture rather than just accepting a version string.

Test isolation uses temporary INI user and system scopes. AppSettings explicitly
selects defaultFormat: Qt's named-application constructor otherwise keeps native
storage even after setDefaultFormat(IniFormat). The normal application still
uses its existing native organization/application namespace. All twelve release
GUI workflows use isolated defaults; QC job settings honor that backend too.
The migration regression also passed three consecutive isolated runs.

Overview: sixty independent randomized column-model comparisons with forced
summary compression, IUPAC/coverage semantics, revision invalidation and metadata
rename behavior. A two-row million-column fixture stays within its **256 KiB
workspace budget** and remains fully conserved when A/C alternate between
columns. This measures owned scan buffers, not whole-process peak RAM. The GUI
uses up to 8192 summaries and defaults to a 16 MiB workspace budget. No Windows
throughput or chromosome-length latency guarantee follows from this fixture.

Recovery: atomic generations, active-window locks, quota/cancellation refusals
preserving previous bytes, latest/previous generation round trips, strict
rejection of a corrupt latest generation, explicit discard and unchanged live
state/history. A child process commits a checkpoint, remains unavailable while
alive, is killed by its owning test, and then leaves readable data with a
reclaimable stale lock. Cancellation also interrupts an actually partially
streamed large row. The GUI workflow checks mutation gating, modal-editor
deferral, unclean window destruction, unsaved recovery copies, retained old
checkpoints during Recover and safe retirement after a new valid checkpoint.

## Environment and interpretation

Local execution uses Linux, GCC 13.3/C++20, CMake/CTest 3.28.3 and Qt **6.4.2**.
A separate frozen validation copy lowers only the Qt package minimum; delivered
CMake retains **Qt >=6.5** and CMake >=3.21. Application/test/tool bytes match the
validated snapshot. Local executable permissions were normalized before running
CTest. No product assertion was disabled or weakened for the environment.

The 37 entries are the same commands registered for release-check. The final
JUnit/text reports and executable-image manifest accompany this source package.
The source archive and its full 0.27.0 prefix are verified against the working
tree, including complete production sources and unchanged license/branding.

Native Windows 0.27.0 and real ARM results remain pending. The user's native
0.26.12 33/33/manual acceptance and MAFFT 7.526 success (--thread 8, seven rows,
width 557, 6.80 s) are recorded in WINDOWS_ACCEPTANCE.md; they are not transferred
to changed code. No Windows installer binary was produced locally. Use the same
Qt 6.5.3 / MSVC2022 x64 kit and packaging inputs for the new test-1 build.

Settings do not cap total RAM; broad analysis/translation workspace limits and
incremental cache changes remain follow-ups. Recovery temporarily gates mutations
and can refuse unavailable/quota-limited/oversized storage; it preserves previous
completed generations and never marks work saved. No fatal-path autosave was
added. Session format remains 4; trace linking remains checkpoint C.
