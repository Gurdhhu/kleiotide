# Building Kleiotide

Use CMake 3.21+, a C++20 compiler and Qt >=6.5 with Widgets, Concurrent and Network. Windows uses an MSVC 2022 x64 Release kit. Qt Creator's own Qt version is independent of the selected application kit. Extract or clone into a clean source directory and use a separate build directory.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/kit -DKLEIOTIDE_BUILD_RELEASE_CHECKS=ON
cmake --build build --parallel 2
cmake --build build --target release-check
```

The full suite registers 47 tests: 20 core, nine Qt unit and 18 GUI workflows. See [TESTING.md](TESTING.md) for logs and optional real-tool checks. MAFFT and BLAST+ are not needed for the normal automated suite. Test fixtures use local fake tools and loopback HTTP.

For core-only, Qt-free builds:

```sh
cmake -S . -B build-core -DKLEIOTIDE_BUILD_GUI=OFF
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure
```

On Windows, configure and build from Qt Creator with the intended Qt/MSVC kit or use an x64 Native Tools command environment with its Qt prefix. Test runtime paths are configured by CMake. If a test cannot start, inspect the selected Qt DLL/plugin paths before interpreting that as a calculation failure.

Install with CMake into a clean staging folder. User documentation is installed under docs; core license/branding/README files remain at the installation root. Windows setup creation is described in [WINDOWS_INSTALLER.md](WINDOWS_INSTALLER.md).
