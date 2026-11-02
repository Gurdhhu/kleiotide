# Testing and release checklist

## Automatic suite

Enable BUILD_TESTING and KLEIOTIDE_BUILD_RELEASE_CHECKS, build Release and run the release-check target. The current suite has 47 tests (20 core, nine Qt unit, 18 GUI workflows). It saves release-tests-Release.log and release-tests-Release.xml in the build directory. From Qt Creator, select release-check as the build target and build it; the Run button launches an executable and does not run CTest.

```sh
cmake --build build --config Release --target release-check --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

On Windows the script tools/windows/validate.ps1 can configure/build/test with a specified QtRoot. GUI tests use disposable data and loopback HTTP, never NCBI. Optional installed BLAST+ validation sets KLEIOTIDE_TEST_REAL_BLAST=1 before running kleiotide_local_blast_check. Use isolated test preferences and synthetic/public inputs for external-tool tests.

## 0.29.3 focused acceptance

1. Pairwise distance preview: drag every column border, including Sequence; switch distance/count views and verify names and Copy table. Try wide names, horizontal scrolling and 100/125/150/200% scaling.
2. Save every distance report as XLSX/CSV/TSV. Compare method descriptions, scope, formulas, exclusions, groups and every result block. Only worksheet arrangement should differ. Inspect numeric/literal cells in Excel or LibreOffice.
3. Nucleotide composition: Calculation methods must put each setting in column A and its nonempty value in column B. Compare all three formats and fractional-IUPAC settings.
4. Rename: Append true lengths, Append aligned lengths and Append row numbers retain the old title, use Separator and preview automatically. Test empty/custom separators, selected rows, zero length, Undo/Redo and save/reopen.
5. Recovery: type in overwrite/insert mode and move the caret across a due checkpoint. Input must resume without a mouse click. The write uses a temporary safety gate, a status message and Cancel checkpoint, without a focus-stealing popup. Switching to another main window must not activate the old one on completion. Test cancellation/quota failure and recovery on disposable data.
6. Settings and both local search types: Download BLAST+ must be a short clickable link to the requested NCBI directory. Browsing is a user action; no automatic download occurs.
7. Review README, topic guides and local links; build the exact source archive and verify installation retains the documentation paths.

## General native and distribution checks

- Open FASTA/AB1/native sessions, edit, Undo/Redo, copy across windows, save/reopen and inspect original trace evidence. New session format 5 requires 0.29.2 or later; old formats remain readable.
- Verify Explorer Open With, drag/drop and multiple interactive windows. Retest native DPI, name-divider cursor and compact dialogs.
- Test real MAFFT success/cancellation and local BLAST tools. For NCBI use one authorized small public query; retain RID/diagnostics and resume it instead of submitting repeated copies.
- Test protected/unwritable paths, final suffix/overwrite checks, failed/cancelled writes and source-file protection. Do not use the only copy of valuable data for crash/full-disk tests.
- Build the matching installer and test launch, TLS, file opening, uninstall/reinstall on a clean machine without Qt/development tools. Keep matching source and third-party notices with binaries.
- Record the exact application/installer revision, Windows/Qt/compiler, external-tool versions and per-check Pass/Fail/Not tested. A previous version's success is not acceptance of changed code.

See [Windows acceptance](WINDOWS_ACCEPTANCE.md), [installer preparation](WINDOWS_INSTALLER.md) and [ARM status](ARM64_COMPATIBILITY.md). No automated offscreen GUI result alone establishes native display/TLS or external-reader behavior.
