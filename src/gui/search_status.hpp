#pragma once
#include <QLabel>
namespace kleiotide {
inline void search_status(QLabel* label,const QString& text,const QString& color){label->setText(text);label->setStyleSheet("QLabel { background: "+color+"; color: #17212b; border: 1px solid #52616b; border-radius: 4px; padding: 6px; font-weight: bold; }");label->setAccessibleName("Search status: "+text);}
}
