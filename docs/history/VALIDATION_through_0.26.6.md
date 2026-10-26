# Validation checkpoint — 7 October 2026

For the current results, see the version 0.26.6 checkpoint at the end. Earlier
checkpoints record the implementation and verification state at that time.

## Verified
- GNU g++ 13.3.0, C++20, optimized Linux build.
- Core compilation with -Wall -Wextra -Wpedantic -Werror.
- 19 core test cases, including independent randomized edit/history checks.
- AddressSanitizer and UndefinedBehaviorSanitizer run.
- Leak detection disabled: LeakSanitizer cannot inspect the process environment
  under this runner. No leak-check success is claimed.

Tests cover FASTA parsing/round trips, BOM and CRLF, UTF-8 headers, case, empty and
unequal-length rows, duplicate names, invalid data, stable IDs, insertion,
overwriting, deletion, append, noncontiguous row removal, flush, non-gap rank
indexes, history branching/budget, and exact undo/redo.

## Synthetic scale benchmark

Release core only, 5,000 rows × 100,000 columns:
- Stored symbol bytes: 500,000,000.
- Generate fixture + construct document: 1,977.99 ms.
- 1,000 single-symbol insert/undo pairs at the middle of one row: 8.82 ms total.
- Average insert/undo pair: 0.00882 ms.
- First non-gap rank query across every row: 413.90 ms.
- Peak child-process RSS reported by getrusage: 494,456 KiB (about 482.87 MiB).
- Entire process wall time: 2.414 seconds.

This benchmark used generated sequences, not disk FASTA loading. It does not
measure Qt painting, large rectangular edits, overview updates, MAFFT, AB1, or
Windows behavior. It is a single baseline measurement in this execution
environment, not a universal performance guarantee.

A small input-name validation change followed the benchmark and passed the final
test run; the benchmark was not rerun because that change does not affect the
buffer/editing algorithm.

## Not verified
- CMake configuration and Qt compilation: tools/libraries unavailable here.
- GUI rendering, mouse/clipboard/focus interactions, asynchronous save continuations.
- Windows build, Unicode Windows paths and Windows Explorer clipboard payloads.
- OS file associations and deployment.
- All planned BioEdit slide/lock semantics.
- Protein functionality, MAFFT or chromatograms.

The Qt source has received a manual source review. It is not claimed to be a
compiled or executable GUI. The next checkpoint must compile it with Qt and
complete the interaction check in README.md.


# Validation checkpoint — 8 October 2026, version 0.1.1

- The user validated the 0.1 Windows build: opening/saving, additions, selections,
  deletion, flush, tooltips, undo/redo, View, Insert and Overwrite modes.
- All 21 core tests pass with g++ 13.3 / C++20, normal and ASan/UBSan builds.
  Leak detection is disabled for the runner, as in the earlier checkpoint.
  New coverage includes exact-header table matching, selected scope, duplicate
  names, non-cascading mappings, malformed/conflicting tables, atomic validation,
  stale/repeated IDs, Unicode names, no-op rename and mixed edit/rename undo/redo.
- Core CMake build and CTest pass. Complete Qt frontend and viewport tool compile
  with CMake 3.28.3 / g++ 13.3 / Qt 6.4.2 on Linux. A separate temporary validation
  copy lowers only the Qt package minimum to 6.4; the shipped source still requires
  Qt >=6.5, matching the user's Qt 6.5.3 kit. No Windows 0.1.1 build is claimed.
- Offscreen raster viewport: 3440×1440 logical pixels; 95 synthetic ragged rows,
  up to 1792 columns. 200 horizontal and 200 vertical scroll steps per run.

| Renderer / scaling | Horizontal ms/step | Vertical ms/step |
|---|---:|---:|
| 0.1 original / 100% | 45.158 | 46.561 |
| 0.1.1 cached / 100% | 3.388 | 3.394 |
| 0.1.1 cached / 125% | 13.372 | 15.539 |
| 0.1.1 cached / 150% | 15.957 | 5.606 |
| 0.1.1 cached / 200% | 4.831 | 9.400 |

These are single local Release runs using software rendering and event processing,
not display FPS or promises about Windows. Fractional display scaling increases
raster work and can prevent pixel reuse; it is included in the checks. Background
load and warm/cold cache mixtures cause run-to-run variation. First-frame loading
and cache invalidation while typing are not separately benchmarked.

The viewport check reads the actual raster backing-store pixels without asking
Qt to repaint first. It compares incremental scrolling with full repaints and
checks both directions, large jumps, short-row ends, edits against a fresh view,
undo/redo, selection and font changes. All four scaling factors pass. An early
ruler-label clipping issue was found and corrected before these final results.
Generated viewport screenshots were visually inspected for row/cell alignment,
legible letters, colors, names and ruler placement.

The two provided AB1 files were inspected as ABIF binary directories: both have
PBAS2 base calls, PLOC2 monotone peak locations, PCON2 qualities, FWO_1 channel
order and DATA9–12 analyzed traces. The premix file has 1622 calls / 17242 samples
per channel; MYX15849 has 580 calls / 6998 samples per channel. This is a feasibility
check, not AB1 importer or chromatogram-editor implementation.

The offscreen GUI workflow check also passes: asynchronous FASTA opening with a
Unicode path, both regex selection actions, individual Unicode rename, table
rename with duplicate original headers and non-cascading mappings, dirty state,
and undo/redo through the real menu actions and modal dialogs.

Remaining validation: Windows 0.1.1 build, physical mouse/scrollbar interactions,
Windows clipboard behavior, save continuations, and all deferred slide/lock, MAFFT,
overview and chromatogram workflows.

Reproduce viewport checks by configuring with
`-DKLEIOTIDE_BUILD_VIEWPORT_CHECK=ON`, then running `kleiotide_viewport_check`
with `QT_QPA_PLATFORM=offscreen` and `QT_SCALE_FACTOR=1`, `1.25`, `1.5` or `2`.

# Validation checkpoint — 8 October 2026, version 0.2

- User confirms 0.1.1 scrolling is fast/smooth on Windows and table renaming works.
- 0.2: all 30 core tests pass in Release and AddressSanitizer/UndefinedBehaviorSanitizer
  builds. Leak detection remains disabled for this runner; no leak-check claim.
- Tests include explicit 758–773 → 788–803 reference movement; locked barriers;
  nearest group obstruction; internal gap preservation; atomic group undo; ragged
  virtual cells; fill; gap insertion/deletion; transformations; DNA/RNA IUPAC;
  minimize; malformed transaction rejection; and 200 randomized slide fixtures
  checking independent residue-order/length conservation and exact undo.
- Complete Qt frontend and validation tools compile on Linux, Qt 6.4.2, g++13.3.
  Temporary copy lowers Qt minimum to 6.4 only for local checking; shipped minimum
  remains 6.5 for the existing Windows Qt6.5.3/MSVC2022 kit.
- Real viewport events exercise rectangle, multiple/noncontiguous rows and ruler
  selection, arrow/maximum movement, drag movement, recent selection-state undo,
  fragment context-menu Ns action, right-click gap edits, Escape and edge autoscroll.
  Tests pass at 100% and 200% display scaling.
- Main-window smoke test passes: prior open/regex/rename workflows plus real
  Lock/Unlock, Uppercase, Degap, Reverse complement, Unalign and Minimize actions
  with subsequent regex checks and undo. Core tests additionally cover Lowercase.
- Existing raster backing-store scroll checks pass. Final 100% run at 3440×1440,
  95 ragged rows, 1792 columns: about 2.97 ms/horizontal step and 3.01 ms/vertical
  step. Earlier 200% 0.2 run: about 5.28 ms and 9.44 ms. Software/offscreen timings
  are not Windows FPS estimates. No unchanged large-core benchmark was repeated.
- Screenshots inspected for yellow A, letter alignment, selection outline,
  ruler and row labels. Windows 0.2 physical input/native context-menu behavior
  still requires the user's check.

Scope: this is rigid-fragment sliding and downstream-block gap editing. The
free-residue compression regime, complete BioEdit menu parity, identity dots,
overview, MAFFT and AB1 editing remain pending. New operations are synchronous;
latency and history cost for huge whole-row/group/minimize changes are not yet
benchmarked. FASTA has no selection or trace metadata.

Optional GUI check tools: configure with
`-DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON -DKLEIOTIDE_BUILD_VIEWPORT_CHECK=ON`, build,
and run `kleiotide_fragment_check`, `kleiotide_gui_smoke`, and
`kleiotide_viewport_check` with `QT_QPA_PLATFORM=offscreen` on Linux.

# Validation checkpoint — 8 October 2026, version 0.2.1

User confirms 0.2 colors, gap tools, Sequence functions, font settings, individual
rename and undo on Windows. The supplied screenshots establish block pushing
through both residues and locked gaps, open-gap consumption in either direction,
and growth past the current end. The earlier obstruction rule is superseded.

- All 34 core tests pass in Release and ASan/UBSan builds. Leak detection remains
  disabled for this runner; no leak-check success is claimed.
- Hand-worked left/right cascades, locked-symbol preservation, rightward growth,
  group-left fit at column 1 and exact undo/redo are tested. 300 randomized
  fixtures agree character-for-character with an independent repeated single-step
  insertion/deletion model. An early fixture used non-nucleotide F; corrected to T.
- The Qt frontend compiles on Linux with Qt6.4.2 using a validation-only minimum
  adjustment. Shipped minimum remains Qt6.5, compatible with the existing kit.
- Real viewport events test cross-row Shift+click rectangles, shrinking from the
  original anchor, drag pushing/growth, ordinary arrows, and undo. Normal and
  200% scaling runs pass, alongside existing fragment-menu/gap/autoscroll tests.
- Main-window smoke passes for double-click Unicode rename, empty-area deselection,
  selected-title/FASTA clipboard export (duplicate/Unicode headers and row order),
  and custom font size/style, plus prior regex/rename/sequence/alignment actions.
- Raster scrolling regression passes: 3440×1440 logical viewport, 95 ragged rows,
  1792 columns, about 2.38 ms horizontal / 2.92 ms vertical per software scroll
  step. This is a local offscreen Release measurement, not Windows FPS.

Ctrl+arrow/context maximum actions retain gap-only jumps; ordinary arrows and
mouse dragging use pushing. Rightward pushing may grow alignment width; unselected
rows are untouched and need not be padded. Clipboard export and large block pushes
are synchronous and not yet benchmarked at maximal dataset size. Windows 0.2.1
native input, clipboard and custom-dialog behavior require user validation.

# Validation checkpoint — 8 October 2026, version 0.3

User validates 0.2.1 on Windows: double-click rename, empty-area deselection,
block selection/pushing, sequence copying, simplified fonts, smooth scrolling,
and ordinary/Ctrl+arrow sliding all work as expected.

- All 38 core tests pass in Release and ASan/UBSan builds. Added tests cover
  content revision/undo branching, case/U/T/ambiguity/ragged gap statistics,
  cooperative scan invalidation, empty inputs, compressed bins and 100 randomized
  fixtures against an independent column model. Leak detection remains disabled.
- Complete Qt GUI and optional check tools compile with g++13.3, CMake3.28.3 and
  Linux Qt6.4.2. The validation copy lowers only the Qt minimum; the delivered
  source requires Qt6.5 and targets the user's existing Qt6.5.3/MSVC2022 kit.
- Real Ctrl+C/Ctrl+V key events and menu actions verify single-fragment plain
  copying, unchanged clipboard for blocks, selected-name FASTA priority, raw
  Insert paste with mixed case/IUPAC/gaps/wrapped lines, caret advancement,
  atomic undo/redo, invalid-input rejection, no-caret and other-mode no-ops,
  and retained FASTA append behavior. Clipboard and prior GUI smoke checks pass
  at 100% and 200% display scaling.
- Overview checks pass at 100% and 200%: known gap/conservation regions,
  1/5/10-column ruler tick pixels, click/drag/end navigation, visible range,
  resize/rebin, rename cache reuse, rapid edits, rightward growth, undo,
  document replacement and clearing. Main-window integration and View-menu
  hide/show pass. An initially disabled dock toggle was corrected before the
  final check. Overview/ruler screenshots were visually inspected.
- Existing fragment mouse, Shift-selection, arrow/drag, context menu, gap and
  autoscroll checks pass at 100% and 200%.
- Raster backing-store scrolling checks pass at all four scaling factors:

| Scaling | Horizontal ms/step | Vertical ms/step |
|---|---:|---:|
| 100% | 2.35 | 2.75 |
| 125% | 12.13 | 15.14 |
| 150% | 16.68 | 6.98 |
| 200% | 5.05 | 9.30 |

The viewport fixture is 95 ragged rows × up to 1792 columns at 3440×1440 logical
pixels. These are single local offscreen Release runs, not Windows FPS guarantees.

The new overview was additionally measured on 5000 rows × 100000 columns
(500 million stored symbols). Its complete calculation, including the 150 ms
edit debounce and event processing, took 2687 ms. A 1 ms GUI heartbeat processed
1099 events while moving the horizontal scrollbar; its largest observed timer
gap was 10.61 ms. The scan processes 4096-symbol batches with a 2 ms work-slice target
and a 4 ms work timer, uses 20 bytes of counters per column (~1.91 MiB at 100000),
and does not copy the sequence dataset. Timer precision, repainting and allocation
can exceed the slice target; this is responsiveness evidence from one run, not
a hard latency guarantee. Fixture creation and FASTA disk parsing are outside
that measured overview interval. An unchanged core insertion benchmark was
not repeated.

Windows 0.3 compilation, physical input/clipboard, and overview behavior on the
user's real datasets still require the local check in TESTING_0.3.md. The user
has confirmed the earlier version's Windows scrolling; the current automated
results are Linux only. Whole-row transformations, large block pushes, clipboard
export and append integration remain synchronous. MAFFT, AB1 editing, identity
dots and free-residue compression are not implemented in this stage.

# Validation checkpoint — 8 October 2026, version 0.3.1

The user confirms 0.3 sequence/string copying and pasting, ruler ticks, overview
navigation, prompt updates and View-menu activation on Windows. The current
correction addresses font-6 clipping, dialog/name-pane width and overview meaning.

- All 39 core tests pass in Release and ASan/UBSan builds. Revised independent
  randomized statistics checks use residue density, exclude ambiguous calls
  from conservation, and average only informative columns. A new focused test
  verifies A/N conservation, all-N/all-gap distinctions and compressed bins with
  no-information columns. Leak detection remains disabled for this runner.
- Complete GUI and optional tools compile with Linux Qt6.4.2/g++13.3/CMake3.28.3
  using the validation-only Qt minimum adjustment. Shipped minimum remains 6.5.
- Layout checks pass at 100%, 125%, 150% and 200% scaling: real divider mouse
  press/drag/release, resize cursor, min/max widths, preserved row/fragment
  selection and left column, font-6 digit ink height against an unclipped
  independent rendering, compact residue rows, and cell/caret paste hits after
  both width and font changes. Ruler/overview screenshots were visually inspected.
- GUI smoke passes at 100% and 200%, including the wide rename dialog via existing
  menu/double-click rename paths, plus prior clipboard, transform, font and
  hide/show workflows. Fragment selection/slide/gap/autoscroll checks also pass.
- Overview tests verify actual residue bar height and pixel colors: conserved
  full-height blue, half-residue half-height blue, strongly variable bright
  orange, all-N full-height gray, all-gap gray baseline, intermediate 95%/80%/70%
  levels, and unchanged blue when nineteen N calls accompany one A. Tests pass
  at 100% and 200%, including navigation, updates, undo, resize and source changes.
- Actual raster backing-store scroll regressions pass at all four scales:

| Scaling | Horizontal ms/step | Vertical ms/step |
|---|---:|---:|
| 100% | 2.50 | 2.79 |
| 125% | 11.81 | 14.15 |
| 150% | 15.30 | 5.36 |
| 200% | 4.56 | 9.53 |

These are local offscreen Release runs on the same 95-row, 1792-column,
3440×1440 fixture. They are not Windows FPS guarantees. The overview scan
algorithm and memory representation did not change; the new statistics are
computed from its cached counts. The large scan benchmark was not repeated.

Conservation is the majority A/C/G/T/U-call fraction (U merged with T), rather
than the fraction of all nongaps used in 0.3. N, other IUPAC ambiguities and ? are
excluded from this calculation, but included in residue height. No-information
columns are excluded from the bin's conservation average. This is a called-base
conservation measure; it does not infer or fractionally allocate ambiguous calls.

Windows 0.3.1 native GUI/clipboard and the user's real data remain for the local
check in TESTING_0.3.1.md. No new dependencies are introduced.

# Validation checkpoint — 8 October 2026, version 0.4

The user confirms the draggable name divider and requests partial IUPAC votes,
selected-row dragging, sorting and mask support. The checkpoint below supersedes
0.3.1's exclusion of partial IUPAC calls from conservation.

