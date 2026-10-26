# Next implementation steps and inputs

Version 0.26.6 adds actual-group color markers in the manager's sequence
list and Check all / Uncheck all labels. See TESTING_0.26.6.md.

Version 0.26.5 adds live group-member counts and Delete empty groups in the
group manager, retaining staged edits and Undo. See TESTING_0.26.5.md.

Version 0.26.4 adds sequence/group column selectors for multi-column group
imports, header labels and legacy two-column defaults. See TESTING_0.26.4.md.

Version 0.26.3 fixes group-table matching for parenthesized and standard
TSV-quoted cells; internal parentheses are retained. See TESTING_0.26.3.md.

Version 0.26.2 adds Ungrouped one/all selection, manager-order previews,
background model-based group import, confirmed clearing and mouse-drag subset
membership controls. See TESTING_0.26.2.md. The user confirmed that 0.26.1
clustering is much faster and fast/exhaustive differences are tiny in their data.

Version 0.26.1 adds word-ranked centroid candidate search and faster affine DP,
preview TSV export, empty-old-group cleanup with Undo, and filtered bulk member
checks. See TESTING_0.26.1.md and IDENTITY_GROUPS.md. Native Windows acceptance
remains pending; no external clustering dependency was added.

Version 0.26 adds previewed identity-based greedy grouping (existing alignment
or internal pairwise semiglobal alignment), first/longest/random selection per
group, and residue-only idle status information at bottom left. See
IDENTITY_GROUPS.md and TESTING_0.26.md. The user confirmed 0.25.2 works as expected.

Version 0.25.2 removes conflict navigation buttons, recognizes composite terminal
N/?/gap regions and prioritizes minimum-edit eligible overlaps. See
TESTING_0.25.2.md; minimum matching support remains a required constraint.

Version 0.25.1 adds conflict/case preview refinements, bounded edit-tolerant
partial-contig merging and narrowly validated stale-offset AB1 import recovery.
See TESTING_0.25.1.md, MERGING.md and AB1_IMPORT_RECOVERY.md.

Version 0.25 adds aligned/IUPAC and exact-overlap sequence merging, including
optional original-Q conflict resolution. See MERGING.md and TESTING_0.25.md.
Native Windows acceptance remains pending; the release-stage order is unchanged.

## Current checkpoint — 20 October 2026

0.24.3 adds a draggable batch-preview divider and prioritizes output filenames
in narrow windows, restoring the chosen source-name width when widened.
Acceptance: TESTING_0.24.3.md. The release-stage order remains unchanged.

0.24.2 adds a configurable batch-export filename suffix (default _edited, optional
empty suffix, live preview, safe collision handling). The user confirmed 0.24.1
opens correctly in BioEdit and Chromas and all suggested tests pass. Acceptance
for the small suffix refinement is in TESTING_0.24.2.md.

0.24.1 addresses the user-confirmed BioEdit export failure, while retaining the
Chromas-tested call/signal representation. It adds previewed batch export of
All/Selected traces with collision avoidance, cancellation and per-file reports.
See TESTING_0.24.1.md. BioEdit's binary parser passes locally; native Windows
acceptance is still required. The user confirmed Chromas opens 0.24 exports.

0.24 adds edited AB1 export: current calls, interpolated inserted positions,
conservative Q and exact analyzed signals in current orientation, including from
portable sessions. See AB1_EXPORT.md and TESTING_0.24.md. User accepted 0.23.

0.23 completes stage 9: previewed duplicate collapse, chosen representatives,
concatenated names or _xN count, and one-step Undo. User declined independent
residue compression; it is removed from scope. See COLLAPSE.md and TESTING_0.23.md.

0.22 implements stage 8: protein MAFFT for whole/selected/column alignment and
full/fragment template addition, explicit molecule validation, protein scoring
controls and lossless integration. Similarity sorting explains N exclusions.
See MAFFT_PROTEINS.md and TESTING_0.22.md.

0.21.1 adds sample SD and naive SE for all mean-distance reports, with explicit
formulas/limitations, and centrally generated version provenance in distance and
composition TSV. Acceptance: TESTING_0.21.1.md. Protein MAFFT followed in 0.22.

