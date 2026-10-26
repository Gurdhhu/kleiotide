# GitHub source-import notes — Kleiotide 0.26.11

Prepared from `Kleiotide_0.26.11_Installer_Prep_Source.zip` (the later
0.26.11 snapshot including installer revision 2). The older
`Kleiotide_0.26.11_Source.zip` has the same application and test source files,
but does not contain the newer installer preparation scripts.

## Changes for GitHub

- Added `.gitignore`, `.gitattributes`, `CONTRIBUTING.md`, `CITATION.cff`, and
  `docs/GIT_WORKFLOW.md`.
- Added a source-repository navigation section to `README.md`.
- Kept the existing GPL-3.0-only license, branding permission policy,
  historical documentation, fixtures, and Windows installer toolchain.
- No modifications to `src/`, `tests/`, `tools/`, `CMakeLists.txt`, or data
  fixtures were made in this packaging step.

## Checks performed

- CMake configured in a Linux environment with `KLEIOTIDE_BUILD_GUI=OFF`.
- The core-only code and tests compiled, and **17/17 CTest tests passed**.
- A separate native Windows + Qt GUI/installer build was **not** run for this
  GitHub preparation step. Consult `VALIDATION.md`, `WINDOWS_ACCEPTANCE.md`,
  and `WINDOWS_INSTALLER.md` for earlier reports and outstanding acceptance.
- No obvious secrets or machine-specific absolute paths were found in code;
  Windows paths in documentation are illustrative examples.

## Before changing the repository's visibility to public

Check that the license and branding policy continue to reflect the creators'
intentions, review third-party notices and any data provenance, and verify a
fresh clone builds in your actual supported Windows Qt configuration. Public
installer deployment still requires independent testing.
