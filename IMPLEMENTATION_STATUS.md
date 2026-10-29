# Implementation status — 0.28.0 testing release

28 October 2026. Native Windows 0.27.2 is accepted: all 38 automatic tests and
all seven focused manual workflows pass. This source implements BLAST and trace
assignment/mapping plus the requested compact reads-to-contig preview.

Trace linking includes four-tier names/manual assignments, shared evidence,
affine forward/reverse mapping, all-optimal-path ambiguity handling, cancellable
preview, atomic Apply/Delete with Undo and existing session persistence.
BLAST includes built-in documented HTTP searches and an optional NCBI BLAST+
remote launcher for advanced controls, batching/dedup, throttling/off-peak waits,
RID/job resume, metadata enrichment, a sortable linked table and TSV/XLSX.

No bundled BLAST+ executable or local BLAST database is supplied. Unsupported
HTTP controls explicitly require that optional backend. Publication precision
is limited to information in GenBank references (often year only). Scientific
mapping labels remain heuristics; native acceptance is still required. Read
BLAST_AND_TRACE_LINKING.md, RELEASE_0.28.0_TESTING.md and VALIDATION.md.

Session format 4, GPL-3.0-only and branding policy remain unchanged. No Git
checkout/remote was available here; no push or Windows installer binary is
claimed. This remains a testing build, not RC1. Real ARM MAFFT is still pending.
