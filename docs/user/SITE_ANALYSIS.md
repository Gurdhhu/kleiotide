# Site analysis, alignment statistics and composition

Highlighting is a read-only view. Several enabled rules combine by union. Its
default population is all nucleotide rows. Analysis scope explicitly chooses
nucleotide or protein rows, all rows or a snapshot of selected names, and all
alignment columns or a snapshot of selected columns. Names and column positions
are captured at Apply. Membership uses stable row IDs, so renaming/reordering is
safe; deleting a row removes its contribution. New rows do not enter a selected
snapshot. Column coordinates remain fixed if the alignment is subsequently edited.
Masks never vote. A selected analysis subset determines column classes and receives the shading.
Rows outside that captured subset remain unshaded. In all-row scope, shading
appears in all rows of the chosen molecule type. Masks and rows of the other
molecule type remain unshaded. The ruler also identifies highlighted sites.

## Site classes versus percentage conservation

Every informative symbol represents a set of possible states. Gaps, absent
trailing cells and unknown calls have no informative evidence and do not constrain
the intersection of possible states.

| Category | Definition |
| --- | --- |
| Conserved highlight | At least one informative call, and all informative calls share a possible state. |
| Variable | The intersection of possible states is empty. |
| Parsimony-informative | At least two definite states each occur at least twice. Ambiguities are not definite observations. |
| Singleton | Variable but not parsimony-informative, preserving the existing summary convention. |
| Non-gap coverage | Non-gap calls / population row count. Unknown calls count; gaps and absent trailing cells do not. |
| Informative coverage | Informative calls / population row count. Partial ambiguities count; unknowns, gaps and absent cells do not. |
| Percentage conservation | Largest state vote / total informative votes. Each call divides its vote equally among its possible states. |

Nucleotides: U = T; partial IUPAC codes vote equally for their 2 or 3 possible
bases. N and ? are unknown. Protein states use the 20 ordinary amino acids plus U,
O and stop (*). B = D/N, Z = E/Q, J = I/L, with equal votes. X and ? are unknown;
N is an ordinary, informative amino acid. These are descriptive conventions, not
probabilistic estimates of the underlying states or a phylogenetic distance model.

Example: nucleotide A plus R shares A, so the site is compatibility-conserved,
but its fractional conservation is 75% (A gets 1.5 votes, G gets 0.5). R alone is
50% conserved by this vote definition. To show strictly conserved columns, use
% Conservation with both bounds 100. All-N/all-gap columns have no votes and are
excluded from conserved/conservation highlights; a coverage range including zero
can intentionally highlight them. Threshold comparisons are inclusive.

## Alignment statistics

Choose Nucleotide or Protein. Results use a snapshot taken when the statistics window opens; other row types are excluded. Width is the longest included row. Statistics follow IQ-TREE-style state intersections: constant sites share a possible state, and gap-only/unknown-only counts are subsets of constant sites. Unique sequences and patterns compare normalized states. Parsimony-informative sites require two definite states each observed at least twice; variable non-informative sites are counted as singleton sites.

Nucleotide U equals T; partial IUPAC calls represent possible bases; N/?/gaps are unknown. Protein statistics use the 20 standard amino acids with B=D/N, Z=E/Q, J=I/L. U/O, X, ?, stops and gaps are unknown for these statistics. This differs deliberately from the broader display-highlighting alphabet described above. It also differs from distance calculations, which discard ambiguous calls rather than using state sets.

## Nucleotide composition

All-row scope spans the longest included nucleotide row. Selected-column scope
uses the requested alignment coordinates, even if other molecule types extend
beyond nucleotide rows. Absent cells are reported as trailing missing data.
Each row is separate, including duplicate names; the final Pooled row sums counts.

Fractional handling starts enabled: each definite or partial-IUPAC call contributes
one scored call, split across A/C/G/T as above. Disabling it counts only definite
A/C/G/T/U calls in the denominator, while retaining a separate ambiguity count.
N/?, open gaps, locked gaps and trailing cells are always excluded from that
denominator and are reported separately. Percentages and GC use pooled counts
for Pooled, never an unweighted mean of row percentages. No scored calls means
N/A frequencies. Display/export round counts and percentages to three decimals;
accumulation uses exact sixths, so rounding does not accumulate between rows.
Save table offers XLSX, TSV and CSV. Calculation settings and values occupy separate columns. The same detailed settings and result cells appear in every format; XLSX separates them into worksheets. Copy retains a text summary of settings and results.

## Responsiveness and persistence

Highlights use debounced cooperative scans on the GUI thread, bounded by symbol
budgets; no rank cache is populated concurrently with painting. Edits/type changes,
row insertion/deletion/reordering and undo/redo invalidate old results immediately.
Pure renaming or group changes retain the calculation. Color/rule changes reuse
an existing completed summary. Composition uses a cancellable worker while the
source is held immutable; its table supplies visible cells on demand.

Optional view JSON stores highlighting rules, ranges, molecule, color and stable-ID
scope snapshots. Current native sessions use format 5; highlighting remains optional view metadata. View settings do not change the
document's sequence-data dirty flag; explicitly save a native session to retain
them. New FASTA windows start without site highlights. Composition tables are
transient; Copy/Export retain their results and method settings.

## Primary references

- [IQ-TREE FAQ: gap, missing and ambiguous characters, and informative patterns](https://iqtree.github.io/doc/Frequently-Asked-Questions).
- [IQ-TREE implementation of alignment site statistics](https://github.com/iqtree/iqtree2/blob/master/alignment/alignment.cpp).
- [NCBI sequence encodings: IUPACna, IUPACaa and NCBIeaa](https://ncbi.github.io/cxx-toolkit/pages/ch_datamod).

Kleiotide's fractional voting and explicit-scope UI are the conventions specified
here; the references do not imply full analysis parity with those applications.
