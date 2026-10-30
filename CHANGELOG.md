# Changelog

## 0.29.0 — local BLAST, SPART and table exchange

- Replace BLAST+ remote searches with local database creation and blastn, blastp,
  blastx, tblastn and tblastx searches; remember databases by target molecule.
- Remove NCBI protein searches after another small, valid live blastp job remained
  pending beyond a reasonable interactive wait. Keep NCBI nucleotide HTTPS.
- Add matricial/XML SPART group import with partition selection, plus SPART/XML/TSV
  export; share TSV/CSV/XLSX loading and column mapping across all three table dialogs.
- Retain only the reads-to-contig merge preview, color unresolved conflicts red and
  Q-resolved conflicts amber, and allow dragging the name divider. Overlap Q
  resolution applies after placement and leaves search acceptance unchanged.
- Make the initial batch AB1 action prominent, restrict row imports to one file,
  compact parameter fields, group BLAST controls, put executable browsing inline,
  shorten tooltips and hide similarity/collapse rules behind expanders.
- Limit BLAST modality to its owner window, open a new window on an empty relaunch,
  preserve XLSX accession hyperlinks, and correct the reported narrowing/shadow warnings.


## 0.28.1 — trace mapping and BLAST diagnostics/defaults

- Trace assignment cells show dropdown arrows, filename-first searchable popups,
  full-width layout with compact initial height, assignment reasons and explicit
  Not attempted states for unassigned rows.
- Unique exact trimmed matches retain original call indices and are Perfect even
  for N-rich, low-quality traces. Adds a regression for this distinction.
- BLAST has saved defaults in Settings, noneditable database menus, visible input
  warnings, tooltips, submission-order query numbers, per-query identity-ranked
  hit numbers and HSP count in the 21-column table/TSV/XLSX.
- New protein searches default SEG off, matching the documented URL API default.
  Configured paths and explicit standard-client routing are separate choices.
- Live countdown/elapsed/server-reply diagnostics replace the fixed waiting label;
  metadata no longer displays an out-of-range batch. Reports and RID links aid
  investigation; BLAST+ version/arguments/stderr are recorded, clients serialized,
  and monitoring has an explicit configurable bound without automatic resubmission.
- The reported native blastp/advanced-client no-result searches require live
  retesting with their RIDs/reports; a server-side cause is not established here.

## 0.28.0 — BLAST and trace assignment

- Fit the reads-to-contig preview to its row count, with scrolling for many rows.
- Link AB1 evidence to selected/all-untraced nucleotide rows using name suggestions
  and manual assignments; preview forward/reverse affine mapping, leave ambiguous
  calls unmapped, preserve original evidence and Apply/Delete in one Undo.
- Add nucleotide/protein BLAST menus: built-in documented HTTPS, optional NCBI
  BLAST+ remote advanced options, query dedup/batches, throttled/off-peak jobs,
  cancellation/resume, GenBank enrichment and linked TSV/XLSX result tables.
- Require Qt Network and verify native Windows Schannel deployment.
- Expand automatic tests to 41, including loopback HTTP/child-process fixtures and
  trace-mapping ground truth. Record native 0.27.2 acceptance; real ARM is pending.

## 0.27.2 — clipboard alignment and session sequence imports

- Copy consensus plus contributing reads in final-contig FASTA alignment.
- Rename aligned merge command; add a motif-search magnifier/nucleotides icon.
- Correct trace shortcut tooltip and handle overview hover events consistently.
- Remove the chromatogram title/close strip, retaining existing visibility controls.
- Add/drop .kleio sequences, masks and linked traces transactionally in one Undo;
  discard imported session metadata with a clear notification, retaining current
  session settings. Whole-session File Open remains available.
- Record native Windows 0.27.1 acceptance, including installer/Open With and
  improved overview performance. ARM Automatic/fallback remains pending.

## 0.27.1 — tester feedback and targeted overview updates

