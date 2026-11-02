# Collapse identical sequences

Sequence → Collapse identical sequences… opens a preview. Choose All sequences
or the current selected sequence names (a snapshot). No names selected means an
empty selected scope; it never implicitly applies to all. Masks are excluded by
default and can be included explicitly. Mask rows are compared only with masks.

## Identity

Matching removes all gaps (both open ~ and locked -; imported periods are open
gaps) and ignores ASCII letter case. The complete remaining string must match.
Shorter matching substrings remain separate. N, ?, other IUPAC ambiguities and
ambiguous protein symbols are literal symbols, not wildcard matches. U and T are
distinct. Row types are separate: nucleotide, protein and mask. Empty/gap-only
rows match other empty/gap-only rows of their own type. The translation display
has no effect on comparisons: underlying nucleotide and protein rows are used.
These are the same sequence equality rules as Group identical sequences, with
an explicit optional mask scope. This command removes duplicates; grouping keeps
all original rows.

## Representative and names

The default representative is the first row of each set in alignment order.
Choose a set in the upper table, inspect its members in the lower table, select
a member and click Use selected member as representative. The two tables are Identical sequence clusters and Members of the selected cluster. They show titles, groups, trace sources, residue/gap counts and the resulting title. Full titles are available in tooltips; saved titles are complete. The redundant aligned-symbol excerpt is omitted.

Result titles has two choices:

- Combine original titles: the chosen representative's complete title comes
  first, followed by other members in alignment order. The separator defaults to
  `;` and is editable. Repeated original titles appear repeatedly, since each
  occurrence represents an actual row. Unicode and spaces are preserved.
- Representative title + count: append `_xN`, where N is the number of current
  rows in this duplicate set, including the representative. Thirteen identical
  rows become `representative_x13`; twelve rows are removed. Singletons keep
  their titles. Count is scoped to this operation; existing title text/suffixes
  are not parsed as earlier counts.

Resulting titles are not automatically sanitized or made unique, consistent
with other renaming commands. Use Edit → Rename → Sanitize names separately when
required for an external program. Changing title settings updates the preview;
scope changes recalculate automatically after a short delay. Scope changes reset representatives; changing only title formatting retains the chosen representative. Comparison rules start collapsed.

## Data and Undo

The chosen representative retains its stable ID, relative row position, symbols,
letter case, exact gap arrangement/states, group and complete trace/protein
provenance. Remaining rows keep their relative order and exact data. The group
catalog (including now-empty groups), next row/group IDs and translation settings
are retained. No realignment or column compression is performed. Alignment extent
may decrease if only removed rows reached its far-right end; surviving rows are
never padded or shifted to retain a removed row's length.

All other members of each set are removed. Their groups/evidence are not combined
with the representative's. The displayed group and trace identify what remains;
choose a different representative to keep another member's data. A saved result
contains only surviving rows and their evidence. Count-based naming does not
archive discarded titles or their traces separately. Undo during the current
editing session restores all removed rows, names, groups and evidence in one
step; Redo repeats the exact operation. Native sessions do not store Undo history.

Preview is read-only. Scanning and full-title preparation run in cancellable
background work with lazy table models; they do not copy the alignment's symbol
matrix. Hash matches receive exact comparison, so hash collisions cannot merge
different sequences. Collapse checks the source revision, representative membership,
row types, equality and names before constructing one history command. A changed
source requires another preview. Cancel and no-duplicate scopes make no edit.
 FASTA exports the retained rows and new titles
in the existing unwrapped format.
