# Kleiotide 0.26.6 — group-color markers in the sequence list

Group manager uses Check all / Uncheck all labels. Grouped sequence names now
have markers in their actual group's color, with group-name tooltips. Ungrouped
rows have no marker. Markers follow staged membership and color edits, including
filtered bulk changes. Counts and Apply/Cancel/Undo behavior remain unchanged.

Extract into a fresh Kleiotide_0.26.6 folder and build with your usual Qt/MSVC kit.
See TESTING_0.26.6.md for checks. Native Windows testing remains pending.

## Changes in 0.26.5

Group manager shows each group as Group name (count), including Ungrouped.
Counts follow staged membership edits and cover all names regardless of filters.
Delete empty groups removes every named group with zero staged members.
Apply/Cancel and single-step Undo/Redo retain their existing behavior.


## Changes in 0.26.4

Import groups from table now accepts wider TSV tables and provides Sequence
name column / Group name column selectors. Column numbers show header names
when First row is a header is checked. Two-column tables automatically retain
the original first-column/name, second-column/group mapping. Saved clustering
preview tables can now be imported directly using Sequence / Proposed group.
Mapping changes refresh the cancellable background preview before Apply.


## Changes in 0.26.3

Import groups from table removes surrounding parentheses and decodes standard
TSV double-quoted cells before matching sequence/group names. Internal
parentheses remain intact; escaped quotes and tabs inside quoted cells work.
The fix applies to both loaded and pasted tables and their background previews.


## Changes in 0.26.2

- One sequence per group offers **Also select from Ungrouped… → One sequence /
  All sequences**. Preview follows the group-manager order, with Ungrouped last.
- Import groups from table validates in a cancellable background job and uses
  a virtual preview table. Large malformed tables no longer trigger repeated
  synchronous cell construction and resizing. Only the latest input can apply.
- Group manager supports mouse-drag range selection with Check selected /
  Uncheck selected controls, alongside the existing filtered bulk controls.
- Clear and delete all groups requires a warning confirmation, stays staged
  until Apply, and supports Cancel and single-step Undo/Redo.

The fast clustering algorithm, session format and external dependencies are
unchanged. Native Windows/MSVC validation remains pending.

## Changes in 0.26.1

- Pairwise identity grouping shortlists up to 16 word-ranked centroid candidates
  per query (adjustable; 0 restores exhaustive search). Every candidate still
  receives a full affine semiglobal alignment. The aligner itself is faster.
- Fast search is heuristic and may choose different groups from exhaustive mode.
  No new external dependencies are required.
- Preview labels use “New group centroid” / “Group member”; Save preview… exports
  the complete table as UTF-8 TSV without applying groups.
- Automatic grouping deletes old groups that become empty. Groups still used by
  untouched/skipped rows survive. Undo restores the previous catalog/memberships.
- Group manager Select all / Unselect all acts on visible, filtered names only.

The supplied 5,555-row dataset completed in about 23 seconds in this Linux
Release environment (97% identity, 80% overlap, 16 candidates). Times vary by CPU.
Native Windows/MSVC validation remains pending.

## Changes in 0.26

- Sequence → Groups → Group by % identity… supports existing alignment columns
  or internal pairwise semiglobal alignment, configurable identity/overlap,
  group-name prefix, longest-first greedy clustering and a membership preview.
- Pairwise alignment uses rolling affine-gap DP and an exact-fragment fast path;
  no additional external dependency. Background calculation supports Stop/Cancel.
- Sequence → Select → One sequence per group… previews first, longest ungapped
  or uniformly random representatives, with an optional Ungrouped representative.
- Bottom-left status is empty without a residue selection; sequence/column counts
  remain at the upper right. Residue selection coordinates remain at bottom left.

C++20, Qt Widgets/Concurrent (Qt >=6.5), optional MAFFT functionality and
native session format 4 remain unchanged.

## Changes in 0.25.2

- Removed Previous/Next conflict buttons; conflict highlighting remains.
- Leading/trailing runs of N/n, ?, open and locked gaps provide no indel evidence.
  A gap counts as internal only when informative nucleotide calls flank it.
- Overlap choice now minimizes assumed differences first, then maximizes matching
  support. Every candidate must still meet the configured minimum. An eligible
  exact overlap wins even if a positive difference allowance is configured.
- The supplied repeat-rich example gives the same 55-base, zero-edit contig with
  allowances 0, 1 and 2. Partial contigs, direct FASTA saving, lowercase and the
  validated AB1 recovery introduced in 0.25.1 are retained.

## Changes in 0.25

- Whole-row aligned merging with informative overlap/conflict counts, IUPAC
  unions and explicit internal gap conflicts.
- Optional highest-original-Q resolution when all selected rows have traces;
  configurable advantage with fallback for ties, missing Q and edited calls.
- Exact-overlap search with a configurable informative minimum, contained reads,
  chaining, placement preview, ambiguous-repeat detection and no mismatch rescue.
- Background cancellable previews, stale-plan protection, custom naming,
  one-row append and single Undo/Redo. Original rows/groups/traces stay intact.
- New merge unit suite, end-to-end Qt workflow check and Windows acceptance
  fixtures. No native session or AB1 export format change.

## Changes in 0.24.3

- Drag the preview header divider to change source-name/output-filename widths.
- Output filenames stretch into remaining space; the source-name column shrinks
  first in narrow windows. Widening restores your preferred source-name width.
- Scope/suffix changes preserve the divider preference. Export behavior is unchanged.

## Changes in 0.24.2

- Batch export has a Filename suffix field, defaulting to _edited. Enter your own
  separator/suffix (for example _trimmed), or leave empty for no added suffix.
- The filename preview updates immediately. Invalid filename characters become
  underscores; existing collision/overwrite protection applies after the suffix.
- Suffix editing is disabled while exporting so reviewed names stay fixed.
- AB1 call/signal export and session formats are unchanged from accepted 0.24.1.

## Changes in 0.24.1

- AB1 directory moved after every external payload, preventing BioEdit's legacy
  tag scanner from treating comment/sample/signal bytes as directory entries.
- Ordered legacy DATA entries, required metadata placeholders and bounded Pascal
  strings. Current calls, peak positions, Q and analyzed samples remain exact.
  Placeholder dates are clearly identified, not inferred acquisition dates.
- File → Batch export edited traces (AB1)… offers All traces or Selected sequence
  names only, current-title filenames and a preview. Duplicate names and existing
  files get numbered suffixes; later conflicts never overwrite anything.
- Per-file background preparation/writing, cancellation, progress and a complete
  success/failure report. Completed files stay on Cancel; incomplete outputs are
  removed. One failed trace does not stop the rest. Sessions/Undo remain unchanged.
- BioEdit 7.7.1's unmodified parser exercised in x86 emulation, plus independent
  Biopython checks. Native Windows UI acceptance remains necessary; see the
  known short-call/unsigned-signal limitations in AB1_EXPORT.md.

## Changes in 0.24

- File → Export edited trace (AB1)… writes an edited copy from any linked row,
  including from a portable session whose original AB1 file is unavailable.
- Current ungapped sequence in both call sets; inserted call positions are
  interpolated, changed/unscored calls receive Q0 and unchanged measured Q stays.
- Exact analyzed signal samples and retained original peak positions. Current
  reverse complement reverses/complements channels and transforms peak positions.
- Read-only preflight, cancellable background preparation/writing, source-file
  protection and atomic destination commit. Source edit history stays unchanged.
- Independent validation: Biopython, Staden io_lib and Kleiotide; details and reader
  limits in AB1_EXPORT.md. Original instrument metadata is not reconstructed.
- Native format 4 and application dependencies are unchanged. Reliability and
  installer preparation remain the next planned stages.

## Changes in 0.23

- Sequence → Collapse identical sequences… with All/Selected names scope,
  read-only preview and optional mask inclusion. Identity ignores gaps and case,
  compares complete strings and keeps row types separate; ambiguities are literal.
- Select any representative per set and inspect its gap pattern, group and trace.
  Its ID, original data and evidence remain; other members are removed.
- Combine complete original titles with an editable separator, or append the
  set count to the representative title: name_x13 for 13 current members.
- One Undo restores all rows/titles/groups/traces. Background matching/title
  preparation is cancellable; preview tables are lazy. Native format stays 4.
- Residue compression is removed from the roadmap. Next: reliability/release
  preparation, then the Windows installer and release candidate.

## Changes in 0.22

- Protein MAFFT for whole alignments, selected sequences, selected columns, and
  full/fragment target addition to templates, using the existing commands.
- Explicit molecule flags, BLOSUM/JTT/transmembrane scoring controls and shared
  strategy/gap/composition settings. Nucleotide-only controls are hidden for AA.
- Mixed participants are rejected before launch. Homogeneous subsets and template
  jobs can operate inside mixed documents; masks remain outside MAFFT.
- Case, titles, IDs, groups, protein provenance and non-target trace evidence
  survive. Template jobs retain all original columns and target insertions.
- Unknown amino-acid and stop symbols are preserved and their MAFFT scoring
  treatment is explained in the settings dialog/report. Results open separately.
- Similarity sorting explicitly explains that N/?/other ambiguities, gaps and
  missing cells are excluded from both difference and comparable-site counts;
  N–N does not count as a match. Sorting calculations are unchanged.

## Changes in 0.21.1

- Overall, within-group and between-group means show SD (pairs) and SE (naive).
  With m valid pair distances, sample SD uses divisor m−1; naive SE is SD/√m.
  Both are N/A for fewer than two valid pairs. Excluded pairs never enter them.
- Formulas and the independence limitation appear in the dialog, tooltips,
  TSV comments and DISTANCES.md. Shared sequences make pair distances dependent;
  the naive SE is not a bootstrap or dependence-adjusted uncertainty estimate.
- Distance and nucleotide-composition Copy/Export TSV now identify the exact
  Kleiotide build version. CMake supplies the application/export version centrally.
- SD is accumulated in one streaming pass; no full matrix is required for means.

## Changes in 0.21

- Statistics → Pairwise distance matrix, Overall mean distance, Within-group mean
  distance and Between-group mean distance, with Copy TSV and atomic TSV export.
- Nucleotides: observed differences, p-distance, Jukes–Cantor, Kimura 2-parameter.
  Proteins: observed differences, p-distance, Poisson correction.
- Explicit molecule, row and column scope, pairwise/complete deletion, minimum
  comparable sites and ungrouped inclusion. Ambiguous/missing states are excluded;
  unavailable estimates show N/A with reasons and denominators.
- Sequence → Sort → By decreasing similarity… uses a deterministic nearest-neighbor
  chain from the first eligible row. Preview/Cancel or Apply with one Undo step.
  Ineligible rows retain their slots; groups, IDs and trace evidence are preserved.
- Cancellable background work, lazy tables, bounded triangular matrix storage,
  streaming means/export and resource estimates. No native schema change.

## Changes in 0.20.3

- Generate a `translation_mask` row from the current annotation; apply an existing
  or pasted mask through Alignment → Translation → Annotate from mask….
  Mask-name context menus offer Annotate using this mask. Both operations undo.
- Mask digits 1/2/3 specify codon phases; all other symbols specify noncoding.
  Short masks pad with noncoding columns; longer masks are rejected.
- Incomplete trailing codons are translated using unknown missing bases (GC → A,
  AT → X). Display, stop counts, Copy and protein snapshots use the same calls.
- Edit → Rename can append/prepend group names, with scope, separator and preview.
- Sequence-type sorting is stable: Mask, Protein, Nucleotide.
- Custom annotation label, separate translation and gap command sections,
  Select/Sort at the bottom of Sequence, Motif search in View, and font last.

## Previous translation refinements (0.20.2)


- Single-letter AA display and output only; no three-letter controls, encoding
  selector or three-letter text command. Translation toolbar controls are spaced.
- Add translated sequences now creates ordinary protein data such as ~M~~K~~*~.
  Letters occupy middle codon positions; surrounding gaps are ordinary open gaps.
  They can be selected, edited, cut/pasted and slid independently.
- Older bound-span translated rows migrate to centered ordinary proteins on load,
  retaining source/code/frame provenance. Imported ordinary protein data stay put.
- All 20 standard amino acids have distinct shades within their existing families.
  Letter/background contrast is adapted to darker and brighter shades.
- Copy translated fragment accepts incomplete codon selections and visibly expands
  to complete codons when invoked. The separate expansion action is removed.
  Ordinary Ctrl+C retains the exact selected underlying text without expansion.
- Native format remains 4; current and earlier 0.20.x sessions remain readable.

## Changes in 0.20.1

- Fixed residue-based amino-acid colors in imported proteins and translated codons.
  Gap-heavy and variable columns no longer leave amino acids uncolored. Ambiguous
  amino-acid symbols are gray; gaps stay uncolored. Monochrome remains available.
- Read-only genetic-code selectors with concise labels and full-name tooltips.
  Custom code… is the final dropdown command, including in snapshot previews;
  Cancel preserves the selected code. Preview-only codes do not change defaults.
- A colored L cell above Leu identifies the one-/three-letter display switch.
  Set selection coding is a wrapped rectangular control.
- Copy translated fragment is enabled only for a complete, eligible single-row
  nucleotide selection while the translation layer and current result are active.
  Both Alignment and fragment context menus show its availability.
- Native sessions remain format 4, compatible with version 0.20.

## Changes in 0.20

- Translation toolbar: codon coloring, same-width one-/three-letter AA preview,
  coding ruler, coding/noncoding annotation controls, 27 NCBI tables, custom code
  import/editor, frames and detailed live stop/uncertainty counts.
- IUPAC consensus, mixed-gap flags, split codons, partial boundaries and contextual
  stop codes share one engine for display, counts and protein snapshots.
- Add translated sequences below selected sources with preview, unique names,
  group inheritance, provenance and one-step Undo. Compact protein window/export
  and explicit three-letter text output are available separately.
- Canonical amino-acid tokens with structural span metadata protect protein
  editing from interpreting three-letter labels as three residues.
- Native format 4 retains translation settings, annotations, protein layouts and
  provenance. Typed whole-sequence clipboard retains this metadata.
- Translation/protein profiles calculate in cooperative revision-checked slices;
  selection and scrolling reuse completed tiles and counts. Trace caches remain.

## Changes in 0.19.4

- Chromatogram signal, imported labels and current labels are cached independently
  of the selection/caret overlay. Extending a selection does not redraw the signal.
- Trace positions and original-call mappings are indexed once per row edit.
  Sample-spacing labels are restricted to the visible sample interval without
  repeatedly scanning each residue prefix. Inserted and overlapping calls remain
  visible with the same mapping, strand and quality semantics.
- Trace switching uses an ID lookup; cache invalidation covers edits, Undo/Redo,
  scroll, zoom, gain, spacing, palette, font, resize and DPI changes.
- Native session format remains 3. Windows checks: TESTING_0.19.4.md.

## Changes in 0.19.3

- Undo and Redo reveal the affected row and column range for residue edits, even
  after scrolling away. Selection restoration is retained. Renaming, grouping and
  reordering do not cause residue-navigation jumps.
- Dragging a selection repaints changed cells and outlines instead of the entire
  viewport. Unchanged selections skip notifications; row/tile work is bounded to
  the dirty area, and translucent glyphs are drawn once per tile.
- Selection updates refresh clipboard actions, status and trace following without
  refreshing document panels/toolbars or copying selected substrings repeatedly.
- Session format remains 3. Details and Windows checks: TESTING_0.19.3.md.

## Changes in 0.19.2

- Upper-right counts yield to the editing/display controls when the window is
  narrow, and return automatically when sufficient width is available.
- Mouse-slide Undo/Redo restores the selected rectangle with the residue block.
  Selection state is independent of drawing-cache resets, and drag interaction
  ends before committing the edit. Keyboard sliding keeps its behavior.
- Ctrl+Shift+C / X / V invokes whole-sequence Copy / Cut / Paste. The bindings
  appear in Edit and sequence-name context menus. Plain Ctrl+C / X / V remains
  fragment-only, independent of selected sequence names.
- Reported MSVC narrowing/shadowing warning sources are corrected without warning
  suppression. The state comparison uses unsigned, clipboard/trace locals have
  descriptive names, and the existing selection reference is reused.
- Native format stays 3. Acceptance and warning details: TESTING_0.19.2.md.

## Changes in 0.19.1

- Two segmented switches: Edit/View and Rewrite/Insert, with spacing before the
  Right click selector. Rewrite replaces the Overwrite name. There is no separate
  Slide mode: dragging selections, Shift+Left/Right (one column), and Ctrl+Left/
  Right (adjacent open gaps) work in both edit submodes. Plain arrows move the caret.
