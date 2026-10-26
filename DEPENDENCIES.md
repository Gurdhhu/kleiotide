# Dependencies and distribution status

- Qt Widgets and Concurrent: required to build/run the GUI; obtain the matching Qt kit and retain its applicable notices. The source archive does not bundle Qt binaries.
- C++ standard library/compiler runtime: use the runtime matching the chosen Windows MSVC build. Check the output of the exact kit's deployment tool.
- MAFFT: optional external program selected by the user. Neither its binaries nor its license are bundled here.
- CMake/Ninja/compiler: development tools, not application runtime components.

The application is licensed under GPL version 3 only; the official logo/icon has a separate policy in BRANDING.md. These terms do not replace third-party licenses. No new third-party dependency was introduced by 0.26.7–0.26.9. `tools/windows/package.ps1` prepares an internal test package and copies a Qt `LICENSES` directory when available; that does not complete a public distribution notice or corresponding-source review. See LICENSE-STATUS.md and RELEASE_CHECKLIST.md.

Authoritative deployment reference: https://doc.qt.io/qt-6.5/windows-deployment.html

The installer-preparation archive adds NSIS as a packaging tool only; see
WINDOWS_INSTALLER.md. It stages the exact kit with windeployqt, embeds application
source and Qt notices, keeps full matching Qt source alongside setup, checks the
Microsoft runtime installer signature and records version/hash metadata. Native
installer acceptance is pending; no packaged binary has been tested here.

Installer test revision 2 allows x64 emulation on ARM64 Windows 11 only after
checking x64 user-mode support. It still ships x64 application/Qt binaries and
the original Microsoft x64 Redistributable. Microsoft's current x64 package
also contains ARM64 components. Actual ARM64 runtime acceptance remains pending.
Reference: https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist
