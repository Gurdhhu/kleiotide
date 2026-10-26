# Kleiotide: implementation and validation plan

Prepared for Oleg Shchepin, 26 October 2026. Status: planning and source inspection completed; application changes are pending.

## 1. Baseline and findings

Use the current GitHub-ready 0.26.11 source as the development baseline. Its 127 source/test files match the current Source_For_Installer archive byte for byte. Preserve the already distributed test-2 release and its corresponding source, checksums and acceptance record. Record the actual Git commit before implementation; do not assume the private repository already contains this exact tree.

Inputs reviewed: TESTER_FEEDBACK_ms.md, MAFFT_error.jpg, no_buttons.jpg, Claude_Review.txt, kleiotide_row_index.patch and kleiotide_core_bench.cpp. The screenshots and uploaded review were read directly. This plan does not treat the external review's reported test runs or timings as independently reproduced results.

The colleague's test-2 setup SHA256 is E8C205431D050F6CEC51428675CE6EC0598D5403D0D368D17D43EBD1530BEE33.

| Evidence | Established result | Still open |
| --- | --- | --- |
| ARM64 Windows 11 Home 25H2, Snapdragon X Elite, 16 GB, 100% scaling, no development tools | Test 2 installs; Start menu launch, FASTA/AB1 import, session save/reopen and basic editing work | Uninstall/reinstall, grouping, broader analyses and workload limits |
| MAFFT 7.526 screenshot | The launcher runs; its log includes FAST_CWD warnings and ends with “Cannot count the number of physical cores” | Whether explicitly selecting one thread fixes this package; whether its Cygwin bundle has additional ARM/Windows compatibility problems |
| Missing-controls screenshot | The upper editing toolbar is absent while menus and the alignment remain visible | The exact hide/overflow/layout trigger |
| Current source | MAFFT thread default is -1; the editing toolbar has a Qt visibility action without an explicit View-menu recovery command; sequence lookups scan rows linearly | Native reproduction and measurements after fixes |
| Export source | Several dialog exporters construct QSaveFile directly without the shared protected-path checks | The full exporter inventory and regression coverage |
| Trace/session source | Immutable original evidence, per-row call maps, reversal and shared embedded evidence already exist | A safe sequence-to-trace alignment/assignment workflow |

Initial workarounds worth testing immediately:
- In the existing MAFFT dialog, choose Threads = 1 and retry a tiny nucleotide alignment. This is a diagnostic workaround, not a confirmed resolution.
- Try the main window's toolbar-area context menu and re-enable Editing. If unavailable, record that result; the fix will add a permanent menu recovery route.
- Keep testing on copies of data. No change to antivirus protection or substitution of individual Cygwin DLLs is part of this plan.

## 2. Delivery order

The requested settings and trace-linking workflow justify a new development cycle rather than immediately calling the existing build RC1. Suggested version names are provisional.

| Checkpoint | Scope | Exit condition |
| --- | --- | --- |
| A: stabilization patch, e.g. 0.26.12 | Export protection, MAFFT diagnostics/thread fallback, toolbar recovery, transaction/exception gaps, sequence-ID index | Existing tests pass; focused Windows x64 and ARM retests; measured row-scaling improvement |
| B: preferences and controls, e.g. 0.27.0 | File → Settings, common mode actions, shortcut swap, recovery foundation and bounded overview | Settings precedence and restart/session tests pass; controls remain reachable; memory behavior measured |
| C: trace linking, e.g. 0.27.1 | Assignment dialog, bounded forward/reverse mapping, preview, atomic link/unlink, persistence and export checks | Synthetic coordinate ground truth and real edited Sanger datasets pass |
| D: release-candidate preparation | Remaining measured scan/cache improvements, user documentation, installers and acceptance | No unresolved data-integrity issue; normal x64 and emulated ARM workflows pass or have explicit documented limitations |

Keep each checkpoint reviewable. Trace attachment is the largest and most scientifically sensitive addition; implement it in several commits, with the mapping core validated before its UI is enabled.

## 3. Reported Windows issues

### 3.1 MAFFT on the ARM laptop

The current dialog persists mafft/threads and defaults to -1, meaning MAFFT counts physical cores itself. The screenshot is consistent with that detection failing in this MAFFT/Cygwin environment. The FAST_CWD warning is relevant but does not, by itself, prove the cause of the failed alignment.

Implementation:
1. Reproduce the failure with a tiny known FASTA through both Kleiotide and the same launcher outside Kleiotide. Compare auto threads with an explicit thread count of 1.
2. Make the default “Automatic (safe)” resolve a bounded positive thread count in Kleiotide. Use a reliable OS core query where available, a conservative positive fallback otherwise, and never equate logical-CPU count with physical-core count in the label. Start with a conservative cap and benchmark it. Keep an explicit numeric setting.
3. Preserve MAFFT's own -1 detection as an advanced option if useful. Migrate the old stored -1 preference clearly to the new safe automatic policy.
4. When the specific physical-core-detection failure occurs after a self-detected automatic run, permit one controlled retry with an explicit positive count. Record the requested/resolved settings and retry in the job report. Do not retry arbitrary failures indefinitely.
5. Add a launcher test that performs a miniature real alignment, not merely --version. A version string establishes availability, not readiness for a job.
6. Capture exit status, bounded stdout/stderr tails, actual arguments, resolved threads and result-file existence/validation. Preserve useful launcher stdout from Windows batch wrappers when output is missing, even if the wrapper returns zero.
7. Show a targeted explanation for this error and leave the original document unchanged. Retain cancellation/process-tree cleanup during testing and retry.
8. If thread 1 still fails, investigate/update the complete official MAFFT package or a supported environment. Do not silently replace Cygwin DLLs or claim that Kleiotide can repair every external-runtime problem. WSL integration is a separate future adapter unless actually needed.

Acceptance: real MAFFT 7.526 on the colleague's ARM machine and Oleg's native x64 machine; whole/selected/column alignment and template/fragment jobs; success, cancellation, missing output, malformed output, path spaces/Unicode and launcher failure.

### 3.2 Disappearing toolbar

The code fixes the toolbar's position but still exposes Qt's toolbar visibility mechanism. The screenshot fits a hidden toolbar; overflow or a platform/layout issue remains possible.

Implementation:
- Add View → Toolbars → Editing controls, and View → Restore default layout.
- Give essential mode commands independent menu access as requested below.
- Make the editing toolbar non-floating and exclude its hide action from accidental default toolbar-area menus; retain deliberate visibility control through View.
- Restore a recoverable default layout without resetting the document, selection or scientific settings.
- Audit width/overflow handling, custom widgets, DPI changes, maximize/restore, touch context menus and the separate alignment-size toolbar.
- If reproduction remains elusive, add bounded diagnostic logging of toolbar visibility and geometry changes, with no sequence content.
- Keep layout visibility separate from persistent document content.

Acceptance: intentionally hide and restore without restarting; narrow/wide windows; all controls reachable through menus; 100%, 125%, 150% and 200% scaling; monitor changes; mouse/touch testing on ARM. Offscreen GUI tests cover state, not native Windows painting behavior.

## 4. File → Settings

### 4.1 Architecture and precedence

Introduce one typed AppSettings model/service and a SettingsDialog, using the existing explicit QSettings organization/application namespace. Centralize defaults, validation, migration and notifications instead of adding unrelated settings reads throughout the GUI.

Precedence:
1. Explicit values in an opened .kleio session, including legacy format-defined defaults.
2. Explicit changes made in that current session.
3. Global defaults when initializing a new blank document or a fresh FASTA/AB1 import.

Changing a default does not retroactively change existing session appearance or genetic codes. Provide an explicit “Use current appearance as defaults” action; applying defaults to an existing session must be an explicit operation. Global resource limits and recovery policy are operational preferences: use their new values for subsequent work in existing windows as well as new ones.

Keep machine-specific MAFFT paths, memory limits and recovery locations out of portable session metadata. Save all actual session translation settings, including the full custom genetic-code definition where relevant.

