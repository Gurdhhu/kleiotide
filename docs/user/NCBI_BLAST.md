# NCBI nucleotide BLAST (HTTPS)

Use BLAST → NCBI nucleotide BLAST. NCBI protein searches and all BLAST+ remote
search options were removed in 0.29.0. See [Local BLAST](LOCAL_BLAST.md) for local searches.
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
prediction. Search windows are modeless and use a snapshot of names and sequences taken when opened; you can edit the originating alignment without changing pending queries.
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

Save table offers XLSX, TSV and CSV, with the same settings and results. XLSX separates them into worksheets; plain tables put settings first. CSV/TSV protect formula-like text. XLSX uses literal text, numeric metrics and real
accession hyperlink relationships. Open RID at NCBI opens the same request with
explicit program context. Save diagnostic report omits query sequences; saved
job copies contain sequence text/names. Review paths, RIDs and server messages
before sharing either file.


GenBank sequence release dates use GBSeq_create-date; Authors uses direct-submission authors. Missing record fields remain blank. A visible status inset distinguishes Ready, Search in progress, Fetching GenBank metadata, Completed, Failed and Cancelled. WAITING is a server status, not a transport error or an estimated completion time. A small query can still wait on NCBI; no client timeout establishes that the remote server stopped its job.
