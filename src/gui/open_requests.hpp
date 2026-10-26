#pragma once
#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QLockFile>
#include <functional>
#include <memory>
namespace kleiotide {
// Per-user atomic file requests work with Windows Explorer's separate launches.
class OpenRequests final:public QObject {
public:
    explicit OpenRequests(QObject* parent=nullptr);
    bool listen(const QString& key);
    static bool forward(const QString& key,const QStringList& paths);
    void queue(const QStringList& paths);
    std::function<void(const QStringList&)> opened;
    std::function<void()> activate;
private:
    static QString directory(const QString& key);
    QTimer timer_,poll_;QStringList pending_;QString directory_;
    std::unique_ptr<QLockFile> lock_;
};
}
