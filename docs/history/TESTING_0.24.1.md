# Kleiotide 0.24.1 — Windows acceptance

Build in a fresh extracted directory using the usual Qt/MSVC desktop kit.
Use copies of traces and sessions for the checks below.

1. Open a normal AB1, substitute several calls, insert calls, delete calls and
   insert alignment gaps. Export an edited AB1. Open it in BioEdit and Chromas;
   verify it opens, all edited calls appear, and peaks/channels remain sensible.
   Try a reverse-complemented row and a portable session with no original AB1.
2. Use File → Batch export edited traces (AB1)…. Select several sequence names,
   including trace and plain rows; confirm Selected sequence names only exports
   only linked rows. Switch to All traces and verify preview counts/order.
   A residue selection must not determine which traces are exported.
3. Choose an output folder. Check current sequence titles become filenames.
   Use duplicate titles, different case, punctuation, Unicode and long names.
   Existing files get numbered suffixes; source traces and old outputs stay intact.
4. Export the batch, open several resulting files in both readers, and compare
   them with the current rows. Different edits of one original trace must export
   separately. The saved/dirty state and Undo history must remain unchanged.
5. Cancel during a batch. Confirm UI responsiveness, a completion report, intact
   completed files, no partial final file, and unprocessed counts. Export into a
   non-writable folder or include an empty/gap-only trace row; inspect the clear
   failure report and verify later valid traces still export.
6. A file created at a reviewed output path after preview must never be replaced.
   Closing/cancelling before export must create no output.

Known reader limits: BioEdit does not support standard inline PBAS in 1–4-call
files and treats unsigned amplitudes above 32767 as signed. See AB1_EXPORT.md.
Unavailable instrument metadata is marked; epoch run-date markers are explicitly
compatibility placeholders, not original acquisition dates.

Automated local results and platform limits are recorded in VALIDATION.md.
