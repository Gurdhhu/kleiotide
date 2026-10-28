# Next steps

Build 0.27.1 with the same Qt 6.5.3 / MSVC2022 x64 Release kit in a fresh build
directory. Run release-check: 38 registered tests. Then use the focused checklist
in RELEASE_0.27.1_TESTING.md. Rebuild test-installer with the exact matching inner
Kleiotide_0.27.1_Source_For_Installer.zip and reuse matching Qt/runtime/NSIS inputs.

Windows Open With is a specific native retest, including with older test builds
installed, paths with spaces/Unicode, no running process and an existing process.
ARM MAFFT can be tested on this or a later version; Threads=1 is the colleague's
confirmed workaround. Preserve reports for Automatic and any fallback.

0.28.0 contains trace linking/mapping (the earlier checkpoint C) and the new
BLAST workflow. See RELEASE_PLAN_0.27.1_0.28.0.md. This remains a testing release;
remove the testing-only manual checkpoint menu item before public release.
