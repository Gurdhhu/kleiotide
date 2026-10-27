# Next steps

0.27.0 implements checkpoint B: File → Settings, session/default precedence,
resource preferences, recovery foundation and bounded overview. The existing
mode/shortcut changes remain from the accepted 0.26.12 checkpoint.

Build the same Qt 6.5.3 / MSVC2022 x64 Release kit in a fresh directory and run
release-check (37 tests). Use SETTINGS_AND_RECOVERY.md for the focused native
checks. WINDOWS_INSTALLER.md explains packaging 0.27.0 test 1 with the exact
matching source ZIP; the one-time Qt/runtime/NSIS inputs can be reused.

Trace assignment, forward/reverse peak mapping, Link/Delete trace menus and
scientific confidence calibration are checkpoint C. Broad incremental scan/cache
work and remaining import/export policy decisions stay measured follow-ups;
chunked storage, virtual Flush and a native ARM build remain deferred.

ARM feedback can be collected on a later version as Oleg requested. The accepted
native x64 0.26.12/MAFFT baseline is recorded in WINDOWS_ACCEPTANCE.md. This is a
development checkpoint, not RC1. License/branding terms remain unchanged.