- Update only affected overview summaries after length-preserving edits, with
  safe coalescing and Undo/Redo; speed the single-nucleotide-row initial scan.
- Add bright caret/selection and contrasting viewport markers.
- Normalize missing/unrecognized FASTA suffixes to .fasta before confirmation.
- Preview original reads aligned to merged consensus, highlight all differences,
  and allow valid partial contigs by default with the existing amber notice.
- Keep Translation toggle next to editing controls and its controls on a separate
  lower row after layout restoration; use Overwrite wording.
- Group view panels under Toolbars; place Settings just above Exit; add explicit
  session appearance application and mark manual checkpoints as testing-only.
- Add local FASTA/AB1 append by drag/drop, GitHub and citation in About.
- Refresh installer Open With registration and include a per-user repair script;
  preserve default apps and guard side-by-side uninstall cleanup. Native retest
  remains required for the reported Explorer loop.
- Expand the release suite to 38 entries. Record native 0.27.0 acceptance and
  earlier ARM MAFFT Threads=1 success; defer BLAST and trace linking to 0.28.0.

## 0.27.0 — preferences, recovery and bounded overview (checkpoint B)

- Add File → Settings with validated new-session font/color/mode/visibility,
  default genetic code (including full custom definitions), FASTA import type,
  memory/Undo/overview limits, default MAFFT launcher/threads and recovery policy.
  Apply/Cancel/Restore defaults are distinct; current sessions keep their choices.
- Migrate legacy MAFFT preferences. Per-job choices no longer change global
  defaults; Settings Test performs a real tiny alignment with cancellation/logs.
- Initialize fresh imports/blank documents with the default genetic code as a
  clean baseline. Saved and legacy sessions retain their own metadata/defaults.
- Add periodic, separately stored atomic .kleio recovery checkpoints, per-window
  locks, up to two generations, disk quota, cancellation/status, Recover/Open copy
  and confirmed discard. Recovery leaves originals and ordinary dirty state intact.
- Make session output cancellable in bounded chunks without changing format 4.
- Replace full-column overview evidence with bounded tiles and up to 8192 GUI
  summaries. Compute conservation before aggregation; preserve IUPAC/coverage
  semantics and reject stale content.
- Isolate GUI workflow preferences in temporary INI profiles, explicitly select
  the configured QSettings backend, and keep the normal native user namespace.
- Expand the release suite to 37 entries with preference, bounded-overview,
  recovery/abrupt-process-exit and end-to-end Settings/recovery coverage.
- Fix source packaging for full x.y.0 version prefixes, matching installer checks.
- Record Oleg’s native 0.26.12 test-1 acceptance and real MAFFT --thread 8 success.
  Native acceptance of 0.27.0 and real ARM MAFFT remain pending. Trace linking and
  broad scan/storage refactoring remain later checkpoints.

## 0.26.12 — implementation checkpoint A

- Windows test correction: replace the fixed-1280 counter-visibility assumption
  in gui.mode_controls with font-aware wide/narrow layout checks, enlarged-font
  coverage and actionable diagnostics. Application code and installer scripts
  are unchanged by this correction; all 33 tests remain enabled.

- Add an eagerly maintained sequence-ID index with preallocated transactional
  updates for insert, delete, reorder and Undo/Redo; failed allocations retain
  document content, published lookup state and history. Add failure-injection
  regressions and a row-scaling benchmark.
- Route merge FASTA and identity/composition/distance table exports through
  protected-path checks, explicit replacement confirmation and atomic streaming
  output. Preserve existing files on failed or cancelled writes.
- Resolve MAFFT Automatic (safe) to a positive logical-core count capped at 8;
  migrate the old -1 preference. Retry the specific core-count failure once
  with one thread; capture bounded batch-wrapper diagnostics and missing output.
- Add View → Toolbars → Editing controls and Restore default layout. Prevent
  accidental toolbar hiding through the default toolbar-area menu.