- Movement commands form a separate context-menu group in both edit modes. Menu
  commands show their available shortcuts, including native context hints. Fragment
  Ctrl+C/X/V stays independent of selected sequence names; whole-row clipboard
  commands keep their explicit menu/context actions.
- Selected-name highlight scope shades only the captured analysis rows. Snapshot
  inputs stay independent of subsequent name/fragment selection changes.
- Alignment totals stay visible at the upper right, including during selection.
  Counts update after edits, row changes and undo/redo; the status bar keeps the
  selected-range information.
- Native format remains 3. Old Slide mode (numeric 3) migrates to Rewrite; old
  Overwrite (numeric 2) is Rewrite. View retains its remembered edit submode.
- Windows acceptance: TESTING_0.19.1.md. Next milestone remains protein coloring
  and translation; this release implements the requested editor refinements.

## Changes in 0.19

- Highlight menu: conserved, variable, parsimony-informative and singleton sites;
  inclusive percentage ranges for coverage and conservation; custom color and
  Clear. Multiple enabled rules combine by union. Pale violet shading and ruler/
  edge cues remain visible in every display style; residue selection has priority.
- Explicit analysis scope: nucleotide or protein, all rows or selected-name
  snapshot, all columns or selected-column snapshot. Masks never contribute.
  Scope stays independent of subsequent selection changes. Native sessions retain
  scope, ranges, rules and color, without a binary format change.
- Statistics menu: the existing summary moves here as Alignment statistics.
  Nucleotide composition adds per-row and pooled counts/frequencies, optional
  fractional IUPAC handling, explicit row/column scope, missing/gap accounting,
  Copy and UTF-8 TSV export. Calculation is asynchronous and cancellable.
- Shared site summaries preserve existing statistics counts and overview voting.
  Compatibility-conserved sites differ from strict 100% conservation; the precise
  definitions and denominators are documented in SITE_ANALYSIS.md.
- Example and Windows acceptance: examples/site_highlight_demo.fasta and
  TESTING_0.19.md. Protein presentation/translation is the next roadmap milestone.

## Changes in 0.18

- Edit → Trim low quality ends and Low-quality calls to N open a settings and
  preview dialog. Scope is explicit: selected sequence names, or all trace-linked
  nucleotide rows. An empty name selection never silently means all. The preview
  reports proteins, masks and unlinked rows as ineligible.
- Trimming uses arithmetic mean Phred Q across complete windows of current
  non-gap symbols. Keep the interval between the first and last windows with
  mean Q >= threshold; internal low-quality islands stay. Only trimmed residues
  become open gaps (~): row length, alignment columns and pre-existing gap states
  are retained. No complete passing window means no change to that row.
- Low-quality calls to N replaces only calls with Q strictly below the threshold,
  preserving positions and letter case. Existing N/n and all gaps stay unchanged.
- Quality is read through original-call mappings, including reversed traces and
  aligned/gapped rows. Missing Q is not Q0. Inserted calls always remain protected.
  Substituted symbols are protected by default, including Ns from earlier cleanup.
  Explicitly enable original-call Q to evaluate them; it remains original evidence
  rather than a new measurement for the current symbol. This policy also applies
  to trimming: the retained interval expands to include every protected call, and
  passing windows require usable Q at every position.
- Preview calculation runs asynchronously and supports Cancel. Settings changes
  clear the old preview and disable Apply. Source editing remains locked while
  preview/cancellation is running. Apply is one atomic undoable change across all
  affected rows; original signals, base calls, qualities and group metadata remain
  intact. Save .kleio to preserve edits and trace evidence. Format remains 3.
- Defaults: Q20 and a ten-symbol trimming window. The last applied threshold and
  window are remembered; original-Q inclusion starts unchecked each time.
- examples/trace_quality_demo.kleio is entirely synthetic and needs no external
  AB1 files. See TESTING_0.18.md for known-result settings and expected edits.

## Changes in 0.17.4

- An eight-pixel logical spacer separates the Edit/View segments from the
  Overwrite/Insert/Slide dropdown; it scales with display DPI.
- Delete selected sequences is grouped with Cut/Copy/Paste sequences in both
  Edit and the sequence-name context menu. Ctrl+Delete retains its meaning.
- Group identical sequences now explains that the complete sequences must match
  after gaps are removed. A shorter sequence matching only a substring of a
  longer sequence is not grouped with it. The grouping algorithm is unchanged.

## Changes in 0.17.3

- The mode switch has two joined, permanently visible Edit and View segments.
  The active segment is blue with white text; the other remains neutral. Click
  the desired segment to choose a mode. Clicking the active segment keeps it
  selected. Both are native keyboard-focusable buttons. New windows still start
  in Edit/Overwrite, and switching back from View restores the last editing mode.
- In View, both the editing-mode and right-click dropdowns are inactive. The
  right-click dropdown shows Menu; returning to Edit restores its previous choice.
- Sequence → Lock gaps in selection and the fragment context menu convert only
  open gaps (~) inside the selected rectangle to locked gaps (-). Residue positions,
  row lengths, outside cells and original trace evidence remain intact. Single-row
  and multi-row selections work in Overwrite, Insert and Slide, independently of
  name selection. Unlock is available beside Lock. Each operation is undoable.
- Native session format remains 3; existing saved View/Edit states retain their
  meaning. No data-format migration is required.

## Changes in 0.17.2

- The left toolbar segments select View/Edit. New windows start in Edit with
  Overwrite. The dropdown to its right contains Overwrite, Insert and Slide.
  Switching to View retains the last edit mode, disables its dropdown and limits
  right-click behavior to Menu (its dropdown is disabled since 0.17.3). Returning
  to Edit restores the last edit mode and
  right-click choice. Saved sessions retain View/Edit and the remembered edit mode;
  existing saved mode values keep their meaning.
- Ctrl+C/X/V and Edit → Copy/Cut/Paste fragment always act on residue selection,
  independently of selected sequence names. They never copy, delete or import
  entire rows. FASTA and native whole-sequence payloads require Paste sequences.
- Edit and the name-area context menu provide Cut selected sequences, Copy
  selected sequences and Paste sequences, without Ctrl+C/X/V shortcuts. Sequence
  copies remain unwrapped FASTA with native type/group/trace metadata. Paste
  inserts after the last selected name or appends with no names selected.
- The context menu on a selected fragment provides Copy in every mode. Cut is
  available in Insert/Overwrite on one row. Paste substitutes a one-row span in
  Overwrite, or inserts after a single selected symbol in Insert. Multi-row
  fragment clipboard operations remain disabled. Shortcuts also support the
  existing unambiguous caret/end boundary in Insert/Overwrite.
- Sequence → Unlock gaps in selection and the fragment context action convert
  only locked dashes inside the selected rectangle to open gaps. Residues and
  outside cells retain positions; short rows are not extended. Names do not
  determine affected rows. The operation is undoable and requires Edit mode.
- Group identical sequences now ignores both open (~) and locked (-) gaps, as
  well as case. Rows of different types remain separate. Duplicate clusters get
  groups, singletons become Ungrouped, and previous grouping is replaced with
  one undo step. Row order is retained.

## Changes in 0.17.1

- Sequence → Select adds Nucleotide sequences, Protein sequences, Masks and
  Sequences with traces. These replace the name selection independently of residue
  selections. The trace option uses embedded links, even if the source AB1 moved.
- Group swatches are ten pixels wide, with extra name padding.
- Sequence → Groups → Group identical sequences replaces the old identical sort.
  It creates groups for duplicate strings (case insensitive, gaps ignored since
  0.17.2, separate row types). Singletons become Ungrouped. Existing
  groups/memberships are replaced in one undoable action; row order is retained.
- Alignment → MAFFT → Align selected sequences opens only the selected rows in a
  new alignment window. Unselected rows never participate. Selected masks remain
  excluded from MAFFT and are retained in the new document; selected proteins are
  rejected until protein MAFFT is implemented. Types, groups and traces survive.
- Both target-to-template MAFFT workflows offer fast/accurate Adjust strand.
  Only targets may reverse; every template strand and original column is checked.
  Reversed targets retain case, IUPAC/RNA symbols and trace provenance. A malformed
  result or attempted template reversal is rejected with the source unchanged.
- Residue Cut/Paste is single-row only, including rejection of 0.17 block payloads.
  Multi-row selection, sliding and Delete/Backspace editing remain available.

## Changes in 0.17

- Sequence → Groups → Manage groups creates/renames/deletes groups, chooses RGB
  colors, edits members and sets group order with the arrow buttons. Changes are
  staged until Apply, then committed in one undo step. Each row has one group or
  Ungrouped; masks and proteins may belong too. Membership uses stable row IDs.
- A color swatch beside sequence names stays visible during selection. Name
  tooltips identify the group. Right-click a sequence name for Add to group or
  Remove from group; an unselected clicked name becomes the action target.
- Sequence → Select → Groups chooses any combination of groups and Ungrouped,
  preserving the independent residue selection. Sequence → Sort → By group uses
  manager order, stays stable within groups, and puts Ungrouped last.
- Import groups from table accepts complete title<TAB>group name, with a header
  checkbox and preview. Exact full titles are matched; unmatched titles are
  reported/skipped. Duplicate alignment titles require explicit permission to
  assign all matches; conflicting assignments block Apply. Empty group means
  Ungrouped. No edits occur until a valid preview is applied.
- Native format 3 retains group IDs, names/colors/order, memberships and the next
  group ID, with retained readers for formats 1 and 2 (old rows are Ungrouped).
  Renaming, reordering, row deletion/undo and MAFFT result creation retain groups.
  Normal Save prefers .kleio when group data is present.
- Edit → Cut selected sequences cuts names selected in any mode. Paste sequences
  inserts them after the last selected name, or appends with no names selected.
  Existing rows are retained. Each Cut and Paste is separately undoable.
- Kleiotide sequence copy/cut supplies ordinary unwrapped FASTA text plus a native
  clipboard payload. Between current Kleiotide windows this preserves types,
  group colors/membership and embedded trace links. A matching group name/color is
  reused; a different-color name collision gets a numeric suffix. Clipboard row
  IDs are remapped to fresh destination IDs. File → Add from clipboard appends.
- Residue Cut/Paste works only on a single row in Insert/Overwrite, regardless
  of selected names. A multi-row fragment selection changes neither clipboard nor
  alignment on Cut/Paste. Older multi-row residue payloads are rejected even at a
  single-row caret. Copy retains the same single-row restriction.
  Fragment Cut leaves an insertion
  boundary at the deleted stretch; Insert paste inserts and Overwrite substitutes.
  Pasted text creates edited/inserted calls, without new measured qualities.
- Ctrl+wheel horizontal speed doubles: six columns per normal wheel notch instead
  of three; the existing pixel-wheel step doubles too. Ordinary vertical and
  native horizontal wheel behavior retains its existing step.

Native clipboard payloads are limited to 256 MiB; an over-limit sequence Cut is
rejected before deletion. TSV file loading is limited to 16 MiB. Plain FASTA and
plain text do not carry group/type/trace metadata. Ctrl+C retains its existing
single-row residue-fragment behavior; whole-sequence copying now also preserves
native metadata when pasted within Kleiotide.

Next: site highlighting and descriptive statistics. Protein presentation,
translation and protein MAFFT remain later roadmap stages.

Source archives use fixed historical ZIP dates to avoid CMake timestamp loops.
Python is only needed to run tools/package_source.py, not to build the application.

## Changes in 0.16

- File → FASTA import type chooses Nucleotides + masks (the unchanged default),
  Proteins + masks, or Masks only. It applies to Open, Add and FASTA clipboard
  imports in this window. ACGT may also be protein: there is no reliable alphabet-
  only inference, and .faa alone does not override the explicit choice. AB1 and
  saved sessions retain their known/stored types.
- Add new sequence has its own type selector. Sequence → Set sequence type…
  applies to selected names atomically and supports one-step undo/redo. It does
  not translate or rewrite symbols. Invalid alphabets reject the whole operation;
  trace-linked nucleotide rows cannot be retyped to protein/mask.
- Small N/P/M badges identify types beside names; hover explains the type. Protein
  rows currently render in neutral gray, support the shared editing/selection/gap
  tools, and never get nucleotide coloring. Exact identity dots compare matching
  types only. Proteins/masks do not vote in nucleotide search/overview/statistics.
- Protein storage uses one-letter codes. It accepts the standard amino-acid
  letters plus B/Z/J/X/U/O, stop *, unknown ?, and gaps; case is preserved and
  dot becomes an open gap. Explicit invalid protein input from FASTA becomes a
  mask, matching the existing unexpected-symbol mask policy. Typed editing and
  type assignment validate strictly, with no silent type changes.
- Normal Save prefers .kleio for proteins, trace-linked rows and explicitly forced
  masks that would look like nucleotides in FASTA. Export FASTA remains unwrapped,
  canonical sequence text. Plain FASTA has no type metadata; choose the desired
  import type when reopening it. Native sessions preserve types, canonical
  representation, row IDs/order, current symbols, view and embedded trace data.
- Format 2 has mandatory type/representation fields with a retained format-1
  reader. Legacy masks stay masks, including protein-looking ones. Unsupported
  encodings/types reject clearly rather than being interpreted as ordinary
  amino-acid letters. Reopening still starts a fresh undo baseline.
- Current MAFFT remains nucleotide-only. Protein-containing whole/column inputs
  and selected protein targets are rejected before launch. Unselected protein
  rows in a nucleotide target/template job retain their original projected axis,
  types and symbols just like other non-target rows.

The next milestone is persistent sequence groups. Clustal-style protein coloring,
translation/codon-positioned representations and protein MAFFT remain planned at
stages 6 and 8; see ROADMAP_1.0.md. Group metadata is not introduced in this release.

All ZIP members use a fixed historical date to avoid CMake timestamp regeneration
loops. Create source archives with `python tools/package_source.py OUTPUT.zip`;
Python is needed only for packaging.

## Changes in 0.15.2

- In residue-background mode, every motif hit uses a bright lavender fill, dark
  letters/dots and a purple outline with a white halo. Other display modes retain
  the light shade. The current editing selection keeps its blue/yellow fill.
- Exclude N-only matches is optional (initially off). It checks the actual matched
  interval, ignoring gap symbols and letter case: all remaining symbols must be N
  for exclusion. Mixed residues/ambiguities and gap-only matches remain. It applies
  to literal/regex searches and both strands before counting/indexing results.
- Find and Next/Previous already select each visited match, so the Select match
  button is removed. Filtering uses constant-time interval queries over a bounded,
  cancellable per-row bitmap; long overlapping N runs are not repeatedly scanned.
  Search/filter settings remain transient; the native session format is unchanged.

## Changes in 0.15.1

- The search panel adds Regex: case-insensitive Perl-style syntax including
  classes, groups, alternation, repeats, anchors and captures. Regex is matched
  against each searched orientation, not reverse-complemented as pattern text.
  Dot is a wildcard; ~/- are literal gaps when gaps are included. Compatible
  IUPAC is disabled in regex mode; explicit classes retain standard regex meaning.
- Find scans the scope once, selects/shows the first match, shades/outlines all
  other intervals and reports Total/Forward/Reverse complement/Both counts.
  Total counts distinct aligned intervals; both-strand intervals contribute to
  both orientation counts. Empty regex matches are ignored and reported.
- Next/Previous/F3 use a compressed index and automatically select each match.
  Name selections remain independent. All-hit highlights survive other cell/name
  selections and unfinished/failed replacement searches until a successful Find
  or panel close. Document revisions clear invalid coordinates with a refresh
  message. Normal editing fills retain their blue/yellow colors.
- Arithmetic runs, varint encoding, ordinal checkpoints and visible-column
  coverage avoid expanding dense hits during navigation/painting. Cancellable
  workers and engine/index limits reject failures without partial counts/results.
  Limits and examples are documented in TESTING_0.15.1.md. No new dependency or
  native session format version is introduced.

## Changes in 0.15

- FASTA Save/export and sequence clipboard copying write each complete sequence
  on one line. Reading arbitrary wrapping retains its existing behavior. A shared
  record writer preserves case, gap symbols, empty rows and masks.
- Edit → Find motif… (Ctrl+Shift+F) opens an optional dock. Find starts at the first
  hit; Next/F3 and Previous/Shift+F3 traverse all overlapping hits in current row
  order and column order. Ctrl+F retains name-regex selection.
- Options search all nucleotide rows or selected names, ignore or match exact gap
  types, include reverse complements, use IUPAC compatibility and wrap around.
  The result reports the row/title, actual aligned columns, strand and wrapping.
  Both-strand matches at one interval are coalesced. Masks are excluded.
- An independent purple outline with white halo marks the current hit without
  changing names, residue selection, caret or mode. Select match explicitly makes
  that interval the editing selection. All display styles retain their selection
  colors. Closing the dock clears the search outline.
- Searches use a cancellable worker and temporarily lock document editing. They
  retain one hit and motif-sized matching state, not an alignment-sized copy or
  complete hit list. Edits/reordering/undo, document replacement and query/scope
  changes invalidate the cursor. Queries/results are transient, not native data.

## Changes in 0.14.1

- Classic is now named Kleio. Its saved `classic` identifier is retained so
  existing sessions restore the same default colors.
- Chromas-style adds exact A #008040, C #0000FF, G #000000, T/U #FF0000 inks
  and cell backgrounds. It also controls trace channels, legend and palette icons.
- The overview explanation reads "Color: conserved column (blue) -> variable
  (orange), gray = no information" in its header and tooltip.
- ROADMAP_1.0.md records completed milestones and the remaining functional,
  reliability, Windows installer and release-candidate stages.

## Changes in 0.14

- View → Residue palette offers Classic (the current colors), High contrast,
  Okabe–Ito and Paul Tol bright alternatives. A compact swatch dropdown beside
  the view buttons shares these actions. Classic remains the default.
- View → Residue display selects colored letters, monochrome or colored cell
  backgrounds. Two compact toolbar toggles provide monochrome and backgrounds;
  clicking the active toggle again restores colored letters. All controls have
  hover help and stay synchronized with the menus.
- Background cells choose black or white lettering for contrast. Selections keep
  their established blue/yellow cues and use dark letters over those fills.
  Identity dots preserve the underlying residue background; copied/exported
  symbols remain actual letters. Masks remain neutral dark gray on white.
- Palettes also update trace-channel colors, the trace legend and toolbar icons.
  Monochrome/background modes apply to the alignment grid. Overview conservation
  retains its independent blue/orange scale. See DISPLAY_COLORS.md for sources,
  exact base colors and contrast adaptations.
- Palette and display mode persist in optional .kleio view settings. Older or
  unknown settings default to Classic colored letters. View changes preserve
  sequences, editing selection, undo history and original trace evidence.
- Cached rendering includes selection glyph tiles for colored backgrounds. A
  fixed selection-edge strip is repainted after horizontal scrolling to avoid
  stale border pixels for wide blocks.

## Changes in 0.13.1

- Alphabetically (sorting by sequence symbols) is removed. The other sorting
  methods, including By title, remain available.
- Edit → Rename → Sanitize names uses the existing preview and one-step undo.
  It defaults to all rows even when names are selected; the explicit scope can
  be changed to selected rows. Outputs contain only ASCII A–Z/a–z, 0–9 and '_'.
  Runs of unsupported characters become a single underscore; newly normalized
  names lose outer underscores and fall back to `sequence` if nothing remains.
  Non-ASCII characters are replaced rather than translated or transliterated.
- Existing safe names are preserved where possible. Exact and case-only duplicates
  receive `_2`, `_3`, ... suffixes, skipping occupied names and all reserved output
  bases. Valid names such as A_B or A_B_2 are not taken by an earlier unsafe title
  that normalizes to the same base. Safe unique numeric or underscore-only names
  remain valid and unchanged. Running sanitization twice is a no-op.
- These are the requested character/uniqueness rules; no name-length truncation
  is performed. Preview/Cancel, selected scope, undo/redo, IDs, sequence symbols,
  mask kinds, traces and native/FASTA persistence retain existing behavior.
- The next display stage is queued: selectable high-contrast/color-blind-friendly
  residue palettes, monochrome view, and residue-colored backgrounds with automatic
  black/white lettering for contrast, accessible through View and toolbar controls.

## Changes in 0.13

- The first-nongap-in-selected-columns option is removed, as requested. Other
  selection tools retain their behavior. The identity-view toolbar icon now
  depicts tiny A/C/G/T letters in their nucleotide colors with dots below.
- Edit → Rename gains direct entries for space/underscore exchange, prepend and
  append text, literal case-sensitive replacement, appending a filename stem,
  reversing names, stripping numeric suffixes, making names unique, names from
  row numbers, aligned lengths and true lengths, and appending taxonomy/text
  using a title-matching table. These share a wide preview dialog.
- The dialog starts with selected rows when names are selected, otherwise all
  rows. Scope is explicit and can be changed. Preview lists current/new names,
  changed/unchanged counts and duplicate resulting names; duplicates are marked
  in amber and remain allowed for operations where duplicate names are intended.
  Editing any option clears the preview and disables Apply until previewed again.
  Cancel/no-op/error leaves data/history unchanged. Apply is one undoable rename.
- Make names unique compares complete titles exactly, preserves the first when
  possible and adds separator + 2, 3, ... to later copies. It avoids every original
  title and unselected name; existing `_2` names are never overwritten/reused.
  Strip numeric suffixes removes only the chosen separator followed by trailing
  ASCII digits. Preview makes any resulting collisions explicit.
- Reverse operates on Unicode grapheme clusters, preserving emoji/combining
  marks. Space conversion affects ordinary spaces. Replace is literal, without
  regex/backreference interpretation. Names remain nonblank single-line headers.
- Row-number names use current alignment row positions, starting at 1, including
  unselected rows. Aligned length includes both gap types; true length excludes
  them and works on masks. An optional prefix can accompany numeric names.
- Filename text is prefilled from the current alignment/session filename stem
  when available, can be edited, and is appended with the chosen separator.
  Untitled documents require entering the desired filename text.
- The existing old-name/new-name table operation also gains preview. Matching
  uses exact complete original titles and is simultaneous, without cascading.
  The preview reports unmatched keys and keys matching several rows in the chosen
  scope; all those matching rows receive the mapping. Conflicting/malformed table
  entries or blank results prevent Apply. Taxonomy/text tables use the same key
  rule and append the supplied value with the chosen separator. No external
  taxonomy service is queried. IDs, symbols, kinds, traces and selections remain
  unchanged; renamed titles persist through native saves and FASTA export.

## Changes in 0.12

- Three compact icon-only toggle buttons follow the editing and right-click
  dropdowns: identity dots, chromatogram peaks and overview bars. Each has hover
  help, an accessible name and a visible checked state. Buttons and View menu
  items share the same actions; closing a panel or restoring a session updates
  both. The chromatogram button is disabled until linked traces are available.
  Icons are drawn natively at multiple DPI scales; no icon dependency is added.
- Sequence → Select adds title-prefix, title-substring and exact-title-list
  matching, one entry per line. Blank lines are ignored; other characters and
  spaces are literal. Match case is on by default and can be disabled for Unicode
  case-insensitive matching. Exact-title lists use a lookup set for large lists.
- Duplicate names can compare the entire title or its trimmed prefix before the
  first '(' or '|'. Choose all members of each duplicate group or only additional
  copies, keeping the first in the current row order unselected.
- Duplicate sequences compare without regard to case. Ignore gaps is on by
  default; turning it off compares exact aligned symbols and gap types. Masks
  group only with masks. All-gap and empty rows follow the same comparison rule.
- The original first-nongap-in-columns selector was removed in 0.13 at the user's
  request.
- Sequences containing a motif searches nucleotide rows, without regard to case.
  Ignore gaps is on by default; reverse-complement matching and compatible IUPAC
  matching are optional. Exact mode matches literal ambiguity codes; compatibility
  tests shared possible bases (N/? may match any base, T/U are compatible).
  With gaps included, their exact type/position still matters. Masks are excluded.
  Whitespace is ignored in the query; motifs are limited to 4096 searchable symbols.
- Random subset selects an exact requested count from all rows without replacement.
  Zero clears name selection. Results retain the current row order.
- Selection searches run in a worker and replace name selection while preserving
  residue selection. Cancel/invalid input leaves selection unchanged; no matches
  clears name selection. Searches and view toggles do not modify sequence data,
  trace evidence or undo history. Ctrl+F retains name-regex selection.

## Changes in 0.11

- Backspace in Insert/Overwrite deletes the selected cells and selects the symbol
  before the deleted stretch. At the beginning it stays at column 1; an empty row
  has an end caret. Delete keeps selecting the following symbol after suffix
  shifting. Multiple-row deletion remains one atomic edit with no ambiguous
  single-row caret. Both views use the same editing controller and undo.
- Sequence-name selection and residue selection are independent. Clicking or
  horizontally dragging residues selects only that row. Dragging vertically or
  Shift-clicking selects the actual rectangle, including intervening unselected
  rows. Ruler selection covers every row, including masks. Selecting/deselecting
  names preserves the residue selection; selecting in Current preserves names.
  Esc still clears both. Existing Ctrl+C priority remains: selected names copy
  FASTA; with no selected names, a single-row substring copies real text.
- View → Identity dots to top sequence is a render-only toggle. Case-insensitive
  exact nongap symbol matches to the current first row appear as neutral gray
  dots below it. The top row, gaps, differing symbols, mask rows and columns beyond
  the reference retain their symbols. Ambiguities match only the same ambiguity
  code; T/U are distinct. If the top row is a mask, no comparison is made.
  Reordering, editing and undo refresh the comparison. Copy/export, statistics,
  MAFFT and trace evidence retain real symbols. The toggle persists in .kleio
  as optional version-1 view metadata; old sessions default to letters.

## Changes in 0.10

- Current, Imported and Original Q labels have opaque white backgrounds above
  scrolling letters/numbers/caret graphics. Every visible base is still drawn.
- Click/drag/Shift-click in **Current** to select a single symbol or stretch.
  Type, paste, copy and delete using the existing toolbar mode and shortcuts.
  View is read-only; Insert requires an unambiguous single cell and inserts after
  it; Overwrite replaces a single-row stretch and advances. Undo/redo and trace
  provenance use the same alignment commands. Slide arrows retain the
  existing rigid-block rules. Imported calls/qualities remain immutable.
- **Alignment spacing** beside Zoom/Gain maps surviving original peaks to their
  current alignment columns. Gap changes, insertions/deletions and sliding
  automatically stretch/compress display intervals. Current includes gap cells;
  inserted bases have no invented original peak/quality. Deleted original signal
  remains inspectable. **Sample spacing** retains original coordinate spacing.
- The display uses an invertible monotone piecewise-linear transform, with
  original extrema retained during compression and display-only interpolation
  during expansion. Coincident original peaks share a display anchor; original
  evidence arrays are never altered. The precise policy is in TESTING_0.10.md.
- Native sessions save the spacing choice and current edits. Binary session format
  stays at version 1; older sessions default to Sample spacing, and old readers
  can ignore the optional spacing view field.

This completes the planned initial paired editable trace workflow. Manual signal
warping independent of alignment edits, free-residue compression and further
BioEdit selection/rename parity remain future refinements.

## Changes in 0.9

- Every visible Current/Imported base is drawn, even if letters overlap. Inserted
  Current symbols remain visible after the caret moves; they do not invent imported
  calls/quality. Clicking them in Current selects their alignment column.
- File → Open selects multiple AB1/FASTA files into one new alignment document.
  Single-file FASTA behavior and File → Add remain available. Command-line filename
  lists and nearby Explorer launches are grouped; later requests create separate
  windows rather than changing an existing document. Optional per-user Windows
  Open With registration and its verification steps are in TESTING_0.9.md.
- Tiny waveform markers beside sequence names identify linked traces; name hover
  shows the source. Dropdown switching and sequence selection continue to work.
- **.kleio sessions** embed imported analyzed signals/calls/peaks/qualities and save
  current symbols, row kinds/order/IDs, editing provenance, strand and view state.
  They reopen without needing the original AB1 paths. Current edits are retained;
  reopening begins a fresh undo history. The versioned binary format is documented
  in SESSION_FORMAT.md, with bounds/integrity checks and atomic file replacement.
- Save defaults to session format when traces are present. Ordinary FASTA documents
  retain FASTA saving, with an optional session format under Save as. Explicit
  Export FASTA preserves interchange while leaving the native session/save state
  intact. Neither export nor session saving overwrites an AB1; FASTA export also
  cannot overwrite a native session.
- Trace information uses **base calls** and **signal samples/channel**. PBAS/PLOC
  are source-format tags and have moved into the tooltip. A signal sample is one
  amplitude measurement, so each called peak spans several samples.

Existing trace formats such as AB1 and SCF describe individual sequencing reads,
not this editor's multi-row alignment plus original/current edit mappings. A small
application session format preserves those additional relationships explicitly;
it does not replace trace interchange formats. Original AB1 files stay unchanged.
SCF reference: https://staden.sourceforge.net/manual/formats_unix_2.html .

Direct trace editing and alignment-space stretching/compression are the next M7
substage. Signals currently retain native sample spacing; the current release
completes native persistence and the user's requested fixes/opening conveniences.

## Changes in 0.8

- File → Open and Add sequence from file accept FASTA and AB1/ABI. File content
  identifies ABIF, so the importer does not rely solely on its extension. Multiple
  files can be added together, and malformed imports leave the document unchanged.
- A linked **Chromatogram** panel shows the analyzed A/C/G/T signals, imported calls
  and quality values, current symbols, source identity and strand. Selection/caret
  navigation follows the corresponding peak; clicking a peak selects its alignment
  column. Alignment/overview scrolling, a trace row picker, independent trace
  panning, Zoom, Gain and previous/next-call controls support read-only inspection.
  View → Chromatogram controls visibility.
- Original samples, peak positions, chosen imported calls and qualities are
  immutable. Gap operations, rigid sliding, case changes, renaming and reordering
  retain their links. Substitution retains the original anchor and marks the
  current call as edited. Insertions use display interpolation only, without
  inventing an original peak or quality; deleted calls retain their original signal.
  Undo/redo restores both sequence and provenance. Reverse complement reverses
  direction, call mapping and complementary signal channels consistently.
- Whole/column MAFFT and target/template result windows preserve linked trace
  evidence. A whole-row MAFFT strand reversal also reverses the trace orientation.
  Mask rows remain excluded from trace/alignment eligibility as before.

The importer supports ABIF 1.x, validates directory/payload bounds, dimensions,
channel identities, call/peak/quality counts and ordered in-range peak positions.
It reads analyzed DATA9–12 and FWO_ dye order, using a coherent edited PBAS/PLOC1
baseline when present, or PBAS/PLOC2 otherwise. PCON is optional for that same call
set; missing qualities remain unavailable. It never combines calls and qualities
from different sets, guesses missing peak positions, trims or re-basecalls a trace.
Files over 64 MiB and unsupported/malformed trace layouts give a clear import error.

**In the initial 0.8 stage, saving exported FASTA sequence text only.** Trace attachments and their
edit provenance are not stored in FASTA; reopening it does not restore them. AB1
sources are never overwritten by FASTA export. This is the planned first read-only
trace stage: native session persistence, direct trace editing and alignment-space
stretching/compression follow in later stages. Signals use native sample spacing;
existing sequence editing remains in the alignment viewport.

ABIF format reference: Applied Biosystems, *ABIF File Format Specification*,
https://archive.gfjc.fiu.edu/workshops/resources/literature/ABIF_File_Format.pdf .

## Changes in 0.7

- Overview labels are now **Height = % non-gap symbols** and
  **Color = Column conservation**. The conservation calculation is unchanged.
- Insert places typed or pasted text **after** the selected symbol. The last
  inserted symbol stays selected, so repeated typing/pasting remains continuous
  before the untouched suffix. Selecting the final symbol appends; an empty-row
  or row-end caret also appends. Wider selections remain non-editable in Insert.
  Overwrite still replaces the exact selected stretch and selects the next symbol.
- Alignment → MAFFT → **Align selected sequences to templates…** uses full-row
  addition (`--add`). **Align selected fragments to templates…** uses fragment
  addition (`--addfragments`); “fragments” means the selected rows contain partial
  sequences, rather than selecting columns within full rows.

Select target sequence names first, then choose the already-aligned template rows
from a checklist. Targets and templates must be separate, with at least one
nonempty nucleotide row in each group. Selected masks and empty rows are skipped
as targets; they remain among the untouched rows. Duplicate titles are shown with
row numbers. The following settings dialog uses the existing MAFFT installation
and nucleotide parameters. Fragment addition uses Automatic, as recommended by
MAFFT; full addition offers existing presets except PartTree. Since 0.17.1,
fast/accurate strand adjustment is available for targets. Template reversal is
rejected, preserving the original template orientation.

