# Dependencies and distribution

| Component | Use | Distribution |
| --- | --- | --- |
| Qt 6.5+ Widgets, Concurrent, Network | GUI, workers and HTTPS | Required by the application; deploy the matching kit and its notices/source |
| C++20 standard library and compiler runtime | Application runtime | Match the chosen compiler; Windows installers include the original signed MSVC x64 runtime installer |
| miniz 3.0.2 tinfl | Reading DEFLATE XLSX ZIP entries | MIT; source and license in third_party/miniz |
| MAFFT | Optional external alignment | Installed and selected separately; not bundled |
| NCBI BLAST+ | Optional local database creation/search | Installed separately; not bundled |
| CMake, compiler, Ninja/Make | Development | Build tools, not application runtime dependencies |
| NSIS | Windows installer compilation | Packaging tool, not an application runtime dependency |

The XLSX/ZIP writer is project code. Reading XLSX uses the included miniz component and needs no extra runtime DLL. NCBI nucleotide BLAST uses built-in HTTPS and never requires BLAST+; Windows deployment includes Qt Network and the Schannel TLS plugin. Other TLS backends depend on the exact platform/Qt configuration.

The project is GPL-3.0-only; see [licensing](../../LICENSE-STATUS.md) and the separate [branding policy](../../BRANDING.md). Third-party licenses are not replaced by the project license. Provide the exact corresponding Kleiotide source, matching Qt source and applicable notices with binary distributions. The Windows installer workflow verifies source matching and stages notices; see [WINDOWS_INSTALLER.md](WINDOWS_INSTALLER.md). External MAFFT/BLAST+ remain the user's separate installations.

Qt deployment reference: https://doc.qt.io/qt-6.5/windows-deployment.html
