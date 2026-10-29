# Focused Windows checks — 0.28.0 test 1

1. Build with the same Qt 6.5.3/MSVC2022 x64 Release kit in a fresh directory.
   Enable release checks; expect 41/41. test-installer uses the exact matching
   inner Source_For_Installer ZIP and revision 1. Qt Network is an added Qt-base
   component; deployment verifies Qt6Network.dll and tls/qschannelbackend.dll.
2. Merge aligned / Find overlap: read preview fits the row count. Copy still
   produces the same equal-width FASTA. Many reads retain scrolling with a
   12-visible-row cap. Try narrow windows, 100/125/150/200% and both merge modes.
3. Open an alignment with nucleotide sequences without traces. Link a batch of
   ordinary AB1s; inspect filename/internal-name suggestions, ambiguous/unassigned
   cases, manual searchable dropdowns and per-row Add. Assign one trace to two
   rows. Test selected scope, selected relinking and all untraced scope.
4. Preview original, reverse-complemented, trimmed and lightly edited copies.
   Inspect orientation/colors/categories, actual peak positions and unknown
   positions near indels/homopolymers. Repeats/palindromes should fail rather than
   pick a strand/placement. Read BLAST_AND_TRACE_LINKING.md for heuristic limits.
   Change assignment and confirm Apply is disabled until another Preview.
5. Cancel a Preview, then Apply a reviewed mapping. Letters remain unchanged;
   all valid links have one Undo/Redo. Delete links using both menus, confirm,
   then Undo. Save/reopen .kleio, move original AB1s temporarily and verify the
   linked original calls, signals/Q and orientation remain. Never delete originals.
6. BLAST: start with disposable copies of **public/non-sensitive** nucleotide
   sequences (~300–1000 bp). Opening the dialog must send nothing. Enter contact
   email, keep core_nt/megablast/10/.05 defaults and identity/coverage 0. Inspect
   the submission confirmation, accept and wait through the server's queue.
   Keep the report/job and confirm no source modification. Real NCBI validation
   is manual; automatic fixtures do not establish a live-server result.
7. Repeat with a public protein query using nr/blastp. Inspect accession links
   (nuccore/protein), identities, union query cover, best-HSP coordinates and
   mismatches, metadata qualifiers/taxonomy/authors/date/year. Compare selected
   examples with NCBI, allowing for changing database contents. Missing values
   stay blank, and record-update dates must not become publication dates.
8. Include exact duplicate ungapped rows and confirm the confirmation lists
   fewer unique queries while each name gets results. Change local identity/
   coverage/E filters, sort numeric columns and check retrieved-target semantics.
   Save TSV/XLSX, open in Excel/LibreOffice and inspect numeric cells, Unicode,
   formula-like names, refusal/cancel and protection of AB1/.kleio destinations.
9. Cancel after an HTTP RID, close/reopen and Resume job. It reuses that RID;
   an expired/failed RID is reported. Try malformed job data and a failed network:
   current results and completed batches remain. Job files contain sequences and
   names and are independent of recovery quota; retain/delete them intentionally.
10. Optional advanced backend: install NCBI BLAST+ separately, choose blastn.exe
    or blastp.exe (not MAFFT or a shell script), and test a tiny remote task with
    an organism restriction or environmental exclusion. Review task-specific
    scores/word size; keep diagnostics for rejected combinations. Cancel a job;
    it stops the child process. Resume explicitly restarts an interrupted batch.
11. Test dialogs at all scaling levels and on colleagues' machines without Qt
    installed. Check an actual HTTPS search: missing TLS packaging must produce
    an error rather than silently downgrading security. Do not disable certificates.
12. ARM real MAFFT Automatic/fallback testing remains pending and can use this
    or a later version. No native ARM build or new ARM success is claimed.
