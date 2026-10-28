# Next steps

Build 0.27.2 with the usual Qt 6.5.3/MSVC2022 x64 Release kit in a fresh directory.
Run release-check (38 entries), then test-installer with the exact matching inner
source ZIP and revision 1. Reuse matching Qt/runtime/NSIS inputs. Follow
RELEASE_0.27.2_TESTING.md for clipboard, tooltips, trace layout and session append.

0.27.1 is accepted on native Windows; no new native Windows/ARM run is claimed
here. BLAST and trace assignment/mapping remain planned for 0.28.0; see the
included RELEASE_PLAN_0.27.1_0.28.0.md. Manual recovery checkpoints are still
marked testing-only for removal before public release.
