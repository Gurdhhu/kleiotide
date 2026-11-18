# Debian 13 test package

The first Linux package targets **Debian 13 (trixie), amd64**. Debian 13 supplies
Qt 6.8 through its own repositories. The package uses Debian's Qt runtime,
including the X11 and Wayland platform plug-ins. It includes a desktop launcher
and file associations for .kleio sessions, .ab1/.abi chromatograms and FASTA.
Choose your default application in your desktop's file manager. MAFFT and NCBI
BLAST+ are optional packages.

## Build the package

Once the workflow is on the repository's default branch, run **Debian 13 test
package** from the Actions tab and download its artifact after the tests pass. Alternatively, from the source
directory on Debian 13 amd64:

```sh
sudo apt install build-essential cmake ninja-build dpkg-dev qt6-base-dev qt6-qpa-plugins fonts-dejavu-core desktop-file-utils shared-mime-info
cmake -S . -B build-debian -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DKLEIOTIDE_BUILD_RELEASE_CHECKS=ON -DKLEIOTIDE_PACKAGE_DEBIAN=ON
cmake --build build-debian --target release-check --parallel 2
cpack --config build-debian/CPackConfig.cmake -B packages
```

The package is `packages/kleiotide_0.29.9-1~deb13_amd64.deb`. The workflow
also installs it in its Debian build container to verify the dependencies and
desktop registration. A native desktop test is still needed.

## Install and test

```sh
sudo apt install ./kleiotide_0.29.9-1~deb13_amd64.deb
```

1. Launch Kleiotide from the desktop menu and from a terminal with `kleiotide`.
2. Use **Open With → Kleiotide** from the file manager for FASTA, AB1 and .kleio.
   Include paths with spaces and Unicode. Check both first launch and an already
   open app. Shared FASTA and AB1 defaults follow the desktop's settings.
3. Open and edit a small disposable alignment, inspect and resize the
   chromatogram and overview panels, then save and reopen the native session.
4. Test MAFFT and local BLAST+ if installed. Test one small public NCBI nucleotide
   search to verify the HTTPS backend and system certificates.
5. Remove the package with `sudo apt remove kleiotide`. Confirm the launcher and
   registered associations are removed, and the files you saved still open after
   reinstalling.

Ubuntu and other Debian releases have their own Qt and system library versions.
Build and test against those releases separately before offering packages for them.
