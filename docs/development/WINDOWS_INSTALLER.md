# Windows installer preparation

For Kleiotide **0.29.4**, use a fresh Qt/MSVC x64 Release build and the exact matching Kleiotide_0.29.4_Source_For_Installer.zip. Set KLEIOTIDE_INSTALLER_REVISION=1 for the first installer attempt of this application version. The test-installer target runs the full 48-test release suite before staging deployment. Native acceptance is tracked separately in [WINDOWS_ACCEPTANCE.md](WINDOWS_ACCEPTANCE.md).

The selected kit must provide Qt Widgets, Concurrent, Network and windeployqt. Windows deployment verifies Qt6Network.dll and tls/qschannelbackend.dll, using Schannel rather than assuming OpenSSL is installed on a recipient's machine. Build scripts do not download packaging inputs automatically.

## What the installer does

- Installs an x64 MSVC Release build for Windows 10 (1809+) / Windows 11 under
  `C:\Program Files\Kleiotide Test 0.29.4-1`, with a Start menu shortcut and
  Windows Settings > Apps uninstall entry. Administrator approval is required.
- Deploys Qt DLLs/plugins using the selected application kit's `windeployqt`.
  Colleagues need neither Qt Creator nor a Qt development installation.
  The optional Mesa software-OpenGL DLL and copied system D3D compiler are
  excluded; this build uses Qt Widgets on Windows 10/11.
- Includes the original Microsoft x64 C++ Redistributable installer, runs it
  without an automatic restart, and checks its result. A restart may be requested.
- Keeps different test revisions in separate folders. Reinstalling the same
  revision requires uninstalling it first; increase the revision for a new package.
- Uninstall removes an explicit list of installed files and empty directories.
  It preserves additional files, user settings and the shared Microsoft runtime.
- Advertises supported files in Open With and refreshes stale application commands.
  Existing default apps and PATH are retained. MAFFT remains an optional external
  program configured by the user; See [MAFFT](../user/MAFFT.md) for configuration.
- Embeds matching Kleiotide source, GPL/branding documents and third-party
  notices. The full matching Qt source archive is beside the setup file.

On native x64 Windows, the existing installation path is retained. On ARM64,
Windows 11 (build 22000+) and reported x64 user-mode support are required. Windows
10 on ARM and 32-bit Windows remain excluded. Microsoft documents that its
current x64 Redistributable also includes ARM64 components; keep using the
original latest `vc_redist.x64.exe`, not a renamed ARM64-only package.

**Broader ARM64 compatibility remains experimental. Retest this changed version.**
See ARM64_COMPATIBILITY.md for the focused procedure and the test-1 finding.

The official icon can be added when available. This test installer is unsigned and uses
the default installer artwork. Signing is a later distribution improvement.
Windows may warn about an unfamiliar unsigned installer; colleagues should
verify its origin/checksum and follow their organisation's software policy.

## 1. Download the one-time packaging inputs

1. Install **NSIS 3.09 or newer**, from https://nsis.sourceforge.io/Download . The standard
   installation includes `makensis.exe` and its `COPYING` license. NSIS is the
   installer compiler, separate from Qt and Visual Studio.
2. Download Microsoft's **x64** Visual C++ Redistributable from
   https://aka.ms/vc14/vc_redist.x64.exe . Save it, for example, as
   `C:\Kleiotide-packaging\vc_redist.x64.exe`. Keep its original filename.
   Use Microsoft's original signed file; the packaging script checks its
   Authenticode signature. Choose a runtime at least as recent as your compiler.
3. For your stock Qt **6.5.3** kit, download
   https://download.qt.io/archive/qt/6.5/6.5.3/single/qt-everywhere-src-6.5.3.zip .
   Save it in `C:\Kleiotide-packaging`. This is approximately **1.2 GB**, downloaded
   once and reused; **do not extract it**. It supplies corresponding Qt source
   and the original notices. If changing Qt versions, use that exact version's
   source archive. Custom/patched Qt builds require their actual matching source
   and a separate packaging review; this workflow assumes the stock Qt kit.
4. Keep the complete **Kleiotide installer-preparation source ZIP** you downloaded.
   The script checks its contents against the source being built and embeds it.

Sources must accompany distribution, including colleague testing. Keep the
whole generated release folder together when transferring it offline, or make
the source archives available beside the installer in the same shared/download
folder. Recipients can download just the installer to run the program; source
must remain available to them. Providing Kleiotide's source alone does not
provide the source for the bundled Qt libraries.

## 2. Open and configure this source in Qt Creator

