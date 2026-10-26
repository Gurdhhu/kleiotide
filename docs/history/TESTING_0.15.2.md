# Version 0.15.2 — motif visibility and N-only filtering

Extract into a fresh Kleiotide_0.15.2 folder and build with the existing Qt/MSVC
kit. Existing sessions remain compatible. Also retain TESTING_0.15.1.md coverage.

1. Search a repeated motif with View → residue-colored backgrounds enabled. Every
   other match should have a bright lavender background, readable dark letters
   (or identity dots) and a purple/white outline across all five palettes. The
   current selection retains its normal blue/yellow fill. In colored-letter and
   monochrome modes the subtle light shading remains.
2. Scroll horizontally and vertically, including dense overlapping results and
   a selection. Highlights should track actual columns without stale pixels.
3. The panel has Exclude N-only matches, initially unchecked, and no Select match
   button. Find selects the first result; Next/Previous/F3 still select each hit.
4. Load nucleotide rows NNN, N-~nN, NAN, NNR, and ???. Search NNN with Compatible
   IUPAC, Ignore gaps and reverse complements enabled. Before exclusion, Total=5,
   Forward=5, Reverse=5, Both=5. With exclusion, each is 3: NAN, NNR and ??? remain.
   This checks matched symbols, not the pattern's spelling. Masks remain excluded.
5. Enable Regex and search N{3} with Ignore gaps on. With exclusion off there are
   two matches in those rows; with exclusion on there are none. Search N[-~]*N
   with Ignore gaps off: N-only gap-spanning hits are also excluded. Searching
   [-~]+ still finds gap-only intervals because they contain no N.
6. Toggling exclusion retains previous highlights/counts until Find completes;
   the completed result updates counts, navigation and highlights together.
   Disable exclusion and Find again to restore the original matching behavior.
7. Cancellation, selected-name scope, regex validation, document-change
   invalidation and closing the panel retain their existing behavior. Search and
   filtering do not change sequence text, traces, undo history or session format.

The optional filter builds a rank bitmap for one searched row at a time (about
half a byte per aligned column). Each interval check takes constant time,
including very long overlapping hits. The filter has a 128 MiB per-row working
budget; an exceeded limit reports an error and preserves previous results.
Existing regex/index limits remain as documented in TESTING_0.15.1.md.
