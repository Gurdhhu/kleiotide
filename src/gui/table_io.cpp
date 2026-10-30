#include "gui/table_io.hpp"
#include "core/group_import.hpp"
#include "third_party/miniz/miniz_tinfl.h"
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMap>
#include <QMessageBox>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QtEndian>
#include <stdexcept>
namespace kleiotide {
namespace {
[[noreturn]] void bad(const QString &message) {
  throw std::runtime_error(message.toStdString());
}
void check_xml(const QXmlStreamReader &x) {
  if (x.hasError())
    bad("Invalid worksheet XML: " + x.errorString());
}
QByteArray zip_entry(const QByteArray &zip, const QByteArray &name) {
  auto u16 = [&](qsizetype at) -> quint16 {
    if (at < 0 || at > zip.size() - 2)
      bad("Truncated XLSX archive.");
    return quint16(quint8(zip.at(at))) |
           quint16(quint16(quint8(zip.at(at + 1))) << 8);
  };
  auto u32 = [&](qsizetype at) -> quint32 {
    if (at < 0 || at > zip.size() - 4)
      bad("Truncated XLSX archive.");
    return quint32(quint8(zip.at(at))) |
           (quint32(quint8(zip.at(at + 1))) << 8) |
           (quint32(quint8(zip.at(at + 2))) << 16) |
           (quint32(quint8(zip.at(at + 3))) << 24);
  };
  qsizetype end = -1;
  for (qsizetype at = zip.size() - 22;
       at >= std::max(qsizetype(0), zip.size() - 65557); --at)
    if (u32(at) == 0x06054b50 && at + 22 + u16(at + 20) == zip.size()) {
      end = at;
      break;
    }
  if (end < 0 || u16(end + 4) || u16(end + 6) || u16(end + 8) != u16(end + 10))
    bad("Unsupported or invalid XLSX ZIP archive.");
  qsizetype at = u32(end + 16);
  for (unsigned n = 0; n < u16(end + 10); ++n) {
    if (u32(at) != 0x02014b50)
      bad("Invalid XLSX directory.");
    const auto method = u16(at + 10), flags = u16(at + 8);
    const quint32 size = u32(at + 24), packed = u32(at + 20),
                  crc = u32(at + 16);
    const auto length = u16(at + 28);
    const auto local = u32(at + 42);
    if (at + 46 + length > zip.size())
      bad("Truncated XLSX directory.");
    if (zip.mid(at + 46, length) == name) {
      if (quint64(local) + 30 > quint64(zip.size()) || flags & 1 ||
          size > 64 * 1024 * 1024 || packed > 64 * 1024 * 1024 ||
          u32(local) != 0x04034b50)
        bad("Unsupported or oversized XLSX entry.");
      const quint64 offset =
          quint64(local) + 30 + u16(local + 26) + u16(local + 28);
      if (offset + packed > quint64(zip.size()))
        bad("Truncated XLSX entry.");
      QByteArray out;
      if (method == 0) {
        out = zip.mid(qsizetype(offset), packed);
        if (out.size() != size)
          bad("Invalid XLSX entry length.");
      } else if (method == 8) {
        out.resize(size);
        const auto decoded = tinfl_decompress_mem_to_mem(
            out.data(), size, zip.constData() + offset, packed, 0);
        if (decoded != size)
          bad("Could not decompress XLSX entry.");
      } else
        bad("Unsupported XLSX compression.");
      quint32 sum = 0xffffffffU;
      for (unsigned char b : out) {
        sum ^= b;
        for (int bit = 0; bit < 8; ++bit)
          sum = (sum >> 1) ^ ((sum & 1U) ? 0xedb88320U : 0U);
      }
      if (~sum != crc)
        bad("Damaged XLSX entry (checksum mismatch).");
      return out;
    }
    at += 46 + length + u16(at + 30) + u16(at + 32);
  }
  return {};
}
} // namespace
QString normalized_table_cell(QString s) {
  if (s.startsWith('(') && s.endsWith(')')) {
    int depth = 0;
    bool balanced = true;
    for (int i = 0; i < s.size(); ++i) {
      if (s[i] == '(')
        ++depth;
      else if (s[i] == ')' && (--depth < 0 || (!depth && i + 1 < s.size())))
        balanced = false;
    }
    if (balanced && !depth)
      s = s.mid(1, s.size() - 2);
  }
  return s;
}
TextTable parse_text_table(QString text, QChar separator) {
  if (text.toUtf8().size() > qsizetype(group_import_max_bytes))
    bad("Table exceeds 16 MiB.");
  if (text.startsWith(QChar(0xfeff)))
    text.remove(0, 1);
  text.replace("\r\n", "\n");
  text.replace('\r', '\n');
  TextTable rows;
  QStringList row;
  QString cell;
  bool quoted = false, closed = false, started = false;
  auto field = [&] {
    if (cell.contains('\n') || cell.contains(QChar(0)))
      bad("Table cells must not contain line breaks or null characters.");
    row << cell;
    cell.clear();
    closed = false;
    started = false;
    if (row.size() > int(group_import_max_columns))
      bad("Table exceeds 4,096 columns.");
  };
  auto line = [&] {
    field();
    if (row.size() > 1 || !row[0].isEmpty())
      rows.push_back(row);
    row.clear();
    if (rows.size() > group_import_max_rows + 1)
      bad("Table exceeds 100,000 data rows.");
  };
  for (qsizetype i = 0; i < text.size(); ++i) {
    auto c = text[i];
    if (quoted) {
      if (c == '"') {
        if (i + 1 < text.size() && text[i + 1] == '"') {
          cell += '"';
          ++i;
        } else {
          quoted = false;
          closed = true;
        }
      } else
        cell += c;
      continue;
    }
    if (c == separator) {
      field();
      continue;
    }
    if (c == '\n') {
      line();
      continue;
    }
    if (closed)
      bad("Unexpected text after a quoted cell.");
    if (!started && c == '"') {
      quoted = true;
      started = true;
    } else {
      cell += c;
      started = true;
    }
  }
  if (quoted)
    bad("Unclosed quoted table cell.");
  if (started || closed || !cell.isEmpty() || !row.isEmpty())
    line();
  return rows;
}
QString table_text(const TextTable &rows) {
  QString out;
  qsizetype bytes = 0;
  for (const auto &row : rows) {
    QStringList escaped;
    for (auto c : row) {
      c.replace('"', "\"\"");
      escaped << '"' + c + '"';
    }
    auto line = escaped.join('\t') + '\n';
    bytes += line.toUtf8().size();
    if (bytes > qsizetype(group_import_max_bytes))
      bad("Table exceeds 16 MiB.");
    out += line;
  }
  return out;
}
TextTable read_xlsx_table(const QString &path, int sheet,
                          QStringList *sheetNames) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly) || file.size() > 64 * 1024 * 1024)
    bad("Cannot read workbook, or file exceeds 64 MiB.");
  const auto zip = file.readAll();
  QStringList names, ids;
  QXmlStreamReader workbook(zip_entry(zip, "xl/workbook.xml"));
  while (!workbook.atEnd()) {
    workbook.readNext();
    if (workbook.isStartElement() && workbook.name() == u"sheet") {
      names << workbook.attributes().value("name").toString();
      ids << workbook.attributes()
                 .value("http://schemas.openxmlformats.org/officeDocument/2006/"
                        "relationships",
                        "id")
                 .toString();
    }
  }
  check_xml(workbook);
  if (names.isEmpty() || sheet < 0 || sheet >= names.size())
    bad("No matching worksheet in workbook.");
  if (sheetNames) {
    *sheetNames = names;
    return {};
  }
  QMap<QString, QString> targets;
  QXmlStreamReader rels(zip_entry(zip, "xl/_rels/workbook.xml.rels"));
  while (!rels.atEnd()) {
    rels.readNext();
    if (rels.isStartElement() && rels.name() == u"Relationship" &&
        rels.attributes().value("TargetMode") != u"External")
      targets[rels.attributes().value("Id").toString()] =
          rels.attributes().value("Target").toString();
  }
  check_xml(rels);
  QString target = targets.value(ids[sheet]);
  if (target.startsWith('/'))
    target.remove(0, 1);
  else
    target = "xl/" + target;
  if (target.contains("..") || !target.startsWith("xl/"))
    bad("Invalid worksheet path.");
  QStringList shared;
  QXmlStreamReader strings(zip_entry(zip, "xl/sharedStrings.xml"));
  QString word;
  while (!strings.atEnd()) {
    strings.readNext();
    if (strings.isStartElement() && strings.name() == u"si")
      word.clear();
    else if (strings.isStartElement() && strings.name() == u"t")
      word += strings.readElementText();
    else if (strings.isEndElement() && strings.name() == u"si") {
      shared << word;
      if (shared.size() > 1000000)
        bad("Too many shared strings.");
    }
  }
  if (!shared.isEmpty())
    check_xml(strings);
  const auto xml = zip_entry(zip, target.toUtf8());
  if (xml.isEmpty())
    bad("Worksheet is missing.");
  QXmlStreamReader x(xml);
  TextTable result;
  QStringList row;
  QString type, value;
  int column = 0;
  bool inCell = false, formula = false, hasValue = false;
  qsizetype bytes = 0;
  while (!x.atEnd()) {
    x.readNext();
    if (x.isStartElement()) {
      if (x.name() == u"row")
        row.clear();
      else if (x.name() == u"c") {
        inCell = true;
        type = x.attributes().value("t").toString();
        const auto ref = x.attributes().value("r").toString();
        column = 0;
        for (auto c : ref) {
          if (!c.isLetter())
            break;
          column = column * 26 + (c.toUpper().unicode() - 'A' + 1);
          if (column > 4096)
            bad("Worksheet exceeds 4,096 columns.");
        }
        if (!column)
          column = row.size() + 1;
        value.clear();
        formula = false;
        hasValue = false;
      } else if (inCell && x.name() == u"f") {
        formula = true;
        x.skipCurrentElement();
      } else if (inCell && (x.name() == u"v" || x.name() == u"t")) {
        value += x.readElementText();
        hasValue = true;
      }
    } else if (x.isEndElement()) {
      if (x.name() == u"c") {
        if (formula && !hasValue)
          bad("A formula has no saved value. Recalculate and save the workbook "
              "first.");
        if (type == "e")
          bad("Worksheet contains an error cell.");
        if (type == "s") {
          bool ok = false;
          int index = value.toInt(&ok);
          if (!ok || index < 0 || index >= shared.size())
            bad("Invalid shared-string index.");
          value = shared[index];
        }
        if (column <= row.size())
          bad("Duplicate or out-of-order worksheet cell.");
        while (row.size() < column)
          row << QString{};
        row[column - 1] = value;
        bytes += value.toUtf8().size();
        if (bytes > qsizetype(group_import_max_bytes))
          bad("Worksheet exceeds 16 MiB.");
        inCell = false;
      } else if (x.name() == u"row" && !row.isEmpty()) {
        result.push_back(row);
        if (result.size() > group_import_max_rows + 1)
          bad("Worksheet exceeds 100,000 data rows.");
      }
    }
  }
  check_xml(x);
  return result;
}
std::optional<QString> load_table_file(QWidget *parent, const QString &title) {
  const auto path = QFileDialog::getOpenFileName(
      parent, title, {}, "Tables (*.tsv *.csv *.xlsx *.txt);;All files (*)");
  if (path.isEmpty())
    return {};
  try {
    if (QFileInfo(path).suffix().compare("xlsx", Qt::CaseInsensitive) == 0) {
      QStringList names;
      read_xlsx_table(path, 0, &names);
      int index = 0;
      if (names.size() > 1) {
        bool ok = false;
        auto name = QInputDialog::getItem(parent, "Choose worksheet",
                                          "Worksheet", names, 0, false, &ok);
        if (!ok)
          return {};
        index = names.indexOf(name);
      }
      return table_text(read_xlsx_table(path, index));
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly) ||
        f.size() > qsizetype(group_import_max_bytes))
      bad("Cannot read table, or file exceeds 16 MiB.");
    const auto bytes = f.readAll();
    auto text = QString::fromUtf8(bytes);
    if (text.toUtf8() != bytes)
      bad("Use UTF-8 encoding for text tables.");
    QChar delimiter = '\t';
    if (QFileInfo(path).suffix().compare("csv", Qt::CaseInsensitive) == 0) {
      delimiter = ',';
      if (!text.section('\n', 0, 0).contains(',') &&
          text.section('\n', 0, 0).contains(';'))
        delimiter = ';';
    }
    return table_text(parse_text_table(text, delimiter));
  } catch (const std::exception &e) {
    QMessageBox::warning(parent, "Load table", QString::fromUtf8(e.what()));
    return {};
  }
}
} // namespace kleiotide
