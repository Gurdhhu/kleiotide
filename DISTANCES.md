# Distance analysis and similarity ordering — 0.21.1

## Current 0.29.2 behavior

Save table offers XLSX, TSV and CSV. XLSX separates calculation settings, the main result and supplementary tables. TSV/CSV retain the full analysis blocks. Copy table copies only the currently displayed results (including matrix count views), without settings or vertical sequence indices. Comparison rules start collapsed; method help remains visible below its selector.

See RELEASE_0.29.2_TESTING.md; it supersedes older UI instructions below.

Statistics has Pairwise distance matrix…, Overall mean distance…,
Within-group mean distance… and Between-group mean distance…. Each uses the same
explicit settings and underlying typed sequence data, independent of the live
translation layer. Protein analysis uses actual Protein rows; create translations
first if that is the intended input. Mixed nucleotide/protein comparisons and
mask comparisons are never calculated.

Choose molecule type, all rows versus a selected-name snapshot, all columns
versus a selected-column snapshot, missing-data deletion, minimum comparable
sites and whether ungrouped sequences are included. Default: p-distance,
pairwise deletion, minimum 1 site, all rows/columns, ungrouped included. Name
selection and residue selection provide independent scopes. A selected scope
with no eligible names or no columns gives a message, never an implicit fallback.
The full-column extent is the longest included sequence; unrelated masks or
other molecule types do not lengthen the analysis. Selected columns use their
exact alignment coordinates. At least two eligible rows are required.

## Comparable states and models

Nucleotide states are definite A/C/G/T, ignoring case and treating U as T.
All IUPAC ambiguity calls (including N and ?), both gap types and absent trailing
cells are missing data. Protein states are the 20 standard amino acids, ignoring
case; B/Z/J/X/?, U/O, stops and gaps are missing data for these models. This does
not modify any symbols. Conservation's fractional votes are not distance votes.

| Method | Molecule | Estimate and domain |
| --- | --- | --- |
| Observed differences | Nucleotide / Protein | Raw unequal comparable sites, without length normalization |
| p-distance | Nucleotide / Protein | `p = differences / comparable sites`, a fraction, not percent |
| Jukes–Cantor | Nucleotide | `−3/4 ln(1−4p/3)`; unavailable at `p ≥ 0.75` |
| Kimura 2-parameter | Nucleotide | `−1/2 ln(1−2P−Q) −1/4 ln(1−2Q)`; both log arguments must be positive |
| Poisson correction | Protein | `−ln(1−p)`; unavailable at `p = 1` |

P is the fraction of transitions (A↔G or C↔T); Q the fraction of transversions.
Corrected distances use substitutions per site. JC assumes equal base
frequencies and equal substitution rates; K2P permits transition/transversion
rate differences but retains equal base frequencies. Corrections use uniform
rates among sites; Poisson is the simple equal-rate protein correction. Gamma
rates, bootstrap errors and additional substitution models are outside this stage.

Pairwise deletion excludes missing states separately for each pair. Complete
deletion removes a column if **any included row** lacks a definite state there,
then uses that common filter for every pair. For group means this filter spans
the whole selected analysis population, not a separately filtered group.

Zero overlap, overlap below the chosen minimum, and invalid correction domains
are distinct N/A reasons. They are never displayed as zero. Matrix tooltips give
the reason, comparable-site count and observed differences; the display selector
also shows the complete comparable-count or raw-difference matrix. A diagonal is
zero only when that row itself meets the same overlap rule; otherwise it is N/A.

## Group and overall means

Overall means use each unordered pair once. Within-group means use unordered
pairs within each group; between-group means use every cross-group pair once.
Only valid distances enter an unweighted arithmetic mean: this is not pooled
mismatches divided by pooled sites. Report valid and excluded pair counts,
separate no-overlap/minimum/saturation counts, and the mean/min/max comparable
sites among valid pairs. A singleton group has no within-group estimate (N/A).

Only groups with eligible members appear; a group can still have no valid pairs.
When enabled, ungrouped rows form an explicit Ungrouped category. Disabling them
removes them from the entire analysis, including complete-deletion filtering.
At least two represented groups are needed for between-group results. Group
membership follows stable row IDs, including after renaming or reordering.

## SD and standard error in mean reports

For each overall, within-group or between-group mean, let m be the number of
valid pair distances used in that estimate and d_i their values **after applying
the chosen distance model**. Each pair has equal weight. Unavailable pairs
(no overlap, insufficient overlap, saturation) are excluded from all three
statistics; they do not contribute zeros or count toward m.

- Mean = sum(d_i) / m.
- Sample SD = sqrt(sum((d_i − mean)^2) / (m − 1)). This describes the spread of
  valid pair distances, using the sample-SD convention rather than division by m.
