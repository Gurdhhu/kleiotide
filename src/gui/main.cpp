#include "gui/main_window.hpp"
#include "gui/open_requests.hpp"
#include "core/version.hpp"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QMessageBox>
#include <QTimer>


int main(int argc, char** argv) {
    QApplication app(argc, argv);
    kleiotide::OpenRequests requests;
    app.installEventFilter(&requests);
    QCoreApplication::setApplicationName("Kleiotide");
    QCoreApplication::setApplicationVersion(QString::fromLatin1(kleiotide::version));
    QStringList files;
    for (const auto& argument : app.arguments().mid(1))
        files.append(QFileInfo(argument).absoluteFilePath());
    const auto identity = QCoreApplication::applicationFilePath() + "\n" + QDir::homePath();
    const auto key = "kleiotide-" + QString::fromLatin1(QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex().left(24));
    if (!requests.listen(key)) {
        app.processEvents();
        files.append(requests.take_pending());
        if (kleiotide::OpenRequests::forward(key, files)) return 0;
        if (!requests.listen(key)) {
            QMessageBox::critical(nullptr, "Could not start Kleiotide", "Could not initialize local file-opening communication.");
            return 1;
        }
    }
    const auto newWindow = [] {
        auto* window = new kleiotide::MainWindow;
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
        window->raise();
        window->activateWindow();
        return window;
    };
    bool startup = true;
    QPointer<kleiotide::MainWindow> blank;
    requests.activate = [&] { newWindow(); };
    requests.opened = [&](const QStringList& paths) {
        auto open = [&](const QStringList& group) {
            auto* window = blank && blank->is_empty_start_window() ? blank.data() : newWindow();
            blank.clear();
            startup = false;
            window->open_paths(group);
        };
        QStringList reads;
        for (const auto& path : paths) {
            if (QFileInfo(path).suffix().compare("kleio", Qt::CaseInsensitive) == 0) {
                if (!reads.isEmpty()) { open(reads); reads.clear(); }
                open({path});
            } else reads.append(path);
        }
        if (!reads.isEmpty()) open(reads);
    };
    if (!files.isEmpty()) requests.queue(files);
    QTimer::singleShot(0, &requests, [&] {
        if (!startup || requests.has_pending()) return;
        startup = false;
        auto* window = newWindow();
#ifdef Q_OS_MACOS
        // Finder may send its first document after the initial window appears.
        blank = window;
#endif
        window->offer_recovery(true);
    });
    return app.exec();
}
