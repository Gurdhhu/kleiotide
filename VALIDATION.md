# Validation of Kleiotide 0.27.1

27 October 2026. This follows the user's accepted native Windows 0.27.0 test 1.
The source archive is a testing deliverable, not a Windows installer produced
here. No Git commit/push or new native Windows/ARM run is claimed.

## Local results

- Release application, fixture and checks built using GCC 13.3 and Qt 6.4.2
  on Linux. Only the validation copy's Qt minimum was lowered from 6.5 to 6.4;
  delivered CMake retains Qt >=6.5 for the user's Qt 6.5.3 MSVC2022 x64 kit.
- All 40 application/fixture/test executables were verified as nonempty ELF
  binaries before execution; no empty executable was counted as a passing test.
- Complete registered CTest suite: **38/38 passed**, no skips/disabled entries.
  19 core suites, six Qt unit suites, thirteen GUI workflows, with captured output.
- GUI workflows at 150%: **13/13 passed**.
- Windows-style offscreen tests at 200%: **4/4 passed** (merge, Settings/recovery,
  mode controls and new release fixes). This does not establish native rendering.
- Offscreen captures inspected for toolbar row placement, overview selection/
  viewport markers, Settings scrolling and merge result banner/footer. A wrapped
  merge-banner height issue found at 200% was corrected and retested; original
  read alignment is independently captured after scrolling it into view.
- Targeted AddressSanitizer + UndefinedBehaviorSanitizer: overview bounds suite
  and all 40 internal merge regressions passed. Changed alignment.cpp,
  overview.cpp and merge.cpp were instrumented; other linked core objects were
  not. Leak detection disabled; no GUI sanitizer or whole-program leak claim.
- Normal native Kleiotide preference file unchanged across isolated test runs.

## Correctness coverage added

Incremental overview summaries are compared with independent fresh full scans,
including randomized overwrites, ambiguity/gaps, rapid edits during a partial
scan, Undo/Redo, expired invalidation history and structural/length fallbacks.
The single-row fast path matches the ordinary per-column IUPAC model. A
million-column two-row alignment still respects a 256 KiB workspace bound.

Merge regressions verify original read calls survive final-contig indel placement,
all differing columns are marked (including compatible ambiguity differences),
partial contigs can Apply/Save without opt-in, and the read alignment is present.
Existing conflict/Q/contig/cancellation/stale-source checks remain.

The new GUI workflow checks menu sections, Overwrite, batch FASTA/AB1 append by
drop with original trace evidence and one Undo, unsupported remote URLs, explicit
session appearance without writing global defaults, translation row placement
after repeated restores/resizes, adjacent toggle and About GitHub/citation.
FASTA suffix tests cover absent/custom/recognized upper/lowercase suffixes;
existing atomic output and trace/session protections remain covered.

## Performance observation

Direct core benchmark, one 50,000,000-column nucleotide row containing bases,
ambiguity, missing calls and gaps, 8,192 summaries: initial scan 0.171598 seconds;
one overwrite update 0.00006002 seconds, rescanning 6,103 columns. Owned overview
workspace: 1,638,408 bytes. These are local CPU timings without GUI scheduling,
not a guarantee or an extrapolation to the user's 729 Mbp Windows assembly.
The important bound is the touched summary range rather than the full sequence.
Initial calculation still reads symbols; structural changes may still rescan.

## Native acceptance still required

Use RELEASE_0.27.1_TESTING.md and WINDOWS_INSTALLER.md in a fresh build directory.
Expected release-check: 38 tests. Windows Explorer's Open With loop was not
reproduced here; installer registration is a targeted repair for suspected stale
commands and needs native validation, including side-by-side uninstall, alternate
administrator accounts and Unicode paths. NSIS/PowerShell execution was not
available here; no Windows installer compilation or registry execution is claimed.

The colleague's earlier ARM 0.26.11 test 2 / explicit Threads=1 success is retained
as a reported baseline. Retest Automatic/fallback and cancellation on ARM when
available. No live NCBI submission occurred; BLAST and trace linking are 0.28.0
work described in RELEASE_PLAN_0.27.1_0.28.0.md.

The outer release bundle includes final logs/XML, executable checksums, targeted
sanitizer output, benchmark source/output and the source-difference manifest.
Historical 0.27.0 validation remains under docs/history/.