- Add synchronized exclusive Edit → Mode actions for View/Edit, Rewrite/Insert
  and all applicable right-click modes; retain right-click choice in sessions.
- Ctrl+Left/Right now slides one column; Shift+Left/Right slides as far as
  permitted. Update context hints and regression checks.
- Promote MAFFT and mode-control workflows into release-check: 33 tests total.
  Record local 33/33 and 150%-scale 11/11 GUI results; native Windows and real
  ARM MAFFT retests remain pending. Settings/recovery/trace linking are not yet
  implemented.

## 0.26.11 — installer test revision 2, ARM64 compatibility

- Replace the native-x64-only install guard with native x64 acceptance plus
  ARM64 Windows 11 acceptance when GetMachineTypeAttributes reports x64
  user-mode support. Windows 10 ARM and 32-bit Windows remain excluded.
- Keep the application, Qt DLLs and build kit x64. No native ARM64 or actual
  ARM64 runtime test is claimed; colleague install/run acceptance is pending.
- Update the packaging revision default to 2, record the test-1 ARM rejection
  and provide precise rebuild and focused ARM testing instructions.


## 0.26.11 — installer preparation, test revision 1

- Application code remains identical to the tested 0.26.11 baseline. Oleg
  reports 30/30 automatic tests and all manual Windows tests passing, including
  the bulk-memory guard. Record hardware and MAFFT 7.526 in acceptance notes.
- Add a Qt Creator `test-installer` target, PowerShell staging and NSIS setup
  for the MSVC x64 Release kit, with original Microsoft runtime installation,
  exact-file uninstall, versioned test installations and no file associations.
- Include matching source, Qt attribution/license notices, a corresponding-source
  release folder, checksums, build metadata and beginner packaging/tester guides.
- Native setup, clean-machine launch and uninstall acceptance remain pending.

This history is reconstructed from the archived README, validation checkpoints and testing notes in `docs/history/`. Entries describe the changes introduced at that time; later releases may replace those behaviors. Versions are ordered numerically. Dates are omitted where a release date is not established, and corrected archives with an unchanged version are not invented as separate releases.


## 0.26.11 — Windows acceptance follow-ups

- Make the merge GUI regression wait for its queued automatic preview to complete before asserting the error banner, avoiding platform-dependent timer ordering.
- Break down overlap-search edits into substitutions, inserted/deleted bases and compatible IUPAC symbol differences. Explain why literal search events can exceed final incompatible consensus columns; matching/scoring behavior is unchanged.
- Validate Windows output names before Save As interprets wildcards as a listing filter. Use a shared validated Qt save dialog on Windows across save/export commands; preserve file-type selection and final-path overwrite checks.
- Add bounded preflight estimates for bulk text edits, transforms, Flush and Minimize; skip unchanged transforms; remove redundant full-row validation copies for ordinary rows. Report oversized/failed Flush operations instead of letting allocation exceptions escape.
- Add regression coverage for atomic memory-limit refusals, enormous virtual flush dimensions, small edits in long ordinary rows, wildcard rejection/retry and search-event accounting. Record the supplied 29/30 Windows run, manual acceptance and scale observations in WINDOWS_ACCEPTANCE.md.

## 0.26.10 — Windows CTest Qt runtime discovery

- Prepend the selected Qt Core DLL directory to PATH for all four Qt unit suites and nine GUI workflows on Windows. Direct CTest runs and the release-check target use the same per-test setup, independently of Qt Creator's Run environment.
- Resolve the directory from the imported Qt target for the active build configuration, retain existing PATH entries and offscreen settings, and provide a CMake 3.21-compatible fallback.
- Document the reported 0xc0000135 startup failure and rerun steps. No editing, file-format or analysis behavior changes.

## 0.26.9 — licensing, branding and Windows testing guide

