# Kleiotide — protein coloring and translation design

Superseded details (19 October 2026): user feedback in 0.20.2 removes all
three-letter presentation and bound protein spans. Added translations now use
ordinary protein rows, centered single letters and independently editable gaps.
Each amino acid has a distinct family shade. Copy translated fragment expands
partial codons automatically; no separate expansion command remains. The
current behavior is TRANSLATION.md and docs/history/TESTING_0.20.2.md; descriptions of the old
span model below are historical design decisions, not current editor behavior.

Revised 18 October 2026, against the user-confirmed 0.19.4 baseline.
Status: implemented in 0.20; refined by user feedback in 0.20.1.
The 0.20.1 requests supersede profile-dependent coloring, editable/searchable
code labels and the separate Custom code toolbar action described below.
Colors are now fixed per residue, code names are concise/read-only with full
tooltips, and Custom code is the final dropdown command. Windows acceptance
is pending. See TRANSLATION.md
and docs/history/TESTING_0.20.md. Content edits currently rebuild profiles cooperatively;
incremental affected-codon recomputation remains a later scale optimization.
This replaces the earlier whole-alignment-translation design in roadmap stage 6.

## 1. Two translation workflows, one set of results

The main workflow is a live translation layer in the current nucleotide window.
It keeps the alignment's columns, nucleotide data and trace evidence intact.
The second workflow creates editable protein snapshots from selected sequence
names. Both use the same codon mapping, genetic-code engine, ambiguity rules,
stop classification and preview results.

Opening a compact translated protein alignment in a new window remains available
as an explicit command. It is no longer the action of the main translation toggle.
Protein MAFFT remains its later roadmap stage; this design prepares its inputs.

## 2. Toolbar and display

The main toggle is **Translation**, with a DNA-to-AA icon, a checked state and a
tooltip. It is also available through View and requires at least one nucleotide
row; an ineligible document gets an explanatory tooltip. With it off, its extra controls are
hidden. With it on, a translation strip appears immediately below the existing
main controls, in the same upper-panel area. This avoids squeezing out Edit/View,
Rewrite/Insert, right-click and trace/display buttons on narrower screens.

| State | What appears in a complete codon's three nucleotide columns |
| --- | --- |
| Translation off | Existing nucleotide display |
| Translation on; Show amino-acid code off | Original nucleotide letters on the encoded amino acid's Clustal-style background |
| Translation on; Show amino-acid code on; 1-letter | One centered amino-acid label occupying the same three-column cell |
| Translation on; Show amino-acid code on; 3-letter | Three-letter amino-acid label occupying that same three-column cell |

One-letter/three-letter switching never changes column widths, alignment length,
scroll position or selection coordinates. Noncoding columns keep their nucleotide
display. Protein rows keep protein coloring; masks keep their neutral appearance.
The normal nucleotide display preference is restored when Translation is off.

The translation strip contains the requested controls:

| Control | Behavior |
| --- | --- |
| Show amino-acid code | Switch between nucleotide letters and AA labels |
| 1-letter / 3-letter | Exclusive presentation switch; disabled when AA labels are hidden |
| Codon positions | Show/hide a separate ruler track with 1, 2, 3 and noncoding marks |
| Coding 1-2-3 | Repeat these phases from the leftmost selected column |
| Coding 2-3-1 | Repeat these phases from the leftmost selected column |
| Coding 3-1-2 | Repeat these phases from the leftmost selected column |
| Noncoding | Mark the selected columns as untranslated |
| Genetic code | Searchable dropdown showing NCBI table number and name |
| Reading frame | 1, 2 or 3; sets the default phase of unannotated columns |
| Stops | Live whole-translation stop count; tooltip explains scope and uncertain calls |

Buttons have distinct icons and brief hover explanations. Menus expose the same
actions. At small widths, the translation strip uses toolbar overflow; essential
editing controls retain priority. The separate ruler track must work at font 6
and fractional DPI without cropping numbers.

## 3. Coding columns and reading frames

Coding annotations apply to alignment columns across all nucleotide rows. They
are independent of selected sequence names. A residue block supplies its horizontal
column interval; the ruler can supply a column selection directly.

