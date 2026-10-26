# Next steps

Use WINDOWS_TESTING.md for step-by-step Windows checks and RELEASE_CHECKLIST.md for the current release gates. The application license is GPL version 3 only; branding permissions and the artwork credit are in BRANDING.md. Complete native Windows/clean-machine acceptance and dependency/source distribution review before RC1. No additional feature work is planned during stabilization. Earlier development checkpoints are retained under docs/history/.

The native 0.26.11 rerun now passes 30/30 and all reported manual checks. Next:
produce the first internal test installer with WINDOWS_INSTALLER.md; accept it
on a Windows machine without development tools, then collect colleague feedback.
The source adds packaging only; the application baseline remains 0.26.11.

Installer revision 2 enables compatibility testing on the reported ARM64 Windows
11 laptop through x64 emulation. Build test 2 from the updated source ZIP and
collect the focused ARM report before making a compatibility claim.
