# Validation of Kleiotide 0.26.10

Executed on 22 October 2026. This patch supplies the selected Qt runtime directory to Qt-dependent Windows CTest entries. Application algorithms, session format, licensing and branding are unchanged.

## Reported Windows run of 0.26.9

The user's full Qt Creator Compile Output records Qt 6.5.3 / MSVC 2022 x64, Release, and the correctly selected release-check target. Test-tool compilation/linking completed. All **17 core suites passed**. The **four Qt unit suites and nine GUI workflows** exited with **0xc0000135** before test execution. This status identifies a missing required DLL; the affected-test pattern points to the Qt runtime search path in CTest's environment. The log does not name the individual missing DLL. The final Ninja error reports the failed CTest command, not a C++ compilation error.

## Patch and local checks

- All 13 Qt-dependent entries call a shared CMake helper that, on Windows, prepends the configured Qt Core target's DLL directory to the test PATH. CMake >=3.22 uses ENVIRONMENT_MODIFICATION to retain the runtime environment; the CMake 3.21 fallback uses an escaped ENVIRONMENT value with the inherited configure-time PATH. Existing offscreen settings remain intact. Core-only tests do not receive the Qt path change.
- Release application/tests rebuilt with GCC 13.3/C++20; no compiler warnings observed in this patch's build log.
- Actual release-check target: **30/30 tests passed** locally. Text/JUnit reports generated; XML contains 30 test cases and no failure/error entries.
- **Six isolated CMake/CTest runtime fixtures passed**: modern and legacy helper branches, each in single-config Release and multi-config Release/Debug. They check configuration-specific imported-target directories, exact inherited PATH retention, semicolon escaping in the fallback, paths containing spaces/Unicode, and preservation of QT_QPA_PLATFORM. The legacy version condition is exercised with a simulated 3.21 CMAKE_VERSION value under the installed CMake 3.28.3 engine; it is not a native 3.21 engine run.
- CMake install includes LICENSE, BRANDING.md and the revised Windows test guide. Source ZIP contents are compared byte-for-byte with the delivery tree and checked for ZIP integrity.

## Environment and limits

The execution host is Linux with Qt 6.4.2 and CMake/CTest 3.28.3. The validation copy lowers only the Qt package minimum to the installed version and selects the working local CTest executable. Delivered CMake retains Qt >=6.5 and the 3.21 minimum. Runtime fixtures exercise CMake environment construction on Linux with mock imported DLL targets and the Windows helper branch enabled; they do not exercise the Windows DLL loader or establish a successful Windows Qt test run.

Native confirmation requires the user's repeat with 0.26.10 and the same Qt 6.5.3 / MSVC 2022 x64 Release kit. No global PATH changes or manual DLL copying are required by the revised CTest setup. Send the new log/XML if startup failures persist. Other native Windows/manual/deployment gates remain in RELEASE_CHECKLIST.md.

Earlier release results remain in docs/history/VALIDATION_0.26.9.md and preceding checkpoints. No import/edit algorithm changed here; the previous AB1 evidence and sanitizer results were not repeated.
