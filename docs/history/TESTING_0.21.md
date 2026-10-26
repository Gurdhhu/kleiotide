# Kleiotide 0.21 — Windows acceptance

Extract into a fresh folder and build kleiotide using your Qt >=6.5 Desktop/MSVC
kit. DISTANCES.md defines methods, data handling and output units. Existing editing,
translation, annotation-mask and trace functionality is retained.

1. Open a nucleotide alignment. Statistics → Pairwise distance matrix… defaults
   to p-distance, all nucleotide rows/columns and pairwise deletion. Calculate.
   Compare an identical pair (0), one mismatch in four comparable sites (0.25),
   and a no-overlap pair (N/A). Hover for comparable counts and N/A reasons.
2. Switch between distance, comparable-count and raw-difference views. Copy TSV
   into a spreadsheet and export TSV to a file. Both include settings, label/ID
   keys, group keys, counts and explicit unavailable-pair reasons. Duplicate names
   remain distinguishable by their sequence index.
3. Change a setting: previous results clear immediately. Choose selected sequence
   names and selected columns independently; missing selections show a message.
   Increase minimum sites beyond a pair's overlap: it becomes N/A, never zero.
4. Compare pairwise with complete deletion on gaps, N/ambiguity and unequal row
   lengths. Complete deletion uses every included row across the whole scope.
   Masks and other molecule types do not vote or extend the default column range.
5. Check Jukes–Cantor (N/A at p >= .75), Kimura 2-parameter, and protein Poisson.
   Protein mode only lists observed differences, p-distance and Poisson. Protein
   comparisons use actual Protein rows, not the nucleotide translation overlay.
   All ambiguous protein calls, U/O, stops and gaps are excluded.
6. Define at least two groups. Check overall, within- and between-group means.
   Overall/within use unique unordered pairs; between uses cross-group pairs.
   Inspect valid/excluded pairs and mean/min/max comparable counts. A singleton
   group has N/A within-group mean. Toggle explicit ungrouped inclusion.
7. Sequence → Sort → By decreasing similarity… opens a preview. For sequences
   AAAA, AAAT, AATA, AATT the chain should be AAAA, AAAT, AATT, AATA. Preview/Cancel
   leaves the alignment unchanged. Apply sorts in one Undo step; Redo restores it.
   Masks and the other molecule type retain their exact slots; groups/traces stay
   linked by sequence ID. Selected-name scope reorders only those eligible slots.
8. Test minimum-overlap ties, unknown-only rows, mixed molecules and disconnected
   overlap sets. The first eligible row stays first. Ties prefer longer overlap,
   then original order; rows without any valid comparison are last.
9. Start a large calculation and Cancel; also close the dialog while it is running.
   The UI remains responsive, no partial result appears, and the document is unchanged.
   Cancel during export must preserve any existing destination. Very large matrices
   should be rejected before allocation with guidance to use a smaller scope/means.
10. Check dialogs at normal and 150% scaling, including Copy/Export and previews.
    Recheck normal editing, selection, trace navigation, translation and session save.

Sessions remain format 4. Analyses/settings are transient; an applied row order
is saved normally. This source release does not include a validated Windows installer.