The result opens in a new window, keeping row order, stable IDs, titles and types.
The source and its undo history remain unchanged. Every original column is
retained, including columns containing only template gaps and columns occupied
only by untouched nucleotide/mask rows. Added target insertions introduce new
columns and open gaps in other rows. Original non-target symbols, case, locked
and open gap states retain their exact original-column relationships. Short rows
are padded with open gaps in the result. Target old gap placements are discarded;
every target's original ungapped symbols and case are restored exactly, or
reverse-complemented when strand adjustment explicitly requests it.

Template residue ordinals anchor the column map; titles and repeated motif text
are not used to infer positions. The adapter verifies identities, residue order,
equal output lengths, and that no template column is split, merged or reordered.
It rejects a result that fails preservation, and does not apply raw output.
Cancellation or a failed process also leaves the source untouched.
Fragment addition requires MAFFT 7.526 or later. During validation, the official
7.525 source build failed on multiple-target fragment jobs, including informative
targets; a sanitizer trace localized the fault to its partial distance-matrix
export. The official 7.526 Linux binary passes these same cases. The adapter
checks the version before fragment alignment and gives an upgrade message on
older installations. Whole/column jobs and full-sequence addition retain their
existing version handling.

A deterministic boundary policy resolves template-only gap intervals: restore all
original columns in an interval before new insertion columns, placing those new
columns immediately before the next template anchor. Trailing insertions follow
the complete original axis, including any longer untouched/mask tail. New columns
that are entirely gaps in MAFFT output are omitted. The Last alignment report
records target/template IDs, parameters, original/restored/new column counts,
this boundary policy and preservation status. Neither `--keeplength` nor
`--mapout` is used, because those suppress target insertions.

MAFFT's official description and gap-only-column example:
https://mafft.cbrc.jp/alignment/software/addsequences.html .

## Changes in 0.6

- Drag rectangles and select ruler columns in View, Insert, Overwrite and
  Slide. Shift-click extends from the initial selection anchor.
  Switching modes preserves the selected range. Ctrl+C copies a single-row
  substring as plain text when no sequence names are selected; a multiple-row
  block leaves the clipboard unchanged. Since 0.17.2, selected names do not change
  fragment shortcuts; use Copy selected sequences for FASTA.
- Insert accepts typed/pasted symbols only at a single-cell caret or the row-end
  caret. Single-cell selection is blue; wider or multiple-row selection is yellow.
- Overwrite accepts typed or pasted symbols for a single-row selection of any
  length, replacing that exact stretch and placing the caret on the following
  symbol. The replacement can grow or shrink the row. Single-row selection is
  blue; multiple-row blocks are yellow and reject typed/pasted replacement.
- Delete and Backspace remove the selected stretch or block in Insert/Overwrite,
  shifting each stored suffix left. One undo restores all rows and exact gap
  states. Shorter rows are clamped; mask rows work in mixed selections.
- Double-click a residue to select its entire continuous nongap run, including
  ambiguous calls. Double-click a gap to select its whole run across gap types.
- Esc clears all selected names, the fragment and caret, and cancels a row drag.
  Row-selection commands now appear only under Sequence → Select. Ctrl+F opens
  Names matching regular expression. Ctrl+A retains sequence Select all.
- Every MAFFT parameter has brief hover help, also on its label where applicable.
- Statistics → Alignment statistics opens a compact window with the nine requested counts
  and a Copy button for one labeled, semicolon-separated line. Calculation runs
  in a background task with the source read-only; other document windows remain
  available. Closing the window cancels a pending scan.

Statistics exclude masks from rows, width and patterns. They use IQ-TREE's
nucleotide state conventions: case ignored, U = T; N, ?, gap types and virtual
trailing cells share the missing state for unique sequences and patterns. Partial
IUPAC calls retain their distinct possible-nucleotide sets. A constant site has a
nonempty intersection of possible bases, including compatible ambiguity-only
sites and all-missing sites. Parsimony-informative sites require at least two
definite bases occurring at least twice; the remaining variable sites are
singletons, matching IQ-TREE's report. Gap-only and N-only counts are subsets of
constant sites. N-only means only N/? plus gaps, with at least one N/?. Shorter
rows are treated as missing at their ends; no padding is written to the document.
Statistics do not invoke or require an IQ-TREE installation.

See VALIDATION.md for actual checks, comparisons and platform limits.

## MAFFT features (introduced in 0.5)


- Alignment → MAFFT → Align whole alignment runs all nucleotide rows after
  removing existing gaps. Masks are excluded from the process and their strings
  remain unchanged in the result. Empty nucleotide rows are restored as open-gap
  rows. Titles, row IDs, order, case, U and ambiguous residues are preserved.
- Realign selected columns uses the selected block/ruler's contiguous range
  across **all nucleotide rows**, regardless of selected names. It aligns only
  that region and reassembles the original prefix and suffix. Columns to the
  right move if its width changes; their symbols and relative alignment remain
  intact. Ragged missing cells are materialized as open gaps where a common
  fragment boundary requires them. Strand adjustment is disabled for fragments.
- Masks participate only in result assembly: their fragment text stays intact,
  with open gaps appended if the region grows. Their prefix/suffix move with the
  other rows. A region cannot shrink below its longest stored mask fragment;
  annotation characters and locked/open gaps are never truncated.
- The result opens immediately in a separate window. The source retains its
  file state and undo history; result edits have independent undo history. A
  result is an unsaved new baseline, with the source still available beside it.
  File → New window is available while MAFFT runs, for working on another file.
- The source window is read-only during export, alignment and validation.
  Export/parsing/assembly run in background tasks; MAFFT runs as a separate
  process. A progress/log dialog supports cancellation, including descendants.
  No output is applied on cancellation, launch failure, nonzero exit or invalid
  output. Residue identity/order, synthetic row IDs and equal output widths are
  checked before creating the result.
- Settings include Auto, FFT-NS-1/2/i, L/G/E-INS-i and PartTree presets; threads;
  iteration/tree counts; gap opening and MAFFT offset; Kimura scoring distance
  and transition/transversion ratio; composition, memory saving and repeatable
  refinement. E-INS-i defaults to offset 0. Whole-alignment strand options are
  keep, fast adjustment and accurate adjustment. Reversed rows are validated
  against their reverse complement and recorded in the report.
- Alignment → MAFFT → Last alignment report lists version, effective arguments,
  source revision/scope, mask count, elapsed time, reversed row IDs and the last
  64 KiB of logs. The launcher and thread choice are remembered between sessions.

MAFFT 7 is required. Current supported MAFFT versions are recommended; the
official source page warns against versions 7.463–7.486 because of a memory bug.
MAFFT is **not bundled** with this archive. On Windows select `mafft.bat` from
the official all-in-one installation, retaining its complete folder. Linux/macOS
can use an executable MAFFT launcher. PowerShell and WSL wrappers are not included
in this first adapter.

Existing open and locked gaps within the realigned scope are both removed before
alignment; newly generated gaps become open `~` gaps. Column-job prefix/suffix
gap states are preserved exactly. The initial alignment stage is not a constraint
aligner for locked gaps. Version 0.7 adds the separate template-based workflow
described above. Version 0.8 retains AB1 trace provenance through both workflows.

References for parameter/launcher behavior:
- https://mafft.cbrc.jp/alignment/software/manual/manual.html
- https://mafft.cbrc.jp/alignment/software/preservecase.html
- https://mafft.cbrc.jp/alignment/software/adjustdirection.html
- https://mafft.cbrc.jp/alignment/software/multithreading.html
- https://mafft.cbrc.jp/alignment/software/windows_without_cygwin.html
- https://mafft.cbrc.jp/alignment/software/source.html

## Changes in 0.4

Historical: Alphabetically was removed in 0.13.1; the identical sorting option
was replaced by Sequence → Groups → Group identical sequences in 0.17.1.

- Select names with Ctrl/Shift, then drag a selected name to reorder the group.
  A blue insertion line shows the destination; edge dragging scrolls vertically.
  Noncontiguous rows move together in their existing relative order. A normal
  click still selects one row; Escape cancels a drag. Each drop has one undo step.
- Sequence → Sort offers all nine requested methods. Sorts act on all rows and
  retain selected row IDs. Equal keys retain their current order. Title sorting
  ignores case, including Unicode titles. Alphabetically sorts the aligned
  sequence text, ignoring letter case but retaining gap characters in the key.
  Physical length includes stored gaps; true length excludes both ~ and -.
  Group by identical sequences groups case-insensitively equal aligned strings,
  with nucleotide and mask groups kept separate, preserving first-group order.
  Reverse order reverses the whole list.
- Frequency sorting needs a selected column range in Slide. It sums
  each row's nucleotide frequencies in those columns and sorts highest first.
  Frequencies come from all nucleotide rows, with U treated as T and partial
  IUPAC calls divided equally among their possible bases. N, ?, gaps and missing
  cells score zero. Mask rows are placed after nucleotide rows. A selected block's
  columns define the range, regardless of its selected rows.
- Sorting computes the order in a background task. Row movement and undo swap
  existing rows, preserving IDs and symbols without copying sequence strings.
- FASTA rows containing unexpected printable ASCII symbols become masks. The
  entire row uses dark gray, including any A/C/G/T characters it contains.
  Masks support selection, mixed blocks, sliding, gap tools, Insert, Overwrite,
  deletion and raw-text paste. Their kind persists through edits and undo/redo.
  Reverse complement skips masks; the core exposes nucleotide-only eligibility
  for future alignment integration. Masks do not affect overview height or color.
- Two- and three-base IUPAC calls now contribute fractional conservation votes:
  R splits between A/G, B between C/G/T, and similarly for other partial calls.
  N and gaps contain no base information. The legacy ? placeholder behaves as N.

Mask cells currently accept printable ASCII other than spaces and the FASTA
header delimiter >. FASTA whitespace is stripped and dot gaps become ~, as for
nucleotide rows. Non-ASCII cells, control characters and malformed FASTA still
produce errors; UTF-8 titles remain supported. FASTA stores symbols but no row
kind: an edited mask containing only valid nucleotide symbols will reopen as a
nucleotide row. Native session persistence remains a later stage.

## Changes in 0.3.1

- The rename dialog is wider (720 logical pixels) for descriptive headers.
- Hover over the vertical name/residue divider for the resize cursor, then drag
  it to adjust the name area. Names are re-elided to fit. The minimum width is
  80 pixels, with room retained for at least four residue cells. The width stays
  in effect when switching documents during the current session.
- The ruler reserves its own header height. Font size 6 no longer clips numbers;
  sequence rows retain their compact spacing. Selection/caret coordinates and
  scrolling follow the new layout.
- Overview bar height now shows the fraction of rows containing a residue,
  including ambiguous calls. Both gap types and missing cells have zero height.
- In 0.3.1 conservation excluded all ambiguous calls; 0.4 includes partial IUPAC
  calls and continues to exclude N. An A/N column is as blue
  as an all-A column; all-N columns are gray with full residue height, and all-gap
  columns have only a gray baseline. No-information columns are also excluded
  from the color average when several columns share a pixel.
- Colors use five discrete levels rather than a continuous muted gradient:

| Majority nucleotide fraction among called bases | Color |
|---|---|
| 100% | Blue |
| 90% to below 100% | Light blue |
| 75% to below 90% | Muted teal |
| 60% to below 75% | Golden orange |
| Below 60% | Bright orange |
| No informative nucleotide votes | Gray |

## Changes in 0.3

- Ctrl+C with no selected names copies a single-row residue fragment as plain
  text, preserving case and gaps. Multiple-row blocks leave the clipboard unchanged.
  Selected names continue to take priority and copy FASTA in document order.
- Ctrl+V in Insert mode inserts raw nucleotide text at the caret. Whitespace
  is removed; IUPAC symbols, U, N, ?, ~ and - are accepted, and dots become ~.
  Unsupported input is rejected as a whole. Paste is one undoable edit and advances
  the caret. Raw-text paste needs a caret and does not edit in other modes.
- FASTA text and copied local FASTA files still add sequences. File → Add from
  clipboard and New alignment from clipboard retain their existing purposes.
- The ruler has a tick for every column, longer ticks at multiples of 5 and
  longest ticks with numeric labels at multiples of 10. Coordinates start at 1.
- The bottom alignment overview spans every column. In 0.3.1 bar height shows
  residue density, and stepped color shows nucleotide conservation, with no-call
  regions gray. A narrow color strip also identifies empty columns.
  Click or drag the plot to center the alignment view there; the outlined frame
  tracks the visible columns. Hover for column ranges and percentages.
  View → Alignment overview hides or shows it.

Overview statistics use all nucleotide rows, regardless of selection, excluding
masks entirely. Residue height is nongap cells divided by nucleotide-row count.
Per-column conservation is the largest A/C/G/T vote total divided by all base
votes; case is ignored and U votes as T. A/C/G/T/U contribute one vote; partial
IUPAC symbols contribute one vote shared equally among their possible bases.
N and ? contribute to residue height but not color; gaps and missing cells
contribute to neither. A/R therefore has conservation 75%, all-R 50%, and A/N
100%. Adding a mask row cannot change any of these percentages.
Pixel bins average residue percentages over all columns and conservation only
over informative columns. Bins with no called bases use gray.

Calculation yields to input between short slices and restarts after sequence
edits, row additions/deletions, or undo/redo. Renaming, scrolling, selection and
font changes reuse cached statistics. During calculation the overview displays
progress instead of a stale plot. The viewport frame and navigation remain usable.
The scan stores about 20 bytes per column and does not copy the alignment.

## Changes in 0.2.1

- Double-click a sequence name to rename that specific row, with undo.
- Clicking blank space below the rows in the name panel clears row/fragment selection.
- Shift+click forms a rectangle across rows and columns from the original clicked
  cell; further Shift+clicks can shrink or reverse that rectangle.
- Drag/arrow slides push residues and locked dashes, consume external open gaps,
  and extend rows/the alignment to the right when needed. Internal block symbols
  and spacing are preserved. Groups retain one shared displacement.
- Edit → Copy sequence titles and Copy selected sequences (FASTA format) export
  selected names in document order, retaining complete headers, duplicate names,
  case and gap types. Version 0.3 adds the single-fragment plain-text behavior above.
- The font dialog has only fixed-width family, size, Regular/Bold/Italic/Bold
  Italic style and a preview. No underline, strikeout or writing-system controls.

## Slide behavior

Choose Slide. Click then Shift+click the opposite corner, or drag a
rectangle. Drag inside it or use Left/Right to slide. Escape clears the fragment.
For named noncontiguous rows, select those names first, then drag horizontally
in a selected row. Dragging vertically selects intervening rows. Ruler dragging
selects columns for selected names, or all rows if no names are selected.

The rectangle stays rigid. When moving right, encountered residues/dashes are
pushed right; the nearest downstream ~ gaps absorb movement. Once these run out,
rows grow. When moving left, preceding residues/dashes are pushed left and the
nearest upstream ~ gaps are consumed. Movement is clamped only when a selected
row cannot fit its preceding non-open-gap content before column 1. Vacated space
is filled with ~. Internal gaps are never consumed. Missing cells are virtual ~,
materialized only when touched.

The existing Ctrl+Left/Right and maximum-movement context actions retain their
previous purpose: **jump through adjacent open gaps** until the next residue or
locked dash (or the current alignment boundary). They do not request unbounded
pushing. Ordinary arrows and dragging implement the corrected pushing behavior.

The right-click dropdown must be “menu” for fragment actions (including N/gap
fill). Its four gap tools act immediately and shift downstream content as a
block. Gap deletion still removes only ~; unlock - before deleting it that way.

Sequence transformations act on selected names. Degap removes ~ only. Unalign
removes both gap types across the document. Minimize removes global all-gap
columns. Reverse complement preserves case/IUPAC/gap types; rows with U and no T
use RNA complements, otherwise DNA complements.

All sequence edits undo/redo atomically. Recent fragment selection positions are
restored for 128 recorded fragment states; older undo may retain current selection
coordinates. FASTA does not store selections.

## Retained functionality

Direct FASTA open/save and additions from file/clipboard; new sequence dialog;
full UTF-8 headers, duplicate names, case and ragged lengths; View/Insert/Overwrite;
row selection/deletion; flush; tooltips; undo/redo; wheel and Ctrl+wheel scrolling;
cached glyph/tile rendering; fonts; individual/table rename; separate name and
aligned-sequence regex row selection. Regex includes gaps, is case-sensitive by
default and accepts (?i). Motif highlighting/navigation is still pending.

