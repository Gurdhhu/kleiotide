# Validation of Kleiotide 0.26.8

Executed on 22 October 2026. This patch changes ABIF unknown-call import, its notices/tests, version metadata and documentation. Session format remains 4; no new external dependencies were added.

## Passed

- Release application and test build with GCC 13.3/C++20 and configured `-Wall -Wextra -Wpedantic`; no new compiler warnings observed.
- Complete CTest release suite: **30/30 passed** — 17 core suites, four Qt unit suites and nine GUI workflows, including the new AB1 import check.
- All nine GUI workflows repeated with `QT_SCALE_FACTOR=1.5`: **9/9 passed**.
- Changed ABIF reader and recovery regression suite rebuilt with AddressSanitizer/UndefinedBehaviorSanitizer, linked with the previously instrumented unchanged core: **7/7 test groups passed**. Leak detection disabled; no full GUI sanitizer or leak-check claim.
- Core regressions cover uppercase/lowercase X in PBAS1 and PBAS2, edited-set preference when a different unedited set exists, case/call mapping, exact peak/Q/channel retention, native-session and AB1 round trips, combined recovery notices, bounded position lists, rejection of unrelated invalid bytes and malformed traces, and unchanged clean-file behavior.
- The real GUI import workflow opened the supplied failing AB1, acknowledged its notice and compared every imported call, peak, quality and channel sample with core import. The session suite also passed its real-file edit/reverse-complement/save/reopen check: **11/11 groups passed**.

## Supplied AB1 evidence

The edited PBAS1 set contains 566 calls, with one lowercase x at one-based call 523 (Q0, peak 15775). Import normalizes it to n and reports the conversion. The different unedited PBAS2 set has 1364 calls and is not substituted.

An independent Python struct-based ABIF parser compared the imported evidence and an unchanged AB1 re-export: all 566 normalized calls, all peak positions and qualities, and every sample in four 16,349-sample analyzed channels match. No calls are deleted or shifted. The source file's SHA-256 remains `ad04607e172bf73a91a7fadb8b4e066647e528ebac42c25b517e5d922eff0531`.

Research AB1 data, exported copies and temporary binary probes are not bundled. Checked-in fixtures are synthetic. The GUI acceptance executable can exercise another local file with `kleiotide_abif_import_check OUTPUT_DIRECTORY FILE.ab1`.

## Documentation

The changelog contains 58 distinct numerically ordered versions, 0.1 through 0.26.8, checked against archived release headings and validation checkpoints. Entries summarize recorded behavior at the time, including later removals. No unverified release dates or extra versions were invented. ICON_GUIDELINES.md records recommended artwork masters and Windows ICO sizes with Microsoft/Qt references.

## Environment and limits

The GUI runner uses Qt 6.4.2 on Linux. A validation-only copy lowers the Qt package minimum and selects a working local CTest 3.28.3 executable; delivered application/test source matches it exactly. Delivered CMake retains **Qt >=6.5**. The primary Windows acceptance kit remains **Qt 6.8.1 / MSVC 2022 x64**.

Native Windows/MSVC execution, Windows deployment and new third-party application acceptance were not available here. Complete RELEASE_CHECKLIST.md before RC1. Prior stabilization results are retained in `docs/history/VALIDATION_0.26.7.md`; earlier development checkpoints remain in `docs/history/VALIDATION_through_0.26.6.md`.
