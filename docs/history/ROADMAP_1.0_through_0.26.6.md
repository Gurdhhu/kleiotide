# Kleiotide 1.0 — expanded roadmap and implementation plan

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

0.26 adds identity-based greedy groups and representative selection as refinements
to the existing group workflow. See IDENTITY_GROUPS.md and TESTING_0.26.md.

Version 0.25.2 removes conflict navigation buttons, recognizes composite terminal
N/?/gap regions and prioritizes minimum-edit eligible overlaps. See
TESTING_0.25.2.md; minimum matching support remains a required constraint.

Version 0.25.1 adds conflict/case preview refinements, bounded edit-tolerant
partial-contig merging and narrowly validated stale-offset AB1 import recovery.
See TESTING_0.25.1.md, MERGING.md and AB1_IMPORT_RECOVERY.md.

Version 0.25 adds aligned/IUPAC and exact-overlap sequence merging, including
optional original-Q conflict resolution. See MERGING.md and TESTING_0.25.md.
Native Windows acceptance remains pending; the release-stage order is unchanged.

Refinement in 0.24.3: draggable batch-preview divider, output-filename priority
in narrow windows and restoration of preferred column widths on widening.

Refinement in 0.24.2: editable batch-export filename suffix with live preview,
optional empty value and existing overwrite protection. The user confirmed
BioEdit/Chromas compatibility and all 0.24.1 acceptance tests. Release-stage
order is unchanged.

Refinement in 0.24.1 (20 October 2026): legacy BioEdit compatibility corrections
and all/selected batch AB1 export, with preview, collision avoidance, cancellable
per-file writes and completion reports. Native Windows acceptance follows
TESTING_0.24.1.md; release-stage order is unchanged.

Additional requested feature delivered in 0.24: edited AB1 export from linked
sequences/portable sessions, current calls and strand orientation, conservative Q,
exact analyzed signals, independent reader checks and protected atomic copies.
See AB1_EXPORT.md and TESTING_0.24.md. Remaining release-stage order is unchanged.

Stage 9 delivered in 0.23: duplicate collapse with chosen representatives, title
concatenation or count suffixes, preview and single-step Undo. See COLLAPSE.md and
TESTING_0.23.md. Independent residue compression is removed from scope by user
request. Remaining stages are reliability/release preparation and installation.

Stage 8 delivered in 0.22: protein MAFFT across all five existing scopes, explicit
molecule validation, protein scoring controls and lossless mixed-document template
projection. See MAFFT_PROTEINS.md and TESTING_0.22.md. Similarity sorting now
explains N/missing-state exclusions. Stages 10–11 remain after the 0.23 refinement.

0.21.1 refinement: mean-distance SD and explicitly labelled naive SE, valid-pair
sample conventions, formula/dependence explanations and versioned statistical TSV.
No change to the remaining milestone order; see TESTING_0.21.1.md.

Stage 7 delivered in 0.21, 19 October 2026: five bounded distance methods across
nucleotides/proteins, pairwise matrices, overall/within-/between-group means,
explicit missing-data/overlap rules, TSV, cancellation and similarity-chain preview.
Contract: DISTANCES.md. Windows acceptance: TESTING_0.21.md. User confirmed 0.20.3.

Latest refinement, 19 October 2026: 0.20.3 completes the requested annotation-mask
workflows, trailing partial-codon translation, group-name renaming, sequence-type
sort and menu refinements. Acceptance: TESTING_0.20.3.md. Remaining stage order
and scope are unchanged.

Latest refinement, 19 October 2026: 0.20.2 uses single-letter translation only,
ordinary centered protein additions with independently editable gaps, distinct AA
family shades and automatic translated-copy expansion. This supersedes the older
three-letter/span-bound display contract below. Acceptance: TESTING_0.20.2.md.

Revised 18 October 2026: stage 6 is implemented in 0.20, pending Windows acceptance. User-feedback fixes in 0.20.1 add fixed residue colors and refined translation controls. See TRANSLATION.md and TESTING_0.20.1.md.

All eight additions requested on 15 October 2026 are included in Windows 1.0. Protein support
is no longer postponed. This supersedes the earlier nucleotide-only roadmap.
Stages 1–9 are delivered; stages 10–11 remain planned. Version 0.17 added persistent
groups and typed cut/paste. Protein presentation, translation and MAFFT are now
implemented; remaining release gates are below.

