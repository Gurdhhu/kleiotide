# Validation of Kleiotide 0.26.11

Executed on 22 October 2026. This patch follows the supplied Windows 0.26.10
29/30 automatic result, manual acceptance and large-file observations recorded
in WINDOWS_ACCEPTANCE.md. Changes cover merge-test synchronization, search-event
explanations, Windows output-name validation, and bounded bulk editing.

## Passed locally

- Release application/tests rebuilt with GCC 13.3/C++20. The final build has no
  compiler warnings; early mistakes in new test code were corrected before this
  delivered build.
- Actual release-check target: **30/30 CTest entries passed**, with text/JUnit
  output. The suite remains 17 core, four Qt unit and nine GUI workflow entries.
- Nine GUI workflow suites at 150% scaling: **9/9 passed**. The merge workflow
  additionally passed **10 consecutive runs at 125%**.
- All 17 core suites rebuilt against the changed Document layout with
  AddressSanitizer and UndefinedBehaviorSanitizer: **17/17 passed**. Leak
  detection disabled; no GUI sanitizer or leak-check claim.
- A controlled 50 ms initial-preview delay plus immediate test polling
  reproduces the old merge assertion exactly. With the completion-state wait,
  the same dialog/test scenario passes. The production preview retains its
  normal zero-delay scheduling. This fixture establishes the timing hole without
  weakening the required error-banner assertion.
- Added merge regression: two incompatible substitutions plus one compatible
  ambiguity difference require a search allowance of 3, produce 2 final residue
  conflicts, and report 3 substitutions / 0 inserted-deleted bases / 1 compatible
  IUPAC difference. Known/random existing exact/tolerant/indel cases still pass.
- Windows filename-rule checks cover wildcards, forbidden characters, reserved
  device names and trailing dot/space, while accepting Unicode/spaces. A real
  Qt dialog in forced Windows-rules mode rejects ???? with a warning, retains
  its file-model filters, and accepts a valid retry while returning the selected
  file type. This runs the custom dialog path on Linux; native Windows still
  needs confirmation.
- Core regressions prove memory-limit refusals retain content/state/history;
  bounded Flush and Undo/Redo still work afterwards; already-ungapped rows are
  skipped; a ten-billion-cell virtual Flush refuses without allocating padding;
  and a one-base edit in a million-base ordinary row works with a tiny text
  working budget. A GUI workflow also triggers the real 2 GiB default Flush
  limit and verifies the live editor/document remain usable.
- Merge-preview captures inspected for complete, partial and error states, with
  the expanded edit breakdown fitting the result banner. Existing output safety,
  sessions, trace provenance, translation and group workflows remain covered.

## Memory policy and scope

Bulk preflight estimates additional text/undo work before large allocations.
The default limit is 2 GiB and is distinct from input size and the 64 MiB nominal
history-retention budget. Gap/case transformations skip unaffected rows. Ordinary
rows no longer build full temporary Sequence objects merely to validate absent
protein metadata. Rich trace/protein edits retain their required validation.
Flush catches allocation/limit exceptions through its GUI action.

The estimate is conservative text working storage, not an exact physical-RAM
reservation; allocator/metadata overhead and existing document/history/system
usage still matter. Low available memory can still cause allocations to fail.
No disk-backed or unlimited rectangular editing is implemented. Refused plans
are rejected before sequence changes; no user history is discarded to make room.

## Environment, native results and remaining checks

Local execution uses Qt 6.4.2 on Linux and CMake/CTest 3.28.3. The validation copy
lowers only the Qt package minimum and selects the working local CTest path;
application/test source matches the delivered source. Delivered CMake retains
Qt >=6.5 and CMake >=3.21.

The follow-up Windows 11 / Qt Creator 15.0.1 / Qt 6.5.3 MSVC2022 x64 Release
report confirms **30/30 automatic tests and all manual tests passing**, including
filename handling and oversized bulk-operation prevention. MAFFT 7.526 and the
hardware configuration are recorded in WINDOWS_ACCEPTANCE.md. The latest result
is user-reported; no additional raw logs accompanied that follow-up. Explorer
associations remain unconfigured and clean-machine deployment remains untested.

The separate licensing/branding policy and native format 4 are unchanged. Prior
validation records are retained under docs/history/.

## Installer-preparation validation (packaging revision 1)

- Application sources, test sources and supplied examples are byte-for-byte
  identical to the original 0.26.11 archive. No new native application test
  result is claimed; the user's passing Windows baseline remains the evidence.
- PowerShell 7.4.6 parser accepts the packaging script. It uses Windows
  PowerShell 5.1-compatible constructs; execution under Windows 5.1 is pending.
- CMake 3.28.3 configures the unchanged core-only path. A simulated MSVC/Windows
  configure registers test-installer with app/release-check dependencies and
  correctly quotes a Qt path containing spaces.
- The packaging script runs end-to-end against controlled fake Qt/runtime/app
  inputs using the real NSIS 3.09 compiler. It compiles installer/uninstaller,
  includes referenced license files beyond filename matching, emits source and
  build records, cleans its stage and produces valid SHA256 checksums. This
  establishes packaging control flow/syntax; the dummy payload cannot run the
  application and is not included in the delivered source archive.
- Refusal checks reject a mismatched source ZIP, an invalid mocked Microsoft
  signature and existing output. The existing setup hash remains unchanged.
- Actual MSVC binary deployment, Microsoft signature/runtime installation,
  Windows setup/UI/shortcuts, close-running-app handling, ordinary-user launch
  and uninstall require the clean-machine checks in WINDOWS_INSTALLER.md.

## Installer revision 2: ARM64 eligibility

The native-only guard explains the colleague's Snapdragon/Windows 11 rejection
before any application file was installed. Revised eligibility retains native
x64 Windows 10/11 and permits ARM64 Windows 11 (build >=22000) only if the
GetMachineTypeAttributes query succeeds and its UserEnabled bit is set for
IMAGE_FILE_MACHINE_AMD64 (0x8664). Application source/tests/examples are unchanged.

The actual Windows API query and all installation/execution behavior on ARM
require testing on the colleague's machine; source/compile checks cannot
establish runtime compatibility or performance.

Revision-2 packaging checks: the real NSIS 3.09 compiler successfully compiled
the updated installer and uninstaller using controlled placeholder application
and runtime files. This checks script syntax/expansion and inclusion of the
System plug-in; it does not execute the Windows API or application. The dummy
setup and payload are not shipped. The 147 archived application/test/example
files are byte-for-byte unchanged from installer revision 1. No native or
emulated application tests were rerun in this environment for this packaging fix.
