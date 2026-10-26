#pragma once
#include "core/merge.hpp"
#include <memory>
class QWidget;
namespace kleiotide {
// Returns the appended row ID, or no value after Cancel/failure.
std::optional<SequenceId> merge_sequences_dialog(std::shared_ptr<Document>,
    const std::vector<SequenceId>& selected,MergeMode,QWidget* parent);
}