A/a uses dark yolk yellow #B47D00, C blue, G black and T/U red. Gap/ambiguity colors
remain provisional. Open (~) and locked (-) gaps are stored separately. Imported
dot gaps normalize to ~. Unexpected printable symbols classify imported rows
as masks in 0.4.

## Limits and remaining work

Free-residue compression mode, manual independent trace warping, full
BioEdit selection/rename parity, tabs and further session/trace refinements remain pending. Protein
features remain low priority. Whole-row transformations, minimize, clipboard
export and integration of added rows currently execute on the UI thread; huge
operations have not been latency-benchmarked. The logical undo budget is 64 MiB;
the newest operation is retained even when it exceeds that limit.

## Build on Windows using Qt Creator

1. Install the Microsoft C++ build tools with **Desktop development with C++**,
   including a 64-bit MSVC toolset matching the Qt kit. MSVC 2022 is the initial
   recommended kit.
2. Use the Qt Online Installer to install a supported Qt 6 release with the
   matching **MSVC 2022 64-bit** desktop package, Qt Creator, CMake, and Ninja.
   Qt Widgets and Qt Concurrent are needed; no QML, WebEngine, or Python is needed.
3. Extract this archive. In Qt Creator, open the top-level **CMakeLists.txt**.
4. Select the matching desktop kit, configure, and build the **kleiotide** target.
5. Run it and use File → Open to open **examples/nucleotide_demo.fasta**.
6. Use the included core test target before testing your own editing workflows.

Version boundaries: application code requests Qt >= 6.5 and CMake >= 3.21, but
the chosen Qt release may impose higher tool requirements. Use the CMake/Ninja
tools provided with that release; do not infer compatibility from these minimums.

Official setup guidance:
- https://doc.qt.io/qt-6/get-and-install-qt.html
- https://doc.qt.io/qt-6/windows.html
- https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170

Do not mix a MinGW Qt library package with an MSVC build. A complete matching
MinGW kit is an alternative if needed; it does not change the source architecture.

## Command-line build (when Qt and CMake are installed)

Use a developer terminal appropriate to your Qt/compiler kit.

~~~text
cmake -S . -B build -DCMAKE_PREFIX_PATH="<path to your matching Qt kit>"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
~~~

For a core-only build, without Qt:

~~~text
cmake -S . -B build-core -DKLEIOTIDE_BUILD_GUI=OFF
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
~~~

For a current core-only build (CMake also generates the version header):

```bash
cmake -S . -B build-core -DKLEIOTIDE_BUILD_GUI=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```Kleiotide 0.21.1 — distance dispersion and export provenance

Version 0.24 exports edited trace-linked sequences as standalone AB1 files,
including insertions/deletions, substitutions and current strand orientation.
Dependencies remain C++20, Qt Widgets and Concurrent (Qt >=6.5).

## Updating on Windows

1. Extract into a fresh `Kleiotide_0.21.1` folder; open CMakeLists.txt with the usual
   Desktop Qt/MSVC kit and build `kleiotide`.
2. Follow TESTING_0.21.1.md. Distance rules are in DISTANCES.md; translation rules
   remain in TRANSLATION.md.
3. New native saves use format 4. Formats 1–3 remain readable; earlier applications
   cannot open format 4. FASTA remains sequence text only. MAFFT is separate.

## Changes in 0.21.1

- Overall, within-group and between-group means show SD (pairs) and SE (naive).
  With m valid pair distances, sample SD uses divisor m−1; naive SE is SD/√m.
  Both are N/A for fewer than two valid pairs. Excluded pairs never enter them.
- Formulas and the independence limitation appear in the dialog, tooltips,
  TSV comments and DISTANCES.md. Shared sequences make pair distances dependent;
  the naive SE is not a bootstrap or dependence-adjusted uncertainty estimate.
- Distance and nucleotide-composition Copy/Export TSV now identify the exact
  Kleiotide build version. CMake supplies the application/export version centrally.
- SD is accumulated in one streaming pass; no full matrix is required for means.

## Changes in 0.21

- Statistics → Pairwise distance matrix, Overall mean distance, Within-group mean
  distance and Between-group mean distance, with Copy TSV and atomic TSV export.
- Nucleotides: observed differences, p-distance, Jukes–Cantor, Kimura 2-parameter.
  Proteins: observed differences, p-distance, Poisson correction.
- Explicit molecule, row and column scope, pairwise/complete deletion, minimum
  comparable sites and ungrouped inclusion. Ambiguous/missing states are excluded;
  unavailable estimates show N/A with reasons and denominators.
- Sequence → Sort → By decreasing similarity… uses a deterministic nearest-neighbor
  chain from the first eligible row. Preview/Cancel or Apply with one Undo step.
  Ineligible rows retain their slots; groups, IDs and trace evidence are preserved.
- Cancellable background work, lazy tables, bounded triangular matrix storage,
  streaming means/export and resource estimates. No native schema change.

## Changes in 0.20.3

- Generate a `translation_mask` row from the current annotation; apply an existing
  or pasted mask through Alignment → Translation → Annotate from mask….
  Mask-name context menus offer Annotate using this mask. Both operations undo.
- Mask digits 1/2/3 specify codon phases; all other symbols specify noncoding.
  Short masks pad with noncoding columns; longer masks are rejected.
- Incomplete trailing codons are translated using unknown missing bases (GC → A,
  AT → X). Display, stop counts, Copy and protein snapshots use the same calls.
- Edit → Rename can append/prepend group names, with scope, separator and preview.
- Sequence-type sorting is stable: Mask, Protein, Nucleotide.
- Custom annotation label, separate translation and gap command sections,
  Select/Sort at the bottom of Sequence, Motif search in View, and font last.

## Previous translation refinements (0.20.2)


- Single-letter AA display and output only; no three-letter controls, encoding
  selector or three-letter text command. Translation toolbar controls are spaced.
- Add translated sequences now creates ordinary protein data such as ~M~~K~~*~.
  Letters occupy middle codon positions; surrounding gaps are ordinary open gaps.
  They can be selected, edited, cut/pasted and slid independently.
- Older bound-span translated rows migrate to centered ordinary proteins on load,
  retaining source/code/frame provenance. Imported ordinary protein data stay put.
- All 20 standard amino acids have distinct shades within their existing families.
  Letter/background contrast is adapted to darker and brighter shades.
- Copy translated fragment accepts incomplete codon selections and visibly expands
  to complete codons when invoked. The separate expansion action is removed.
  Ordinary Ctrl+C retains the exact selected underlying text without expansion.
- Native format remains 4; current and earlier 0.20.x sessions remain readable.

## Changes in 0.20.1

- Fixed residue-based amino-acid colors in imported proteins and translated codons.
  Gap-heavy and variable columns no longer leave amino acids uncolored. Ambiguous
  amino-acid symbols are gray; gaps stay uncolored. Monochrome remains available.
- Read-only genetic-code selectors with concise labels and full-name tooltips.
  Custom code… is the final dropdown command, including in snapshot previews;
  Cancel preserves the selected code. Preview-only codes do not change defaults.
- A colored L cell above Leu identifies the one-/three-letter display switch.
  Set selection coding is a wrapped rectangular control.
- Copy translated fragment is enabled only for a complete, eligible single-row
  nucleotide selection while the translation layer and current result are active.
  Both Alignment and fragment context menus show its availability.
- Native sessions remain format 4, compatible with version 0.20.

## Changes in 0.20

- Translation toolbar: codon coloring, same-width one-/three-letter AA preview,
  coding ruler, coding/noncoding annotation controls, 27 NCBI tables, custom code
  import/editor, frames and detailed live stop/uncertainty counts.
- IUPAC consensus, mixed-gap flags, split codons, partial boundaries and contextual
  stop codes share one engine for display, counts and protein snapshots.
- Add translated sequences below selected sources with preview, unique names,
  group inheritance, provenance and one-step Undo. Compact protein window/export
  and explicit three-letter text output are available separately.
- Canonical amino-acid tokens with structural span metadata protect protein
  editing from interpreting three-letter labels as three residues.
- Native format 4 retains translation settings, annotations, protein layouts and
  provenance. Typed whole-sequence clipboard retains this metadata.
- Translation/protein profiles calculate in cooperative revision-checked slices;
  selection and scrolling reuse completed tiles and counts. Trace caches remain.

## Changes in 0.19.4

- Chromatogram signal, imported labels and current labels are cached independently
  of the selection/caret overlay. Extending a selection does not redraw the signal.
- Trace positions and original-call mappings are indexed once per row edit.
  Sample-spacing labels are restricted to the visible sample interval without
  repeatedly scanning each residue prefix. Inserted and overlapping calls remain
  visible with the same mapping, strand and quality semantics.
- Trace switching uses an ID lookup; cache invalidation covers edits, Undo/Redo,
  scroll, zoom, gain, spacing, palette, font, resize and DPI changes.
- Native session format remains 3. Windows checks: TESTING_0.19.4.md.

## Changes in 0.19.3

- Undo and Redo reveal the affected row and column range for residue edits, even
  after scrolling away. Selection restoration is retained. Renaming, grouping and
  reordering do not cause residue-navigation jumps.
- Dragging a selection repaints changed cells and outlines instead of the entire
  viewport. Unchanged selections skip notifications; row/tile work is bounded to
  the dirty area, and translucent glyphs are drawn once per tile.
- Selection updates refresh clipboard actions, status and trace following without
  refreshing document panels/toolbars or copying selected substrings repeatedly.
- Session format remains 3. Details and Windows checks: TESTING_0.19.3.md.

## Changes in 0.19.2

- Upper-right counts yield to the editing/display controls when the window is
  narrow, and return automatically when sufficient width is available.
- Mouse-slide Undo/Redo restores the selected rectangle with the residue block.
  Selection state is independent of drawing-cache resets, and drag interaction
  ends before committing the edit. Keyboard sliding keeps its behavior.
- Ctrl+Shift+C / X / V invokes whole-sequence Copy / Cut / Paste. The bindings
  appear in Edit and sequence-name context menus. Plain Ctrl+C / X / V remains
  fragment-only, independent of selected sequence names.
- Reported MSVC narrowing/shadowing warning sources are corrected without warning
  suppression. The state comparison uses unsigned, clipboard/trace locals have
  descriptive names, and the existing selection reference is reused.
- Native format stays 3. Acceptance and warning details: TESTING_0.19.2.md.

## Changes in 0.19.1

- Two segmented switches: Edit/View and Rewrite/Insert, with spacing before the
  Right click selector. Rewrite replaces the Overwrite name. There is no separate
  Slide mode: dragging selections, Shift+Left/Right (one column), and Ctrl+Left/
  Right (adjacent open gaps) work in both edit submodes. Plain arrows move the caret.
- Movement commands form a separate context-menu group in both edit modes. Menu
  commands show their available shortcuts, including native context hints. Fragment
  Ctrl+C/X/V stays independent of selected sequence names; whole-row clipboard
  commands keep their explicit menu/context actions.
- Selected-name highlight scope shades only the captured analysis rows. Snapshot
  inputs stay independent of subsequent name/fragment selection changes.
- Alignment totals stay visible at the upper right, including during selection.
  Counts update after edits, row changes and undo/redo; the status bar keeps the
  selected-range information.
- Native format remains 3. Old Slide mode (numeric 3) migrates to Rewrite; old
  Overwrite (numeric 2) is Rewrite. View retains its remembered edit submode.
- Windows acceptance: TESTING_0.19.1.md. Next milestone remains protein coloring
  and translation; this release implements the requested editor refinements.

## Changes in 0.19

- Highlight menu: conserved, variable, parsimony-informative and singleton sites;
  inclusive percentage ranges for coverage and conservation; custom color and
  Clear. Multiple enabled rules combine by union. Pale violet shading and ruler/
  edge cues remain visible in every display style; residue selection has priority.
- Explicit analysis scope: nucleotide or protein, all rows or selected-name
  snapshot, all columns or selected-column snapshot. Masks never contribute.
  Scope stays independent of subsequent selection changes. Native sessions retain
  scope, ranges, rules and color, without a binary format change.
- Statistics menu: the existing summary moves here as Alignment statistics.
  Nucleotide composition adds per-row and pooled counts/frequencies, optional
  fractional IUPAC handling, explicit row/column scope, missing/gap accounting,
  Copy and UTF-8 TSV export. Calculation is asynchronous and cancellable.
- Shared site summaries preserve existing statistics counts and overview voting.
  Compatibility-conserved sites differ from strict 100% conservation; the precise
  definitions and denominators are documented in SITE_ANALYSIS.md.
- Example and Windows acceptance: examples/site_highlight_demo.fasta and
  TESTING_0.19.md. Protein presentation/translation is the next roadmap milestone.

## Changes in 0.18

- Edit → Trim low quality ends and Low-quality calls to N open a settings and
  preview dialog. Scope is explicit: selected sequence names, or all trace-linked
  nucleotide rows. An empty name selection never silently means all. The preview
  reports proteins, masks and unlinked rows as ineligible.
- Trimming uses arithmetic mean Phred Q across complete windows of current
  non-gap symbols. Keep the interval between the first and last windows with
  mean Q >= threshold; internal low-quality islands stay. Only trimmed residues
  become open gaps (~): row length, alignment columns and pre-existing gap states
  are retained. No complete passing window means no change to that row.
- Low-quality calls to N replaces only calls with Q strictly below the threshold,
  preserving positions and letter case. Existing N/n and all gaps stay unchanged.
- Quality is read through original-call mappings, including reversed traces and
  aligned/gapped rows. Missing Q is not Q0. Inserted calls always remain protected.
  Substituted symbols are protected by default, including Ns from earlier cleanup.
  Explicitly enable original-call Q to evaluate them; it remains original evidence
  rather than a new measurement for the current symbol. This policy also applies
  to trimming: the retained interval expands to include every protected call, and
  passing windows require usable Q at every position.
- Preview calculation runs asynchronously and supports Cancel. Settings changes
  clear the old preview and disable Apply. Source editing remains locked while
  preview/cancellation is running. Apply is one atomic undoable change across all
  affected rows; original signals, base calls, qualities and group metadata remain
  intact. Save .kleio to preserve edits and trace evidence. Format remains 3.
- Defaults: Q20 and a ten-symbol trimming window. The last applied threshold and
  window are remembered; original-Q inclusion starts unchecked each time.
- examples/trace_quality_demo.kleio is entirely synthetic and needs no external
  AB1 files. See TESTING_0.18.md for known-result settings and expected edits.

## Changes in 0.17.4

- An eight-pixel logical spacer separates the Edit/View segments from the
  Overwrite/Insert/Slide dropdown; it scales with display DPI.
- Delete selected sequences is grouped with Cut/Copy/Paste sequences in both
  Edit and the sequence-name context menu. Ctrl+Delete retains its meaning.
- Group identical sequences now explains that the complete sequences must match
  after gaps are removed. A shorter sequence matching only a substring of a
  longer sequence is not grouped with it. The grouping algorithm is unchanged.

## Changes in 0.17.3

- The mode switch has two joined, permanently visible Edit and View segments.
  The active segment is blue with white text; the other remains neutral. Click
  the desired segment to choose a mode. Clicking the active segment keeps it
  selected. Both are native keyboard-focusable buttons. New windows still start
  in Edit/Overwrite, and switching back from View restores the last editing mode.
- In View, both the editing-mode and right-click dropdowns are inactive. The
  right-click dropdown shows Menu; returning to Edit restores its previous choice.
- Sequence → Lock gaps in selection and the fragment context menu convert only
  open gaps (~) inside the selected rectangle to locked gaps (-). Residue positions,
  row lengths, outside cells and original trace evidence remain intact. Single-row
  and multi-row selections work in Overwrite, Insert and Slide, independently of
  name selection. Unlock is available beside Lock. Each operation is undoable.
- Native session format remains 3; existing saved View/Edit states retain their
  meaning. No data-format migration is required.

## Changes in 0.17.2

- The left toolbar segments select View/Edit. New windows start in Edit with
  Overwrite. The dropdown to its right contains Overwrite, Insert and Slide.
  Switching to View retains the last edit mode, disables its dropdown and limits
  right-click behavior to Menu (its dropdown is disabled since 0.17.3). Returning
  to Edit restores the last edit mode and
  right-click choice. Saved sessions retain View/Edit and the remembered edit mode;
  existing saved mode values keep their meaning.
- Ctrl+C/X/V and Edit → Copy/Cut/Paste fragment always act on residue selection,
  independently of selected sequence names. They never copy, delete or import
  entire rows. FASTA and native whole-sequence payloads require Paste sequences.