- **SE (naive) = SD / sqrt(m)**, the conventional standard error under independent
  observations. Here m counts pairs, not sequences, groups, sites or bootstrap
  replicates. Pairs sharing sequences are dependent, so this value does not
  account for that dependence and must not be read as a bootstrap or otherwise
  dependence-adjusted uncertainty estimate for evolutionary distance.

SD and naive SE have the same units as the distance. They are N/A for m < 2;
a single valid pair still has a reportable mean. With at least two equal valid
distances, SD and naive SE are both zero. No confidence interval is inferred.
The result table places SD (pairs) and SE (naive) beside the mean; explanations
are visible in the dialog and available on header/cell hover. TSV comments carry
the formulas, denominator, exclusions and independence limitation.

A Welford accumulator computes centered squared deviations in one streaming pass
with constant storage per mean, avoiding subtraction of large raw second moments.
The existing compensated sum still supplies the reported mean.

## Similarity sorting

Sequence → Sort → By decreasing similarity… opens a preview using raw observed
difference counts. Choose molecule/scope/deletion/minimum overlap as above.
The first eligible row is retained; the next row is the closest remaining row
to the last one placed (nearest-neighbor chain). This is not fixed-top ranking,
percent identity, a corrected evolutionary model or a phylogenetic tree.

Ties prefer more comparable sites, then original order. A streaming first pass
identifies rows having no valid comparison anywhere; these go last in original
order, except that the first eligible row always remains first. When a chain
cannot connect to remaining comparable rows, restart from their first original
row. Two streaming passes avoid retaining a full quadratic matrix. Rows outside
the chosen molecule/name/group scope retain their exact slots; only eligible
slots receive the new order. Masks never become a biological reference. Raw
counts can favor short overlaps; the minimum-site control is explicit.

Preview and Cancel change no data. Apply is a single undoable row permutation,
preserving IDs, groups, symbols, traces and annotation. No result is applied after
cancellation, errors, or a changed source revision.

## Export, cancellation and resources

Calculate/Preview runs in a worker with cancellation checks inside long column
comparisons. Settings changes clear old results; no stale result can be copied
under new settings. Close during a job cancels it before the dialog closes.

Copy TSV and Export TSV identify the exact Kleiotide build version and include
all analysis settings, sequence indices/IDs/names,
group IDs/names and eligible group sizes. Matrix exports include distance and
comparable-site matrices plus explicit reasons for unavailable estimates. Indices
keep duplicate sequence names unambiguous. Tab/newline characters in labels are
converted to spaces in TSV. Means include exclusions and overlap denominators.
Numeric output uses a decimal point and 12 significant digits.

The Qt model displays only requested cells. Matrix storage is one lower triangle;
means stream pairs into summary accumulators. Before allocation, output above
128 MiB is rejected with guidance to select fewer names or use means. Complete
deletion's byte-per-column filter separately has a 128 MiB bound. Clipboard text
is bounded to 64 MiB; file export streams the full table to an atomic save file.
Cancellation/error preserves the existing destination file and clipboard.
Analyses are transient; native session format remains 4.

## Primary references checked 19 October 2026

- [MEGA distance estimators and formulas](https://megasoftware.net/mega1_manual/Distance.html),
  equations 4.1, 4.6, 4.10, 4.48 and 4.50.
- [MEGA XII Jukes–Cantor](https://www.megasoftware.net/web_help_12/Jukes-Cantor_distance.htm)
  and [Kimura 2-parameter](https://www.megasoftware.net/web_help_12/Kimura_2-parameter_distance.htm).
- [MEGA XII Poisson correction](https://megasoftware.net/web_help_12/Poisson_Correction_PC_distance.htm).
- [MEGA XII analysis preferences](https://www.megasoftware.net/web_help_12/Analysis_Preferences_Distance_Computation.htm)
  and [pairwise deletion](https://www.megasoftware.net/web_help_12/Pairwise-deletion_option.htm).

Formulas follow these references. Strict ambiguity/20-AA filtering, unweighted
valid-pair means, resource limits and the chain sort are explicit Kleiotide
choices; they do not imply all MEGA options or identical missing-data defaults.

SD/SE formula references checked 19 October 2026:
[NIST sample standard deviation](https://www.itl.nist.gov/div898/handbook/mpc/section3/mpc3332.htm)
and [NIST mean standard-error term](https://www.itl.nist.gov/div898/handbook/eda/section3/eda352.htm).
The independence limitation for shared-sequence pair distances is why the UI
explicitly labels the SE as naive.

## Similarity sorting help (0.22)

The sort dialog and command tooltip now state the existing missing-state policy
explicitly: N, ?, other ambiguities, gaps and missing cells contribute neither
differences nor comparable sites. N–N is not a match. Only definite nucleotide
states (U=T) or the 20 standard amino acids participate. Rows without valid
overlap go last, except the retained starting row; minimum-overlap and deletion
settings still apply. This changes explanation only, not ordering calculations.