- All 45 core tests pass in Release and ASan/UBSan builds. Tests cover all IUPAC
  vote mappings, mask-only and mixed overviews, mask import/edit/save/history,
  reverse-complement exclusion and nucleotide-only alignment eligibility.
  Permutation validation is atomic, selected group movement preserves relative
  order, all nine sort methods have independent expected results, and 150 random
  row permutations undo/redo exact IDs, row kinds and text. Reordering history
  is exercised together with deletion and symbol editing. Leak detection remains
  disabled for this runner.
- The full GUI and all optional validation tools compile in Release with Linux
  Qt6.4.2/g++13.3/CMake3.28.3 using the validation-only Qt minimum adjustment.
  The shipped Qt minimum remains 6.5; Windows binaries are not bundled.
- The stage-0.4 GUI check passes at 100% and 200% scaling. Actual mouse events
  move noncontiguous selected rows to both ends, preserve selection, cancel
  with Escape and scroll at viewport edges. Raster pixels confirm mask A is
  gray while nucleotide A is yolk yellow. Mixed mask/DNA blocks slide correctly;
  mask overwrite and raw paste accept annotation symbols. All nine real Sort
  menu actions produce the expected exported FASTA and undo/redo atomically.
  Unicode title case folding, stable ties, missing-column no-op and mask-skipping
  reverse complement are checked through the actual main window.
- Existing clipboard/rename/font workflows, block/gap editing, divider/ruler
  layout and interactive overview checks pass at 100% and 200%. Focused overview
  checks cover partial IUPAC conservation and actual color/height pixels with
  a mask row excluded from numerator, denominator and base votes.
- Actual raster backing-store scroll checks pass at four scales:

| Scaling | Horizontal ms/step | Vertical ms/step |
|---|---:|---:|
| 100% | 2.90 | 2.58 |
| 125% | 12.48 | 15.08 |
| 150% | 16.61 | 5.84 |
| 200% | 5.10 | 9.32 |

These are local offscreen Release runs of the 95-row, 1792-column, 3440×1440
fixture, not Windows FPS guarantees. The revised overview scans ordinary bases
directly and uses exact six-unit counts for fractional IUPAC votes. A 5000-row
× 100000-column scan (500 million symbols) took 2662 ms including debounce and
event processing. A 1 ms GUI heartbeat processed 1136 events while moving the
horizontal scrollbar; the largest observed timer gap was 10.17 ms. The cache
still uses 20 bytes per column and does not copy the sequence dataset. This is
one local responsiveness measurement, not a hard latency guarantee; fixture
creation and FASTA parsing are outside the measured interval.

Sorting reads immutable symbols in a background task; applying the permutation
uses row swaps and compact undo records rather than copying sequence strings.
Overview scanning is restarted if a reorder occurs mid-scan. Length sorting
uses cached nongap counts; frequency sorting scans only the selected columns.
Large sorting latency has not been benchmarked separately. Other whole-row
transformations, clipboard export and append integration remain synchronous.

Masks use printable ASCII cells and preserve their kind within the document.
FASTA lacks type metadata; nucleotide-only edited masks are inferred as
nucleotides on reopening. MAFFT integration is still pending and must use the
nucleotide-only eligibility API. Windows 0.4 compilation, physical drag/input
and clipboard behavior on the user's data remain for TESTING_0.4.md.

# Validation checkpoint — 8 October 2026, version 0.5

User confirms all 0.4 functionality. This stage implements the original plan's
external MAFFT/new-result-document contract for whole alignments and all-row
selected-column realignment. Target/template reintegration remains pending.

- All **55 core tests** pass in Release and ASan/UBSan builds. Ten added tests
  cover temporary IDs; duplicate Unicode titles; exact case and RNA restoration;
  empty/mask retention; malformed/missing/repeated/unequal output; fragment
  growth/shrink with exact prefix/suffix gap states; mask-width protection;
  permitted DNA/RNA orientation; cancellation/stale source rejection; options;
  independent result history; 100 randomized output order/gap layouts; and 80
  randomized fragment-context checks. Leak detection is disabled in this runner.
  A direct g++ build with -Wall -Wextra -Wpedantic -Werror also succeeds.
- The application and all optional tools build with Linux Qt6.4.2/g++13.3/
  CMake3.28.3, using the validation-only Qt minimum adjustment. The shipped
  minimum remains Qt6.5. MSVC UTF-8 source encoding is applied to all targets.
- Official **MAFFT 7.525** source was compiled and installed only for validation.
  Actual external-process runs pass for Auto, FFT-NS-1, FFT-NS-2, FFT-NS-i,
  L-INS-i, G-INS-i, E-INS-i and PartTree. Fixtures contain unequal lengths, mixed
  case, partial IUPAC, N, U, ? and masks. All returned nucleotide residues are
  validated, case restored, masks excluded and source documents unchanged.
  Real selected-column jobs and both fast/accurate strand adjustments also pass;
  reversed IDs are recorded. These small correctness fixtures do not benchmark
  MAFFT on the 5000 × 100000 editing target.
- Deterministic subprocess checks cover invalid version, nonzero exit, empty
  output, corrupt output, missing executable, preparation cancellation and
  active-job cancellation. An actual spawned descendant writes a heartbeat file;
  that file stops changing after process-tree cancellation. A GUI heartbeat
  continues while each job executes. Failed/cancelled jobs return no document.
- Real settings/menu checks pass at 100% and 200% scaling: whole and column jobs,
  new result windows, unchanged saved source, report access, Cancel button and
  File → New window while the source is read-only. The complete MAFFT process/UI
  workflow additionally passes ASan/UBSan. Settings screenshots were inspected.
- Windows .bat/.cmd invocation uses cmd.exe with delayed expansion off and
  launcher/input/output paths in quoted environment-variable references. Only
  generated validated flags/numbers enter its command string. Tests check this
  command construction and reject shell-bearing options. Windows cancellation
  combines job objects and taskkill /T /F; Linux uses a separate process group.
  **The Windows adapter has not been run on Windows here.** Batch/runtime/path
  and descendant-cancellation checks on the user's Qt6.5.3/MSVC2022 machine are
  the remaining validation in TESTING_0.5.md. No MAFFT binaries/runtime are bundled.
- Existing clipboard/rename/font, block/gap, overview, ruler/divider and stage-0.4
  dragging/sorting/mask GUI checks pass at 100% and 200%. Actual raster scrolling
  checks pass at all four scales on the 95-row, 1792-column, 3440×1440 fixture:

| Scaling | Horizontal ms/step | Vertical ms/step |
|---|---:|---:|
| 100% | 2.55 | 3.69 |
| 125% | 13.02 | 14.49 |
| 150% | 15.47 | 5.69 |
| 200% | 4.64 | 9.08 |

Timings are local offscreen runs, not Windows FPS guarantees. The overview
algorithm is unchanged from 0.4, so its large-scan benchmark was not repeated.

The source stays immutable during a job. Export is streamed with synthetic IDs;
stdout alignment output goes to a file; parsing/validation/result construction
run in background tasks. The output baseline retains source row IDs and creates
independent history without copying the source undo stack. The contiguous buffer
backend still normalizes/copies result strings transiently during construction;
large-job memory and latency have not been benchmarked. Masks retain stored text,
and a fragment's width is clamped to avoid discarding mask annotations. Existing
gaps inside a realigned scope are removed, and new gaps are open; outside-fragment
case/gap states remain exact. Native trace provenance and constrained locked-gap
alignment are not implemented in this milestone.

## 0.5 corrected source archive — 8 October 2026

The original ZIP was written with the packaging host's local wall-clock dates
(UTC+3). Its CMakeLists.txt ZIP timestamp was 23:03:04, later than the Windows
build log's 22:25:38 start. Interpreting those ZIP dates in UTC+2 reproduced
exactly 100 CMake regenerations and Ninja's "build.ninja still dirty" failure.
The diagnostic identified CMakeLists.txt as newer than generated build.ninja.
The same archive built normally when interpreted in the host's UTC+3 zone.

`tools/package_source.py` now writes every ZIP member with the fixed date
2000-01-01 and avoids build folders and local Qt Creator user settings. The
corrected 0.5 archive retains byte-identical CMake configuration, application
sources and core tests; only documentation and the packaging helper change.

The corrected archive was extracted with its historical dates preserved and
configured into fresh Ninja build directories. Release core and Qt GUI builds
completed, all 55 core tests passed, and both subsequent builds reported
"ninja: no work to do" without CMake regeneration. GUI validation used the same
local-only adjustment to installed Qt6.4.2 as the initial 0.5 checks; the shipped
requirement remains Qt6.5. Windows runtime validation remains pending.

## 0.6 selection/editing and statistics — 9 October 2026

The user confirms that 0.5 MAFFT features work as expected on Windows. This
release implements the eight requested updates, with IQ-TREE nucleotide counting
conventions chosen by the user. Dependencies and the fixed historical ZIP dates
are retained. This release still needs its Windows/Qt6.5.3/MSVC2022 runtime check
in TESTING_0.6.md; no Windows execution is claimed here.

- All **59 core tests** pass in Linux Release and ASan/UBSan builds. Added cases
  cover compatible/incompatible partial ambiguities, IQ-TREE pattern normalization
  and uniqueness, masks/empty/ragged rows, within-row scan cancellation and one
  exact block-deletion transaction with undo/redo. Leak detection remains disabled
  because of this runner's limitations.
- The application and all optional GUI tools compile with Qt6.4.2/g++13.3/
  CMake3.28.3. The validation copy alone lowers the Qt minimum to installed 6.4;
  the shipped CMake minimum remains Qt6.5.
- New stage06 GUI checks pass at 100% and 200% scaling, and under ASan/UBSan:
  mouse/Shift-click/ruler selection in all four modes; selection preservation and
  exact blue/yellow backgrounds; double-click mixed-gap and residue runs; Insert
  rejection of ambiguous selections; typed and pasted variable-length Overwrite;
  following-cell/end carets; invalid-paste atomicity; mixed mask/ragged block
  Delete/Backspace with one undo; mask replacement; Esc; removal of the duplicate
  menu; actual Ctrl+C/Ctrl+V/Ctrl+F; all parameter tooltips; asynchronous
  statistics, all nine displayed counts, one-line Copy and closing/cancellation.
  Statistics screenshots were inspected.
- The independent official **IQ-TREE 2.4.0 Linux binary** agrees with the nine-
  column ambiguity fixture (8 patterns, 1 informative, 3 singleton, 5 constant),
  plus **12 randomized 8-row, 65-column** nucleotide/partial-ambiguity/missing-
  state fixtures. Sequence/column/pattern and all three IQ-TREE-reported site
  counts matched. This comparison used -st DNA -m JC -n 0 -nt 1. IQ-TREE is a
  validation-only download and is not bundled or required by Kleiotide.
- Statistics on **5000 × 100000** synthetic calls completed in **11.14 s**.
  Peak process RSS, including the 500 MB source and its construction, was
  **496220 KiB**. The implementation uses row-major scanning and O(columns +
  sequences) representative/hash storage, checks hash collisions exactly, and
  keeps the document immutable while calculating in a worker. This fixture is
  highly repetitive (12 unique rows/patterns); it is not a worst-case timing
  guarantee for real data or Windows.
- Existing clipboard/rename/font GUI smoke, rigid-fragment selection/sliding,
  name-divider/font-6/ruler checks, stage04 dragging/sorting/masks and raster
  backing-store scroll checks pass at 100%/200%. The legacy stage04 Escape check
  and GUI smoke's old Overwrite-paste rejection were updated for the explicitly
  changed requirements. Overview checks also pass.
- MAFFT subprocess validation, failures/cancellation/process-tree checks and GUI
  whole/column/result/report checks pass again, including real MAFFT 7.525 runs
  of all eight presets, fragment jobs and both strand adjustments.

Statistics use the longest nucleotide row, excluding mask-only extensions; short
rows have virtual missing ends. Unique sequences compare aligned state strings,
not ungapped sequences, and ignore titles/case. U=T; all gap types and N/? share
IQ-TREE's missing state for patterns/uniqueness. Partial ambiguities retain their
possible-nucleotide sets. Constants have a nonempty intersection, including all-
missing sites; informative sites need two definite states observed twice each.
Singleton is variable minus informative. Gap-only/N-only are separately exposed
subsets of constants, with N-only allowing gaps and requiring one N/?. The dialog
reports its opening-time baseline; reopen it after edits to refresh the counts.

The source archive is made with tools/package_source.py and every member's date
is 2000-01-01. Source files, rather than build products or Qt Creator user
settings, are packaged.

## Version 0.7 checkpoint — 9 October 2026

The user confirms that every 0.6 feature works exactly as intended on Windows.
This release implements the next M6c MAFFT target/template stage and two requested
adjustments: the overview legend wording and Insert-after behavior.

- **63 core tests pass**, including 120 randomized template projection fixtures,
  plus the existing randomized editing, gap sliding, sorting and MAFFT checks.
  New tests cover duplicate titles, repeated motifs, original gap-only intervals,
  longer untouched/mask tails, ragged rows, leading/internal/trailing insertions,
  exact case/U restoration, independent derived baselines, overlap and eligibility,
  cancellation/stale revisions, and rejecting split/merged/truncated/unknown/
  duplicated/reversed/unequal-length output. ASan/UBSan core checks pass with
  leak detection disabled, as in earlier stages.
- Qt GUI builds and checks pass on Linux with Qt 6.4.2 using a validation-only
  copy of the sources whose CMake Qt minimum is changed to 6.4. The shipped
  minimum remains **Qt 6.5**, for the user's Qt 6.5.3/MSVC2022 kit. No Windows
  binary was built or executed here.
- New stage07 checks pass at **100% and 200% scaling** and under ASan/UBSan:
  Insert after the selected cell; repeated insertion before an untouched suffix;
  final-cell/empty-row append; mask insertion; selection/undo; unchanged Overwrite;
  target/template eligibility; checkbox selection and duplicate-title numbering;
  settings restrictions; separate result windows; preservation reports; output
  rejection, cancellation and quoted Windows target-path command construction.
  An older fragment engine gets the upgrade message before alignment starts.
  The template chooser screenshot was visually inspected.
- The official **MAFFT 7.526 Linux binary** passes full and fragment additions,
  single and multiple templates, multiple targets (including an N-only target),
  partial/gapped templates, unrelated nucleotide rows, long masks and empty rows.
  Full addition also passes FFT-NS-1, FFT-NS-2, FFT-NS-i, L-INS-i, G-INS-i and
  E-INS-i, in addition to Automatic. Fragment addition uses Automatic.
- Additional testing found that the official **7.525 source build** fails on
  multiple-target --addfragments jobs. A standalone minimal command reproduces
  the error without Kleiotide. A validation-only rebuild of pairlocalalign with
  sanitizers identifies a heap-buffer-overflow in io.c's
  WriteHat2_pointer_clustaloformat, reached from WriteHat2_part_pointer while
  exporting a partial matrix. Official 7.526 passes the same cases. Consequently,
  the adapter requires **MAFFT >=7.526 for fragment addition**. No MAFFT code is
  patched or shipped with Kleiotide. Full addition was also exercised with 7.525.
- Existing whole/column MAFFT checks pass again with 7.526, including all eight
  presets, both strand adjustments, malformed output, failed/version probes,
  cancellation/process descendants and GUI result/report lifecycle. The earlier
  7.525 whole/column checks also passed before the new template tests.
- Existing GUI smoke, all-mode selection/clipboard/overwrite/deletion/undo,
  statistics/shortcuts/help, rigid-block sliding, drag/sort/masks, overview,
  divider/font-6/ruler and raster scrolling checks pass at 100%/200%. Stage06
  checks also pass under ASan/UBSan. The legacy Insert checks now expect the
  expressly requested after-symbol behavior. Measured scrolling remained smooth
  in this Linux check; actual Windows interaction remains the user validation.
- Source packaging uses tools/package_source.py, retains fixed **2000-01-01** ZIP
  member timestamps, excludes builds/user settings, and verifies every archived
  file against the source. A fresh core-only archive build/tests and repeated
  no-work Ninja build verify that the timestamp fix is retained.

Preservation contract: original-axis columns are never deleted or merged.
Non-target stored characters (including each gap state and mask symbol) project
exactly through the old-to-new map; virtual ends become open gaps in the result.
Targets retain their exact ungapped symbols/case but acquire new gap positions.
Template residue ordinals anchor the map. Original template-all-gap intervals
are restored before new insertion columns immediately preceding the next anchor;
trailing insertions follow the entire old axis. Reports expose the boundary policy,
IDs and preservation counts. --keeplength/--mapout are not used. A failure does not
apply raw output or mutate the original document/history.

MAFFT validation references:
https://mafft.cbrc.jp/alignment/software/addsequences.html and
https://mafft.cbrc.jp/alignment/software/linux.html . The validation-only binary
was downloaded from the official mafft_7.526-1_amd64.deb link.