- Edit and the name-area context menu provide Cut selected sequences, Copy
  selected sequences and Paste sequences, without Ctrl+C/X/V shortcuts. Sequence
  copies remain unwrapped FASTA with native type/group/trace metadata. Paste
  inserts after the last selected name or appends with no names selected.
- The context menu on a selected fragment provides Copy in every mode. Cut is
  available in Insert/Overwrite on one row. Paste substitutes a one-row span in
  Overwrite, or inserts after a single selected symbol in Insert. Multi-row
  fragment clipboard operations remain disabled. Shortcuts also support the
  existing unambiguous caret/end boundary in Insert/Overwrite.
- Sequence → Unlock gaps in selection and the fragment context action convert
  only locked dashes inside the selected rectangle to open gaps. Residues and
  outside cells retain positions; short rows are not extended. Names do not
  determine affected rows. The operation is undoable and requires Edit mode.
- Group identical sequences now ignores both open (~) and locked (-) gaps, as
  well as case. Rows of different types remain separate. Duplicate clusters get
  groups, singletons become Ungrouped, and previous grouping is replaced with
  one undo step. Row order is retained.

## Changes in 0.17.1

- Sequence → Select adds Nucleotide sequences, Protein sequences, Masks and
  Sequences with traces. These replace the name selection independently of residue
  selections. The trace option uses embedded links, even if the source AB1 moved.
- Group swatches are ten pixels wide, with extra name padding.
- Sequence → Groups → Group identical sequences replaces the old identical sort.
  It creates groups for duplicate strings (case insensitive, gaps ignored since
  0.17.2, separate row types). Singletons become Ungrouped. Existing
  groups/memberships are replaced in one undoable action; row order is retained.
- Alignment → MAFFT → Align selected sequences opens only the selected rows in a
  new alignment window. Unselected rows never participate. Selected masks remain
  excluded from MAFFT and are retained in the new document; selected proteins are
  rejected until protein MAFFT is implemented. Types, groups and traces survive.
- Both target-to-template MAFFT workflows offer fast/accurate Adjust strand.
  Only targets may reverse; every template strand and original column is checked.
  Reversed targets retain case, IUPAC/RNA symbols and trace provenance. A malformed
  result or attempted template reversal is rejected with the source unchanged.
- Residue Cut/Paste is single-row only, including rejection of 0.17 block payloads.
  Multi-row selection, sliding and Delete/Backspace editing remain available.

## Changes in 0.17

- Sequence → Groups → Manage groups creates/renames/deletes groups, chooses RGB
  colors, edits members and sets group order with the arrow buttons. Changes are
  staged until Apply, then committed in one undo step. Each row has one group or
  Ungrouped; masks and proteins may belong too. Membership uses stable row IDs.
- A color swatch beside sequence names stays visible during selection. Name
  tooltips identify the group. Right-click a sequence name for Add to group or
  Remove from group; an unselected clicked name becomes the action target.
- Sequence → Select → Groups chooses any combination of groups and Ungrouped,
  preserving the independent residue selection. Sequence → Sort → By group uses
  manager order, stays stable within groups, and puts Ungrouped last.
- Import groups from table accepts complete title<TAB>group name, with a header
  checkbox and preview. Exact full titles are matched; unmatched titles are
  reported/skipped. Duplicate alignment titles require explicit permission to
  assign all matches; conflicting assignments block Apply. Empty group means
  Ungrouped. No edits occur until a valid preview is applied.
- Native format 3 retains group IDs, names/colors/order, memberships and the next
  group ID, with retained readers for formats 1 and 2 (old rows are Ungrouped).
  Renaming, reordering, row deletion/undo and MAFFT result creation retain groups.
  Normal Save prefers .kleio when group data is present.
- Edit → Cut selected sequences cuts names selected in any mode. Paste sequences
  inserts them after the last selected name, or appends with no names selected.
  Existing rows are retained. Each Cut and Paste is separately undoable.
- Kleiotide sequence copy/cut supplies ordinary unwrapped FASTA text plus a native
  clipboard payload. Between current Kleiotide windows this preserves types,
  group colors/membership and embedded trace links. A matching group name/color is
  reused; a different-color name collision gets a numeric suffix. Clipboard row
  IDs are remapped to fresh destination IDs. File → Add from clipboard appends.
- Residue Cut/Paste works only on a single row in Insert/Overwrite, regardless
  of selected names. A multi-row fragment selection changes neither clipboard nor
  alignment on Cut/Paste. Older multi-row residue payloads are rejected even at a
  single-row caret. Copy retains the same single-row restriction.
  Fragment Cut leaves an insertion
  boundary at the deleted stretch; Insert paste inserts and Overwrite substitutes.
  Pasted text creates edited/inserted calls, without new measured qualities.
- Ctrl+wheel horizontal speed doubles: six columns per normal wheel notch instead
  of three; the existing pixel-wheel step doubles too. Ordinary vertical and
  native horizontal wheel behavior retains its existing step.

Native clipboard payloads are limited to 256 MiB; an over-limit sequence Cut is
rejected before deletion. TSV file loading is limited to 16 MiB. Plain FASTA and
plain text do not carry group/type/trace metadata. Ctrl+C retains its existing
single-row residue-fragment behavior; whole-sequence copying now also preserves
native metadata when pasted within Kleiotide.

Next: site highlighting and descriptive statistics. Protein presentation,
translation and protein MAFFT remain later roadmap stages.

Source archives use fixed historical ZIP dates to avoid CMake timestamp loops.
Python is only needed to run tools/package_source.py, not to build the application.

## Changes in 0.16

- File → FASTA import type chooses Nucleotides + masks (the unchanged default),
  Proteins + masks, or Masks only. It applies to Open, Add and FASTA clipboard
  imports in this window. ACGT may also be protein: there is no reliable alphabet-
  only inference, and .faa alone does not override the explicit choice. AB1 and
  saved sessions retain their known/stored types.
- Add new sequence has its own type selector. Sequence → Set sequence type…
  applies to selected names atomically and supports one-step undo/redo. It does
  not translate or rewrite symbols. Invalid alphabets reject the whole operation;
  trace-linked nucleotide rows cannot be retyped to protein/mask.
- Small N/P/M badges identify types beside names; hover explains the type. Protein
  rows currently render in neutral gray, support the shared editing/selection/gap
  tools, and never get nucleotide coloring. Exact identity dots compare matching
  types only. Proteins/masks do not vote in nucleotide search/overview/statistics.
- Protein storage uses one-letter codes. It accepts the standard amino-acid
  letters plus B/Z/J/X/U/O, stop *, unknown ?, and gaps; case is preserved and
  dot becomes an open gap. Explicit invalid protein input from FASTA becomes a
  mask, matching the existing unexpected-symbol mask policy. Typed editing and
  type assignment validate strictly, with no silent type changes.
- Normal Save prefers .kleio for proteins, trace-linked rows and explicitly forced
  masks that would look like nucleotides in FASTA. Export FASTA remains unwrapped,
  canonical sequence text. Plain FASTA has no type metadata; choose the desired
  import type when reopening it. Native sessions preserve types, canonical
  representation, row IDs/order, current symbols, view and embedded trace data.
- Format 2 has mandatory type/representation fields with a retained format-1
  reader. Legacy masks stay masks, including protein-looking ones. Unsupported
  encodings/types reject clearly rather than being interpreted as ordinary
  amino-acid letters. Reopening still starts a fresh undo baseline.
- Current MAFFT remains nucleotide-only. Protein-containing whole/column inputs
  and selected protein targets are rejected before launch. Unselected protein
  rows in a nucleotide target/template job retain their original projected axis,
  types and symbols just like other non-target rows.

The next milestone is persistent sequence groups. Clustal-style protein coloring,
translation/codon-positioned representations and protein MAFFT remain planned at
stages 6 and 8; see ROADMAP_1.0.md. Group metadata is not introduced in this release.

All ZIP members use a fixed historical date to avoid CMake timestamp regeneration
loops. Create source archives with `python tools/package_source.py OUTPUT.zip`;
Python is needed only for packaging.

## Changes in 0.15.2

- In residue-background mode, every motif hit uses a bright lavender fill, dark
  letters/dots and a purple outline with a white halo. Other display modes retain
  the light shade. The current editing selection keeps its blue/yellow fill.
- Exclude N-only matches is optional (initially off). It checks the actual matched
  interval, ignoring gap symbols and letter case: all remaining symbols must be N
  for exclusion. Mixed residues/ambiguities and gap-only matches remain. It applies
  to literal/regex searches and both strands before counting/indexing results.
- Find and Next/Previous already select each visited match, so the Select match
  button is removed. Filtering uses constant-time interval queries over a bounded,
  cancellable per-row bitmap; long overlapping N runs are not repeatedly scanned.
  Search/filter settings remain transient; the native session format is unchanged.

## Changes in 0.15.1

- The search panel adds Regex: case-insensitive Perl-style syntax including
  classes, groups, alternation, repeats, anchors and captures. Regex is matched
  against each searched orientation, not reverse-complemented as pattern text.
  Dot is a wildcard; ~/- are literal gaps when gaps are included. Compatible
  IUPAC is disabled in regex mode; explicit classes retain standard regex meaning.
- Find scans the scope once, selects/shows the first match, shades/outlines all
  other intervals and reports Total/Forward/Reverse complement/Both counts.
  Total counts distinct aligned intervals; both-strand intervals contribute to
  both orientation counts. Empty regex matches are ignored and reported.
- Next/Previous/F3 use a compressed index and automatically select each match.
  Name selections remain independent. All-hit highlights survive other cell/name
  selections and unfinished/failed replacement searches until a successful Find
  or panel close. Document revisions clear invalid coordinates with a refresh
  message. Normal editing fills retain their blue/yellow colors.
- Arithmetic runs, varint encoding, ordinal checkpoints and visible-column
  coverage avoid expanding dense hits during navigation/painting. Cancellable
  workers and engine/index limits reject failures without partial counts/results.
  Limits and examples are documented in TESTING_0.15.1.md. No new dependency or
  native session format version is introduced.

## Changes in 0.15

- FASTA Save/export and sequence clipboard copying write each complete sequence
  on one line. Reading arbitrary wrapping retains its existing behavior. A shared
  record writer preserves case, gap symbols, empty rows and masks.
- Edit → Find motif… (Ctrl+Shift+F) opens an optional dock. Find starts at the first
  hit; Next/F3 and Previous/Shift+F3 traverse all overlapping hits in current row
  order and column order. Ctrl+F retains name-regex selection.
- Options search all nucleotide rows or selected names, ignore or match exact gap
  types, include reverse complements, use IUPAC compatibility and wrap around.
  The result reports the row/title, actual aligned columns, strand and wrapping.
  Both-strand matches at one interval are coalesced. Masks are excluded.
- An independent purple outline with white halo marks the current hit without
  changing names, residue selection, caret or mode. Select match explicitly makes
  that interval the editing selection. All display styles retain their selection
  colors. Closing the dock clears the search outline.
- Searches use a cancellable worker and temporarily lock document editing. They
  retain one hit and motif-sized matching state, not an alignment-sized copy or
  complete hit list. Edits/reordering/undo, document replacement and query/scope
  changes invalidate the cursor. Queries/results are transient, not native data.

## Changes in 0.14.1

- Classic is now named Kleio. Its saved `classic` identifier is retained so
  existing sessions restore the same default colors.
- Chromas-style adds exact A #008040, C #0000FF, G #000000, T/U #FF0000 inks
  and cell backgrounds. It also controls trace channels, legend and palette icons.
- The overview explanation reads "Color: conserved column (blue) -> variable
  (orange), gray = no information" in its header and tooltip.
- ROADMAP_1.0.md records completed milestones and the remaining functional,
  reliability, Windows installer and release-candidate stages.

## Changes in 0.14

- View → Residue palette offers Classic (the current colors), High contrast,
  Okabe–Ito and Paul Tol bright alternatives. A compact swatch dropdown beside
  the view buttons shares these actions. Classic remains the default.
- View → Residue display selects colored letters, monochrome or colored cell
  backgrounds. Two compact toolbar toggles provide monochrome and backgrounds;
  clicking the active toggle again restores colored letters. All controls have
  hover help and stay synchronized with the menus.
- Background cells choose black or white lettering for contrast. Selections keep
  their established blue/yellow cues and use dark letters over those fills.
  Identity dots preserve the underlying residue background; copied/exported
  symbols remain actual letters. Masks remain neutral dark gray on white.
- Palettes also update trace-channel colors, the trace legend and toolbar icons.
  Monochrome/background modes apply to the alignment grid. Overview conservation
  retains its independent blue/orange scale. See DISPLAY_COLORS.md for sources,
  exact base colors and contrast adaptations.
- Palette and display mode persist in optional .kleio view settings. Older or
  unknown settings default to Classic colored letters. View changes preserve
  sequences, editing selection, undo history and original trace evidence.
- Cached rendering includes selection glyph tiles for colored backgrounds. A
  fixed selection-edge strip is repainted after horizontal scrolling to avoid
  stale border pixels for wide blocks.

## Changes in 0.13.1

- Alphabetically (sorting by sequence symbols) is removed. The other sorting
  methods, including By title, remain available.
- Edit → Rename → Sanitize names uses the existing preview and one-step undo.
  It defaults to all rows even when names are selected; the explicit scope can
  be changed to selected rows. Outputs contain only ASCII A–Z/a–z, 0–9 and '_'.
  Runs of unsupported characters become a single underscore; newly normalized
  names lose outer underscores and fall back to `sequence` if nothing remains.
  Non-ASCII characters are replaced rather than translated or transliterated.
- Existing safe names are preserved where possible. Exact and case-only duplicates
  receive `_2`, `_3`, ... suffixes, skipping occupied names and all reserved output
  bases. Valid names such as A_B or A_B_2 are not taken by an earlier unsafe title
  that normalizes to the same base. Safe unique numeric or underscore-only names
  remain valid and unchanged. Running sanitization twice is a no-op.
- These are the requested character/uniqueness rules; no name-length truncation
  is performed. Preview/Cancel, selected scope, undo/redo, IDs, sequence symbols,
  mask kinds, traces and native/FASTA persistence retain existing behavior.
- The next display stage is queued: selectable high-contrast/color-blind-friendly
  residue palettes, monochrome view, and residue-colored backgrounds with automatic
  black/white lettering for contrast, accessible through View and toolbar controls.

## Changes in 0.13

- The first-nongap-in-selected-columns option is removed, as requested. Other
  selection tools retain their behavior. The identity-view toolbar icon now
  depicts tiny A/C/G/T letters in their nucleotide colors with dots below.
- Edit → Rename gains direct entries for space/underscore exchange, prepend and
  append text, literal case-sensitive replacement, appending a filename stem,
  reversing names, stripping numeric suffixes, making names unique, names from
  row numbers, aligned lengths and true lengths, and appending taxonomy/text
  using a title-matching table. These share a wide preview dialog.
- The dialog starts with selected rows when names are selected, otherwise all
  rows. Scope is explicit and can be changed. Preview lists current/new names,
  changed/unchanged counts and duplicate resulting names; duplicates are marked
  in amber and remain allowed for operations where duplicate names are intended.
  Editing any option clears the preview and disables Apply until previewed again.
  Cancel/no-op/error leaves data/history unchanged. Apply is one undoable rename.
- Make names unique compares complete titles exactly, preserves the first when
  possible and adds separator + 2, 3, ... to later copies. It avoids every original
  title and unselected name; existing `_2` names are never overwritten/reused.
  Strip numeric suffixes removes only the chosen separator followed by trailing
  ASCII digits. Preview makes any resulting collisions explicit.
- Reverse operates on Unicode grapheme clusters, preserving emoji/combining
  marks. Space conversion affects ordinary spaces. Replace is literal, without
  regex/backreference interpretation. Names remain nonblank single-line headers.
- Row-number names use current alignment row positions, starting at 1, including
  unselected rows. Aligned length includes both gap types; true length excludes
  them and works on masks. An optional prefix can accompany numeric names.
- Filename text is prefilled from the current alignment/session filename stem
  when available, can be edited, and is appended with the chosen separator.
  Untitled documents require entering the desired filename text.
- The existing old-name/new-name table operation also gains preview. Matching
  uses exact complete original titles and is simultaneous, without cascading.
  The preview reports unmatched keys and keys matching several rows in the chosen
  scope; all those matching rows receive the mapping. Conflicting/malformed table
  entries or blank results prevent Apply. Taxonomy/text tables use the same key
  rule and append the supplied value with the chosen separator. No external
  taxonomy service is queried. IDs, symbols, kinds, traces and selections remain
  unchanged; renamed titles persist through native saves and FASTA export.

