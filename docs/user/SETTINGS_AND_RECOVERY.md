# Settings and recovery

## File → Settings

OK saves preferences and closes; Apply saves without closing. Cancel abandons
unapplied edits. Restore defaults edits the dialog draft until Apply/OK. Restore
defaults never deletes recovery files. Settings use the existing per-user
Kleiotide QSettings namespace, with validated, versioned values.

Appearance and editing: alignment font family, point size, bold/italic, residue
palette, colored letters/monochrome/backgrounds, identity dots, initial View/Edit
and Overwrite/Insert, editing right-click choice and overview/trace visibility.
The preview shows the chosen font/colors. Ruler sizing follows the alignment
font using the existing readability rules. Use current appearance as defaults
copies the current session's display choices into the dialog draft.

Translation: choose a built-in default genetic code, or use the current
session's full custom code. Frames and coding-region coordinates remain
session data and are not global defaults. A new blank document or fresh
FASTA/AB1 import starts with this code without creating an Undo operation.

Files: default FASTA import type initializes new windows. The existing File →
FASTA import type command remains an explicit choice for imports in that window.
Saved sessions and AB1 files retain their declared row types.

MAFFT: default launcher path and Automatic (safe) or an explicit thread count.
Browse selects a launcher; Test a small alignment actually runs two short
nucleotide rows through that launcher, preserving its report and cancellation.
It does not merely query --version. Per-job Align dialog changes do not update
global defaults. Existing mafft/launcher and mafft/threads values initialize the
new preferences when no new-format preference value exists; old -1 becomes safe
automatic. Unavailable launchers can be replaced in Settings or per job.

## Precedence and existing windows

New clean sessions use the global appearance/code defaults. Opening a .kleio
uses its saved document metadata and view state; missing legacy view fields use
legacy defaults, not the user's new preferences. View controls still override
the appearance in the current session. Applying global defaults does not change
already open alignment letters, colors, modes, genetic codes or dirty state.

Resource preferences take effect for subsequent work in existing windows in the
same application instance. A busy window defers them until its job finishes.
Other simultaneously running application instances read preferences on restart.
Machine-specific preferences are not added to portable session settings.

## Memory and performance

| Setting | Migration/default value | Meaning |
| --- | --- | --- |
| Additional bulk-edit working limit | 2048 MiB | Existing preflight bound for bulk text/Undo work |
| Undo retention target | 64 MiB | Older history can be pruned; newest command retained even when larger |
| Overview workspace limit | 16 MiB | Bounds overview tile, row index and summary vector storage |
| Overview debounce | 150 ms | Delay before restarting the overview after content changes |

These are separate policies, not a total process RAM cap or an allocation
reservation. Stored alignment/trace evidence, other analyses, allocator overhead
and external MAFFT require additional memory. The overview setting does not
bound every translation/highlight/analysis workspace. Lowering Undo retention
may prune old commands immediately in idle windows. Jobs retain captured limits.

The overview processes bounded column tiles, then retains up to 8192 summaries.
Each column's IUPAC-weighted conservation is calculated before aggregation;
conserved A and conserved C columns do not become falsely variable when binned.
Coverage still includes N/? as non-gap symbols, while they cast no conservation
votes. Protein and mask rows are excluded. Small alignments retain exact column
resolution; long alignments are rendered by grouping complete bounded summaries.
Tooltips report the actual represented column ranges. No full-length evidence
array is allocated, but the scan still visits the stored nucleotide symbols.

## Recovery

Enabled by default, with a 120-second interval, short idle delay and a default
1024 MiB retained-data disk quota. Continuous typing does not indefinitely
postpone a due checkpoint: after twice the interval it can run at the next safe
GUI turn. Busy jobs and open modal editors defer writing. No unconditional crash/OOM handler attempts
to serialize damaged live state.

A checkpoint is a separate .kleio under the application-data recovery folder;
Settings displays the exact location and can open it. It never replaces the
source FASTA, AB1 or saved session. The source is read-stable while a worker
streams the checkpoint; editing in that window is temporarily gated. Writes are reported in the status bar with a Cancel checkpoint button, without opening a progress window. When the safety gate ends, the prior keyboard focus is restored if the window is still active; switching to another window is respected. No second full Document is copied.
The ordinary document saved/dirty state is preserved. Like ordinary .kleio files,
checkpoints restore current data/metadata/evidence; they do not store Undo history.

Up to two completed generations per document are retained within the configured
quota. Replacement is atomic and does not remove the previous completed file
before commit. A temporary replacement may use additional disk space beyond the
retained quota; disk/write/serialization errors and cancellation leave prior
completed generations intact. The status bar reports the result; File → Recovery
status shows the current status. Recovery may refuse oversized sessions, quota
exhaustion or unavailable storage; it is not a guarantee against all data loss.

Each window has a unique locked recovery identity. Active windows/processes are
excluded from recovery choices. Startup without an input file offers available
checkpoints; File → Recover session… is always the manual entry point. A previous
generation can be selected if the newest cannot be read. Native checksums and
strict input validation remain enabled.

- Recover opens a new unsaved window. The claimed old checkpoint is retained
  until the new window saves, commits a new checkpoint, or confirms discard.
- Open copy opens a new unsaved window and leaves the old checkpoint available.
- Discard selected asks for confirmation and deletes only that recovery group's
  generations; original input/output files are untouched.

File → Write recovery checkpoint now (testing only) provides an explicit checkpoint for unsaved
work, including when periodic recovery is disabled. Successful ordinary Save or
confirmed document close/replacement retires that document's recovery files.
FASTA export alone does not retire recovery or falsely mark metadata as saved.
Turning recovery off does not delete previous checkpoints.


## Applying appearance to the current session

Apply to current session on the appearance page explicitly applies the draft appearance/editing choices to this window without saving global defaults or changing its genetic code. Cancel does not undo that explicit action.

## BLAST defaults

The BLAST page separates NCBI HTTPS options from local BLAST+ executables and local search defaults. Executable paths never change HTTPS routing. Each search can override its settings without rewriting global defaults. Local tools are installed separately; the page includes a short download link. See [NCBI BLAST](NCBI_BLAST.md) and [Local BLAST](LOCAL_BLAST.md).
