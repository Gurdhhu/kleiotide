# Kleiotide

**Kleiotide** is a desktop nucleotide sequence and chromatogram editor designed primarily for **molecular taxonomy and phylogenetics**. It brings alignment editing, Sanger trace inspection, sequence comparison and common data preparation tasks into one convenient workspace.

I started Kleiotide to build a practical tool for the daily work of myself and my colleagues: checking sequences against chromatograms, preparing and refining alignments, comparing specimens, and organizing molecular data for taxonomic and phylogenetic studies. Although shaped by these needs, I hope it will also prove useful to a broader community of biologists.

Kleiotide is explicitly **nucleotide sequence-centered**. As of v0.29.3, protein sequence support remains limited and secondary, providing translation and basic protein editing and analysis. Kleiotide is not intended to be an all-in-one bioinformatics platform or a framework for complex analysis pipelines. Its focus is the everyday work that benefits from direct interaction with sequences and their underlying evidence.

The interface follows a straightforward principle: common tasks should be easy to start. Open an alignment directly, inspect a chromatogram, or align two sequences without first creating a project, defining a pipeline, or completing a multistep import procedure. Alignments open in independent main windows, avoiding differently sized document windows nested inside another window.

Kleiotide draws inspiration from several established tools: **BioEdit** for its philosophy of direct, hands-on alignment editing; **AliView** for translation within the nucleotide alignment; **MEGA** for accessible distance calculations; and **VSEARCH** for similarity-based sequence clustering. These influences guide particular workflows rather than imply complete feature or algorithmic equivalence.

A central feature is the persistent connection between an alignment and its chromatograms. **Every trace-linked sequence can retain its trace throughout an editing session, including after alignment with MAFFT.** Residues are mapped to original base calls and signal peaks, allowing navigation between an alignment position and the corresponding chromatogram. You can move conveniently among linked traces, inspect original and current calls, and edit sequences while checking the signal.

AB1 files can be opened directly or linked in batches to sequences already present in an alignment, with assignment and mapping previews for review. Once imported, the working trace data are independent of the original AB1 files: editing does not modify those files, and saved native sessions embed the analyzed signals, original calls, peak positions, available quality scores and sequence-to-trace mappings. Reopening a session therefore does not require locating the original AB1 files. Original trace evidence remains preserved alongside sequence edits; manual changes do not acquire invented quality scores.

Other capabilities include:

- **Direct alignment editing:** insert and overwrite residues, select and move blocks, lock gaps, reverse-complement sequences, and undo or redo changes. An alignment overview, configurable colors and site highlighting help navigate and inspect the data.
- **MAFFT integration:** align complete datasets, selected sequences, or sequences against templates while retaining trace associations.
- **Trace editing and quality tools:** inspect and clean up reads using available original quality information, and export edited chromatograms individually or in batches.
- **Sequence merging:** build an IUPAC consensus from the current alignment or find overlaps between reads. Previews show read placements and conflicts; eligible conflicts can optionally be resolved using original trace quality scores.
- **Translation within the alignment:** view amino-acid translation while retaining nucleotide coordinates, choose genetic codes and reading frames, annotate coding and noncoding regions, and generate translated sequences.
- **Sequence organization:** group identical sequences, perform VSEARCH-inspired similarity clustering, select group representatives, collapse duplicates, and exchange group assignments through tables or SPART files.
- **Comparison and inspection:** calculate MEGA-inspired pairwise and group distances, inspect alignment statistics and nucleotide composition, highlight conserved or variable sites, and search motifs using literal patterns or regular expressions.
- **Database searches and reusable results:** run NCBI nucleotide BLAST searches over HTTPS or search local databases through BLAST+, and export analysis tables as XLSX, TSV or CSV.

Native **`.kleio` sessions** preserve the alignment together with linked trace evidence, sequence groups and translation metadata, so work can resume with its supporting context intact. FASTA remains available for straightforward sequence exchange with other tools.

## Getting started

Open FASTA, AB1 or `.kleio` files with File → Open or drag them onto a main window. Save a native session to keep trace evidence and metadata; use FASTA for sequence-only exchange. The current testing version is **0.29.3**. Earlier sessions load, but new format-5 sessions require Kleiotide 0.29.2 or later.

- [User guides](docs/README.md#user-guides)
- [Build from source](docs/development/BUILDING.md)
- [Windows installer preparation](docs/development/WINDOWS_INSTALLER.md)
- [Testing and acceptance](docs/development/TESTING.md)
- [Changelog](CHANGELOG.md)

## Build and test

Requirements: CMake 3.21+, a C++20 compiler, and Qt 6.5 or later with Widgets, Concurrent and Network. A Qt/MSVC 2022 x64 Release kit is used for Windows builds.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/kit -DKLEIOTIDE_BUILD_RELEASE_CHECKS=ON
cmake --build build --target release-check --parallel 2
```

The full release suite currently registers **47 tests**. Automated remote BLAST checks use loopback HTTP and do not submit sequences to NCBI. See [building](docs/development/BUILDING.md) for core-only builds and platform details.

## Source, citation and licensing

Source and issue tracker: [github.com/Gurdhhu/kleiotide](https://github.com/Gurdhhu/kleiotide). See [CONTRIBUTING.md](CONTRIBUTING.md) for bug reports and development contributions. When citing Kleiotide, identify the version used; machine-readable citation metadata is in [CITATION.cff](CITATION.cff).

Copyright © 2026 Oleg Shchepin. The program is licensed under [GPL version 3 only](LICENSE), without warranty; see [licensing](LICENSE-STATUS.md). Logo/icon designer: **Elizaveta Shchepina (Storminka / Io Tikhaya)**. The name and official artwork have a separate [branding policy](BRANDING.md). Third-party components retain their own licenses; see [dependencies](docs/development/DEPENDENCIES.md).
