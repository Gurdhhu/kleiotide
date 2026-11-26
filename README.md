# Kleiotide

## Table of contents

- [Introduction](#introduction)
- [Why use Kleiotide?](#why-use-kleiotide)
- [Installation](#installation)
  - [Windows](#windows)
  - [Debian](#debian)
  - [Other Linux (build from source)](#other-linux-build-from-source)
  - [macOS (build from source)](#build-on-macos-experimental)
- [Getting started](#getting-started)
- [Acknowledgments](#acknowledgments)
- [Source, citation and licensing](#source-citation-and-licensing)

## Introduction

**Kleiotide** is a desktop editor for nucleotide sequences, alignments and Sanger chromatograms, designed primarily for **molecular taxonomy and phylogenetics**. I developed it to make everyday work in our lab easier, from checking base calls against chromatograms and refining alignments to comparing specimens and organizing sequence data for analysis. The editor focuses on nucleotide workflows and also supports translation and basic protein editing and analysis.

## Why use Kleiotide?

- **Check and edit Sanger reads against the original signal.** Open AB1 files directly or link them in batches to an existing alignment. Jump between alignment positions and chromatogram peaks, review quality scores and double peaks, and export edited chromatograms. A `.kleio` session stores the original trace evidence alongside your edits, so you can reopen it without the source AB1 files.
- **Edit alignments directly.** Insert or overwrite bases, move blocks, lock gaps, reverse-complement sequences and undo or redo edits. Use the alignment overview to navigate, and highlight sites or search for sequence motifs.
- **Align or merge sequences.** Use MAFFT to align a dataset, selected sequences or columns, or align against templates while retaining linked traces, sequence groups and row order. Build a consensus from overlapping reads and review conflicts before merging.
- **Compare and organize specimens.** Group sequences manually or by different criteria, calculate pairwise and group distances, inspect alignment statistics and nucleotide composition, cluster similar sequences, collapse duplicates and exchange group assignments through tables or SPART files.
- **Inspect translations in place.** View amino acids alongside nucleotide coordinates, choose genetic codes and reading frames, and export translated sequences.
- **Search reference databases.** Run NCBI nucleotide BLAST over HTTPS, open the same query in the browser, or search local databases with BLAST+. Export analysis tables as XLSX, TSV or CSV.

## Installation

Downloads below target [**v0.30.5-rc1**][release]. Other versions are available on the [releases page](https://github.com/Gurdhhu/kleiotide/releases).

### Windows

Download the [Windows x64 installer ZIP][windows-zip], extract it, and run the setup `.exe`. The installer supports x64 Windows 10 (version 1809 or later) and Windows 11. Windows 11 on ARM can use the x64 installer through emulation. Setup lets you choose the installation folder, register file types and install optional copies of MAFFT and NCBI BLAST+.

### Debian

Download the [Debian 13 (trixie) amd64 package][debian-deb]. In a terminal opened in the download folder, run:

```sh
sudo apt install ./kleiotide_0.30.5-1.deb13_amd64.deb
```

Launch **Kleiotide** from your desktop menu. APT normally installs the recommended MAFFT and NCBI BLAST+ packages. Use `--no-install-recommends` to skip them.

### Other Linux (build from source)

Download the [tar.gz source archive][source-tar] and extract it. Install a C++20 compiler, CMake 3.21 or later, and Qt 6.5 or later with the Widgets, Concurrent and Network development components. From the extracted source directory, run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target kleiotide --parallel 2
./build/kleiotide
```

If CMake cannot find Qt, add `-DCMAKE_PREFIX_PATH=/path/to/Qt` to the configure command. See [Build from source](docs/development/BUILDING.md) for more details.

### Build on macOS (experimental)

Download and extract the [source ZIP][source-zip], then open its folder in Terminal. Install Apple’s Command Line Tools with `xcode-select --install` if needed. With Homebrew installed, run:

```sh
brew install cmake qtbase
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
cmake --build build --target kleiotide --parallel 2
open build/kleiotide.app
```

You can copy `build/kleiotide.app` to Applications and open `.kleio` sessions from Finder. To make Kleiotide the default for these files, use **Get Info → Open with → Kleiotide → Change All**. For optional alignment and local search tools, run `brew install mafft blast`.

## Getting started

Open FASTA, AB1 or `.kleio` files with File → Open or drag them onto a main window. Save a native session to keep trace evidence and metadata. Use FASTA for sequence-only exchange. Save As and exports start in the alignment’s own folder. For a new alignment they use the original input folder or the last folder used to open a file.

Windows setup can install MAFFT and NCBI BLAST+ and create a desktop shortcut. Debian’s standard APT installation includes both recommended tools. **Locate** beside an executable field finds an installed tool, and **Browse** lets you choose your own copy.

**Help → Check for updates…** lists newer public builds. From there you can download and install a compatible Windows x64 installer or Debian 13 amd64 package. Debian in-app installation uses APT and PolicyKit. Other systems can open the release page for source and build instructions.

- [Latest release](https://github.com/Gurdhhu/kleiotide/releases/tag/v0.30.5-rc1)
- [User guides](docs/README.md#user-guides)
- [Build from source](docs/development/BUILDING.md)
- [Windows installer preparation](docs/development/WINDOWS_INSTALLER.md)
- [Testing and acceptance](docs/development/TESTING.md)
- [Changelog](CHANGELOG.md)

## Acknowledgments

Many thanks to Martin Schnittler, Dmytro Leontyev, Maho Inoue, Kristina Kuprina, Manuela Bog and other people for testing Kleiotide, reporting bugs and suggesting improvements.

## Source, citation and licensing

The source code and issue tracker are on [GitHub](https://github.com/Gurdhhu/kleiotide). See [CONTRIBUTING.md](CONTRIBUTING.md) for bug reports and development contributions. When citing Kleiotide, identify the version used. Machine-readable citation metadata is in [CITATION.cff](CITATION.cff).

Copyright © 2026 Oleg Shchepin. The program is licensed under [GPL version 3 only](LICENSE) and provided without warranty. See the [licensing details](LICENSE-STATUS.md). **Elizaveta Shchepina (Storminka / Io Tikhaya)** designed the logo and icon. The name and official artwork have a separate [branding policy](BRANDING.md). Third-party components retain their own licenses. See [dependencies](docs/development/DEPENDENCIES.md).

[release]: https://github.com/Gurdhhu/kleiotide/releases/tag/v0.30.5-rc1
[windows-zip]: https://github.com/Gurdhhu/kleiotide/releases/download/v0.30.5-rc1/Kleiotide_0.30.5_rc1_Windows_x64_Setup.zip
[debian-deb]: https://github.com/Gurdhhu/kleiotide/releases/download/v0.30.5-rc1/kleiotide_0.30.5-1.deb13_amd64.deb
[source-tar]: https://github.com/Gurdhhu/kleiotide/archive/refs/tags/v0.30.5-rc1.tar.gz
[source-zip]: https://github.com/Gurdhhu/kleiotide/archive/refs/tags/v0.30.5-rc1.zip
