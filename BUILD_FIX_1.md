# Kleiotide 0.29.1 — build fix 1

30 October 2026. This replaces the original 0.29.1 testing source package.
The application version remains 0.29.1; use installer revision 2 to distinguish
this corrected build from the original package.

## Reported Windows failure

The supplied Qt 6.5.3 / MSVC2022 x64 log stops while compiling
`src/gui/dialog_policy.cpp`. Repeated `c` declarations in the widget-reading
if/else chain trigger C4456 shadow warnings, then C2374/C2086/C2371
redefinition errors. C2039 for `toPlainText` follows that failed declaration.
No other failing source file is shown in the supplied build log.

Each widget cast now has a distinct descriptive variable and a braced body.
This removes those redeclarations and preserves the same signature values,
preview timing, and editing behavior. No features or session formats change.

## Build

1. Extract the corrected Source_For_Installer ZIP to a new source folder.
2. Configure a fresh build directory using Qt 6.5.3 / MSVC2022 x64 Release and
   `KLEIOTIDE_BUILD_RELEASE_CHECKS=ON`.
3. Set `KLEIOTIDE_SOURCE_ARCHIVE` to this corrected inner source ZIP, not the
   original 0.29.1 ZIP. Keep your usual Qt source archive and VC runtime inputs.
4. Set `KLEIOTIDE_INSTALLER_REVISION=2`, then build `test-installer`.
   Expect 45 registered tests and `Kleiotide_0.29.1_test2_Windows_x64_Setup.exe`.

The accompanying validation uses Linux GCC 13 / Qt 6.4.2; only the validation
copy lowers the Qt minimum. Shipped source still requires Qt >=6.5.
Native MSVC compilation and Windows installer execution cannot be verified in
this environment. The original 0.29.1 release guide remains applicable.
