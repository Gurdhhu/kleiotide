# Testing Kleiotide on Windows: step by step

These instructions are for Qt Creator with a 64-bit MSVC kit. Your usual
Qt 6.5.3 / MSVC 2022 x64 kit meets the project's Qt >=6.5 requirement. An
installed Qt 6.8.1 / MSVC x64 kit also works as a build choice; record the exact
one used. Keep different Qt kits in separate build directories.

There are two complementary checks:

- **Automatic tests:** the program's test executables check expected results
  through CTest. The current complete suite has **30 tests**: 17 core suites,
  four Qt unit suites and nine GUI workflows. Each entry contains many checks.
- **Manual checks:** you run Kleiotide normally and use real Windows dialogs,
  mouse/clipboard, display scaling and research files. Automated GUI checks run
  offscreen and cannot establish every native Windows interaction.

No Python, MAFFT installation or separate testing framework is needed for the
registered automatic suite. The MAFFT cancellation test uses a controlled
process fixture. Testing the real MAFFT installation is a separate manual step.

## 1. Open a fresh source release

1. Extract the source ZIP into a new folder, such as
   `C:\Kleiotide-testing\Kleiotide_0.26.11`. Avoid mixing source releases.
2. In Qt Creator, open that folder's `CMakeLists.txt` as you usually do.
3. Select your existing **MSVC x64 Qt kit**, choose **Release**, and configure.
   Use a fresh build directory for this release.
4. In **Projects > Build Settings**, note the **Build directory**. Test reports
   will be written there. It is commonly outside the source directory.

## 2. Enable the automatic suite

In **Projects > Build Settings**, find the **CMake** configuration table:

1. Search for `KLEIOTIDE_BUILD_RELEASE_CHECKS`; set it to **ON** (checked).
2. Check that `BUILD_TESTING` is **ON** too. Keep `KLEIOTIDE_BUILD_GUI` **ON**.
3. If a variable is not listed, use **Add > Boolean** and enter its exact name
   and ON value. The equivalent CMake arguments are:

   ```text
   -DBUILD_TESTING=ON
   -DKLEIOTIDE_BUILD_RELEASE_CHECKS=ON
   ```

4. Select **Run CMake** / apply the configuration changes and wait for success.
   Leave developer/benchmark and historical GUI-check options OFF.

Qt Creator wording/layout can vary slightly by version. The values belong in
the CMake variable configuration, not the application's command-line arguments.

## 3. Build and run the automatic checks inside Qt Creator

1. In **Projects > Build Settings > Build Steps**, expand the **CMake Build**
   step if it is collapsed.
2. In its build-target selection, choose **`release-check`** for this run,
   replacing `all` or the previous default target.
3. Choose **Build Project** (**Ctrl+B**).
4. Wait for compilation and then the CTest results in **Compile Output**. The
   first build takes longer because it builds the test programs as well.
5. Success ends with:

   ```text
   100% tests passed, 0 tests failed out of 30
   ```

The target builds the app and all required test programs before running CTest.
The green Run button launches a selected executable; **Ctrl+B with the
`release-check` target** performs this automated run. Small/offscreen test
windows may be created automatically; no input is required.

The build directory receives:

- `release-tests-Release.log` — text report;
- `release-tests-Release.xml` — machine-readable JUnit report;
- `Testing/Temporary/LastTest.log` — detailed CTest output.

### If an earlier release reported 0xc0000135

In the supplied 0.26.9 Windows run, compilation/linking succeeded and all 17 core
tests passed. The four Qt unit suites and nine GUI workflows failed to start
with `0xc0000135` (a required DLL could not be found). That pattern points to the
Qt DLL search path in the build/CTest environment, which can differ from Qt
Creator's normal Run environment; it does not identify a failed test assertion.

Version 0.26.10 configures the Qt-dependent tests to prepend the selected Qt
kit's DLL directory to their own PATH, retaining the existing entries. This
applies both to `release-check` and direct CTest. It uses the configured Qt Core
target for the active Release/Debug configuration. No global PATH change or
manual DLL copying is required for the test setup.

Extract 0.26.10 into a fresh folder, open its CMakeLists.txt, select the same
kit/Release configuration, enable the same two test options, select
`release-check` and press Ctrl+B again. Keep the new log/XML reports. The subsequent 0.26.11 Windows rerun confirms all 30 tests pass. If a DLL startup
failure persists, send the new output; the original log does not name the
individual missing DLL.

