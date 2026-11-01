#include "gui/table_export.hpp"
#include "gui/spart.hpp"
#include "gui/file_output.hpp"
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMap>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSet>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <stdexcept>
namespace kleiotide {
namespace {
[[noreturn]] void bad(const QString &m) {
  throw std::runtime_error(m.toStdString());
}
bool valid_name(const QString &s) {
  return QRegularExpression("^[A-Za-z0-9_]+$").match(s).hasMatch();
}
QString subset_id(QString value) {
  qsizetype first = 0;
  while (first + 1 < value.size() && value[first] == '0')
    ++first;
  return value.mid(first);
}
void sample(const QString &s) {
  if (!valid_name(s))
    bad("SPART sequence names may contain only ASCII letters, digits and "
        "underscores: " +
        s + ". Use Rename → Sanitize names, or export TSV.");
}
void partition_name(const QString &s) {
  if (s.trimmed().isEmpty() ||
      s.contains(QRegularExpression("[,/:;\\[\\]\\r\\n]")))
    bad("A partition name cannot be empty or contain , : / ; [ or ].");
}
std::vector<SpartPartition> parse_matrix(QString text) {
  text.replace("\r\n", "\n");
  text.replace('\r', '\n');
  bool comment = false;
  for (auto &c : text) {
    if (c == '[') {
      if (comment)
        bad("Nested SPART comments are invalid.");
      comment = true;
      c = ' ';
    } else if (c == ']') {
      if (!comment)
        bad("Unmatched SPART comment bracket.");
      comment = false;
      c = ' ';
    } else if (comment && c != '\n')
      c = ' ';
  }
  if (comment)
    bad("Unclosed SPART comment.");
  const auto start =
      QRegularExpression("\\bbegin\\s+spart\\s*;",
                         QRegularExpression::CaseInsensitiveOption)
          .match(text);
  if (!start.hasMatch())
    bad("Missing begin spart; block.");
  text = text.mid(start.capturedEnd());
  QStringList order;
  QMap<QString, QString> commands;
  bool ended = false;
  for (const auto &chunk : text.split(';')) {
    const auto command = chunk.trimmed();
    if (command.isEmpty())
      continue;
    if (command.compare("end", Qt::CaseInsensitive) == 0) {
      ended = true;
      break;
    }
    const int equal = command.indexOf('=');
    if (equal < 0)
      bad("Invalid SPART command.");
    auto key = command.left(equal).trimmed().toLower();
    if (commands.contains(key))
      bad("Duplicate SPART command: " + key);
    order << key;
    commands[key] = command.mid(equal + 1).trimmed();
  }
  if (!ended)
    bad("Missing end; in SPART block.");
  const QStringList required = {"project_name",  "date",
                                "n_spartitions", "n_individuals",
                                "n_subsets",     "individual_assignment"};
  for (int i = 0; i < required.size(); ++i)
    if (order.value(i) != required[i] || commands[required[i]].isEmpty())
      bad("Missing or out-of-order required SPART command: " + required[i]);
  const auto spec = commands["n_spartitions"];
  const auto colon = spec.indexOf(':');
  bool ok = false;
  int count = spec.left(colon).trimmed().toInt(&ok);
  if (colon < 0 || !ok || count < 1 || count > 10000)
    bad("Invalid number of partitions.");
  auto labels = spec.mid(colon + 1).split('/');
  if (labels.size() != count)
    bad("Partition count does not match names.");
  std::vector<SpartPartition> parts;
  QSet<QString> named;
  for (auto label : labels) {
    label = label.section(',', 0, 0).trimmed();
    partition_name(label);
    if (named.contains(label))
      bad("Duplicate partition name.");
    named.insert(label);
    parts.push_back({label, {}});
  }
  const auto individuals = commands["n_individuals"].split('/'),
             subsets = commands["n_subsets"].split('/');
  if (individuals.size() != count || subsets.size() != count)
    bad("Invalid partition count in SPART dimensions.");
  QMap<QString, QString> names;
  for (auto line : commands.value("kleiotide_group_names")
                       .split('\n', Qt::SkipEmptyParts)) {
    auto c = line.indexOf(':');
    if (c > 0) {
      auto id = line.left(c).trimmed();
      auto encoded = line.mid(c + 1).trimmed().toLatin1();
      auto decoded = QByteArray::fromBase64(
          encoded, QByteArray::AbortOnBase64DecodingErrors);
      if (decoded.isNull())
        bad("Invalid Kleiotide group label.");
      names[id] = QString::fromUtf8(decoded);
    }
  }
  std::vector<int> assigned(count);
  std::vector<QSet<QString>> used(count);
  QSet<QString> seen;
  for (const auto &line :
       commands["individual_assignment"].split('\n', Qt::SkipEmptyParts)) {
    const auto c = line.indexOf(':');
    if (c < 0)
      bad("Missing ':' in SPART assignment.");
    auto id = line.left(c).trimmed();
    sample(id);
    if (seen.contains(id))
      bad("Duplicate SPART individual: " + id);
    seen.insert(id);
    if (seen.size() > 100000)
      bad("SPART exceeds 100,000 individuals.");
    auto values = line.mid(c + 1).split('/');
    if (values.size() != count)
      bad("Assignment does not contain every partition.");
    for (int i = 0; i < count; ++i) {
      auto v = values[i].trimmed();
      if (v != "?" && !QRegularExpression("^[0-9]+$").match(v).hasMatch())
        bad("Invalid SPART subset number: " + v);
      if (v != "?") {
        v = subset_id(v);
        ++assigned[i];
        used[i].insert(v);
      }
      parts[i].assignments.push_back(
          {id, v == "?" ? QString{} : names.value(v, v)});
    }
  }
  for (int i = 0; i < count; ++i) {
    bool a = false, b = false;
    int n = individuals[i].trimmed().toInt(&a),
        s = subsets[i].section(':', 0, 0).trimmed().toInt(&b);
    if (!a || !b || n != assigned[i] || s != used[i].size())
      bad("SPART dimension counts do not match partition " + parts[i].name);
  }
  return parts;
}
std::vector<SpartPartition> parse_xml(const QByteArray &bytes) {
  QXmlStreamReader x(bytes);
  QStringList individuals;
  QSet<QString> declared, partitionNames;
  std::vector<SpartPartition> result;
  QMap<QString, QString> assignments;
  QSet<QString> labels;
  QString label;
  bool inIndividuals = false, inPartition = false, inSubset = false;
  bool project = false, date = false;
  while (!x.atEnd()) {
    x.readNext();
    const auto tag = x.name();
    if (x.isStartElement()) {
      if (tag == u"project_name") {
        project = !x.readElementText().isEmpty();
      } else if (tag == u"date") {
        date = !x.readElementText().isEmpty();
      } else if (tag == u"individuals")
        inIndividuals = true;
      else if (tag == u"spartition") {
        if (inPartition)
          bad("Nested SPART partitions.");
        auto name = x.attributes().value("label").toString();
        partition_name(name);
        if (partitionNames.contains(name))
          bad("Duplicate partition label.");
        partitionNames.insert(name);
        result.push_back({name, {}});
        if (result.size() > 10000)
          bad("Too many partitions.");
        inPartition = true;
        assignments.clear();
        labels.clear();
      } else if (tag == u"subset" && inPartition) {
        if (inSubset)
          bad("Nested SPART subsets.");
        auto numeric = x.attributes().value("label").toString();
        if (!QRegularExpression("^[0-9]+$").match(numeric).hasMatch() ||
            labels.contains(numeric))
          bad("Invalid or duplicate subset label.");
        numeric = subset_id(numeric);
        if (labels.contains(numeric))
          bad("Duplicate numeric subset label.");
        labels.insert(numeric);
        label = x.attributes().value("kleiotideName").toString();
        if (label.isEmpty())
          label = numeric;
        inSubset = true;
      } else if (tag == u"individual") {
        if (inIndividuals) {
          auto id = x.attributes().value("id").toString();
          sample(id);
          if (declared.contains(id))
            bad("Duplicate SPART individual.");
          declared.insert(id);
          individuals << id;
          if (individuals.size() > 100000)
            bad("Too many individuals.");
        } else if (inPartition && inSubset) {
          auto id = x.attributes().value("ref").toString();
          if (!declared.contains(id) || assignments.contains(id))
            bad("Unknown or multiply assigned SPART individual: " + id);
          assignments[id] = label;
        }
      }
    } else if (x.isEndElement()) {
      if (tag == u"individuals")
        inIndividuals = false;
      else if (tag == u"subset")
        inSubset = false;
      else if (tag == u"spartition") {
        for (const auto &id : individuals)
          result.back().assignments.push_back({id, assignments.value(id)});
        inPartition = false;
      }
    }
  }
  if (x.hasError() || !project || !date || result.empty() ||
      individuals.isEmpty())
    bad("Invalid or incomplete SPART XML: " + x.errorString());
  return result;
}
} // namespace
QString spart_sanitize(QString name) {
  name.replace(QRegularExpression("[^A-Za-z0-9_]"), "_");
  return name.isEmpty() ? QString("_") : name.toLower();
}
std::vector<SpartPartition> parse_spart(const QByteArray &bytes) {
  if (bytes.size() > 16 * 1024 * 1024)
    bad("SPART file exceeds 16 MiB.");
  auto text = QString::fromUtf8(bytes);
  if (text.startsWith(QChar(0xfeff)))
    text.remove(0, 1);
  return text.trimmed().startsWith('<') ? parse_xml(bytes) : parse_matrix(text);
}
QByteArray export_spart(const Document &d, bool xml, const QString &project,
                        const QString &partition) {
  partition_name(partition);
  if (project.trimmed().isEmpty() ||
      project.contains(QRegularExpression("[;\\[\\]\\r\\n]")))
    bad("Enter a project name without ; [ or ] or line breaks.");
  QSet<QString> seen;
  QMap<GroupId, int> groups;
  QMap<GroupId, QString> labels;
  int assigned = 0;
  for (const auto &row : d.rows()) {
    auto name = spart_sanitize(QString::fromStdString(row.name));
    sample(name);
    if (seen.contains(name))
      bad("SPART requires unique sequence names. Use Rename → Make names "
          "unique, or export TSV.");
    seen.insert(name);
    if (row.groupId) {
      ++assigned;
      if (!groups.contains(row.groupId)) {
        groups[row.groupId] = int(groups.size()) + 1;
        const auto *g = d.group(row.groupId);
        if (!g)
          bad("Missing group.");
        labels[row.groupId] = QString::fromStdString(g->name);
      }
    }
  }
  if (seen.isEmpty())
    bad("No sequences to export.");
  const auto date = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
  if (!xml) {
    QString out = "begin spart;\nProject_name = " + project +
                  ";\nDate = " + date + ";\nN_spartitions = 1: " + partition +
                  ";\nN_individuals = " + QString::number(assigned) +
                  ";\nN_subsets = " + QString::number(groups.size()) +
                  ";\nIndividual_assignment =\n";
    for (const auto &row : d.rows())
      out += spart_sanitize(QString::fromStdString(row.name)) + " : " +
             (row.groupId ? QString::number(groups[row.groupId]) : "?") + "\n";
    out += ";\nKleiotide_group_names =\n";
    for (auto it = groups.cbegin(); it != groups.cend(); ++it)
      out += QString::number(it.value()) + " : " +
             QString::fromLatin1(labels[it.key()].toUtf8().toBase64()) + "\n";
    out += ";\nend;\n";
    return out.toUtf8();
  }
  QByteArray bytes;
  QXmlStreamWriter x(&bytes);
  x.setAutoFormatting(true);
  x.writeStartDocument();
  x.writeStartElement("root");
  x.writeTextElement("project_name", project);
  x.writeTextElement("date", date);
  x.writeStartElement("individuals");
  for (const auto &row : d.rows()) {
    x.writeEmptyElement("individual");
    x.writeAttribute("id", spart_sanitize(QString::fromStdString(row.name)));
  }
  x.writeEndElement();
  x.writeStartElement("spartitions");
  x.writeStartElement("spartition");
  x.writeAttribute("label", partition);
  x.writeStartElement("subsets");
  for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
    x.writeStartElement("subset");
    x.writeAttribute("label", QString::number(it.value()));
    x.writeAttribute("kleiotideName", labels[it.key()]);
    for (const auto &row : d.rows())
      if (row.groupId == it.key()) {
        x.writeEmptyElement("individual");
        x.writeAttribute("ref",
                         spart_sanitize(QString::fromStdString(row.name)));
      }
    x.writeEndElement();
  }
  x.writeEndElement();
  x.writeEndElement();
  x.writeEndElement();
  x.writeEndElement();
  x.writeEndDocument();
  return bytes;
}
void export_sequence_groups(const Document &d, QWidget *parent) {
  QString filter;
  auto path =
      choose_output_filename(parent, "Export sequence groups", {},
                             "Matricial SPART (*.spart);;SPART XML "
                             "(*.spart.xml);;Excel workbook (*.xlsx);;Tab-separated table (*.tsv);;Comma-separated table (*.csv)",
                             &filter, QFileDialog::DontConfirmOverwrite);
  if (path.isEmpty())
    return;
  try {
    QByteArray bytes;
    const bool tsv = filter.contains("*.tsv")||filter.contains("*.csv")||filter.contains("*.xlsx"),
               xml = filter.contains("*.spart.xml");
    if (tsv) {
      TextTable table{{"Sequence name", "Group name"}};
      for (const auto &row : d.rows()) {
        auto *group = d.group(row.groupId);
        table.push_back(
            {QString::fromStdString(row.name),
             group ? QString::fromStdString(group->name) : QString{}});
      }
      if(filter.contains("*.xlsx"))bytes=report_bytes(path,{{"Sequence groups",table}});
      else if(filter.contains("*.csv")){QString text;for(const auto& row:table){QStringList cells;for(auto cell:row){cell.replace('"',"\"\"");cells<<'"'+cell+'"';}text+=cells.join(',')+'\n';}bytes=text.toUtf8();}
      else bytes=table_text(table).toUtf8();
    } else {
      bool ok = false;
      auto label = QInputDialog::getText(parent, "Export sequence groups",
                                         "Partition name", QLineEdit::Normal,
                                         "Kleiotide_groups", &ok);
      if (!ok)
        return;
      bytes = export_spart(d, xml, "Kleiotide", label);
    }
    path = approve_output_path(parent, path, OutputFormat::Table, &d);
    if (path.isEmpty())
      return;
    const auto error =
        write_export_atomic(path, OutputFormat::Table, &d, [&](auto &out) {
          out.write(bytes.constData(), bytes.size());
        });
    if (!error.isEmpty())
      bad(error);
  } catch (const std::exception &e) {
    QMessageBox::warning(parent, "Export sequence groups",
                         QString::fromUtf8(e.what()));
  }
}
} // namespace kleiotide
