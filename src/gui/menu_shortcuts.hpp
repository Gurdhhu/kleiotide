#pragma once
#include <QAction>
#include <QKeySequence>
namespace kleiotide {
// Context commands share shortcuts handled by the alignment/main window. Show
// native key names without registering a second, competing shortcut handler.
inline QAction* shortcut_hint(QAction* action,const QKeySequence& keys){
    const auto hint=keys.toString(QKeySequence::NativeText);
    action->setText(action->text().section('\t',0,0)+'\t'+hint);
    action->setProperty("shortcutHint",hint);
    return action;
}
}
