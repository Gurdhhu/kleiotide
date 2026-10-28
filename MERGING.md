# Sequence merging — 0.27.1

Select two or more nucleotide sequence names, then use a merge command in
Sequence. Whole rows are processed independently of any residue-block selection.
Original rows, groups and trace evidence remain intact. Apply appends one
ungrouped consensus, selects it, and is one Undo step. Its editable default name
is the first contributing row's name in alignment order plus _merged. The
consensus has no fabricated single-read trace or measured consensus Q values.
Native session format remains 4. Save a session to retain the original traces.

## Preview and case

The result banner distinguishes complete results (green), partial results
(amber) and failure (red). Explanatory rules are collapsed by default. The full
sequence preview highlights conflict positions in red, and original-Q-resolved
positions in green. This includes indel conflicts even when the retained consensus symbol is unambiguous.

An informative position contributed by one read retains that read's case,
including in extensions and contained-read flanks. Positions with two or more
informative calls use uppercase consensus. N/? do not force a flank uppercase.
A lone lowercase n is retained as n; other unknown-only positions become N.
U/u is normalized to T/t. Source case is never changed by preview or Apply.

The scrollable read alignment shows consensus and original reads in final
contig coordinates. Amber columns mark any differing symbols, ignoring case
and normalizing U/T; this includes compatible IUPAC differences. Blank exterior
flanks are outside a read and do not create false conflicts. Internal gaps are
shown. The preview paints visible cells only and retains the original call order.
The body scrolls on smaller/high-DPI screens while footer actions remain reachable.

## Merge selected sequences

Uses existing alignment columns. N, ?, absent trailing columns and entire
leading/trailing regions made only of N/?/gaps supply no evidence. A gap is
internal only when informative nucleotide calls occur on both sides; terminal
missing runs can mix open gaps, locked gaps, N/n and ?. Source placement spans
still describe the original non-gap extent, independently of this evidence rule. An informative call fills missing observations.
An all-gap output column is an open gap. The longest selected row supplies the
output extent; unselected longer rows do not pad it.

Take the union of informative IUPAC states at each column: A/G becomes R,
A/C/G becomes V. Compatible calls A/R give R without a conflict. A residue
conflict means that the intersection of all informative observations is empty,
including across three or more rows. An internal gap/residue disagreement is an
indel conflict. IUPAC cannot encode base-or-deletion: retain the residue
consensus and count/highlight the gap conflict. Open and locked gaps are
biologically equivalent for merging; originals retain their gap state.

The banner reports informative overlapping positions (at least two calls),
conflicting positions (each column once), residue/indel counts and Q resolutions.
Residue and indel conflicts can occur in the same column, so their counts may
sum to more than the total conflicting positions.

## Optional original trace quality

Available for aligned merging when all selected rows have linked traces. Off by
default, with required Q advantage 10 (configurable 0–255). Only unchanged
original calls have usable Q. Measured Q0 differs from missing Q. Original-call
mappings are followed through gaps, trimming and reverse complement; case-only
changes are cosmetic. Substitutions and synthetic insertions are unscored.

Resolve only if every informative contributor is scored, a highest-Q call is
concrete A/C/G/T, and it strictly exceeds every different observation by at least
the margin. Agreeing best calls are allowed; ties with a different concrete or
ambiguous call, missing Q, edited contributors, insufficient advantage or an
internal gap conflict retain ordinary consensus. Low-Q majorities do not
outvote a stronger original call. N/? observations are ignored.

## Find overlap and merge

Remove alignment gaps and search in the current orientation. No automatic
reverse complement is performed. N/? keep coordinates, but do not cause
mismatches or count toward minimum support. Case is ignored in comparisons,
U equals T, and other IUPAC symbols match literally. The minimum is the number
of informative matching positions, default 20; use 4 for the short original
examples in merge_demo.fasta.

**Maximum differences per overlap** defaults to 0, preserving exact matching.
With a positive value, each substituted residue and each inserted/deleted base
costs one. A two-base insertion consumes two differences. Terminal overhangs
are free. Each added read must fit the allowed total against the current
contig consensus and still have the minimum number of informative matches.
The setting applies per added read/overlap, not to the sum over the entire
multi-read contig. The reported edit-event total is the sum across additions;
it can exceed the configured per-overlap limit. Final conflict counts are
computed from all contributing aligned reads, not from pairwise edit totals.

Tolerant search uses semi-global dynamic programming over exact edit counts,
minimizing edits among overlaps that meet the configured minimum matching
support, then maximizing informative matches to break an edit-count tie. Gap-placement ties
within an otherwise chosen overlap use deterministic traceback. Distinct equally
strong repeat placements are deferred. Consensus retains inserted residues and
uses IUPAC for residue conflicts, with the same missing-information rules as
aligned merging. Q does not relax overlap search. A union created by a previous
conflict remains observed evidence, including a four-base union N; it is not
mistaken for an originally uninformative N during the next comparison.

Search builds components, starting from the longest available read, and selects
the largest found component first. Previously isolated singleton seeds can be
reconsidered after another component grows. This is a conservative best-first
contig tool; it does not exhaustively backtrack through weaker placements or
promise a globally unique/general genome assembly. An eligible exact overlap always wins even when the allowance is positive.
Exact searching runs first; if the best exact placements are tied, the ambiguity
is retained rather than introducing differences to select a different placement.
If no qualifying exact overlap exists, tolerant DP selects the smallest eligible
edit count. This same edit-first priority selects the next read to add. The
maximum allowance does not require using any differences. The component search
is still best-first, not a globally exhaustive minimum-edit multiple assembly. Tolerant comparisons have a
64-million-state traceback limit and a maximum configured allowance of 100;
reduce the allowance or work with shorter reads if a comparison exceeds it.
Preview runs in a cancellable background worker.

## Partial results and saving

If not all selected reads join, the preview still shows every selected name,
the contributing placements and excluded reads. Independently found components
are selectable in Result contig. A component must contain at least two reads.
No singleton or empty consensus is fabricated. Ambiguous/unmatched rows stay
excluded. The table identifies reads belonging to a different found component.

Valid partial contigs can Apply or Save FASTA immediately. The amber banner
and per-read exclusions remain visible; no opt-in checkbox is required. Apply appends only that component at
column 1; existing rows are not realigned. Save FASTA writes the reviewed name,
case and full sequence directly and leaves the source document/history intact.
Changing search settings invalidates the preview. Editing the result name needs
no rescan. Source revision is checked again before Apply or Save. Cancel and
failed searches do not edit the document.

## Search edits versus final conflicts

Maximum differences per overlap is a literal read-to-current-contig search
budget: each substitution and internal inserted/deleted base costs one; terminal
overhangs are free and N/? supply no matching evidence. Other IUPAC codes are
compared as literal symbols. For example R versus A uses one search difference,
although the base sets overlap and the final consensus column is not a conflict.

The preview now reports substitutions, inserted/deleted bases and the subset of
substitutions that are compatible IUPAC symbol differences. These counts sum
search events over read additions. Conflict counts describe unique final columns
with incompatible residue evidence or supported internal gap evidence; multiple
read additions can also revisit one column. The two totals need not agree.
