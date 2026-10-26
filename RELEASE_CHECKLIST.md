# Release checklist

0.26.11 is a stabilization build. Do not label it RC1 until the unchecked gates below have evidence. Feature development is frozen during this pass. Start with WINDOWS_TESTING.md for beginner instructions using Qt Creator, including the existing Qt 6.5.3 / MSVC 2022 x64 kit. Qt-dependent CTest entries resolve the selected kit's runtime directory themselves on Windows.

The user's 0.26.11 follow-up confirms **30/30 automatic tests and all manual
tests passing**, including the bulk-memory guard. WINDOWS_ACCEPTANCE.md records
the tested Windows/Qt/MSVC environment, hardware and MAFFT 7.526. Packaging and
clean-machine acceptance remain pending. Begin with WINDOWS_INSTALLER.md to
produce a versioned internal test installer from this baseline.

## Repeatable automated checks

From the x64 Native Tools for VS 2022 environment, run PowerShell with your Qt MSVC kit:

```powershell
./tools/windows/validate.ps1 -QtRoot C:/Qt/6.5.3/msvc2019_64
```

This configures Release with the release regression suite, builds, runs CTest, and retains logs plus JUnit and environment/result records. It does not change machine execution policy. You may run the CMake/CTest commands inside the script directly instead.

The `release-check` CMake target builds all registered unit tests and nine GUI workflow programs, then runs CTest (30 entries). It retains text and JUnit reports as `release-tests-Release.log` and `release-tests-Release.xml` in the Release build directory and fails when no tests are found. GUI checks cover AB1 import notices, save safety, identity/group management/import, translation, merge, AB1 export, collapse and distances. Historical workflow executables remain available separately.

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
`test-installer` target. The first actual installer still requires a Windows
build and clean-machine installation test.

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

## Refactoring boundary

The reviewed save/export policies, operation locking, duplicate parser and repeated build definitions are addressed in this stabilization build. Old codon-span session migration remains supported; unreachable Document editing branches were removed after checking entry-point normalization. Large controller/undo rewrites remain deferred to avoid mixing structural churn into RC validation.

## Installer test revision 2 — ARM report

The colleague's ARM64 Windows 11 laptop was rejected by test 1's native-only
guard. Test 2 adds a Windows-reported x64 user-mode capability check. Build the
new setup using WINDOWS_INSTALLER.md and complete ARM64_COMPATIBILITY.md.
ARM runtime acceptance remains open; Oleg's existing native x64 application
acceptance is retained. A native ARM64 build is outside this packaging change.