- Add the unmodified GNU GPLv3 license and explicitly license application code, project documentation, scripts and tests as GPL-3.0-only, with Oleg Shchepin's copyright notice.
- Add BRANDING.md with the owner's allowed-use policy, separate official artwork permissions, designer credit to Elizaveta Shchepina (Storminka / Io Tikhaya), and the project permission record. No logo/icon artwork is included yet.
- Add copyright, license and designer credit to About. Install LICENSE, BRANDING.md and the Windows guide with release documents; remove obsolete license-undecided statements from active guidance.
- Add beginner instructions for automatic and manual Windows testing. The release-check target retains text/JUnit results, and both CTest entry points fail if no tests are found.

## 0.26.8 — AB1 unknown-call import and historical changelog

- Accept `X/x` in ABIF base calls as unknown nucleotide `N/n`, retaining case, call count, selected call set, positions, measured qualities and all analyzed signal samples. Report the conversion and affected one-based call positions in an import notice; do not modify the input file.
- Keep other invalid calls and malformed traces rejected, with call-set, position and byte diagnostics for invalid symbols. Never fall back from edited calls to an unedited call set because of `X/x`.
- Add core/session/export regressions and a GUI import-notice workflow to the release checks.
- Reconstruct the changelog back to 0.1 from the archived development records and add Windows icon asset guidance.


## 0.26.7 — stabilization

- Ordinary Save retains frame, genetic code, custom codes and manual coding annotations in `.kleio` sessions.
- Resolve complete output filenames before source/session protection and replacement confirmation, including single-trace AB1 suffix handling.
- Share atomic, checked streaming output across session/FASTA, protein-only FASTA and single-trace AB1 export. Failed writes, short writes, cancellation and commit errors are covered by regressions.
- Disable translation mutations immediately on busy transitions and guard handlers against programmatic signals during workers.
- Remove the unused rename-table parser, moving distinct cases to tests of the application parser. Remove unreachable legacy protein editing branches while retaining validated old-session migration.
- Compile GUI implementation once for application/workflow targets. Make developer probes/benchmarks opt-in. Register eight GUI release checks with CTest and add a `release-check` target.
- Display the generated project version in About. Consolidate current build/release instructions and retain older notes under `docs/history/`.
- Add Windows validation and portable-package scripts plus CMake install rules. Native Windows execution, clean-machine deployment, interoperability acceptance and the application license decision remain pending.


## 0.26.6 — Group colors in the manager

- Check all / Uncheck all replace the former bulk-selection labels.
- Sequence-list markers show staged group colors and group-name tooltips; Ungrouped rows have no marker.

## 0.26.5 — Group counts and empty-group cleanup

- Show current staged membership counts for every group, including Ungrouped, independently of filters.
- Delete empty groups removes unused named groups, with Apply/Cancel and one-step Undo/Redo.

## 0.26.4 — Group import column mapping

- Allow sequence-name and group-name column selection in tables with more than two columns, including saved clustering previews.
- Keep the original first-column/second-column mapping for two-column tables; refresh previews after mapping changes.

## 0.26.3 — Quoted and parenthesized group tables

- Decode standard TSV quoted cells and remove surrounding parentheses before matching names in loaded or pasted tables.
- Preserve internal parentheses, escaped quotes and tabs inside quoted cells.

## 0.26.2 — Group workflow refinements

- Add One sequence / All sequences choices for Ungrouped representatives; order the preview like the group manager.
- Move group-table validation to cancellable background work with a virtual preview and stale-result protection.
- Add mouse-drag range selection and Check selected / Uncheck selected in the group manager.
- Add staged Clear and delete all groups with warning confirmation and Undo/Redo.

## 0.26.1 — Faster identity grouping

- Add heuristic word-ranked centroid shortlisting, defaulting to 16 candidates; zero restores exhaustive search. Candidates still receive affine semiglobal alignment.
- Speed up the aligner, clarify centroid/member labels and export complete grouping previews as TSV.
- Remove old groups that become empty after grouping; restrict manager bulk checks to visible filtered names.

