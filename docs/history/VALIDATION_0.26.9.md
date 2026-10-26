# Validation of Kleiotide 0.26.9

Executed on 22 October 2026. This release records licensing/branding, adds Windows test instructions and report output, and updates About/version text. Editing, grouping, ABIF parsing/export and session format are unchanged from 0.26.8.

## Passed

- Release application and tests rebuilt with GCC 13.3/C++20 and the configured warning flags. No compiler warnings observed in this update's build log.
- The actual **release-check CMake target** builds its dependencies and runs the complete suite successfully: **30/30 passed** (17 core, four Qt unit and nine GUI workflow suites).
- `release-tests-Release.log` and `release-tests-Release.xml` are generated in the build directory. XML parsing confirms all 30 test entries and no failure/error entries.
- CTest with the documented `--no-tests=error` option rejects an empty test directory with a nonzero exit code.
- CMake install layout includes LICENSE, BRANDING.md and WINDOWS_TESTING.md, with byte-exact copies of the source documents.
- LICENSE is byte-for-byte identical to the system copy of the full GNU GPL version 3 text. GPL-3.0-only scope/copyright notices are stated separately; the GPL text itself is unmodified.
- Source/document audit confirms the designer's name and both aliases in About, README, BRANDING.md and LICENSE-STATUS.md; the user's permission table and project permission record are present. Active license-undecided statements were replaced, while historical release records remain historical.

The full suite already covers AB1 unknown-call import, output safety, metadata persistence, grouping/imports, translation, merge/export/collapse and distances. No import/edit algorithm changed in this release, so the independent AB1 evidence comparison and targeted sanitizer results from 0.26.8 were not repeated. They remain in docs/history/VALIDATION_0.26.8.md.

## Environment and limits

The available GUI runner uses Qt 6.4.2 on Linux. A separate validation copy lowers the Qt package minimum and selects a working local CTest 3.28.3 runner; application/test sources match the release. Delivered CMake retains Qt >=6.5.

The Windows guide supports the user's existing Qt 6.5.3 / MSVC 2022 x64 kit and also documents use of an installed 6.8.1 kit. These are build/test instructions, not a claim of native execution here. Native Windows, PowerShell scripts, deployment, real Windows MAFFT, display scaling and third-party AB1-reader acceptance still need the user's checks. The guide distinguishes automatic offscreen tests, manual native acceptance and clean-machine deployment.

The application license is now selected: GPL version 3 only. The official artwork has a separate policy and recorded credit/permission; the artwork will be supplied later. Exact binary dependency notices and corresponding-source arrangements still need review before distribution.
