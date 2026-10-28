# Implementation status — 0.27.1

27 October 2026. Follows Oleg's accepted native Windows 0.27.0 test 1: all
37 automatic tests and focused manual Settings/recovery/overview checks pass.
This is a correction release, not RC1. No Git checkout/remote is available in
this workspace; changes are supplied as source archives and a patch.

Implemented: bounded incremental overview plus single-row initial scan path;
caret/selection and viewport markers; FASTA suffix normalization; virtual read
alignment merge preview and default partial results; Overwrite wording; View
menu regrouping; separate translation row and adjacent toggle after restoration;
About GitHub/citation; Settings section and explicit session-appearance action;
local FASTA/AB1 batch drag/drop; Windows Open With registration refresh and
per-user repair script; testing-only checkpoint label.

See RELEASE_PLAN_0.27.1_0.28.0.md for every request, decisions and the detailed
BLAST/trace-link plan. Those workflows are scheduled for 0.28.0. Session format
remains 4 and GPL/branding terms are unchanged. Local verification is recorded in
VALIDATION.md; Windows Explorer/installer and real ARM behavior require native
checks. A colleague reports MAFFT 7.526 with explicit Threads=1 succeeds on ARM
using 0.26.11 test 2; the automatic fallback in later versions still needs an
actual ARM run. No new ARM automatic-policy success is claimed.
