# BLAST and trace linking — 0.29.0 testing release

## Trace assignment

Sequence → Link trace (AB1) offers selected nucleotide names or all nucleotide
rows without traces. Selected rows can replace existing links; proteins/masks
are excluded. The selected-name context menu also offers Link trace and Delete
trace. Delete removes session associations only, with confirmation and one Undo;
sequence letters and original files remain. Undo restores the links.

Add a batch of AB1 files. Suggestions use the first matching tier: exact filename
stem, mutual substring of name/stem, exact internal AB1 sample name, then mutual
substring of internal name. Comparisons are case-sensitive. Ambiguous best-tier
matches stay unassigned, even if a weaker tier could pick one. File cells are
searchable dropdowns with visible arrows; single-click to choose, or use the per-row Add AB1
button to load and assign exactly one file. Several rows can use the same evidence;
unassigned rows/unused files are allowed. Imported AB1 recovery notes appear in
file-cell tooltips. Filenames are shown before directories; duplicate basenames
are disambiguated by directory. The popup can be wider than the column. Assignment
reasons record filename/internal-name/manual choices; unassigned rows say Not
attempted and recommend manual assignment. Original calls, peaks, qualities and channels remain immutable.

Preview checks forward and reverse-complement orientations after removing open
and locked alignment gaps and normalizing case/U. A bounded local affine
alignment gives terminal trimming/additions free ends. Large queries use
13-base seeds to select candidate windows before alignment. There is no attempt
to globally align a whole genomic assembly to a short trace: query cap is two
million ungapped residues, and each alignment window is limited to eight million
DP cells. Preview output/workspace also respects the bulk-memory setting and a
256 MiB cap; loaded AB1 evidence has a separate 512 MiB cap (or the lower bulk
limit), with a 64 MiB per-file input bound. Automatic name matching yields to
the UI between small chunks. Reduce the scope/trim flanks when these bounds are exceeded.

Equally strong orientations or distinct repeat placements are refused. Within a
placement, all optimal affine traceback paths are considered: a call retains an
original index only when those paths agree. Insertions, terminal additions and
ambiguous near-indel positions have -1, with no invented peak/Q. Unique exact full or trimmed-call
matches retain the original indices, including N/ambiguous calls, when the
orientation is unambiguous and there is enough concrete evidence. Full-call
palindromes do not establish orientation. This is an overlap heuristic, not a
statistical claim that the biological sample identity has been established.

The table reports Forward/Reverse/No match and Perfect/Good/Terrible/Failed.
Perfect means a unique exact full or trimmed-call match or a fully identical mapped query with
no indels. Good uses at least 90% overlap identity; Terrible is weaker accepted
matching evidence (at least 65%). Short/insufficient evidence, ambiguity or bounds
failure gives Failed. Colors are green/amber/red. Review Terrible assignments
and actual traces carefully. These categories are not Phred scores, probabilities,
or a guarantee of mapping correctness. Four previously supplied real AB1 reads
and constructed ground-truth edits were checked locally; broader manual
calibration on research reads remains part of native acceptance.

Changing assignments invalidates Preview. Apply checks the source revision and
adds valid links together in one Undo step. Unassigned/failed rows retain their
current trace associations; remove old links through Delete trace when desired.
Preview/file loading run in workers; Cancel leaves sequence data unchanged.
Linked evidence is embedded/deduplicated in format-4 .kleio sessions as before.

## NCBI nucleotide BLAST (HTTPS)

Use BLAST → NCBI nucleotide BLAST. NCBI protein searches and all BLAST+ remote
search options were removed in 0.29.0. Use LOCAL_BLAST.md for local searches.
Choose selected nucleotide names or all nucleotide rows. Gaps are removed,
case is normalized and ? becomes N. Identical queries share a search and expand
back to their original names and submission ordinals in the table. Only synthetic
query IDs and sequence text are sent; names and traces stay on this computer.

Settings → BLAST separates HTTPS defaults from the optional local executables.
HTTPS offers core_nt and the listed databases, megablast/blastn, target count,
E value, identity/query-cover result filters, documented scoring/masking options
and a monitoring limit. A valid contact email (marked *) is required. Review the
submission summary before sending. Per-search choices do not rewrite defaults.

Contacts are spaced by at least ten seconds and the same RID is polled at most
once per minute. The countdown is the next allowed contact, not a completion
prediction. Requests run asynchronously and only the originating window is modal.
Large batches respect the existing off-peak policy. Query limits remain 100,000
residues per query, 20 million residues/20,000 unique queries per public job,
64 MiB accumulated raw results and 500,000 expanded table rows.

Cancel or the monitoring limit retains completed results and any saved RID.
Resume reuses that RID; it never silently submits another copy. Retired external
remote/protein jobs cannot restart through this interface. Standard older
nucleotide HTTPS jobs remain compatible; older jobs lacking alias ordinals
cannot recover interleaved original numbering that was never saved.

Results have 21 columns. Query numbers follow original submitted order, including
duplicate aliases. Hit numbers restart within each query, ranked by decreasing
identity. Query cover is the union of all HSP query spans; identity, E value,
alignment length, mismatches, coordinates and bit score describe the best
bit-score HSP. GenBank fields can be absent. Fetch GenBank metadata retries
missing supplementary record data without redoing BLAST.

TSV protects formula-like text. XLSX uses literal text, numeric metrics and real
accession hyperlink relationships. Open RID at NCBI opens the same request with
explicit program context. Save diagnostic report omits query sequences; saved
job copies contain sequence text/names. Review paths, RIDs and server messages
before sharing either file.

The user accepted 0.28.1 nucleotide HTTPS, trace linking/mapping and monitoring
behavior on Windows. Another public 76-aa nr query was acknowledged as blastp
but still returned WAITING after more than twelve minutes. This establishes no
client-side cause and does not mean it was processed as blastn. Following the
requested fallback, remote protein search is removed rather than claiming its
server performance has been repaired. See VALIDATION.md and the live evidence.
