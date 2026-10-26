# Translation and ordinary protein rows in 0.20.3

Translation is a live layer over nucleotide sequences. Its toolbar provides the
AA lettering toggle, codon ruler, coding/noncoding annotation menu, genetic code,
reading frame and stop count. Amino-acid codes are always single-letter. Changing
the lettering toggle never changes sequence data or alignment column widths.
Nucleotide lettering retains Edit; AA lettering is a View preview. Choosing Edit
returns nucleotide letters while retaining translated codon colors.

Coding annotations apply to alignment columns independently of selected names.
Explicit phases override the default frame. Noncoding columns are skipped;
split codons translate if phases follow 1–2–3. Phase errors are flagged rather
than guessed. Frames 2/3 omit leading incomplete codons. Incomplete trailing codons are
translated using N for missing positions: GC resolves to A, AT resolves to X.
An all-gap fragment remains a gap. Copy, counts and snapshots share these rules;
AA labels/snapshots use the middle available coding cell without adding columns. Stop counts cover all
nucleotide rows, excluding proteins and masks; the tooltip explains possible
stops, context-dependent calls, unknowns and incomplete boundaries. Genetic-code
selectors are read-only, with concise labels and full names in tooltips. Custom
code… is their final option. A code created inside a translation preview applies
to that result without changing the alignment defaults.

Alignment → Translation → Generate translation mask appends an ordinary Mask
row named translation_mask. It records the current frame and annotation as one
symbol per alignment column: 1/2/3 for coding phase, # for noncoding. No sequence
data or annotation is changed by generation. The row addition is one Undo step.

Annotate from mask… selects an existing Mask or accepts pasted text. Only 1/2/3
are coding; all other symbols are noncoding. Pasted CR/LF line breaks are ignored;
spaces count as noncoding columns. Short masks fill remaining columns as noncoding;
longer masks are rejected. This replaces all previous coding overrides while
retaining the selected genetic code and frame; one Undo restores the annotation.
Existing masks are read-only in the dialog. Annotate using this mask in the name
context menu applies the clicked Mask directly. Invalid phase transitions remain
visible as annotation errors rather than being silently corrected. Applied
annotations appear as Custom annotation and survive native session save/reopen.

Select nucleotide names and use Add translated sequences… from Sequence or the
name context menu. The preview supplies code/frame/annotation options and counts.
Added protein rows retain alignment width: each AA is stored at the middle
coding position of its codon, with ordinary open gaps in the surrounding cells.
For ATGAAATAA, the stored result is ~M~~K~~*~. Gaps and letters are independent:
select, rewrite, insert, delete, cut, paste, lock/unlock or slide any of them just
as in imported protein rows. No display span binds them into a block. Sources
keep their nucleotide data and trace links; derived rows inherit groups and use
unique _TRANSLATED names. All additions form one Undo step. Source name/ID,
fingerprint, revision and translation settings remain provenance, not edit locks.

Copy translated fragment requires an active, current translation and one
nucleotide row. Selecting part of a codon (including a trailing partial codon) is sufficient: invoking Copy
expands the visible residue selection to include the complete amino acid(s), then
copies canonical AA text. Split codons expand across intervening noncoding cells.
Purely noncoding selections, unresolved boundary fragments, masks, protein rows
and multi-row selections remain unavailable. Ordinary Ctrl+C copies underlying
text exactly and does not expand the selection. No separate expansion action.

Every standard AA has a distinct shade within its Clustal-style color family:
hydrophobic blue; basic red; acidic magenta; polar green; cysteine pink; histidine/
tyrosine cyan; glycine orange; proline yellow. Shades do not depend on gap coverage
or conservation. Ambiguous calls have neutral gray shades; stops retain their
own cue. Letters are adapted for contrast on white, and backgrounds use readable
black/white symbols. Monochrome remains deliberately monochrome.

Translate to a new protein window… remains a separate compact output command,
with one AA per column. Export protein sequences… retains ordinary real gaps.
Saved/copied alignment FASTA likewise retains all alignment columns and gaps.
Native whole-sequence clipboard preserves protein types, groups and provenance;
fragment clipboard uses ordinary alignment text, including independently selected
gaps. Protein MAFFT is available in 0.22; see MAFFT_PROTEINS.md.

Native sessions remain format 4. Old format-4 translated rows with bound codon
spans are validated, recentered and converted to ordinary protein rows on load.
Their old label preferences are ignored; provenance is retained. Existing ordinary
protein rows remain unchanged. Formats 1–3 remain readable. New saves contain
ordinary protein rows and no three-letter preference; version 0.20/0.20.1 can
still read them. Conversion never changes an original file until explicitly saved.
