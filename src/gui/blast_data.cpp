#include "gui/blast_data.hpp"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
namespace kleiotide {
QString BlastOptions::error() const {
  if (!std::isfinite(expect) || expect <= 0 || expect > 1e6 ||
      waitMinutes < 1 || waitMinutes > 1440 || maxHits < 1 || maxHits > 5000 ||
      !std::isfinite(identity) || !std::isfinite(coverage) || identity < 0 ||
      identity > 100 || coverage < 0 || coverage > 100)
    return "Invalid E value, hit limit, filter or time limit.";
  if (local) {
    if (database.isEmpty() || launcher.isEmpty())
      return "Choose a local database and the matching BLAST+ executable.";
    if (!QStringList{"blastn", "blastp", "blastx", "tblastn", "tblastx"}
             .contains(program))
      return "Choose a supported local search type.";
    if (threads < 1 || threads > 256 || word < 0 || word > 256 ||
        queryCode < 1 || databaseCode < 1)
      return "Invalid local search settings.";
    return {};
  }
  if (protein)
    return "NCBI protein searches are unavailable. Use Local protein BLAST.";
  if (forceToolkit || !entrez.trimmed().isEmpty() || excludeEnvironmental ||
      strand != "both")
    return "BLAST+ remote searches have been removed. Use HTTPS defaults or "
           "Local BLAST.";
  if (database.isEmpty() || !blast_databases(protein).contains(database))
    return "Choose an NCBI database.";
  if (!QRegularExpression("^[^\\s@]+@[^\\s@]+\\.[^\\s@]+$")
           .match(email)
           .hasMatch())
    return "Enter your contact email in NCBI contact email *.";
  if (!(protein ? QStringList{"blastp"} : QStringList{"megablast", "blastn"})
           .contains(task))
    return "This task is unavailable for HTTPS. Use Local BLAST for other "
           "tasks.";
  const QList<int> allowed = protein ? QList<int>{3, 5, 6}
                             : task == "megablast"
                                 ? QList<int>{16, 20, 24, 28, 32, 48, 64}
                                 : QList<int>{7, 11, 15};
  if (!allowed.contains(word))
    return "Choose a word size supported by this HTTPS task.";
  if (!protein &&
      !QStringList{"1,-2", "1,-3", "2,-3", "1,-4", "4,-5", "1,-1"}.contains(
          QString::number(reward) + "," + QString::number(penalty)))
    return "Unsupported nucleotide scoring pair.";
  if (gapOpen || gapExtend) {
    const auto pair =
        QString::number(gapOpen) + " " + QString::number(gapExtend);
    const QStringList gaps =
        protein ? QStringList{"11 2", "10 2", "9 2",  "8 2",  "7 2", "6 2",
                              "13 1", "12 1", "11 1", "10 1", "9 1"}
                : QStringList{"4 4", "2 4", "0 4", "3 3",
                              "6 2", "5 2", "4 2", "2 2"};
    if (!gaps.contains(pair))
      return "Unsupported HTTPS gap-cost pair.";
  }
  return {};
}
bool BlastOptions::toolkit() const { return local; }
QJsonObject BlastOptions::json() const {
  return {{"local", local},
          {"program", program},
          {"threads", threads},
          {"queryCode", queryCode},
          {"databaseCode", databaseCode},
          {"forceToolkit", forceToolkit},
          {"waitMinutes", waitMinutes},
          {"protein", protein},
          {"database", database},
          {"task", task},
          {"email", email},
          {"entrez", entrez},
          {"launcher", launcher},
          {"strand", strand},
          {"matrix", matrix},
          {"maxHits", maxHits},
          {"word", word},
          {"reward", reward},
          {"penalty", penalty},
          {"gapOpen", gapOpen},
          {"gapExtend", gapExtend},
          {"composition", composition},
          {"expect", expect},
          {"identity", identity},
          {"coverage", coverage},
          {"excludeEnvironmental", excludeEnvironmental},
          {"mask", mask},
          {"softMask", softMask},
          {"shortQuery", shortQuery}};
}
BlastOptions BlastOptions::from_json(const QJsonObject &j) {
  BlastOptions o;
  o.protein = j["protein"].toBool();
  o.local = j["local"].toBool();
  o.program = j["program"].toString();
  o.threads = j["threads"].toInt(1);
  o.queryCode = j["queryCode"].toInt(1);
  o.databaseCode = j["databaseCode"].toInt(1);
#define S(x) o.x = j[#x].toString(o.x)
  S(database);
  S(task);
  S(email);
  S(entrez);
  S(launcher);
  S(strand);
  S(matrix);
#undef S
#define I(x) o.x = j[#x].toInt(o.x)
  I(waitMinutes);
  I(maxHits);
  I(word);
  I(reward);
  I(penalty);
  I(gapOpen);
  I(gapExtend);
  I(composition);
#undef I
  o.forceToolkit = j["forceToolkit"].toBool(!o.launcher.isEmpty());
  o.expect = j["expect"].toDouble(.05);
  o.identity = j["identity"].toDouble();
  o.coverage = j["coverage"].toDouble();
  o.excludeEnvironmental = j["excludeEnvironmental"].toBool();
  o.mask = j["mask"].toBool(true);
  o.softMask = j["softMask"].toBool(true);
  o.shortQuery = j["shortQuery"].toBool();
  return o;
}
std::vector<BlastQuery> blast_queries(const Document &d,
                                      const std::vector<SequenceId> &ids,
                                      bool protein, bool local) {
  int ordinal = 0;
  std::size_t uniqueResidues = 0, nameBytes = 0;
  std::vector<BlastQuery> result;
  std::map<QByteArray, std::vector<std::size_t>> hashes;
  for (auto id : ids) {
    const auto &row = d.sequence(id);
    if (row.kind != (protein ? RowKind::Protein : RowKind::Nucleotide))
      continue;
    if (!row.symbols.residue_count())
      continue;
    if (!local && row.symbols.residue_count() > 100000)
      throw std::length_error("A query exceeds 100,000 residues; use local "
                              "BLAST+ for long assemblies.");
    nameBytes += row.name.size();
    if (nameBytes > 16 * 1024 * 1024)
      throw std::length_error(
          "Query names exceed 16 MiB; select a smaller scope.");
    QString sequence;
    for (char c : row.symbols.text())
      if (!is_gap(c))
        sequence += QChar::fromLatin1(c).toUpper();
    sequence.replace('?', protein ? 'X' : 'N');
    if (sequence.isEmpty())
      continue;
    if (!local && sequence.size() > 100000)
      throw std::length_error("A query exceeds 100,000 residues. Use local "
                              "BLAST+ for long assemblies.");
    const auto hash = QCryptographicHash::hash(sequence.toLatin1(),
                                               QCryptographicHash::Sha256);
    std::size_t found = result.size();
    for (auto n : hashes[hash])
      if (result[n].sequence == sequence) {
        found = n;
        break;
      }
    if (found == result.size()) {
      uniqueResidues += sequence.size();
      if (!local && (uniqueResidues > 20000000 || result.size() >= 20000))
        throw std::length_error("Public-server job exceeds 20 million residues "
                                "or 20,000 unique queries.");
      hashes[hash].push_back(found);
      result.push_back({"q" + QString::number(found + 1), sequence, {}, {}});
    }
    result[found].names.append(QString::fromStdString(row.name));
    result[found].numbers.push_back(++ordinal);
  }
  return result;
}
std::vector<std::vector<BlastQuery>>
blast_batches(const std::vector<BlastQuery> &queries, std::size_t maxResidues,
              std::size_t maxQueries) {
  if (!maxResidues || !maxQueries)
    throw std::invalid_argument("Invalid batch limit");
  std::vector<std::vector<BlastQuery>> result;
  std::size_t size = 0;
  for (const auto &q : queries) {
    if (std::size_t(q.sequence.size()) > maxResidues)
      throw std::length_error("Query exceeds batch size");
    if (result.empty() || result.back().size() >= maxQueries ||
        size + q.sequence.size() > maxResidues) {
      result.emplace_back();
      size = 0;
    }
    result.back().push_back(q);
    size += q.sequence.size();
  }
  return result;
}
QString blast_fasta(const std::vector<BlastQuery> &queries) {
  QString s;
  for (const auto &q : queries)
    s += ">" + q.id + "\n" + q.sequence + "\n";
  return s;
}
QUrlQuery blast_parameters(const BlastOptions &o,
                           const std::vector<BlastQuery> &q) {
  if (o.toolkit())
    throw std::invalid_argument("Advanced options require the toolkit backend");
  if (o.protein)
    throw std::invalid_argument(
        "NCBI protein searches are unavailable. Use Local protein BLAST.");
  QUrlQuery p;
  auto a = [&](const QString &key, const QString &v) {
    p.addQueryItem(key, v);
  };
  a("CMD", "Put");
  a("PROGRAM", o.protein ? "blastp" : "blastn");
  a("DATABASE", o.database);
  a("QUERY", blast_fasta(q));
  a("EXPECT", QString::number(o.expect, 'g', 16));
  a("HITLIST_SIZE", QString::number(o.maxHits));
  a("WORD_SIZE", QString::number(o.word));
  a("FILTER", o.mask ? (o.softMask ? "mL" : "L") : "F");
  a("SHORT_QUERY_ADJUST", o.shortQuery ? "true" : "false");
  a("tool", "Kleiotide");
  a("email", o.email);
  if (o.protein) {
    a("MATRIX", o.matrix);
    a("COMPOSITION_BASED_STATISTICS", QString::number(o.composition));
  } else {
    if (o.task == "megablast")
      a("MEGABLAST", "on");
    a("NUCL_REWARD", QString::number(o.reward));
    a("NUCL_PENALTY", QString::number(o.penalty));
  }
  if (o.gapOpen || o.gapExtend)
    a("GAPCOSTS",
      QString::number(o.gapOpen) + " " + QString::number(o.gapExtend));
  return p;
}
QStringList blast_arguments(const BlastOptions &o, const QString &input) {
  if (!o.local)
    throw std::invalid_argument(
        "BLAST+ is available only for local databases.");
  const auto p = o.program;
  const bool nucleotide = p == "blastn";
  QStringList a = {"-query",
                   input,
                   "-db",
                   o.database,
                   "-evalue",
                   QString::number(o.expect, 'g', 16),
                   "-max_target_seqs",
                   QString::number(o.maxHits),
                   "-outfmt",
                   "15",
                   "-num_threads",
                   QString::number(o.threads)};
  if (nucleotide || p == "blastp")
    a << "-task" << o.task;
  if (o.word)
    a << "-word_size" << QString::number(o.word);
  a << (nucleotide ? "-dust" : "-seg") << (o.mask ? "yes" : "no");
  a << "-soft_masking" << (o.softMask ? "true" : "false");
  if (nucleotide)
    a << "-reward" << QString::number(o.reward) << "-penalty"
      << QString::number(o.penalty);
  else {
    a << "-matrix" << o.matrix;
    if (p != "tblastx")
      a << "-comp_based_stats" << QString::number(o.composition);
  }
  if (p == "blastn" || p == "blastx" || p == "tblastx")
    a << "-strand" << o.strand;
  if (p == "blastx" || p == "tblastx")
    a << "-query_gencode" << QString::number(o.queryCode);
  if (p == "tblastn" || p == "tblastx")
    a << "-db_gencode" << QString::number(o.databaseCode);
  if (p != "tblastx" && (o.gapOpen || o.gapExtend))
    a << "-gapopen" << QString::number(o.gapOpen) << "-gapextend"
      << QString::number(o.gapExtend);
  return a;
}
std::vector<BlastHit> parse_blast(const QByteArray &bytes,
                                  const std::vector<BlastQuery> &queries) {
  if (bytes.size() > 64 * 1024 * 1024)
    throw std::length_error("BLAST response too large");
  QJsonParseError e;
  auto doc = QJsonDocument::fromJson(bytes, &e);
  if (e.error != QJsonParseError::NoError || !doc.isObject())
    throw std::runtime_error("Malformed BLAST JSON response");
  auto outputs = doc.object()["BlastOutput2"].toArray();
  if (outputs.isEmpty())
    throw std::runtime_error("Missing BLAST reports");
  std::vector<BlastHit> result;
  std::set<QString> seen;
  for (auto value : outputs) {
    const auto report = value.toObject()["report"].toObject();
    if (report.contains("error"))
      throw std::runtime_error("NCBI returned a search error");
    const auto search = report["results"].toObject()["search"].toObject();
    const auto id = search["query_title"].toString().section(' ', 0, 0);
    const auto q =
        std::find_if(queries.begin(), queries.end(),
                     [&](const auto &query) { return query.id == id; });
    if (q == queries.end() || !seen.insert(id).second ||
        search["query_len"].toInt() != q->sequence.size())
      throw std::runtime_error("BLAST query identity/length mismatch");
    if (search.contains("error"))
      throw std::runtime_error("NCBI returned a query error");
    for (auto hv : search["hits"].toArray()) {
      const auto h = hv.toObject();
      const auto desc = h["description"].toArray();
      if (desc.isEmpty())
        continue;
      const auto d = desc.first().toObject();
      const auto hsps = h["hsps"].toArray();
      if (hsps.isEmpty())
        continue;
      BlastHit hit;
      hit.queryId = id;
      hit.accession = d["accession"].toString();
      hit.title = d["title"].toString();
      hit.species = d["sciname"].toString();
      QJsonObject best;
      double bestScore = -1;
      std::vector<std::pair<int, int>> intervals;
      for (auto hv2 : hsps) {
        const auto s = hv2.toObject();
        const auto from = s["query_from"].toInt(), to = s["query_to"].toInt(),
                   len = s["align_len"].toInt(), ident = s["identity"].toInt(),
                   gaps = s["gaps"].toInt();
        const auto score = s["bit_score"].toDouble(-1),
                   ev = s["evalue"].toDouble(-1);
        if (from < 1 || to < 1 || std::max(from, to) > q->sequence.size() ||
            len <= 0 || ident < 0 || gaps < 0 || ident + gaps > len ||
            score < 0 || ev < 0 || !std::isfinite(score) || !std::isfinite(ev))
          throw std::runtime_error("Invalid BLAST HSP metrics");
        intervals.push_back({std::min(from, to), std::max(from, to)});
        if (score > bestScore) {
          best = s;
          bestScore = score;
        }
      }
      std::sort(intervals.begin(), intervals.end());
      int covered = 0, start = 0, end = -1;
      for (auto range : intervals) {
        if (range.first > end + 1) {
          if (end >= start)
            covered += end - start + 1;
          start = range.first;
          end = range.second;
        } else
          end = std::max(end, range.second);
      }
      if (end >= start)
        covered += end - start + 1;
      hit.hsps = h["hsps"].toArray().size();
      hit.length = best["align_len"].toInt();
      hit.identity = 100.0 * best["identity"].toInt() / hit.length;
      hit.coverage = 100.0 * covered / q->sequence.size();
      hit.mismatches =
          hit.length - best["identity"].toInt() - best["gaps"].toInt();
      hit.bits = bestScore;
      hit.evalue = best["evalue"].toDouble();
      hit.qstart = best["query_from"].toInt();
      hit.qend = best["query_to"].toInt();
      hit.hstart = best["hit_from"].toInt();
      hit.hend = best["hit_to"].toInt();
      if (hit.accession.isEmpty() || hit.hstart < 1 || hit.hend < 1)
        throw std::runtime_error("Missing accession or coordinates");
      result.push_back(hit);
      if (result.size() >= 500000)
        throw std::length_error("Too many BLAST hits");
    }
  }
  if (seen.size() != queries.size())
    throw std::runtime_error("Incomplete BLAST reports; queries omitted");
  return result;
}
void enrich_blast(std::vector<BlastHit> &hits, const QByteArray &bytes) {
  if (bytes.size() > 32 * 1024 * 1024)
    throw std::length_error("Metadata response too large");
  QXmlStreamReader x(bytes);
  struct Record {
    QString accession, species, taxonomy, identifier, authors, date;
  };
  std::vector<Record> records;
  Record record;
  QString qualifier;
  bool inReference = false;
  QStringList authors;
  QString journal, fallbackAuthors;
  while (!x.atEnd()) {
    x.readNext();
    const auto tag = x.name().toString();
    if (x.isStartElement()) {
      if (tag == "ERROR")
        throw std::runtime_error(x.readElementText().toStdString());
      if (tag == "GBSeq") {
        record = {};

        fallbackAuthors.clear();
      } else if (tag == "GBSeq_accession-version" ||
                 tag == "GBSeq_primary-accession") {
        auto s = x.readElementText();
        if (record.accession.isEmpty() || tag == "GBSeq_accession-version")
          record.accession = s;
      } else if (tag == "GBSeq_organism")
        record.species = x.readElementText();
      else if (tag == "GBSeq_create-date")
        record.date = x.readElementText();
      else if (tag == "GBSeq_taxonomy")
        record.taxonomy = x.readElementText();
      else if (tag == "GBQualifier_name")
        qualifier = x.readElementText();
      else if (tag == "GBQualifier_value") {
        auto value = x.readElementText();
        if (QStringList{"culture_collection", "isolate", "specimen_voucher",
                        "clone", "strain"}
                .contains(qualifier)) {
          if (!record.identifier.isEmpty())
            record.identifier += "; ";
          record.identifier += qualifier + ": " + value;
        }
      } else if (tag == "GBReference") {
        inReference = true;
        authors.clear();
        journal.clear();
      } else if (tag == "GBAuthor" && inReference)
        authors << x.readElementText();
      else if (tag == "GBReference_journal" && inReference)
        journal = x.readElementText();
    } else if (x.isEndElement()) {
      if (tag == "GBReference") {
        inReference = false;
        if (fallbackAuthors.isEmpty())
          fallbackAuthors = authors.join("; ");
        if (journal.startsWith("Submitted", Qt::CaseInsensitive)) {
          record.authors = authors.join("; ");
        }
      }
      if (tag == "GBSeq") {

        records.push_back(record);
      }
    }
  }
  if (x.hasError() || records.empty())
    throw std::runtime_error("Malformed or empty GenBank metadata XML");
  for (auto &h : hits)
    for (const auto &r : records)
      if (h.accession == r.accession ||
          h.accession.section('.', 0, 0) == r.accession.section('.', 0, 0)) {
        h.species = r.species;
        h.identifier = r.identifier;
        h.taxonomy = r.taxonomy;
        h.authors = r.authors;
        h.date = r.date;
        break;
      }
}
QStringList blast_databases(bool protein) {
  return protein ? QStringList{"nr", "swissprot", "refseq_protein", "pdb"}
                 : QStringList{"core_nt",        "nt",  "refseq_rna",
                               "refseq_genomes", "wgs", "tsa_nt"};
}
QStringList blast_columns(bool local) {
  if (local)
    return {"Query number",
            "Query sequence",
            "Hit number",
            "Identity (%)",
            "Subject sequence name",
            "Query cover (%)",
            "HSP count",
            "E value",
            "Alignment length",
            "Mismatches",
            "Query start",
            "Query end",
            "Hit start",
            "Hit end",
            "Bit score"};
  return {"Query number",
          "Query sequence",
          "Hit number",
          "Identity (%)",
          "GenBank accession",
          "Species",
          "Source identifiers",
          "Query cover (%)",
          "HSP count",
          "E value",
          "Alignment length",
          "Mismatches",
          "Query start",
          "Query end",
          "Hit start",
          "Hit end",
          "Bit score",
          "Hit title",
          "Taxonomy",
          "Authors",
          "Sequence release date"};
}
bool blast_numeric_column(int c, bool local) {
  if (local)
    return c == 0 || c == 2 || c == 3 || c >= 5;
  return c == 0 || c == 2 || c == 3 || (c >= 7 && c <= 16);
}
BlastTable blast_table(const std::vector<BlastQuery> &queries,
                       const std::vector<BlastHit> &hits,
                       const BlastOptions &o) {
  BlastTable rows;
  std::map<QString, std::vector<const BlastHit *>> byQuery;
  std::set<QString> known;
  for (const auto &q : queries)
    known.insert(q.id);
  for (const auto &h : hits) {
    if (!known.count(h.queryId))
      throw std::logic_error("Hit refers to an unknown query");
    if (h.identity + 1e-9 >= o.identity && h.coverage + 1e-9 >= o.coverage &&
        h.evalue <= o.expect)
      byQuery[h.queryId].push_back(&h);
  }
  for (auto &[id, group] : byQuery)
    std::stable_sort(group.begin(), group.end(),
                     [](const auto *a, const auto *b) {
                       if (a->identity != b->identity)
                         return a->identity > b->identity;
                       if (a->bits != b->bits)
                         return a->bits > b->bits;
                       return a->accession < b->accession;
                     });
  struct Alias {
    int number;
    QString name, id;
  };
  std::vector<Alias> aliases;
  int fallback = 0;
  for (const auto &q : queries)
    for (int i = 0; i < q.names.size(); ++i) {
      ++fallback;
      aliases.push_back({q.numbers.size() == std::size_t(q.names.size())
                             ? q.numbers[i]
                             : fallback,
                         q.names[i], q.id});
    }
  std::stable_sort(
      aliases.begin(), aliases.end(),
      [](const auto &a, const auto &b) { return a.number < b.number; });
  for (const auto &alias : aliases) {
    int number = 0;
    for (const auto *hp : byQuery[alias.id]) {
      const auto &h = *hp;
      if (rows.size() >= 500000)
        throw std::length_error("Expanded result table exceeds 500,000 rows");
      rows.push_back({QString::number(alias.number),
                      alias.name,
                      QString::number(++number),
                      QString::number(h.identity, 'f', 2),
                      h.accession,
                      h.species,
                      h.identifier,
                      QString::number(h.coverage, 'f', 2),
                      QString::number(h.hsps),
                      QString::number(h.evalue, 'g', 12),
                      QString::number(h.length),
                      QString::number(h.mismatches),
                      QString::number(h.qstart),
                      QString::number(h.qend),
                      QString::number(h.hstart),
                      QString::number(h.hend),
                      QString::number(h.bits, 'g', 12),
                      h.title,
                      h.taxonomy,
                      h.authors,
                      h.date});
    }
  }
  if (o.local)
    for (auto &row : rows) {
      QStringList projected;
      for (int c : {0, 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16})
        projected << row[c];
      row = std::move(projected);
    }
  return rows;
}
QByteArray blast_tsv(const BlastTable &rows, bool local) {
  QString out = blast_columns(local).join('\t') + '\n';
  for (const auto &row : rows) {
    QStringList safe;
    for (auto s : row) {
      s.replace('\t', ' ');
      s.replace('\r', ' ');
      s.replace('\n', ' ');
      const auto stripped = s.trimmed();
      if (!stripped.isEmpty() && QString("=+-@").contains(stripped[0]))
        s.prepend('\'');
      safe << s;
    }
    out += safe.join('\t') + '\n';
    if (out.size() > 32 * 1024 * 1024)
      throw std::length_error("Export exceeds 64 MiB");
  }
  return out.toUtf8();
}
namespace {
void u16(QByteArray &b, unsigned v) {
  b += char(v);
  b += char(v >> 8);
}
void u32(QByteArray &b, unsigned v) {
  u16(b, v);
  u16(b, v >> 16);
}
unsigned crc(const QByteArray &b) {
  unsigned c = 0xffffffff;
  for (unsigned char byte : b) {
    c ^= byte;
    for (int i = 0; i < 8; ++i)
      c = (c >> 1) ^ ((c & 1U) ? 0xedb88320U : 0U);
  }
  return ~c;
}
unsigned checked_zip_size(qsizetype size, unsigned limit = 0xffffffffU) {
  if (size < 0 || quint64(size) > limit)
    throw std::length_error("ZIP size is out of range");
  return static_cast<unsigned>(size);
}
QByteArray zip(const std::vector<std::pair<QByteArray, QByteArray>> &files) {
  QByteArray out, central;
  for (const auto &[name, data] : files) {
    const auto offset = out.size();
    const auto sum = crc(data);
    u32(out, 0x04034b50);
    u16(out, 20);
    u16(out, 0);
    u16(out, 0);
    u16(out, 0);
    u16(out, 33);
    u32(out, sum);
    u32(out, checked_zip_size(data.size()));
    u32(out, checked_zip_size(data.size()));
    u16(out, checked_zip_size(name.size(), 65535));
    u16(out, 0);
    out += name;
    out += data;
    u32(central, 0x02014b50);
    u16(central, 20);
    u16(central, 20);
    u16(central, 0);
    u16(central, 0);
    u16(central, 0);
    u16(central, 33);
    u32(central, sum);
    u32(central, checked_zip_size(data.size()));
    u32(central, checked_zip_size(data.size()));
    u16(central, checked_zip_size(name.size(), 65535));
    for (int i = 0; i < 4; ++i)
      u16(central, 0);
    u32(central, 0);
    u32(central, checked_zip_size(offset));
    central += name;
  }
  auto start = out.size();
  out += central;
  u32(out, 0x06054b50);
  u16(out, 0);
  u16(out, 0);
  u16(out, checked_zip_size(static_cast<qsizetype>(files.size()), 65535));
  u16(out, checked_zip_size(static_cast<qsizetype>(files.size()), 65535));
  u32(out, checked_zip_size(central.size()));
  u32(out, checked_zip_size(start));
  u16(out, 0);
  return out;
}
} // namespace
QByteArray blast_xlsx(const BlastTable &rows, bool local, bool protein) {
  QByteArray sheet, relationships;
  QXmlStreamWriter x(&sheet), rels(&relationships);
  const QString relationshipNs =
      "http://schemas.openxmlformats.org/officeDocument/2006/relationships";
  x.writeStartDocument();
  x.writeStartElement("worksheet");
  x.writeDefaultNamespace(
      "http://schemas.openxmlformats.org/spreadsheetml/2006/main");
  x.writeNamespace(relationshipNs, "r");
  x.writeStartElement("sheetData");
  auto write = [&](const QStringList &row, bool header) {
    x.writeStartElement("row");
    for (int column = 0; column < row.size(); ++column) {
      x.writeStartElement("c");
      if (!header && !local && column == 4)
        x.writeAttribute("s", "1");
      if (!header && blast_numeric_column(column, local))
        x.writeTextElement("v", row[column]);
      else {
        x.writeAttribute("t", "inlineStr");
        x.writeStartElement("is");
        x.writeTextElement("t", row[column]);
        x.writeEndElement();
      }
      x.writeEndElement();
    }
    x.writeEndElement();
    if (sheet.size() > 64 * 1024 * 1024)
      throw std::length_error("XLSX exceeds 64 MiB");
  };
  write(blast_columns(local), true);
  for (const auto &row : rows)
    write(row, false);
  x.writeEndElement();
  rels.writeStartDocument();
  rels.writeStartElement("Relationships");
  rels.writeDefaultNamespace(
      "http://schemas.openxmlformats.org/package/2006/relationships");
  if (!local) {
    x.writeStartElement("hyperlinks");
    int n = 0;
    for (const auto &row : rows) {
      const auto id = "rId" + QString::number(++n);
      x.writeEmptyElement("hyperlink");
      x.writeAttribute("ref", "E" + QString::number(n + 1));
      x.writeAttribute(relationshipNs, "id", id);
      rels.writeEmptyElement("Relationship");
      rels.writeAttribute("Id", id);
      rels.writeAttribute("Type", relationshipNs + "/hyperlink");
      rels.writeAttribute(
          "Target", "https://www.ncbi.nlm.nih.gov/" +
                        QString(protein ? "protein/" : "nuccore/") +
                        QString::fromLatin1(QUrl::toPercentEncoding(row[4])));
      rels.writeAttribute("TargetMode", "External");
    }
    x.writeEndElement();
  }
  rels.writeEndElement();
  rels.writeEndDocument();
  x.writeEndElement();
  x.writeEndDocument();
  return zip(
      {{"[Content_Types].xml",
        "<Types "
        "xmlns=\"http://schemas.openxmlformats.org/package/2006/"
        "content-types\"><Default Extension=\"rels\" "
        "ContentType=\"application/"
        "vnd.openxmlformats-package.relationships+xml\"/><Default "
        "Extension=\"xml\" ContentType=\"application/xml\"/><Override "
        "PartName=\"/xl/workbook.xml\" "
        "ContentType=\"application/"
        "vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/"
        "><Override PartName=\"/xl/worksheets/sheet1.xml\" "
        "ContentType=\"application/"
        "vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/"
        "><Override PartName=\"/xl/styles.xml\" "
        "ContentType=\"application/"
        "vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/></"
        "Types>"},
       {"_rels/.rels", "<Relationships "
                       "xmlns=\"http://schemas.openxmlformats.org/package/2006/"
                       "relationships\"><Relationship Id=\"rId1\" "
                       "Type=\"http://schemas.openxmlformats.org/"
                       "officeDocument/2006/relationships/officeDocument\" "
                       "Target=\"xl/workbook.xml\"/></Relationships>"},
       {"xl/workbook.xml",
        "<workbook "
        "xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/"
        "relationships\"><sheets><sheet name=\"BLAST hits\" sheetId=\"1\" "
        "r:id=\"rId1\"/></sheets></workbook>"},
       {"xl/_rels/workbook.xml.rels",
        "<Relationships "
        "xmlns=\"http://schemas.openxmlformats.org/package/2006/"
        "relationships\"><Relationship Id=\"rId1\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
        "relationships/worksheet\" "
        "Target=\"worksheets/sheet1.xml\"/><Relationship Id=\"rId2\" "
        "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/"
        "relationships/styles\" Target=\"styles.xml\"/></Relationships>"},
       {"xl/styles.xml",
        R"STYLE(<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><fonts count="2"><font><sz val="11"/><name val="Calibri"/></font><font><sz val="11"/><color rgb="FF0563C1"/><name val="Calibri"/><u/></font></fonts><fills count="2"><fill><patternFill patternType="none"/></fill><fill><patternFill patternType="gray125"/></fill></fills><borders count="1"><border/></borders><cellStyleXfs count="1"><xf/></cellStyleXfs><cellXfs count="2"><xf fontId="0" fillId="0" borderId="0"/><xf fontId="1" fillId="0" borderId="0" applyFont="1"/></cellXfs><cellStyles count="1"><cellStyle name="Normal" xfId="0" builtinId="0"/></cellStyles></styleSheet>)STYLE"},
       {"xl/worksheets/sheet1.xml", sheet},
       {"xl/worksheets/_rels/sheet1.xml.rels", relationships}});
}
} // namespace kleiotide
