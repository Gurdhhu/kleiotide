# Release split and implementation plan — 27 October 2026

## 0.27.1: corrections to the accepted 0.27.0 checkpoint

| Feedback / request | Decision and delivered behavior |
| --- | --- |
| Slow overview after an overwrite | Reuse bounded summaries. Length-preserving edits invalidate affected summary bins, coalescing quick edits and Undo/Redo. Length, row-order/type or membership changes require a full scan. A single-nucleotide-row fast path reduces initial scan work. |
| Recovery preferences wording | Disabling recovery or restoring defaults preserves already completed files. These actions need not create a new checkpoint. Test instructions clarified. |
| FASTA suffix | Append .fasta when no recognized FASTA suffix is typed. Keep recognized suffixes case-insensitively. Confirm replacement against the final path; keep AB1/session protections. |
| Merge preview | Show original reads in final contig coordinates, alongside consensus, with all symbol-difference columns highlighted. Virtual painting avoids one widget per residue. |
| Partial merge | Apply/Save enabled for a valid multi-read partial contig by default. Amber notice and excluded-read table retained; remove opt-in checkbox. |
| Overview caret/selection and viewport | Bright magenta caret/range with pointer; contrasting white/blue viewport frame. Overlay changes need no rescan. |
| Rename Rewrite | User-facing Overwrite; stable enum/serialized IDs remain compatible. |
| View menus | Motif search, Alignment overview, Chromatogram grouped with Editing controls in View → Toolbars. |
| Toolbar layout | Translation toggle before stretch, beside palette. Translation controls stay on their own lower row, including after Restore default layout. Nonfloating toolbar. |
| About | Clickable GitHub link and version-specific preliminary software citation. No invented DOI. |
| Settings menu | Separate File section immediately above Exit. |
| Dropped FASTA/AB1 | Append through the existing Add sequence from file path, preserving rows/evidence, batching into one Undo; local supported files only. |
| Windows Open with loop | Installer refreshes Applications commands/SupportedTypes and versioned ProgIDs, refreshes existing legacy commands, notifies Explorer. Uninstall removes shared entry only if owned by this installation. Per-user repair script included. Native loop reproduction/retest required. |
| Manual checkpoint action | Labeled “testing only”; remove the action before public release while keeping periodic recovery. |
| Apply to current session | Explicit appearance/editing-page draft action below “Use current session appearance as defaults”. Does not save defaults or change genetic code; Cancel does not undo an explicit application. |

## 0.28.0: new analysis and trace workflows

### A. Link and delete trace (previous checkpoint C)

Implement the previously approved core first: batch filename/internal-name matching
with ambiguity left unassigned; manual per-sequence assignments and Add file;
many rows may share one trace; selected or all untraced nucleotide scope. Then a
cancellable forward/reverse mapping preview that tolerates long terminal edits
and few internal indels, refuses ambiguous placements, and reports orientation
and calibrated mapping-quality categories. Preserve immutable original calls,
peaks, Q and signals; unmapped edited residues use -1. Apply atomically in one
Undo step after checking source revision; delete unlinks only. Add Sequence and
selected-name context actions after ground-truth tests pass. Existing native
session format already represents shared evidence and mappings.

### B. Remote nucleotide/protein BLAST

1. **Backend and options capability matrix.** Compare the official C++
   CRemoteBlast API with the documented Common URL API before selecting the
   backend. The latter explicitly supports a subset of web options; do not
   invent undocumented parameters to imitate every web control. Use a supported
   C++ API path where necessary for required classic options. Cover blastn,
   megablast, discontiguous megablast, blastp, short-query behavior, database,
   organisms/exclusions/Entrez restrictions, word size, score/reward/penalty,
   gap costs, low-complexity/soft masking, strand, composition statistics,
   E threshold and hit limit. Validate task-dependent combinations and document
   any server limitation visibly before submission. Avoid silent option loss.
2. **Menus and query capture.** BLAST → Nucleotide BLAST / Protein BLAST and a
   separate selected-name context section. Selected/all scope for one molecule;
   exclude masks and wrong-molecule rows. Strip alignment gaps, capture stable
   IDs/names/revision, validate empty/oversized inputs. Show sequence count,
   unique queries, residues, database and options before sending to NCBI.
   Submit no traces/signals or unrelated document metadata.
