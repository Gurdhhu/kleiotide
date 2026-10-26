# Windows acceptance — Kleiotide 0.19

For the current editing controls and highlight row scope, see TESTING_0.19.1.md.

Extract into a fresh `Kleiotide_0.19` directory. Open CMakeLists.txt with the usual
Qt Creator / Qt 6.8.1 MSVC 2022 64-bit kit, configure and build. The shipped minimum
remains Qt 6.5. No installer is included in this development source release.

## Highlighting

Open `examples/site_highlight_demo.fasta`. It has four nucleotide rows and a mask;
the mask is excluded from all calculations. Highlight defaults to all nucleotide
rows and all alignment columns, independently of selected names.

Test each rule separately, using Clear highlighting between tests:

| Highlight rule | Expected columns (1-based) |
| --- | --- |
| Conserved sites | 1, 4, 8, 10 |
| Variable sites | 2, 3, 7, 9 |
| Parsimony-informative sites | 2 |
| Singleton sites | 3, 7, 9 |
| % Conservation, minimum = maximum = 100 | 1, 8, 10 |
| % Conservation, minimum = maximum = 75 | 3, 4 |
| % Coverage, non-gap, 0 through 50 | 5, 8, 9 |
| % Coverage, informative, minimum = maximum = 0 | 5, 6 |

- Enable several rules: their **union** is highlighted. Reopening a percentage
  command lets you adjust its inclusive range or disable it. Cancel retains the
  previous settings. Clear turns every rule off, preserving scope/color.
- Change Highlight color. Check all residue palettes, colored letters, monochrome,
  colored backgrounds and identity dots. Shading and edge lines remain visible,
  with a marker in the ruler. Blue/yellow residue selection takes priority.
- Select the names row1 and row4 and columns 2–4. In Analysis scope, explicitly
  choose selected names and selected columns, then Apply. Variable sites marks
  columns 2 and 3. Clicking another name or residue does not change that snapshot.
  Choose Analysis scope again to capture a different selection. Undo/redo of edits
  refreshes highlights; calculations never add history entries.
- Protein and nucleotide scopes are separate. Protein N is an ordinary amino
  acid; protein X and ? are unknown. Masks never contribute or receive site
  shading. Columns are shaded in every row of the chosen molecule type, including
  rows outside a selected-name analysis subset.
- Save as a `.kleio` session and reopen. Scope snapshots, rule choices/ranges and
  custom color return. FASTA contains sequence text only. Binary session format
  stays 3; older format-3 readers ignore the optional highlighting view keys.

## Statistics

Statistics → Alignment statistics replaces the old Alignment menu entry. For the
example, expect 4 total and 4 unique sequences, 10 columns, 9 distinct patterns,
1 parsimony-informative, 3 singleton, 6 constant, 1 gap-only and 1 N-only site.
Copy still produces one paste-ready string. Existing definitions are retained:
constant counts include compatibility with ambiguous calls and all-missing sites.
See SITE_ANALYSIS.md for the deliberate distinction from strict conservation.

Statistics → Nucleotide composition:

1. Choose all nucleotide rows or the selected-name snapshot; independently choose
   all nucleotide alignment columns or selected alignment columns. Calculate.
2. Inspect per-row values and the final Pooled row. N/?, partial ambiguities,
   open/locked gaps and missing trailing cells have separate counts. U counts as T.
3. Toggle fractional IUPAC handling. The old result clears; Calculate refreshes
   it. A/C/G/T and GC percentages use scored calls, excluding unknowns/gaps/trailing
   cells. With fractional handling disabled, partial ambiguities are also excluded.
   A zero denominator gives N/A, not 0%.
4. Copy and Export TSV include the full table and the analysis settings. Paste
   into a text editor or spreadsheet. Horizontal scrolling reveals all columns;
   headers can be resized. Duplicate titles remain separate rows.
5. Close during a calculation: it cancels safely, then restores editing. No
   analysis changes symbols, trace evidence, group assignments or undo history.

## Regression and scope

Check recent trace-quality preview/Apply/Undo, independent name/fragment selection,
Edit/View controls, fragment-only shortcuts, whole-sequence context actions and
gap locking. These workflows are retained.

Linux verification covers all 12 CTest suites, independent random site fixtures,
offscreen GUI scope/range/color/native/copy/export/cancel checks, all display styles
and selection precedence, sanitizer checks and a 150% scale run. These do not
replace Windows acceptance on the user's actual kit.

Next roadmap milestone: protein coloring and translation. Distances, group means,
similarity sorting, protein MAFFT and the installer remain subsequent milestones.
