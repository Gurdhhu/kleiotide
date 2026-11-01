# Kleiotide 0.29.2 testing release

Build the matching Source_For_Installer ZIP in a fresh directory using the
Qt 6.5.3 / MSVC2022 x64 Release kit. Enable KLEIOTIDE_BUILD_RELEASE_CHECKS;
expect **47 registered tests**. Set KLEIOTIDE_SOURCE_ARCHIVE to this exact inner
ZIP and KLEIOTIDE_INSTALLER_REVISION=1, then build test-installer. Keep the usual
Qt source archive and VC runtime inputs. No Windows setup EXE is supplied here.

## Focused manual acceptance

1. Sequence-name context menu: nucleotide NCBI BLAST has a separate section above
   group operations. The BLAST menu groups its three searches together, followed
   by the two local database creation commands in their own section.
2. Every distance report starts with pairwise comparison rules collapsed. Method
   help is directly below Distance method and changes even when the chosen
   molecule has no eligible rows. Check all nucleotide and protein methods.
3. Save table offers XLSX, TSV and CSV for distances, both BLAST interfaces,
   identity grouping and nucleotide composition. Omit the filename extension and
   verify the selected format's suffix is added. Open workbooks in Excel or
   LibreOffice and inspect actual numeric cells and literal names beginning =.
4. Distance workbooks: Calculation methods first; one worksheet for the main
   result, comparable sites, observed differences, unavailable estimates, sequence
   labels and groups when used. TSV and CSV retain the prior full block layout.
   Switch the visible matrix between distances/counts: Copy table must copy only
   the displayed matrix, with names, no settings or separate analysis blocks.
5. BLAST workbooks: settings first and results second. Inspect Kleiotide version,
   BLAST version when reported, search start time (UTC), options and actual HTTPS
   request parameters or local command arguments. TSV/CSV put settings above
   results. Remote accessions remain blue, underlined HTTPS links; local names
   have no GenBank hyperlinks or fabricated record metadata.
6. Identity grouping: Save table puts settings/comparison rules first, and result
   rows second. TSV/CSV include settings above results. Saving never changes
   memberships. File > Export sequence groups also supports XLSX and CSV; ordinary
   group tables retain original sequence names.
7. Alignment statistics: choose Nucleotide or Protein; switch repeatedly,
   including an absent type. Protein statistics use 20 standard amino acids with
   B=D/N, Z=E/Q, J=I/L; U/O, X, ?, stops and gaps are unknown states. Unique rows
   and patterns use normalized states; informative sites require two definite
   states occurring twice; constant sites share a possible state. Gap-only and
   unknown-only counts are subsets of constant sites. Results use a snapshot of
   the alignment taken when the window opened; other row types are excluded.
8. Start HTTPS and local BLAST from the actual main menu. Both search windows
   leave their originating alignment interactive. Edit, rename, reorder, add or
   delete rows during a pending search: submitted queries and result aliases must
   continue to use the snapshot captured when that search window opened. Cancel
   retains results and saved HTTP RID; no automatic duplicate submission occurs.
9. Results show a visible status inset: Ready, Search in progress, Fetching
   GenBank metadata (HTTPS), Completed, Failed or Cancelled. Verify successful
   zero-hit completion as well as failures; color is accompanied by status text.
10. At 100/125/150/200% scaling, particularly 2560×1440 at 150%, open both BLAST
    windows and both merge dialogs. Search forms scroll inside a bounded window;
    the footer must stay on screen. Merge overlap limits and Q controls share
    logical rows; the status inset has compact spacing; previews start shorter
    and remain scrollable. Group identical, MAFFT, sequence type, Add to group,
    selectors and font dialogs must have no blank expandable strip below buttons.
11. In translated sequence, trace linking, both merge, group import, every distance,
    quality cleanup and batch rename preview, drag every column border and widen
    names beyond the viewport. Horizontal scrolling must reveal complete values.
    Copy table copies current results as TSV, without vertical sequence indices,
    settings, additional analysis blocks or trace-row Add AB1 action cells.
    Reads-to-contig alignment still has its separate FASTA copy command.
12. Link traces, wait for automatic peak mapping, then open and close an assignment
    selector without choosing anything different. Clicking another cell must not
    clear or recalculate the preview; assignment reason and mapping stay intact.
    After confirming Link traces, the current session shows the chromatogram.
13. Relink an original AB1 to a sequence with manually extended terminal bases.
    Supported concrete terminal calls receive inferred signal positions; ambiguous
    or unsupported signal remains unassigned. Verify forward and reverse cases,
    display alignment, AB1 export in Chromas/BioEdit, edits, Undo/Redo and session
    reopening. Letters and measured signal samples must never change; inferred
    calls have no original call index or measured Phred Q and export with Q0.
14. SPART sanitation replaces each disallowed character with an underscore,
    preserves leading/trailing spaces as underscores, and lowercases names.
    Test Rename, matricial/XML export, and default sanitation-aware import.
    Export-only sanitation and import comparison leave alignment names unchanged.
    Sanitation collisions still require explicit review.

## Native sessions and trace reference check

New sessions use format **5** to retain inferred peak positions separately from
original evidence. Formats 1–4 remain readable; versions through 0.29.1 cannot
open format-5 saves. Keep the earlier session if you need to reopen it in an
older version. Importing an older session preserves its existing mappings;
relink the original AB1 to calculate a new mapping.

The supplied AB1 was already exported by the earlier version. A reference check
reconstructed its pre-export boundary by treating its 14 terminal Q0 calls as
unassigned additions while retaining all measured channel samples. The unchanged
full sequence received 14/14 inferred signal anchors and its AB1 PLOC values
followed those anchors. This is not a comparison against the unavailable original
pre-link AB1. Existing exported PLOC positions are preserved on ordinary import;
relink the original AB1 when recalculating an older edited trace.

Inference is deliberately heuristic: terminal concrete calls, bounded signal
search, monotone order, sequence support and peak spacing; long homopolymers need
multiple signal maxima. It does not invent missing original calls or Phred scores.
Clean-machine Windows/TLS, native cursor/style behavior and ARM MAFFT remain
manual acceptance checks. No NCBI performance or pending-job resolution is claimed.
