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

ResidueColorEditor::ResidueColorEditor(QWidget* parent):QWidget(parent){
    auto* grid=new QGridLayout(this);
    grid->setContentsMargins(0,0,0,0);
    for(std::size_t i=0;i<colors_.size();++i){
        const QString symbol(QChar::fromLatin1(custom_palette_symbols[i]));
        auto* button=new QPushButton(symbol,this);
        swatches_[i]=button;
        button->setObjectName("residueColor."+symbol);
        button->setToolTip("Choose the color for "+symbol+".");
        button->setMinimumWidth(45);
        grid->addWidget(button,static_cast<int>(i/5),static_cast<int>(i%5));
        refresh(i);
        connect(button,&QPushButton::clicked,this,[this,i,symbol]{
            const auto color=QColorDialog::getColor(colors_[i],this,"Color for "+symbol);
            if(!color.isValid()||color==colors_[i])return;
            colors_[i]=color;
            refresh(i);
            if(colors_changed)colors_changed();
        });
    }
}
void ResidueColorEditor::refresh(std::size_t index){
    const auto& color=colors_[index];
    swatches_[index]->setStyleSheet(QString("QPushButton { background: %1; color: %2; }")
                                    .arg(color.name(),black_or_white(color).name()));
}
void ResidueColorEditor::set_colors(const CustomResidueColors& colors){
    colors_=colors;
    for(std::size_t i=0;i<colors_.size();++i)refresh(i);
}

std::optional<CustomResidueColors> choose_residue_colors(QWidget* parent,CustomResidueColors colors){
    QDialog dialog(parent);
    dialog.setWindowTitle("Custom residue palette");
    dialog.setObjectName("customResiduePalette");
    QVBoxLayout layout(&dialog);
    ResidueColorEditor editor(&dialog);
    editor.set_colors(colors);
    layout.addWidget(&editor);
    QDialogButtonBox buttons(QDialogButtonBox::RestoreDefaults|QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
    layout.addWidget(&buttons);
    QObject::connect(buttons.button(QDialogButtonBox::RestoreDefaults),&QPushButton::clicked,&dialog,[&]{
        editor.set_colors(default_residue_colors());
    });
    QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()==QDialog::Accepted)return editor.colors();
    return {};
}
}
