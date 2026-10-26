# Windows check — Kleiotide 0.5

## Corrected source archive

Use `Kleiotide_0.5_Source_fixed.zip`. Stop the previous build, extract this ZIP
into a fresh parent folder, and open the extracted CMakeLists.txt in Qt Creator.
Keep the same Qt 6.5.3/MSVC2022 kit and Release configuration, with a fresh build
directory. Do not copy the previous build directory or CMakeLists.txt.user file.

The original archive's CMakeLists.txt was stamped 2026-10-08 23:03:04 while the
reported Windows build began at 22:25:38. Ninja repeatedly regenerated a build
file that remained older than its CMake input, then failed with "manifest
'build.ninja' still dirty after 100 tries". The corrected archive stores all file
dates as 2000-01-01 to avoid future source dates after extraction. Application
source contents and the Qt requirement are unchanged. The optional
WrapVulkanHeaders discovery message is unrelated to this timestamp correction.

## Build and MAFFT installation

1. Extract Kleiotide_0.5 into a fresh folder. Open CMakeLists.txt in Qt Creator
   and build Release with your existing Qt 6.5.3/MSVC2022 kit.
2. If MAFFT is not already installed, download its official Windows all-in-one
   package: https://mafft.cbrc.jp/alignment/software/windows_without_cygwin.html
   Extract the complete folder to a stable location, for example C:\Tools.
3. Keep mafft.bat beside its accompanying mafftdir/runtime files. Kleiotide does
   not bundle MAFFT. Select the mafft.bat launcher when opening alignment settings.
   The first test can use an ASCII installation path; then check spaces/Unicode
   paths with your actual Windows installation.

## Whole-alignment workflow

1. Open examples/mafft_demo.fasta. Keep the source window open.
2. Alignment → MAFFT → Align whole alignment. Browse to mafft.bat, keep Automatic
   and automatic threads, then click Align. Check the progress log and UI response.
3. The result should open in a new window. Titles/order/case and every nucleotide
   residue must survive; empty rows become gap-only rows. The mask stays gray
   and its text is unchanged. The original window/file is unchanged.
4. Check the report from either window. It records source revision, version,
   parameters, scope, excluded masks and reverse-complemented row IDs.
5. Save the result as a new FASTA, reopen and compare. Make a manual edit and
   confirm its undo/redo works. The original window retains its own history.

## Column realignment

1. Return to the source. In Select / Slide, select a continuous column range
   using a substring, block or ruler.
2. Alignment → MAFFT → Realign selected columns. This uses those columns across
   all nucleotide rows; selecting fewer names does not restrict participating rows.
3. Confirm prefix and suffix symbols, case and locked/open gaps remain exact.
   The right-hand suffix moves together if the fragment width changes.
4. A mask's fragment text is retained, with ~ padding when the new region is
   longer. Stored mask text sets a minimum width, preventing annotation truncation.
   Empty and ragged fragments receive gaps where needed for the new column range.
5. Strand adjustment must remain disabled in a column job. Whole-job gap states
   within the realigned region are intentionally discarded; new gaps are ~.

## Options and cancellation

1. Try a fast FFT-NS preset, then L-INS-i or E-INS-i on a small file. E-INS-i starts
   with offset 0. Change opening/offset and iteration limits, and verify the report.
2. For a file containing forward and reverse-oriented rows, use fast or accurate
   whole-row strand adjustment. Inspect the changed rows and report.
3. Cancel an active job using Cancel or the progress window's close button.
   It must stop the process tree, open no result and leave the source unchanged.
4. During a longer job, use File → New window and work with another alignment.
   The source window stays read-only until completion/cancellation.
5. Check a missing/broken launcher and an insufficient scope (fewer than two
   nonempty nucleotide rows). Errors should preserve the source and show logs.
6. Recheck clipboard, dragging, sorting, masks, overview and smooth scrolling.

Linux Release and sanitizer checks include real MAFFT 7.525 runs. The Windows
batch launcher, Unicode/space paths and Windows process-tree cancellation are
implemented but need this local runtime check with your machine/kit.
