#pragma once
#include "core/alignment.hpp"
#include <memory>
class QWidget;
namespace kleiotide {
bool collapse_sequences_dialog(std::shared_ptr<Document>,const std::vector<SequenceId>& selected,QWidget* parent);
}