# Validation checkpoint — 10 October 2026, version 0.8

The user confirms 0.7 works correctly and authorizes the next implementation stage.

- C++20 core, g++ 13.3 / CMake 3.28.3: 63 existing core cases plus 12 trace groups
  pass CTest, including synthetic ABIF endian/inline/padded-directory fixtures,
  coherent call sets/missing quality, malformed shapes/offsets/channels/peaks,
  2,000 mutated/truncated inputs and 500 random edits with exact undo/redo.
- Both supplied real AB1 files import: 580 calls / 6998 samples, and 1622 calls /
  17242 samples. An independent Python struct decoder matches C++ hashes over
  every imported call, peak, quality and all four canonical signal channels.
  Neither research file is bundled in the source release.
- Core ASan/UBSan with -Werror passes, including both real files, randomized
  provenance/history and malformed-input cases. Leak detection is disabled for
  this runner; no leak-check success is claimed.
- Complete Qt frontend and optional checks compile on Linux / Qt 6.4.2. Only the
  separate validation copy lowers the Qt requirement; delivered source still
  requires Qt >=6.5 for the established Qt 6.5.3/MSVC2022 Windows kit.
- Offscreen stage 0.8 GUI checks pass: both AB1 imports, bidirectional residue/peak
  navigation, native pan/zoom/min-max decimation, original/current/quality labels,
  inserted/deleted/substituted anchors, reverse complement, row deletion/undo,
  View hide/show, Unicode file-path clipboard append/undo, separate FASTA export
  and rejection of ABIF overwrite. Screenshots were inspected for rendering.
- Official MAFFT 7.526 whole, selected-column and target/template jobs on a real
  trace retain the original evidence and valid residue-to-column provenance.
  The adapter's existing strand/template rules remain unchanged.
- The complete stage 0.8 Qt workflow also passes ASan/UBSan, including export
  protection and real MAFFT jobs; leak detection is disabled as above.
- Existing stage 0.6 selection/edit/statistics checks, stage 0.7 insertion and
  full/fragment template workflows (including real MAFFT presets), and the GUI
  clipboard/rename/font smoke check pass against 0.8.

This checkpoint is Linux/offscreen verification, not a Windows 0.8 execution
claim. TESTING_0.8.md describes the Windows interaction check. Native session
persistence and alignment-space trace warping/direct trace editing are not yet
implemented; FASTA exports sequence text only.

# Validation checkpoint — 12 October 2026, version 0.9

- User confirms 0.8 opening/addition/trace switching on Windows and supplies three
  screenshots of missing original and inserted labels. The rendering correction
  removes base-label overlap culling and draws every visible current symbol,
  including synthetic anchors, independently of selection. Dense-call tests cover
  25/100/400% zoom, caret movement and reverse complement; screenshots inspected.
- 63 existing core cases, 12 trace groups and 6 native-session groups pass CTest.
  The native groups include exact evidence/row/ID/type/strand/provenance round trips,
  fresh undo after reopening, shared trace deduplication, forced masks, bad IDs,
  truncated/future-version/damaged/oversized headers and 2,000 mutated inputs.
- Both supplied AB1 files pass additional native round trips after insertion,
  deletion and reverse complement, retaining every imported signal/peak/quality
  and the edited anchor map. No research AB1 files are bundled in the release.
- Core ASan/UBSan with -Werror passes all three suites. The full 0.9 GUI workflow
  also passes ASan/UBSan. Leak detection is disabled for this runner; no leak-check
  success is claimed.
- Qt frontend compiles on Linux / Qt 6.4.2 in a separate validation copy which
  lowers only the Qt minimum. Delivered source still requires Qt >=6.5 and has
  no added runtime dependency for session storage or external file requests.
- Offscreen GUI checks exercise the actual multi-file Open and native Save dialogs,
  reopen/edit/undo after source fixture deletion, selected mode/caret/name width/
  trace zoom/gain restoration, dense and persistent insertion labels, mixed
  FASTA/mask/AB1 imports and separate shell-request coalescing.
- Existing stage 0.6 editing/statistics and GUI copy/paste/font smoke checks pass.
  Stage 0.8 trace navigation/undo/reverse/show-hide/export-protection checks pass,
  including real MAFFT 7.526 whole/column/template jobs.
- External launches use a per-user lock and atomic request files, not sockets;
  their file lists are collected after 500 ms of quiet into a new document.
  The optional Windows Open With registration script is packaged but has not been
  executed here. Windows Explorer behavior and the 0.9 Windows build require the
  user check in TESTING_0.9.md; no Windows execution success is claimed.

Native sessions retain existing edits and view state, with a fresh undo baseline
on reopen. They embed analyzed trace data, not a byte-for-byte source AB1 archive.
Direct trace editing and alignment-space stretching/compression are the next M7
substage. The session layout/reader limits are documented in SESSION_FORMAT.md.

# Validation checkpoint — 12 October 2026, version 0.10

- User confirms all 0.9 functions work correctly on Windows. The supplied screenshot
  of scrolled row-label overlap is corrected with final-layer opaque backgrounds.
- 85 synthetic core/trace/session/coordinate groups pass CTest (63/12/6/4), including
  peak-to-column and inverse consistency after gap insertion, insertions/deletions,
  strand reversal, coincident peaks, all-deleted/synthetic rows, 200 randomized
  edits and an exact original-signal session round trip.
- The full Qt frontend compiles on Linux / Qt 6.4.2 in a separate validation copy;
  shipped source retains Qt >=6.5, with no new dependency.
- Offscreen stage 0.10 GUI checks pass: View/Insert/Overwrite gating, direct typing,
  Current drag/Shift-click, actual Paste/Copy actions, deletion, undo/redo, gap
  insertion, arrow sliding, reverse complement, uniform alignment-column spacing,
  opaque-label raster invariance while scrolling and native-session restoration.
  The original trace evidence pointer/arrays remain unchanged by edits/display.
- Existing stage 0.6 editing/statistics, stage 0.9 dense labels/multi-file/session/
  external-request checks and stage 0.8 trace workflows pass, including actual
  MAFFT 7.526 whole/column/template jobs on the supplied AB1 files.
- Core ASan/UBSan with -Werror passes all four suites. Stage 0.10 GUI ASan/UBSan
  passes the complete editing/session workflow. Leak detection is disabled for
  this runner; no leak-check success is claimed.

This is Linux/offscreen validation; Windows 0.10 interaction remains for the user's
checklist. Alignment spacing automatically warps display intervals from alignment
edits, with native Sample spacing retained. Independent manual signal warping and
free-residue compression are not implemented. Original evidence is never resampled
in storage. Version-1 native sessions remain compatible.

# Validation checkpoint — 13 October 2026, version 0.11

- User confirms 0.10 works correctly on Windows and requests directional Backspace
  and independence of sequence-name/residue selection. Both are implemented in
  the shared alignment/trace controller. Planned B23 identity dots are added as
  cached rendering against the current first row, with optional session metadata.
- Full Qt frontend and the new stage 0.11 tool compile on Linux / Qt 6.4.2 in a
  separate validation copy. Delivered source still requires Qt >=6.5.
- All 85 core/trace/session/coordinate groups pass CTest (63/12/6/4).
- New offscreen GUI checks pass: Backspace/Delete in Insert and Overwrite on
  single cells/stretches/gaps, first/last/whole rows and end carets; atomic mixed
  mask/DNA block deletion; exact text/provenance/selection restoration on undo.
  Named rows remain selected without determining which residue cells are edited.
- Click/drag/Shift-click, name clicks/commands/blank-area deselection, ruler
  columns and trace navigation preserve independent selections in every mode.
  Existing FASTA-versus-substring clipboard priority and Esc behavior pass.
- Actual raster tests verify neutral dots for case-insensitive exact matches,
  including matching ambiguity codes; top/gaps/masks/mismatches remain unchanged.
  Reference edits, reorder, mask-first references, undo, toggling, ragged rows,
  real-text copying and .kleio setting/independent-selection round trips pass.
- Existing stage 0.10 direct-trace workflow (with new Backspace/name-selection
  checks), stage 0.6 editing/statistics, fragment sliding and GUI clipboard/font
  workflows pass. Original signal/provenance retain their existing protection.
- At 3440×1440, the 200-frame viewport check passes incremental/full-repaint pixel
  comparisons, font changes, editing and undo/redo, both with letters and dots.
  This run measured about 4.74 ms/horizontal and 4.67 ms/vertical frame for letters,
  and 4.74 / 6.41 ms for dots. These are Linux/offscreen synthetic measurements,
  not a Windows performance guarantee.
- Stage 0.11 GUI passes AddressSanitizer and UndefinedBehaviorSanitizer. Leak
  detection is disabled for this runner; no leak-check success is claimed.

Windows build/interaction remains for TESTING_0.11.md on the user's existing kit.
No new runtime dependency or binary session version is introduced.

# Validation checkpoint — 13 October 2026, version 0.12

- User confirms 0.11 works and requests toolbar buttons for identity dots, trace
  and overview. Three icon-only actions are shared with the View menu, retaining
  checked state, tooltips, accessible names and session/dock-close synchronization.
  Native multi-DPI icons add no runtime dependency. The prototype About text now
  reports 0.12 consistently with the application and CMake version.
- The next M5 selection substage implements title-prefix/substring/exact lists,
  duplicate full/prefix names, duplicate aligned/ungapped sequences, start-column
  selection, nucleotide motifs including optional reverse complement/IUPAC
  compatibility, and exact-count random selection without replacement. Work runs
  outside the UI thread; no data/provenance/history mutation occurs.
- All 94 test groups pass CTest: nine new selection groups plus the existing
  85 core/trace/session/coordinate groups (63/12/6/4). New groups exercise exact
  matching and gap-to-column coordinates, earliest reverse/forward hits, ambiguity
  and gap rules, 64/128-bit boundaries, the 4096-symbol limit, duplicate kind/case/
  gap rules, first-nongap ranges and seeded random samples. 2000 randomized motif
  comparisons agree with an independent brute-force matcher.
- Full Qt frontend compiles on Linux / Qt 6.4.2 in a separate validation copy
  lowering only the minimum. Delivered source retains Qt >=6.5. Final application
  build has no compiler warnings.
- Actual offscreen GUI dialogs/buttons pass Unicode literal matching, all added
  selectors, additional-copy options, cancellation and zero-match behavior,
  preserved residue blocks, unchanged original trace arrays/data/undo, toolbar/
  menu state, panel closure and session restoration. An empty document disables
  the trace toggle; linked sessions enable it. Toolbar rendering was inspected.
- Stage 0.11 deletion/independence/dot workflows, stage 0.10 trace editing/spacing/
  persistence, stage 0.6 editing/statistics/regex/MAFFT-help and clipboard/font smoke
  workflows pass. The 3440×1440 identity-dot viewport check passes incremental
  repaint, font, edit and undo/redo comparisons (about 3.00 ms/horizontal and
  3.31 ms/vertical frame on this Linux/offscreen run).
- New selection tests and stage 0.12 GUI workflow pass AddressSanitizer and
  UndefinedBehaviorSanitizer. Leak detection is disabled for the runner; no leak
  check success is claimed.

Windows interaction remains for TESTING_0.12.md using the existing Qt/MSVC kit.
This release adds row-selection tools; batch rename parity and a complete motif
hit browser remain later M5 work. Binary session version and trace evidence are
unchanged. Existing view metadata also restores the new toolbar buttons.

# Validation checkpoint — 14 October 2026, version 0.13

- User confirms 0.12 except for the first-nongap-in-columns selector and explicitly
  requests removal rather than a fix. Its UI action, unused predicate and test are
  removed. The identity toolbar icon now draws tiny A/C/G/T in the alignment
  nucleotide colors above neutral dots, rasterized at three DPI scales.
- Applicable M5 batch-name transformations share a wide current/new-name preview:
  spaces/underscores, literal add/replace, filename text, grapheme-safe reversal,
  numeric suffix stripping/uniqueness, global row numbering, aligned/true lengths
  and taxonomy/text append tables. Existing exact replacement tables also preview
  unmatched and multiply matched entries before an atomic name-only Apply.
- All 101 groups pass six CTest suites: 63 core, 12 trace, 6 session, 4 coordinate,
  8 retained selection and 8 new rename groups. New tests verify all rule types,
  Unicode emoji/combining clusters, occupied suffixes/outside-scope names, literal
  noncascading tables, scope-aware diagnostics, invalid/conflicting data, no-ops,
  stale plans, atomic undo and exact native name/signal persistence.
- Full frontend compiles on Linux / Qt 6.4.2 in a separate validation copy lowering
  only the minimum. Delivered source retains Qt >=6.5. Final application builds
  have no compiler warnings and introduce no new dependency.
- Offscreen stage 0.13 checks pass selector absence, icon colors/dots, actual
  selected/all preview dialogs, Cancel/error behavior, option invalidation,
  duplicate/missing table reports, Unicode/unique/length names, atomic undo/redo,
  unchanged symbols/kinds/original evidence/provenance, native title/selection/view
  restoration and editable filename prefilling. Preview layout and toolbar were
  visually inspected; multiline table diagnostics are fully visible.
- Updated table-preview clipboard/font smoke checks pass alongside stage 0.12
  selection/toolbar synchronization, stage 0.11 deletion/dot/selection independence
  and stage 0.10 direct trace editing/spacing/persistence. The 3440×1440 identity-dot
  viewport check passes incremental/full pixel comparisons, fonts, edit and undo/
  redo (about 2.93 ms/horizontal and 3.08 ms/vertical frame on this run).
- New rename groups and stage 0.13 GUI pass AddressSanitizer and
  UndefinedBehaviorSanitizer. Leak detection is disabled for this runner; no leak-check success is
  claimed.

This is Linux/offscreen verification; Windows checks remain in TESTING_0.13.md for
the existing Qt/MSVC kit. Native format version and immutable trace evidence are
unchanged. Preview and name-application work currently execute on the UI thread;
very large batches have not been latency-benchmarked. Full motif-hit navigation,
title concatenation/deduplication and independent free-residue semantics remain
separate refinements.

# Validation checkpoint — 14 October 2026, version 0.13.1

- User confirms 0.13, requests removing Alphabetically sort and adding Sanitize
  names. The unused sequence-symbol sorting action/enum branch is removed; all
  remaining sorting methods keep their behavior. The next palette/monochrome/
  colored-background display stage is recorded without requiring further inputs.
- Sanitization defaults to all rows, replaces unsupported runs, uses only ASCII
  letters/digits/underscores and supplies sequence for otherwise empty normalized
  names. Case-insensitive collision checks allocate unused numeric suffixes.
  Original safe titles/output bases are reserved first, so safe A_B and A_B_2 are
  preserved even if an earlier unsafe name normalizes to the same base.
- All 104 groups pass six CTest suites: 63 core, 12 trace, 6 session, 4 coordinate,
  8 selection and 11 rename. Three new groups cover unsafe/case/non-ASCII/fallback
  collisions, reserved numeric names, selected scope/outside names, atomic undo,
  and ASCII-only/unique/idempotent outputs for 2000 varied input names.
- Full frontend compiles without warnings on Linux / Qt 6.4.2 in the separate
  validation copy; delivered source retains Qt >=6.5. Actual-dialog tests pass
  menu presence/removal, default all-row scope despite selected names, read-only
  Preview/Cancel, Apply, case-insensitive uniqueness, original Unicode restoration
  on one Undo, and selection/original signal retention. Existing batch/native
  persistence and clipboard/font/table-preview checks also pass.
- The stage 0.4 sorting/mask/row-drag workflow passes all remaining menu sort
  actions and their undo/redo, mixed mask editing and Unicode title order. Its
  missing-columns check now clears residues explicitly with Esc, following the
  already-agreed independent name/residue selection behavior from 0.11.
- Rename rules and the updated batch GUI workflow pass AddressSanitizer and
  UndefinedBehaviorSanitizer. Leak detection is disabled for this runner; no
  leak-check success is claimed.

Windows interaction remains for TESTING_0.13.1.md. The requested naming rule does
not perform transliteration or truncate titles. Display palettes and new display
modes are queued as the next implementation stage and are not included here.

# Validation checkpoint — 15 October 2026, version 0.14

- User confirms 0.13.1 and authorizes the display stage. Four residue palettes
  and three display styles are implemented with synchronized menu/toolbar
  controls, matching trace channels/legend and optional native view preferences.
  Classic letter colors remain exact; overview conservation stays independent.
- All 108 groups pass seven CTest suites: 63 core, 12 trace, 6 session,
  4 coordinate, 8 selection, 11 rename and 4 residue-style groups. New tests
  cover exact Classic colors, alternative ink contrast against white, maximum
  black/white contrast for cells, neutral masks/monochrome and stable preference
  identifiers with unknown-value fallback.