3. **Deduplication and bounded batches.** Hash unaligned sequence bytes, verify
   equality before sharing results, preserve every original query name. Define
   case handling explicitly; do not silently equate different ambiguity calls.
   Bound batches by both residues and count, split oversized requests and
   retain per-batch errors. Exact duplicate results expand back to every row.
   Large local datasets may require local BLAST+ instead of public servers.
4. **Job state.** Asynchronous submission, RID persistence, compliant scheduled
   polling, bounded parsing/download sizes, timeout/backoff, restart/resume,
   partial completion and cancellation. NCBI currently asks for >=10 seconds
   between contacts and >=60 seconds between polls for the same RID, with tool
   and contact-email parameters. Cancellation stops further polling/submission;
   it cannot retract query data already sent. Keep a reproducible options/log
   record without putting secrets in logs.
5. **Defaults requested by Oleg.** blastn core_nt; environmental/uncultured
   exclusion exposed but OFF; megablast; max targets 10; E threshold 0.05;
   identity and query-coverage filters 0%. Other classic options exposed with
   suitable task defaults. Identity/coverage are local result filters, so a
   top-10 server search can display fewer than 10 rows after filtering; explain
   this and allow a larger retrieval limit. Protein database/task defaults must
   be explicit and independently reviewed.
6. **Results and enrichment.** Query name, identity %, accession link, scientific
   name, source identifiers (culture collection/isolate/voucher/clone/strain),
   query cover %, E value, alignment length, mismatches, query/hit coordinates,
   bit score, hit title, taxonomy, authors and publication date. BLAST results
   do not guarantee all GenBank qualifiers/publication fields: batch-fetch
   missing record/taxonomy metadata through supported NCBI services, cache by
   accession/version, leave unavailable fields clearly blank. Do not confuse
   record-update dates with publication dates. For proteins link to the protein
   record; for nucleotides link to nuccore. Preserve one-based coordinates and
   reverse-strand orientation.
7. **Metric definitions.** Decide one row per hit versus HSP explicitly. Recommended
   default: one row per query/accession; best HSP supplies identity/E/alignment
   fields, coverage uses the union of query intervals across that hit's HSPs,
   not a sum that double-counts overlap. Offer HSP detail and label metrics so
   filters and exports can be independently reproduced. Protein similarity
   (positives) is distinct from percent identity.
8. **Tables and export.** Model/view table, sorting/filtering without duplicating
   large strings, clickable accession delegate, TSV and XLSX atomic export with
   final-path confirmation, Unicode and spreadsheet formula-injection handling.
   Choose/test an XLSX writer and package its dependency/license notices.
   Saving a report must not mark the alignment session saved.
9. **Verification gates.** Frozen nucleotide/protein server-response fixtures,
   multiple HSPs, reverse hits, no hits, duplicates across batches, ambiguous
   names, metadata absent, malformed/oversized responses, rate-limit errors,
   resumed RIDs, cancellation and atomic export failures. Use a fake HTTP server
   for automatic tests. Only small, explicitly reviewed live NCBI tests; no
   automatic suite should upload research data or require internet access.

### References and limits

- Official C++ BLAST API: https://ncbi.github.io/cxx-toolkit/pages/ch_blast
- Supported URL options: https://blast.ncbi.nlm.nih.gov/doc/blast-help/urlapi.html
- Usage policy: https://blast.ncbi.nlm.nih.gov/doc/blast-help/developerinfo.html
- Requested reference: https://github.com/Gurdhhu/bioinf_scripts
  (annotate_blast_hits.py). The repository/script could not be fetched in this
  environment on 27 October; no script review is claimed. Read it before the
  0.28.0 implementation, or use an attached copy if retrieval remains blocked.
- Windows registration: https://learn.microsoft.com/en-us/windows/win32/shell/app-registration

The two new workflows belong in 0.28.0 because they introduce substantial
scientific mapping or network/backend/dependency behavior. They will be delivered
in testable checkpoints; 0.27.1 does not include placeholder BLAST or trace-link
menus. Native ARM is still the x64 build under Windows 11 emulation. Chunked
storage, virtual Flush, compressed Undo and a new session format stay deferred.
