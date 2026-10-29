# BLAST and trace linking — 0.28.0 testing release

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
editable searchable dropdowns; double-click to choose, or use the per-row Add AB1
button to load and assign another file. Several rows can use the same evidence;
unassigned rows/unused files are allowed. Imported AB1 recovery notes appear in
file-cell tooltips. Original calls, peaks, qualities and channels remain immutable.

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
ambiguous near-indel positions have -1, with no invented peak/Q. Exact full-call
matches retain the original indices, including N/ambiguous calls, when the
orientation is unambiguous and there is enough concrete evidence. Full-call
palindromes do not establish orientation. This is an overlap heuristic, not a
statistical claim that the biological sample identity has been established.

The table reports Forward/Reverse/No match and Perfect/Good/Terrible/Failed.
Perfect means exact full-call matching or a fully identical mapped query with
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

## Nucleotide / protein BLAST

Use BLAST → Nucleotide BLAST / Protein BLAST, or the selected-name context menu.
The Search options tab contains configuration; Results shows the running job and table. Choose selected names or all matching molecule rows. Masks/wrong types and empty
rows are excluded. Alignment gaps are removed, case is uppercased and ? becomes
N (nucleotide) or X (protein). Other ambiguity symbols and U are retained.
Identical resulting queries share one search and expand back to all names.
The remote FASTA contains synthetic q1/q2 IDs, rather than specimen names;
no trace data or session metadata are submitted.

Defaults: nucleotide core_nt / megablast, 10 target sequences, E threshold 0.05,
local identity 0% and query cover 0%, environmental/uncultured exclusion OFF.
Protein defaults are nr / blastp. Enter a contact email. Before sending, the
confirmation states counts, residues, batches, task/database and backend. The
program does not send a request merely because the dialog was opened.

### Backend/options matrix

| Option | Built-in HTTPS | Optional NCBI BLAST+ remote |
| --- | --- | --- |
| blastn, megablast, blastp; database, E, targets | Supported documented URL parameters | Supported |
| Word size, nucleotide scores, protein matrix/composition | Documented valid combinations | Executable validates task-specific combinations |
| Gap open/extension | Documented valid pairs; 0/0 omits override | 0/0 task default, otherwise explicit arguments |
| DUST/SEG and soft masking | Supported | Supported |
| Automatic short-query adjustment | Supported | Choose a short-query task instead |
| dc-megablast, blastn-short, blastp-short, blastp-fast | Use BLAST+ | Supported by configured executable, subject to its remote-service support |
| Organism/Entrez restrictions, uncultured/environmental exclusion, single strand | Use BLAST+ | Supported remote arguments |
| Percent identity / query cover | Local filters on received results | Same local filters |

Classic advanced controls are visible, with the required backend identified.
NCBI's current documented URL API exposes only a subset of web options. The
program does not rely on undocumented HTTP keys. For advanced searches, install
NCBI BLAST+ separately and choose its **blastn.exe** or **blastp.exe** in the
dialog. This is the optional NCBI C++ Toolkit-backed remote client, not a bundled
local database. A launcher is not required for standard built-in HTTPS searches.
The application starts it directly with an argument list, never a shell command.
An external executable/service may reject a combination; its diagnostic is shown.
No real installed BLAST+ remote success is claimed from a controlled fixture.

### Batches, cancellation and resumable jobs

Batches contain at most 50 unique queries and 100,000 residues. Individual queries
are capped at 100,000 residues; a public job at 20,000 unique queries/20 million
residues, with 16 MiB of original names. Use local BLAST+ or another appropriate
service for very large projects. Contacts are separated by at least 10 seconds,
HTTP polls for a RID by at least 60 seconds. For over 50 batches, new submissions
wait for weekends or 21:00–05:00 US Eastern; existing RIDs may finish. These waits
are asynchronous and cancellable. Public-server queues can take hours.

The job file in the user's application-data blast-jobs folder records queries,
original names, options, completed batches, cached accession metadata and the current HTTP RID. Its path is
shown in status. Save job copy exports a copy; Resume job reads that state.
Malformed jobs cannot replace the current results. Job files contain sequence
data: keep/delete them using the file manager when no longer needed. They are
separate from recovery checkpoints and the recovery quota. They are bounded at
128 MiB; completed raw responses are bounded at 64 MiB, accumulated/expanded hits
at 500,000 rows. Failures retain previous completed batches.

Cancel stops future contacts and kills a running BLAST+ process. Already sent
queries cannot be retracted. HTTP Resume reuses the saved RID; if it expired, the
service reports failure. An interrupted BLAST+ batch must be restarted explicitly,
because its RID is internal to the external client. If HTTP submission failed
before returning an RID, a query might already be running: review that warning
before explicitly restarting. There is no automatic POST retry after uncertainty.

The job/report dialog keeps its source window read-stable. Search/results/export
never edit the alignment or mark it saved. Other Kleiotide processes/windows
retain independent documents. A completed search can be filtered by changing
identity, query cover or E; a looser filter cannot recover hits the server never
returned. Retrieving 10 targets may display fewer than 10 after local filtering.

### Results and exports

One row per original query name and returned hit. Identity, E, bit score,
alignment length, mismatches and coordinates use the highest bit-score HSP.
Coverage is the union of query intervals across all HSPs for the hit, so overlaps
are counted once. Coordinates remain one-based, including reverse hit start/end.
Protein identity is not the BLAST positives/similarity statistic. The table
includes accession link, scientific name, source identifiers, hit title, taxonomy,
authors and publication date/year. Click an accession to open its nucleotide or
protein record. Click headers to sort (numeric columns sort numerically).

GenBank metadata is fetched in batches, cached by accession/version in the job, and can be retried with Fetch GenBank
metadata. Identifiers include isolate, specimen voucher, culture collection,
clone and strain where provided. The first published reference supplies authors
and its available publication year. If only a submission reference exists, its authors are retained but publication date stays blank; other unavailable fields stay blank. This is not
the record-update date. GenBank often provides only a year, not an exact date.
Missing metadata/network failure retains the basic BLAST hit table.

TSV and XLSX exports are atomic, confirm the final destination and protect source
traces/sessions. TSV neutralizes formula-like text and embedded tabs/newlines.
XLSX uses literal inline strings for text and numeric cells for metrics; query
names such as '=name' are never formulas. The small uncompressed OOXML/ZIP writer
has no new third-party dependency. Native Excel/LibreOffice opening remains a
manual check, although ZIP/openpyxl independently validated the test workbook.

## Sources and test boundaries

- https://blast.ncbi.nlm.nih.gov/doc/blast-help/urlapi.html
- https://blast.ncbi.nlm.nih.gov/doc/blast-help/developerinfo.html
- https://ncbi.github.io/cxx-toolkit/pages/ch_blast
- https://doc.qt.io/qt-6/ssl.html
- https://github.com/Gurdhhu/bioinf_scripts (annotate_blast_hits.py remained
  inaccessible in this environment; this implementation was independently
  written from the NCBI interfaces, not claimed as a review/reuse of that script).

Automatic tests use a loopback HTTP server and controlled child executable, never
NCBI. Live NCBI TLS, task/database availability, real BLAST+ and native Windows
rendering need the focused manual checks. Qt Network is now required; the Windows
installer verifies Qt6Network.dll and the native Schannel TLS plugin. It avoids
an untracked OpenSSL dependency and never disables certificate validation.