- The complete frontend compiles on Linux with Qt 6.4.2 in a separate validation
  copy. Delivered source retains Qt >=6.5 and C++20; no new dependency is added.
  Actual GUI tests cover the full 4×3 palette/style raster matrix, blue/yellow
  selection cues, masks, identity-dot backgrounds, real-symbol copying, editing
  and undo, toolbar/menu/trace synchronization, native Save/reopen and old/unknown
  preference defaults. The palette display was also visually inspected.
- Existing stage 0.13, 0.12, 0.11, 0.10, 0.6 and clipboard/font/table-preview
  workflow checks pass. The new GUI workflow and residue-style groups pass
  AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection is disabled
  for this runner; no leak-check success is claimed.
- Raster backing-store comparisons check incremental versus full redraws,
  horizontal/vertical jumps, ragged ends, edits, fresh-view comparison, undo/redo,
  selection and fonts. Monochrome and background styles pass, including a wide
  selected block with identity dots. A fixed left selection border needs its
  shifted pixels repainted; edge strips are handled separately to avoid visiting
  all intermediate tiles. These are Linux software-renderer checks, not Windows
  frame-rate guarantees.

Windows interaction and normal display scaling remain for TESTING_0.14.md using
the existing Qt/MSVC kit. DISPLAY_COLORS.md records source palettes and ink
adaptations; nominal contrast checks do not establish whole-app accessibility
conformance. Display changes do not modify alignment data or immutable evidence.

# Validation checkpoint — 15 October 2026, version 0.14.1

- User confirms 0.14. Kleio replaces the visible Classic label, retaining its
  `classic` session ID. Chromas-style adds exact green A #008040, blue C #0000FF,
  black G #000000 and red T/U #FF0000. These base inks bypass darkening so the
  requested hex values apply to letters, trace channels, swatches and cell fills.
- All 109 groups pass seven CTest suites. The added style group covers the Kleio
  label/legacy ID and exact Chromas colors, including lowercase/U, cell fills and
  the new session ID. Existing style/mask/contrast/metadata tests include the fifth
  palette where applicable; contrast-adapted ink tests remain for the three
  adapted palettes rather than imposing a new color on the requested red.
- Complete frontend and the updated GUI workflow compile on Linux/Qt 6.4.2 in
  the separate validation copy; shipped Qt minimum remains 6.5. GUI checks pass
  all 15 palette/style combinations, View/toolbar labels, the exact overview
  explanation, trace legend/colors, masks, selection/identity-dot rendering,
  editing/undo/copy and Chromas session Save/reopen with synchronized controls.
- The rendered Chromas grid, trace legend/channels and complete overview header
  were visually inspected at 1280×850. The calculation and scrolling algorithms
  are unchanged from the validated 0.14 baseline; Windows checks are listed in
  TESTING_0.14.1.md. No new Windows installer is included in this source release.
- ROADMAP_1.0.md reconciles the original M0–M8 milestones with current work,
  identifies the remaining specifications, and proposes the reliability,
  distribution and release-candidate sequence. NEXT_STEPS.md now points to it;
  historical checkpoints remain available without presenting completed trace
  persistence/editing as pending.

# Validation checkpoint — 15 October 2026, version 0.15

- Implements authorized stage 1 of the expanded 1.0 plan. User-facing FASTA Save,
  export and clipboard share an unwrapped record writer. Wrapped/whitespace/CRLF
  input, empty rows, masks, case and both gap types retain existing semantics.
- All 115 model groups pass seven CTest suites: 64 core, 13 selection, 12 trace,
  6 session, 4 coordinate, 11 rename and 5 style. New coverage includes unwrapped
  long-record round trips, failed output streams, overlaps, both navigation
  directions, wrap/no-wrap, aligned gap spans, reverse/palindromic/both-strand
  matches, explicit empty scopes, stale cursors and cancellation inside long rows.
  350 random all-hit forward/backward traversals agree with independent brute
  force, alongside the existing 2000 randomized first-hit checks.
- The matcher uses streaming KMP for literal queries and multiword Shift-And for
  compatible IUPAC queries. It retains motif-sized coordinate/matcher buffers and
  one result; it does not normalize/copy each entire row or build a complete hit
  list. Forward resumes after the current start; backward scans the relevant row
  prefix. Cancellation is checked per row and at 4096-column intervals.
- Complete frontend compiles without compiler warnings on Linux/Qt 6.4.2 in the
  separate validation copy; delivered source retains Qt >=6.5. Actual 0.15 GUI
  checks pass file Save/clipboard bytes, shortcuts and Ctrl+F preservation, all
  navigator controls, empty selected scope, far-row/column navigation, cancel,
  replaced-document result rejection, explicit Select match, edit/undo
  invalidation and unchanged evidence/history during search.
- The GUI check verifies visible purple search-outline pixels in every one of
  the 15 palette/display combinations with identity dots. The 1280×850 navigator
  layout and hit outline were visually inspected. Existing stage 0.14 display/
  native persistence and 0.12 selection/toolbar workflows also pass.
- Raster backing-store incremental/full comparisons pass at 100% and 150% with
  background cells, identity dots, a wide selected block and an independent wide
  search hit, including horizontal/vertical movement, jumps, edits, fresh-view
  comparison, undo/redo and fonts. Single synthetic runs at 3440×1440 measured
  about 9.09/7.00 ms per horizontal/vertical step at 100%, and 92.41/10.58 ms at
  150%. These concurrent Linux software-renderer measurements are not Windows
  frame-rate guarantees; fractional-scaling paint cost remains worth measuring
  on the user's machine.
- Core/selection models and the final 0.15 GUI workflow pass AddressSanitizer and
  UndefinedBehaviorSanitizer. Leak detection is disabled for this runner; no
  leak-check success is claimed.

Search is ephemeral and independent of editing/name selections. It introduces no
native format extension or stored signal changes. Windows acceptance is listed
in TESTING_0.15.md. The next roadmap milestone is molecule types/native-session
foundations; quality/group/protein features remain planned.

# Validation checkpoint — 15 October 2026, version 0.15.1

- User confirms 0.15 and requests regex syntax, automatic hit selection, all-hit
  outlines/shading and total/strand counts. Find now indexes the complete scope;
  cached navigation selects the first/next/previous interval while retaining name
  selections. Completed overlays persist through cell/name changes and failed or
  cancelled replacement searches. Panel close releases them. Document revisions
  invalidate coordinates with an explicit refresh message.
- All 122 model groups pass eight CTest suites: the previous 115 plus seven new
  QtCore motif-search groups. These verify literal/regex totals and orientations,
  exact gap spans, palindromic coalescing, classes/alternation/repeats/anchors/
  backreferences, reverse variable-width hits at the same aligned start, empty
  selected scopes, zero-width handling, cancellation and engine-budget failure.
  Random interval/index round trips and visible shading/boundary masks agree with
  independent brute-force results across ordinal checkpoints and rows.
- The immutable index encodes arithmetic runs with varints and ordinal/prefix-
  maximum-end checkpoints. A million regularly spaced hits encode to under 256
  bytes. Visible coverage resolves dense runs without enumerating all hits; row
  boundary lines are batched. Selected blue/yellow cell fills are not tinted.
- Regex runs against the actual forward/reverse-complemented subjects, then maps
  coordinates back and coalesces equal intervals. No attempt is made to reverse
  regex syntax. ASCII subjects avoid repeated UTF validation for each overlap.
  Individual PCRE2 calls have explicit match/depth/heap limits; runtime engine
  failure is checked and never counted as a clean no-match result. Index and
  regex working limits are documented in TESTING_0.15.1.md; jobs publish atomically.
- Full frontend builds on Linux/Qt 6.4.2 in the separate validation copy; shipped
  Qt minimum remains 6.5. Actual GUI checks pass regex controls/errors, count text
  and both-strand explanation, automatic selection, persistent results, selected
  scope, cancellation, panel-close cleanup and read-only search history. Raster
  checks find other-hit outline pixels in all 15 palette/display combinations and
  verify actual light shading. The search layout was visually inspected.
- The revised 0.15 GUI regression passes unwrapped actual file/clipboard output,
  scopes, strands, wrapping, far navigation, stale-result rejection, editing/undo
  invalidation and original trace evidence. New model/GUI workflows pass ASan and
  UBSan. Leak detection is disabled for this runner; no leak-check success claimed.
- Raster incremental/full comparisons pass at 100% and 150% with dense all-hit
  overlays, identity dots and background colors (plus selection at 150%). Local
  3440×1440 software-renderer measurements were about 6.06/3.17 ms per horizontal/
  vertical step at 100% and 83.58/8.71 ms at 150%. Fractional-scaling raster cost
  remains environment-dependent, as in previous releases; Windows measurements
  remain part of acceptance rather than inferred from these numbers.

Synthetic optimized Linux scan: 500 random DNA rows × 100000 columns (50 million
symbols), with reverse complements enabled. One run, concurrent validation load:

| Query | Unique intervals | Complete scan | Encoded index | 10000 ordinal lookups |
| --- | ---: | ---: | ---: | ---: |
| Literal AAG | 1564479 | 723 ms | 8494280 bytes | 7 ms |
| Literal A | 25008450 | 1363 ms | 125848053 bytes | 7 ms |
| Regex AA[GT] | 3126068 | 3189 ms | 17162937 bytes | — |

Peak process RSS through the literal runs was about 179408 KiB, including the
50-million-symbol document; this is not an index-only allocation figure or a
universal speed guarantee. Full counts necessarily require the initial scan;
Next/Previous use the completed index. Native format and real sequence symbols
remain unchanged. Windows checklist: TESTING_0.15.1.md.


# Validation checkpoint — 15 October 2026, version 0.15.2

- All eight CTest suites pass: 125 model groups, including ten motif-search
  groups. New checks cover mixed/lowercase N, open/locked gaps, other ambiguity
  codes, both strands, literal/regex behavior, dense overlapping N runs and
  cancellation. Random interval filtering agrees with brute-force content checks
  across 64-column rank boundaries. Search leaves document/history unchanged.
- The optional N-only filter uses per-row bitmaps and rank totals, with constant-
  time interval checks and a 128 MiB working budget. Exclusion occurs before hit
  indexing, regex result buffering and strand totals; default-off searches avoid
  building this extra state. Gap-only and mixed non-N intervals remain searchable.
- Full Qt frontend builds in the Linux/Qt 6.4.2 validation copy; shipped minimum
  stays Qt 6.5. GUI checks pass the new checkbox/default/tooltip, removal of Select
  match, retained automatic navigation/selection, filtering/counts in both syntax
  modes, previous-result retention and close cleanup. The bright fill is checked
  in every palette and the layout/rendered screenshot was visually inspected.
  Background-mode hit glyphs/fills share cached row tiles; replacing results
  and closing the panel are checked for stale cached shading.
- Model and actual GUI workflows pass AddressSanitizer plus UndefinedBehavior-
  Sanitizer; runner leak detection is disabled. The prior broad 0.15 GUI regression
  also passes FASTA output, scopes, reverse complements, wrapping, stale results,
  edit/undo invalidation and immutable original trace evidence.
- The new white halo is clipped inside match coverage. Raster comparisons found
  and corrected an initial one-pixel outer-halo scroll artifact before packaging.
  Dense bright matches with identity dots and Chromas-style backgrounds pass
  incremental-versus-full repaint checks at 100% and 150% scaling, including
  both scroll directions, large jumps, row ends, edit and undo. Local software-
  renderer steps at 3440×1440 were about 11.2/6.5 ms horizontal/vertical at 100%
  and 100.6/16.6 ms at 150%, under concurrent validation load. Caching removed
  a duplicate glyph/fill pass (an earlier 150% run took about 187 ms horizontally).
  Fractional offscreen raster cost remains substantial and environment-specific;
  these figures are not Windows performance guarantees.

Windows acceptance is in TESTING_0.15.2.md. No native session format change,
sequence edits or new runtime dependency is introduced by this patch.


# Validation checkpoint — 16 October 2026, version 0.16

- All 135 model groups in nine CTest suites pass: 64 core, 13 selection,
  12 trace, 9 session, 4 coordinate, 11 rename, 5 residue-style, 10 motif and
  7 type groups. Existing nucleotide/mask behavior remains covered by the full
  suite, including MAFFT merge/preservation rules and current editing operations.
- New type tests cover strict declared alphabets; legacy nucleotide/mask inference;
  explicit protein imports of ACGT, empty and gap-only rows; unknown/stop/case/gap
  preservation; forced masks; failed batch ID/history atomicity; undoable retyping;
  trace retyping rejection; protein edits and mixed block sliding; nucleotide-only
  calculations/search/complements; and MAFFT rejection/non-target projection.
- The independently encoded version-1 fixture migrates masks that look like DNA
  or proteins without reclassification. It preserves stable IDs, embedded signal,
  qualities and inserted-call anchors without an available source file. Version 2
  retains exact protein types and canonical representation. Checksummed malformed
  kind/alphabet/encoding fields reject semantically; earlier truncation/checksum,
  length-limit and 2000-mutation checks pass with the new format.
- The full frontend builds on Linux/Qt 6.4.2 in the validation copy; shipped Qt
  minimum remains 6.5. Final frontend builds report no compiler warnings. Actual
  stage016 GUI checks pass import preferences/file/clipboard/new-sequence paths,
  neutral protein backgrounds across styles, strict editing, atomic retyping and
  undo/redo, mixed MAFFT input rejection before setup, native-save preference and
  round trip, explicit FASTA export, stored-type precedence, and AB1 protection.
  The framed N/P/M badges and layout were visually inspected.
- Seven new type groups, all nine session groups (including migration/mutation
  checks) and the actual GUI stage016 workflow pass ASan plus UBSan. Leak detection
  is disabled in this runner; no leak-check success is claimed.
- Existing motif/regex/filter/highlight GUI regression passes across palettes,
  including cached-hit replacement/close cleanup. Existing chromatogram/multi-file
  import/native-view/session/shell-request regression passes. Dense overlay raster
  comparisons pass incremental versus full repaints at 100% and 150% scaling,
  including scrolling, jumps, edit and undo. Local software-renderer steps were
  about 6.3/3.8 ms horizontal/vertical at 100% and 100.1/8.2 ms at 150%; these are
  environment-specific offscreen figures, not Windows performance guarantees.
- The actual MAFFT integration regression passes simulated errors/cancellation/
  descendant termination, batch quoting, GUI settings/separate result windows,
  and real nucleotide presets/strand adjustment with MAFFT 7.526. A restored
  validation dvtditr executable was initially truncated; re-extraction from the
  cached package restored a valid binary and all real checks then passed.

Scope: explicit types and canonical storage are delivered. Clustal-style protein
colors, translation/codon representations, groups and protein MAFFT are future
roadmap milestones. Native format 2 reads version 1; older applications that only
read format 1 intentionally reject new saves. Windows checklist: TESTING_0.16.md.


# Validation checkpoint — 16 October 2026, version 0.17

- All 142 model groups in ten CTest suites pass: 64 core, 13 selection, 12 trace,
  9 session, 4 coordinate, 11 rename, 5 style, 10 motif, 7 type and 7 group groups.
  New checks cover catalog/member transactions, stable IDs, color/order/name edits,
  delete/rename/reorder/undo, stable group sorting, invalid/retired IDs and dangling
  references, exact-title TSV import with headers/Unicode/duplicates/conflicts,
  stale previews, native backward reading and clipboard group collision remapping.
- Native format 3 preserves mandatory group catalog/order/colors, memberships and
  next-group ID. Independent format-1/2 fixtures migrate without guessing types or
  group membership. Semantic corruption tests recompute checksums and reject
  unsupported kinds/encodings, missing group references and invalid next-group IDs.
  The existing 2000-mutation, truncation and resource-limit checks pass.
- Group-only metadata edits keep the biological content revision unchanged.
  MAFFT whole and target/template derived documents retain catalogs/memberships;
  source groups, masks, trace links, row IDs and underlying symbols are preserved.
- Actual GUI stage017 passes group creation/assignment, staged rename/color/member/
  order edits, Cancel, context actions on unselected names, group selection with
  independent residue selection, selected-name swatch visibility, stable sorting,
  TSV preview/duplicate/conflict handling and one-step undo/redo.
- Actual sequence Cut/Paste passes between windows, after selected destination
  names, with normal FASTA plus native clipboard data, preserved protein/mask
  types, group name/color collision resolution, embedded trace samples, original
  evidence identity on Cut undo, and one-step pasted-row/group undo.
- Actual residue Cut/Paste passes single substrings and multi-row blocks, Insert
  deletion boundaries, Overwrite substitutions, mixed target validation without
  partial edits, and inactive residue Cut outside Insert/Overwrite. Standard
  Ctrl+X/Ctrl+V action bindings are verified. A rectangular destination originally
  needed an explicit anchor independent of the ordinary single-row caret; the GUI
  test caught this and the block-paste path was corrected before release.
