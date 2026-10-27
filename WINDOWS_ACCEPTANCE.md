# Windows acceptance and scale observations

Reports supplied by Oleg Shchepin for 0.26.10 and the subsequent **0.26.11**
rerun. These are the user's native results, separate from local Linux regression
runs. The 0.26.11 rerun confirms **30/30 automatic tests and all manual tests
pass**, including prevention of oversized bulk operations. A later colleague report confirms limited test-2 ARM acceptance, recorded below.
Acceptance of the changed 0.26.12 code remains pending.

## Environment

- Windows 11 Home 25H2, OS build 26200.9711; Windows Feature Experience Pack
  1000.26100.372.0.
- Qt Creator 15.0.1, itself based on Qt 6.8.1 / MSVC 2022 x86_64.
- Application build kit: **Qt 6.5.3 / MSVC2022 64 bit**, Release. The Qt Creator
  host Qt version is distinct from the application's selected kit.
- Manual checks at display scaling 100%, 125% and 150%; 3440 × 1440 screen.
- AMD Ryzen 7 6800H with Radeon Graphics, 3.20 GHz.
- 32.0 GB RAM (31.2 GB usable); NVIDIA GeForce RTX 3070 Ti Laptop GPU, 8 GB.
- Real MAFFT version: **7.526**. Storage and external AB1-reader versions remain
  unspecified. This hardware is a tested configuration, not a minimum requirement.

## Automatic results

The latest 0.26.11 report states **100% tests passed, 0 tests failed out of 30**.
The previous 0.26.10 run passed 29/30; its `gui.merge_check` timing failure is
resolved in the rerun. Qt runtime-path corrections also remain confirmed.
The follow-up result is user-reported; no new raw logs accompanied that message.

## Manual results

| Area | User result |
| --- | --- |
| Ordinary editing, sliding, gaps, Undo/Redo, save/reopen | Pass. |
| Mouse selection and cross-window clipboard | Pass. Explorer associations/opening not configured and not tested. |
| Group management, counts, colors, filters and staged deletion | Pass. |
| Group-table imports, wider mappings, parentheses and malformed large tables | Pass. |
| Identity grouping and representatives, including 5,555 rows | Pass. |
| Ordinary AB1s, stale-offset recovery and x-at-call-523 import | Pass. |
| Trace editing, portable sessions and single/batch AB1 export | Pass as reported; external-reader versions unspecified. |
| Translation metadata, Save/reopen, Save-before-close and cancellation | Pass. |
| Output paths | Pass in the 0.26.11 rerun; includes the corrected Windows filename-validation behavior. |
| Analyses and cancellation | Pass in the 0.26.11 rerun, including the revised overlap-event explanation. The earlier report compared 3 search edit events with 2 final residue conflicts; 0.26.11 exposes the distinction. |
| Real Windows MAFFT | Pass, including cancellation. |
| Display at 100%, 125%, 150% | Pass. |
| Bulk operations exceeding the working-memory bound | Prevented in the 0.26.11 follow-up; no silent termination reported in this rerun. |

## Scale observations (not controlled benchmarks)

| Input, as described by the user | Observations |
| --- | --- |
| 5,500 sequences, 750 bp | No visible delays in basic operations on the laptop/high-resolution screen. User reports BioEdit becomes slow at this size/screen configuration. |
| 10,000 sequences, 45 Kbp | Open/scroll/edit comfortable; overview about 5 s; degap/flush about 2 s; unlock all gaps about 5 s. |
| 146,000 sequences, up to 260 Kbp | Open about 4 s; scrolling, motif search and ordinary edits responsive; overview about 5 s. Flush and bulk gap removal silently terminated the old build. |
| Genomic assembly described as 728 Mbp | Open about 15 s; edits about 5 s; overview about 20 s; motif search about 5 s. Exact contig-length distribution not supplied. |

These are approximate user observations, not repeatable CPU/RAM or throughput
claims. Row-length distributions, total stored symbols, file sizes and memory
measurements were not supplied. Maximum row length is not necessarily the
length of every row. A rectangular 146,000 × 260,000 alignment would contain
37,960,000,000 cells (about 35.4 GiB at one byte per cell) before undo or temporary
storage. Flush may create that rectangle from much smaller ragged input.

## Follow-up scope and limits

0.26.11 skips bulk transformations on rows that cannot change, removes redundant
whole-row validation copies for ordinary rows, and rejects bulk plans exceeding
2 GiB of estimated additional working memory before materializing their text or
undo buffers. Flush now catches and reports allocation/limit errors through the
GUI. The bound is an operation safety limit, not a maximum input length or a
promise about available physical RAM. System-wide memory pressure can still
prevent allocations. Users should save sessions before demanding bulk work.

The core API can lower the working-memory bound for constrained callers; the
current GUI uses the 2 GiB default. Refused operations retain sequence content
and undo history. Small edits in long ordinary rows remain allowed. This patch
does not implement disk-backed editing or promise unbounded genomic matrices.

## ARM colleague test — installer revision 1

A colleague's system screenshot supplied on 25 October 2026 shows Snapdragon
X Elite X1E80100, 16.0 GB RAM (15.6 GB usable), and ARM64 Windows 11 Home 25H2,
OS build 26200.9711. Installer test 1 stopped at its native-x64-only guard before
installing Kleiotide. This is a packaging restriction; application execution was
not tested. Device, product and machine identifiers are intentionally omitted.

Revision 2 allows ARM64 Windows 11 when Windows reports x64 user-mode support.

## ARM colleague test — installer revision 2

The supplied feedback confirms installation, Start menu launch and basic
FASTA/AB1/session/editing workflows on the Snapdragon laptop, without Qt or
development tools. The reported setup SHA256 is
E8C205431D050F6CEC51428675CE6EC0598D5403D0D368D17D43EBD1530BEE33.

MAFFT 7.526 failed with “Cannot count the number of physical cores” and Cygwin
FAST_CWD warnings. Another screenshot shows the editing toolbar absent while
the menu/alignment remain visible. The exact hiding trigger is not established.
Uninstall/reinstall, broader analyses and workload limits remain unconfirmed.

0.26.12 adds bounded positive automatic threads, one specific-error retry and
batch diagnostics, plus permanent toolbar/menu recovery commands. Those fixes
have local fixture coverage; real MAFFT and native UI retests on this machine
are pending. No blanket ARM/MAFFT compatibility claim follows from installation.

## 0.26.12 test 1 native x64 acceptance — 26 October 2026

Oleg reports all 33 automatic tests passing and all focused manual checks
passing: shared Edit → Mode controls, Ctrl one-column/Shift maximum sliding
with Undo/Redo, toolbar hide/restore/default layout including narrow windows
and scaling, and merge/table export replacement/cancellation/source protection.
The corrected toolbar test no longer blocks packaging.

Real MAFFT 7.526 via the Windows batch launcher succeeds with Automatic (safe),
resolved --thread 8. The supplied report sends seven selected nucleotide rows,
returns width 557, reports no reversed rows, and completes in 6.80 seconds.
Cancellation is reported passing in the focused check. The report's
--threadit 0 records deterministic refinement; it is distinct from --thread 8.
ARM retesting remains pending and may use a later version as requested.
