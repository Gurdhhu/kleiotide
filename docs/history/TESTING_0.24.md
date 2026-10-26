# Kleiotide 0.24 — Windows acceptance

Extract to a fresh Kleiotide_0.24 directory and build kleiotide with the existing
Qt >=6.5 Desktop/MSVC kit. Native session format remains 4.

1. Open an AB1, edit several bases, insert calls, delete calls and insert both
   open and locked alignment gaps. File → Export edited trace (AB1)… should show
   the current calls, inserted/substituted counts, omitted gaps and Q policy.
2. Export to a new _edited.ab1 file. Open it in your usual AB1 viewer/editor:
   every current non-gap symbol should appear in the stored sequence, including
   insertions and ambiguity codes. Deleted calls/gaps should be absent. Unchanged
   signal peaks stay at their original samples; inserted calls may overlap labels
   in crowded regions. The signal itself must be unchanged.
3. Reopen the export in Kleiotide and compare Current/Imported with the source's
   current ungapped sequence. PCON Q should be retained for unchanged calls and
   zero for changed/inserted/unscored calls. This export is a new edited baseline;
   keep the source .kleio session for pre-edit calls and evidence.
4. Reverse-complement a trace-linked sequence, export, and open in both programs.
   Calls, peak positions and reversed/complemented signals must agree. Case and
   IUPAC ambiguities must survive. Reversing twice should return the original
   orientation.
5. Open several AB1 files in one window. Select one sequence name and invoke
   export; that trace should be preferred. The dialog's chooser can export the
   other linked rows. Plain sequences, masks and proteins are excluded.
6. Save .kleio, move/remove the original AB1, reopen the session, and export again.
   Embedded signal evidence should suffice. Unicode titles/file paths and both
   .ab1 and .abi output extensions should work.
7. Attempt to export over an existing imported source AB1: the dialog should ask
   for another name and source bytes must remain unchanged. Export to an existing
   separate output uses the usual overwrite confirmation. Cancel the destination
   dialog or export preparation: no source/history/saved-state changes should occur.
8. A sequence without trace evidence should leave the export menu unavailable.
   An empty/gap-only linked sequence or unsupported amplitude/position range
   should show an error and keep Export inactive. No values should be clipped.
9. Check ordinary FASTA/session saves, trace editing, Undo/Redo, scrolling, groups,
   translation and duplicate collapse after exports. An export must not mark the
   editing session saved or change its filename.

AB1_EXPORT.md gives exact data rules, supported ranges and known reader limits.
VALIDATION.md records automated reader comparisons and Linux/platform limits.
