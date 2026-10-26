#pragma once
#include "core/alignment.hpp"
#include <memory>
class QWidget;
namespace kleiotide {
bool identity_groups_dialog(std::shared_ptr<Document>,const std::vector<SequenceId>& selected,QWidget* parent);
}