Settings actions: OK, Apply, Cancel, Restore defaults. Restoring defaults never deletes recovery checkpoints. Restore defaults edits the dialog draft until applied. Invalid values cannot be committed. Migrate the existing MAFFT launcher/thread keys once and preserve fallback behavior if a font or launcher is unavailable.

### 4.2 Recommended first settings

| Page | Settings | Behavior |
| --- | --- | --- |
| Appearance | Alignment font family, size and style; ruler readability; residue palette; colored letters/monochrome/colored backgrounds; identity dots | Defaults for new clean sessions; live preview in the dialog; session overrides remain authoritative |
| Editing and display | Initial View/Edit and Rewrite/Insert choice, default right-click behavior, initial overview/trace visibility | Preserve the current startup defaults during migration; View mode always uses a safe context menu |
| Memory and performance | Bulk-edit additional working-memory limit; Undo-history target; analysis/trace-preview workspace budget; background-scan delay | Separate labels and explanations; no misleading claim of a process-wide RAM ceiling |
| Translation | Default built-in genetic code; supported custom default code definition | Initializes new documents; never replaces saved coding annotations, frames or genetic codes in an opened session |
| MAFFT | Default launcher path, Browse, Test, safe automatic/numeric thread policy and optional thread cap | Per-job overrides remain possible; a temporary job choice does not silently replace the global default |
| Files and recovery | Recovery enabled, checkpoint interval, retention/disk quota; default FASTA import type; standard FASTA export gap policy; remember last folders | Explicit import/export semantics; recovery status and location available to the user |

Memory policy:
- Retain the current 2 GiB additional bulk-edit limit and 64 MiB nominal Undo target as migration defaults.
- Choose an initial conservative analysis/preview budget, provisionally 256 MiB, after measurements.
- Validate positive ranges and overflow-safe byte conversions. Increasing a limit permits larger plans; it does not reserve RAM or guarantee success.
- Distinguish retained document/evidence storage from temporary workspace and Undo retention.
- Lowering the Undo target may prune older history; explain this in the settings UI. The target is not a strict cap when one retained command itself is larger.
- Capture limits at job start. Do not mutate a running job's budget or discard its history mid-command.
- Resource refusal must keep content, groups, traces and saved-state bookkeeping intact.

Do not include global coding-region coordinates: those are document-specific data. Avoid a large set of unsupported rendering/performance toggles in the first dialog.

### 4.3 Settings tests

Cover restart persistence, Apply/Cancel/reset, missing fonts, invalid/tampered values, old-key migration, multiple open windows, old/new sessions, per-job MAFFT overrides and independent new-document defaults. Verify global-default changes do not dirty or modify an already open scientific document.

## 5. Modes and shortcuts

### 5.1 Common actions

Create one action/state model for the toolbar, Edit → Mode and alignment context menus. Use exclusive checkable QAction groups, rather than independently maintained menu booleans.

Edit → Mode contains:
- View/Edit subsection: View and Edit; exactly one checked.
- Rewrite/Insert subsection: Rewrite and Insert; exactly one stored choice. Disable these editing controls in View mode while retaining the last choice.
- Right-click submenu: Context menu; Insert gap; Delete open gap; Insert gaps in others; Delete open gaps in others. In View mode only Context menu is applicable. Restore the saved edit-mode preference on returning to Edit.

Use the same actions to synchronize mouse and keyboard changes, session restoration and busy-state disabling. Preserve locking/open-gap rules and independent sequence-name/residue selections.

### 5.2 Shortcut swap

| Shortcut | New residue-selection behavior |
| --- | --- |
| Ctrl+Left / Ctrl+Right | Slide selected residues/block exactly one column, subject to existing constraints |
| Shift+Left / Shift+Right | Slide as far as permitted by the existing gap/locking rules |

Update the viewport event handler, context-menu labels, tooltips, status hints, documentation and GUI regression tests together. Keep normal text-field shortcuts untouched: these alignment commands act when the alignment has focus and an eligible fragment is selected. Reserve Ctrl+Shift+Left/Right for selection/navigation: do not invoke sliding unless the modifier is exactly Ctrl or exactly Shift.

Test both directions, row/block/column fragments, locks, boundaries, insufficient gaps, View mode, busy mode and Undo/Redo. Menu operations and shortcuts must produce identical document changes.

