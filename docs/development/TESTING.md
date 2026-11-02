# Testing and release checklist

## Automatic suite

Enable BUILD_TESTING and KLEIOTIDE_BUILD_RELEASE_CHECKS, build Release and run the release-check target. The current suite has 48 tests (20 core, nine Qt unit, 19 GUI workflows). It saves release-tests-Release.log and release-tests-Release.xml in the build directory. From Qt Creator, select release-check as the build target and build it; the Run button launches an executable and does not run CTest.

```sh
cmake --build build --config Release --target release-check --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

On Windows the script tools/windows/validate.ps1 can configure/build/test with a specified QtRoot. GUI tests use disposable data and loopback HTTP, never NCBI. Optional installed BLAST+ validation sets KLEIOTIDE_TEST_REAL_BLAST=1 before running kleiotide_local_blast_check. Use isolated test preferences and synthetic/public inputs for external-tool tests.

## 0.29.4 focused acceptance

1. In one window open a file from folder A, save/export in B and open a different file dialog: B should be suggested. Repeat with tables, AB1, executable browsing and directory selection, then restart or launch another instance. Suggested filenames and extension/protection checks must remain correct. Navigate elsewhere and Cancel: the previous folder should remain. Rename/remove a remembered folder and verify a safe fallback. Settings Restore defaults must not erase navigation history.
2. BLAST Resume job must still start in the normal blast-jobs folder. Cancel or load a saved job and confirm the ordinary remembered folder is unchanged. Automatic jobs/recovery must not redirect subsequent dialogs.
3. Select a single residue, a row fragment, a multirow block and ruler columns. Use Select to beginning/end in Edit and the selected-residue context menu. Keep the same rows; extend to alignment column 1 or the last alignment column. Check View/Overwrite/Insert, ragged rows, horizontal scrolling, Clear fragment selection and independent name selection. These commands must not change sequence letters, dirty state or history.
4. Verify the new selection section is just above Rename, and the three existing motif commands have their own section. Existing shortcuts and right-click gap-edit modes retain their behavior.
5. Start with a distant vertical scroll and use each Sequence → Select family: type/traces, all/invert, groups, one per group, names/regular expressions, duplicates, motif and random. The uppermost selected row must become visible; horizontal position and residue selection stay unchanged. No-match/cancel/unselect must not jump. Reopen a session and verify its saved scroll position still wins.
6. Inspect Download MAFFT in whole/selected/column options, full/fragment template workflows and Settings. It opens the official platform-appropriate page without changing options or starting a download. Check compact layout at 100/125/150/200% scaling.

## General native and distribution checks

- Open FASTA/AB1/native sessions, edit, Undo/Redo, copy across windows, save/reopen and inspect original trace evidence. New session format 5 requires 0.29.2 or later; old formats remain readable.
- Verify Explorer Open With, drag/drop and multiple interactive windows. Retest native DPI, name-divider cursor and compact dialogs.
- Test real MAFFT success/cancellation and local BLAST tools. For NCBI use one authorized small public query; retain RID/diagnostics and resume it instead of submitting repeated copies.
- Test protected/unwritable paths, final suffix/overwrite checks, failed/cancelled writes and source-file protection. Do not use the only copy of valuable data for crash/full-disk tests.
- Build the matching installer and test launch, TLS, file opening, uninstall/reinstall on a clean machine without Qt/development tools. Keep matching source and third-party notices with binaries.
- Record the exact application/installer revision, Windows/Qt/compiler, external-tool versions and per-check Pass/Fail/Not tested. A previous version's success is not acceptance of changed code.

See [Windows acceptance](WINDOWS_ACCEPTANCE.md), [installer preparation](WINDOWS_INSTALLER.md) and [ARM status](ARM64_COMPATIBILITY.md). No automated offscreen GUI result alone establishes native display/TLS or external-reader behavior.
