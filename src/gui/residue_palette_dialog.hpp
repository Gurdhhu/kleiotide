#pragma once
#include "gui/residue_style.hpp"
#include <QWidget>
#include <functional>
#include <optional>
class QPushButton;
namespace kleiotide {
class ResidueColorEditor final : public QWidget {
public:
    explicit ResidueColorEditor(QWidget* parent=nullptr);
    const CustomResidueColors& colors() const { return colors_; }
    void set_colors(const CustomResidueColors& colors);
    std::function<void()> colors_changed;
private:
    void refresh(std::size_t index);
    CustomResidueColors colors_=default_residue_colors();
    std::array<QPushButton*,custom_palette_symbols.size()> swatches_{};
};
std::optional<CustomResidueColors> choose_residue_colors(QWidget*,CustomResidueColors);
}
