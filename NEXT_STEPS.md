# Next steps

0.26.12 completes implementation checkpoint A and brings forward the requested
shortcut swap and Edit → Mode controls. Read IMPLEMENTATION_STATUS.md for the
full boundary and VALIDATION.md for results. The application is not RC1.

Build in a fresh Qt 6.5.3 / MSVC2022 x64 Release directory and run release-check
(33 tests). Check the changed controls and exports using WINDOWS_TESTING.md.
Use WINDOWS_INSTALLER.md to produce 0.26.12 test 1; keep the exact corresponding
source and Qt source available alongside the setup. Retest real MAFFT 7.526 and
toolbar recovery on native x64 and the colleague's ARM laptop.

Continue checkpoint B with typed global settings, new-session precedence,
recovery and bounded overview. Implement trace linking as checkpoint C with
coordinate-ground-truth tests before enabling its UI. Keep the broader storage
rewrites deferred as specified in docs/IMPLEMENTATION_PLAN_2026-10-26.md.

The license remains GPL-3.0-only; branding/artwork permissions remain in
BRANDING.md. Existing native 0.26.11 acceptance and partial ARM test-2 acceptance
are retained in WINDOWS_ACCEPTANCE.md; changed code needs new native acceptance.
