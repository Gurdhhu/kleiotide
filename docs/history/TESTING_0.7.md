# Windows check — Kleiotide 0.7

Extract Kleiotide_0.7_Source.zip into a fresh folder and open CMakeLists.txt in
Qt Creator. Keep your Desktop Qt 6.5.3/MSVC2022 64bit kit and Release / Выпуск.
Use a new build directory, build Ctrl+B and run kleiotide. The archive retains
historical timestamps to avoid the earlier Ninja regeneration loop. Dependencies
are unchanged; the separately installed MAFFT launcher remains remembered.
**Fragment addition requires MAFFT 7.526 or later.** An older installation should
produce an upgrade message before alignment starts, leaving the source intact.

## Two requested adjustments

1. The overview should display **Height = % non-gap symbols** and
   **Color = Column conservation**. Its height, colors and navigation should work
   as before; masks remain excluded and N/gaps carry no conservation information.
2. Open any short sequence, such as ACGT. In Insert, select C and type t:
   ACtGT. Type a, then paste GG: ACtaGGGT. Each addition follows the selected
   symbol and leaves its own last symbol selected. The original GT suffix should
   remain together. Undo each addition; the original selection should return.
3. Select the last symbol and type/paste repeatedly: text should append. Try an
   empty sequence and a mask. A wider stretch or multi-row block should continue
   rejecting insertion. Overwrite should still replace the selected stretch,
   then select the following symbol. Copying, deleting, scrolling and ruler
   selection should continue to work as previously validated.

## Full-sequence addition to templates

1. Open examples/mafft_templates_demo.fasta. Select the target_full sequence name.
   Use Alignment → MAFFT → **Align selected sequences to templates…**.
2. In the checklist, keep only template_one and template_two checked. Row numbers
   disambiguate duplicate titles in other files. Continue to the existing MAFFT
   settings dialog, choose your launcher if needed and Align.
3. A result should open in a new window. The source should remain unchanged and
   saved, with its previous undo history. Row order/titles/types should remain.
4. The target's inserted AaGCu calls must remain, with their exact case and U.
   Other rows should receive open gaps in newly introduced columns, while every
   original symbol and gap state stays on its original column relative to all
   other untouched rows. Specifically, the template -~~ interval must survive;
   the untouched ACG occupying that interval must remain aligned with it.
   All mask digits, including its longer tail, must survive. Previously short
   untouched rows will be padded with open gaps in the result.
5. Open Last alignment report in either window. It should show the selected row
   IDs, parameters, restored template-gap columns, new insertion count and passed
   preservation checks. The boundary policy restores old gap intervals before new
   insertions, and places trailing insertions after the complete original axis.
6. Try one template, several targets, duplicate titles and repeated motifs. All
   original columns must remain. A preset other than Automatic should work for
   full addition; PartTree and strand adjustment are disabled for this workflow.

## Fragment addition

Select target_fragment, choose **Align selected fragments to templates…**, and
again choose template_one/template_two. Fragment addition uses MAFFT --addfragments
with Automatic. These are whole rows containing partial sequences; selecting a
column range still uses the separate Realign selected columns command.
Check the result, preservation report, original window and exact target letters.

## Eligibility, cancellation and regressions

- Select a mask together with a nucleotide target: the mask is skipped as a
  target and remains preserved among untouched rows. Empty nucleotide rows are
  also skipped. Masks and empty rows cannot be chosen as templates.
- With no target names selected, or no eligible separate template, the picker
  explains what is needed and Continue is disabled. Clear/check template boxes
  to verify validation updates immediately. Cancel either dialog: no job starts.
- Cancel a running addition. The source should become editable and remain exact;
  no result should appear. A failed launcher/output must also leave it intact and
  show an error. No unchecked/raw output is applied automatically.
- Recheck whole-alignment and selected-column MAFFT, selection/copy/Overwrite/
  deletion/undo, sorting, mask edits, overview navigation and smooth scrolling.
  TESTING_0.6.md remains useful except for its historical Insert-before step.

Official MAFFT addition reference:
https://mafft.cbrc.jp/alignment/software/addsequences.html .
