#include "gui/table_export.hpp"
#include "core/version.hpp"
#include "gui/file_output.hpp"
#include <QAbstractItemModel>
#include <QFileInfo>
#include <QXmlStreamWriter>
#include <cmath>
#include <stdexcept>
namespace kleiotide {
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

constexpr auto ns = "http://schemas.openxmlformats.org/spreadsheetml/2006/main";
constexpr auto relns =
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships";
QString reference(int col, qsizetype row) {
  QString out;
  for (++col; col; col = (col - 1) / 26)
    out.prepend(QChar('A' + (col - 1) % 26));
  return out + QString::number(row + 1);
}
QString field(QString v, QChar sep, bool protect) {
  if (protect && !v.trimmed().isEmpty() &&
      QString("=+-@").contains(v.trimmed()[0]))
    v.prepend('\'');
  if (v.contains(sep) || v.contains('"') || v.contains('\n') ||
      v.contains('\r')) {
    v.replace("\"", "\"\"");
    v = '"' + v + '"';
  }
  return v;
}
bool numeric(const TableSheet &sheet, qsizetype row, int col) {
  bool ok = false;
  const auto n = sheet.rows[std::size_t(row)][col].toDouble(&ok);
  return row > 0 && sheet.numericColumns.contains(col) && ok &&
         std::isfinite(n);
}
} // namespace
QString table_format_filters() {
  return "Excel workbook (*.xlsx);;Tab-separated table "
         "(*.tsv);;Comma-separated table (*.csv)";
}
QString choose_table_path(QWidget *parent, const QString &initial,
                          const Document *source) {
  return choose_export_path(parent, "Save table", initial,
                            table_format_filters(), OutputFormat::Table,
                            source);
}
QByteArray report_xlsx(const TableReport &sheets,
                       const std::function<bool()> &cancel) {
  if (sheets.empty())
    throw std::invalid_argument("No tables to save.");
  std::vector<std::pair<QByteArray, QByteArray>> files;
  QByteArray types, book, bookRels;
  QXmlStreamWriter ct(&types), wb(&book), wr(&bookRels);
  ct.writeStartElement("Types");
  ct.writeDefaultNamespace(
      "http://schemas.openxmlformats.org/package/2006/content-types");
  auto overridePart = [&](QString path, QString type) {
    ct.writeEmptyElement("Override");
    ct.writeAttribute("PartName", path);
    ct.writeAttribute("ContentType", type);
  };
  for (const auto &ext : QStringList{"rels", "xml"}) {
    ct.writeEmptyElement("Default");
    ct.writeAttribute("Extension", ext);
    ct.writeAttribute(
        "ContentType",
        ext == "xml"
            ? "application/xml"
            : "application/vnd.openxmlformats-package.relationships+xml");
  }
  overridePart(
      "/xl/workbook.xml",
      "application/"
      "vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml");
  overridePart(
      "/xl/styles.xml",
      "application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml");
  wb.writeStartElement("workbook");
  wb.writeDefaultNamespace(ns);
  wb.writeNamespace(relns, "r");
  wb.writeStartElement("sheets");
  wr.writeStartElement("Relationships");
  wr.writeDefaultNamespace(
      "http://schemas.openxmlformats.org/package/2006/relationships");
  QSet<QString> names;
  qsizetype bytes = 0;
  int id = 0;
  for (const auto &sheet : sheets) {
    if (sheet.rows.size() > 1048576)
      throw std::length_error("Excel worksheets support at most 1,048,576 "
                              "rows. Save TSV or CSV instead.");
    QString name = sheet.name.left(31);
    for (auto c : QString("[]:*?/\\"))
      name.replace(c, '_');
    if (name.isEmpty())
      name = "Table";
    const auto original = name;
    int duplicate = 1;
    while (names.contains(name.toLower()))
      name = original.left(26) + " " + QString::number(++duplicate);
    names.insert(name.toLower());
    const auto index = QString::number(++id), rid = "rId" + index;
    wb.writeEmptyElement("sheet");
    wb.writeAttribute("name", name);
    wb.writeAttribute("sheetId", index);
    wb.writeAttribute(relns, "id", rid);
    wr.writeEmptyElement("Relationship");
    wr.writeAttribute("Id", rid);
    wr.writeAttribute("Type", QString(relns) + "/worksheet");
    wr.writeAttribute("Target", "worksheets/sheet" + index + ".xml");
    overridePart(
        "/xl/worksheets/sheet" + index + ".xml",
        "application/"
        "vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml");
    QByteArray data, relationships;
    QXmlStreamWriter x(&data), rels(&relationships);
    x.writeStartElement("worksheet");
    x.writeDefaultNamespace(ns);
    x.writeNamespace(relns, "r");
    x.writeStartElement("sheetViews");
    x.writeStartElement("sheetView");
    x.writeAttribute("workbookViewId", "0");
    x.writeEmptyElement("pane");
    x.writeAttribute("ySplit", "1");
    x.writeAttribute("topLeftCell", "A2");
    x.writeAttribute("state", "frozen");
    x.writeEndElement();
    x.writeEndElement();
    x.writeStartElement("sheetData");
    for (qsizetype row = 0; row < qsizetype(sheet.rows.size()); ++row) {
      if (cancel && cancel())
        throw std::runtime_error("Table saving cancelled.");
      const auto &cells = sheet.rows[std::size_t(row)];
      if (cells.size() > 16384)
        throw std::length_error("Excel worksheets support at most 16,384 "
                                "columns. Save TSV or CSV instead.");
      x.writeStartElement("row");
      x.writeAttribute("r", QString::number(row + 1));
      for (int col = 0; col < cells.size(); ++col) {
        if (cells[col].size() > 32767)
          throw std::length_error("An Excel cell exceeds 32,767 characters. "
                                  "Save TSV or CSV instead.");
        const auto ref = reference(col, row);
        x.writeStartElement("c");
        x.writeAttribute("r", ref);
        if (sheet.links.contains(ref))
          x.writeAttribute("s", "1");
        else if (!row)
          x.writeAttribute("s", "2");
        if (numeric(sheet, row, col))
          x.writeTextElement("v", cells[col]);
        else {
          x.writeAttribute("t", "inlineStr");
          x.writeStartElement("is");
          x.writeStartElement("t");
          x.writeAttribute("xml:space", "preserve");
          x.writeCharacters(cells[col]);
          x.writeEndElement();
          x.writeEndElement();
        }
        x.writeEndElement();
      }
      x.writeEndElement();
      if (bytes + data.size() > 256 * 1024 * 1024)
        throw std::length_error("Workbook exceeds the 256 MiB export memory "
                                "limit. Save TSV or CSV instead.");
    }
    x.writeEndElement();
    rels.writeStartElement("Relationships");
    rels.writeDefaultNamespace(
        "http://schemas.openxmlformats.org/package/2006/relationships");
    if (!sheet.links.isEmpty()) {
      x.writeStartElement("hyperlinks");
      int link = 0;
      for (auto it = sheet.links.cbegin(); it != sheet.links.cend(); ++it) {
        const auto linkid = "rId" + QString::number(++link);
        x.writeEmptyElement("hyperlink");
        x.writeAttribute("ref", it.key());
        x.writeAttribute(relns, "id", linkid);
        rels.writeEmptyElement("Relationship");
        rels.writeAttribute("Id", linkid);
        rels.writeAttribute("Type", QString(relns) + "/hyperlink");
        rels.writeAttribute("Target", it.value().toString(QUrl::FullyEncoded));
        rels.writeAttribute("TargetMode", "External");
      }
      x.writeEndElement();
    }
    rels.writeEndElement();
    x.writeEndElement();
    bytes += data.size() + relationships.size();
    files.push_back({("xl/worksheets/sheet" + index + ".xml").toUtf8(), data});
    files.push_back(
        {("xl/worksheets/_rels/sheet" + index + ".xml.rels").toUtf8(),
         relationships});
  }
  wb.writeEndElement();
  wb.writeEndElement();
  wr.writeEmptyElement("Relationship");
  wr.writeAttribute("Id", "styles");
  wr.writeAttribute("Type", QString(relns) + "/styles");
  wr.writeAttribute("Target", "styles.xml");
  wr.writeEndElement();
  ct.writeEndElement();
  files.push_back({"[Content_Types].xml", types});
  files.push_back({"xl/workbook.xml", book});
  files.push_back({"xl/_rels/workbook.xml.rels", bookRels});
  files.push_back(
      {"_rels/.rels",
       R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/></Relationships>)"});
  files.push_back(
      {"xl/styles.xml",
       R"(<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><fonts count="3"><font><sz val="11"/><name val="Calibri"/></font><font><sz val="11"/><color rgb="FF0563C1"/><name val="Calibri"/><u/></font><font><sz val="11"/><name val="Calibri"/><b/></font></fonts><fills count="2"><fill><patternFill patternType="none"/></fill><fill><patternFill patternType="gray125"/></fill></fills><borders count="1"><border/></borders><cellStyleXfs count="1"><xf/></cellStyleXfs><cellXfs count="3"><xf fontId="0" fillId="0" borderId="0"/><xf fontId="1" fillId="0" borderId="0" applyFont="1"/><xf fontId="2" fillId="0" borderId="0" applyFont="1"/></cellXfs><cellStyles count="1"><cellStyle name="Normal" xfId="0" builtinId="0"/></cellStyles></styleSheet>)"});
  return zip(files);
}
QByteArray report_delimited(const TableReport &sheets, QChar sep) {
  QString text;
  for (const auto &sheet : sheets) {
    if (!text.isEmpty())
      text += '\n';
    if (sheets.size() > 1)
      text += "# " + sheet.name + '\n';
    for (qsizetype row = 0; row < qsizetype(sheet.rows.size()); ++row) {
      QStringList cells;
      const auto &values = sheet.rows[std::size_t(row)];
      for (int col = 0; col < values.size(); ++col)
        cells << field(values[col], sep, !numeric(sheet, row, col));
      text += cells.join(sep) + '\n';
      if (text.size() > 128 * 1024 * 1024)
        throw std::length_error(
            "Table exceeds the 256 MiB export memory limit.");
    }
  }
  return text.toUtf8();
}
QByteArray report_bytes(const QString &path, const TableReport &sheets,
                        const std::function<bool()> &cancel) {
  const auto ext = QFileInfo(path).suffix().toLower();
  return ext == "xlsx" ? report_xlsx(sheets, cancel)
                       : report_delimited(sheets, ext == "csv" ? ',' : '\t');
}
QString copy_model_table(const QAbstractItemModel *model, bool rowLabels) {
  if (!model)
    return {};
  std::vector<int> columns;
  QStringList header;
  if (rowLabels)
    header << "Sequence";
  for (int c = 0; c < model->columnCount(); ++c) {
    const auto name = model->headerData(c, Qt::Horizontal).toString();
    if (name == "Add AB1…")
      continue;
    columns.push_back(c);
    header << name;
  }
  QString text;
  auto append = [&](const QStringList &row) {
    QStringList cells;
    for (const auto &value : row)
      cells << field(value, '\t', false);
    text += cells.join('\t') + '\n';
    if (text.size() > 32 * 1024 * 1024)
      throw std::length_error("Table exceeds the 64 MiB clipboard limit. Save "
                              "it to a file instead.");
  };
  append(header);
  for (int r = 0; r < model->rowCount(); ++r) {
    QStringList row;
    if (rowLabels)
      row << model->headerData(r, Qt::Vertical).toString();
    for (int c : columns)
      row << model->index(r, c).data().toString();
    append(row);
  }
  if (text.toUtf8().size() > 64 * 1024 * 1024)
    throw std::length_error("Table exceeds the 64 MiB clipboard limit.");
  return text;
}

TableReport grouping_workbook(const Document &d,
                              const IdentityGroupPlan &plan) {
  const auto &o = plan.options;
  TableSheet methods{
      "Grouping settings",
      {{"Setting", "Value"},
       {"Kleiotide version", version},
       {"Identity calculation", o.alignment == IdentityAlignment::Existing
                                    ? "Existing alignment"
                                    : "Pairwise centroid search"},
       {"Minimum identity (%)", QString::number(o.minimumIdentity)},
       {"Minimum overlap (%)", QString::number(o.minimumOverlap)},
       {"Maximum centroid candidates (pairwise only)",
        QString::number(o.maximumCandidates)},
       {"Scope",
        o.selectedOnly ? "Selected sequence snapshot" : "All sequences"},
       {"Group name prefix", QString::fromStdString(o.prefix)},
       {"Sequence order",
        "Descending trimmed ungapped length; ties follow original row order"},
       {"Assignment",
        "First qualifying centroid; otherwise create a new group"},
       {"Identity", "Exact concrete matches / compared overlap columns; "
                    "unknown calls excluded from identity"},
       {"Overlap",
        "Residue-to-residue columns / shorter trimmed ungapped sequence"},
       {"End gaps",
        "Free terminal overhangs; internal indels count in the denominator"},
       {"Candidate search", "Word-ranked candidate limit can omit qualifying "
                            "centroids; 0 means exhaustive"}}};
  if (plan.source != &d || plan.revision != d.state_id())
    throw std::invalid_argument(
        "The alignment changed; refresh the grouping preview.");
  TableSheet result{
      "Grouping results",
      {{"Sequence", "Proposed group", "Group centroid", "Identity (%)",
        "Overlap (%)", "Result", "Sequence ID", "Centroid ID"}},
      {3, 4, 6, 7}};
  QMap<GroupId, QString> names;
  for (const auto &g : plan.groups)
    names[g.id] = QString::fromStdString(g.name);
  for (const auto &m : plan.members)
    result.rows.push_back(
        {QString::fromStdString(d.sequence(m.id).name),
         m.group ? names[m.group] : "Unchanged",
         m.centroid ? QString::fromStdString(d.sequence(m.centroid).name)
                    : QString{},
         m.group ? QString::number(m.pair.identity(), 'f', 2) : QString{},
         m.group ? QString::number(m.overlap, 'f', 2) : QString{},
         !m.skipped.empty()   ? QString::fromStdString(m.skipped)
         : m.id == m.centroid ? "New group centroid"
                              : "Group member",
         QString::number(m.id),
         m.centroid ? QString::number(m.centroid) : QString{}});
  return {methods, result};
}
TableReport distance_workbook(const DistanceResult &r) {
  TableSheet methods{
      "Calculation methods",
      {{"Method / setting", "Value"},
       {"Kleiotide version", version},
       {"Molecule", r.options.scope.molecule == RowKind::Protein
                        ? "Protein"
                        : "Nucleotide"},
       {"Distance method",
        QString::fromStdString(distance_method_name(r.options.method))},
       {"Deletion", r.options.deletion == DistanceDeletion::Pairwise
                        ? "Pairwise"
                        : "Complete"},
       {"Minimum comparable sites", QString::number(r.options.minimumSites)},
       {"Rows", r.options.scope.selectedRows ? "Selected sequence snapshot"
                                             : "All matching rows"},
       {"Columns (1-based inclusive)",
        QString::number(r.begin + 1) + "–" + QString::number(r.end)},
       {"Include ungrouped", r.options.includeUngrouped ? "Yes" : "No"},
       {"Complete-deletion sites", QString::number(r.completeSites)},
       {"Pair comparison", "Definite A/C/G/T (U=T), or 20 standard amino "
                           "acids; other states and gaps excluded"},
       {"Mean", "Unweighted mean of valid pair distances; N/A pairs excluded "
                "and counted"},
       {"Sample SD", "sqrt(sum((d-mean)^2)/(m-1)); unavailable for m < 2"},
       {"Naive SE", "SD/sqrt(m); shared-sequence pairs are dependent; not "
                    "bootstrap uncertainty"}}};
  TableReport report{methods};
  TableSheet main{r.report == DistanceReport::Matrix ? "Distance matrix"
                  : r.report == DistanceReport::Overall
                      ? "Overall mean distance"
                  : r.report == DistanceReport::WithinGroups
                      ? "Within-group mean distance"
                      : "Between-group mean distance"};
  auto num = [](double v) {
    return QString::fromStdString(distance_number(v));
  };
  if (r.report == DistanceReport::Matrix) {
    TableSheet counts{"Comparable site counts"},
        diffs{"Observed difference counts"},
        unavailable{"Unavailable estimates",
                    {{"Sequence 1", "Sequence 2", "Reason"}}};
    QStringList header{"Sequence"};
    for (const auto &row : r.rows)
      header << QString::fromStdString(row.name);
    main.rows.push_back(header);
    counts.rows.push_back(header);
    diffs.rows.push_back(header);
    for (std::size_t i = 0; i < r.rows.size(); ++i) {
      QStringList a{QString::fromStdString(r.rows[i].name)}, b = a, c = a;
      for (std::size_t j = 0; j < r.rows.size(); ++j) {
        const auto &pair = r.pair(i, j);
        a << (pair.valid() ? num(pair.value) : "N/A");
        b << QString::number(pair.comparable);
        c << (pair.comparable ? QString::number(pair.differences) : "N/A");
        if (j <= i && !pair.valid())
          unavailable.rows.push_back(
              {QString::fromStdString(r.rows[j].name),
               QString::fromStdString(r.rows[i].name),
               QString::fromStdString(distance_failure_name(pair.failure))});
      }
      main.rows.push_back(a);
      counts.rows.push_back(b);
      diffs.rows.push_back(c);
    }
    for (int c = 1; c < header.size(); ++c) {
      main.numericColumns.insert(c);
      counts.numericColumns.insert(c);
      diffs.numericColumns.insert(c);
    }
    report.push_back(main);
    report.push_back(counts);
    report.push_back(diffs);
    report.push_back(unavailable);
  } else {
    main.rows.push_back({"Group 1", "Group 2", "Mean distance", "SD (pairs)",
                         "SE (naive)", "Valid pairs", "Excluded pairs",
                         "No overlap", "Below minimum", "Saturated",
                         "Mean comparable sites", "Min comparable",
                         "Max comparable"});
    for (int c = 2; c < 13; ++c)
      main.numericColumns.insert(c);
    for (const auto &m : r.means) {
      const bool overall = r.report == DistanceReport::Overall;
      main.rows.push_back(
          {overall ? "Overall" : QString::fromStdString(r.groups[m.first].name),
           overall ? "Overall"
                   : QString::fromStdString(r.groups[m.second].name),
           m.valid ? num(m.value()) : "N/A",
           m.sample_sd() ? num(*m.sample_sd()) : "N/A",
           m.naive_se() ? num(*m.naive_se()) : "N/A", QString::number(m.valid),
           QString::number(m.excluded()), QString::number(m.noOverlap),
           QString::number(m.belowMinimum), QString::number(m.saturated),
           m.valid ? num(double(m.comparableSum / m.valid)) : "N/A",
           m.valid ? QString::number(m.minimumComparable) : "N/A",
           m.valid ? QString::number(m.maximumComparable) : "N/A"});
    }
    report.push_back(main);
  }
  TableSheet members{
      "Sequences in analysis",
      {{"Sequence name", "Sequence ID", "Group name", "Group ID"}},
      {1, 3}},
      groups{"Groups in analysis",
             {{"Group name", "Group ID", "Eligible rows"}},
             {1, 2}};
  QMap<GroupId, QString> names;
  bool assigned = false;
  for (const auto &g : r.groups) {
    names[g.id] = QString::fromStdString(g.name);
    groups.rows.push_back(
        {names[g.id], QString::number(g.id), QString::number(g.rows)});
    assigned |= g.id != 0;
  }
  for (const auto &row : r.rows)
    members.rows.push_back({QString::fromStdString(row.name),
                            QString::number(row.id), names[row.group],
                            QString::number(row.group)});
  report.push_back(members);
  if (assigned || r.report == DistanceReport::WithinGroups ||
      r.report == DistanceReport::BetweenGroups)
    report.push_back(groups);
  return report;
}
} // namespace kleiotide