If a test fails, keep these files and copy the failed test's name and error text.
Also save Qt Creator's Compile Output if compilation failed. A build failure or
"No tests were found" is not a successful test run. The release target reports
an error when no tests are registered.

## 4. Run Kleiotide for manual testing

1. Return the build target to **`kleiotide`** (or your usual `all` target).
2. Select **`kleiotide`** as the run configuration in Qt Creator. Enabling tests
   adds other executables to that list, so confirm the application is selected.
3. Build and press the green **Run** button, as you normally do.
4. In **Help > About Kleiotide**, verify version **0.26.11**, Oleg Shchepin's
   copyright, GPLv3-only notice and Elizaveta Shchepina's designer credit.

Use a separate output folder and test copies of your research files. Record
Pass / Fail / Not tested for each applicable item below. Unavailable external
programs or checks can be marked Not tested with a short reason.

| Area | What to do | Expected result |
| --- | --- | --- |
| Ordinary editing | Open `examples/nucleotide_demo.fasta`; select, type, insert, delete, slide and lock gaps; Undo/Redo; save and reopen. | Symbols, case and gap states follow edits; Undo restores the previous state; the saved file reopens correctly. |
| Windows interaction | Try mouse-drag selections, independent name/residue selections, copy/paste between two Kleiotide windows and file opening from Explorer if associations are already configured. | Correct rows/fragments are targeted, metadata is retained by native sequence copying, and existing windows/documents are handled as expected. No association setup is required for these source tests. |
| Groups | Open `examples/groups_demo.fasta`; create/reorder groups, change colors/members, filter names, check/uncheck, delete empty groups and test Clear and delete all groups. | Counts/colors follow staged membership; bulk checks affect visible names; Clear asks for confirmation; Cancel leaves the document unchanged and Apply has one Undo step. |
| Group tables | Import `examples/groups_demo.tsv`, paste a wider table and map its sequence/group columns; try parenthesized cells and a malformed large table. | Correct names match, previews explain invalid input, the window remains responsive, and Cancel prevents application. |
| Identity grouping | Test a small alignment, then the 5,555-sequence research alignment. Cancel a preview and run a valid one; inspect representative selection and export its preview. | Cancellation works, only the current preview applies, group order matches the manager, Ungrouped One/All works and Undo restores prior groups. |
| AB1 import | Open ordinary AB1 files, the earlier stale-name-offset file, and the supplied file with x at call 523. | Ordinary files open normally; recoverable files show an import notice. The x call imports as n while calls/peaks/Q/signals remain aligned. |
| Trace edits and export | Inspect traces; edit, reverse-complement, save a .kleio session and reopen it; export an edited AB1 copy and batch-export several traces. | Current symbols/orientation and original evidence survive sessions; exports do not overwrite original AB1s. Open outputs in your usual BioEdit/Chromas and compare the calls and signals. |
| Translation and Save | Change frame, code or annotation; Save a .kleio session, close/reopen, test Save-before-close and cancel Save As. Export FASTA separately. | Metadata persists in sessions, cancelled saves keep the source open/unsaved, and FASTA export does not falsely mark unsaved session metadata as saved. |
| Output paths | Use filenames with spaces and Unicode. On a test output, try an existing filename, a nonstandard typed suffix and an unwritable destination. | The final path is confirmed before replacement; No/Cancel preserves the old file; errors are reported without damaging existing output. |
| Analysis and responsiveness | Run motif search, statistics, distances, merge and collapse using the supplied examples. Cancel a background operation. While a job is running, inspect translation/edit controls. | Results/Undo agree with the preview; Cancel leaves source data intact; busy-source mutation controls are unavailable. |
| Real Windows MAFFT | With your existing MAFFT installation, align a small example, then cancel a running job. | A valid result opens separately, the original is retained, and cancellation stops the job. The automatic fixture does not establish this real-installation result. |
| Display scaling | At Windows display scaling 100%, then 150%, restart the app and inspect ruler, names, trace labels, selection and dialogs. | Text remains visible, mouse coordinates match cells, and controls do not overlap or become inaccessible. |

