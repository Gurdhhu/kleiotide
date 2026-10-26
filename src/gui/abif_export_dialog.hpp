#pragma once
#include "core/alignment.hpp"
#include <QString>
#include <memory>
class QWidget;
namespace kleiotide {
QString export_abif_dialog(std::shared_ptr<const Document>,SequenceId preferred,QWidget* parent);
}
