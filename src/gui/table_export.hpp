#pragma once
#include "core/distances.hpp"
#include "core/composition.hpp"
#include "core/identity_groups.hpp"
#include "gui/table_io.hpp"
#include <QMap>
#include <QSet>
#include <QUrl>
#include <functional>
class QAbstractItemModel;
class QWidget;
namespace kleiotide {
struct TableSheet {
  TableSheet(QString title = {}, TextTable cells = {}, QSet<int> numbers = {},
             QMap<QString, QUrl> hyperlinks = {})
      : name(std::move(title)), rows(std::move(cells)),
        numericColumns(std::move(numbers)), links(std::move(hyperlinks)) {}
  QString name;
  TextTable rows;
  QSet<int> numericColumns;
  QMap<QString, QUrl> links; // Excel cell references, e.g. E2.
};
using TableReport = std::vector<TableSheet>;
QString table_format_filters();
QString choose_table_path(QWidget *, const QString &initial = {},
                          const Document *source = nullptr);
QByteArray report_xlsx(const TableReport &,
                       const std::function<bool()> &cancel = {});
QByteArray report_delimited(const TableReport &, QChar delimiter = '\t',
                            const std::function<bool()> &cancel = {});
QByteArray report_bytes(const QString &path, const TableReport &,
                        const std::function<bool()> &cancel = {});
QString copy_model_table(const QAbstractItemModel *, bool rowLabels = false);
TableReport distance_workbook(const DistanceResult &, const std::function<bool()> &cancel = {});
TableReport composition_workbook(const NucleotideComposition &);
TableReport grouping_workbook(const Document &, const IdentityGroupPlan &);
} // namespace kleiotide