## Baseline and release target

Already delivered: FASTA editing, independent selections, undo/redo, rigid-block
pushing and gap tools, sorting and batch renaming, masks, overview/statistics,
nucleotide MAFFT including lossless target/template integration, AB1 viewing and
Current-call editing, sample/alignment trace spacing, portable .kleio sessions,
identity dots and five residue palettes.

The expanded target is an installable Windows application for nucleotide and
basic protein editing, trace cleanup, groups, site highlighting, descriptive and
distance analyses, and MAFFT. End users will not need Qt Creator or a compiler.

## Implementation order

Milestones are dependency/review boundaries, not calendar estimates. Each may
take more than one development release.

| Order | Milestone | Deliverables and dependency |
| --- | --- | --- |
| 1 | FASTA output and search — delivered in 0.15 | Unwrapped saved/copied FASTA, wrapped input retained; motif next/previous-hit navigator with overlaps, strands, regex, complete counts, automatic residue selection and all-hit shading. Optional N-only filtering and stronger background-mode highlighting in 0.15.2. Windows acceptance follows TESTING_0.15.2.md. |
| 2 | Types and native-session foundation — delivered in 0.16 | Explicit nucleotide/protein/mask types, explicit FASTA import choices, atomic undoable retyping, type badges, neutral protein editing, canonical representation metadata and format-1-to-2 session migration. Existing masks/traces are protected. Acceptance: TESTING_0.16.md. |
| 3 | Sequence groups — delivered in 0.17 | Manual/TSV assignments with preview and conflict handling, name swatches, context actions, group selection and stable manager-order sorting. Stable-ID membership, undo, MAFFT preservation and format-3 sessions. Also sequence/residue cut-paste and doubled Ctrl+wheel. 0.17.1 adds type/trace selectors, identical grouping, selected-only MAFFT and target strand adjustment; residue clipboard editing is single-row only. 0.17.2 adds View/Edit controls, gap-insensitive grouping, fragment-only shortcuts/context actions and rectangular gap unlock. 0.17.3 makes both Edit/View states visible, disables the View right-click dropdown and adds rectangular gap locking. 0.17.4 adjusts toolbar spacing, sequence menu grouping and identity help. Acceptance: TESTING_0.17.4.md. |
| 4 | Trace-quality editing — delivered in 0.18 | Explicit selected/all-trace scope, mean-Q non-gap windows, column-preserving trimming, low-Q-to-N, original-call provenance, protected missing/substituted calls, asynchronous preview/cancellation, atomic Apply/Undo and native persistence. Acceptance: TESTING_0.18.md. |
| 5 | Highlight and descriptive statistics — delivered in 0.19 | Shared site summaries; explicit row/type/column scope, union highlighting, coverage/conservation ranges, color and native view persistence. Statistics menu and nucleotide composition with per-row/pooled results, fractional-IUPAC choice and Copy/TSV. Acceptance: TESTING_0.19.md; definitions: SITE_ANALYSIS.md. 0.19.1 limits selected-scope shading to those rows and merges sliding into Rewrite/Insert; 0.19.2 adds control-first toolbar layout, mouse-drag selection undo, whole-sequence shortcuts and warning cleanup; acceptance: TESTING_0.19.2.md. |
| 6 | Basic proteins and translation — delivered in 0.20 | Clustal-style protein coloring; same-window translation/codon coloring with fixed-width single-letter AA labels, coding-position ruler/annotations, genetic-code/frame controls and live stop counts; selected-name translation preview and ordinary protein snapshots with centered letters and independent gaps. Compact new-window output remains explicit. Contract: PROTEIN_TRANSLATION_PLAN.md. |
| 7 | Distances and similarity sorting — delivered in 0.21 | Pairwise matrices, overall/within-/between-group means and observed-difference ordering. Reuse groups, molecule types and explicit missing-data rules. Cancellable workers, triangular matrix/lazy table, streaming means/export, TSV, minimum-overlap and pairwise/complete deletion. Contract: DISTANCES.md; acceptance: TESTING_0.21.md. |
| 8 | Protein MAFFT — delivered in 0.22 | Whole/column and target/template/fragment workflows, protein parameters and strict rejection of mixed nucleotide/protein input. |
| 9 | Collapse identical sequences — delivered in 0.23 | Gap/case-insensitive complete-string matching by type, chosen representative and retained group/evidence, concatenated titles or _xN count, All/Selected scope, preview, cancellation and atomic Undo. Residue compression removed from scope. |
| 10 | Reliability and release preparation | Cancellation, asynchronous large operations, history memory, recovery design, regression/performance tests, feature freeze and documentation. |
| 11 | Installer, release candidate, 1.0 | Reproducible Windows bundle, runtime deployment, shortcuts/optional associations, upgrade/uninstall, clean-machine tests and blocker fixes. |

