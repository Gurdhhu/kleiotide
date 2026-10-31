# Kleiotide 0.29.1

**Build fix 1:** corrected automatic-preview control declarations for MSVC.
See [BUILD_FIX_1.md](BUILD_FIX_1.md); use the corrected source ZIP and installer revision 2.

Kleiotide is a C++20/Qt Widgets alignment and chromatogram editor. This testing
release adds local BLAST databases/searches, SPART group exchange, CSV/XLSX table
loading and merge/trace/dialog improvements. NCBI nucleotide searches use HTTPS;
BLAST+ remote searches and NCBI protein searches have been removed. It is not RC1.
See RELEASE_0.29.1_TESTING.md, LOCAL_BLAST.md and SPART_AND_TABLES.md.

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

Expected: 45/45 registered tests (20 core, eight Qt unit, seventeen GUI workflows).
Automatic remote BLAST tests use loopback HTTP, never NCBI. Local UI checks do
not require BLAST+; optional actual-tool validation is described in VALIDATION.md.
Read [WINDOWS_TESTING.md](WINDOWS_TESTING.md), [VALIDATION.md](VALIDATION.md),
[RELEASE_0.29.1_TESTING.md](RELEASE_0.29.1_TESTING.md) and
[WINDOWS_INSTALLER.md](WINDOWS_INSTALLER.md) for native tests and packaging.

## 0.29.1 follow-up

Automatic previews with explicit named confirmation buttons, unified output
extensions, SPART-specific sanitation and matching, direct-submission BLAST
metadata, visible workbook links, compact dialogs and Cancel on the right.
See RELEASE_0.29.1_TESTING.md; it supersedes older UI instructions.

## New workflows

Sequence → Link trace (AB1) assigns batch evidence to selected/all-untraced
nucleotide rows, previews orientation/peak mapping and applies the reviewed links.
Delete trace unlinks associations only. The original calls/signals remain intact.
The merge read preview now fits its row count, with scrolling for many reads.

BLAST → NCBI nucleotide BLAST sends ungapped queries after confirmation. It uses
HTTPS with saved RID resume, GenBank enrichment, numbered/sortable results and
TSV/XLSX exports with accession hyperlinks. Local BLAST supports all five search
programs and databases created from FASTA/.kleio; see LOCAL_BLAST.md. SPART group
exchange and common TSV/CSV/XLSX import are described in SPART_AND_TABLES.md.

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