There is one precedence rule: explicit annotations override the default frame.
Initially, every column uses the default. Frame 1 starts complete codons at column
1; frames 2 and 3 start at columns 2 and 3. Leading partial codons are marked as
partial. A manual coding command assigns its specified phase to the leftmost
selected column and cycles through that interval. Noncoding is an explicit mark.

Changing frame updates unannotated columns and preserves manual annotations.
A **Custom coding** indicator appears when overrides exist; the frame tooltip
then says it controls unannotated columns. Additional menu actions **Use default
frame in selected columns** and **Reset coding annotations** remove overrides.
This avoids a frame change silently discarding annotated exons or introns.

Codon construction follows ordered 1-2-3 coding positions. Noncoding positions
are skipped. A codon can therefore span an explicitly annotated intron, provided
the flanking phases agree. Unexpected phase jumps are visibly flagged rather
than guessed. For example, `1, noncoding, 2, 3` is one codon; `1, noncoding, 3`
has an unresolved phase break.

For a split codon, only its three coding cells receive AA coloring. Three-letter
labels place one character in each coding cell; a one-letter label is anchored
at the first coding cell. Noncoding text is never painted over. A tooltip lists
the participating nucleotide columns and translation.

Coding annotations and the document's default frame/genetic code are undoable
semantic metadata, saved in sessions. Display toggles are view preferences and
do not enter residue-edit history. Dialog-only parameter changes do not modify
the document's defaults. Row-local insert,
delete and slide operations leave column annotations fixed: moving a sequence
relative to the common coding track is intentional. Whole-alignment column
operations move/remove manual annotations with their columns; automatic phases
are recalculated from the resulting alignment coordinates. New columns default
to automatic. Undo restores both data and annotations together.

## 4. Editing and copying the live layer

Nucleotide lettering with codon colors retains normal nucleotide editing,
selection, sliding and trace interaction. The layer updates after edits/Undo/Redo.

AA lettering is a preview in View mode. Its three-letter labels do not define
three editable residues, and an AA key cannot specify a unique replacement codon.
Entering AA lettering temporarily remembers the user's Edit/View and Rewrite/
Insert choices. Choosing Edit returns to nucleotide lettering while keeping
translation colors. Leaving AA lettering restores the remembered mode unless
the user explicitly changed it. Coding annotation commands remain available as
explicit metadata operations.

Alignment selection coordinates and ordinary fragment shortcuts keep their
existing meanings. Ctrl+C copies underlying nucleotide text from an eligible
single-row fragment, even in the live AA preview; its menu label/tooltip makes
that target explicit. **Copy translated fragment** is a separate command for
complete codons selected in one row. Partial-codon selections do not silently
expand or discard bases. Whole-sequence copy/export likewise keeps original
data unless an explicitly translated command is chosen.

Identity-dot display is suspended on translated nucleotide rows and restored on
exit, so a dot cannot hide a stop or imply nucleotide identity means AA identity.
Existing site highlights remain nucleotide analyses; their outlines/markers are
drawn without replacing the codon fill. Overview, statistics and chromatogram
evidence continue to describe the underlying document. Tooltips identify the
coordinate and molecule type where the preview could otherwise be confusing.

## 5. Translation rules and stop counts

Default operation is translation of aligned coding columns, not independent
removal of every row's gaps. Thus homologous columns retain a common codon map.

| Input or condition | Result |
| --- | --- |
| Complete unambiguous codon | AA from the chosen genetic-code table |
| IUPAC ambiguity with one possible AA outcome | That AA, e.g. GCN → Ala |
| Multiple possible AA outcomes | X / Xaa; no arbitrary choice |
| Three gaps, including mixed open/locked gaps | One protein-gap token |
| Residue/gap mixture within a complete codon | X / Xaa, with a partial-gap flag |
| Complete codon with unknown/missing calls | Resolve only if all possible outcomes agree; otherwise X |
| Partial codon at an alignment boundary | Keep dimmed nucleotide letters in the overlay; omit from protein output and report it |
| Inconsistent internal coding phases | Flag in the overlay; block snapshot creation until corrected or annotation use is disabled |
| Definite stop | * / Ter; keep translating subsequent codons |

