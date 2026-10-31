# Kleiotide 0.29.1 testing release

Build fix 1 supersedes the original source ZIP; see BUILD_FIX_1.md.

Build the matching Source_For_Installer ZIP in a fresh directory with the same
Qt 6.5.3/MSVC2022 x64 Release kit. Enable KLEIOTIDE_BUILD_RELEASE_CHECKS:
expect 45 registered tests. Build test-installer revision 2 with this exact
inner source ZIP. No Windows installer EXE is supplied here.

This follows the user's accepted 0.29.0 manual tests. It retains nucleotide
HTTPS and all five local BLAST programs. No protein NCBI search is restored.

## Focused acceptance

1. NCBI query scope says All nucleotide sequences with the matching tooltip.
   Settings uses the same DUST label/help in its HTTPS and local sections.
2. Save local/remote BLAST TSV/XLSX, jobs/reports, FASTA, sessions, AB1 and groups
   without typing suffixes; correct suffixes are appended before replacement or
   source protection checks. Recognized alternative suffixes remain unchanged.
   Resume job starts in the normal blast-jobs directory.
3. The sequence-name context menu offers nucleotide NCBI BLAST; local protein
   BLAST is accessible through the BLAST menu. Local database popups show their
   full names up to screen width, with scrolling for longer text.
4. Remote XLSX accessions are clickable, blue and underlined. Sequence release
   date uses GenBank GBSeq_create-date; Authors uses direct-submission authors,
   not journal article authors. Missing values stay blank. Old metadata caches
   are refreshed so old article fields do not reappear on Resume.
5. Scroll a merge preview across column-number boundaries: the leftmost number
   has an opaque padded background. Unresolved conflicts are red, Q-resolved
   conflicts bright yellow; only the merged result has a colored name background.
6. Rename, trace linking, merging, trace-quality editing and collapse previews
   update automatically after a short delay. Their Preview buttons are removed.
   Explicit Rename / Link traces / Merge / Trim ends / Convert calls to N /
   Collapse buttons commit the reviewed changes; no typing changes the alignment.
   Change options quickly, wait for the preview, then confirm and test history.
7. Batch add AB1 files suggests matches and maps assigned rows automatically.
   Manual assignment also maps automatically. Unassigned rows say Not attempted.
   Add another batch and check inference for every currently unassigned row.
   Readding a file must not strand the dialog without a usable preview.
8. Quality rules start collapsed and fields have short explanatory tooltips.
   Collapse has a short separator field, prominent headers and no Aligned symbols
   member column. Choose a representative, then change title formatting: that
   choice must be retained. Group-identical rules start collapsed; its compact
   prompt shows the group count and replacement warning. Add-to-group lists
   have color markers.
9. SPART export sanitizes names only in the exported file. Rename has a separate
   Sanitize names for SPART action. Leading/trailing whitespace becomes underscores.
   SPART import defaults Assume SPART name sanitation on; toggle it and inspect
   matching. Alignment names remain unchanged. Colliding sanitized names require
   review: export rejects ambiguous duplicate names, while import applies the
   ordinary duplicate-title validation rather than silently choosing one row.
10. Group export sits directly below batch AB1 export. Set sequence type,
    duplicate/motif selectors, MAFFT settings, analysis scope and Alignment font
    use compact content heights. Cancel is on the right of action buttons,
    including search dialogs. Check 100/125/150/200% scaling and narrow windows.
11. Hover the main name/residue divider after selections, drags and focus changes.
    A stale fragment-drag anchor no longer suppresses the resize cursor.

## Pending NCBI job

The supplied eight-query job (7,528 residues), RID E8T40K3T014, repeatedly received
WAITING replies. It also encountered one two-minute transport timeout; Resume
reused the RID. A read-only check of that same RID still returned WAITING.
No query was resubmitted. This does not establish a client defect or a predictable
server completion time. Elapsed display now includes earlier monitoring of the
RID rather than restarting its apparent age on Resume. Native network latency
and NCBI server performance remain external to Kleiotide's control.

Native Windows acceptance, clean-machine TLS and ARM MAFFT remain manual gates.
