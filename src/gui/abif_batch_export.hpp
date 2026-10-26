#pragma once
#include "core/alignment.hpp"
#include <QStringList>
#include <atomic>
#include <functional>
class QWidget;
namespace kleiotide {
struct AbifBatchItem {SequenceId id;QString title,path;};
struct AbifBatchResult {QStringList written,errors,notes;bool cancelled=false;};
std::vector<AbifBatchItem> abif_batch_items(const Document&,const std::vector<SequenceId>& selected,bool onlySelected,const QString& directory,const QString& suffix=QStringLiteral("_edited"));
AbifBatchResult write_abif_batch(const Document&,const std::vector<AbifBatchItem>&,const std::atomic_bool& cancel,const std::function<void(std::size_t)>& progress={});
void export_abif_batch_dialog(std::shared_ptr<const Document>,std::vector<SequenceId> selected,QWidget* parent);
}
