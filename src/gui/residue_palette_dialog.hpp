#pragma once
#include "gui/residue_style.hpp"
#include <optional>
class QWidget;
namespace kleiotide {
std::optional<CustomResidueColors> choose_residue_colors(QWidget*,CustomResidueColors);
}