## Feature contracts and proposed defaults

The requested functions are committed scope. Details marked proposed are defaults
to review at the relevant milestone, not prerequisites blocking work now.

### 1. FASTA without wrapping

All user-facing FASTA saving/export and sequence copying writes each complete
sequence on one line after its header. Preserve case/gaps and empty sequences.
Continue accepting arbitrary wrapping and supported whitespace/line endings.
Share the writer between clipboard and files; audit internal MAFFT serialization
separately. Deliberately update the existing zero-width rejection rather than
accidentally bypassing it. Test long, empty, ragged, mask and protein round trips.

### 2. Trace-quality editing — delivered in 0.18

Both Edit commands explicitly choose selected names or all trace-linked nucleotide
rows. No selection does not mean all. Preview reports unlinked, protein and mask
rows; settings changes invalidate it. Calculation is asynchronous/cancellable;
Apply is atomic and undoable. Imported signals/Q are immutable.

Trim low quality ends uses arithmetic mean Phred Q over complete windows of
current non-gap symbols. Mean >= threshold passes. Keep the interval from the
first passing window through the last, retaining internal low-quality islands.
Trimmed residues become open gaps, preserving every column and existing gap state.
Rows shorter than a full window or without a passing window remain unchanged.

Low-quality calls to N uses Q strictly below threshold, retains case, positions
and gaps, and does not count existing N/n as new changes. Quality follows original
call mappings on both strands. Missing Q differs from measured Q0. Insertions
always remain protected; substitutions are protected by default. Explicit inclusion
may use their original-call Q (not a newly measured current quality). Substitutions
include prior cleanup replacements. Trim intervals expand to retain protected
calls, and scoring windows require usable Q for every symbol.

Defaults are Q20 / ten-symbol windows; inclusion resets off on each dialog. The
synthetic .kleio example and TESTING_0.18.md give known-result settings. Native
format remains 3. A compacting/deleting trim alternative is outside this release.

### 3. Groups

Provide a manager to create/rename/delete groups, assign colors and manage rows.
Proposed first version: one group per row plus Ungrouped. Overlapping memberships
are not silently assumed in means or sorting.

Import `Sequence_name<Tab>Group_name` with an explicit header-row option and
preview. Match complete titles exactly; report unmatched keys, duplicate-title
matches and conflicting assignments before Apply. Membership attaches to stable
row IDs, so rename/reorder cannot break it. Save groups in native sessions and
preserve them through undo/redo and MAFFT result creation.

Color names or show a group swatch that remains visible during name selection.
Add row context actions Add to group… (existing/new) and Remove from group;
Sequence → Select → Groups… supports several groups and Ungrouped. Add By group
sorting, stable within each group; proposed group-manager order, Ungrouped last.
Translated rows inherit the source group by default. Masks may carry membership
but do not contribute to biological statistics.

### 4. Highlight menu and column calculations — delivered in 0.19

Add Conserved sites, Variable sites, Parsimony-informative sites, Singleton sites,
% Coverage…, % Conservation…, Highlight color… and Clear highlighting.
Proposed multiple enabled rules combine by union; threshold dialogs state the
comparison and denominator. Row/column analysis scope is explicit rather than
silently tied to name selection.

Use pale violet by default, distinct from the blue/yellow selection colors.
Selections take visual precedence. Highlights need an edge/ruler cue and must
remain visible with colored backgrounds, monochrome and identity dots.

Extract shared column summaries for highlighting, applicable statistics and
conservation coloring. Calculate protein and nucleotide populations separately;
masks never vote. Preserve documented existing statistics conventions. In
particular, IQ-TREE-style compatibility-constant sites and strict 100% column
conservation are different concepts; freeze examples and name the distinctions
before implementation rather than silently equating them.

