# Windows acceptance record

This record summarizes reported native testing; it is distinct from automated fixture results. Tests not expressly reported remain unestablished.

## 0.30.0 RC 1 testing

The Windows x64 laptop passed all 51 automatic tests. The tester confirmed
the double peak menus, preview, scope changes, copy and export, and the undo
and session workflows. The report noted peak-height ratios that appeared too
high and proposals drawn from two smaller peaks in a noisy chromatogram.

## 0.29.10 RC 1 testing

The Windows x64 laptop passed all 49 automatic tests. The tester confirmed
that modifier keys keep the insertion caret in place and that marks follow
cells through edits, undo, reversal and session save/reload. Native Debian
license display and resize-handle checks are pending.

## 0.29.9 RC 1 testing

The Windows x64 laptop passed 49/49 automatic tests. The tester confirmed
the Settings palette controls and native and AB1 round trips for deleted-call
and open-sample insertion. The report identified modifier keys cancelling
the insertion caret and highlights staying behind when cells moved.

On Debian 13 GNOME, the tester reported successful package installation, file
associations, HTTPS and local BLAST, and local MAFFT. The 0.29.9 release target
stopped at two automatic test failures. The report also identified missing
license metadata and low-contrast resize handles.

## 0.29.7 RC 2 testing

All 49 automatic tests passed on the Windows x64 testing laptop. The tester
confirmed the merge warning was resolved and highlighting rendered as intended.
The tester requested a return to simultaneous overview and chromatogram scrolling,
and reported jumps during panel resizing. Layout and highlight persistence
checks were partially accepted. Windows fresh-install/upgrade and Debian desktop
checks are pending.

## 0.29.6 RC 1 testing

All 49 automatic tests passed on the Windows x64 laptop. The tester confirmed
that the two bottom panels resize, hide, and reopen independently and that
Restore default layout places the overview below the chromatogram. Manual
highlight borders left the letters clear, but were hard to see on colored
cells. The report also identified striped column highlights, overview drag
lag when the selection extended offscreen, a dead resize cursor above the
trace grip and a shadowed-variable MSVC warning in the merge preview.
Windows fresh-install/upgrade and Debian desktop checks are pending.

## 0.29.5 RC 1 testing

The Windows x64 laptop passed all 49 automatic tests. The tester confirmed
MAFFT dialog resizing, prepending row numbers, trace selection and clearing,
and the low-quality and manual highlight commands. Hiding a bottom panel
changed the other panel's height. The default install folder was rejected
until the tester created it, and per-residue highlight borders obscured
letters. The full Windows upgrade and Debian desktop checks are pending.

## Latest accepted baseline: 0.29.4 test 1

All 48 automatic and six focused manual checks passed on the Windows x64
testing laptop. A colleague also reported successful installation, MAFFT safe
thread selection and the main workflows on a Windows 11 ARM Snapdragon laptop
under x64 emulation.

## Previous baseline: 0.29.3 test 1

Reported on 2 November 2026: all **47 automatic tests passed** on the native Windows laptop. All seven focused manual checks passed: draggable distance names/columns and result-only copies, identical detailed table content across formats, separate composition setting/value cells, numeric append renaming and separators/history, recovery keyboard focus in both editing modes and across windows, BLAST+ links and compact layout, and documentation/link/installation paths.

## Previous baseline: 0.29.2 test 1

Reported on 1 November 2026: all **47 automatic tests passed** on the tester's Windows laptop. Focused manual checks passed for menus, distance UI, table format/suffix choices, BLAST settings/results and links, group exchange, nucleotide/protein statistics, editable sessions during BLAST, status indicators, compact dialogs and DPI, preview column resizing/copying, unchanged trace assignments, trace activation, improved terminal signal mapping/AB1 export and SPART lowercase sanitation.

The report identified follow-up defects addressed in 0.29.3: distance names were still in a fixed-width row header; method descriptions differed across table formats; composition settings were packed into single cells; numeric batch naming replaced titles; recovery could leave keyboard input unfocused. The tester also requested BLAST+ download links and public documentation cleanup. The subsequent 0.29.3 report accepted those fixes.

## Earlier baseline history

| Version | Reported automatic result | Scope |
| --- | --- | --- |
| 0.26.11 | 30/30 | Editing, groups, traces, analyses, real MAFFT and x64 display checks |
| 0.26.12 | 33/33 | Mode/toolbar controls, exports and MAFFT Automatic on x64 |
| 0.27.0 | 37/37 | Settings, recovery and resource behavior |
| 0.27.1 / 0.27.2 | 38/38 | Overview updates, file opening, recovery and session additions |
| 0.28.0 / 0.28.1 | 41/41 | Trace linking and nucleotide HTTPS BLAST; remote protein/external searches remained pending |
| 0.29.0 | 41/41 | Local databases/searches, SPART/tables, merge/quality UI; remote protein removed |
| 0.29.1 test 2 | 41/41 | Suffixes, GenBank metadata/links, automatic previews, sanitation and compact dialogs |
| 0.29.2 test 1 | 47/47 | Full focused report summarized above |
| 0.29.3 test 1 | 47/47 | All seven focused checks passed |

These are user-reported passes, not a claim that every historic package was independently reproduced by the current maintainer environment. Earlier detailed reports remain in Git history.

## Platforms and outstanding acceptance

The principal testing laptop runs Windows 11 x64 with an AMD Ryzen 7 6800H and 32 GB RAM. The established build workflow uses Qt 6.5.3/MSVC2022 x64 Release; record the actual kit for each subsequent build. This hardware is a tested configuration, not a minimum requirement.

On an ARM64 Windows 11 Snapdragon laptop, x64-emulated installation/basic editing was reported for 0.26.11 test 2. MAFFT 7.526 with explicit Threads=1 worked; Automatic/core-detection previously failed. Current ARM MAFFT Automatic/fallback, broader ARM workflows and changed-version acceptance remain outstanding. See [ARM64_COMPATIBILITY.md](ARM64_COMPATIBILITY.md).

Clean-machine HTTPS/TLS and current clean-machine install/uninstall require their own results. The 0.30.1 Windows laptop passed all 51 tests and the focused double-peak checks. Version 0.30.2 needs native installer and upgrade checks.