When reporting a failure, include the exact action, expected and actual result,
whether it repeats, the smallest input that reproduces it, and a screenshot if
the problem is visual. For an overwrite/save issue, include the exact entered
filename and file type.

## 5. What to send back

- Windows version; Qt Creator version; exact Qt kit/version; MSVC version;
  Release/Debug; Windows scaling percentage.
- Automatic result (30/30 or failed names), the text log and JUnit XML.
- Manual checklist results and descriptions of failures or skipped checks.

The automatic suite previously passed on Linux. A successful run on your PC is
the first evidence for this exact Windows/Qt/MSVC combination. Passing it plus
manual acceptance supports release preparation; it does not claim every possible
file or Windows installation has been tested.

## Optional: command-line equivalent and existing PowerShell script

You can do everything above through Qt Creator. For a configured Qt/MSVC build
environment, the equivalent build/test commands are:

```powershell
cmake --build "C:\path\to\your\build-directory" --config Release --target release-check
```

To run just CTest again after the binaries are built:

```powershell
ctest --test-dir "C:\path\to\your\build-directory" -C Release --output-on-failure --no-tests=error
```

Run these with the compiler/build tools available, or use the full paths to
cmake.exe/ctest.exe configured in Qt Creator. Version 0.26.10 supplies the Qt DLL
directory to the Qt-dependent CTest processes; the terminal does not need a
global Qt PATH entry for them. Reuse the configured build directory rather than
creating a second configuration with a different generator or Qt kit.

The optional `tools/windows/validate.ps1` creates a separate Ninja Release build
and saves configure/build/test logs plus JUnit/environment records. To use it:

1. Open **x64 Native Tools Command Prompt for VS 2022** from the Start menu,
   then start `powershell -NoProfile` from it so MSVC's environment is inherited.
2. Ensure `cmake`, `ctest` and `ninja` are available. Qt Creator's configured
   CMake/Ninja paths show where your installations are located.
3. Change to the extracted source folder. Run:

   ```powershell
   .\tools\windows\validate.ps1 -QtRoot "C:\Qt\6.5.3\msvc2019_64"
   ```

   Replace the example QtRoot with your actual **64-bit MSVC Qt kit directory**,
   containing `bin\qmake.exe`. It is not Qt Creator's directory or a MinGW kit.
   A kit folder named `msvc2019_64` can be used by your configured MSVC 2022 kit;
   the path must match the Qt version you actually selected.
4. The default output directory is `build-release` inside the source folder.
   Preserve `configure.log`, `build.log`, `tests.log`, `tests.xml` and
   `validation.json`. Choose `-BuildDir` with another fresh path if needed.

If downloaded-script execution is blocked, the Qt Creator route above needs no
PowerShell script or execution-policy change. The script was source-reviewed
here but has not been executed on Windows.

Portable packaging and clean-machine testing come after these checks; follow
RELEASE_CHECKLIST.md. Running from Qt Creator does not test whether a user
without Qt installed can launch the distributed package.

References: [Qt Creator CMake build settings](https://doc.qt.io/qtcreator/creator-build-settings-cmake.html),
[CTest command reference](https://cmake.org/cmake/help/v3.21/manual/ctest.1.html).

## Follow-up checks for 0.26.11

The 0.26.11 follow-up reports 30/30 automatic tests and all manual checks
passing. Repeat release-check when rebuilding for distribution. See
WINDOWS_ACCEPTANCE.md for the environment and reported results, and
WINDOWS_INSTALLER.md for the next packaging/clean-machine stage.

In Save As on Windows, enter ???? and press Save: an explicit invalid-filename
warning should appear, the files should remain listed, and entering a valid
name afterwards should work. Save/export now uses a validated Qt dialog on
Windows so the application can reject a wildcard before it becomes a filter.
File-type choice, final normalized destination confirmation and Cancel remain
part of the checks.

In overlap merging, review the new edit breakdown. If a count remains unclear,
retain the two source sequences or session as well as the screenshot so each
search difference can be traced. Compatible IUPAC symbol changes and repeated
read additions can make search-event totals differ from final conflict columns.

For very large ragged alignments, a bulk operation may now report the 2 GiB
additional working-memory limit. Dismiss the message and verify that the rows,
undo history and subsequent small edits remain usable. The limit prevents
attempting enormous padding/history buffers; it does not restrict opening the
same input for inspection, searching or small edits.
