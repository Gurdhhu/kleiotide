#pragma once
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVersionNumber>
#include <functional>

class QWidget;
namespace kleiotide {
enum class UpdatePlatform { WindowsX64, Debian13Amd64, Other };

struct ReleaseUpdate {
    QString tag, asset, windowsInstaller;
    QUrl releaseUrl, assetUrl;
    QByteArray digest;
    qint64 size = 0;
    bool preview = false;
};

ReleaseUpdate find_release_update(const QByteArray&, const QVersionNumber& current, UpdatePlatform);
QByteArray verified_update_payload(const ReleaseUpdate&, QByteArray);
void check_for_updates(QWidget*, const std::function<void(const QString&)>& installWindows);
bool launch_windows_update(const QString& installer);
}
