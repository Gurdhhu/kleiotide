# Kleiotide 0.20.2 acceptance

Extract into a fresh Kleiotide_0.20.2 folder and build with the usual Qt >=6.5/MSVC
kit. Native sessions remain format 4. Core and GUI tests use synthetic fixtures;
Windows/MSVC and physical-monitor acceptance remain to be checked.

1. Translation toolbar and Add translated sequences have no three-letter mode,
   encoding selector or three-letter copy command. Existing preferences cannot
   restore them. Code dropdowns and custom-code editing still work. Check the
   small spaces between controls at normal/narrow widths and 100%/150% scaling.
2. Translate ATGAAATAA and add its result to the current alignment. The ordinary
   protein is ~M~~K~~*~: centered single letters and open gaps. Click/select/cut
   just a gap, insert it back, overwrite just M, slide M one column, delete just
   M or select a block crossing part of its original codon. No token expands or
   blocks ordinary editing. Undo/Redo must retain text, type, groups and provenance.
3. Check existing .kleio files from 0.20/0.20.1 with translated protein rows:
   old spans become centered ordinary letters/gaps, with provenance intact. Old
   three-letter display preferences are ignored. Imported ordinary proteins stay
   unchanged. Save/reopen and native Copy/Paste retain ordinary editable rows.
4. All 20 standard amino acids have distinct shades, while their color families
   remain recognizable. Repeat in imported proteins and translated codons, both
   Letters and Backgrounds, including >98% gaps. Check symbol contrast on dark
   blue/red backgrounds. Monochrome deliberately uses no residue colors.
5. Select one/two cells inside a nucleotide codon with Translation active. Copy
   translated fragment must be enabled. Invoke it from both menus: selection
   expands to complete codons and clipboard receives amino acids. Partial ends
   across two codons expand both ends; split codons include all their coding
   cells across noncoding intervals. Underlying symbols and selected names stay
   unchanged. Ordinary Ctrl+C does not expand. No separate expansion action.
6. With Translation off, only noncoding cells selected, unresolved boundary calls,
   masks/proteins selected or multiple rows selected, translated Copy stays
   unavailable. Traces, overview, highlighting and original nucleotide editing
   keep their meanings and responsiveness.

Automated targets: kleiotide_translation_tests, kleiotide_translation_check and
kleiotide_translation_controls_check. The full CTest/core sanitizer suites also
cover sessions, coordinates, trace evidence, groups, types, motifs and editing.