## 6. Sequence → Link trace (AB1)

### 6.1 Menu placement and target scope

Add a separated trace section immediately below Set sequence type:
- Link trace (AB1)…
- Delete trace

Add the same two actions to the selected-sequence context menu. Delete trace removes the session association, never the AB1 file; current sequence letters remain unchanged and Undo restores the association.

The link dialog offers:
- Selected sequences, with a count.
- All nucleotide sequences without traces, with a count.

Default to selected names when available, otherwise all eligible untraced rows. Proteins/masks are ineligible; report exclusions. Empty nucleotide sequences can be listed with a clear failure reason. Selected rows with existing traces may be relinked explicitly.

### 6.2 Candidate loading and name assignment

Load a batch of AB1/ABI files through the existing defensive parser. Keep each successful parse as immutable evidence; cache by canonical identity/content hash and reuse one original object for repeated assignments. Parse failures appear per file without discarding valid files.

For each still-unassigned sequence, apply the requested priorities:
1. Sequence name equals the file basename without extension.
2. Sequence name contains that basename, or vice versa.
3. Sequence name equals the AB1 internal sample name.
4. Sequence name contains that internal name, or vice versa.

Start with literal comparison plus conservative whitespace normalization. Any optional case-folded candidates must be distinguishable from exact matches. Do not strip arbitrary suffixes, punctuation or sequence identifiers automatically.

A unique candidate at the best applicable level is a preliminary assignment, not proof of identity. Ties remain visibly ambiguous and unassigned; never select the first file by directory order. A file is not “consumed”: the same trace can be assigned to several rows.

Use indexed exact-name lookups and cancellable, bounded substring matching; a large sequence/file cross-product must not run synchronously in the GUI.

No all-versus-all sequence alignment is necessary at this stage. Content mapping is performed only on the pairs actually assigned.

### 6.3 Assignment table

Use a model/view table with editor delegates so thousands of rows do not require thousands of permanent combo-box widgets.

Proposed columns:
- Include/apply.
- Sequence name, with stable internal row ID.
- AB1 choice: no assignment, an existing trace or a loaded file.
- Add file button for that row.
- Name-match reason.
- Orientation: Forward / Reverse / No match.
- Peak mapping confidence: Perfect / Good / Terrible / Failed.

Each AB1 choice is searchable and distinguishes same-name files in different folders. Adding a file extends the common inventory and selects it for that row. It does not alter other manual assignments.

For a row already linked, distinguish keeping its existing association from explicitly removing it; do not make an empty combo accidentally mean deletion. Display planned replacements/removals before Apply.

Unused files and unassigned sequences are acceptable. Show counts in a footer, without repeatedly interrupting the workflow.

### 6.4 Mapping algorithm

Reuse the existing TraceLink representation:
- Original channels/calls/peaks/qualities remain immutable.
- Each current non-gap residue has an original-call index or -1 for no supported call.
- Reversal changes the per-row orientation and mapping order, not the stored original evidence.

Map against the edited sequence, not a text replacement of that sequence.

Proposed pipeline:
1. Preflight row, evidence and workspace sizes before copying or allocating. Build a gap-free normalized nucleotide view and recover residue-to-alignment-column correspondence through iterators/rank helpers or bounded windows where practical; avoid a second chromosome-sized position array. Keep the displayed letters, case and gap states intact.
2. Evaluate the original calls in both forward and reverse-complement orientation.
3. Try unique exact placement first where applicable. Require informative support; an all-N or very short repetitive match is not evidence of identity.
4. Find unique informative anchors through the candidate overlap. N/? provide no positive anchoring evidence; IUPAC compatibility is weaker than exact A/C/G/T identity.
5. Chain consistent anchors and refine the regions between them with a bounded affine-gap alignment. Terminal clipping/overhangs are inexpensive; internal indels are penalized more than substitutions, matching the expected edited-Sanger scenario.
6. Use adaptive bands and a bounded fallback for short pairs with few anchors. Estimate memory/work before allocating. Never allocate an unrestricted full matrix for a chromosome-length row against a trace.
7. Compare forward/reverse scores and competing placements within each orientation. Require enough aligned informative sequence and an adequate separation from alternatives.
8. Convert confident alignment columns into original-call anchors. An edited substitution may retain its original peak; an inserted residue receives -1. Trimmed original calls are omitted.
9. Identify ambiguously placed gaps/repeats and leave unsupported residues unmapped instead of inventing peak correspondences. Display interpolation may position these residues, but supplies no original quality or peak.
10. Validate strict monotonic call order, bounds, orientation, mapping length and source evidence before producing a preview result.

