# Kleiotide 0.28.0

Kleiotide is a C++20/Qt Widgets alignment and chromatogram editor. This testing
release adds nucleotide/protein BLAST and trace assignment/mapping to the
accepted native Windows 0.27.2 baseline. It is not RC1.

Open FASTA, AB1 or native .kleio sessions. Save sessions to retain groups,
translation metadata and embedded original trace evidence; FASTA export writes
sequence text without falsely marking unsaved session metadata saved.

## Build and test

Requirements: CMake 3.21+, C++20 compiler, Qt >=6.5 Widgets, Concurrent and Network.
The usual Qt 6.5.3/MSVC2022 x64 Release kit remains suitable. Qt Creator's own
Qt 6.8.1 is separate from the selected application kit. Use a fresh build directory.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/kit -DKLEIOTIDE_BUILD_RELEASE_CHECKS=ON
cmake --build build --target release-check --parallel 2
```

Expected: 41/41 registered tests (20 core, seven Qt unit, fourteen GUI workflows).
Automatic BLAST tests use loopback HTTP and a controlled process, never NCBI.
Read [WINDOWS_TESTING.md](WINDOWS_TESTING.md), [VALIDATION.md](VALIDATION.md),
[RELEASE_0.28.0_TESTING.md](RELEASE_0.28.0_TESTING.md) and
[WINDOWS_INSTALLER.md](WINDOWS_INSTALLER.md) for native tests and packaging.

## New workflows

Sequence → Link trace (AB1) assigns batch evidence to selected/all-untraced
nucleotide rows, previews orientation/peak mapping and applies in one Undo.
Delete trace unlinks associations only. The original calls/signals remain intact.
The merge read preview now fits its row count, with scrolling for many reads.

BLAST → Nucleotide BLAST / Protein BLAST sends captured ungapped queries after
confirmation. Standard searches use built-in HTTPS; advanced options can use a
separately installed NCBI BLAST+ blastn/blastp -remote executable. Batching,
deduplication, cancellation/RID resume, GenBank enrichment, linked sortable
results and TSV/XLSX export are included. Read
[BLAST_AND_TRACE_LINKING.md](BLAST_AND_TRACE_LINKING.md) for supported options,
limits, metric definitions, mapping heuristics and required native acceptance.

The other workflows remain: nucleotide/protein/mask editing and gap locking,
Undo/Redo, groups/table imports, identity clustering/representatives, trace edits
and AB1 export, quality operations, merge/collapse, translation/annotations,
motif search, site highlights/statistics, distances, MAFFT, global Settings and
recovery. Feature guides and historical records remain included. Session format
stays 4. ARM remains the x64 build under Windows 11 emulation; real MAFFT
Automatic/fallback testing is pending on the colleague's machine.

## Source, citation and licensing

https://github.com/Gurdhhu/kleiotide

See [docs/GIT_WORKFLOW.md](docs/GIT_WORKFLOW.md), CONTRIBUTING.md and CITATION.cff.
The source ZIP/package script is matched to this exact version for embedding in
the Windows installer. No Git commit/push or installer binary was produced here.

Copyright © 2026 Oleg Shchepin. GPL version 3 only, without warranty; see LICENSE
and LICENSE-STATUS.md. Logo/icon designer: Elizaveta Shchepina (Storminka /
Io Tikhaya); separate policy in BRANDING.md. The artwork will be supplied later.
Dependencies/distribution are documented in DEPENDENCIES.md and WINDOWS_INSTALLER.md.
