# Focused Windows checks — 0.27.1 test 1

Run release-check (38 entries) before test-installer. All checks below use copies
of test data; ordinary regressions remain in WINDOWS_TESTING.md.

1. Export FASTA with no suffix, .fas, .FA, .fna, .faa and an unrelated suffix.
   Missing/unrecognized suffix gains .fasta. Try a final filename that already
   exists, refuse replacement, cancel, and try .ab1/.kleio. Old files survive.
2. Open a long assembly and wait for initial overview. Overwrite one base near
   the middle, then Undo/Redo and several rapid overwrites. Only affected summary
   bins recalculate. Insert/delete a residue and add/remove/reorder/change row
   type: these may rescan. The initial scan still reads nucleotide symbols; it
   is not an instantaneous thumbnail or a persistent on-disk cache. Record size
   and initial/update times without treating a machine-specific time as a limit.
3. Move the caret and select a residue block/all columns. Magenta marker/range
   follows it; white/blue viewport frame follows scrolling. Markers update
   without restarting calculations. Click/drag overview navigation still works.
4. Find overlap and merge: test perfect overlaps, substitutions, internal indels,
   contained reads, excluded reads and separate components. Inspect originals
   aligned to consensus; amber highlights every symbol-difference column,
   ignoring case and normalizing U/T. Empty flanks mean outside the read; an
   internal gap remains visible. Partial results can Apply/Save immediately;
   amber notice and exclusion list remain. Apply is one Undo; originals remain.
5. Check Overwrite label everywhere. View → Toolbars contains all four panel
   controls. Show Translation, narrow/widen repeatedly, toggle Editing controls,
   Restore default layout repeatedly. Expanded translation is on a lower row;
   toggle remains beside palette. Try translation disabled during restore too.
   At 100/125/150/200% ensure dialog buttons remain reachable and previews scroll.
6. About contains a working GitHub link and preliminary citation with this
   version. Settings is its own File section just above Exit. Its appearance
   page has “Apply to current session” directly below the current-defaults copy
   button: it applies the draft appearance/editing choices immediately, without
   saving global defaults or changing code. Cancel keeps that explicitly applied
   appearance; ordinary Apply still saves defaults for new sessions only.
7. Drag local FASTA and AB1 files together onto the alignment. They append as
   Add sequence from file, retaining existing rows and trace evidence, with one
   Undo. Decline malformed mixed batches; no partial append. Sessions are opened
   using File → Open, not this append-only drop path. Remote URLs are ignored.
8. Install the new test package. In Explorer use Open with → Choose another app
   → Choose an app on your PC → the new installed kleiotide.exe for AB1, FASTA
   and .kleio, with spaces/Unicode, both with no process and one running. Check
   existing default apps are unchanged. With two test versions installed, uninstall
   the older one and verify the newer Open With command remains. Installer refresh
   is a fix for suspected stale registration, not a confirmed native reproduction.
   If alternate administrator credentials were used, per-user entries may belong
   to that administrator: run the installed tools/Register-Kleiotide.ps1 as the
   affected ordinary user with -Executable pointing to the new EXE. Do not elevate
   that repair script. It advertises Open With; it does not change default apps.
9. Manual checkpoint menu says “testing only”. To test preference preservation:
   create a completed checkpoint, note its path/status, turn periodic recovery
   OFF and Apply; verify the completed file still exists. Restore defaults and
   Apply; it still exists. No new checkpoint is expected just because preferences
   changed. Then use the existing recovery crash-test procedure on disposable
   unsaved work and restore your intended preferences.
10. Real ARM test when available: MAFFT Automatic (safe), explicit Threads=1 if
    necessary, cancellation and retained logs. Earlier 0.26.11 explicit-1 success
    does not establish this version's automatic fallback behavior.
