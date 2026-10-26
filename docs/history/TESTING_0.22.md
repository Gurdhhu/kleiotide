# Kleiotide 0.22 — Windows acceptance

Extract into a fresh folder, open CMakeLists.txt with your existing Qt >=6.5
Desktop MSVC kit, and build kleiotide. This release keeps native session format 4;
formats 1–3 remain readable. Use a copy of your research data for acceptance.
MAFFT remains external; fragment addition requires 7.526 or later.

1. Sequence → Sort → By decreasing similarity: verify the dialog explains that
   N, ?, other ambiguities, gaps and missing cells enter neither difference nor
   comparable-site counts. N–N is not a match. Preview/Apply/Undo still work.
2. Choose File → FASTA import type → Proteins + masks, then open
   examples/protein_mafft_demo.faa. Confirm protein colors/types, including the
   ACGT-only row. Whole alignment should show protein controls, BLOSUM 62 by
   default, and no nucleotide-only direction/scoring controls. Hover tooltips.
3. Align with Automatic and other presets. Try BLOSUM 30/45/80, JTT PAM and
   transmembrane PAM, and composition adjustment. All residues, case, names and
   groups should survive in a separate result window; the source stays unchanged.
   Unknown/stop calls must survive, with their scoring policy described in dialog
   and report. Save/reopen a result as .kleio to retain metadata.
4. Select a column range and realign it. Check exact prefixes/suffixes and masks.
   A mask inside the region may prevent shortening. Empty protein rows stay
   protein rows and get open-gap padding; mask text is never aligned.
5. Select protein names and Align selected sequences. Only those rows should be
   present in the result, with IDs/groups retained. This also works in a document
   containing unselected nucleotide rows and masks.
6. Select protein target names and use each template-addition command. Only
   separate nonempty protein rows should be offered as templates. Check retained
   original gap-only columns, new target insertions, non-target residues/masks,
   and group membership. Test a mixed document with unselected trace-linked DNA:
   its symbols and evidence must retain their original column relationships.
7. Add a nucleotide participant to a whole/column/selected-only job: reject the
   mixed request without starting MAFFT. A homogeneous subset still works. The
   translation display alone must not change which molecule MAFFT receives.
8. Recheck nucleotide whole/column jobs, selected-only jobs, both template modes,
   strand adjustment and masks. Nucleotide scoring controls should remain present.
9. Cancel during preparation and a long-running job; close the progress window
   after an error. Source data and its edit history must remain intact. Check
   paths containing spaces/non-ASCII characters with the Windows MAFFT launcher.

Local automated results and platform limits are recorded in VALIDATION.md.