0.21 implements roadmap stage 7: distance matrices, overall/within-/between-group
means and observed-difference nearest-neighbor ordering. Analysis contracts are
in DISTANCES.md; Windows acceptance steps are TESTING_0.21.md. The user confirmed
0.20.3 (including translation annotations) before this milestone.

0.20.3 adds annotation-mask generation/import (including the mask context action),
trailing partial-codon calls shared by display/copy/output, group-name renaming,
stable type sorting and menu organization. Native format remains 4.
Acceptance: TESTING_0.20.3.md. The next planned scientific stage is unchanged.

0.20.2 applies the latest translation simplification: single-letter display only,
centered ordinary protein additions with independent gaps, legacy-span migration,
distinct family shades, panel spacing and automatic translated-copy expansion.
See TRANSLATION.md and TESTING_0.20.2.md. Native sessions remain format 4.


0.20.1 fixes user-reported translation controls and missing amino-acid colors.
All residues now use fixed Clustal-style colors independent of conservation and
gaps; code dropdowns are read-only with concise names and a final Custom code
command. Copy-translated eligibility and the AA/coding controls are refined.
Native format remains 4; acceptance: TESTING_0.20.1.md.


Stage 6 is implemented in 0.20: protein coloring; live translation with genetic
codes, frames, coding annotations and stop counts; selected-name protein
snapshots; compact protein output; canonical protein-span editing; format-4
persistence and format-1/2/3 migration. See TRANSLATION.md, TESTING_0.20.md and
PROTEIN_TRANSLATION_PLAN.md. Local automated checks precede Windows acceptance.
Profiles currently rebuild cooperatively after content edits; affected-codon
incremental recomputation remains a scale optimization, not a paint/selection path.

1. Check the 0.24.3 preview divider; 0.24.1 BioEdit/Chromas acceptance is complete.
2. Reliability, scale, recovery review, documentation and feature freeze.
3. Windows installer, clean-machine candidate testing and 1.0.

Residue compression is removed from scope. Earlier checkpoints below preserve
historical plans and are superseded by this current checklist.

ROADMAP_1.0.md is the detailed plan. Older checkpoints below are historical.

## Version 0.3 checkpoint
User confirms all 0.2.1 corrections, including block pushing and Ctrl+arrows, on
Windows. Single-fragment plain Ctrl+C, Insert-mode raw nucleotide Ctrl+V and
1/5/10-column ruler ticks are added. The next stage introduces an interactive
full-alignment overview with gap-height/conservation-color, viewport framing,
click/drag navigation, tooltips, hide/show and cooperative calculation slices.
MAFFT, chromatograms, identity dots and free-residue compression remain pending.

## Version 0.3.1 checkpoint
User confirms 0.3 copying, pasting, ruler and overview updates/navigation/hide-show
on Windows. Font-6 ruler clipping is corrected with separate header geometry.
The rename dialog is wider, and the name divider is draggable. Overview height
now shows residues; conservation excludes ambiguous calls and no-information
columns, with five discrete orange-to-blue levels and gray for no called bases.

## Version 0.4 checkpoint
User confirms the adjustable name divider and requests partial IUPAC conservation,
drag/drop row reordering, nine sorting methods and non-nucleotide mask rows.
These are implemented: partial calls contribute equal fractional base votes;
masks retain their type while editing, render gray, share block/gap/edit behavior,
and are excluded from reverse complement and overview statistics. Future alignment
integration must use nucleotide-only eligibility and leave masks untouched.
Sort keys are calculated asynchronously; row reordering uses compact swap history.
Windows interaction feedback is the next validation step. MAFFT is still pending.

## Version 0.5 checkpoint
User confirms all 0.4 features and authorizes the next stage. The original plan's
external MAFFT/new-result-document workflow is now implemented for the whole
alignment and all-row selected-column realignment. Settings expose presets,
nucleotide scoring/gap parameters, threads and whole-row direction adjustment.
Output validation restores exact residue case and titles, preserves IDs and
outside-fragment context, excludes masks from alignment, and prevents truncated
mask annotations. Results open in separate windows; cancellation and failed jobs
leave the source unchanged. Windows batch runtime verification is still needed.
Next MAFFT work is lossless target/template integration, preserving every original
column and non-target relationship without --keeplength insertion loss.


