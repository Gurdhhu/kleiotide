#pragma once

#include <QStringList>
#include <functional>

class QBoxLayout;
class QLineEdit;

namespace kleiotide {
QString find_local_tool(const QString& name, const QStringList& siblingLaunchers = {});
void add_locate_button(QBoxLayout* layout, QLineEdit* field,
                       std::function<QString()> locate);
}
