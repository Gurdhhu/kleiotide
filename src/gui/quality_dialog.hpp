#pragma once
#include "core/trace_quality.hpp"
#include <memory>
class QWidget;
namespace kleiotide {
bool edit_trace_quality(std::shared_ptr<Document>,const std::vector<SequenceId>&,QualityOperation,QWidget*);
}