Treat U as T during lookup and preserve underlying case/text. Do not automatically
recode an alternative initiator as methionine: an alignment may contain an internal
fragment rather than a complete CDS. Translate rows in their displayed orientation;
existing Reverse complement supplies the opposite strand. This stage's frame
dropdown is 1/2/3, without an additional hidden strand transformation.

The offline catalog contains all 27 tables currently listed by NCBI, using their
stable IDs: 1–6, 9–16 and 21–33. Bundle a dated catalog and check it against NCBI
before release. A small optional **Custom genetic code…** table editor/import
provides explicitly named user overrides for experimentally supported mappings
outside that catalog; save the full custom table and provenance in the session.
Do not infer organism-specific recoding, RNA editing, Sec or Pyl from sequence
alone.

NCBI tables 27, 28 and 31 contain codons whose stop/sense role depends on context.
Use their sense translation in the AA display and mark such sites as context
dependent. Never silently turn them into unconditional stops or infer their
termination context from the end of an alignment fragment.

**Stops: N** counts definite stop instances across all complete translated codons
in all nucleotide rows of the current alignment. It includes terminal stops,
excludes masks and existing protein rows, and does not change merely because of
scrolling, name selection or display-code switching. The tooltip separately reports
ambiguous codons that could encode a stop and context-dependent sites; codes with
context-dependent termination also show a small explicit indicator beside the
count. Stop count is not an ORF prediction or a count of distinct stop columns.

Preview and count always share the same settings/result revision. While a new
count is being computed, show **Counting…**, never a stale number or a false zero.

## 6. Add translated sequences

**Add translated sequences…** appears in the sequence-name context menu and
Sequence menu. Its scope is selected sequence names, independent of residue
selection. Only nucleotide rows are eligible; the dialog reports skipped
proteins/masks. With no eligible selection, the command is disabled.

The dialog contains genetic code, frame, 1-/3-letter AA presentation, and live
counts of translated rows, stops, unknown/partial-gap codons and omitted boundary
fragments. It opens with the current translation settings. When manual coding
annotations exist, **Use coding annotations** is shown and initially checked;
frame controls unannotated columns. Unchecking it explicitly requests uniform
aligned-column translation at the chosen frame. The dialog previews actual names
and a short translation; Add is enabled only for the currently completed preview.

Each result is inserted directly below its source, retaining the source group.
The name is the complete source name plus `_TRANSLATED`; collisions get `_2`, `_3`,
etc., without renaming existing rows. All additions form one Undo step and become
the selected sequence names. Original residues and trace links stay intact.
The new protein rows have no chromatogram link.

Results are independent snapshots, not automatically regenerated copies. Store
source ID/name, source revision, code/table definition, frame, annotation settings
and original codon coordinates as provenance. Later source edits do not overwrite
manual protein edits; provenance can indicate that source and snapshot differ.

In this alignment, a complete codon owns three mapped display cells. One-letter
presentation places the AA in the first coding position with two gap placeholders;
three-letter presentation distributes its AA name across those same positions.
Noncoding and omitted boundary positions have gap placeholders in the derived row.
For `ATG AAA TAA`, the logical protein is `MK*`; aligned one-letter display is
`M--K--*--`, and three-letter display is `MetLysTer`.

## 7. Protein identity, layout and coloring

Store canonical AA tokens and a separate display-span/codon map. `Met` must never
become the three residues M, E and T. Structural placeholders are distinct from
biological gaps and from the user's ordinary open/locked gaps. The same canonical
protein underlies either display encoding.

Ordinary imported protein rows retain one AA per alignment column. Codon-positioned
derived rows retain their mapped spans. Direct protein editing accepts standard
one-letter input; three-letter clipboard text requires an explicitly labeled
parser rather than guessing whether `MET` means one or three residues. Operations
on spanned rows act on complete AA tokens. A single-row click on a continuation
cell targets its AA. A multi-row edit cutting through a token is disabled with an
explanation and an explicit **Expand selection to full amino acids** action.
Expansion is shown before any edit, including any additional nucleotide columns.
Padding cannot be consumed as an open gap.