## User feedback checkpoint, 8 October 2026
- Windows hardware: Ryzen 7 6800H, 32 GB DDR5, RTX 3070 Ti Laptop; 3440×1440 screen.
- Insert mode explicitly confirmed working after the original report.
- 0.1.1 addresses viewport rendering and clipboard labels, adds table/single rename,
  font controls and separate name/sequence regex row selection.
- Remaining Selection menu parity: title prefix/substring/exact lists; duplicate
  title variants (full, before '(' or '|'); case-insensitive duplicate sequence
  selection; title concatenation + deduplication; numeric duplicate-title suffixes;
  first non-gap in selected columns; subsequence including reverse complement;
  random selection. Separate name/sequence regex row-selection actions now exist.
- Remaining Rename work: space/underscore exchange; supplied taxonomy and append;
  filename insertion; newline replacement; reversal; stripping unique suffixes;
  row-number and sequence-length names. Exclude NCBI/GI/GenBank-related features.
  Keep applicable Extras actions directly under Rename.
- Current colors use A dark yolk yellow, C blue, G black, T red; exact
  ambiguity/gap colors remain provisional.
- Slide example 758–773 -> 788–803 shows a 16-column block moving +30 columns,
  preserving internal symbols and stopping before the downstream CAA. A preceding
  example involves locked dashes and a selected single G. Static screenshots do
  not establish all modifier, drag or compression semantics.
- Both AB1 files have ABIF headers, PBAS2/PLOC2 and four analyzed trace channels.
  One has 1622 calls / 17242 samples per channel; the other 580 / 6998.
  Directory metadata inspected only; no AB1 viewer/import is implemented yet.

