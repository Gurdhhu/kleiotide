# Validation of Kleiotide 0.26.7

Executed on 21 October 2026. This records the stabilized source, not the earlier review of 0.26.6.

## Passed

- Release build with GCC 13.3 and configured `-Wall -Wextra -Wpedantic`; no compiler warnings observed in the fresh build or final rebuilds.
- The complete `release-check` target: **29/29 CTest tests passed** — 17 core suites, four Qt unit suites and eight GUI workflow suites.
- All eight GUI release workflows repeated with `QT_SCALE_FACTOR=1.5`: **8/8 passed**.
- All 17 core suites rebuilt with AddressSanitizer and UndefinedBehaviorSanitizer: **17/17 passed**. Leak detection was disabled. These were not ThreadSanitizer checks.
- Core-only configuration with both GUI and tests disabled omits the development probes and benchmarks by default.
- CMake install layout contains the application and current release documents. This validates install rules on Linux, not Windows runtime deployment.

The new `file_output` unit suite verifies session eligibility for each translation metadata kind; normalized paths; session/trace protection by contents and imported-source identity; Unicode paths; successful atomic replacement; serialization/stream failures; positive short writes; cancellation; cleanup of temporary files; and an injected commit failure.

The new `gui.save_safety_check` exercises the real controls/dialogs for reading-frame, genetic-code, custom-code and annotation Save/reopen; final-path replacement Yes/No; Save-before-close/open and cancelled Save; FASTA export retaining dirty metadata; protein-only export and session protection; normalized AB1 destination confirmation; and blocked translation controls/handlers during save, statistics, grouping and a cancellable MAFFT process fixture.

The other GUI suites cover identity grouping, representative selection, group management/table imports, translation and translation controls, merge, single/batch AB1 export, duplicate collapse and distance analyses. The 5,555-row mapped import workflow remained responsive. Old-session format 1–4 fixtures and translated span migration remain covered by core tests after removing obsolete editing branches.

## Environment and limits

The available GUI runner has Qt 6.4.2 on Linux. A separate validation copy lowers the minimum Qt check to that version and selects a working local CTest 3.28.3 runner; application source files are compared with the delivered source. Delivered CMake retains **Qt >=6.5**. The primary Windows kit remains **Qt 6.8.1 / MSVC 2022 x64**.

No native Windows/MSVC execution, Windows PowerShell execution, clean-machine runtime deployment, new real Windows MAFFT run, signing/installer test, or new BioEdit/Chromas interoperability acceptance was available here. The Windows scripts are supplied for those checks; their execution is not included in the passing results above. Complete RELEASE_CHECKLIST.md before RC1. The application license decision also remains pending.

Earlier development validation is retained in `docs/history/VALIDATION_through_0.26.6.md`.
