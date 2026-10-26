# Windows 11 on ARM — test installer revision 2

The reported laptop has a Snapdragon X Elite ARM64 processor, 16 GB RAM and
Windows 11 Home 25H2. Test installer 1 explicitly accepted only native x64
Windows, so it refused before trying the application. No hardware replacement
is implied by that refusal.

Windows 11 supports x64 user-mode application emulation. Test installer 2 permits
ARM64 Windows 11 when the Windows capability query confirms x64 user-mode
support. The application and Qt libraries remain **x64**, built with the same
Qt 6.5.3 / MSVC2022 x64 Release kit. This is not a native ARM64 build or a claim
that the application has already passed ARM testing.

## Build and give the updated setup to the colleague

Follow the final section of WINDOWS_INSTALLER.md. Use the updated source ZIP,
a fresh source/build folder and installer revision **2**. Reuse the existing
matching Qt source archive and original Microsoft `vc_redist.x64.exe`. The
current Microsoft x64 Redistributable includes ARM64 components as well.

The result is `Kleiotide_0.26.11_test2_Windows_x64_Setup.exe`. Its x64 filename
describes the application binaries; ARM computers use Windows 11 emulation.
Keep corresponding source available beside the installer when distributing it.

## First ARM test

1. Install test 2 and note any exact setup/runtime error. The earlier test-1
   architecture rejection installed no Kleiotide files and needs no uninstall.
2. Launch from Start as an ordinary user. Confirm Help > About says 0.26.11.
3. With copies of small files, open FASTA, AB1 and .kleio; edit, Undo/Redo,
   save into Documents and reopen. Check a group import and a small analysis.
4. Check cancellation and display scaling. Start with modest inputs before
   comparing larger-file performance with the native x64 laptop.
5. If MAFFT is installed/configured on that computer, test a small alignment
   and cancellation. Its external executable/launcher needs its own ARM
   compatibility result; mark it Not tested if unavailable.
6. Close Kleiotide, uninstall, confirm saved files remain, then reinstall and
   launch again. Include setup revision and emulated ARM64 status in feedback.

If an error remains, send its exact text and the step where it occurred. Do not
change Windows compatibility settings or bypass installer checks to continue.

## Authoritative references

- Windows x64 emulation on ARM:
  https://learn.microsoft.com/en-us/windows/arm/apps-on-arm-x86-emulation
- Runtime architecture capability query (Windows build 22000+):
  https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getmachinetypeattributes
- UserEnabled capability bit:
  https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ne-processthreadsapi-machine_attributes
- Microsoft x64 Redistributable, including ARM64 components:
  https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist
