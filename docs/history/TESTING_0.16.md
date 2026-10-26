# Version 0.16 — sequence types and native-session foundation

Extract into a fresh Kleiotide_0.16 folder; build with the usual Desktop Qt/MSVC
kit. The shipped Qt minimum remains 6.5. This stage adds types/import/persistence,
with neutral protein editing. Clustal-style coloring, translation/codon labels
and protein MAFFT remain later roadmap milestones.

1. Open an existing nucleotide FASTA or .kleio session normally. Default File →
   FASTA import type is Nucleotides + masks, retaining previous behavior. Existing
   unexpected-symbol masks—including protein-like masks—stay masks.
2. Choose File → FASTA import type → Proteins + masks, then open
   examples/sequence_types_demo.fasta. The first two rows should show P badges,
   even the ACGT row; the digit-containing annotation shows M. N denotes a
   nucleotide. Hover a name for its declared type and linked-trace information.
   The preference applies to Open, Add and FASTA clipboard imports in this window.
   File extensions alone do not determine types; .faa is supported in file filters.
3. In Masks only mode, all imported FASTA rows—including ACGT—are masks. AB1
   imports always remain trace-linked nucleotides, regardless of FASTA preference.
   Saved sessions use stored types and also ignore that preference.
4. Select sequence names, then Sequence → Set sequence type. Change ACGT from
   protein to nucleotide and back; text, gaps, case and row IDs remain unchanged.
   One Undo/Redo restores the complete batch. Changing the protein row containing
   E/F to nucleotide must fail inline without partially changing other selected
   rows. Trace-linked rows cannot become protein/mask; cancelling changes nothing.
5. Add new sequence has an explicit type selector. Choose Protein and enter
   WQX*. Try typing/pasting amino acids in Insert/Overwrite, deleting, selecting,
   copying a single substring, and sliding a mixed protein/nucleotide/mask block.
   Existing caret, selection and undo behavior should remain. Invalid typed
   symbols are rejected according to the row's type without silently making masks.
6. Proteins render neutrally in all nucleotide palettes/display styles. Selection
   remains blue/yellow. Identity dots compare to the actual top alignment row only
   when both rows have the same biological type; masks retain their symbols.
7. Nucleotide motif search, overview and statistics exclude proteins/masks. Reverse
   complement skips both, even for ACGT declared as protein. Duplicate-sequence
   selection and identical grouping treat different types separately. General
   name/length sorting and gap operations preserve declared types.
8. Current MAFFT is nucleotide-only. Whole/column jobs containing protein rows
   and selected protein targets are rejected before setup/launch. Existing
   nucleotide-only jobs should still work. In target/template jobs, unselected
   protein rows retain their original projected columns and type.
9. Save the mixed typed document: normal Save prefers .kleio so type metadata is
   retained. Reopen with the opposite FASTA import preference: saved types remain.
   Export FASTA still writes one complete canonical sequence per line; it carries
   no type metadata, so choose the intended import type when reopening text.
10. Open an old session with traces and masks; save/reopen it in 0.16. Trace signals,
    qualities, mappings, sequence edits, names/order and view should survive. Newly
    saved sessions use format 2 and require 0.16+. Reopened history starts fresh.
11. Verify motif highlighting, smooth scrolling, overview navigation, font/name
    sizing and traces at normal Windows DPI and 150% scaling. N/P/M badges should
    stay inside the name area and leave the trace sign visible.

Accepted protein input is case-insensitive for validation and case-preserving in
storage: the standard one-letter amino-acid alphabet, B/Z/J/X/U/O, *, ?, and gap
symbols. Dot input normalizes to open gap ~; - remains locked. Unknown printable
FASTA symbols cause a whole-row mask, as before. Mask symbols remain printable
ASCII except spaces and >. Type assignment is declaration, not translation.

Source validation: configure BUILD_TESTING=ON and run CTest. The optional
KLEIOTIDE_BUILD_GUI_SMOKE_CHECK target kleiotide_stage016_check exercises the
actual GUI workflows; tests/fixtures/session_v1.kleio is an independently encoded
legacy migration fixture, not a user's chromatogram.
