#pragma once
#include "gui/rename_rules.hpp"
#include <QDialog>
namespace kleiotide {
// Modal preview; returns changed names only after Apply. Cancel has no effect.
std::optional<std::vector<std::pair<SequenceId,std::string>>> show_batch_rename(
    const Document&,const std::vector<SequenceId>& selected,const QString& filename,RenameOperation,QWidget* parent);
QString rename_operation_label(RenameOperation);
}
