#pragma once
#include "core/alignment.hpp"
#include "gui/table_export.hpp"
#include <QByteArray>
#include <QJsonObject>
#include <QStringList>
#include <QUrlQuery>
namespace kleiotide {
struct BlastQuery {
  QString id, sequence;
  QStringList names;
  std::vector<int> numbers;
};
struct BlastOptions {
  bool local = false, protein = false, excludeEnvironmental = false,
       mask = true, softMask = true, shortQuery = false, forceToolkit = false;
  QString program;
  int threads = 1, queryCode = 1, databaseCode = 1;
  QString database = "core_nt", task = "megablast", email, entrez, launcher,
          strand = "both", matrix = "BLOSUM62";
  int waitMinutes = 120, maxHits = 10, word = 28, reward = 1, penalty = -2,
      gapOpen = 0, gapExtend = 0, composition = 2;
  double expect = .05, identity = 0, coverage = 0;
  QString error() const;
  bool toolkit() const;
  QJsonObject json() const;
  static BlastOptions from_json(const QJsonObject &);
};
struct BlastHit {
  QString queryId, accession, species, identifier, title, taxonomy, authors,
      date;
  double identity = 0, coverage = 0, evalue = 0, bits = 0;
  int hsps = 0, length = 0, mismatches = 0, qstart = 0, qend = 0, hstart = 0,
      hend = 0;
};
std::vector<BlastQuery> blast_queries(const Document &,
                                      const std::vector<SequenceId> &,
                                      bool protein, bool local = false);
std::vector<std::vector<BlastQuery>>
blast_batches(const std::vector<BlastQuery> &, std::size_t maxResidues = 100000,
              std::size_t maxQueries = 50);
QString blast_fasta(const std::vector<BlastQuery> &);
QUrlQuery blast_parameters(const BlastOptions &,
                           const std::vector<BlastQuery> &);
QStringList blast_arguments(const BlastOptions &, const QString &input);
std::vector<BlastHit> parse_blast(const QByteArray &,
                                  const std::vector<BlastQuery> &);
void enrich_blast(std::vector<BlastHit> &, const QByteArray &xml);
QStringList blast_columns(bool local = false);
bool blast_numeric_column(int, bool local = false);
QStringList blast_databases(bool protein);
using BlastTable = std::vector<QStringList>;
BlastTable blast_table(const std::vector<BlastQuery> &,
                       const std::vector<BlastHit> &, const BlastOptions &);
TableReport blast_report(const BlastTable&, const BlastOptions&, const QString& searchTime, const QString& backendVersion, const QString& arguments = {});
QByteArray blast_tsv(const BlastTable &, bool local = false);
QByteArray blast_xlsx(const BlastTable &, bool local = false,
                      bool protein = false);
} // namespace kleiotide