For reverse matches, map back into the original AB1 call index space in decreasing order and set the reversed flag. Do not physically reverse original channels.

Free terminal gaps must not turn a tiny accidental local match into “Perfect.” Include aligned-core length, informative support, retained-sequence coverage, internal edits, ambiguity and alternative-placement/orientation margin in the confidence calculation.

Provisional confidence definitions:
- Perfect: an unambiguous placement where every retained sequence residue has a supported original call and agrees after normalization. Trimming extra original calls at the ends is allowed.
- Good: a strongly supported orientation/placement with limited internal edits or unmapped terminal additions.
- Terrible: a plausible but weak or partly ambiguous placement; substantial doubt remains about peak association.
- Failed: no credible overlap, unresolved orientation/placement, invalid evidence or a resource/processing failure.

These are heuristic mapping-confidence labels, not Phred scores. Define numerical thresholds in one documented policy after calibrating on ground-truth fixtures and representative real edited reads. Do not present arbitrary initial thresholds as a validated scientific statistic.

A forward/reverse tie or palindromic/repetitive ambiguity is reported as No match with the reason, not broken arbitrarily. A later advanced orientation/anchor override can be considered if real workflows require it.

### 6.5 Preview, cancellation and Apply

Preview operates on an immutable/read-stable source with a captured document revision, row IDs, evidence identity and parameter generation.

- Run file loading and mapping outside long GUI-blocking loops.
- Bound concurrency, workspace and mapping-vector allocations. The current representation requires an integer per retained residue, so very long rows may need a clear resource refusal even when the AB1 itself is short.
- Cache repeated pair results where the sequence, evidence and options are unchanged.
- Cancel safely without changing the source.
- Changing a file/assignment/sequence/parameter invalidates the affected preview and prevents stale Apply.
- Labels always accompany accessible color/icon cues. Suggested outcomes: green Perfect, blue/green Good, amber Terrible, red Failed.
- Default inclusion to successful high-confidence rows. Terrible requires deliberate inclusion; Failed cannot be committed as a fabricated mapping. Unassigned rows remain unchanged.
- Repeated Preview is supported. Report a compact summary and expose detailed alignment/coverage/edit counts only on demand.

Apply uses a dedicated batch trace-metadata command, with no redundant full-sequence text copies. Preflight and validate every included link before committing; the complete accepted batch has one Undo step. Failure leaves all rows and history unchanged.

Separate text-content changes from trace-metadata changes so attachment/deletion does not unnecessarily recompute whole-genome conservation. Refresh trace indicators and the trace selector using appropriate change notifications.

### 6.6 Persistence and evidence protection

Current session format 4 already serializes shared original chromatograms once, then per-row mappings and orientation. Reuse it if the feature's stored data fits this model; a format change is not necessary simply to add the dialog.

The loader should return the same shared evidence object for repeated use so both RAM and session deduplication work. Preview diagnostics can be recomputed; do not persist an unreliable confidence score as if it were original evidence.

Validate session round trips, native clipboard behavior, Undo/Redo, reverse-complement, row duplication/removal/reordering, subsequent edits and edited-AB1 exports. Original source files remain protected throughout relinking/unlinking; audit provenance checks when links are removed or replaced.

### 6.7 Mapping test corpus

