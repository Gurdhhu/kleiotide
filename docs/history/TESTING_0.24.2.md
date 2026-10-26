# Kleiotide 0.24.2 — filename suffix

The user confirmed 0.24.1 exports open in BioEdit and Chromas and all suggested
tests pass. The AB1 writer is unchanged; this update refines batch filenames.

1. Open File → Batch export edited traces (AB1)…. Filename suffix defaults to
   _edited. Change it to _trimmed and check the preview before export.
2. Clear the field: filenames should be the sanitized sequence titles plus .ab1.
   Try punctuation, invalid path characters and Unicode. Invalid filename
   characters become underscores; no subdirectories should be created.
3. Export selected/all traces. Check actual names match the preview. Duplicate
   names and existing files still receive numeric suffixes and are never replaced.
   Suffix editing must be unavailable during the export.

Build/application requirements and previous acceptance steps remain in README.md
and TESTING_0.24.1.md. Current targeted validation is recorded in VALIDATION.md.
