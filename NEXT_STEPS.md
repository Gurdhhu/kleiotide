# Next steps

Build 0.28.0 with the existing Qt 6.5.3/MSVC2022 x64 Release kit and a fresh build
directory. Run release-check (41 entries), then test-installer with the exact
matching source ZIP, revision 1 and existing Qt/runtime/NSIS inputs. Qt Network
and the Windows Schannel plugin are now required deployment checks.

Follow RELEASE_0.28.0_TESTING.md. Standard BLAST uses built-in HTTPS. Optional
NCBI BLAST+ blastn/blastp is required only for the clearly identified advanced
controls; it is separate from MAFFT. Live public-query NCBI and native Windows
rendering/install tests remain manual. Trace mapping needs review on actual
research edits despite synthetic and supplied-read ground-truth checks.

0.27.2 is the accepted native baseline; ARM MAFFT can be tested on this or a later
build. Preserve sources/dependency materials beside setup. Keep the manual
recovery checkpoint marked testing-only and remove it before public release.
