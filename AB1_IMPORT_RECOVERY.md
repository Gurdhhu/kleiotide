# AB1 import compatibility

## Unknown X/x calls — 0.26.8

Some edited ABIF files contain X/x as an unknown nucleotide. Kleiotide imports
these as N/n, preserving case and call count. The import notice identifies the
selected PBAS set, conversion count and the first ten one-based positions.
The supplied failing trace contains one lowercase x at call 523 of 566 in PBAS1;
it imports as n with its original Q0 and peak position 15775. PBAS2 contains a
different, unedited 1364-call sequence and is deliberately not substituted.

Only this unknown-call alias is accepted. Other invalid call bytes still reject
with their tag/set, position and byte value. Normal bounds, shape, call/peak/Q
count, dye-order and trace validation remain mandatory. Notices are returned
only after validation succeeds, and combine with an offset-recovery notice if
both apply. The source AB1 is never rewritten. The normalized imported calls,
original peak/Q arrays and all analyzed samples are retained in native sessions;
AB1 export writes N/n rather than restoring the nonstandard X/x alias.

## Recovery of stale name-edit offsets — 0.25.1

The supplied MYX15849_PHYkar_cf_i2469_S2.ab1 contains a longer Pascal sample-name
string inserted into an older unused name area. It grew from 5 to 28 bytes.
Everything after it moved by 23 bytes, but the root directory offset and later
payload offsets were not updated. The actual directory begins at 253121 rather
than the declared 253098. Reading the declared location as directory entries
produced the earlier out-of-file payload error and a garbled binary tag name.

The import fallback is limited to uniquely identifiable single Pascal-string
length changes. It requires a nearby complete directory, an exact-size printable
replacement at a name payload or bounded unused gap, consistent constant offset
shift, and valid dimensions/bounds for every recovered tag. More than one
plausible repair is rejected. A broken unrelated payload is not simply skipped.
Recovery is attempted only after ordinary directory parsing fails; normal files
retain the standard path. Complete calls, peaks, quality, dye order, channels and
trace invariants are validated before the import is accepted or a recovery note
is returned. The source file is never rewritten.

On this file, Kleiotide recovers 580 calls and 6,998 samples per channel. Its
active SMPL tag still contains 7337; the new text was written into an earlier
unused sample-name area. The row title follows the filename as usual. A
one-time import message reports the 23-byte recovery. Native session saving and
edited AB1 export then use the ordinary validated trace model.

Validation compares all recovered calls, peaks, Q values and every sample in
all four dye channels against an independently offset-corrected reference using
a separate Python struct parser. Longer/shorter/empty/max-length name changes,
live and unused name payloads, rejection cases, editing and export are covered
by synthetic tests. The research AB1 is not bundled in the source archive.
