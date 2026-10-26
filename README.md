# Kleiotide 0.26.11

Kleiotide is a C++20/Qt Widgets alignment and chromatogram editor. This is a stabilization build preparing for a release candidate. It is not yet a publicly released RC.

Open FASTA, AB1 chromatograms, or native `.kleio` sessions. Save uses a native session when groups, traces, proteins, explicit row types, or translation metadata need preserving. Export FASTA writes sequence text only and does not mark unsaved session changes as saved.

## Build

Requirements: CMake 3.21+, C++20 compiler, Qt 6.5+ Widgets and Concurrent. The primary Windows acceptance kit is Qt Creator 15.0.1 / Qt 6.8.1 / MSVC 2022 x64.

In Qt Creator, open `CMakeLists.txt`, select the MSVC x64 Qt kit, configure, and build `kleiotide`.

From an initialized compiler environment:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/kit
cmake --build build --parallel 2
```

To build and run the core, Qt unit, and selected GUI release checks:

```sh
cmake -S . -B build -DKLEIOTIDE_BUILD_RELEASE_CHECKS=ON
cmake --build build --target release-check
```

GUI release checks run offscreen; native Windows interaction remains part of acceptance. Start with [WINDOWS_TESTING.md](WINDOWS_TESTING.md) for step-by-step Qt Creator instructions. See [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) for remaining gates, and [VALIDATION.md](VALIDATION.md) for actual results of this build. The `release-check` target also saves `release-tests-Release.log` and `release-tests-Release.xml` in the build directory.

Optional development builds:

- `KLEIOTIDE_BUILD_GUI=OFF`: core library and tests without Qt.
- `BUILD_TESTING=OFF`: omit unit tests.
- `KLEIOTIDE_BUILD_DEVELOPER_TOOLS=ON`: benchmarks and the ABIF probe.
- `KLEIOTIDE_BUILD_GUI_SMOKE_CHECK=ON`: historical GUI workflow tools.
- `KLEIOTIDE_BUILD_VIEWPORT_CHECK=ON`: viewport/drag checks.

GUI implementation sources are compiled once into a shared static build target, then linked into the application and checks. Qt itself remains dynamically linked.

## Features and formats

The application supports nucleotide, protein and mask rows; editing and gap locking; undo/redo; group management and table imports; identity grouping; representative selection; trace editing/export and quality operations; merge/overlap workflows; translation and coding annotations; motif search; site highlights and statistics; composition and distance analyses; and external MAFFT integration.

Feature details remain in `IDENTITY_GROUPS.md`, `MERGING.md`, `COLLAPSE.md`, `SESSION_FORMAT.md`, `PROTEIN_TRANSLATION_PLAN.md`, `SITE_ANALYSIS.md`, `DISTANCES.md`, `DISPLAY_COLORS.md`, `MAFFT_PROTEINS.md`, and the AB1 documents. The source archive retains older development notes and testing guides under `docs/history/`.

See [CHANGELOG.md](CHANGELOG.md) for the complete recorded version history and [ICON_GUIDELINES.md](ICON_GUIDELINES.md) for artwork formats and export sizes.

## Source repository

For an initial import into an empty private GitHub repository and a minimal command-line Git workflow, see [docs/GIT_WORKFLOW.md](docs/GIT_WORKFLOW.md). Contributor guidance is in [CONTRIBUTING.md](CONTRIBUTING.md), and [CITATION.cff](CITATION.cff) provides provisional software citation metadata. The repository intentionally tracks source and release-building scripts, not compiled binaries or generated build directories.

Copyright (C) 2026 Oleg Shchepin. Kleiotide is free software under the **GNU GPL version 3 only** (`GPL-3.0-only`), without warranty; see [LICENSE](LICENSE) and [LICENSE-STATUS.md](LICENSE-STATUS.md). The official logo/icon has a separate policy in [BRANDING.md](BRANDING.md). Its designer is **Elizaveta Shchepina (Storminka / Io Tikhaya)**; the artwork will be supplied later. Dependencies and distribution preparation are described in [DEPENDENCIES.md](DEPENDENCIES.md).

Native Windows acceptance and the user's scale observations are recorded in [WINDOWS_ACCEPTANCE.md](WINDOWS_ACCEPTANCE.md). Bulk edits have a 2 GiB estimated additional working-memory limit; operations above it report an error before making sequence changes. This is separate from stored alignment size and the undo-history budget.

Windows test installer preparation is described in [WINDOWS_INSTALLER.md](WINDOWS_INSTALLER.md).
The application baseline remains 0.26.11 (30/30 automatic and all manual Windows
checks reported passing); installer revision 2 permits experimental Windows 11 ARM64 x64-emulation tests;
clean-machine and ARM64 acceptance remain pending. See ARM64_COMPATIBILITY.md.