- Ctrl+wheel tests verify six columns per normal notch and doubled pixel steps,
  with three-row ordinary vertical steps and native horizontal wheel unchanged.
- New group/session model groups and the full new GUI workflow pass ASan+UBSan.
  Leak detection is disabled in this runner; no leak-check success is claimed.
  An initial GUI test helper expected a filename dialog for Save on an already
  native path; it was corrected to Save As and made lifetime-safe. The issue was
  in the checker, not the application.
- The full frontend builds without warnings in the Linux/Qt 6.4.2 validation copy;
  shipped Qt minimum remains 6.5. Group-manager/colored-name screenshots were
  visually inspected. Existing 0.16 type/edit/import/native-save and 0.15.2 motif/
  regex/filter/cached-highlight GUI regressions pass.
- Grouped-name raster checks pass incremental/full repaint comparisons at 100%
  and 150%, with dense search overlays, background colors, identity dots and a
  selection at 150%. Local software-renderer horizontal/vertical steps were about
  8.9/5.4 ms at 100% and 197.7/17.9 ms at 150% under validation load. Fractional
  offscreen raster cost is substantial/environment-specific; no Windows speed
  guarantee is inferred from these figures.
- Existing MAFFT integration passes mocked failures/cancellation/tree cleanup,
  batch quoting, real MAFFT 7.526 nucleotide presets/strand adjustment and GUI
  settings/result-window workflows. The validation MAFFT runtime was freshly
  extracted; an empty restored check executable was relinked before the final run.

Windows acceptance: TESTING_0.17.md. Groups use one membership per stable row ID;
Ungrouped is 0. New native saves require 0.17+, while formats 1 and 2 still read.
The next milestone is trace-quality editing; protein palettes/translation and
protein MAFFT remain later milestones.

# Validation checkpoint — 16 October 2026, version 0.17.1

Refinement release after the user's successful 0.17 review. Qt >=6.5 remains the
shipped minimum; validation copies alone use installed Linux Qt 6.4.2. Windows
Qt 6.8.1/MSVC acceptance instructions: TESTING_0.17.1.md. No Windows installer or
native Windows build was produced in this environment. Session format remains 3.

- Release application builds. Ten CTest suites pass after relinking against the
  updated core. The group suite now has nine cases, including atomic replacement
  of previous groups by duplicate clusters, exact gap/type matching, singleton
  clearing, undo/redo, and target-only reversal with immutable trace evidence.
- Updated group/clipboard GUI workflow passes under release and ASan+UBSan
  (leak detection disabled): type/trace selectors preserve residue selection;
  the ten-pixel group swatch is visible; the old sorting action is absent;
  identical grouping replaces memberships with one-step undo. Sequence clipboard
  priority works even with an independent residue block. One-row Insert/Overwrite
  Cut/Paste works; block Cut leaves clipboard/history unchanged; raw block Paste
  and legacy multi-row residue payloads are rejected with no mutation.
- MAFFT process/settings/result-window regression passes with a deterministic
  fixture and real MAFFT 7.526: selected-only jobs exclude unselected proteins
  and masks, retain stable IDs/groups and leave source intact; the menu opens a
  result containing only selected rows. Existing whole/column presets, strand
  changes, process errors, cancellation/tree termination and path quoting pass.
- The MAFFT process regression also passes under ASan+UBSan with real MAFFT,
  including selected-subset source lifetime and separate result windows.
- Template integration/settings regression passes with real MAFFT 7.526: full
  and fragment additions each accept fast and accurate Adjust strand, reverse
  the target and preserve template orientation/residues and original columns.
  Core validation rejects reversed templates and unauthorized target reversal.
  Reversed trace links retain the exact original evidence object.
- Viewport incremental/full redraw comparisons pass at 100% and 150% with groups,
  identity dots, residue backgrounds and all-match motif shading. Observed
  horizontal/vertical means were 7.1/3.7 ms at 100% and 61.2/7.7 ms at 150% while
  builds ran concurrently; these are Linux validation observations, not Windows
  performance guarantees. Grouped name layout was also inspected visually.

The next planned roadmap milestone remains trace-quality end trimming and
low-quality calls to N. These refinements do not advance the session format.

# Validation checkpoint — 16 October 2026, version 0.17.2

Basic workflow refinements following the user's successful 0.17.1 review. Shipped
Qt minimum remains 6.5; Linux validation copies alone use Qt 6.4.2. Windows
Qt 6.8.1/MSVC acceptance steps: TESTING_0.17.2.md. No native Windows build or
installer was produced here. Native binary format remains 3; optional version-1
view JSON gains remembered edit submode without changing old numeric mode values.

- Release application builds. Ten CTest suites pass after linking the updated
  core. The ten-case group suite includes gap-insensitive duplicate grouping,
  separate row types, old membership replacement/undo and rectangular gap unlock
  on ragged rows with unchanged trace evidence/mapping. Invalid row IDs reject
  before mutation. Group core checks also pass with ASan+UBSan.
- New focused 0.17.2 GUI regression passes in Release and ASan+UBSan (leak
  detection off): Edit/Overwrite startup; View/Edit switch and dropdown placement;
  remembered edit mode in sessions; View gap protection even against a direct
  gap-mode setter; context Copy/Cut/Paste across all modes and single-row/block
  shapes; actual Ctrl+C/X/V events with other sequence names selected; unchanged
  clipboard/document for block and name-only shortcuts; native whole-sequence
  payload rejection by fragment Paste; explicit sequence context Cut/Copy/Paste;
  fragment and sequence undo; menu/context rectangular gap unlock; and duplicate
  grouping ignoring case, gap type, placement and count while separating types.
- Existing GUI smoke, direct chromatogram editing and independent selection/native
  persistence regressions pass with revised menu names and controls. Protein/type
  and group/clipboard workflows also pass. Whole-sequence payloads retain native
  types/groups/traces; fragment paste remains residue-only.
- Sorting/row dragging/mask regression passes with the revised explicit sequence
  copy action. The MAFFT process regression also passes with a deterministic
  fixture and real MAFFT 7.526: whole/column/selected-only results, presets,
  direction adjustment, source immutability, busy controls, cancellation/tree
  termination, separate result windows and reports.
- Viewport incremental/full redraw comparisons pass with groups, identity dots,
  residue backgrounds and all-match highlighting at 100% and 150%. Observed
  horizontal/vertical means were 7.8/4.4 ms at 100% and 70.7/7.6 ms at 150% while
  builds ran concurrently. These Linux observations are not Windows performance
  guarantees. The updated toolbar and grouped names were visually inspected.

The next planned milestone remains trace-quality end trimming and low-quality
calls to N; these changes refine the existing editor rather than advance it.

# Validation checkpoint — 17 October 2026, version 0.17.3

Shipped Qt minimum remains 6.5. Linux validation copies alone use installed
Qt 6.4.2. Native session format remains 3 and numeric saved modes are unchanged.
Windows Qt 6.8.1/MSVC acceptance: TESTING_0.17.3.md; no native Windows build or
installer was produced in this environment.

- Release application and focused GUI checks build. Ten CTest suites pass after
  relinking against the updated core. The eleven-case group suite also passes
  with ASan+UBSan: selected gap locking preserves outside gaps, ragged ends,
  unselected rows, immutable trace evidence and residue mapping; one Undo/Redo
  restores the rectangle; already-locked spans create no history; invalid IDs
  cannot partially mutate the document.
- Updated GUI workflow passes in Release and ASan+UBSan (leak detection off):
  both Edit/View labels remain visible, segments are mutually exclusive, clicking
  the active segment preserves its state, both menu paths lock the selected
  rectangle, lock Undo/Redo works, View hides gap mutations and disables the
  right-click dropdown, and saved View/remembered editing mode restores correctly.
  Existing fragment shortcuts and explicit sequence clipboard checks still pass.
- Direct chromatogram editing/selection/paste/delete/undo and native session
  regression passes with the new segment controls. Default remains Edit/Overwrite.
- The complete focused GUI workflow also passes at 150% scale. Both Edit and View
  states were visually inspected at 100% and 150%; labels, active segment and
  disabled dropdowns fit without clipping. The two joined segments use native
  checkable Qt buttons, with keyboard focus and standard button activation.

The next planned milestone remains trace-quality end trimming and low-quality
calls to N; this is a refinement release of the existing editor.

# Validation checkpoint — 17 October 2026, version 0.17.4

Small layout/wording refinement built on the exact delivered 0.17.3 source.
Shipped Qt minimum remains 6.5; Linux validation copies use Qt 6.4.2. Session
format remains 3. Windows acceptance: TESTING_0.17.4.md; no Windows installer or
native Windows build was produced here.

- Release application and existing focused GUI workflow build and pass at 100%
  and 150% scale, retaining Edit/View segments, disabled View controls, lock/unlock,
  Undo/Redo, fragment shortcuts, explicit sequence clipboard actions and session
  restoration. Toolbar spacing was visually inspected.
- Reviewed both menus: Delete selected sequences follows sequence Paste before
  the section separator; fragment and sequence-selection actions are separated.
  The existing Ctrl+Delete binding and delete handler are unchanged.
- The identical-grouping dialog now explains complete ungapped identity and
  explicitly excludes a shorter sequence matching only a substring of a longer
  sequence.
  Its matching algorithm is unchanged. No new tests were added for these
  reversible layout and text edits.

The 0.17.3 controls/gap-locking changes remain intact. The next roadmap milestone
continues to be trace-quality trimming and low-quality calls to N.

# Validation checkpoint — 17 October 2026, version 0.18

Roadmap stage 4: trace-quality editing. Built from the exact delivered 0.17.4
baseline, preserving segmented modes, toolbar/menu refinements, grouping rules
and selection gap locking. Shipped minimum remains Qt 6.5; validation copies alone
use installed Linux Qt 6.4.2. Windows Qt 6.8.1/MSVC acceptance: TESTING_0.18.md.
No native Windows build or installer was produced here. Native format remains 3.

- Release application builds. Eleven CTest suites pass. Seventeen trace-quality
  core cases pass in Release and ASan+UBSan (leak detection off), including 150
  deterministic randomized comparisons against an exhaustive window reference.
  Cases cover exact means/threshold equality, strict low-Q conversion, gapped
  coordinates, internal low-quality islands, no-passing/short windows, absent Q
  versus Q0, manual substitutions, inserted calls, both strands, lowercase/N
  idempotence, scopes and ineligible types, stale/wrong-document rejection,
  cancellation, atomic multirow history and native session persistence.
- Core quality calculation reads immutable text and original evidence directly;
  it does not construct mutable residue-rank caches on the background worker.
  Edited symbols include earlier N cleanup; original-Q inclusion is explicit and
  reset off per dialog. Protected calls anchor the retained trim interval.
- New GUI workflow passes in Release and ASan+UBSan: empty selected scope is not
  all; explicit all/selected scopes; asynchronous Preview; controls locked while
  computing; cancelling waits for worker completion before source editing is
  enabled; changed settings clear the old preview and disable Apply; exact trim
  counts and retained intervals; ignored rows; missing/edited Q policy; original-Q
  inclusion; no destructive all-bad trim; atomic Apply/Undo/Redo; reversed evidence;
  groups and portable native roundtrip. Original calls/qualities/signals remain
  immutable and substituted current symbols gain no invented measured Q.
- Existing 0.17.3 basics workflow passes, retaining mode switching, disabled View
  controls, fragment-only shortcuts, explicit sequence clipboard, rectangle gap
  Lock/Unlock, grouping and session restoration.
- Complete new GUI workflow also passes at 150% scale. Trim/N preview dialogs were
  visually inspected at 100% and 150%; controls, policies, table and buttons fit.
  examples/trace_quality_demo.kleio is generated from the entirely synthetic test
  fixture; it embeds all evidence and reproduces the guide's known-result counts.

The next roadmap milestone is Highlight and descriptive statistics: shared column
summaries, Highlight menu, Statistics menu and nucleotide composition.

## Version 0.19 — 17 October 2026

Baseline: user-confirmed 0.18, restored from the exact delivered archive. Shipped
CMake minimum remains Qt 6.5; Linux validation uses the installed Qt 6.4.2 in an
isolated source copy with only that minimum lowered. No native Windows/MSVC build
or installer is claimed. Binary session format remains 3.

Verified:
- Release app build and all 12 CTest suites: sites, trace quality, groups, types,
  selections, core, traces, sessions, coordinates, motifs, residue styles, renames.
- New site/composition fixtures: known-answer categories, compatibility versus
  strict conservation, partial IUPAC/protein votes, masks/mixed types, empty scopes,
  ragged/trailing data, pooled denominators, zero-denominator N/A and cancellation.
  200 independently counted random nucleotide alignments preserve old summary
  values and highlight categories. Summary fixture has 9 patterns: gap-only and
  N-only columns share the same normalized missing-state pattern.
- Full 0.19 offscreen GUI check in Release and ASan+UBSan, plus 150% scale. Covers
  union/ranges, inclusive limits, explicit fixed row/column scope, cancel, edit/
  undo invalidation, color choice, native restore, all five palettes and three
  display styles with identity dots, byte-identical selected cells, menu move,
  composition scopes/ambiguity choices, settings invalidation, Copy/TSV equality
  and close-during-calculation. Screens inspected at 100% and 150%.
- Core/site/trace-quality suites under ASan+UBSan. Leak detection disabled because
  this environment cannot support the leak-check process; no leak-free claim.
- Retained 0.18 quality GUI, 0.17.3 basics GUI and 0.6 editing/statistics GUI pass.
- Raster backing-store viewport checks pass with highlighted column bands in
  background/identity/Chromas/group mode and selected monochrome mode. Incremental
  scroll/jumps, edits, undo/redo and font/cache changes match fresh repaints.

Single Linux measurements (not a Windows speed guarantee):
- 1,000 x 20,000 symbols: cooperative site scan 94.73 ms in 4,884 budgeted steps;
  slowest 4,096-budget step 0.262 ms; mask generation 0.060 ms; composition 159.44 ms.
- 3440 x 1440 viewport: highlighted backgrounds/identity/groups, 5.47 ms horizontal
  and 3.18 ms vertical per frame; selected monochrome, 7.91 / 3.39 ms.

Acceptance guide: TESTING_0.19.md. Scientific conventions: SITE_ANALYSIS.md.
Next planned milestone: protein coloring and translation, followed by distances,
similarity sorting, protein MAFFT and release/installer work.

## Version 0.19.1 — 18 October 2026

Baseline: the exact user-confirmed 0.19 source release. The native binary format
remains 3. Shipped CMake still requires Qt >=6.5; clean Linux validation builds use
installed Qt 6.4.2 in a separate source copy with only that minimum lowered. No
native Windows/MSVC build or installer is claimed.

Verified:
- Clean Release application build and all 12 CTest suites pass.
- New full-window GUI check passes in Release, ASan+UBSan and at 150% scale:
  Edit/View and Rewrite/Insert switches, exclusivity, disabled View controls,
  remembered submode, 12-pixel spacing, caret arrows, Shift one-column sliding,
  unchanged Ctrl maximum gap sliding, single-/multi-row dragging, deferred drag
  commit, residue/locked-gap pushing, open-gap consumption, alignment extension,
  undo/redo selection restore and read-only View.
- Scoped-row raster checks confirm only the analyzed name snapshot receives site
  shading, while outside rows/masks remain unchanged. Later name selections do not
  replace that explicit scope. New and legacy mode sessions migrate/restore safely.
- Main/context shortcut indications are checked, including Copy/Cut/Paste versus
  explicitly separate whole-row actions, selection and all movement commands.
  Movement is a separate context group. Block Cut/Paste remains disabled.
- Dimensions remain visible against the window's right edge at tested logical
  widths 1280, 900 and 720, including during selections, append and undo. Screens
  inspected at 100% and 150% scale.
- Retained clipboard/group/gap-lock basics, 0.19 analysis/composition, 0.18 quality
  and 0.10 trace editing workflows pass with current controls/shortcuts.
- Raster backing-store viewport check passes with site highlights, residue
  backgrounds, identity dots, Chromas palette and group swatches. Partial scrolling,
  jumps, editing, undo/redo and font changes match full repaint/fresh views.
  One Linux measurement at 3440 x 1440: 5.31 ms horizontal / 3.00 ms vertical per
  frame. This is not a Windows performance guarantee.

ASan/UBSan report no errors in the new GUI workflow; leak detection is disabled
for this runner, so no leak-check success is claimed. Windows acceptance steps
are in TESTING_0.19.1.md. Protein coloring/translation remains the next milestone.

## Version 0.19.2 — 18 October 2026

Baseline: exact user-confirmed 0.19.1 source archive. Native binary format stays 3;
shipped Qt minimum stays 6.5. Linux validation uses installed Qt 6.4.2 in a separate
source copy with only that minimum lowered. Native MSVC execution is unavailable.

