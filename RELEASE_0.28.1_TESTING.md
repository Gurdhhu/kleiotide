# Kleiotide 0.28.1 — focused Windows acceptance

Use the same Qt 6.5.3/MSVC2022 x64 Release kit, a fresh build directory, and
revision 1 with the matching 0.28.1 Source_For_Installer ZIP. Expected: 41 tests.
Qt Network / Windows Schannel deployment requirements are unchanged.

1. Link trace: inspect the dropdown arrow before editing; single-click to choose.
   Filenames appear first, paths in tooltips; duplicate basenames include their
   directories. Open a long filename list and search; its popup may be wider than
   the column. Narrow/widen the dialog and inspect 100/125/150/200% scaling.
   The table uses the available width; initial height follows row count, with a
   16-row cap and scrolling. Assignment reasons distinguish filename/internal AB1
   name/manual matches; ambiguous or absent names recommend manual assignment.
   Unassigned rows must say Not attempted in both mapping columns after Preview.
2. For the supplied 7263 trace, paste the supplied trimmed sequence, assign the
   AB1 and Preview: Forward / Perfect; all retained residue indices must point
   into the original calls at the correct trim offset, including N calls. This
   category concerns mapping certainty, not Phred/trace quality. Test reverse,
   original full-length, edited and repeated reads too. Apply/Delete/Undo/Redo
   and session evidence persistence must still work without changing letters.
3. Settings → BLAST: set contact email, both executable paths, molecule-specific
   databases/tasks, target count, E value, local identity/cover filters, filtering,
   backend preference and per-batch monitoring limit. Apply/Cancel/Restore defaults
   must follow ordinary Settings semantics. Restart and inspect new nucleotide
   and protein dialogs. Per-search choices must not rewrite global defaults.
   Configured executable paths alone must keep standard searches on HTTPS;
   advanced options select BLAST+ automatically. Database menus are not editable.
4. Clear email and click Search NCBI on Search options: a visible warning must
   explain what to enter; no submission occurs. Hover every option and Results
   button for concise explanations, especially Fetch GenBank metadata (retry
   missing supplementary record data, without redoing BLAST).
5. Repeat real HTTPS blastn and blastp using small public/non-sensitive queries.
   Protein SEG now defaults off, matching the documented NCBI URL API default.
   Results show query number in original submitted order (including duplicate
   aliases), hit number within each query ranked by descending Identity, and HSP
   count after query cover. Verify sorting, linked accession and 21-column
   TSV/XLSX numeric/text cells in Excel/LibreOffice. Existing metric definitions
   and local filtering rules still apply.
6. During a pending search, watch the changing next-contact countdown, elapsed
   time, timestamped WAITING replies/poll count and request/metadata phases. The
   60 seconds is the minimum interval before another RID poll, not a predicted
   completion time. Metadata must never show batch 2/1. Cancel and Resume must
   reuse a saved HTTPS RID. Old 0.28.0 jobs still load; old duplicate-alias order
   cannot be reconstructed if it was not recorded by that version.
7. If blastp stays pending, use Open RID at NCBI to inspect the same request,
   then Save diagnostic report and Save job copy. Keep the RID, timestamps and
   report. Do not submit repeated copies to test whether it has finished. A
   server WAITING response is distinguished from a transport/client failure.
8. Test actual separately installed NCBI blastn.exe/blastp.exe -remote with a
   small advanced search. Results diagnostics show executable, arguments, stderr
   and changing elapsed time; a running process alone is not evidence that NCBI
   is making progress. Test Cancel and an intentionally short monitoring limit
   on disposable public queries. Completed results survive; HTTP keeps its RID.
   At the limit the external process stops with an explicit message. Restarting
   an interrupted external batch requires confirmation and can duplicate a
   still-running server submission. Restore your intended monitoring limit.
9. Failed/expired RID and network errors must stop visibly and retain completed
   hits. Save the diagnostic report for errors rather than only screenshots.
   Reports omit query sequences but may contain paths, RIDs and server/client
   messages; inspect them before sharing. Job JSON contains sequences/names.
10. Repeat clean-machine HTTPS/TLS checks and ARM MAFFT Automatic/fallback when
    available; these remain outstanding acceptance items, not established here.

A long queue wait cannot be fixed by claiming a result or polling faster. NCBI
requires at least 10 seconds between contacts and 60 seconds between RID polls.
See BLAST_AND_TRACE_LINKING.md for definitions and official reference links.
