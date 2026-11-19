# Testing and release checklist

## Automatic suite

Enable BUILD_TESTING and KLEIOTIDE_BUILD_RELEASE_CHECKS, build Release and run the release-check target. The current suite has 51 tests (21 core, nine Qt unit, 21 GUI workflows). It saves release-tests-Release.log and release-tests-Release.xml in the build directory. From Qt Creator, select release-check as the build target and build it; the Run button launches an executable and does not run CTest.

```sh
cmake --build build --config Release --target release-check --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

On Windows the script tools/windows/validate.ps1 can configure/build/test with a specified QtRoot. GUI tests use disposable data and loopback HTTP, never NCBI. Optional installed BLAST+ validation sets KLEIOTIDE_TEST_REAL_BLAST=1 before running kleiotide_local_blast_check. Use isolated test preferences and synthetic/public inputs for external-tool tests.

## 0.30.1 focused acceptance

1. Open Double peak calling and confirm the secondary threshold starts at 60%. Hover each setting, label and preview column. The displacement tooltip should explain that one sample is a recorded signal point.
2. Recheck a visibly short secondary peak at a 90% threshold and a site with a broad dominant signal above two smaller peaks. Confirm that the height ratio agrees with the displayed signal and that a site with no clear apex in the tallest signal is skipped.
3. Check a genuine pair that excludes the current symbol. It should start unchecked and show “Detected peak pair does not include the current symbol”. Verify that copy and all three save formats include the updated rules and version.

## 0.30.0 focused acceptance

1. Open an alignment with linked traces. Find Double peak calling in its own section just after Delete trace in the Sequence menu and sequence-name context menu. Switch between Selected and All. Change each threshold and check that the preview refreshes.
2. Use a trace with visually identifiable double peaks, shifted peaks, and sites with three peaks. Inspect the proposed alignment columns by clicking rows. Compare both strands, manually adjusted anchors, and a sequence with gaps. Check the inclusive secondary threshold and the third-peak exclusion. Confirm that positions without an anchor are counted and skipped.
3. Uncheck a supported proposal and review a mismatched or ambiguous current symbol. Copy the preview and save XLSX, TSV and CSV. Check the rules, version, ratios, alignment columns and checked status. Apply changes, then verify Undo/Redo and save/reopen. Verify Cancel leaves residues and trace data intact.

## 0.29.10 focused acceptance

1. Click between calls in the chromatogram and press and release Ctrl or Shift. The insertion caret should stay at the clicked sample. Type a capital nucleotide with Shift. Check both trace spacing modes.
2. Mark a mixture of gaps and residues manually and add low-quality marks on a linked read. Insert and delete gaps or bases before and inside the marked ranges. Drag a block through open gaps and slide it back. Check that surviving marks follow their cells, deleted ones disappear, and new cells start unmarked. Verify Undo/Redo, reverse complement, and save/reopen. Check that column shading still follows its rules.
3. Build the Debian 13 package and run release-check without BLAST+ installed. Install the package and inspect its GPL-3.0-only metadata in GNOME Software. Check the two resize handles in light and dark themes, then run the remaining Debian desktop checks.

## 0.29.9 focused acceptance

1. In File → Settings → Residue palette, click the U and R swatches and choose distinct colors. Check that Custom is selected under Appearance and editing and the preview reflects both changes. Test color-picker Cancel, Settings Cancel, Apply, Restore Defaults and reopened Settings. Check that new alignments use the saved defaults and that Apply to current session updates its alignment and trace legend.
2. Delete an original base between two current calls. Click its crossed-out letter on the Imported line and type a nucleotide. Compare the inserted sample with the deleted call's original sample and check the position relative to surrounding bases. The new call has no original Q.
3. Click blank space between Current calls, a peak without a call, and an open position between peaks. Type a base at each caret. Repeat with both flanks, a reverse-complemented trace, alignment gaps, Sample spacing and Alignment spacing. Verify Undo/Redo, native save/reopen and edited AB1 export.

## 0.29.8 focused acceptance

1. With a linked chromatogram visible, drag across the overview and confirm the trace scrolls during the drag. Drag each panel grip slowly up and down. Each panel should follow the pointer, including after hiding and reopening the other panel.
2. In Edit with Sample spacing, drag an original call towards a neighbor and check it stops before crossing. Verify the alignment text, imported position and Q. Click each flank, check the alignment insertion caret, type a base and verify its signal position. Reposition the inserted base. Repeat with a reversed trace, leading/trailing alignment gaps, and Alignment spacing. Select several bases from the alignment and check the trace displays the selection. Check Undo/Redo, native save/reopen, edited AB1 export in Chromas, and a previous format-5 session.
3. Mark a gap-only selection. Apply a low-quality snapshot to selected traces, unhighlight a qualifying cell, and rerun the same settings. Check the cell is marked again. Save and reopen with both kinds of marks and a column rule. Verify older low-quality rules and exclusions migrate to the shared marks.
4. Hold a click and make a horizontal selection in the top and bottom visible rows. The view should stay on the same rows. Drag outside the viewport to verify continued autoscroll. Change rows and columns per wheel step in Settings and test wheel, horizontal wheel and Ctrl+wheel.
5. Make a custom palette in View and in Settings, including U and an ambiguous base. Check cancel, default settings, session round trip, the trace legend and the inverted selection colors. Verify the Motif search toggle appears beside the alignment overview toggle. Check the Flush alignment label and the Colored residue backgrounds tooltip.
6. Build on Apple Silicon using the README instructions. Open an AB1, edit calls, save/reopen, export and run Homebrew MAFFT after launching from Finder. Repeat the [Windows installer checks](WINDOWS_INSTALLER.md#release-candidate-installation-and-upgrade) and [Debian desktop checks](DEBIAN_PACKAGE.md#install-and-test).

## 0.29.7 focused acceptance

1. Build with MSVC and check the merge preview compiles without the C4456 warning for `label`.
2. In View → Residue display → Colored residue backgrounds, manually highlight an isolated base and an adjacent block. Clear the residue selection and verify the pale fill, dark border and readable letters. Enable a column rule and verify even shading across rows and the ruler in each residue display mode. Repeat with protein rows, translation enabled, two palettes, small and large fonts and Windows scaling.
3. With a chromatogram open, drag across the overview on the 3440×1440 display with the selected block inside, partly outside and fully outside the visible alignment. The alignment follows the drag and the chromatogram recenters on release. The selection remains intact. Repeat by clicking the overview and by scrolling the alignment directly.
4. In View, check the separators around Restore default layout immediately above Alignment font. Hover just above the chromatogram grip and check that only the grip offers resizing. Drag both grips, hide/reopen both panels and restore the default layout.
5. Enable a column rule and low-quality highlighting. Unhighlight one low-quality call and manually mark a different base. Save to `.kleio`, reopen, and verify all three layers and the exclusion. Clear highlighting, save, and reopen to verify the cleared state. Open an earlier session and check that its saved column and quality rules still load.
6. Run the [Windows RC installation and upgrade checks](WINDOWS_INSTALLER.md#release-candidate-installation-and-upgrade), starting with a fresh default-folder install. Follow the [Debian desktop checks](DEBIAN_PACKAGE.md#install-and-test) on Debian 13.

## 0.29.6 focused acceptance

1. Open an alignment with a linked trace. Drag each bottom panel's grip and check that the other panel keeps its height. Hide and reopen each panel in turn. The alignment should gain the freed space, and the reopened panel should return at its prior height. Restore the default layout and verify that the overview is below the chromatogram.
2. Highlight one residue and a block of adjacent residues, then clear the selection to inspect the highlights. Verify that each letter remains legible within its border. Repeat with low-quality highlights, small and large alignment fonts, and Windows display scaling.
3. Run the [Windows RC installation and upgrade checks](WINDOWS_INSTALLER.md#release-candidate-installation-and-upgrade), starting with a fresh install into the default folder before it exists. Follow the [Debian desktop checks](DEBIAN_PACKAGE.md#install-and-test) on a Debian 13 machine.

## 0.29.5 focused acceptance

1. Drag the sides and lower edge of the whole, selected, selected-column and template MAFFT dialogs. Width should change and height should remain fixed. Repeat with nucleotide and protein data at different display scales.
2. Select nonadjacent sequence names and use Prepend row numbers with a custom or empty separator. Check one-based positions in the whole alignment, preview, Cancel, Rename and undo.
3. Open an alignment with traced and untraced rows. Select a traced residue, then an untraced residue, and clear the residue selection. The chromatogram should clear in both cases. Choose a trace in its dropdown and verify it selects its row's first residue.
4. Drag the grips above the chromatogram and overview independently. Verify that the overview stays at the bottom and the other panel keeps its height. Toggle the panels and restore the default layout. Select a single column and a broad range. The top bracket should mark the selection, while the blue viewport frame stays visible.
5. In Highlight → Highlight low-quality calls, test all traces and a selected-name subset. Verify strict threshold, Q0, missing Q, gaps, inserted or edited calls, and reverse traces. Mark a block manually and unhighlight part of it from the right-click menu. Check that sequence data and undo history remain untouched.
6. Run the [Windows RC installation and upgrade checks](WINDOWS_INSTALLER.md#release-candidate-installation-and-upgrade). Follow the [Debian desktop checks](DEBIAN_PACKAGE.md#install-and-test) on a Debian 13 machine.

## 0.29.4 focused acceptance

1. In one window open a file from folder A, save/export in B and open a different file dialog: B should be suggested. Repeat with tables, AB1, executable browsing and directory selection, then restart or launch another instance. Suggested filenames and extension/protection checks must remain correct. Navigate elsewhere and Cancel: the previous folder should remain. Rename/remove a remembered folder and verify a safe fallback. Settings Restore defaults must not erase navigation history.
2. BLAST Resume job must still start in the normal blast-jobs folder. Cancel or load a saved job and confirm the ordinary remembered folder is unchanged. Automatic jobs/recovery must not redirect subsequent dialogs.
3. Select a single residue, a row fragment, a multirow block and ruler columns. Use Select to beginning/end in Edit and the selected-residue context menu. Keep the same rows; extend to alignment column 1 or the last alignment column. Check View/Overwrite/Insert, ragged rows, horizontal scrolling, Clear fragment selection and independent name selection. These commands must not change sequence letters, dirty state or history.
4. Verify the new selection section is just above Rename, and the three existing motif commands have their own section. Existing shortcuts and right-click gap-edit modes retain their behavior.
5. Start with a distant vertical scroll and use each Sequence → Select family: type/traces, all/invert, groups, one per group, names/regular expressions, duplicates, motif and random. The uppermost selected row must become visible; horizontal position and residue selection stay unchanged. No-match/cancel/unselect must not jump. Reopen a session and verify its saved scroll position still wins.
6. Inspect Download MAFFT in whole/selected/column options, full/fragment template workflows and Settings. It opens the official platform-appropriate page without changing options or starting a download. Check compact layout at 100/125/150/200% scaling.

## General native and distribution checks

- Open FASTA/AB1/native sessions, edit, Undo/Redo, copy across windows, save/reopen and inspect original trace evidence. New session format 6 requires 0.29.8 or later. Formats 1–5 remain readable.
- Verify Explorer Open With, drag/drop and multiple interactive windows. Retest native DPI, name-divider cursor and compact dialogs.
- Test real MAFFT success/cancellation and local BLAST tools. For NCBI use one authorized small public query; retain RID/diagnostics and resume it instead of submitting repeated copies.
- Test protected/unwritable paths, final suffix/overwrite checks, failed/cancelled writes and source-file protection. Do not use the only copy of valuable data for crash/full-disk tests.
- Build the matching installer and test launch, TLS, file opening, uninstall/reinstall on a clean machine without Qt/development tools. Keep matching source and third-party notices with binaries.
- Record the exact application/installer revision, Windows/Qt/compiler, external-tool versions and per-check Pass/Fail/Not tested. A previous version's success is not acceptance of changed code.

See [Windows acceptance](WINDOWS_ACCEPTANCE.md), [installer preparation](WINDOWS_INSTALLER.md) and [ARM status](ARM64_COMPATIBILITY.md). No automated offscreen GUI result alone establishes native display/TLS or external-reader behavior.
