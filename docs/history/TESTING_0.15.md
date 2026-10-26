# Version 0.15 — unwrapped FASTA and motif-hit navigation

Extract into a fresh Kleiotide_0.15 folder and build with the working Qt/MSVC kit.
There is no new dependency or native binary format version.

1. Open FASTA with wrapped sequences longer than 80 characters, mixed case, both
   gap types, empty rows and masks. Save and copy selected sequences as FASTA:
   each header is followed by one complete sequence line. Reopen the output and
   confirm the same symbols. Existing wrapped inputs still work.
2. Edit → Find motif… (Ctrl+Shift+F) opens the panel. Ctrl+F still opens name-regex
   selection. Find starts at the first hit; Previous/Next or Shift+F3/F3 navigate
   row order and then column order, including overlapping matches.
3. Try AAA in AAAA: starts 1 and 2 are both visited. Use duplicate sequence titles
   and masks to check row identity and mask exclusion. A hit is a purple outline;
   name selection, residue selection, caret and editing mode remain unchanged.
4. Test Ignore gaps with A-~A~G and motif AAG. The outline spans the actual aligned
   columns including intervening gaps. With Ignore gaps off, gap types/positions
   must match literally. Query dots denote open gaps. Matching ignores case.
5. Enable reverse complements and check a reverse-only CTT hit for AAG. The result
   states the orientation. Palindromic motifs and compatible hits on both strands
   are visited once per interval and labeled both strands.
6. Compatible IUPAC is optional: R may match A/G and N/? may match any nucleotide.
   Off means literal ambiguity codes; T/U are compatible only when enabled.
   Whitespace in a query is ignored; at most 4096 searchable symbols are allowed.
7. Wrap around reports boundary crossings. Turn it off: a failed next/previous
   leaves the current match visible. Selected sequence names searches only those
   rows; an empty selected scope does not search the whole alignment.
8. Search a far-off row/column: it scrolls into view. Use Select match to make the
   interval the editing selection, then edit/copy as usual. Existing name-selection
   clipboard priority remains: selected names copy FASTA; clear names to copy a
   single-row substring.
9. On long data, Cancel search should restore editing and retain the previous
   match. Closing the panel cancels a running search and clears its outline.
   Invalid input is explained inline. Changed query/options/scope or any document
   revision clears old hits; Undo does not resurrect a stale search result.
10. Try every palette, colored backgrounds, monochrome and identity dots. The
    purple outline has a white halo and should remain visible without replacing
    blue/yellow editing cues. Scroll horizontally/vertically with wide hits and
    selections, including at normal Windows display scaling.

Search is transient: it does not add undo entries, change sequence data/trace
evidence, or persist query/cursor/outline in .kleio. Native sessions still retain
the actual editing selection and view preferences. Search jobs temporarily lock
document editing, are cancellable, and reject results for a changed document.

The navigator retains one hit rather than a complete match list. Forward matching
resumes after the current start; previous-hit lookup scans the relevant row prefix
in a worker. Repetitive data therefore does not allocate a huge hit table, though
backward traversal of very long rows can take longer and remains cancellable.
