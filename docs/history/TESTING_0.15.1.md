# Version 0.15.1 — regex and complete motif results

Build from a fresh Kleiotide_0.15.1 folder with the existing Qt/MSVC kit.

1. Open Edit → Find motif (Ctrl+Shift+F). In literal mode, search AAA in AAAA:
   both overlapping intervals should count. Find automatically selects/shows the
   first match. F3/Next and Shift+F3/Previous select each visited match.
2. All matches have a purple outline and light shade, including matches outside
   the current viewport when you scroll to them. Click elsewhere or change name
   selection: the completed highlights remain. Closing the panel removes them;
   the normal editing selection remains independently selected.
3. Check Total, Forward, Reverse complement and Both. For AT in ATAT with reverse
   complements enabled, Total=2, Forward=2, Reverse=2, Both=2. Total counts each
   aligned interval once, while Both contributes to both strand counts. With
   reverse disabled, Reverse/Both are zero.
4. Enable Regex: try A[AG]G, (AAG|CTT), A.{2,4}G, A{2,3}, ^AAG$ and (AC)\1.
   Matching is case-insensitive and includes overlaps. Regex operates on the
   forward sequence and, optionally, its reverse complement; anchors refer to
   each searched orientation. All results map back to original alignment columns.
5. Regex dot is a wildcard. With Ignore gaps off, ~ matches an open gap and - a
   locked gap. In regex mode Ignore gaps removes gaps from the searched sequence,
   not from pattern syntax. IUPAC compatibility is disabled: write explicit
   classes such as [AG]. Literal-mode IUPAC and dot-as-open-gap behavior remain.
6. Empty regex matches (for example ^ alone) have no residues to select and are
   ignored, with their count shown. Invalid patterns and resource-limit failures
   are reported inline, preserving the previous completed results. Test Cancel
   during a search: previous counts/highlights/selection should remain unchanged.
7. Search selected sequence names, including an empty selection and mask-only
   selections. Neither silently searches all rows. Masks are excluded. Verify
   a gap-spanning forward hit and reverse-only hit, and duplicate titles.
8. Try every palette, monochrome/background modes and identity dots. Other hits
   should remain outlined/shaded; blue/yellow editing fills retain their color.
   Scroll both directions with dense results, long overlapping intervals and a
   selected block, including at normal Windows display scaling.
9. Editing/undo/reordering/replacing the document invalidates old coordinates and
   clears results, with a refresh message. Query/options/scope changes retain the
   previous completed overlay until the next successful Find or panel close.
10. Unwrapped FASTA output and existing .kleio trace/view persistence remain as in
    0.15. Search results/regex/counts are transient and are not sequence data.

The first Find scans the entire scope to count and index results. Navigation then
uses the index; it does not repeat the search. Repeated arithmetic hit runs are
compressed, and painting resolves only visible columns. Regex is subject to
bounded PCRE2 engine work; a failed/over-budget job publishes no partial results.
The encoded index budget is 512 MiB; regex also has a 16-million-column per-row
working limit and a 128 MiB per-row temporary-hit budget. Actual memory includes
container capacity and temporary data. Narrow the scope/refine a pattern if a
limit is reported; counts are never silently truncated.