1. If Windows shows **Properties > Unblock** on the downloaded Kleiotide ZIP,
   verify its origin and unblock that ZIP before extraction. Extract into a fresh
   folder. This allows the local packaging script to run; no machine-wide
   execution-policy change is needed. Organisation policy still applies.
2. Open its `CMakeLists.txt`, select the same **Qt 6.5.3 / MSVC2022 64 bit** kit
   and **Release**. Use a fresh build directory. Qt Creator's own Qt version is unrelated to the DLLs needed by this build.
   A short build path such as `C:\Kleiotide-build\0.29.4` avoids Windows path
   length problems while staging the original third-party notice paths.
3. Keep `BUILD_TESTING=ON` and `KLEIOTIDE_BUILD_RELEASE_CHECKS=ON`.
4. Run CMake. In Projects > Build Settings > CMake, set the following variables
   (use **Add > File path** if necessary). Use your actual downloaded paths:

   | Variable | Example value |
   | --- | --- |
   | `KLEIOTIDE_SOURCE_ARCHIVE` | `C:/Kleiotide-packaging/Kleiotide_0.29.4_Source_For_Installer.zip` |
   | `KLEIOTIDE_QT_SOURCE_ARCHIVE` | `C:/Kleiotide-packaging/qt-everywhere-src-6.5.3.zip` |
   | `KLEIOTIDE_VC_REDIST` | `C:/Kleiotide-packaging/vc_redist.x64.exe` |

   Normally leave `KLEIOTIDE_NSIS_COMPILER` empty; the standard NSIS location is
   detected. For a custom install, point it at the actual `makensis.exe`.
   Set `KLEIOTIDE_INSTALLER_REVISION` to **1** for this new application version,
   including when copying CMake settings from the old version.
5. Run CMake again. In Build Steps > CMake Build, select **`test-installer`**
   as the only build target, then press **Ctrl+B**. This builds the app, reruns
   the enabled `release-check` suite and creates the installer. The script uses
   a process-local `RemoteSigned` policy; it does not alter Windows policy.

The **Compile Output** ends with the setup filename and release-folder path.
Inside the build directory, find:

```text
test-installers/Kleiotide_0.29.4_test1/
```

It contains the `...Windows_x64_Setup.exe`, matching Kleiotide/Qt source ZIPs,
`READ-ME.txt`, `BUILD-MANIFEST.json` and `SHA256SUMS.txt`. The manifest records
versions and source/runtime hashes; it contains no private local build paths.
No network download occurs during installation on a colleague's computer.

An error leaves the partial output for inspection and does not overwrite a
previous package. Correct the reported problem and increase
`KLEIOTIDE_INSTALLER_REVISION` to **2** before retrying. Return the build target
to `kleiotide` for ordinary development after packaging.

## 3. Accept the package on a clean Windows computer

First try the setup on your own PC, then on at least one x64 Windows machine
or fresh VM **without Qt Creator, Qt DLLs in PATH or development tools**. A
successful launch on your developer laptop alone can conceal missing DLLs.

1. Run setup, approve installation and check the installed version in Help >
   About. Launch from Start, outside Qt Creator. The default test path is under
   Program Files and ordinary users should still be able to run the app.
2. Open a FASTA, an AB1 and a `.kleio` session using File > Open. Make a small
   edit, Undo/Redo, save into Documents using spaces/Unicode, close and reopen.
   Use test data; do not try saving into the protected installation folder.
3. Check group import, a small analysis and cancellation. Try the colleague's
   display scaling and, where available, a substantially lower-memory computer.
   The tested laptop's 32 GB is not an established minimum requirement.
4. With MAFFT configured on that machine, check success and cancellation.
   Record Not tested if MAFFT is unavailable; basic editing does not require it.
5. Close Kleiotide. Uninstall through Windows Settings > Apps. Confirm the
   shortcut/program are removed and your saved data remains. Reinstall and
   launch once more. Also verify uninstall asks you to close a running app.
6. Record Windows edition/build, CPU/RAM, scaling, test package revision,
   installer/runtime messages, result of each step and any skipped check.

After this passes, share the same package with testers using [TESTING.md](TESTING.md). Save their package SHA256 and reports. Repackage changed
application binaries under a new application version; packaging-only attempts
can increment the test revision. Retain exact package hashes and results for every distributed revision.

## References

- Qt 6.5 deployment: https://doc.qt.io/qt-6.5/windows-deployment.html
- Qt licensing/notices: https://doc.qt.io/qt-6.5/licensing.html and
  https://doc.qt.io/qt-6.5/licenses-used-in-qt.html
- Microsoft runtime distribution:
  https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files
- NSIS: https://nsis.sourceforge.io/License