## 0.26 — Identity grouping and representatives

- Group sequences by identity using existing alignment columns or internal pairwise semiglobal alignment, with identity/overlap settings and cancellable previews.
- Select first, longest ungapped or random representatives per group, optionally including Ungrouped.
- Keep alignment totals at the upper right and show residue coordinates at the bottom left only during selection.

## 0.25.2 — Conservative overlap evidence

- Ignore terminal unknown/gap runs as indel evidence and require informative calls on both sides of an internal gap.
- Prefer eligible overlaps with fewer assumed differences, then greater matching support; exact overlaps win over tolerant alternatives.
- Remove Previous/Next conflict buttons while retaining conflict highlighting.

## 0.25.1 — Tolerant overlaps and AB1 recovery

- Add bounded substitution/indel-tolerant overlaps, reviewed partial/component results and direct preview FASTA saving.
- Preserve lowercase in single-source consensus regions; add conflict/Q-resolution highlights and clearer result banners.
- Recover uniquely validated stale ABIF offsets caused by an in-place Pascal sample-name edit, with an import notice and no source-file modification.
- Reorganize sequence-type and merge commands. Conflict navigation introduced here was removed in 0.25.2.

## 0.25 — Sequence merging

- Add aligned-row consensus with informative overlap/conflict counts, IUPAC unions and explicit internal-gap conflicts.
- Offer original-Q resolution for traced rows with a configurable advantage and conservative fallback.
- Add exact-overlap searching, contained reads, chaining, placement previews and ambiguous-repeat detection.
- Provide cancellable previews, stale-plan checks, custom result names and one-step Undo/Redo while retaining source rows.

## 0.24.3 — Batch export preview layout

- Allow dragging the source-name/output-filename divider and preserve its preference across scope and suffix changes.
- Give output filenames the remaining width and restore preferred name width when the window expands.

## 0.24.2 — Batch export filename suffix

- Add an editable suffix, initially _edited, with immediate filename previews and invalid-character cleanup.
- Apply collision protection after the suffix and freeze reviewed names while exporting.

## 0.24.1 — AB1 compatibility and batch export

- Move the ABIF directory after external payloads and add bounded legacy metadata for BioEdit compatibility while retaining calls, peaks, quality and analyzed samples.
- Add background batch export for all traces or selected names, filename previews, numbered collision avoidance, cancellation and per-file reports.
- Exercise BioEdit's parser in emulation and independent Biopython checks; retain documented short-call/unsigned-signal reader limitations.

## 0.24 — Edited AB1 export

- Export edited linked traces, including traces embedded in portable sessions, through preflight and cancellable atomic writing.
- Preserve analyzed signals and original anchors; interpolate inserted-call positions and assign Q0 to changed or unscored calls.
- Transform channel orientation and peak positions for reverse complements; protect source AB1 files.

## 0.23 — Collapse identical sequences

- Preview duplicate sets by complete ungapped, case-insensitive sequence and row type, with optional mask inclusion.
- Choose the retained representative, combine titles or append counts, and preserve its identity, groups and trace evidence.
- Apply removals in one undoable action with cancellable background preparation and lazy previews.

## 0.22 — Protein MAFFT

- Support protein whole/selected/column alignment and full/fragment template addition with explicit molecule and protein scoring settings.
- Reject mixed participating molecule types, preserve non-target evidence and keep template columns and target insertions.
- Explain ambiguity exclusions in similarity sorting; N–N is not counted as a match.

## 0.21.1 — Distance dispersion

- Add sample SD of pair distances and explicitly naive SE to overall, within-group and between-group means, with formulas and dependence limitations.
- Include the generated application version in distance and composition TSV output; accumulate means without requiring a full matrix.

## 0.21 — Distances and similarity sorting

- Add nucleotide observed differences, p-distance, JC and K2P; protein observed differences, p-distance and Poisson correction.
- Provide matrix and overall/within-/between-group means with explicit scope, deletion rules, minimum sites, reasons for unavailable estimates and TSV output.
- Add previewed, undoable nearest-neighbor similarity sorting with stable ineligible-row slots and bounded background calculations.

## 0.20.3 — Translation masks and refinements

- Generate translation masks and annotate coding phases from existing or pasted masks, with Undo support.
- Translate incomplete trailing codons using unknown missing bases consistently across display, counts, copy and snapshots.
- Add group-name prepend/append renaming, stable type sorting and clearer translation/gap menu sections.

## 0.20.2 — Ordinary protein snapshots

- Use single-letter amino-acid display/output and center snapshot letters in ordinary editable protein rows; migrate earlier bound-span rows on load.
- Give standard amino acids distinct shades within their color families and expand translated-fragment copying to complete codons automatically.
- Remove three-letter controls and the separate selection-expansion action; retain session format 4 compatibility.

## 0.20.1 — Translation colors and controls

- Correct amino-acid colors in imported proteins and translated codons, including gap-heavy columns.
- Refine genetic-code selectors, custom-code cancellation and translated-fragment copy availability.
- Improve coding-selection controls and the then-present one-/three-letter display switch (removed in 0.20.2).

## 0.20 — Translation and coding annotations

- Add translation toolbar, coding ruler, frame selection, 27 NCBI genetic codes, custom codes and live stop/uncertainty counts.
- Use one translation engine for ambiguity consensus, split/partial codons, display, copying and protein snapshots with provenance and group inheritance.
- Introduce native format 4 for translation settings, coding annotations and protein provenance. Initial three-letter/bound-span presentation was simplified in 0.20.2.

## 0.19.4 — Chromatogram performance

- Cache signal/imported/current layers separately from selection overlays and index trace positions and call mappings per edit.
- Limit sample-spacing labels to the visible interval and use ID lookups for trace switching while preserving call/strand/quality behavior.

## 0.19.3 — Selection and history performance

- Reveal affected residues after Undo/Redo without navigation jumps for renaming, grouping or reordering.
- Repaint changed selection areas, skip unchanged notifications and avoid repeated panel refreshes and substring copies.

## 0.19.2 — Toolbar and clipboard fixes

- Let alignment totals yield to editing controls in narrow windows and return when space is available.
- Restore mouse-slide selections through Undo/Redo and add Ctrl+Shift+C/X/V for whole-sequence clipboard actions.
- Correct reported MSVC narrowing/shadowing warning sources without suppressing warnings.

## 0.19.1 — Editing controls and shortcuts

- Use Edit/View and Rewrite/Insert switches; allow sliding in both edit submodes with dedicated movement shortcuts.
- Keep fragment clipboard shortcuts independent of selected sequence names and expose movement shortcuts in menus.
- Retain captured analysis-row scope and improve alignment-count layout.

## 0.19 — Site highlighting and composition

- Add conserved, variable, informative, singleton, coverage and conservation-range highlights with explicit molecule/row/column scopes and custom colors.
- Add nucleotide composition with per-row/pooled counts, optional fractional ambiguities and TSV output.
- Move alignment summary into Statistics and persist analysis scope/highlight settings in sessions.

## 0.18 — Trace quality operations

- Add previewed trimming by complete mean-Phred windows and conversion of calls below a quality threshold to N/n.
- Use original measured qualities conservatively, retain alignment coordinates and gaps, and report ineligible/unscored rows.
- Apply quality edits with Undo/Redo and unchanged original trace evidence.

## 0.17.4 — Toolbar and menu polish

- Separate mode controls visually and group sequence deletion with whole-sequence clipboard actions.
- Clarify that identical grouping compares complete ungapped sequences, rather than matching substrings.

## 0.17.3 — Edit/View switch and gap locking

- Show joined, keyboard-accessible Edit/View segments and disable edit/right-click controls in View while remembering their previous choices.
- Lock only open gaps inside the selected rectangle with one undoable action, preserving residues and trace evidence.

