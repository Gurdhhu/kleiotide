#pragma once
#include "core/alignment.hpp"
#include <memory>
class QWidget;
namespace kleiotide { bool link_trace_dialog(std::shared_ptr<Document>,const std::vector<SequenceId>&,QWidget*); }
