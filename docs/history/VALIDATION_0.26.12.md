# Validation of Kleiotide 0.26.12 — checkpoint A

Executed 26 October 2026. Source baseline: supplied GitHub-ready 0.26.11.
See IMPLEMENTATION_STATUS.md for implemented and pending plan items. The old
0.26.11 validation record is retained in docs/history/VALIDATION_0.26.11.md.

## Results

- Release application, unit tests and release workflows compiled with GCC
  13.3 / C++20. Final C++ build completed without compiler warnings.
- Complete registered suite: **33/33 passed**, no skipped entries: 18 core,
  four Qt unit and eleven GUI workflow suites. JUnit report retained.
- GUI suites with QT_SCALE_FACTOR=1.5: **11/11 passed**. The mode/toolbar
  capture was visually inspected: controls, alignment and status hints fit.
  Offscreen rendering does not establish native Windows DPI/touch behavior.
- Targeted AddressSanitizer + UndefinedBehaviorSanitizer run of the changed
  alignment implementation and row-index regression: passed, no diagnostics.
  Other linked core objects were not instrumented in this targeted run; this
  is not a new whole-core or GUI sanitizer claim. Leak detection was disabled.

## New regressions

Row-index failure injection tries each allocation failure position before a
successful operation: append 11, multi-span erase 13, reorder 19, load 4,
restore session 110, composite collapse 20, splice Undo 5 and splice Redo 5.
Every refusal retains rows, IDs, content/saved-state identity, next sequence ID
and Undo/Redo availability. Two hundred structural cycles check lookup identity;
const lookup is checked while allocations are forbidden.

Export checks cover protected ABIF/session data under misleading extensions,
streamed table commit, serialization failures and preservation of old output.
Distance export cancellation now explicitly accepts replacement of its test
output before testing cancellation; production replacement remains user-driven.

The MAFFT workflow covers a positive safe default, one targeted retry, both
nonzero and zero-with-empty-output core-count failures, refusal of repeated
retries, no retry for an explicit numeric choice, process errors/cancellation,
separate result windows and source preservation. It uses a controlled fixture,
not the colleague's MAFFT/Cygwin package. Toolbar hide/restore and default-layout
recovery keep the document intact.

The promoted gui.mode_controls suite covers toolbar/menu synchronization,
View restrictions, remembered rewrite/right-click preferences, session round
trip, the swapped sliding shortcuts, caret navigation and existing gap behavior.
Existing translation-preview/session regressions pass with shared actions.

## Row-scaling experiment

Same Linux host and GCC 13.3 -O3, using the same benchmark against baseline and
changed core libraries. Each row contains 200 A residues. Times below are one
run in milliseconds, not a statistical performance guarantee. Results include
fingerprint checks after Undo. No Windows, process-RAM or chromosome-length
performance claim is made.

| Rows | Build | Delete one column | Undo | Insert gap in every row | Delete first half of rows |
| --- | --- | ---: | ---: | ---: | ---: |
| 20,000 | 0.26.11 | 543.51 | 359.07 | 723.93 | 20.67 |
| 20,000 | 0.26.12 | 8.87 | 1.12 | 7.61 | 1.66 |
| 40,000 | 0.26.11 | 6,986.18 | 4,560.18 | 9,270.91 | 95.73 |
| 40,000 | 0.26.12 | 13.25 | 2.00 | 14.93 | 3.31 |
| 80,000 | 0.26.12 | 27.86 | 3.69 | 29.44 | 7.16 |

The baseline 80,000-row case was not run. Reproduce with the optional
row_index_benchmark developer target; its source is tools/row_index_benchmark.cpp.
This supports removing repeated row scans in these operations. It does not
remove full-row copying costs for edits to very long sequences.

## Environment and remaining acceptance

Local Qt is **6.4.2**, Linux offscreen platform, CMake/CTest 3.28.3. Only the
Qt package minimum is lowered in a separate validation copy. Delivered source
retains **Qt >=6.5**, CMake >=3.21 and the production code used in these tests.
A CMake configure warning about unavailable XKB development files did not
prevent offscreen compilation/execution.

The user's 0.26.11 native Windows 30/30/manual acceptance remains the baseline;
it is not transferred to changed 0.26.12 code. This version still needs the
Qt 6.5.3 / MSVC2022 x64 release-check run, native dialogs/shortcuts/toolbar checks,
and real MAFFT 7.526 on x64 and ARM. The colleague's test-2 ARM basic workflows
passed, but MAFFT failed and the toolbar disappeared, motivating these fixes.

The installer retains the existing ARM64 Windows-11 capability guard; the new
application version starts packaging revision 1. No real Windows installer or
MSVC binary was produced here. Reuse the accepted packaging inputs and build
with WINDOWS_INSTALLER.md. Settings/recovery/trace linking are pending.

## Windows release-check follow-up: toolbar test

Oleg reports 32/33 for 0.26.12 on Qt 6.5.3 / MSVC2022 x64, with only
“Persistent toolbar dimensions” failing in gui.mode_controls. Installer assembly
was blocked by this prerequisite. An 18-pt toolbar font reproduces the original
assertion locally: the counter is legitimately hidden at fixed width 1280.

The test now checks exact text independently of visibility, sizes a fitting
window from actual metrics, requires full visibility/right alignment there,
checks deliberate narrow-window hiding and recovery, and repeats with an
increased font. Application code and installer scripts are unchanged. The
corrected workflow passed six local combinations: Fusion 100%/150%; Qt Windows
style 100%/125%/150%/200%, all offscreen on Linux. Native Windows confirmation
is still required. See docs/WINDOWS_TOOLBAR_TEST_FIX.md for rebuilding steps.

After this correction, the actual release-check target was rerun locally:
**33/33 passed**, no skipped entries (13.28 seconds). The targeted six-style/
scaling runs all passed as well. The delivered source still requires Qt >=6.5;
only the local validation copy uses Qt 6.4.2, as described above.