Verified:
- Release application build and all 12 CTest suites pass.
- Focused mouse-history test reproduces a cache-related selection-undo failure in
  0.19.1, then passes the fix in both Rewrite and Insert. Mouse drag, full rectangle
  Undo/Redo and independent display/palette cache changes are covered.
- Full-window Release and ASan+UBSan check passes: counts hide before controls lose
  space and return on widening; menu Undo/Redo and actual Ctrl+Z restore mouse-drag
  selection; actual Ctrl+Shift+C/X/V operates on selected sequences and preserves
  native types/groups; plain Ctrl+C/X remains fragment-only. Empty name selection,
  FASTA row paste, context shortcut hints and one-step undo are covered.
- Full-window test passes at 150% scale; wide/narrow screenshots inspected.
- Extra syntax checks pass: -Wshadow -Werror=shadow on the affected GUI files and
  -Wconversion -Werror=conversion on statistics.cpp. The original MSVC warning
  sources are removed without suppressing diagnostics. This is not a claim of a
  locally verified warning-free MSVC build.
- Current mode/scope/native-session GUI, trace-quality GUI and chromatogram editing
  GUI pass. Raster viewport checks pass with site highlights, residue backgrounds,
  identity dots, Chromas colors and group swatches, including incremental scrolling,
  undo/redo and font/cache changes.
- One Linux viewport sample at 3440 x 1440: horizontal 6.24 ms/frame, vertical
  3.02 ms/frame. It is not a Windows performance guarantee.

ASan/UBSan reports no errors in the new GUI/history checks. Leak detection is
unavailable for this runner and is disabled. Windows acceptance and diagnostic
explanations are in TESTING_0.19.2.md. Protein coloring/translation remains next.

## Version 0.19.3 checkpoint — 18 October 2026

- Release application builds; all 12 CTest suites pass, including 65 core cases.
  New core assertions cover precise Undo/Redo residue destinations, common
  unchanged patch prefixes/suffixes, insertion/deletion boundaries, batch edits
  and absence of residue targets for rename/reorder/group history.
- A real MainWindow at 3440×1440 with 2,000 rows × 4,000 columns passes mouse
  selection and raster checks. Growing/shrinking selection pixels match full
  repaint results. Font-6 checks cover blue/yellow transitions, noncontiguous
  row sets, both editing modes, backgrounds and unchanged-selection suppression.
- Menu Undo/Redo reveals offscreen rewrite/fill, paste, insertion, deletion,
  gap-lock, block-slide and full-row changes. Name selection and the restored
  multi-row slide rectangle are preserved. Metadata undo does not move scrollbars.
- The same GUI checks pass at 150% scale, including fractional-scale outline
  cleanup. ASan+UBSan passes the GUI checks at 150% scale and all 65 core cases.
  Leak detection is disabled because it is unavailable under this runner.
- Existing 0.19.1/0.19.2 full-window checks and standalone drag-history checks pass:
  mode controls, counts priority, shortcuts/clipboard metadata, scoped highlighting,
  legacy/native mode persistence, mouse and keyboard block sliding are retained.
- Viewport raster checks pass with site highlights, residue backgrounds, identity
  dots, Chromas palette and groups at 100% and 150%, including scrolling and undo.

One Release/offscreen Linux comparison using the same mouse-drag workload:

| Style | 0.19.2 median event + paint | 0.19.3 median event + paint |
| --- | ---: | ---: |
| Colored letters | 4.74 ms | 1.66 ms |
| Residue backgrounds | 5.18 ms | 1.82 ms |

These are local samples, approximately threefold median improvement. The reported
half-second Windows delay was not reproduced locally; native Windows/MSVC timing
and driver behavior are not verified here. Partial repainting, cached ID checks,
deduplicated tile draws, unchanged-selection suppression and selection-only window
updates reduce the identified synchronous work. No artificial delay or timed
selection debounce is introduced. Windows acceptance is in TESTING_0.19.3.md.
Qt minimum 6.5 and native session format 3 remain unchanged; protein presentation
and translation remain the next planned feature stage.

## Version 0.19.4 checkpoint — 18 October 2026

The remaining lag with chromatograms open was reproduced using a real MainWindow
at 3440×1440 with two synthetic 1,000-call traces and an untraced row. Previous
selection updates redrew the signal and recalculated label positions repeatedly,
including scanning residue prefixes for offscreen calls.

- Trace positions, synthetic-run interpolation and original-call inverse mappings
  are indexed when the active row changes. Sample-mode labels use the visible
  interval. Gap positions and inserted-call interpolation match the core model.
- Viewport-sized cached layers hold signal/imported labels and Current labels;
  selection and caret remain dynamic. Selection-only updates do not redraw signal.
  Real geometry/style/data changes invalidate the cache, and trace IDs use a lookup.
- Release application builds and all 12 CTest suites pass. Existing direct trace
  GUI checks pass for typing, paste, deletion, gaps, sliding, Undo/Redo, evidence
  immutability, sticky labels, spacing and saving/reopening native sessions.
- New trace-panel checks pass at 100% and 150% scaling: cached/partial pixels equal
  a fresh render across selections, scrolling, zoom, gain, palettes, font and resize;
  visible label sets include overlapping/inserted calls. Trace switching/following
  and mixed traced/untraced selections retain their behavior.
- Cached positions and label properties match the core model through 75 random
  edits/reversals, coincident peaks, gaps, deleted calls and a 2,048-symbol synthetic
  run. Clearing the trace clears indexed labels. ASan+UBSan passes these checks at
  150% scaling; leak detection is unavailable and disabled.
- The 0.19.3 selection/raster/Undo-navigation GUI regression check also passes.

One Release/offscreen Linux comparison, median event + paint time per update:

| Viewer open, drag behavior | 0.19.3 | 0.19.4 |
| --- | ---: | ---: |
| Sample spacing, fixed starting column | 84.07 ms | 0.23 ms |
| Sample spacing, moving starting column | 88.62 ms | 0.57 ms |
| Alignment spacing, fixed starting column | 48.25 ms | 0.30 ms |
| Alignment spacing, moving starting column | 50.17 ms | 0.88 ms |

Timings vary with hardware/build/scaling and are not a native Windows/MSVC
performance guarantee. The Windows acceptance guide is TESTING_0.19.4.md. Qt
minimum 6.5 and native session format 3 are unchanged; no dependencies were added.

## Version 0.20 checkpoint — 18 October 2026

Stage 6 implements protein coloring and live/snapshot translation. Source minimum
remains Qt 6.5; a separate validation copy used Qt 6.4.2 on Linux, C++20, Release.
No Windows/MSVC or physical-monitor result is claimed.

- All 13 CTest suites pass (translation, site analysis, quality, groups, types,
  selections, core, traces, sessions, coordinates, motif search, styles, rename).
- Translation has 12 cases, including independent NCBI gc.prt catalog verification,
  all 64 concrete codons in each of 27 tables, IUPAC/stop/context rules, frames,
  coding/split-codon maps, stale-scan rejection, groups/naming/atomic snapshot Undo,
  token editing/sliding, narrow-gap rewrite, last-token deletion/reinsertion,
  column annotation movement, Clustal thresholds and format-4 provenance/clipboard.
- All 10 core suites pass with AddressSanitizer and UndefinedBehaviorSanitizer.
  Leak detection remains disabled for the runner; no leak-check success is claimed.
- translation_check passes at 100% and 150% DPI: controls, genetic-code Undo,
  nucleotide clipboard in AA preview, layer-off edit/re-enable, dialog previews,
  atomic snapshots, protein rewrite/native fragment paste, font-6 ruler, narrow
  controls, warm-cache pixels and 3440×1440 interaction with traces open.
- In the 161-row/9000-column synthetic trace-open translation check, final selection
  p95 was 2.36 ms (NT lettering) / 1.69 ms (AA) at 100%, and 4.36 / 2.48 ms at 150%.
  These are local offscreen measurements, not Windows performance guarantees.
- The previous 0.19.3 selection-pixel and Undo/Redo-navigation check passes,
  including large selections, font 6, trace-independent metadata and block moves.
- trace_selection_check passes its full open/hidden, sample/alignment, pixel-cache,
  trace-following, label/coordinate, edited/reversed evidence, coincident-peak,
  synthetic-run, zoom/gain/palette/font/resize checks.
- Release compilation emitted no GCC warnings in the final build log. MSVC and
  real-monitor acceptance steps are provided in TESTING_0.20.md.

Content edits currently rebuild profiles in cooperative revision-checked slices.
Affected-codon incremental recalculation remains a future scale optimization;
selection, scrolling and label changes reuse completed translation/count results.
Protein MAFFT remains roadmap stage 8. Native format 4 reads 1–3; earlier program
versions cannot open new native saves. Native layouts cannot be preserved by FASTA.


## Version 0.20.1 checkpoint — 18 October 2026

The seven requested translation/color-control fixes are implemented. Source
minimum remains Qt 6.5, with Qt 6.4.2 used only in a separate validation copy.

- All 13 Release CTest suites pass. The updated color case verifies every
  standard/rare/ambiguous amino-acid symbol has a color, case does not change it,
  gaps are uncolored, and fixed family RGB values replace conservation thresholds.
- All 10 core ASan/UBSan suites pass (leak detection disabled as before).
- translation_controls_check passes at 100% and 150% scaling: read-only concise
  genetic-code labels and full tooltips, no standalone custom button, final custom
  dropdown command, cancel/create/undo, nested preview-only custom-code creation
  and snapshot provenance, wrapped Set selection coding button, both copy menus'
  availability, and all 20 standard residues colored at >98% gap coverage in
  NT/AA/three-letter translation and imported protein rows.
- Original translation_check passes at 100% and 150%: code/Undo, preview/snapshots,
  native protein fragment editing, narrow controls, font-6 ruler, cached pixels,
  and 3440×1440 translation with a linked trace open. Final selection p95 was
  2.05 ms NT / 2.48 ms AA at 100%, and 2.89 / 2.49 ms at 150% in this offscreen
  runner. These measurements are not Windows performance guarantees.
- Build logs emitted no GCC warnings. Two test executables lost their executable
  permission in this runner; restoring that permission and rerunning those tests
  passed. No application logic change was needed for this runner artifact.
- Native sessions remain format 4, compatible with 0.20; full genetic-code names
  and assignments remain unchanged in the catalog and semantic metadata.

Windows/MSVC and real-monitor acceptance remain TESTING_0.20.1.md. Fixed colors
supersede the profile-dependent color contract described in the historical 0.20
checkpoint and original translation specification.


## Version 0.20.2 checkpoint — 19 October 2026

The five translation simplification requests are implemented. Source minimum
remains Qt 6.5; Qt 6.4.2 was used only in a separate Linux validation copy.
Windows/MSVC and physical-monitor acceptance remain TESTING_0.20.2.md.

- All 13 Release CTest suites pass. Translation cases retain the independent NCBI
  catalog/64-codon verification, IUPAC/context-stop rules, coding maps and stale
  result rejection. New cases verify centered ordinary protein records, independent
  gap/AA rewrite/delete/insert/slide/degap, retyping/provenance Undo, native clipboard,
  ordinary column minimization, format-4 persistence, 20 distinct family shades and
  independently serialized legacy bound-span migration, including malformed spans.
- All 10 core suites pass under AddressSanitizer/UndefinedBehaviorSanitizer.
  Leak detection is disabled under this runner, as in earlier checkpoints.
- Updated translation_check passes at 100%/150% DPI: no encoding control, centered
  snapshots, single-gap Cut/Paste, single-AA sliding, ordinary fragment Copy versus
  automatically expanded translated Copy, selected-name/data preservation, custom
  codes and Undo, font-6 ruler, narrow controls and cached pixels.
- translation_controls_check passes at both scalings: absent three-letter/standalone
  expansion commands, spaced panel, read-only short code labels, both custom-code
  workflows, all 20 shaded AAs at >98% gaps, black/white contrast, both copy menus,
  incomplete-codon expansion and split-codon/noncoding-only eligibility.
- 3440×1440 translation with a linked trace open remains responsive. Final selection
  p95: NT/AA 4.82/2.69 ms at 100%, 4.68/5.17 ms at 150% in this offscreen runner.
  These measurements are not Windows performance guarantees.
- No GCC warnings were emitted in the final build logs. A test executable lost its
  execute bit in the runner; restoring that permission allowed the complete
  sanitizer suite to pass without an application logic change.

Native format remains 4; its old display byte is reserved/zero and legacy span
records are validated and converted before document installation. New additions
and migrated translations retain source/code/frame provenance but use only
ordinary aligned protein cells. Old ordinary protein data do not move.


## Version 0.20.3 checkpoint — 19 October 2026

- Release Linux application and relevant validation targets build with GCC 13.3.
  Validation uses Qt 6.4.2 in a separate copy; delivered source keeps Qt >=6.5.
- All 13 CTest suites pass, including 16 translation cases and new group-name
  rename coverage. Mask tests cover exact phases, noncoding symbols, short padding,
  overlong rejection, atomic undo/redo, native persistence and stable type sorting.
- All 10 core suites pass under AddressSanitizer and UndefinedBehaviorSanitizer
  with leak detection disabled (no leak-check result is claimed).
- Translation GUI/control checks pass at 100% and 150% scale. New checks cover
  generation, pasted/existing mask application, overlong Apply disablement,
  annotation label and Undo, name-context section, menu placement, group-name
  preview/apply/Undo and tail-copy expansion. Existing color and editing checks pass.
- Wide 3440×1440 tests with traces open check cached/forced paint equality and
  selection without translation recounts. At 100%, median selection processing
  was 1.86 ms for nucleotide labels and 1.51 ms for AA labels (runner measurement).
- Application startup and screenshot inspected in the offscreen Linux environment.
  Windows/MSVC compilation and native Windows interaction remain user acceptance;
  no Windows binary or installer validation is claimed for this source patch.
- Native schema stays format 4. See TESTING_0.20.3.md for acceptance steps.


## Version 0.21 checkpoint — 19 October 2026

- Release application and distance GUI workflow tool build with GCC 13.3 and
  C++20. Validation uses Qt 6.4.2 in a separate source copy; shipped CMake retains
  the Qt >=6.5 requirement. The final changed-target build has no compiler warnings.
- All 14 CTest suites pass. The distance suite has 13 cases: independent numeric
  answers for all five models, deletion/overlap rules, unavailable/saturated
  estimates, unique-pair and group means, unweighted averaging, strict ambiguity
  filtering, chain ordering/ties/disconnected components, fixed ineligible slots,
  cancellation, resource rejection, TSV and 100 randomized column-oracle datasets.
- All 11 core suites pass under AddressSanitizer and UndefinedBehaviorSanitizer.
  Leak detection is disabled in this runner; no successful leak check is claimed.
- GUI workflows pass at 100% and 150% scale: all four report entries, both molecule
  types, model lists, independent selected-name/column scope, matrix/count views,
  exact matching Copy/TSV file export, stale-result invalidation, sort preview,
  Cancel/Apply/Undo/Redo, cancellation, closing during a job, and cancelling a file
  export while retaining the original destination contents. Dialog screenshots
  were inspected internally. Default/editor translation behavior is covered by
  unchanged existing regression suites; no alignment-painting changes were made.
- Matrices allocate a bounded lower triangle; means and sort passes avoid a full
  quadratic matrix. File output streams via QSaveFile; clipboard text is bounded.
  Large calculations are cancellable. No universal runtime bound is claimed:
  distance work still scales with pairs times analyzed columns.
- Native sessions remain format 4. Sorting uses the existing atomic row-permutation
  history. Analyses do not mutate the document and are not persisted as session data.
- Native Windows/MSVC compilation, Windows DPI/input behavior and end-user installer
  testing are not available here. Follow TESTING_0.21.md on Windows.


## Version 0.21.1 checkpoint — 19 October 2026

- Release application and GUI workflow tool build without compiler warnings in
  changed code using GCC 13.3. Validation uses Qt 6.4.2 in the separate validation
  copy; shipped source retains Qt >=6.5. A fresh core-only CMake build verifies
  generation of the central version header independently of the existing GUI build.
- All 14 CTest suites pass. The distance suite now has 16 cases, including exact
  SD/SE numeric answers, group-specific valid-pair denominators, fewer-than-two
  cases, equal distances, and an independent two-pass dispersion oracle across
  all five models, including unavailable/saturated pairs.
- The affected distance and site/composition suites pass under AddressSanitizer
  and UndefinedBehaviorSanitizer. Leak detection is disabled in this environment;
  no leak-check result is claimed. Existing unaffected suites retain their prior
  sanitizer checkpoint; they were not unnecessarily rebuilt for this patch.
- GUI workflows pass at 100% and 150% scale: mean SD/SE cells, visible formula
  and dependence explanation, matrix/version TSV copy/export, composition Copy
  version, normal analysis/scope/sort/Undo, cancellation/close, and atomic export
  cancellation. Internal screenshots were inspected for layout/readability.
