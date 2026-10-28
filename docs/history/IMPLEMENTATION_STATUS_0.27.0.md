# Implementation status — 0.27.0 checkpoint B

26 October 2026. This source follows Oleg's accepted native x64 0.26.12 test-1
build. The previous checkpoint's source and acceptance record are preserved.
No Git repository/remote was available here; no commit or push is claimed.

The approved plan is docs/IMPLEMENTATION_PLAN_2026-10-26.md. Its opening status
records the planning date; this file and VALIDATION.md record implementation.

| Work | Status |
| --- | --- |
| A: transactional row index, protected exports, safe MAFFT threads/retry, toolbar recovery | Implemented; native x64 33/33 and focused manual acceptance reported |
| Shared mode controls and Ctrl/Shift swap | Implemented in A; native acceptance reported |
| File → Settings: typed defaults, validation, migration, Apply/Cancel/reset | Implemented in B |
| Appearance/mode/code defaults versus saved/legacy sessions | Implemented; independent new-document defaults, existing sessions preserved |
| Bulk/Undo/overview workspace preferences | Implemented; operational changes deferred for busy windows |
| Default MAFFT launcher/threads and real small-alignment Test | Implemented; per-job choices no longer replace defaults |
| Recovery foundation | Implemented: separate atomic generations, locks, quota, cancellation, startup/manual recovery, copy/discard and status |
| Bounded overview | Implemented: per-column conservation before bounded aggregation, revision checks, no full-length evidence array |
| Trace assignment and forward/reverse peak mapping, Link/Delete trace menus | Pending checkpoint C; no new trace-linking UI is exposed |
| General analysis/trace-preview workspace policy, shared scan scheduling and incremental highlight/translation caches | Further measured work; overview limits do not claim to cap all analyses |
| Conventional FASTA gap-policy/last-folder preferences and remaining import summaries | Follow-up; not exposed as unsupported Settings switches |
| Chunked storage, virtual Flush, compressed Undo, new session format, native ARM/WSL adapter | Deferred as in the approved plan |

## Important boundaries

Session format remains 4, including original trace evidence and deduplication.
The writer now checks cancellation between bounded chunks; deterministic output
matches the baseline bytes. Preferences are machine/user settings rather than
new portable session fields. Code/frame/annotation data in existing sessions
remain authoritative. Default code initialization does not create Undo history.

Recovery streams from a read-stable document while gating mutations in that
window; it does not make a second full Document copy or promise a pause-free
snapshot. Original input/save files are never the checkpoint destination.
Recovery failures are reported and completed generations remain available.
No crash/OOM handler attempts to serialize arbitrary live state.

The bounded overview still visits nucleotide symbols. Its resolution is limited
for long alignments, with tooltips reporting actual column ranges. It avoids
full-column accumulators while preserving per-column IUPAC conservation before
aggregation. Broad storage/scan rewrites are not folded into this checkpoint.

## Native acceptance and next work

The user reports 0.26.12 test 1 passes all 33 automatic tests and focused manual
checks, including MAFFT 7.526 Automatic (safe), resolved --thread 8, seven selected
rows, width 557 and 6.80 s. ARM MAFFT remains pending and may use a later version.
This is not transferred into a native 0.27.0 acceptance claim.

Build 0.27.0 with the existing Qt 6.5.3 / MSVC2022 x64 Release kit in a fresh
build directory. The release suite now has 37 entries. Use
SETTINGS_AND_RECOVERY.md and WINDOWS_TESTING.md, then WINDOWS_INSTALLER.md for
the exact matching source/installer workflow. Checkpoint C follows with the
scientifically validated trace-mapping core before enabling its assignment UI.
