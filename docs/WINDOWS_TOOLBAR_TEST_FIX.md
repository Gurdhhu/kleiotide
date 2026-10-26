# 0.26.12 Windows toolbar-test correction

26 October 2026. Oleg's Qt 6.5.3 / MSVC2022 x64 Release run passed 32/33
entries; gui.mode_controls failed at “Persistent toolbar dimensions”. The
release-check dependency stopped test-installer before installer assembly.

The assertion combined existence, exact text and visibility at a fixed
1280-pixel window width. The application intentionally hides the alignment
counter when the editing controls need the available toolbar width. Font and
style metrics vary by platform, so 1280 does not guarantee room for both.

The same original assertion was reproduced locally by increasing the main
window's toolbar font to 18 pt. This supports the font/available-width diagnosis;
it does not constitute a native Windows reproduction.

## Correction

Only tools/stage0191_check.cpp and documentation/package records change.
Application behavior, session format, version number and installer scripts
remain unchanged. The test is neither skipped nor disabled; there are still
33 registered entries.

The test now chooses a wide window from the actual toolbar size hints and
requires the counter to be fully visible and right-aligned there. It checks
unchanged count text during selection and narrow layouts, verifies editing
controls take priority when space is insufficient, and verifies visibility
returns on widening. It repeats these checks with an enlarged font. Failures
report window width, toolbar size hints, counter visibility and actual text.

Local checks: the corrected workflow passes with Fusion at scale 1 and 1.5,
and Qt's Windows style at scale 1, 1.25, 1.5 and 2. These are Linux offscreen
style/scaling tests, not native Windows or ARM acceptance. Full-suite results
are retained in the accompanying test-results directory.

## Rebuild on Windows

1. Download the corrected checkpoint bundle and extract its updated
   Kleiotide_0.26.12_Source_For_Installer.zip to a fresh source folder. Keep
   that exact ZIP; do not combine an edited source file with the old ZIP.
2. Open CMakeLists.txt with the existing Qt 6.5.3 / MSVC2022 x64 Release kit.
   Use a fresh build folder, for example Kleiotide-build/0.26.12-fix1.
3. Enable BUILD_TESTING and KLEIOTIDE_BUILD_RELEASE_CHECKS. Point
   KLEIOTIDE_SOURCE_ARCHIVE at the updated Source_For_Installer ZIP. Reuse the
   same matching Qt source ZIP, original Microsoft x64 runtime and NSIS.
4. Select test-installer and press Ctrl+B. Its release-check prerequisite must
   report 33/33 before packaging starts. Installer revision 1 remains suitable
   when the failed prerequisite created no installer output. If a packaging
   output folder from another attempt already exists, use a fresh revision.

If this assertion still fails on Windows, retain the new detailed failure line
and release-tests-Release.log/XML. Do not bypass the release-check dependency.