- SD uses centered online Welford accumulation over valid pair distances only;
  naive SE is SD/sqrt(valid pairs), with its independence limitation explicit.
  Neither is reported with fewer than two valid pairs. No bootstrap claim is made.
- Application and distance/composition TSV version strings derive from the same
  CMake project version. Native schema remains 4; analysis results remain transient.
- Native Windows/MSVC and Windows UI validation remain pending user acceptance.
  See TESTING_0.21.1.md and DISTANCES.md.


## Version 0.22 — 19 October 2026

Stage 8: protein MAFFT uses declared row types and strict molecule validation in
all five existing workflows. Protein scoring controls replace nucleotide-only
settings. All legal AA symbols survive, with unknown-symbol scoring explained;
output residue validation is molecule-aware (protein U is not nucleotide T).
Whole/column/template merges now preserve protein provenance as well as existing
group/trace metadata. Similarity sorting explains N/?/ambiguity exclusions in its
dialog and visible command tooltip. Native format 4 and shipped Qt >=6.5 remain.

Local verification:

- Core-only Debug build with GNU g++ 13.3.0; all 12 CTest suites passed.
- Full application, process fixtures and Qt workflow tools compiled without GCC
  warnings. All 15 CTest suites passed, including the new protein_mafft suite
  (7 cases, including 24 preset/model combinations, all 4 BLOSUM values,
  corruption/strand rejection, ACGT-only proteins, column context, mixed scope,
  source immutability, provenance and lossless template projection).
- MAFFT v7.526 (2024/Apr/26), official Linux binary: 21 direct real protein jobs
  cover 8 presets, 4 BLOSUM matrices, JTT, transmembrane, composition, columns,
  selected-only in a mixed document, and both full/fragment template additions,
  including insertions and complete original-column projection. Tests also check
  pre-launch mixed/wrong-type rejection, cancellation and typed settings controls.
- Five further real protein jobs through MainWindow menus verify every workflow,
  separate result windows, selected-only scope, mixed rejection, group retention,
  same-type template settings and immutable trace evidence on unselected DNA.
  The same tool checks the requested N explanation in the actual sort dialog.
- Existing MAFFT workflow check passed with the process fixture and real MAFFT:
  8 nucleotide presets, selected columns and 2 strand-adjustment methods; masks,
  case, selected-only, process/version/output errors, early/running cancellation,
  descendant process termination, Windows batch quoting, settings and result windows.
- AddressSanitizer + UndefinedBehaviorSanitizer: all 5 affected suites passed
  (protein_mafft, core, traces, groups, sequence_types). Leak detection was disabled
  for this runner; no leak-check result is claimed.
- Protein settings screenshot inspected at 760x650: all fields and buttons fit,
  model-dependent fields switch correctly, and nucleotide-only rows are hidden.

Qt verification uses the available Qt 6.4.2 in an isolated validation source copy
with only its local minimum changed. The delivered source still requires Qt 6.5.
No Windows/MSVC or native Windows MAFFT execution was available here. Test Windows
launcher paths, real data and installation using TESTING_0.22.md before acceptance.

Reproduction (Qt >=6.5; use absolute executable/launcher paths):

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build -DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON
cmake --build build --target kleiotide_protein_mafft_check \
  kleiotide_protein_mafft_window_check kleiotide_mafft_check kleiotide_mafft_fixture
QT_QPA_PLATFORM=offscreen /absolute/build/kleiotide_protein_mafft_check /absolute/mafft
QT_QPA_PLATFORM=offscreen /absolute/build/kleiotide_protein_mafft_window_check /absolute/mafft
QT_QPA_PLATFORM=offscreen /absolute/build/kleiotide_mafft_check \
  /absolute/build/kleiotide_mafft_fixture /absolute/mafft
```

The optional checks are not enabled in ordinary application builds. The source
archive uses tools/package_source.py's fixed historical ZIP timestamps and omits
build/validation products. Archive CRC, file contents and the Qt minimum are checked
before delivery.


## Version 0.23 — 19 October 2026

Stage 9 delivers Sequence → Collapse identical sequences, read-only preview,
selectable representatives, concatenated titles/custom separator or _xN counts,
explicit All/Selected scope, mask opt-in and one-command Undo/Redo. Case and all
gap states are ignored for full-string equality; row types stay separate and
ambiguities remain literal. Representative IDs, original symbol/gap layout,
groups and complete trace/protein metadata remain. Free-residue compression is
removed from scope at the user's request. Native format stays 4; Qt minimum 6.5.

Local results:

- Core-only Debug build, GNU g++ 13.3.0: all 13 CTest suites passed.
- Full Qt application and optional collapse workflow tool built without GCC
  warnings. All 16 CTest suites passed. New collapse suite has 9 cases, including
  200 randomized datasets checked against an independent complete ungapped-string
  oracle; each collapse is checked through exact native serialization Undo/Redo.
- Cases cover U/T distinction, literal ambiguities/N, shorter substrings, nucleotide/
  protein/mask separation, empty/gap-only rows, explicit selected scope, count 13
  including the representative, Unicode/repeated titles, custom separators,
  chosen gap layout, group conflicts, immutable trace/protein provenance, native
  persistence, invalid/overlapping/stale choices, no-op and cancellation.
- Menu-driven Qt workflow passed: lazy set/member tables, aligned-symbol previews,
  alternative representatives in all three biological sets, retained group/trace
  paths/protein metadata, custom title separator, live switch to/from counts,
  selected-only x2 with an unselected duplicate retained, mask opt-in, Cancel,
  no-match Apply disabling, stale-source rejection, preparation cancellation and
  one-step Undo/Redo restoring every original title, row and evidence object.
- AddressSanitizer and UndefinedBehaviorSanitizer: all 5 affected suites passed
  (collapse, core, traces, groups, sessions). Leak detection disabled for the
  runner; no leak-check result is claimed.
- Synthetic scale check: 10,000 rows x 2,000 residues (some with extra gaps),
  1,000 sets of 10, removing 9,000 rows and Undo restoring 10,000. Debug core:
  scan 799.5 ms, title preparation 5.4 ms, validated atomic Apply 833.8 ms;
  history accounting about 19.8 MB. These are runner-specific timings, not release
  performance guarantees. Scan and title preparation run off the GUI thread;
  final validation/commit and Undo remain synchronous atomic document operations.
- Preview screenshot inspected at 1120x780: both tables, retained-data/name
  summary and buttons fit; long titles/symbol excerpts elide in preview while
  actual generated titles remain complete. The retained summary reserves height
  to avoid clipping when its content changes.

Qt checks use the available Qt 6.4.2 with a lowered minimum only in an isolated
validation copy. Delivered source retains Qt >=6.5. Windows/MSVC execution was
not available here; follow TESTING_0.23.md for the existing Windows kit and data.
No installer is included yet. Reliability/recovery/scale review and installer
acceptance are the remaining roadmap stages.

Reproduction with Qt >=6.5:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build -DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON
cmake --build build --target kleiotide_collapse_check
QT_QPA_PLATFORM=offscreen /absolute/build/kleiotide_collapse_check
```

The optional GUI tool is off in ordinary builds. Source packaging uses fixed
historical ZIP timestamps to avoid future-dated CMake build loops. Before delivery,
CRC, every archived source byte, source-only contents, version and Qt minimum
are checked.


## Version 0.24 — 19 October 2026

Requested addition: File → Export edited trace (AB1). A standalone ABIF 1.01 copy
contains the current ungapped sequence in both call sets, measured Q for unchanged
calls and Q0 for substituted/inserted/unscored calls. Retained call positions and
analyzed signals are exact; inserted positions are interpolated. Reverse-complement
orientation transforms channels/sample axes together with calls. Native format
remains 4; no extra application dependencies are introduced. Instrument-specific
metadata is not reconstructed; AB1_EXPORT.md documents precise scope and limits.

Verification:

- GNU g++ 13.3.0, core-only Debug: all 14 CTest suites passed.
- Full Qt application and GUI export workflow built without GCC warnings.
  All 17 CTest suites passed. The new AB1 export suite has 11 cases, including
  75 randomized edited traces and exact checks of every exported channel sample.
  Cases include set-1/set-2 imports; substitutions/insertions/deletions; open and
  locked gaps; crowded/coincident positions; entirely unanchored calls; reverse
  complements; 1–4-call inline tags; unsigned 16-bit samples/positions; absent/full
  byte-range Q; UTF-8 sample labels; portable sessions; invalid plans/ranges;
  cancellation, failing streams and unchanged source evidence/history.
- Biopython 1.88 independently reads 24 exports: a synthetic source and three
  real Biopython ABI fixtures (3730.ab1, 310.ab1, A6_1-DB3.ab1), each unchanged,
  edited, reverse-complemented, crowded and entirely unanchored, plus 1–4-call
  traces. Both PBAS/PLOC/PCON sets match the expected sequence, peaks and quality.
  Every DATA sample is compared to the original source in the expected orientation.
- Staden io_lib 1.14.15 (Debian library, independent read_abi API) independently
  reads the 20 normal-length exports with the expected calls, positions and every
  channel sample. Its uppercase-N to '-' convention and unsigned signal storage
  are normalized only by the test wrapper. The four short inline-PBAS traces use
  Biopython/Kleiotide because of Staden's pre-existing short-call read limitation.
- Independent probes and verifier sources are included; external readers and real
  fixture files are validation-only and are not bundled into the application.
- GUI workflow passed: selected/preferred trace; only eligible choices; edit/Q/
  orientation summaries; imported-source overwrite protection; Unicode/spaced
  paths; automatic .ab1 suffix and .abi acceptance; forward and RC output;
  export with a missing original file from a portable session; unchanged source
  document, saved state, title and history; Cancel; responsive preparation
  cancellation; reopening the export; and disabled action without trace evidence.
- AddressSanitizer + UndefinedBehaviorSanitizer: all 5 affected suites passed
  (abif_export, trace_quality, core, traces, sessions). Leak detection disabled
  for this runner; no leak-check success is claimed.
- Export dialog screenshot inspected at 780x450; controls, policy, summary, source
  protection message and buttons fit.

Qt verification uses available Qt 6.4.2 with only the validation copy's minimum
lowered. Delivered source retains Qt >=6.5. Windows/MSVC and commercial Windows
AB1 viewers were unavailable; TESTING_0.24.md covers acceptance with the user's
usual viewer/editor. These tests establish compatibility with the tested readers,
not instrument re-basecalling or universal vendor-software support. Known
reader-specific edge cases (very short PBAS and high-byte PCON) are documented in
AB1_EXPORT.md; values are not silently rewritten to accommodate reader bugs.

