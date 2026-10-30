#pragma once
#include <QStringList>
#include <optional>
#include <vector>
class QWidget;
namespace kleiotide {
using TextTable = std::vector<QStringList>;
TextTable parse_text_table(QString text, QChar separator = '\t');
QString table_text(const TextTable &);
QString normalized_table_cell(QString);
std::optional<QString> load_table_file(QWidget *, const QString &title);
TextTable read_xlsx_table(const QString &path, int sheet = 0,
                          QStringList *sheetNames = nullptr);
} // namespace kleiotide
