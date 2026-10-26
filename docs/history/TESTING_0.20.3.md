# Kleiotide 0.20.3 — acceptance checks

Build in a fresh source folder using your usual Qt 6.5+ Desktop/MSVC kit.

1. Enable Translation and annotate coding/noncoding columns. The count label
   says Custom annotation. Generate translation mask: a dark-gray Mask named
   translation_mask has the exact displayed 123 phases and # for noncoding.
   Undo removes only that row; Redo restores it.
2. Use Alignment → Translation → Annotate from mask… and choose a mask or paste
   one. 1/2/3 are phases, all other characters noncoding. Pasted line breaks are
   ignored. Short masks leave the rest noncoding; overlong masks disable Apply.
   Cancel changes nothing. Apply replaces the annotation; Undo restores it.
3. Right-click a mask name: Annotate using this mask applies it directly. It and
   Add translated sequences… have a separate section. Save/reopen a native session
   to check annotation and mask retention.
4. With frame 1, ATGGC translates as MA, ATGAT as MX. Check the final partial
   codon in NT/AA views, translated Copy, snapshots and compact output. Missing
   bases are unknown; all-gap fragments stay gaps. No extra columns are created.
5. Sequence → Sort → By sequence type orders Masks, Proteins, Nucleotides while
   keeping the order within each type. Undo restores the previous row order.
6. Sequence gap commands have their own section; Select and Sort are last.
   View shows Motif search and ends with a separated Alignment font… command.
7. Edit → Rename → Append/Prepend group name… defaults to selected names (or all
   rows when none are selected). Choose a separator and Preview. Ungrouped names
   stay unchanged; Apply/Undo changes all previewed names atomically.
8. Recheck previous translation controls, editing and wide-window/trace scrolling.

Native session schema remains format 4; no migration is needed for this update.
