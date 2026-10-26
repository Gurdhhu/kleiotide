# Kleiotide 0.25.2 — acceptance checks

Extract into a fresh Kleiotide_0.25.2 folder and build kleiotide with the usual
Qt Creator / Qt 6.8.1 / MSVC 2022 kit. Dependencies and native session format are
unchanged. These checks supersede the conflict-navigation and overlap-ranking
parts of TESTING_0.25.1.md; its other workflows still apply.

1. Open a merge dialog with conflicting nucleotide rows. Red/green conflict
   highlights remain. Previous/Next conflict buttons are absent.
2. Open examples/merge_terminal_and_priority.fasta. Select known_tail and
   missing_tail, then Merge selected sequences. Expect zero residue/indel
   conflicts and no conflict highlights. The second row's mixed N/?/gap terminal
   region supplies no sequence information, even where it contains locked gaps.
3. Repeat with leading_missing and leading_known. Leading mixed missing regions
   follow the same rule. The rule is case-insensitive for n/N. Internal gaps
   enclosed by informative calls still count as indel conflicts.
4. Select seq1 and seq2. Find overlap and merge, minimum matching positions 20.
   Preview with maximum differences 0, 1 and 2. Each produces exactly:

   ```text
   TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTATTTTTTTTTTTTTTT
   ```

   Expect 55 columns, zero edit events and no conflict highlights. Neither A is
   converted into an assumed insertion. Reverse the two source rows' order and
   repeat; the resulting sequence remains the same.
5. The selection priority is: meet the configured minimum matching support,
   then minimize differences, then maximize informative matches. An exact
   overlap below the configured minimum remains ineligible. A positive allowance
   is a maximum, not a request to introduce differences into an eligible exact
   overlap. Tied best exact repeat placements remain ambiguous.
6. Recheck the substitution/indel examples from TESTING_0.25.1.md. When no eligible
   exact overlap exists, tolerant search still works, but chooses the smallest
   edit count that meets the minimum. Partial-result selection, Save FASTA,
   lowercase, highlighting, original evidence and one-step Undo/Redo remain.

The new regression cases include both supplied examples, both terminal ends,
all-missing rows, genuine internal gaps, IUPAC information boundaries, allowances
0/1/2 in either row order and an independent exhaustive short-sequence ranking
oracle. The Qt workflow checks the repeat example in the actual dialog at all
three allowances and asserts that the navigation buttons are absent.