## 0.17.2 — Fragment clipboard separation

- Make Ctrl+C/X/V consistently act on residue fragments and provide explicit whole-sequence clipboard commands.
- Refine View/Edit mode persistence and context-menu availability.
- Ignore gaps when grouping identical sequences.

## 0.17.1 — Type selection and grouping

- Add nucleotide, protein, mask and trace-linked name selectors and wider group swatches.
- Replace identical-sequence sorting with undoable grouping that retains row order.
- Add selected-sequence MAFFT result windows and target strand adjustment for template workflows.

## 0.17 — Persistent groups and native clipboard

- Add staged group creation, names, colors, membership and order; group swatches, context actions, group selection and stable group sorting.
- Import exact-title group tables with preview, duplicate/conflict handling and explicit Ungrouped assignments.
- Introduce native format 3 for groups and a typed whole-sequence clipboard retaining groups and embedded traces with fresh destination IDs.
- Add sequence cut/paste, single-row fragment cut/paste and faster horizontal wheel scrolling.

## 0.16 — Explicit protein and mask types

- Add FASTA import-type choices, typed new rows, undoable type assignment and type badges.
- Support canonical one-letter protein storage and shared editing while keeping nucleotide analyses type-aware.
- Introduce native format 2 with mandatory row types and format-1 migration; prefer sessions when explicit types need preservation.

## 0.15.2 — Motif highlights and filtering

- Improve motif visibility on residue backgrounds and add optional exclusion of N-only hits for literal/regex searches and both strands.
- Remove the redundant Select match button and bound filtering work for long overlapping N runs.

## 0.15.1 — Regex motif search

- Add case-insensitive regex search, distinct interval/strand counts and all-hit highlighting.
- Index results for Next/Previous navigation with cancellation and bounded storage; retain independent name selections.

## 0.15 — Motif navigation and unwrapped FASTA

- Write each FASTA sequence on one line for Save/export and sequence clipboard copying while continuing to read wrapped FASTA.
- Add a motif-search dock with overlapping-hit navigation, explicit scope, gap/strand/IUPAC options and an independent current-hit outline.

## 0.14.1 — Kleio and Chromas palettes

- Rename Classic to Kleio while retaining its saved identifier; add Chromas-style colors across residues and trace channels.
- Clarify overview conservation labels and record completed/future milestones.

## 0.14 — Display palettes

- Add alternative residue palettes, colored-letter/monochrome/background modes and synchronized toolbar controls.
- Adapt letter contrast to backgrounds and update trace colors while preserving real copied/exported symbols and neutral masks.

## 0.13.1 — Safe unique names

- Add previewed name sanitization to ASCII letters, digits and underscores, with deterministic collision suffixes.
- Remove alphabetical sorting by sequence symbols while retaining title and other sorting methods.

## 0.13 — Batch renaming

- Add previewed text, filename, number, length, uniqueness, reversal and table-based name operations with explicit all/selected scope and one-step Undo.
- Preserve exact full-title, simultaneous table matching and report unmatched/duplicate keys and invalid results.
- Remove the first-nongap-in-selected-columns selector and refine the identity-display icon.

## 0.12 — Selection tools and view toggles

- Add compact identity-dot, chromatogram and overview toolbar toggles.
- Add title prefix/substring/list, duplicate-name/sequence, motif and exact-size random-subset selectors with cancellable searches.
- Keep name-selection searches independent of residue selections and document history.

## 0.11 — Independent selections and identity dots

- Separate sequence-name selection from residue rectangles and refine Backspace caret behavior.
- Add render-only identity dots against the top sequence, retaining actual symbols for copy, export and analyses and persisting the view choice.

## 0.10 — Editable chromatogram Current row