## Changes in 0.12

- Three compact icon-only toggle buttons follow the editing and right-click
  dropdowns: identity dots, chromatogram peaks and overview bars. Each has hover
  help, an accessible name and a visible checked state. Buttons and View menu
  items share the same actions; closing a panel or restoring a session updates
  both. The chromatogram button is disabled until linked traces are available.
  Icons are drawn natively at multiple DPI scales; no icon dependency is added.
- Sequence → Select adds title-prefix, title-substring and exact-title-list
  matching, one entry per line. Blank lines are ignored; other characters and
  spaces are literal. Match case is on by default and can be disabled for Unicode
  case-insensitive matching. Exact-title lists use a lookup set for large lists.
- Duplicate names can compare the entire title or its trimmed prefix before the
  first '(' or '|'. Choose all members of each duplicate group or only additional
  copies, keeping the first in the current row order unselected.
- Duplicate sequences compare without regard to case. Ignore gaps is on by
  default; turning it off compares exact aligned symbols and gap types. Masks
  group only with masks. All-gap and empty rows follow the same comparison rule.
- The original first-nongap-in-columns selector was removed in 0.13 at the user's
  request.
- Sequences containing a motif searches nucleotide rows, without regard to case.
  Ignore gaps is on by default; reverse-complement matching and compatible IUPAC
  matching are optional. Exact mode matches literal ambiguity codes; compatibility
  tests shared possible bases (N/? may match any base, T/U are compatible).
  With gaps included, their exact type/position still matters. Masks are excluded.
  Whitespace is ignored in the query; motifs are limited to 4096 searchable symbols.
- Random subset selects an exact requested count from all rows without replacement.
  Zero clears name selection. Results retain the current row order.
- Selection searches run in a worker and replace name selection while preserving
  residue selection. Cancel/invalid input leaves selection unchanged; no matches
  clears name selection. Searches and view toggles do not modify sequence data,
  trace evidence or undo history. Ctrl+F retains name-regex selection.

## Changes in 0.11

- Backspace in Insert/Overwrite deletes the selected cells and selects the symbol
  before the deleted stretch. At the beginning it stays at column 1; an empty row
  has an end caret. Delete keeps selecting the following symbol after suffix
  shifting. Multiple-row deletion remains one atomic edit with no ambiguous
  single-row caret. Both views use the same editing controller and undo.
- Sequence-name selection and residue selection are independent. Clicking or
  horizontally dragging residues selects only that row. Dragging vertically or
  Shift-clicking selects the actual rectangle, including intervening unselected
  rows. Ruler selection covers every row, including masks. Selecting/deselecting
  names preserves the residue selection; selecting in Current preserves names.
  Esc still clears both. Existing Ctrl+C priority remains: selected names copy
  FASTA; with no selected names, a single-row substring copies real text.
- View → Identity dots to top sequence is a render-only toggle. Case-insensitive
  exact nongap symbol matches to the current first row appear as neutral gray
  dots below it. The top row, gaps, differing symbols, mask rows and columns beyond
  the reference retain their symbols. Ambiguities match only the same ambiguity
  code; T/U are distinct. If the top row is a mask, no comparison is made.
  Reordering, editing and undo refresh the comparison. Copy/export, statistics,
  MAFFT and trace evidence retain real symbols. The toggle persists in .kleio
  as optional version-1 view metadata; old sessions default to letters.

## Changes in 0.10

- Current, Imported and Original Q labels have opaque white backgrounds above
  scrolling letters/numbers/caret graphics. Every visible base is still drawn.
- Click/drag/Shift-click in **Current** to select a single symbol or stretch.
  Type, paste, copy and delete using the existing toolbar mode and shortcuts.
  View is read-only; Insert requires an unambiguous single cell and inserts after
  it; Overwrite replaces a single-row stretch and advances. Undo/redo and trace
  provenance use the same alignment commands. Slide arrows retain the
  existing rigid-block rules. Imported calls/qualities remain immutable.
- **Alignment spacing** beside Zoom/Gain maps surviving original peaks to their
  current alignment columns. Gap changes, insertions/deletions and sliding
  automatically stretch/compress display intervals. Current includes gap cells;
  inserted bases have no invented original peak/quality. Deleted original signal
  remains inspectable. **Sample spacing** retains original coordinate spacing.
- The display uses an invertible monotone piecewise-linear transform, with
  original extrema retained during compression and display-only interpolation
  during expansion. Coincident original peaks share a display anchor; original
  evidence arrays are never altered. The precise policy is in TESTING_0.10.md.
- Native sessions save the spacing choice and current edits. Binary session format
  stays at version 1; older sessions default to Sample spacing, and old readers
  can ignore the optional spacing view field.

This completes the planned initial paired editable trace workflow. Manual signal
warping independent of alignment edits, free-residue compression and further
BioEdit selection/rename parity remain future refinements.

## Changes in 0.9

- Every visible Current/Imported base is drawn, even if letters overlap. Inserted
  Current symbols remain visible after the caret moves; they do not invent imported
  calls/quality. Clicking them in Current selects their alignment column.
- File → Open selects multiple AB1/FASTA files into one new alignment document.
  Single-file FASTA behavior and File → Add remain available. Command-line filename
  lists and nearby Explorer launches are grouped; later requests create separate
  windows rather than changing an existing document. Optional per-user Windows
  Open With registration and its verification steps are in TESTING_0.9.md.
- Tiny waveform markers beside sequence names identify linked traces; name hover
  shows the source. Dropdown switching and sequence selection continue to work.
- **.kleio sessions** embed imported analyzed signals/calls/peaks/qualities and save
  current symbols, row kinds/order/IDs, editing provenance, strand and view state.
  They reopen without needing the original AB1 paths. Current edits are retained;
  reopening begins a fresh undo history. The versioned binary format is documented
  in SESSION_FORMAT.md, with bounds/integrity checks and atomic file replacement.
- Save defaults to session format when traces are present. Ordinary FASTA documents
  retain FASTA saving, with an optional session format under Save as. Explicit
  Export FASTA preserves interchange while leaving the native session/save state
  intact. Neither export nor session saving overwrites an AB1; FASTA export also
  cannot overwrite a native session.
- Trace information uses **base calls** and **signal samples/channel**. PBAS/PLOC
  are source-format tags and have moved into the tooltip. A signal sample is one
  amplitude measurement, so each called peak spans several samples.

Existing trace formats such as AB1 and SCF describe individual sequencing reads,
not this editor's multi-row alignment plus original/current edit mappings. A small
application session format preserves those additional relationships explicitly;
it does not replace trace interchange formats. Original AB1 files stay unchanged.
SCF reference: https://staden.sourceforge.net/manual/formats_unix_2.html .

Direct trace editing and alignment-space stretching/compression are the next M7
substage. Signals currently retain native sample spacing; the current release
completes native persistence and the user's requested fixes/opening conveniences.

## Changes in 0.8

- File → Open and Add sequence from file accept FASTA and AB1/ABI. File content
  identifies ABIF, so the importer does not rely solely on its extension. Multiple
  files can be added together, and malformed imports leave the document unchanged.
- A linked **Chromatogram** panel shows the analyzed A/C/G/T signals, imported calls
  and quality values, current symbols, source identity and strand. Selection/caret
  navigation follows the corresponding peak; clicking a peak selects its alignment
  column. Alignment/overview scrolling, a trace row picker, independent trace
  panning, Zoom, Gain and previous/next-call controls support read-only inspection.
  View → Chromatogram controls visibility.
- Original samples, peak positions, chosen imported calls and qualities are
  immutable. Gap operations, rigid sliding, case changes, renaming and reordering
  retain their links. Substitution retains the original anchor and marks the
  current call as edited. Insertions use display interpolation only, without
  inventing an original peak or quality; deleted calls retain their original signal.
  Undo/redo restores both sequence and provenance. Reverse complement reverses
  direction, call mapping and complementary signal channels consistently.
- Whole/column MAFFT and target/template result windows preserve linked trace
  evidence. A whole-row MAFFT strand reversal also reverses the trace orientation.
  Mask rows remain excluded from trace/alignment eligibility as before.

The importer supports ABIF 1.x, validates directory/payload bounds, dimensions,
channel identities, call/peak/quality counts and ordered in-range peak positions.
It reads analyzed DATA9–12 and FWO_ dye order, using a coherent edited PBAS/PLOC1
baseline when present, or PBAS/PLOC2 otherwise. PCON is optional for that same call
set; missing qualities remain unavailable. It never combines calls and qualities
from different sets, guesses missing peak positions, trims or re-basecalls a trace.
Files over 64 MiB and unsupported/malformed trace layouts give a clear import error.

**In the initial 0.8 stage, saving exported FASTA sequence text only.** Trace attachments and their
edit provenance are not stored in FASTA; reopening it does not restore them. AB1
sources are never overwritten by FASTA export. This is the planned first read-only
trace stage: native session persistence, direct trace editing and alignment-space
stretching/compression follow in later stages. Signals use native sample spacing;
existing sequence editing remains in the alignment viewport.

ABIF format reference: Applied Biosystems, *ABIF File Format Specification*,
https://archive.gfjc.fiu.edu/workshops/resources/literature/ABIF_File_Format.pdf .

## Changes in 0.7

- Overview labels are now **Height = % non-gap symbols** and
  **Color = Column conservation**. The conservation calculation is unchanged.
- Insert places typed or pasted text **after** the selected symbol. The last
  inserted symbol stays selected, so repeated typing/pasting remains continuous
  before the untouched suffix. Selecting the final symbol appends; an empty-row
  or row-end caret also appends. Wider selections remain non-editable in Insert.
  Overwrite still replaces the exact selected stretch and selects the next symbol.
- Alignment → MAFFT → **Align selected sequences to templates…** uses full-row
  addition (`--add`). **Align selected fragments to templates…** uses fragment
  addition (`--addfragments`); “fragments” means the selected rows contain partial
  sequences, rather than selecting columns within full rows.

Select target sequence names first, then choose the already-aligned template rows
from a checklist. Targets and templates must be separate, with at least one
nonempty nucleotide row in each group. Selected masks and empty rows are skipped
as targets; they remain among the untouched rows. Duplicate titles are shown with
row numbers. The following settings dialog uses the existing MAFFT installation
and nucleotide parameters. Fragment addition uses Automatic, as recommended by
MAFFT; full addition offers existing presets except PartTree. Since 0.17.1,
fast/accurate strand adjustment is available for targets. Template reversal is
rejected, preserving the original template orientation.

The result opens in a new window, keeping row order, stable IDs, titles and types.
The source and its undo history remain unchanged. Every original column is
retained, including columns containing only template gaps and columns occupied
only by untouched nucleotide/mask rows. Added target insertions introduce new
columns and open gaps in other rows. Original non-target symbols, case, locked
and open gap states retain their exact original-column relationships. Short rows
are padded with open gaps in the result. Target old gap placements are discarded;
every target's original ungapped symbols and case are restored exactly, or
reverse-complemented when strand adjustment explicitly requests it.

Template residue ordinals anchor the column map; titles and repeated motif text
are not used to infer positions. The adapter verifies identities, residue order,
equal output lengths, and that no template column is split, merged or reordered.
It rejects a result that fails preservation, and does not apply raw output.
Cancellation or a failed process also leaves the source untouched.
Fragment addition requires MAFFT 7.526 or later. During validation, the official
7.525 source build failed on multiple-target fragment jobs, including informative
targets; a sanitizer trace localized the fault to its partial distance-matrix
export. The official 7.526 Linux binary passes these same cases. The adapter
checks the version before fragment alignment and gives an upgrade message on
older installations. Whole/column jobs and full-sequence addition retain their
existing version handling.

A deterministic boundary policy resolves template-only gap intervals: restore all
original columns in an interval before new insertion columns, placing those new
columns immediately before the next template anchor. Trailing insertions follow
the complete original axis, including any longer untouched/mask tail. New columns
that are entirely gaps in MAFFT output are omitted. The Last alignment report
records target/template IDs, parameters, original/restored/new column counts,
this boundary policy and preservation status. Neither `--keeplength` nor
`--mapout` is used, because those suppress target insertions.

MAFFT's official description and gap-only-column example:
https://mafft.cbrc.jp/alignment/software/addsequences.html .

## Changes in 0.6

- Drag rectangles and select ruler columns in View, Insert, Overwrite and
  Slide. Shift-click extends from the initial selection anchor.
  Switching modes preserves the selected range. Ctrl+C copies a single-row
  substring as plain text when no sequence names are selected; a multiple-row
  block leaves the clipboard unchanged. Since 0.17.2, selected names do not change
  fragment shortcuts; use Copy selected sequences for FASTA.
- Insert accepts typed/pasted symbols only at a single-cell caret or the row-end
  caret. Single-cell selection is blue; wider or multiple-row selection is yellow.
- Overwrite accepts typed or pasted symbols for a single-row selection of any
  length, replacing that exact stretch and placing the caret on the following
  symbol. The replacement can grow or shrink the row. Single-row selection is
  blue; multiple-row blocks are yellow and reject typed/pasted replacement.
- Delete and Backspace remove the selected stretch or block in Insert/Overwrite,
  shifting each stored suffix left. One undo restores all rows and exact gap
  states. Shorter rows are clamped; mask rows work in mixed selections.
- Double-click a residue to select its entire continuous nongap run, including
  ambiguous calls. Double-click a gap to select its whole run across gap types.
- Esc clears all selected names, the fragment and caret, and cancels a row drag.
  Row-selection commands now appear only under Sequence → Select. Ctrl+F opens
  Names matching regular expression. Ctrl+A retains sequence Select all.
- Every MAFFT parameter has brief hover help, also on its label where applicable.
- Statistics → Alignment statistics opens a compact window with the nine requested counts
  and a Copy button for one labeled, semicolon-separated line. Calculation runs
  in a background task with the source read-only; other document windows remain
  available. Closing the window cancels a pending scan.

Statistics exclude masks from rows, width and patterns. They use IQ-TREE's
nucleotide state conventions: case ignored, U = T; N, ?, gap types and virtual
trailing cells share the missing state for unique sequences and patterns. Partial
IUPAC calls retain their distinct possible-nucleotide sets. A constant site has a
nonempty intersection of possible bases, including compatible ambiguity-only
sites and all-missing sites. Parsimony-informative sites require at least two
definite bases occurring at least twice; the remaining variable sites are
singletons, matching IQ-TREE's report. Gap-only and N-only counts are subsets of
constant sites. N-only means only N/? plus gaps, with at least one N/?. Shorter
rows are treated as missing at their ends; no padding is written to the document.
Statistics do not invoke or require an IQ-TREE installation.

See VALIDATION.md for actual checks, comparisons and platform limits.

## MAFFT features (introduced in 0.5)


- Alignment → MAFFT → Align whole alignment runs all nucleotide rows after
  removing existing gaps. Masks are excluded from the process and their strings
  remain unchanged in the result. Empty nucleotide rows are restored as open-gap
  rows. Titles, row IDs, order, case, U and ambiguous residues are preserved.
- Realign selected columns uses the selected block/ruler's contiguous range
  across **all nucleotide rows**, regardless of selected names. It aligns only
  that region and reassembles the original prefix and suffix. Columns to the
  right move if its width changes; their symbols and relative alignment remain
  intact. Ragged missing cells are materialized as open gaps where a common
  fragment boundary requires them. Strand adjustment is disabled for fragments.
- Masks participate only in result assembly: their fragment text stays intact,
  with open gaps appended if the region grows. Their prefix/suffix move with the
  other rows. A region cannot shrink below its longest stored mask fragment;
  annotation characters and locked/open gaps are never truncated.
- The result opens immediately in a separate window. The source retains its
  file state and undo history; result edits have independent undo history. A
  result is an unsaved new baseline, with the source still available beside it.
  File → New window is available while MAFFT runs, for working on another file.
- The source window is read-only during export, alignment and validation.
  Export/parsing/assembly run in background tasks; MAFFT runs as a separate
  process. A progress/log dialog supports cancellation, including descendants.
  No output is applied on cancellation, launch failure, nonzero exit or invalid
  output. Residue identity/order, synthetic row IDs and equal output widths are
  checked before creating the result.
- Settings include Auto, FFT-NS-1/2/i, L/G/E-INS-i and PartTree presets; threads;
  iteration/tree counts; gap opening and MAFFT offset; Kimura scoring distance
  and transition/transversion ratio; composition, memory saving and repeatable
  refinement. E-INS-i defaults to offset 0. Whole-alignment strand options are
  keep, fast adjustment and accurate adjustment. Reversed rows are validated
  against their reverse complement and recorded in the report.