Proposed non-gap coverage includes N/X, matching overview height; informative
coverage excludes unknowns and is labeled separately. Nucleotide conservation
retains fractional IUPAC votes with N/?/gaps excluded. Define analogous protein
unknown/gap behavior explicitly. Invalidate cached results after relevant edits.

### 5. Statistics and distances — delivered in 0.21

Move the current summary into Statistics → Alignment statistics, preserving Copy
without duplicating menu entries. Add Nucleotide composition, Pairwise distance
matrix, Overall mean distance, Within-group mean distance and Between-group mean
distance.

Composition reports per-row and pooled counts/frequencies, denominators, gaps
and ambiguities, with an explicit fractional-IUPAC option. Exclude proteins and
masks from nucleotide counts.

Proposed bounded 1.0 distance set: nucleotide observed differences, p-distance,
Jukes–Cantor and Kimura two-parameter; protein observed differences, p-distance
and Poisson correction. “Different methods” does not imply every MEGA model,
codon-selection analysis or bootstrap standard error.

Expose row/column scope, molecule type, pairwise versus complete deletion and a
minimum comparable-site threshold. Proposed distance default excludes ambiguous
states rather than implicitly using fractional conservation votes. Report the
comparable-site count. Zero overlap and model saturation return N/A with reasons,
never zero. Mixed nucleotide/protein matrices are not meaningful inputs.

Proposed averages use valid pair distances: unique unordered pairs overall and
within each group; cross-group pairs between groups. Report valid/excluded pair
counts; fewer than two eligible rows gives no within-group estimate. Ungrouped
inclusion is explicit. Copy/export labeled TSV tables with analysis settings.

Use cancellable jobs and bounded/lazy matrix display. Means need not store the
full quadratic matrix; estimate resources before allocating a large matrix.

### 6. Similarity sorting — delivered in 0.21

Use simple observed difference counts, not a corrected model or a replacement
with percent identity. Keep the current top biological sequence first; masks do
not become a biological reference.

The settled interpretation is a nearest-neighbor chain: append the nearest
remaining row to the last row placed, starting at the first eligible row.
DISTANCES.md defines ties, disconnected components and rows with no valid overlap.
Reuse explicit gap/unknown/minimum-overlap rules. Proposed ties: more comparable
sites, then original order; no-comparison rows last. Mixed molecules require an
eligible subset. Apply one deterministic, cancellable, undoable row permutation
that preserves groups and trace links.

### 7. Protein editing, coloring and translation

Protein is a biological row type, not a mask. Open/copy/paste/edit it with the
existing selections, gap tools and history. Provide explicit import/type choices:
alphabets overlap, so all-ACGT protein strings cannot be inferred reliably.
Preserve old mask workflows rather than silently reclassifying them. Document
accepted amino-acid ambiguities, stops, unknowns and uncommon symbols separately
from MAFFT's accepted input. Nucleotide-only commands must clearly skip/reject
proteins; mixed nucleotide/protein display and selections remain supported.

Use the supplied screenshot as the ClustalW-style reference. It corresponds to
conservation-dependent ClustalX rules, not a fixed per-letter lookup. Implement
thresholds/precedence including the conserved-cysteine override, and test variable
and conserved columns in each display/highlight mode. Initial protein palette: one.

The authoritative implementation contract is PROTEIN_TRANSLATION_PLAN.md.
The main Translation toggle now supplies a live layer in the same window;
opening a compact translated alignment in a new window is an explicit secondary
command. AA-code display uses the same three nucleotide columns for 1-/3-letter
labels and temporarily uses View mode; editing returns to nucleotide lettering.

Provide coding 1-2-3/2-3-1/3-1-2/noncoding commands, a separate codon ruler track,
all current NCBI tables, frame 1/2/3 and asynchronous whole-translation stop counts.
Manual column annotations override default-frame phases and can span annotated
noncoding regions when the coding phases agree. Resolve ambiguity conservatively;
distinguish definite from possible/context-dependent stops and incomplete codons.

Selected sequence names offer Add translated sequences with code/frame/encoding
and a live preview/count. Add independent protein snapshots after their sources,
retain groups, append _TRANSLATED with collision suffixes, and use one Undo step.
Canonical AA tokens, structural padding, display spans and creation provenance
must remain distinct; Met never becomes M/E/T. Partial-span edits require explicit
selection expansion. Ordinary alignment FASTA retains column coordinates; compact
protein output and future MAFFT remove structural padding explicitly. Preserve
the 0.19.4 trace caches and verify 3440×1440 interaction with both layers open.

### 8. Protein MAFFT — delivered in 0.22

Support the existing workflow coverage: whole/column alignment and supported
target/template/fragment additions. Force declared input type, expose protein
scoring choices and disable nucleotide-only direction/scoring controls.

Reject mixed nucleotide/protein input before launching MAFFT. Masks remain
nonparticipants. Centered protein snapshots are ordinary one-letter rows; their
independent gaps are removed for MAFFT. Legacy spans are normalized on load.
Nonparticipating mixed-document rows share the exact original-column projection;
never silently change their data or column relationships.

Retain titles, IDs, case, non-target relationships, group metadata and trace
provenance under the existing preservation contracts. Validate output and open
results separately. Test actual nucleotide/protein MAFFT jobs, cancellation,
unsupported symbols, mixed-input rejection and lossless template integration.

## Persistence, reliability and release gates

The current 0.20 application writes binary .kleio format 4 with types, canonical representation,
group catalog and memberships; readers and migration fixtures for formats 1 and 2
are retained. Noncanonical translation/display mappings require further explicit
semantic schema extensions with retained old readers; do not hide them solely in
optional view JSON. Older applications must reject unsupported new files clearly
rather than lose groups or misread proteins. Current encoding 0 stores one symbol
per alignment column, with one-letter protein data; no three-letter/codon-spaced
representation is silently inferred.

Stage 6 plans format 4 for coding metadata, protein span layouts and translation
provenance/custom code definitions, retaining readers for formats 1–3. It is not
implemented yet. View toggles remain separate from semantic document metadata.

Data-changing batches use stable IDs, atomic validation and undo. Read-only
analyses and highlights do not alter sequence data. Revision-check background
results before applying. Preserve imported AB1 channels/Q and trace mappings.
Exercise groups/derived rows across rename, deletion, sorting and undo/redo.

Before feature freeze: known-answer fixtures for trimming, genetic codes, site
classes, distance models and means; mixed-type/mask checks; old/new session round
trips; failure/cancellation/history tests; and a short guide stating scientific
conventions. Review autosave/recovery as a release safeguard; measure large-data
responsiveness and memory on Windows, including matrices and computed coloring.

Installer work includes Qt/runtime deployment, shortcuts, optional .kleio/AB1/
FASTA associations, multi-file Open With, upgrades and uninstall. Verify on clean
Windows without developer tools, with Unicode/space-containing paths, high-DPI
screens and portable sessions. MAFFT remains externally configured unless a
separately verified bundle is selected. Signing/public distribution are later
release decisions. Release the exact tested candidate after blocking data-loss
and core-workflow findings are resolved.

## Inputs needed later and scope boundaries

Feature stages through protein translation, distances, protein MAFFT and duplicate
collapse are implemented. Independent residue compression has been dropped.
Representative large nucleotide/protein/trace datasets and a clean Windows machine
will help the remaining reliability and installer acceptance. No new feature
specification is required for the next release-preparation stage.

macOS/Linux installers, raw signal-amplitude editing, independent manual trace
warping, additional formats and automatic updating remain later extensions.
Basic proteins are expressly inside 1.0.

## Primary implementation references

- [Jalview: ClustalX color rules](https://www.jalview.org/help/html/colourSchemes/clustal.html).
- [MEGA XII distance options](https://www.megasoftware.net/web_help_12/Analysis_Preferences_Distance_Computation.htm),
  [difference counts](https://www.megasoftware.net/web_help_12/No_of_differences_Nucleotide.htm),
  [no common sites](https://megasoftware.net/web_help_12/No_Common_Sites.htm).
- [MAFFT manual: explicit molecule types and protein parameters](https://mafft.cbrc.jp/alignment/software/manual/manual.html),
  [addition workflows](https://mafft.cbrc.jp/alignment/software/addsequences.html).
- [NCBI genetic-code tables](https://www.ncbi.nlm.nih.gov/Taxonomy/Utils/wprintgc.cgi).

References guide implementation; full MEGA/Clustal application parity is not implied.