- Support Current-row selection, typing, paste, delete and sliding through the shared edit/Undo model; keep imported calls and qualities immutable.
- Add alignment-spacing display alongside sample spacing, with monotone coordinate mapping and no invented measured peaks or quality.
- Keep call/Q labels legible and persist trace-spacing preferences in sessions.

## 0.9 — Portable sessions and multi-file opening

- Introduce .kleio sessions embedding trace evidence, edited symbols, stable IDs, row order, provenance and view state, with integrity checks and atomic saves.
- Open multiple AB1/FASTA files into a new document and coordinate nearby external file launches.
- Show linked-trace markers, keep overlapping Current/Imported calls visible and protect AB1/session files from incompatible saves.

## 0.8 — AB1 import and chromatograms

- Import ABIF calls, peaks, qualities, dye order and analyzed channels and show a linked, navigable chromatogram panel.
- Preserve immutable original evidence and edit mappings through substitutions, insertions, deletions, gaps, Undo/Redo and reverse complements.
- Retain trace provenance through MAFFT workflows and reject malformed imports atomically.

## 0.7 — Template MAFFT

- Add full-sequence and fragment addition to templates while preserving original columns and untouched rows and inserting new target columns.
- Validate target/template identity and order; require MAFFT 7.526+ for fragment addition and keep source documents unchanged.
- Make Insert place text after the selected symbol and clarify overview height/color labels.

## 0.6 — Editing, selections and statistics

- Support residue rectangles and ruler selections across modes, single-cell insertion, single-row replacement, deletion and run selection.
- Refine Escape, name-selection commands and MAFFT parameter help.
- Add cancellable alignment statistics with mask exclusion and documented ambiguity/missing-state conventions.

## 0.5 — Initial MAFFT integration

- Add whole-alignment and selected-column nucleotide MAFFT with presets/settings, strand handling, reports and cancellation.
- Validate result identities and preserve titles, case, IDs, masks and untouched sequence regions in a separate result window.
- Keep source documents unchanged on failures and cancellation. Corrected archives of this version retain the same version number.

## 0.4 — Row order, sorting and masks

- Add undoable multi-row drag reordering and background stable sorting by titles, lengths, identity, frequency and other keys.
- Treat FASTA rows containing unexpected printable symbols as editable masks and exclude them from nucleotide analyses.
- Include partial IUPAC calls fractionally in overview conservation. Later releases removed sequence-symbol alphabetical sorting and replaced identical sorting with groups.

## 0.3.1 — Layout and overview refinements

- Add a draggable name/residue divider, wider rename dialog and ruler layout that remains legible at small fonts.
- Separate overview residue density from stepped conservation colors and display no-information columns in gray.

## 0.3 — Clipboard fragments and overview

- Add plain-text single-row fragment copying and validated raw-text insertion while retaining FASTA clipboard imports.
- Add a detailed column ruler and an interactive alignment overview with cached, cooperative statistics.

## 0.2.1 — Sliding and selection refinements

- Let rigid slides push residues and locked gaps, consume external open gaps and grow rows to the right.
- Add double-click name renaming, anchored Shift-click rectangles, blank-area deselection, title/FASTA copying and a simpler fixed-width font dialog.

## 0.2 — Rigid fragment editing

- Add rectangular/ruler selection, keyboard/mouse rigid-block sliding, gap locking/unlocking and gap insertion/deletion.
- Add fill, case/strand transformations, degap/unalign/minimize operations and atomic group Undo/Redo.
- The initial slide-obstruction policy was replaced by pushing behavior in 0.2.1.

## 0.1.1 — Scrolling and table renaming

- Cache viewport rendering for faster scrolling and correct ruler clipping across display scales.
- Add exact-title table renaming with selected scope, duplicate/conflict checks, simultaneous mappings and atomic Undo/Redo.

## 0.1 — Initial editor

- Introduce FASTA loading/saving, sequence addition/removal, name/residue selection and View/Insert/Overwrite editing.
- Add stable row identities, efficient sequence storage, gap-aware indexing and bounded Undo/Redo history.