- Alignment → MAFFT → Last alignment report lists version, effective arguments,
  source revision/scope, mask count, elapsed time, reversed row IDs and the last
  64 KiB of logs. The launcher and thread choice are remembered between sessions.

MAFFT 7 is required. Current supported MAFFT versions are recommended; the
official source page warns against versions 7.463–7.486 because of a memory bug.
MAFFT is **not bundled** with this archive. On Windows select `mafft.bat` from
the official all-in-one installation, retaining its complete folder. Linux/macOS
can use an executable MAFFT launcher. PowerShell and WSL wrappers are not included
in this first adapter.

Existing open and locked gaps within the realigned scope are both removed before
alignment; newly generated gaps become open `~` gaps. Column-job prefix/suffix
gap states are preserved exactly. The initial alignment stage is not a constraint
aligner for locked gaps. Version 0.7 adds the separate template-based workflow
described above. Version 0.8 retains AB1 trace provenance through both workflows.

References for parameter/launcher behavior:
- https://mafft.cbrc.jp/alignment/software/manual/manual.html
- https://mafft.cbrc.jp/alignment/software/preservecase.html
- https://mafft.cbrc.jp/alignment/software/adjustdirection.html
- https://mafft.cbrc.jp/alignment/software/multithreading.html
- https://mafft.cbrc.jp/alignment/software/windows_without_cygwin.html
- https://mafft.cbrc.jp/alignment/software/source.html

## Changes in 0.4

Historical: Alphabetically was removed in 0.13.1; the identical sorting option
was replaced by Sequence → Groups → Group identical sequences in 0.17.1.

- Select names with Ctrl/Shift, then drag a selected name to reorder the group.
  A blue insertion line shows the destination; edge dragging scrolls vertically.
  Noncontiguous rows move together in their existing relative order. A normal
  click still selects one row; Escape cancels a drag. Each drop has one undo step.
- Sequence → Sort offers all nine requested methods. Sorts act on all rows and
  retain selected row IDs. Equal keys retain their current order. Title sorting
  ignores case, including Unicode titles. Alphabetically sorts the aligned
  sequence text, ignoring letter case but retaining gap characters in the key.
  Physical length includes stored gaps; true length excludes both ~ and -.
  Group by identical sequences groups case-insensitively equal aligned strings,
  with nucleotide and mask groups kept separate, preserving first-group order.
  Reverse order reverses the whole list.
- Frequency sorting needs a selected column range in Slide. It sums
  each row's nucleotide frequencies in those columns and sorts highest first.
  Frequencies come from all nucleotide rows, with U treated as T and partial
  IUPAC calls divided equally among their possible bases. N, ?, gaps and missing
  cells score zero. Mask rows are placed after nucleotide rows. A selected block's
  columns define the range, regardless of its selected rows.
- Sorting computes the order in a background task. Row movement and undo swap
  existing rows, preserving IDs and symbols without copying sequence strings.
- FASTA rows containing unexpected printable ASCII symbols become masks. The
  entire row uses dark gray, including any A/C/G/T characters it contains.
  Masks support selection, mixed blocks, sliding, gap tools, Insert, Overwrite,
  deletion and raw-text paste. Their kind persists through edits and undo/redo.
  Reverse complement skips masks; the core exposes nucleotide-only eligibility
  for future alignment integration. Masks do not affect overview height or color.
- Two- and three-base IUPAC calls now contribute fractional conservation votes:
  R splits between A/G, B between C/G/T, and similarly for other partial calls.
  N and gaps contain no base information. The legacy ? placeholder behaves as N.

Mask cells currently accept printable ASCII other than spaces and the FASTA
header delimiter >. FASTA whitespace is stripped and dot gaps become ~, as for
nucleotide rows. Non-ASCII cells, control characters and malformed FASTA still
produce errors; UTF-8 titles remain supported. FASTA stores symbols but no row
kind: an edited mask containing only valid nucleotide symbols will reopen as a
nucleotide row. Native session persistence remains a later stage.

## Changes in 0.3.1

- The rename dialog is wider (720 logical pixels) for descriptive headers.
- Hover over the vertical name/residue divider for the resize cursor, then drag
  it to adjust the name area. Names are re-elided to fit. The minimum width is
  80 pixels, with room retained for at least four residue cells. The width stays
  in effect when switching documents during the current session.
- The ruler reserves its own header height. Font size 6 no longer clips numbers;
  sequence rows retain their compact spacing. Selection/caret coordinates and
  scrolling follow the new layout.
- Overview bar height now shows the fraction of rows containing a residue,
  including ambiguous calls. Both gap types and missing cells have zero height.
- In 0.3.1 conservation excluded all ambiguous calls; 0.4 includes partial IUPAC
  calls and continues to exclude N. An A/N column is as blue
  as an all-A column; all-N columns are gray with full residue height, and all-gap
  columns have only a gray baseline. No-information columns are also excluded
  from the color average when several columns share a pixel.
- Colors use five discrete levels rather than a continuous muted gradient:

| Majority nucleotide fraction among called bases | Color |
|---|---|
| 100% | Blue |
| 90% to below 100% | Light blue |
| 75% to below 90% | Muted teal |
| 60% to below 75% | Golden orange |
| Below 60% | Bright orange |
| No informative nucleotide votes | Gray |

## Changes in 0.3

- Ctrl+C with no selected names copies a single-row residue fragment as plain
  text, preserving case and gaps. Multiple-row blocks leave the clipboard unchanged.
  Selected names continue to take priority and copy FASTA in document order.
- Ctrl+V in Insert mode inserts raw nucleotide text at the caret. Whitespace
  is removed; IUPAC symbols, U, N, ?, ~ and - are accepted, and dots become ~.
  Unsupported input is rejected as a whole. Paste is one undoable edit and advances
  the caret. Raw-text paste needs a caret and does not edit in other modes.
- FASTA text and copied local FASTA files still add sequences. File → Add from
  clipboard and New alignment from clipboard retain their existing purposes.
- The ruler has a tick for every column, longer ticks at multiples of 5 and
  longest ticks with numeric labels at multiples of 10. Coordinates start at 1.
- The bottom alignment overview spans every column. In 0.3.1 bar height shows
  residue density, and stepped color shows nucleotide conservation, with no-call
  regions gray. A narrow color strip also identifies empty columns.
  Click or drag the plot to center the alignment view there; the outlined frame
  tracks the visible columns. Hover for column ranges and percentages.
  View → Alignment overview hides or shows it.

Overview statistics use all nucleotide rows, regardless of selection, excluding
masks entirely. Residue height is nongap cells divided by nucleotide-row count.
Per-column conservation is the largest A/C/G/T vote total divided by all base
votes; case is ignored and U votes as T. A/C/G/T/U contribute one vote; partial
IUPAC symbols contribute one vote shared equally among their possible bases.
N and ? contribute to residue height but not color; gaps and missing cells
contribute to neither. A/R therefore has conservation 75%, all-R 50%, and A/N
100%. Adding a mask row cannot change any of these percentages.
Pixel bins average residue percentages over all columns and conservation only
over informative columns. Bins with no called bases use gray.

Calculation yields to input between short slices and restarts after sequence
edits, row additions/deletions, or undo/redo. Renaming, scrolling, selection and
font changes reuse cached statistics. During calculation the overview displays
progress instead of a stale plot. The viewport frame and navigation remain usable.
The scan stores about 20 bytes per column and does not copy the alignment.

## Changes in 0.2.1

- Double-click a sequence name to rename that specific row, with undo.
- Clicking blank space below the rows in the name panel clears row/fragment selection.
- Shift+click forms a rectangle across rows and columns from the original clicked
  cell; further Shift+clicks can shrink or reverse that rectangle.
- Drag/arrow slides push residues and locked dashes, consume external open gaps,
  and extend rows/the alignment to the right when needed. Internal block symbols
  and spacing are preserved. Groups retain one shared displacement.
- Edit → Copy sequence titles and Copy selected sequences (FASTA format) export
  selected names in document order, retaining complete headers, duplicate names,
  case and gap types. Version 0.3 adds the single-fragment plain-text behavior above.
- The font dialog has only fixed-width family, size, Regular/Bold/Italic/Bold
  Italic style and a preview. No underline, strikeout or writing-system controls.

## Slide behavior

Choose Slide. Click then Shift+click the opposite corner, or drag a
rectangle. Drag inside it or use Left/Right to slide. Escape clears the fragment.
For named noncontiguous rows, select those names first, then drag horizontally
in a selected row. Dragging vertically selects intervening rows. Ruler dragging
selects columns for selected names, or all rows if no names are selected.

The rectangle stays rigid. When moving right, encountered residues/dashes are
pushed right; the nearest downstream ~ gaps absorb movement. Once these run out,
rows grow. When moving left, preceding residues/dashes are pushed left and the
nearest upstream ~ gaps are consumed. Movement is clamped only when a selected
row cannot fit its preceding non-open-gap content before column 1. Vacated space
is filled with ~. Internal gaps are never consumed. Missing cells are virtual ~,
materialized only when touched.

The existing Ctrl+Left/Right and maximum-movement context actions retain their
previous purpose: **jump through adjacent open gaps** until the next residue or
locked dash (or the current alignment boundary). They do not request unbounded
pushing. Ordinary arrows and dragging implement the corrected pushing behavior.

The right-click dropdown must be “menu” for fragment actions (including N/gap
fill). Its four gap tools act immediately and shift downstream content as a
block. Gap deletion still removes only ~; unlock - before deleting it that way.

Sequence transformations act on selected names. Degap removes ~ only. Unalign
removes both gap types across the document. Minimize removes global all-gap
columns. Reverse complement preserves case/IUPAC/gap types; rows with U and no T
use RNA complements, otherwise DNA complements.

All sequence edits undo/redo atomically. Recent fragment selection positions are
restored for 128 recorded fragment states; older undo may retain current selection
coordinates. FASTA does not store selections.

## Retained functionality

Direct FASTA open/save and additions from file/clipboard; new sequence dialog;
full UTF-8 headers, duplicate names, case and ragged lengths; View/Insert/Overwrite;
row selection/deletion; flush; tooltips; undo/redo; wheel and Ctrl+wheel scrolling;
cached glyph/tile rendering; fonts; individual/table rename; separate name and
aligned-sequence regex row selection. Regex includes gaps, is case-sensitive by
default and accepts (?i). Motif highlighting/navigation is still pending.

A/a uses dark yolk yellow #B47D00, C blue, G black and T/U red. Gap/ambiguity colors
remain provisional. Open (~) and locked (-) gaps are stored separately. Imported
dot gaps normalize to ~. Unexpected printable symbols classify imported rows
as masks in 0.4.

## Limits and remaining work

Free-residue compression mode, manual independent trace warping, full
BioEdit selection/rename parity, tabs and further session/trace refinements remain pending. Protein
features remain low priority. Whole-row transformations, minimize, clipboard
export and integration of added rows currently execute on the UI thread; huge
operations have not been latency-benchmarked. The logical undo budget is 64 MiB;
the newest operation is retained even when it exceeds that limit.

## Build on Windows using Qt Creator

1. Install the Microsoft C++ build tools with **Desktop development with C++**,
   including a 64-bit MSVC toolset matching the Qt kit. MSVC 2022 is the initial
   recommended kit.
2. Use the Qt Online Installer to install a supported Qt 6 release with the
   matching **MSVC 2022 64-bit** desktop package, Qt Creator, CMake, and Ninja.
   Qt Widgets and Qt Concurrent are needed; no QML, WebEngine, or Python is needed.
3. Extract this archive. In Qt Creator, open the top-level **CMakeLists.txt**.
4. Select the matching desktop kit, configure, and build the **kleiotide** target.
5. Run it and use File → Open to open **examples/nucleotide_demo.fasta**.
6. Use the included core test target before testing your own editing workflows.

Version boundaries: application code requests Qt >= 6.5 and CMake >= 3.21, but
the chosen Qt release may impose higher tool requirements. Use the CMake/Ninja
tools provided with that release; do not infer compatibility from these minimums.

Official setup guidance:
- https://doc.qt.io/qt-6/get-and-install-qt.html
- https://doc.qt.io/qt-6/windows.html
- https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170

Do not mix a MinGW Qt library package with an MSVC build. A complete matching
MinGW kit is an alternative if needed; it does not change the source architecture.

## Command-line build (when Qt and CMake are installed)

Use a developer terminal appropriate to your Qt/compiler kit.

~~~text
cmake -S . -B build -DCMAKE_PREFIX_PATH="<path to your matching Qt kit>"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
~~~

For a core-only build, without Qt:

~~~text
cmake -S . -B build-core -DKLEIOTIDE_BUILD_GUI=OFF
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
~~~

A direct g++ build, as used in the development environment:

~~~text
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Isrc src/core/alignment.cpp src/core/fasta.cpp src/core/overview.cpp src/core/sorting.cpp src/core/mafft_alignment.cpp tests/core_tests.cpp -o core_tests
./core_tests
~~~

No Windows executable is bundled. The source archive deliberately excludes build
products and compiler-specific binaries.

## First interaction check

After compiling the Qt target:
1. Open the demo FASTA; confirm short rows are not padded on opening.
2. Copy a two-record FASTA text and paste; undo the append.
3. Copy a FASTA file in Explorer and paste it into the name panel.
4. Select multiple names using Shift/Ctrl; delete, undo and redo.
5. Choose Insert, click a residue, type N or ~, delete, then undo.
6. Choose Overwrite and confirm the row length stays fixed except at its end.
7. Check vertical wheel and Ctrl+wheel scrolling on a wider/longer fixture.
8. Save as a new FASTA; reopen and compare names, case and gap symbols.
9. Modify a file, request Open/New/Close, and test Save/Discard/Cancel.
10. Test paths containing spaces and Cyrillic characters.

Use a small copied fixture for this first check. Report compiler diagnostics
verbatim and describe any unexpected interaction with a tiny before/after example.

## Structure

- src/core: GUI-independent data model and FASTA adapters.
- src/gui: Qt desktop shell and visible-area alignment rendering.
- tests: deterministic and randomized core correctness checks.
- tools: synthetic core benchmark, optional raster viewport check and GUI workflow check.
- examples: nucleotide, sliding, conservation and mask/sorting fixtures.
- VALIDATION.md: measured results and important verification limits.
- NEXT_STEPS.md: user inputs and subsequent implementation work.

No public application license has been selected. See LICENSE-STATUS.md.


## Optional developer GUI checks

Build with `-DKLEIOTIDE_BUILD_VIEWPORT_CHECK=ON` and/or
`-DKLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON` to include the validation executables.
`kleiotide_viewport_check` checks actual raster backing-store pixels against a
full repaint, then measures a synthetic 95-row, 1792-column alignment at 3440×1440.
`kleiotide_gui_smoke` exercises asynchronous opening, regex selection, table and
single renaming, contextual clipboard shortcuts, overview hide/show and undo/redo
through the real window actions and dialogs. `kleiotide_overview_check` verifies
overview statistics, stepped color/residue-height pixels, navigation, resizing,
revisions and ruler tick heights. `kleiotide_view_layout_check` verifies divider
dragging/limits, preserved selections, font-6 digit rendering and cell/caret hits.
`kleiotide_stage04_check` covers mask glyph colors, mixed blocks, mask typing
and paste, selected-group drag/drop, Escape/autoscroll, all nine sort menu actions,
Unicode title ordering and atomic undo/redo through the actual window.
`kleiotide_mafft_check` checks process failures/cancellation, output validation,
settings, result/report windows and real MAFFT presets/direction when a launcher
is supplied. Its deterministic `kleiotide_mafft_fixture` subprocess is a test
fixture only, not an aligner or replacement for MAFFT. Example:

~~~text
kleiotide_mafft_check <absolute path to kleiotide_mafft_fixture> <absolute path to mafft> [settings screenshot.png]
~~~
For an optional large scan/GUI heartbeat measurement, run
`kleiotide_overview_check overview.png 5000 100000` (about 500 MB of sequence data).

On Linux use `QT_QPA_PLATFORM=offscreen`; for viewport scaling checks additionally
set `QT_SCALE_FACTOR` to 1, 1.25, 1.5 or 2. Timings are local measurements, not FPS
guarantees. These optional tools are not needed to build or use the application.

`kleiotide_stage06_check [statistics screenshot.png]` covers all-mode mouse/ruler
selection and colors, substring typing/paste/deletion/undo, double-click runs,
actual Ctrl+C/Ctrl+V/Ctrl+F, Escape, parameter help and statistics/copy/cancellation.
