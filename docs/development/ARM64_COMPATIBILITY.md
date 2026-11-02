# Windows 11 on ARM

Kleiotide's Windows package currently contains x64 application and Qt binaries. It is not a native ARM64 build. The installer permits ARM64 Windows 11 only when Windows reports x64 user-mode support; Windows 10 on ARM and 32-bit Windows are excluded. Keep the original Microsoft x64 Redistributable input required by the installer workflow.

A Snapdragon Windows 11 laptop was reported to install and run basic FASTA/AB1/session/editing workflows with 0.26.11 test 2. Separately installed MAFFT 7.526 worked with explicit Threads=1; automatic core detection had failed with a physical-core-count error. The current launcher resolves safe positive thread counts and provides a bounded retry/report for recognized failures, but current native ARM Automatic/fallback acceptance is still outstanding.

For a retest, record Windows build, installer revision and executable versions. Check startup without development tools, FASTA/AB1/session roundtrips, clipboard/editing, narrow/high-DPI dialogs, HTTPS/TLS and uninstall/reinstall. With MAFFT installed, test Automatic, Threads=1, success/cancellation and diagnostics. Do not infer all workflows or memory limits from successful launch. See [testing](TESTING.md) and [installer preparation](WINDOWS_INSTALLER.md).