Create synthetic traces with known call/peak indices and derived sequences for:
- Forward/reverse exact matches and trimmed reads.
- Long terminal deletions and additions on either sequence.
- Many substitutions but few small internal indels.
- Homopolymers, tandem repeats, equivalent gap placements, palindromes and near-tied orientations.
- N/IUPAC-heavy, short and unrelated sequences.
- Locked/open alignment gaps and mixed case.
- Missing qualities, repeated peak positions, recoverable AB1 parsing notices and malformed files.
- Duplicate sequence names; multiple matching filenames; conflicting internal sample names.
- One trace reused by many rows; manually supplied unrelated files.
- Cancellation, stale previews, very long rows and allocation-limit refusal.
- Link/delete/replace Undo/Redo, save/reopen and exported trace integrity.

Correct original-call indices are the primary assertion. A high alignment identity alone is insufficient to prove correct peak placement.

## 7. Decisions on the external code review

| Item | Decision | Implementation boundary |
| --- | --- | --- |
| D1: recovery and exceptions | Adopt checkpoint recovery and local exception containment; reject unconditional catch-and-continue | Handle exceptions inside GUI slots and transactional commands. Do not assume a terminate/bad_alloc handler can safely serialize arbitrary live state |
| D2: session corruption recovery | Defer format redesign; retain strict validation now | Later add framed, length-bounded, checksummed records and explicit partial recovery; never “open anyway” into trusted trace evidence |
| D3: FASTA open-gap output | Adopt explicit interoperability policy | Conventional FASTA exports use '-' for gaps; retain an explicit legacy/Kleiotide gap-state option and preserve existing Save round-trip behavior until deliberately migrated |
| D4: export/dirty-state confusion | Improve wording, preserve data-safety semantics | Explain which session metadata remains unsaved after export; never clear native-session dirty state merely because a FASTA was exported |
| D5: exporter overwrite bypasses | Highest-priority fix | Route every export through common path approval and atomic output handling, including preview tables and background exporters |
| D6: file-open forwarding | Add acknowledgement and timeout/failure handling | QLockFile already checks stale owners, including reused PIDs with different process names. The clearer remaining gap is that request-file creation is not an acknowledgement of delivery |
| D7: silent mask inference | Adopt import summary and explicit type review | Show nucleotide/protein/mask counts and analysis exclusions; do not guess protein type reliably from an all-IUPAC alphabet |
| P1: ID lookup index | Adopt the optimization with a safer implementation | No lazy mutation in const lookup; stage allocations and preserve index/document/history consistency on failure |
| P2: scan invalidation | Adopt selective invalidation and controlled scheduling | Point edits can be local; inserting/deleting within one row changes its aligned suffix, so O(edit-length) is not a universal guarantee |
| P3: column-accumulator memory | High priority | Bounded tiles and overview bins while preserving per-column conservation semantics; never pool all base votes in a bin before classifying conservation |
| P4: long-row storage/rank rebuilding | Measure and make small safe improvements first | Incremental rank maintenance where correct; avoid unconditional 10% reserve on huge documents; chunked storage is a later architectural project |
| P5: virtual Flush | Defer | Changes alignment/gap/export invariants; keep current guarded materialization meanwhile |
| P6: GUI cache overhead | Adopt selectively with measurements | Cache columns, structural ID sets and trace-list membership; avoid unnecessary selection-history copies |
| P7: compact transform Undo | Defer except for proven isolated cases | Inverse operations must preserve case, gaps, traces, protein metadata, annotations and exact state IDs |
| P8: FASTA parser throughput | Secondary measured optimization | Retain validation, cancellation, useful errors and import-kind semantics; test peak memory, not just throughput |
| P9: coordinate limits | Audit early and guard explicitly | Existing session fields already have limits; validate actual GUI/pixel arithmetic and import limits instead of adding an arbitrary blanket file-size cap |

### 7.1 Why the row-index patch needs changes

The performance diagnosis matches the current code: sequence(id) and mutable_sequence(id) use linear searches and are repeatedly invoked inside row-wide work.

The attached patch clears/reserves/rebuilds a mutable unordered_map, sometimes after rows have already changed, and permits rebuilding inside a const lookup. If allocation fails during rebuilding, an old true validity flag or partially rebuilt map can coexist with changed rows. Observable consistency and worker-read assumptions need stronger guarantees.

Implement an eagerly maintained ID-to-position index, with no const-path allocation or mutation. Prepare allocation work before committing structural changes, or provide tested rollback for the complete command. Cover load, session restore, append/insert/delete, reorder/move/sort and mixed structural Undo/Redo. Duplicate/unknown IDs retain clear validation errors.

Adopt the benchmark as a developer tool after review. Its Uppercase input is already uppercase, so that case measures a no-op; add a real lowercase transform. Keep giant overview runs opt-in because the current baseline itself allocates roughly proportional to sequence length.

Reproduce 20k/40k/80k-row scaling in the same optimized build, verify output/Undo equivalence, and record wall time plus peak memory. The review's “47 seconds to 44 milliseconds” is an external measurement, not a promised result on the user's laptops. Quadratic work is also not proof that earlier bulk crashes had the same cause as the already identified memory explosion.

### 7.2 Bounded overview and scan correctness

The current overview allocates one NucleotideEvidence per alignment column, then averages column conservation into display bins. Replace the full-length accumulator with bounded column tiles and a small number of aggregate bins/summaries.

Compute each column's conservation before folding it into a bin. Pooling A votes from one perfectly conserved column with C votes from another would falsely suggest variability. Preserve coverage, ambiguity weighting and no-information semantics, and compare against the old exact implementation on small alignments.

Use revision-aware cancellation and a shared scheduling budget for overview/highlights/translation. Debounce after typing and avoid restarting unrelated scans for metadata-only edits. For text edits, invalidate the correct scope: substitutions, one-row shifts, whole-column insertion/deletion, translation-frame effects and selected-scope changes differ.

For highlighting, use compact results and bounded visible-window detail rather than a heavyweight accumulator for every chromosome position. Background workers need a read-stable source; moving work to a thread alone does not solve races or copying costs.

### 7.3 Recovery design

Add periodic checkpoints under the user's application-data recovery directory, not over the original input or saved session. Suggested first behavior:
- Enabled by default; a two-minute checkpoint interval with a short idle delay.
- Track elapsed time since the last valid checkpoint, so continuous editing does not postpone recovery indefinitely.
- Retain two completed generations per document within a configurable disk quota.
- Atomic writes; do not delete the previous valid recovery before the new checkpoint commits.
- Per-document identity and lock handling; no confusion between multiple windows or source files.
- Startup Recover/Open copy/Discard choices; recovery never silently replaces the original.
- Preserve the ordinary saved/dirty state when checkpointing.
- Report unavailable/oversized/failed recovery honestly.

With the current mutable document representation, first write from a read-stable document while temporarily gating mutations and keeping progress/cancellation responsive. This avoids a second full document copy. Measure the editing pause and I/O cost for large sessions; do not claim zero-pause recovery. Copy-on-write/chunked snapshots are a later architectural improvement if measurements justify them.

Do not rely on allocating new buffers, invoking GUI functions or streaming an arbitrary document from a terminate/OOM handler. Qt explicitly limits recovery after exceptions in its own internals; exceptions in client slots must be caught before leaving the slot. Keep any fatal-path handling minimal and rely principally on previously committed recovery files.

### 7.4 Export and import details

Add an appropriate table/text format to the shared output policy, rather than pretending every TSV is FASTA merely to reuse a suffix. Resolve the final destination and protect imported/native/trace files before starting worker output. Keep cancellation and atomic replacement behavior. Test misleading suffixes, file magic, equivalent paths, existing outputs, read-only destinations and failed writes.

Separate public conventional FASTA export from internal writers and legacy Save behavior. Normalize gaps in streaming output without creating a second whole-document string. Explain that conventional FASTA cannot retain the distinction between open and locked gaps. Native sessions preserve it.

On import, provide counts and an actionable type summary when rows become masks; retain explicit protein/nucleotide/mask selection. On close after a lossy export, clearly identify the native session information still unsaved.

## 8. Code organization

Proposed additions/refactoring boundaries:
- gui/app_settings and gui/settings_dialog: typed defaults, validation, migration and UI.
- Shared mode QAction groups owned by the window or a small mode controller.
- core/trace_linking: pure matching/alignment/confidence policy, without dialogs or files.
- gui/trace_link_dialog and its table model: file inventory, assignments and async preview.
- A dedicated batch trace-metadata command in Document.
- gui/recovery_manager: per-document checkpoint scheduling and recovery discovery.
- Existing file_output policy extended for tables and adopted by all exporters.
- Incremental/bounded overview work behind the existing view-facing API where practical.

Reuse AB1 parsing, TraceLink validation, trace-coordinate rendering, session deduplication and existing output/cancellation infrastructure. Avoid a broad rewrite of MainWindow or SequenceBuffer in the same commits.

## 9. Validation and release gates

### Automated

Run the existing full 30-entry release-check baseline and add focused tests for new behavior. Add:
- ID-index invariants across structural commands, unknown IDs, Undo/Redo and injected allocation failure.
- Every exporter protecting native/AB1 sources and preserving existing output on failed writes.
- Settings migration, precedence, restart, invalid values and multi-window updates.
- Toolbar hide/restore, mode action synchronization, local-focus shortcuts and busy-state behavior.
- MAFFT version-only success versus alignment failure; auto-core error and bounded retry; output validation/cancellation.
- Trace mapping ground truth, assignment ambiguity, shared evidence, stale preview and one-step Apply/delete Undo.
- Recovery interruption, previous-checkpoint preservation, quota failure, startup recovery and no modification of the original.
- Bounded overview equivalence and memory scaling.
- Coordinate/size limits at input and GUI conversion boundaries.

Run the changed core under AddressSanitizer/UndefinedBehaviorSanitizer where available, and add randomized mapping/property tests with known original-call ground truth.

Keep performance measurements separate from brittle pass/fail timing assertions. Record optimized compiler flags, OS, CPU/RAM, input dimensions, baseline/new result and peak memory. Use correctness invariants and explicit allocation budgets in ordinary tests.

### Native Windows

Retest the original Qt 6.5.3 / MSVC2022 x64 Release kit. Repeat basic editing, groups, exports and sessions at 100/125/150% scaling; add 200% and narrow-window checks for the controls.

On the colleague's ARM machine, explicitly complete:
- Install/uninstall/reinstall and ordinary-user startup.
- Recovery of the toolbar without restart.
- Real MAFFT success and cancellation using the chosen explicit/safe thread policy.
- Group workflows and the new Settings precedence behavior.
- Forward/reverse trace attachment, edit, session round trip and selected AB1 export.
- Small-data tests before large-file stress, with measured memory and responsive cancellation.

Preserve legacy session fixtures v1–v4 and the existing AB1 recovery/export corpus. A checkpoint is not a release candidate until source-correspondence, native deployment, saved-data integrity and cancellation have evidence.

## 10. Items intentionally left for later

A new partially recoverable/compressed session format, chunked sequence storage, copy-on-write document snapshots, virtual Flush, general transform-history compression, a native ARM64 build and a WSL launcher adapter should each receive a separate design and compatibility test cycle. They are not prerequisites for correcting the reported toolbar/core-count failures or implementing a carefully bounded trace-linking workflow.

No additional user decision blocks this plan. Mapping-confidence thresholds, recovery pause limits and safe automatic thread caps require experimental calibration during implementation; record those results before fixing their final defaults.

## References

- MAFFT multithreading and physical-core automatic detection:
  https://mafft.cbrc.jp/alignment/software/multithreading.html
- Official MAFFT Windows variants:
  https://mafft.cbrc.jp/alignment/software/windows.html
- Qt 6.5 toolbar/window visibility mechanisms:
  https://doc.qt.io/qt-6.5/qtoolbar.html
  https://doc.qt.io/qt-6.5/qmainwindow.html
- Qt 6.5 persistent settings:
  https://doc.qt.io/qt-6.5/qsettings.html
- Qt exception-safety and event-loop limitations:
  https://doc.qt.io/qt-6.5/exceptionsafety.html
- Qt lock-file stale-owner handling:
  https://doc.qt.io/qt-6.5/qlockfile.html

All unmeasured implementation choices above are proposals, not claims of completed fixes or proven scientific mapping accuracy.

