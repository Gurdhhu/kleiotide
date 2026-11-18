#include "gui/residue_palette_dialog.hpp"
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QJsonObject>
#include <QPushButton>
#include <QVBoxLayout>

namespace kleiotide {
QJsonObject encode_residue_colors(const CustomResidueColors& colors){
    QJsonObject object;
    for(std::size_t i=0;i<colors.size();++i)
        object[QString(QChar::fromLatin1(custom_palette_symbols[i]))]=colors[i].name();
    return object;
}
CustomResidueColors decode_residue_colors(const QJsonObject& object){
    auto colors=default_residue_colors();
    for(std::size_t i=0;i<colors.size();++i){
        const QColor color(object[QString(QChar::fromLatin1(custom_palette_symbols[i]))].toString());
        if(color.isValid())colors[i]=color;
    }
    return colors;
}

std::optional<CustomResidueColors> choose_residue_colors(QWidget* parent,CustomResidueColors colors){
    QDialog dialog(parent);
    dialog.setWindowTitle("Custom residue palette");
    dialog.setObjectName("customResiduePalette");
    QVBoxLayout layout(&dialog);
    QGridLayout grid;
    layout.addLayout(&grid);
    std::array<QPushButton*,custom_palette_symbols.size()> swatches{};
    const auto refresh=[&](std::size_t i){
        const auto& color=colors[i];
        swatches[i]->setStyleSheet(QString("QPushButton { background: %1; color: %2; }")
                                   .arg(color.name(),black_or_white(color).name()));
    };
    for(std::size_t i=0;i<colors.size();++i){
        const QString symbol(QChar::fromLatin1(custom_palette_symbols[i]));
        auto* button=new QPushButton(symbol,&dialog);
        swatches[i]=button;
        button->setObjectName("residueColor."+symbol);
        button->setToolTip("Choose the color for "+symbol+".");
        button->setMinimumWidth(45);
        grid.addWidget(button,static_cast<int>(i/5),static_cast<int>(i%5));
        refresh(i);
        QObject::connect(button,&QPushButton::clicked,&dialog,[&,i,symbol]{
            const auto color=QColorDialog::getColor(colors[i],&dialog,"Color for "+symbol);
            if(color.isValid()){colors[i]=color;refresh(i);}
        });
    }
    QDialogButtonBox buttons(QDialogButtonBox::RestoreDefaults|QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    layout.addWidget(&buttons);
    QObject::connect(buttons.button(QDialogButtonBox::RestoreDefaults),&QPushButton::clicked,&dialog,[&]{
        colors=default_residue_colors();
        for(std::size_t i=0;i<colors.size();++i)refresh(i);
    });
    QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()==QDialog::Accepted)return colors;
    return {};
}
}
