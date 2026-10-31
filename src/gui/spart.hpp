#pragma once
#include "core/alignment.hpp"
#include "gui/table_io.hpp"
namespace kleiotide {
struct SpartPartition {
  QString name;
  TextTable assignments;
};
QString spart_sanitize(QString);
std::vector<SpartPartition> parse_spart(const QByteArray &);
QByteArray export_spart(const Document &, bool xml, const QString &project,
                        const QString &partition);
bool import_spart_groups(Document &, QWidget *);
void export_sequence_groups(const Document &, QWidget *);
} // namespace kleiotide
