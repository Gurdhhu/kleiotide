# Windows 11 on ARM — compatibility status

The reported laptop has a Snapdragon X Elite ARM64 processor, 16 GB RAM and
Windows 11 Home 25H2. Test installer 1 explicitly accepted only native x64
Windows, so it refused before trying the application. No hardware replacement
is implied by that refusal.

Windows 11 supports x64 user-mode application emulation. Test installer 2 permits
ARM64 Windows 11 when the Windows capability query confirms x64 user-mode
support. The application and Qt libraries remain **x64**, built with the same
Qt 6.5.3 / MSVC2022 x64 Release kit. This is not a native ARM64 build or a claim
that all ARM workflows have passed.

## Reported result and current retest

The colleague's 0.26.11 test-2 installer worked. Start menu launch, basic
FASTA/AB1 import, session save/reopen and editing passed without development
tools. MAFFT 7.526 failed with “Cannot count the number of physical cores” and
Cygwin FAST_CWD warnings. The toolbar disappeared in a separate screenshot.
Broader analyses, uninstall/reinstall and workload limits remain unconfirmed.

0.26.12 test 1 retains the ARM64 capability guard and adds safe positive MAFFT
threads, bounded retry/diagnostics and toolbar restoration. Follow
WINDOWS_INSTALLER.md to build it with the existing x64 kit and exact matching
source ZIP. The version change is an application change; do not overwrite the
old test-2 installer or its corresponding source.

Test a tiny alignment using Automatic (safe), then an explicit thread count of
1 if it fails; keep both job reports. Check cancellation, Edit → Mode, View →
Toolbars → Editing controls, and Restore default layout. Check save/reopen and
uninstall/reinstall with test data. Native rendering/touch and the real MAFFT
package cannot be validated by Linux fixture tests. Do not replace individual
Cygwin DLLs to work around an unexplained failure.

## Authoritative references

- Windows x64 emulation on ARM:
  https://learn.microsoft.com/en-us/windows/arm/apps-on-arm-x86-emulation
- Runtime architecture capability query (Windows build 22000+):
  https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getmachinetypeattributes
- UserEnabled capability bit:
  https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ne-processthreadsapi-machine_attributes
- Microsoft x64 Redistributable, including ARM64 components:
  https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist
