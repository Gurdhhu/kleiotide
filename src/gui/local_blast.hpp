#pragma once
#include "core/alignment.hpp"
#include <QJsonObject>
#include <QStringList>
#include <memory>
class QWidget;
namespace kleiotide {
struct LocalDatabase {
  QString manifest, prefix, title;
  bool protein = false;
  QJsonObject names;
};
QString local_blast_executable(const QString &program);
LocalDatabase read_local_database(const QString &manifest);
QStringList local_database_paths(bool protein);
void remember_local_database(const QString &manifest);
QJsonObject prepare_local_database(const QString &source, bool protein,
                                   const QString &fasta);
void create_local_database(bool protein, QWidget *);
void local_blast_dialog(std::shared_ptr<Document>,
                        const std::vector<SequenceId> &, bool protein,
                        QWidget *);
} // namespace kleiotide
