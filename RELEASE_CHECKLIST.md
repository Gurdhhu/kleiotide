# Release checklist

0.27.0 is implementation checkpoint B, not RC1. Settings, recovery and bounded
overview are implemented; trace linking remains checkpoint C. See
IMPLEMENTATION_STATUS.md. The following checked
native items record **0.26.11 baseline acceptance**, not a new 0.27.0 result.
Repeat the changed workflows and retain new logs before distributing this build.
WINDOWS_ACCEPTANCE.md also records the colleague's limited test-2 ARM results.

## Repeatable automated checks

From the x64 Native Tools for VS 2022 environment, run PowerShell with your Qt MSVC kit:

```powershell
./tools/windows/validate.ps1 -QtRoot C:/Qt/6.5.3/msvc2019_64
```

This configures Release with the release regression suite, builds, runs CTest, and retains logs plus JUnit and environment/result records. It does not change machine execution policy. You may run the CMake/CTest commands inside the script directly instead.

The `release-check` CMake target builds all registered unit tests and twelve GUI workflow programs, then runs CTest (37 entries). It retains text and JUnit reports as `release-tests-Release.log` and `release-tests-Release.xml` in the Release build directory and fails when no tests are found. GUI checks cover AB1 import notices, save safety, identity/group management/import, translation, merge, AB1 export, collapse, distances, MAFFT and synchronized mode controls. Historical workflow executables remain available separately.

## Native Windows acceptance — user-reported baseline passes

The broad manual rerun passes. Keep the detailed checklist for future releases.
Explorer file associations remain unconfigured/untested; compiler-warning review,
full-disk testing and exact external-reader versions were not separately reported.
Do not infer those narrower checks from a general Pass.

- [ ] Selected Qt >=6.5 / MSVC 2022 x64 Release build has no unresolved compiler warnings; retain validation logs and record the exact Qt version (for example 6.5.3 or 6.8.1).
- [x] Windows Save As validates wildcard/reserved names without filtering the listing, confirms the final filename including typed nonstandard suffixes, and preserves the old destination on No/Cancel. Save/export uses the validated Qt dialog on Windows.
- [x] Save frame, genetic code, custom code and coding annotations; reopen and compare. Test Save-before-close/open and cancellation.
- [x] During save, analyses, grouping and MAFFT, translation controls and menu actions cannot alter the source document.
- [ ] Unicode/space-containing and long paths; read-only output folders and failed/short/full-disk writes report errors without damaging existing output. Automated tests inject failures; native filesystem behavior needs acceptance too.
- [x] FASTA and protein-only exports preserve unsaved session state; attempts to overwrite native sessions or imported chromatograms are blocked.
- [ ] Multiple windows, Explorer file-opening/forwarding, clipboard editing, drag selection, undo/redo, ruler and trace rendering at 100% and 150% scaling.
- [x] MAFFT success, invalid output, launcher failure, cancellation and descendant process cleanup using the real Windows installation.
- [x] Edited AB1 output opens correctly in the intended BioEdit/Chromas versions; original signal and orientation remain correct.
- [x] Representative research files, including the 5,555-sequence alignment, pass the normal editing/grouping/export workflow. Oversized Flush/gap edits produce a working-memory-limit message and leave rows/history usable; the limit is additional estimated working storage, not a maximum input size.

## Distribution acceptance — results/decisions still required

For the new installer workflow, use **WINDOWS_INSTALLER.md** and the
`test-installer` target. The new 0.27.0 installer requires a Windows build and acceptance. Basic
0.26.11 test-2 ARM installation/launch was reported; broader checks remain.

For the older folder/ZIP packaging workflow after validation:

```powershell
./tools/windows/package.ps1 -QtRoot C:/Qt/6.5.3/msvc2019_64
```

The script installs into a new versioned folder, runs that kit's `windeployqt`, checks the Windows platform plugin, retains available Qt notices, and creates a checksum list and ZIP. It refuses to overwrite an existing package folder/archive. It prepares a portable test package; installer creation, signing, clean-machine acceptance and distribution compliance remain separate release tasks. The script has not been executed on Windows in this environment.

- [x] Application license chosen by Oleg Shchepin: GNU GPL version 3 only. LICENSE and LICENSE-STATUS.md record the terms; BRANDING.md records the separate artwork policy, credit and permission. Artwork integration remains pending.
- [ ] Ship LICENSE, BRANDING.md, retained credits and applicable third-party notices; provide GPL-compliant corresponding source for the exact distributed binaries.
- [ ] Review notices and distribution requirements for the exact Qt/compiler binaries included. Follow [Qt 6.5 deployment documentation](https://doc.qt.io/qt-6.5/windows-deployment.html); do not assume deployment tooling gathers every third-party dependency or notice.
- [ ] Test the package on a clean Windows machine without Qt Creator or a Qt development PATH. Confirm required MSVC runtime installation and startup/plugins.
- [ ] Decide installer/file associations, signing and public distribution channel. The portable package does not register associations.
- [ ] Record acceptance results and any remaining known limitations before tagging RC1.

## Checkpoint-B gates

- [x] Native 0.26.12 test-1 acceptance reported: 33/33 and focused manual checks.
- [x] Real native x64 MAFFT 7.526 Automatic (safe), resolved --thread 8, passed.
- [ ] 0.27.0 Qt 6.5.3 / MSVC2022 x64 Release: 37/37 plus new manual checks.
- [ ] Settings Apply/Cancel/reset, restart and multiple windows with session precedence.
- [ ] Recovery of disposable data after process termination, cancellation, quota
  refusal, older generation and Save As; source files must remain unchanged.
- [ ] Large-file overview memory behavior on representative research inputs.
- [ ] Native dialogs at relevant DPI and narrow window sizes.
- [ ] Real ARM MAFFT/Settings/recovery when the colleague is available.
- [ ] Checkpoint-C trace mapping ground truth and application acceptance before RC.
- [ ] Build and accept the matching 0.27.0 installer, including clean-machine uninstall.

The local test record is VALIDATION.md. Large storage/controller rewrites remain
deferred; no global catch-and-continue handler or fatal-path autosave was added.