An explicit compact-protein command produces one column per codon in a new window.
That window uses amino-acid coordinates; a three-letter label still occupies one
biological AA cell, with enough display width for the label.
The current alignment's normal FASTA export and whole-sequence text clipboard
retain alignment coordinates, with one-letter protein tokens and gap placeholders.
An explicit **Export compact proteins** command and future MAFFT inputs use
canonical one-letter AA data, stripping only structural display padding and
retaining biological gap tokens. Three-letter output is explicitly labeled
presentation output, not standard one-letter protein FASTA. Native clipboard
payloads retain the full typed layout and group metadata. Fragment copying from
protein rows likewise copies canonical one-letter data in the selected alignment
coordinates; **Copy as three-letter labels** is explicit.

The initial protein scheme is the requested Clustal style, using documented
ClustalX profile thresholds/precedence, including the conserved-cysteine override.
Actual protein coloring counts protein tokens once; continuation characters and
padding never vote. Compare ordinary protein rows on their one-column grid and
codon-positioned rows on matching codon-span grids, rather than mixing unrelated
coordinate units. Translation-layer coloring uses virtual AAs from nucleotide
rows only, so added copies cannot double-weight their source. Within each profile,
use all eligible rows at that logical position as the denominator; biological
gaps and unknowns contribute no AA-group votes. Imported B/Z/J/X remain neutral
unless the chosen documented scheme defines their treatment. Record strict
threshold comparisons and precedence in coloring fixtures.

Triplet backgrounds use the encoded AA color and a readable black/white symbol.
No-information/unresolved calls are neutral; stops have a distinct visible symbol
and cue. Ordinary protein display supports the current letter, background and
monochrome styles; selection cues retain priority. Masks stay neutral.

## 8. Persistence, speed and acceptance

Plan native session format 4 for coding annotations, protein span/representation
metadata, provenance and optional custom codes. Keep readers for formats 1–3.
Old versions must reject new-format sessions clearly. Optional visual choices
remain view state; semantic metadata must not live only in view JSON. FASTA cannot
preserve these annotations or a mixed row's display layout by itself.

Preserve the 0.19.4 performance approach. Paint visible cached codon/AA tiles;
cache profile summaries and coordinate maps. Precompute each genetic code's small
IUPAC-codon lookup so ambiguity expansion is not repeated during painting.
Selection drag, scrolling, code-label
switching and trace following must not trigger whole-alignment translation or stop
rescans. Revision-key caches distinguish sequence content, coding metadata and code
settings. Recompute affected rows/codons after local edits; full frame/code changes
use cancellable background work. Discard stale results. Cache counts separately
from paint and keep the chromatogram caches independent.

Implementation order within this stage:

1. Canonical protein/span model, coding-map rules, table catalog and translation
   engine; known-answer tests and session migration.
2. Clustal-style protein coloring and cached virtual-AA profiles.
3. Live translation controls/ruler, editing-mode interaction and asynchronous count.
4. Add-translated dialog, compact new-window output, clipboard/export and atomic Undo.
5. End-to-end correctness, performance and Windows acceptance.

Acceptance includes all table IDs and every concrete codon, ambiguity/stop cases,
default versus custom phases, codons split by noncoding columns, terminal fragments,
mixed gaps, ragged rows, masks/proteins, reverse complement, unchanged trace evidence,
snapshots/groups/name collisions, partial-span editing, complete Undo/Redo, and
old/new session round trips. At 3440×1440 and 100%/150% DPI, test selection and
scrolling with Translation, AA labels, site highlights and chromatograms all open.
Measure warm drag/scroll work separately from full code/frame recomputation;
compare cached pixels against fresh paints.

## References and provenance

The behavioral choices above are Kleiotide design decisions. AliView is a partial
interaction reference, not a promise of identical editing or file semantics.

- [AliView official help](https://www.ormbunkar.se/aliview/): translation view,
  translated output and column coding annotations.
- [NCBI genetic-code tables](https://www.ncbi.nlm.nih.gov/Taxonomy/Utils/wprintgc.cgi):
  table catalog and context-dependent assignments; checked 18 October 2026.
- [Jalview ClustalX scheme](https://www.jalview.org/help/html/colourSchemes/clustal.html):
  conservation-dependent color criteria. The source page's example text and table
  disagree on one percentage; use the normative threshold table with explicit
  fixtures and record the chosen scheme version.
