# Implementation status — 0.26.12 checkpoint A

Prepared 26 October 2026 from the supplied GitHub-ready 0.26.11 tree. This is
the first reviewable checkpoint of the approved plan, not RC1. The baseline is
an extracted source snapshot; no Git commit or remote push is claimed.

The original plan is retained in docs/IMPLEMENTATION_PLAN_2026-10-26.md.
Its opening status describes the planning date; this file records implementation.

| Work | Status in this source |
| --- | --- |
| Sequence-ID index | Implemented, including transactional structural updates and allocation-failure tests |
| Protected dialog exports | Implemented for merge FASTA and identity/composition/distance tables |
| MAFFT automatic threads, one targeted retry, batch diagnostics | Implemented and tested with a controlled launcher; real Windows/ARM acceptance pending |
| Toolbar restoration | Implemented, offscreen workflow tested; native touch/DPI checks pending |
| Ctrl one-column / Shift maximum sliding | Implemented; alignment focus and existing gap/lock rules preserved |
| Shared Edit → Mode controls | Implemented; toolbar synchronization and session restoration tested |
| File → Settings | Pending checkpoint B |
| Recovery and bounded overview | Pending checkpoint B |
| Trace assignment, mapping, Link/Delete trace menus | Pending checkpoint C; no new trace-linking UI is exposed |
| Incremental scan/cache changes and remaining import/export policy decisions | Pending measured follow-up and checkpoint D |
| Native ARM build, chunked storage, virtual Flush, compressed Undo | Deferred as in the approved plan |

## Review decisions applied

The row-index idea was implemented with allocation preparation before mutation;
the supplied patch was not applied blindly. Const lookup never rebuilds a cache
or allocates. A private transaction index receives structural changes and is
published only after the command completes. Append/load failures also restore
the sequence-ID allocator. Existing Undo/Redo and native session format 4 remain.

The export audit found direct QSaveFile use in several analysis dialogs. These
now use the shared path policy and streaming atomic writer. A final protection
check runs immediately before writing, including recognition of existing ABIF
and session contents even under unexpected extensions.

Local error handling was added to sequence deletion. No global Qt exception
handler catches an exception and attempts to continue after an unknown failure.
The large storage/controller rewrites from the review remain deferred.

## Compatibility and limitations

Session format remains 4; the added right-click preference is optional metadata,
so older sessions default to Context menu. Machine-specific paths are unaffected.
No global preferences dialog, autosave or new trace mapping is included yet.
The existing 2 GiB bulk working-memory policy and 64 MiB nominal history target
are unchanged. The row index adds storage proportional to the number of rows.

Automatic MAFFT uses 1–8 logical threads, not a physical-core measurement.
An explicit user thread count is never changed by retry. A specific physical-core
diagnostic during a failed automatic job can trigger one retry at one thread.
Other MAFFT/Cygwin compatibility failures remain external-runtime issues.

## Validation and next action

See VALIDATION.md for measured results and limits. Build this version in a fresh
Qt 6.5.3 / MSVC2022 x64 Release directory and run release-check (33 entries).
Then retest the changed controls, exports and real MAFFT on native Windows and
the colleague's ARM laptop. WINDOWS_INSTALLER.md describes producing the new
0.26.12 test-1 setup from the matching Source_For_Installer ZIP.

Checkpoint B follows with a typed settings service, explicit new-session default
precedence, recovery and bounded overview. Trace mapping stays separate until
its scientific coordinate tests are ready; a successful checkpoint-A test run
does not validate that future feature.

## Windows test follow-up

The first native 0.26.12 run passed 32/33; the toolbar workflow's fixed-width
visibility assertion blocked packaging. The corrected test covers font-dependent
fitting/narrow layouts without changing production code. See
docs/WINDOWS_TOOLBAR_TEST_FIX.md. Native rerun and real MAFFT remain pending.
