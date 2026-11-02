# Contributing to Kleiotide

Kleiotide is under active development. Please coordinate substantial changes through a GitHub issue before investing time in a large implementation. For ordinary bugs, include the program version, operating system, Qt version (if building from source), reproducible steps, and a small non-confidential input file when possible.

## Building and testing

Follow [README.md](README.md) for the application build, then run the available tests:

```sh
cmake -S . -B build -DKLEIOTIDE_BUILD_RELEASE_CHECKS=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

On Windows, prefer the project-tested MSVC/Qt kit. Run the release regression checks and the native GUI acceptance procedure in [testing guide](docs/development/TESTING.md) before proposing a release. GUI check availability depends on the CMake options and local Qt installation. In particular, enabling `KLEIOTIDE_BUILD_RELEASE_CHECKS` makes the GUI release suites available.

For core-only, Qt-free testing:

```sh
cmake -S . -B build-core -DKLEIOTIDE_BUILD_GUI=OFF
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure
```

Please keep changes focused and add or update automated tests for behavioral changes. Avoid committing build directories, compiler outputs, installers, user-specific configurations, or large real specimen files. Do not submit biological data unless you have the right to share it.

## Licensing

Contributions to the GPL-covered program are expected to be distributable under **GPL-3.0-only**, consistent with [LICENSE-STATUS.md](LICENSE-STATUS.md). Confirm that any added code or data is compatible and include copyright/attribution notices as needed. The Kleiotide name and official artwork are subject to the separate [BRANDING.md](BRANDING.md) policy; artwork contributions must respect its ownership and permissions.
