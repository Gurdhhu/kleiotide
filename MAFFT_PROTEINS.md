# Protein MAFFT — 0.22

The existing Alignment → MAFFT commands now accept declared protein rows:
whole alignment, selected sequence names only, selected alignment columns,
full selected targets to templates, and selected fragments to templates.
Results open in a separate window; the source is unchanged. MAFFT is a separate
installation. Fragment addition requires MAFFT 7.526 or later.

## Types and scope

Set File → FASTA import type to Proteins + masks before importing protein FASTA, or use
Sequence → Sequence type to correct existing row declarations. A/C/G/T-only
proteins remain proteins: letter frequencies never choose the MAFFT alphabet.
The translation display does not change underlying row types or align DNA as AA.

Whole/column jobs require one biological row type throughout their scope. Masks
are excluded; empty/gap-only biological rows are retained as open-gap rows.
Selected-only jobs use just selected names and can align a homogeneous subset
inside a mixed document. Selected masks remain masks in that result. At least two
biological rows must contain residues in the chosen alignment scope.

For template jobs, select target names, then check templates of the same type.
Selected masks and empty rows are skipped by the chooser. At least one target
and one template must contain residues. Mixed nucleotide/protein participants
are rejected before launching MAFFT. Other rows may be any type and stay outside
the calculation. The same-type template list prevents accidental mixed jobs.

## Protein settings and symbols

Protein jobs explicitly use --amino; nucleotide jobs use --nuc. Protein settings
replace nucleotide strand, Kimura and transition/transversion controls with:

- BLOSUM 30, 45, 62 (default), or 80;
- JTT PAM or transmembrane PAM, with a positive distance from 1 to 1000;
- optional amino-acid composition adjustment (--fmodel).

The existing eight strategies, threads, refinement, gap costs, memory setting
and repeatable refinement option are shared. PartTree is unavailable for template
addition; fragment addition uses Automatic. Protein jobs cannot adjust strands.

All legal protein row symbols are retained, including letter case. B/Z/J/X and
U/O/*/? receive MAFFT's unknown-symbol treatment rather than a specific amino-acid
substitution score. The protein dialog and job report explain this policy.
--anysymbol prevents stop calls and unusual residues being discarded; masks are
still never sent. Characters outside Kleiotide's protein alphabet cannot enter
a declared protein row. Returned row IDs, widths and residue order/content are
validated; malformed output is discarded. Protein U is not treated as T.

Ordinary protein gaps are removed before alignment. This includes the independent
gaps around centered translated snapshots; resulting proteins are normal editable
one-letter rows. Legacy three-letter/spanned snapshots are normalized when loaded.
No new three-letter representation or native-format change is introduced.

## Preservation

Whole jobs retain names, stable IDs, group membership, row types, mask text and
protein provenance. Column jobs also retain exact prefixes/suffixes; mask width
can prevent shortening the selected region so its annotations are never truncated.
New alignment gaps are open (~), following the existing nucleotide behavior.

Template jobs retain the complete original column axis and its gap states,
including columns containing gaps in every template. Target insertion columns
are retained and receive open gaps in all other rows. Every original symbol in
non-target rows, including nucleotide rows with traces and masks, is projected
together into the result. Duplicate names are safe because IDs/residue ordinals
identify anchors. Protein snapshot provenance and immutable trace evidence remain
attached. This is the existing lossless template contract, now molecule-aware.

The Last alignment report identifies molecule flags, scoring parameters, MAFFT
version, participant IDs, insertion counts and preservation checks. Cancellation
and process errors discard the result without modifying the source.

## References

MAFFT's primary documentation: [scoring parameters and molecule flags](https://mafft.cbrc.jp/alignment/software/manual/manual.html),
[acceptable/unknown symbols](https://mafft.cbrc.jp/alignment/software/anysymbol.html),
and [sequence/fragment addition](https://mafft.cbrc.jp/alignment/software/addsequences.html).