Reproduction (Qt >=6.5):

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build -DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON
cmake --build build --target kleiotide_abif_export_check
QT_QPA_PLATFORM=offscreen /absolute/build/kleiotide_abif_export_check
/absolute/build/kleiotide_abif_export_probe abi-check /path/to/input.ab1
python tools/check_abif_compatibility.py abi-check
```

The Python check requires Biopython. Optional tools/staden_abif_probe.c links to
an installed Staden io_lib; its executable can be passed as the Python check's
second argument. Neither is needed for ordinary Kleiotide use. Source ZIPs retain
fixed historical timestamps; CRC, source bytes, version, Qt minimum and absence
of build/validation products are checked before delivery.


## Version 0.24.1 checkpoint — 20 October 2026

- Restored the previously delivered 0.24 ZIP, verified SHA-256
  ac22cb375ac244fdcf8e54d8d051be151a14c7a6aa5516021377f5951ede2521,
  and applied the compatibility/batch changes in a fresh source tree.
- Release core build, GCC 13.3 and C++20: all 14 core suites pass, including 12
  AB1 export cases (new legacy directory/metadata regression).
- Qt application and extended single/batch export GUI tool build successfully.
  Local Qt 6.4.2 was used in a validation-only copy; shipped minimum stays 6.5.
  No GCC warnings in the final application build.
- Three Qt suites (motif search, residue styles, renaming) also pass: 17 distinct
  core/Qt suites in total. The Qt AB1 suite was additionally checked there.
- Offscreen GUI workflow passes: preferred current trace, exported calls/Q/peaks,
  reverse complement, portable sessions, Unicode paths, original-file protection,
  cancelled preflight, unchanged session/history and reopening; batch scopes,
  name preview, duplicate/existing filenames, reserved/long filenames, source and
  destination overwrite refusal, per-file errors, cancellation and cleanup.
  Both dialogs were visually inspected for layout.
- Biopython 1.88 reads all 24 new synthetic/3730/310/A6_1-DB3 exports;
  checks include both call sets, every current residue, position, Q and sample.
- The official BioEditFullFolder.zip SHA-1 matches the author's published value
  688d6607f0ace12f93a92019788f07231e2aef1c. Its 7.7.1 ABIF parser runs unmodified
  in Unicorn with file/allocation/string runtime stubs. The 0.24 writer reproduces
  an invalid allocation (1634624545 bytes) on an edited synthetic trace. The
  revised writer passes 20 normal-length exports with exact calls, positions and
  every analyzed sample. A short inline sample-label case also passes.
- BioEdit's standard-inline-PBAS limitation excludes 1–4-call files from parser
  emulation; those are checked using Kleiotide and Biopython. Its signed-signal
  display limitation for unsigned amplitudes is documented, with no silent
  amplitude conversion. Run-date markers are explicitly labeled placeholders.
- No native Windows/MSVC, BioEdit GUI or new Chromas execution was available.
  The user confirmed Chromas reads 0.24 exports. Native 0.24.1 acceptance follows
  TESTING_0.24.1.md. Emulated parser success is not a Windows rendering guarantee.
- Application dependencies and native session format remain unchanged.


## Version 0.24.2 checkpoint — 20 October 2026

- User confirmed 0.24.1 opens exported traces in BioEdit and Chromas without
  errors, with all suggested acceptance tests passing.
- Verified the 0.24.1 source ZIP SHA-256 before copying the release tree:
  1a942c4bf96c4acaa88cd97a798ae0f9b9a1f01f7f284ed7d0d32029c514b57e.
- Added the editable Filename suffix field, default _edited, optional empty
  value, immediate preview updates and filename-safe suffix handling.
- Release Qt application and extended export GUI workflow tool build without
  GCC warnings. Validation-only Qt 6.4.2 copy; shipped minimum remains Qt 6.5.
- Extended GUI workflow passes: default/custom/empty suffix, invalid path
  characters, live preview, actual output filename, selected/all scopes,
  collision handling, source/output protection, per-file failures, Cancel,
  portable evidence and reopening. New suffix cases reuse the existing tool.
  The updated batch dialog was visually inspected.
- AB1 writer code is byte-identical to accepted 0.24.1 source. Only centrally
  generated version metadata changes to 0.24.2. No new format/parser changes.
  Broad scientific/MAFFT and independent-reader checks were not repeated for
  this filename/UI-only refinement. Native session format remains 4.


## Version 0.24.3 checkpoint — 20 October 2026

- Based on the delivered 0.24.2 ZIP, verified SHA-256
  7748e24a2940a52a9170118e5189811a838a5d207f1f462e7e6b344ec1dea8f0.
- The batch preview's source-name section is interactive; the output-filename
  section stretches. On narrowing, source names yield first; preferred source
  width returns when sufficient space is available.
- Release application build passes without GCC warnings. Local Qt 6.4.2 in a
  validation-only copy; the delivered source still requires Qt >=6.5.
- Existing export workflow passes. A temporary validation-only extension sends
  actual mouse press/move/release events to the header, verifies divider motion,
  narrows the dialog to 450 px, checks output-column priority, widens to 850 px,
  checks preferred-width restoration and verifies suffix changes preserve it.
  Narrow and wide dialog captures were visually reviewed. The temporary checks
  are not additional permanent test infrastructure for this small UI change.
- Export naming, AB1 writer and native session code remain unchanged. Scientific,
  MAFFT and independent-reader suites were not repeated for this layout change.


## Version 0.25 checkpoint — 21 October 2026

- Based on the delivered 0.24.3 source. Added aligned/IUPAC and exact-overlap
  merging, optional original-Q conflict resolution and cancellable previews.
- Shipped CMake/core build succeeds with GCC 13.3, C++20, Release and
  -Wall/-Wextra/-Wpedantic. All 15 core CTest suites pass. The new merge suite
  contains 21 test groups, all three requested examples in every input order,
  all IUPAC unions and 100 independently generated three-read overlap cases.
- Covers informative overlap/conflict counts, unknowns, internal/terminal gaps,
  case/U normalization, literal exact ambiguities, containment, both extension
  directions, chains, disconnected/short/mismatching reads, repeat ambiguity and
  a bridge that resolves unknown evidence. No partial result is appended.
- Q tests cover measured zero versus missing Q, ties (including ambiguous calls),
  exact/insufficient margin, multiple observations, substitutions/insertions,
  gap/reverse-complement coordinate mappings and retained immutable evidence.
- Result append, custom naming, ungrouped/untraced consensus, retained originals,
  single Undo/Redo, native session round trip, invalid inputs, source identity,
  stale revision and cancellation pass. Sequence IDs remain monotonic on Undo,
  consistent with the existing Document append contract.
- Full Qt application and new kleiotide_merge_check workflow tool build. Local
  Qt 6.4.2 was used in a validation-only copy; shipped minimum stays Qt >=6.5.
  No compiler warnings in the final application build. The three existing Qt
  suites (motif search, residue styles, renaming) pass: 18 suites total.
- Offscreen Qt workflow passes at scale factors 1 and 1.5: menu eligibility,
  automatic preview, minimum overlap, custom/empty titles, Q activation/margin,
  setting invalidation, actual Apply, selected result, source groups/evidence,
  Undo/Redo, Cancel, stale rejection and cancellation while the worker runs.
  Aligned, exact and Q dialogs were captured; 100% and 150% layouts inspected.
- New merge translation unit and merge tests pass AddressSanitizer and UBSan,
  linked to the validated Release core objects. LeakSanitizer was disabled
  because this execution environment cannot inspect /proc task information.
- The conservative exact-overlap search is best-first and does not backtrack
  through weaker placements; ambiguous repeats are reported. This limitation
  and the internal base/gap consensus policy are explicit in MERGING.md.
- Native Windows/Qt 6.8.1/MSVC execution is unavailable here. Follow
  TESTING_0.25.md on the user's normal kit. Native session format remains 4;
  AB1 writer, original evidence, translations and MAFFT code are unchanged.


## Version 0.25.1 checkpoint — 21 October 2026

- Implements all seven follow-up requests: Sequence type first and separate
  merge/collapse section; prominent complete/partial/failure banners and collapsed
  rules; single-source lowercase; conflict/Q-resolution highlights and navigation;
  bounded substitution/indel-tolerant overlap; reviewed partial/component results
  with direct FASTA saving; stale name-offset AB1 recovery.
- Release core build with GCC 13.3/C++20 and all 16 core CTest suites pass. Merge
  suite: 32 groups, including 100 original random exact-overlap cases and 40 new
  known three-edit overlaps (substitution plus two inserted bases), precise
  conflict coordinates, case, individual/combined edit budgets, containment,
  components, partial Undo, omitted identities, cancellation and memory bounds.
- Tolerant search uses semi-global DP over edit counts, rolling score rows and
  byte traceback with a 64-million-state limit. Zero differences preserves the
  exact search path. Component search is best-first rather than exhaustive.
  A previously isolated singleton may be reconsidered as another contig grows.
- New synthetic ABIF recovery suite covers live/unused Pascal-name areas and
  growth, shrinkage, empty and 255-character names, exact recovered evidence,
  malformed recovery refusal, clean-file behavior and editing/export round trips.
- Supplied MYX15849_PHYkar_cf_i2469_S2.ab1 opens: 580 calls, 6,998 samples/channel,
  recovery delta +23. Its original SHA-256 remains
  40631f10e370e034664f0bb84c8ed690a45818e59f7689ccbf74a344ff0bbaf2.
  Independent Python struct parsing compares every PBAS1/PCON1/PLOC1 value and
  all four dye-mapped DATA9–12 arrays against a separately repaired reference;
  Kleiotide's unchanged re-export matches all of them exactly. Research data and
  temporary repaired/reference exports are not included in this source ZIP.
- Qt Release application builds without new compiler warnings. Local Qt 6.4.2
  was used in a validation-only copy; the shipped minimum remains Qt >=6.5.
  Three Qt regression suites pass (motif search, residue style, renaming):
  19 core/Qt suites total.
- Extended GUI workflow passes at scale factors 1 and 1.5: menu order and section,
  complete/error/partial banners, lowercase, substitution/indel preview, exact
  highlight locations/navigation, partial opt-in, excluded identities, separate
  component switching, Save FASTA content and source-history preservation,
  original aligned/exact/Q Apply, custom/empty naming, Undo/Redo, stale checks,
  cancellation and the actual renamed AB1 import/recovery notice. Complete,
  failed, Q and partial preview layouts inspected in rendered captures.
- New merge and ABIF reader translation units plus their respective regression
  suites pass AddressSanitizer/UBSan, with validated Release objects for unchanged
  core code. LeakSanitizer is disabled due to this runtime's /proc restriction.
- Native Windows Qt 6.8.1/MSVC execution remains unavailable. TESTING_0.25.1.md
  provides the acceptance procedure and examples/merge_refinements.fasta provides
  small reproducible inputs. Native session format stays 4; original AB1 files,
  analyzed signals and AB1 writer behavior are not modified by recovery.


## Version 0.25.2 checkpoint — 21 October 2026

- Removed both conflict-navigation buttons, their handlers and state. Colored
  conflict/Q-resolution highlighting remains. Result labels retain enough height
  for all explicit statistics lines after the shorter control layout.
- Gap conflict classification now uses the first/last informative nucleotide
  calls. Mixed terminal N/n, ?, open-gap and locked-gap runs are absent evidence.
  Original placement spans still track non-gap symbols; source data, provenance
  and quality mappings are unchanged. Gaps enclosed by informative calls, including
  IUPAC ambiguities, still contribute indel conflicts.
- Candidate ranking now minimizes edit count first, then maximizes informative
  matching support, subject to the configured minimum. Qualifying exact overlaps
  short-circuit tolerant DP even with a positive allowance. Exact-repeat ambiguity
  is preserved. The same edit-first order chooses the next read added to a contig;
  component search remains best-first rather than globally exhaustive.
- The supplied terminal-tail example yields no conflicts/highlights. The supplied
  repeat-rich pair yields the exact requested 55-base sequence, zero differences
  and no conflicts with allowances 0, 1 and 2, in either row order.
- Release C++20/GCC 13.3 core/application builds pass without new warnings. All 16
  core CTest suites pass, with 38 merge groups. New cases cover mixed missing tails
  at both ends, all-missing rows, true internal gaps, IUPAC evidence boundaries,
  exact-repeat ambiguity with a positive allowance and 100 independently enumerated
  short-sequence alignment oracles for edit-first ranking. Existing 100 random
  exact and 40 known three-edit overlap cases continue to pass.
- Three Qt suites also pass: 19 core/Qt suites total. Extended offscreen GUI
  workflow passes at scale factors 1 and 1.5, explicitly confirming absent buttons,
  retained conflict highlights, the requested 55-base preview at allowances 0/1/2,
  zero edit counts and actual Apply. Other merging/Q/partial/FASTA/Undo/cancellation
  and real renamed-AB1 workflows continue to pass. Updated layouts inspected.
- The new merge translation unit and all 38 merge test groups pass AddressSanitizer
  and UBSan with unchanged core Release objects. LeakSanitizer remains disabled
  due to the runtime's /proc limitation. No AB1-reader or session-format change.
- Qt 6.4.2 was used in a validation-only copy; shipped Qt minimum remains 6.5.
  Native Windows Qt 6.8.1/MSVC execution remains pending. TESTING_0.25.2.md and
  examples/merge_terminal_and_priority.fasta provide the acceptance cases.


## Version 0.26 checkpoint — 21 October 2026

- Added Group by % identity to Sequence → Groups, with existing-alignment or
  internal pairwise semiglobal identity, scope, threshold, overlap and prefix.
  Defaults are 97% identity and 80% coverage of the shorter trimmed sequence;
  overlap can be lowered to zero to allow arbitrarily short qualifying overlaps.
- Greedy processing is stable descending trimmed ungapped length, comparing
  against fixed representatives in creation order. There is no document reorder
  or all-pairs matrix. Protein/nucleotide separation, literal ambiguity policy,
  missing-call exclusion, internal indels and terminal missing regions are
  documented in IDENTITY_GROUPS.md. Existing catalog entries are retained;
  eligible scoped rows receive new, collision-free numbered memberships.
- Pairwise implementation uses exact-fragment KMP and three affine rolling rows,
  with O(min(n,m)) DP workspace and a 128 MiB workspace guard. It optimizes the
  alignment score, then computes identity; it is not a VSEARCH replica. Current
  orientation only. General divergent comparisons remain quadratic in length.
- Added lazy membership preview, prominent result summaries, background
  calculation, Stop/Cancel, source/revision checks and one-step group Undo/Redo.
  Group validation and group patch application now use linear passes/hashed
  lookups instead of repeated per-member searches. Group history preserves its
  existing monotonic ID allocation and metadata-only content revision semantics.
- Added One sequence per group to Sequence → Select: first in current order,
  longest ungapped (ties first), or uniform reservoir random selection, exact
  preview and Draw again. Empty groups skip; Ungrouped is an explicit opt-in.
  Selection does not modify data/history. Bottom-left idle counts removed;
  residue selection coordinates/row count retained, upper-right totals retained.
- Release C++20/GCC 13.3 application and core builds pass, without new warnings.
  All 17 core CTest suites and three Qt suites pass (20 total). The new core
  suite contains 15 groups, including independent exhaustive short affine-overlap
  enumeration, a supported internal gap and extension, threshold boundaries,
  masks/unknowns/types/codon padding, scoped metadata/Undo/Redo, stale/foreign
  rejection, cancellation, random-distribution sanity, 20,000-row group updates
  through reorder history, million-base exact matches and the DP memory guard.
- The actual menu/dialog workflow passes offscreen at scale factors 1 and 1.5:
  both calculation methods, longest-first representatives, collision-free prefix,
  selected scope, skipped reasons, settings invalidation, validation errors,
  stale-plan rejection, responsive Stop and worker-close cancellation, one-step
  Undo/Redo, all three representative choices, Ungrouped and Cancel, unchanged
  selection history/data, removed idle counts and retained residue information.
  Both dialog layouts were inspected in rendered captures at both scales.
- Identity/group/alignment translation units and the new identity suite plus
  existing group suite pass AddressSanitizer/UBSan; unchanged core objects use
  the validated Release build. LeakSanitizer remains disabled because of the
  runner's /proc restriction. No leak-check success is claimed.
- The bundled FASTA's expected results were calculated: three groups using its
  alignment and two with realignment at 90% identity / 80% overlap, five assigned
  rows and one skipped missing row in both modes. TESTING_0.26.md supplies these
  cases and native Windows acceptance steps.
- GUI validation used Qt 6.4.2 in a separate validation-only copy. The shipped
  Qt minimum stays 6.5; native Qt 6.8.1/MSVC 2022 Windows execution is pending.
  No external dependency, native session, AB1 import/export or merge change.


## Version 0.26.1 checkpoint — 21 October 2026

Implemented all five requested grouping refinements: internal word-ranked
centroid candidates (default maximum 16, 0 exhaustive), faster affine DP,
centroid/member labels, atomic UTF-8 preview TSV, deletion of old groups that
become empty, and filtered Select/Unselect all membership controls.

All 17 core and 3 Qt suites pass. The identity suite has 19 test groups.
Qt workflows pass at both 100% and 150% scaling for the new export/control/
catalog/member behavior plus existing grouping, selection and cancellation.
The changed identity engine passes AddressSanitizer and UndefinedBehaviorSanitizer
checks with leak detection disabled. It builds with -Wall -Wextra -Wpedantic
-Werror. The final Qt changes build without warnings under the available GCC kit.

The packed affine recurrence matches the original 0.26 implementation on all
five result counters for 4,000 randomized nucleotide/protein pairs and for
combined lengths 8,190 and 8,191 around the dispatch boundary. Candidate tests
also cover short/seedless/ambiguous exact strings, protein separation, acceptance
thresholds, deterministic ranking and agreement on separated synthetic clusters.

The supplied 5,555-sequence FASTA completes in 19.13 seconds (a separate run:
22.57 seconds), 6,939 comparisons, 101 groups at 97% identity/80% overlap with
16 candidates. The complete preview TSV has 5,555 rows; all meet both thresholds.
The first 500 source rows take 60.82 seconds in 0.26, 6.44 seconds in the new
exhaustive mode, and 2.52 seconds in the new fast mode. Fast search can choose
different centroids; see TESTING_0.26.1.md for measured differences and commands.
No claim of exhaustive or VSEARCH equivalence is made for the heuristic search.

Distributed sources retain Qt >=6.5. Qt workflow validation uses the available
Qt 6.4.2, with only the validation copy's minimum version lowered. Native
Qt 6.8.1 / MSVC 2022 acceptance remains pending. No input FASTA was modified.


## Version 0.26.2 checkpoint — 21 October 2026

All five requested refinements are implemented: Ungrouped one/all selection,
manager-order representative preview, responsive background table validation,
confirmed clear-and-delete-all, and mouse-drag range selection with subset
membership controls. Clear and import operations retain Cancel/Undo semantics.

All 20 test suites pass: 17 core and 3 Qt executables. The expanded identity
suite has 20 groups and group suite has 13. Both changed engines pass ASan/UBSan
with leak detection disabled. The importer compiles under -Werror. Qt workflows
pass at 100% and 150% scaling, including 5,555-row malformed preview exports,
real drag selection, all clear confirmation branches, manager ordering, all
Ungrouped options, active-worker Cancel and replacing input during validation.
The large-import runs took 624–709 ms for invalid/header/corrected previews;
maximum observed UI heartbeat gap was 65 ms. No input files were modified.

The importer uses a virtual table and debounced cancellable snapshot workers,
with latest-input/revision guards. Bounds are 16 MiB and 100,000 data rows.
No new dependencies, native session format change or clustering-algorithm change.
Qt workflow tests use Qt 6.4.2 in a validation-only copy; distributed sources
retain Qt >=6.5. Native Qt 6.8.1/MSVC 2022 testing remains pending.


## Version 0.26.3 checkpoint — 21 October 2026

The shared group-table parser removes enclosing parentheses and decodes standard
TSV double-quoted cells before matching titles and groups. It retains literal
internal parentheses, decodes doubled quotes/tabs, and rejects malformed quotes
and invalid group names. Loaded and pasted tables share this behavior.
All 14 group test groups and the Qt workflows pass, including wrapped corrected
input after a 5,555-row malformed table and active-worker cancellation/replacement.
ASan/UBSan checks pass for the updated parser (leak detection disabled).
Native Qt 6.8.1/MSVC validation remains pending.


## Version 0.26.4 checkpoint — 21 October 2026

The group importer now supports arbitrary sequence/group column mapping for
wider TSV tables, numbered selectors with optional header names, and automatic
first/second-column defaults for two-column input. Saved clustering previews
are directly importable. Mapping changes retain worker cancellation, immediate
Apply invalidation, revision checks and atomic Undo semantics.

All 15 group test groups pass, including a 5,555-row multi-column table, custom
column order, quoted/parenthesized fields, header metadata, unused quoted tabs,
missing/equal mappings, malformed data, bounds and cancellation. Qt workflows
pass at 100% and 150% scaling, including switching to unrelated columns and
back, restoring two-column defaults, Apply/Undo and active-job replacement.
Maximum measured UI heartbeat gaps were 22 ms / 48 ms; multi-step mapped import
checks took 1.24–1.30 seconds including debounce delays. The changed importer
passes ASan/UBSan with leak detection disabled. Bounds include 4,096 columns
to protect the selector UI from pathologically wide input.

Qt validation uses Qt 6.4.2 in a validation-only copy; distributed sources retain
Qt >=6.5. Native Qt 6.8.1/MSVC testing remains pending.


## Version 0.26.5 checkpoint — 21 October 2026

Group manager displays live staged counts for every group and Ungrouped, using
incremental membership updates and a single label refresh after bulk actions.
Delete empty groups removes zero-member named groups using the full staged
membership, independent of filters. Group names and stored metadata are unchanged.

The application builds without new warnings. Expanded GUI workflows pass at
100% and 150% scaling for counts, member moves through every assignment path,
filter independence, deletion of newly empty groups, focus preservation/fallback,
Cancel, Apply, exact Undo/Redo and display-only suffixes. Existing group/import/
selection workflows pass. No core data format or clustering change was introduced.
Qt workflow validation uses Qt 6.4.2 in a validation-only copy; distributed sources
retain Qt >=6.5. Native Qt 6.8.1/MSVC validation remains pending.


## Version 0.26.6 checkpoint — 21 October 2026

Group-manager buttons now read Check all / Uncheck all. The sequence list shows
cached group-color markers and membership tooltips for actual staged groups;
Ungrouped rows are unmarked. Markers update after every membership edit path,
color edits and group deletion. Text uses normal theme colors and selected icons
retain their original group colors.

Application compilation and expanded GUI workflows pass at 100% and 150% scaling.
Checks cover labels, exact marker colors in normal/selected states, Ungrouped,
other-group membership while viewing a different group, tooltips, individual/
filtered/subset edits, live recoloring, group deletion, staged Cancel, Apply and
Undo. Previous counts/cleanup/import/selection workflows continue to pass. No
core/file-format change or new dependency was introduced. Native Windows/MSVC
validation remains pending; Qt checks use the available Qt 6.4.2 validation copy.
