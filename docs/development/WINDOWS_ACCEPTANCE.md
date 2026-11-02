# Windows acceptance record

This record summarizes reported native testing; it is distinct from automated fixture results. Tests not expressly reported remain unestablished.

## Latest accepted baseline: 0.29.3 test 1

Reported on 2 November 2026: all **47 automatic tests passed** on the native Windows laptop. All seven focused manual checks passed: draggable distance names/columns and result-only copies, identical detailed table content across formats, separate composition setting/value cells, numeric append renaming and separators/history, recovery keyboard focus in both editing modes and across windows, BLAST+ links and compact layout, and documentation/link/installation paths.

0.29.4 adds shared remembered file folders, boundary-extension selection commands, grouped motif commands, navigation to selected rows and MAFFT download links. Its full suite registers 48 tests. Native 0.29.4 build and focused acceptance remain pending; the previous pass is not proof of changed code. See [focused checks](TESTING.md#0294-focused-acceptance).

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
| 0.29.3 test 1 | 47/47 | All seven focused checks passed; latest accepted baseline |

These are user-reported passes, not a claim that every historic package was independently reproduced by the current maintainer environment. Earlier detailed reports remain in Git history.

## Platforms and outstanding acceptance

The principal testing laptop runs Windows 11 x64 with an AMD Ryzen 7 6800H and 32 GB RAM. The established build workflow uses Qt 6.5.3/MSVC2022 x64 Release; record the actual kit for each subsequent build. This hardware is a tested configuration, not a minimum requirement.

On an ARM64 Windows 11 Snapdragon laptop, x64-emulated installation/basic editing was reported for 0.26.11 test 2. MAFFT 7.526 with explicit Threads=1 worked; Automatic/core-detection previously failed. Current ARM MAFFT Automatic/fallback, broader ARM workflows and changed-version acceptance remain outstanding. See [ARM64_COMPATIBILITY.md](ARM64_COMPATIBILITY.md).

Clean-machine HTTPS/TLS and current clean-machine install/uninstall require their own results. Native 0.29.4 build, compiler-warning review and focused checks are pending at source-package preparation. No Windows installer binary is produced by Linux validation.