## Version 0.2 checkpoint
Dark yolk-yellow A (#B47D00); rigid Select/Slide; fragment fill/max movement;
right-click gap tools; lock/unlock; nucleotide transformations; minimize/unalign.
Reference +30 shift and offscreen event checks pass. Rigid sliding treats external
locked dashes as barriers, preserves internal gaps, and limits groups uniformly.
User feedback is needed for exact free-residue/compression behavior before that
separate regime is added. The overview is the next substantial new visualization.

## Version 0.2.1 correction
User-supplied screenshots supersede the barrier interpretation: rigid fragments
push external residues/locked gaps and consume external open gaps. Leftward fit
protects the beginning; rightward overflow grows the alignment. Shift+click now
selects across rows from a stable original corner. Double-click rename, blank
name-panel deselection, title/FASTA copying and a minimal font dialog are included.
All previously confirmed right-click actions retain their behavior.

## Version 0.7 checkpoint
User confirms 0.6 and authorizes the next stage. Full-row and fragment target
addition against chosen templates is implemented, using the M6c preservation
contract from the original architecture plan. The complete original axis is
retained and non-target rows (including masks) are projected exactly, with open
gaps only for new columns/virtual ends. Template residue ordinals verify anchor
relationships, even for duplicate titles and repeated motifs. New target
insertions are retained; neither --keeplength nor --mapout is used. Results open
separately and failed/cancelled/unverified output does not change the source.
The boundary policy and preservation counts appear in the report. Fragment
addition uses Automatic and requires MAFFT >=7.526 after an older-engine failure
was isolated and verified against the current official binary. Full addition
excludes PartTree; strand stays fixed.
The overview labels and Insert-after semantics are also updated.
Next substantial planned stage is ABIF import and linked chromatogram viewing;
free-residue compression still needs its own concrete movement semantics.

## Version 0.8 checkpoint
User confirms 0.7 and authorizes the next stage. The M7 first delivery now imports
ABIF 1.x analyzed Sanger traces and provides read-only paired sequence/trace
navigation, row selection, native-sample pan/zoom/gain, imported/current calls and
quality display. Immutable evidence and residue-ordinal provenance survive existing
alignment edits, row/history operations, reverse complement and MAFFT-derived
results. Inserted residues receive display-only interpolated anchors; substitutions
are flagged and do not inherit original quality as their own; deleted original
calls remain inspectable. The original AB1 is never overwritten by sequence export.
FASTA still stores sequence text only. Next M7 work: define/implement a versioned
native session format retaining original evidence and edit provenance, then direct
trace editing and alignment-coordinate stretching/compression. The first viewer
retains native sample spacing. Neither research AB1 file is bundled in the release.

## Version 0.9 checkpoint, 12 October 2026
User confirms 0.8 trace import/add/switching and reports culled imported/current
letters and inserted labels disappearing off selection. Both are corrected;
Current draws every mapped/synthetic visible base independently of selection.
Multi-file Open, filename-list launches and grouped nearby Explorer requests are
implemented without changing previously opened work. Linked names have tiny
waveforms with source tooltips. Optional per-user Open With registration is
packaged, requiring Windows verification. Portable versioned .kleio sessions embed
analyzed trace evidence, current edits/provenance, row IDs/order/kinds and view
state. Original source paths are provenance, not dependencies. Reopening starts a
fresh undo history; subsequent editing works. Explicit FASTA export preserves
interchange. Native persistence is now complete for the existing editor state.
Next M7 substage: direct trace editing and alignment-space stretch/compression.

## Version 0.10 checkpoint
User confirms 0.9 works without encountered bugs and authorizes the next stage.
Fixed opaque labels resolve the supplied scrolling screenshot. Current supports
click/drag/Shift-click selection and shared editor typing/paste/copy/delete, modes,
arrow sliding and undo/redo. Sample/Alignment spacing toggles native inspection
versus automatic monotone display warping tied to current column provenance.
Original analyzed evidence is immutable; gaps/inserted cells have their own aligned
positions, deleted peaks remain inspectable, reverse complement is consistent.
The spacing choice persists as optional view metadata in version-1 sessions.
Next: Windows interaction feedback on these trace semantics; further editing/view
parity (including identity dots), and a separately specified free-residue regime.
Manual independent trace warping remains a possible refinement, not implemented.

## Version 0.11 checkpoint
User confirms 0.10 works and requests directional Backspace and independence of
residue/name selections. Backspace now deletes the selected single-row stretch
and selects its predecessor (clamped at the beginning); Delete retains the
following-symbol behavior. Block deletion is atomic and leaves no ambiguous
single-row caret. Name commands/clicks and trace navigation preserve the other
selection; residue clicks never inherit named rows and the ruler covers all rows.
Esc clears both and existing clipboard priorities remain. Identity dots to the
current top row complete planned B23 as a cached rendering option, preserving
actual symbols and gap types. Masks are excluded from comparison, including a
mask reference. The view toggle persists in portable version-1 sessions.
Next: Windows feedback on 0.11; remaining selection/rename parity, with a separate
concrete specification needed for the free-residue/compression regime.

## Version 0.12 checkpoint
User confirms 0.11 and asks for intuitive toolbar view toggles next to the editing
dropdowns. Identity, trace and overview buttons share their View-menu actions,
including tooltips, checked state, panel-close synchronization and native restore.
The next M5 selection substage adds literal Unicode title-list filters, duplicate
title variants, case-insensitive duplicate sequences, first-nongap-in-columns,
nucleotide motif matching with optional reverse complement/IUPAC compatibility,
and exact-size random subsets. Searches replace name selection across all rows,
preserving residue selection and leaving data/history/evidence unchanged.
The model motif matcher maps its first hit back to actual alignment columns;
a full interactive hit browser is not yet exposed. Next substantial programming
work: remaining batch-rename functions and preview/atomic-undo behavior. Free
residue compression still needs a concrete independent movement specification.

## Version 0.13 checkpoint
User confirms 0.12 except for the start-column selector and explicitly requests
removing it without a fix. The action, unused predicate and check are removed.
The identity toggle now depicts colored A/C/G/T above neutral dots at multiple
DPI scales. M5 batch renaming adds the inventoried applicable title transforms,
numeric duplicate suffixes, filename text, row/length names and taxonomy/text
append tables. Exact replacement tables retain their simultaneous matching but
now report missing/multiple matches before Apply. The wide preview displays old/
new names and duplicate results, is invalidated by option changes and commits
one atomic name-only operation. Cancel/error/no-op does not affect data/history;
Unicode reversal preserves grapheme clusters, traces and stable row IDs survive.
Headers already reject newline/null, so literal text replacement covers usable
header text without adding an ineffective newline-specific command.
Next: Windows feedback, motif-hit navigation and further explicitly specified
name-concatenation/deduplication behavior. Independent free-residue compression
still needs concrete movement semantics.

## Version 0.13.1 checkpoint
User confirms 0.13, requests removing Alphabetically sorting and adding Sanitize
names, and queues more accessible color/view options for the next stage. Sorting
by sequence symbols is removed; title sorting remains. Sanitization defaults to
all rows, uses ASCII letters/digits/underscores, replaces unsupported runs, supplies
a fallback and allocates case-insensitive unique numeric suffixes. Original safe
names and existing numbered bases are reserved before allocation; scope changes,
preview and one-step undo retain the established batch workflow. The operation is
idempotent and preserves row IDs, symbols, kinds and immutable trace evidence.
No extra user input is needed for the new display stage. Concrete semantics are
still required only for the separate free-residue movement regime, when pursued.

## Version 0.14 checkpoint
User confirms 0.13.1 and authorizes the display stage. Four palettes are available:
the accepted Classic default, custom High contrast, Okabe–Ito and Paul Tol bright.
Colored letters, monochrome and colored backgrounds share synchronized View and
toolbar actions. Backgrounds choose black/white symbols for contrast; selections
keep blue/yellow cues and masks stay neutral. Palettes update channel curves,
legend and icons; overview conservation retains its independent scale. Optional
native view fields preserve choices without changing symbols or trace evidence.
Exact colors, source attribution and contrast adaptations are in DISPLAY_COLORS.md.
The next independent feature is motif-hit navigation. Free-residue compression
and title concatenation/deduplication still require their specific behavior to be
agreed when pursued.

## Version 0.14.1 checkpoint
User confirms 0.14. The default label is Kleio with its legacy session ID retained.
Chromas-style adds the exact requested A green/C blue/G black/T red palette;
base inks are not darkened. All five palettes share View/toolbar/trace/session
behavior. The overview uses the requested conserved-blue to variable-orange
explanation. ROADMAP_1.0.md replaces stale immediate-work entries with a current
ordered path to a tested Windows installer and release.

## Expanded 1.0 scope, 15 October 2026
User confirms 0.14.1 and explicitly moves basic protein support into 1.0. The
roadmap now includes unwrapped FASTA, two trace-quality commands, site highlighting,
persistent groups with TSV/context/select/sort workflows, a Statistics menu,
composition and distance/group analyses, observed-difference similarity sorting,
protein editing/coloring, both translation layouts and protein MAFFT parity.
Foundational molecule types/session migration and groups precede dependent
analyses. Clustal-style coloring is conservation-dependent per the readable
reference. This is a planning update; none of those new functions are claimed
implemented. Earlier protein deferrals are superseded for basic 1.0 support.

## Version 0.15 checkpoint
User authorizes the expanded roadmap's next stage. FASTA copying/saving is now
unwrapped through a shared record writer, while arbitrary wrapped input remains
accepted. Edit Find motif provides cancellable next/previous navigation through
overlapping and reverse/IUPAC hits, selected-name/all-row scope, wrapping and an
independent purple cue. Select match explicitly updates residue selection. Search
state is invalidated on document/settings/scope changes and remains transient.
Model, GUI, raster and sanitizer checks pass; TESTING_0.15.md covers Windows.
Next implementation milestone: explicit molecule types and native-session
foundations, before groups and protein editing. No new input is needed to start.

## Version 0.15.1 checkpoint
User confirms 0.15 and requests regex, automatic current-match selection,
persistent all-hit outlines/shading and complete forward/reverse/total counts.
These are implemented with a compressed immutable index and cached navigation.
Both-strand intervals count once in Total and in each strand count, with a
separate Both count/explanation. Existing highlights survive selections and
failed/cancelled replacement searches until a successful replacement or panel
close; edits invalidate stale coordinates safely. Regex remains cancellable and
bounded. New model, GUI, sanitizer, raster and scale checks pass. The next roadmap
milestone remains explicit molecule types and native-session foundations.
