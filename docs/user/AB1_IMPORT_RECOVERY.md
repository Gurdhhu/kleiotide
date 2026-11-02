# AB1 import compatibility

The importer validates call/peak/quality counts, channel shapes, dye order, sample positions and payload bounds before accepting a file. The original AB1 is never rewritten. Imported calls, analyzed channels, peak positions and available original qualities are embedded in native sessions.

## Nonstandard X/x calls

Some edited ABIF files use X/x for an unknown nucleotide. Kleiotide imports these as N/n, preserving case and call count, and reports the selected PBAS set, conversion count and positions. Other invalid call bytes reject with tag/set and position information. A different call set is not silently substituted to avoid an error. AB1 export uses the normalized N/n symbols.

## Stale sample-name offsets

A narrowly bounded fallback can recognize a unique Pascal-string sample-name length change that shifted a complete directory and later data by one constant offset. It requires a uniquely identifiable repair and validates every recovered tag plus all ordinary trace invariants. Multiple plausible repairs or unrelated malformed payloads reject. Recovery is attempted only after normal directory parsing fails, and an import notice explains the recovery.

This compatibility path is not a general repair utility. Preserve the original instrument file and inspect the imported trace. [Edited AB1 export](AB1_EXPORT.md) writes a new viewing/editing file from the validated evidence rather than repairing the source in place.
