# Version 0.18 — trace-quality editing

Extract into a fresh Kleiotide_0.18 directory and build `kleiotide` with your usual
Qt 6.8.1/MSVC kit. Minimum Qt remains 6.5. Native format stays 3, reading 1/2/3.
The preceding Edit/View, clipboard, menu organization and gap-locking refinements
remain available. This package contains source rather than an installer.

## Known-result synthetic example

Open examples/trace_quality_demo.kleio. It contains synthetic signals and seven
rows: trim_demo, reverse_demo, missing_Q_demo, unlinked, protein, mask, edited_demo.
Original AB1 files are not needed. The data is entirely synthetic.

1. With no names selected, open Edit → Trim low quality ends. The initial Selected
   sequence names scope must not enable Preview. Explicitly choose All sequences
   with traces. Set Q25 and window 3, leaving original-Q inclusion unchecked.
   Click Preview. Expect 3 of 4 trace-linked rows to change, 8 symbols total;
   unlinked, protein and mask rows are ignored. Cancel and confirm no history/edit.
2. Repeat and Apply. Expected complete aligned strings:
   - trim_demo: ~~GTACGT~~ (two calls trimmed at each end; retained calls 3–8).
   - reverse_demo: ~TACG~ (one call trimmed at each end; retained calls 2–5).
   - edited_demo: TCGTAC~~ (the manually substituted first T is protected).
   - missing_Q_demo stays ACGT; other rows stay unchanged.
   One Undo restores the entire batch, and one Redo restores the trim. Undo again
   before testing the N command.
3. Edit → Low-quality calls to N, All sequences with traces, Q20, inclusion off:
   expect 9 replacements across 3 rows. trim_demo becomes NNGTACGTNN,
   reverse_demo becomes NTACGN, edited_demo becomes TNGTACNN. Missing Q is protected.
   The threshold is strict: a Q20 call is not converted at threshold 20.
4. Select edited_demo only. With Q20 and default protection, Preview reports 3
   changes and one protected edited call. Enable original-call Q: the old preview
   must clear and Apply disable. Preview again: now 4 changes, yielding NNGTACNN.
   The checkbox uses original evidence; it does not assign measured Q to new Ns.
5. Original-Q inclusion resets unchecked each time. Edited includes any symbol
   differing from its imported call, including Ns from a previous cleanup. To
   trim such Ns using original evidence, enable the inclusion option explicitly.

## Checks with your trace alignments

- Test selected sequence names versus the explicit All scope; residue selection
  does not decide which rows are processed. Mixed selected proteins/masks/unlinked
  names must be reported and left unchanged. Duplicate titles have row numbers.
- Trim windows count non-gap symbols and use arithmetic mean Phred Q. Mean equal
  to threshold passes. Internal low-quality islands are retained. Only terminal
  residue calls become '~'; existing locked/open gaps, row lengths and all column
  positions remain. Rows too short for a complete window or with no passing window
  stay unchanged; no whole row is silently cleared.
- Unknown Q is not Q0. Inserted calls are always protected. By default substituted
  calls are protected too. Trim intervals expand to include protected calls;
  every passing window must have usable Q at every position. Read No Q and Edited
  protected counts and the retained current-call interval in the preview.
- Test reversed traces, gapped alignments, missing Q, insertions and manual edits.
  Low-Q-to-N preserves lowercase as n, leaves existing N/n unchanged, and never
  modifies gaps. Inspect Current versus Imported/Original Q after Apply.
- Change any setting: the old preview clears and cannot be applied. Cancel before
  or during Preview: source/history remains intact. Calculation runs off the GUI
  thread; cancellation completes before source editing is enabled again.
- Apply affects the entire preview in one undo step. Undo/Redo must restore exact
  current symbols and trace links. Original samples, calls and qualities remain
  immutable. Group memberships and names survive. Save/reopen .kleio and confirm
  edits and embedded evidence; exported FASTA contains sequence text only.
- Check the dialogs at 100% and 150% display scaling. Preview columns have tooltips
  for long text. Recheck normal editing, clipboard, modes, group actions, motif
  navigation, overview updates and scrolling after applying quality edits.

The next roadmap milestone is Highlight and descriptive statistics, including
shared column summaries, the Statistics menu and nucleotide composition.
