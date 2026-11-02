# Local BLAST

Install a package matching your operating system: [Download NCBI BLAST+](https://ftp.ncbi.nlm.nih.gov/blast/executables/blast+/LATEST/). This link is also available in local search dialogs and Settings.

Install NCBI BLAST+ separately. Kleiotide does not bundle its executables.
Set paths in File → Settings → BLAST, under Local BLAST+. You can set each tool,
leave paths blank to find it beside another configured tool or on PATH, or browse
for the required search executable in a search dialog. NCBI remote HTTPS searches
have a separate section and never run BLAST+.

## Create a database

BLAST → Create a local nucleotide/protein database file… accepts FASTA or .kleio.
FASTA is interpreted as the selected molecule; native sessions supply only rows
of the requested type. Masks and empty rows are skipped, all gaps are removed,
and ? is converted to N or X. Sequence names, including duplicate or long names,
are retained through synthetic database IDs.

Choose a new .kblast filename. Creation runs makeblastdb locally, using version-4
BLAST database files. A description and name map are saved as name.kblast; its
sequence/index files are in name.kblast.files/. Keep these together when moving
or sharing a database. Existing output is never overwritten. A successful build
is added to the recent database lists. If moved, Browse to the .kblast file again.

## Search

BLAST → Local nucleotide BLAST… and Local protein BLAST… open the same dialog
with different initial query types. Select a program:

| Program | Query | Database |
| --- | --- | --- |
| blastn | Nucleotide | Nucleotide |
| blastp | Protein | Protein |
| blastx | Translated nucleotide | Protein |
| tblastn | Protein | Translated nucleotide |
| tblastx | Translated nucleotide | Translated nucleotide |

The database list shows only the target molecule for that program. Query scope
skips masks and rows of the other molecule. Identical uppercase queries are
searched once and expanded to their original aliases in the results.

Options include task presets where available, seed size (0 uses the task default),
E value, target count, identity/query-cover filters, masking, strands, nucleotide
scores, protein matrix/composition statistics, applicable genetic codes, CPU
threads and a time limit. tblastx is ungapped, so gap-cost controls are hidden.
The installed executable validates scoring combinations it supports.

Execution is asynchronous. The search window uses a names/sequence snapshot captured when it opened and leaves the originating alignment editable. Cancel or the
running-time limit stops the local child process. The Results tab records the
executable/version, arguments, elapsed time and errors. Local searches do not
use RIDs or resume remote jobs. BLAST usage reporting is disabled for these
local child processes.

## Results

The 15-column table and XLSX/TSV/CSV exports include query/hit ordinals, original
subject names, identity, query coverage, HSP count, E value, length, mismatches,
query/hit coordinates and bit score. Identity and most metrics describe the
highest-bit-score HSP; coverage is the union of all query spans. Translated
search alignment lengths are in amino acids, while query/hit coordinates use
their respective sequence coordinate systems. No taxonomy/GenBank metadata is
invented for local sequences. TSV protects formula-like text; XLSX uses literal
text and numeric metric cells.

Limits include 100,000 database sequence-name entries, 16 MiB description files,
64 MiB result JSON and 500,000 displayed rows. Large jobs may require narrower
scope/target limits. Database building reads the selected source in a background
worker; cancelling during a source read waits for that read to finish safely.

Save table includes search settings (Kleiotide and reported BLAST+ versions, start time and actual arguments) before the results. XLSX uses separate worksheets; TSV/CSV contain the same sections in one table. A color/text status inset distinguishes active, completed, failed and cancelled searches.
