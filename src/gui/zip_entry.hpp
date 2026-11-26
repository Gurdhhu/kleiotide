#pragma once
#include <QByteArray>

namespace kleiotide {
QByteArray zip_entry(const QByteArray& zip, const QByteArray& name, quint32 limit = 64 * 1024 * 1024);
}
