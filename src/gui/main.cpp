#include "gui/main_window.hpp"
#include "core/version.hpp"
#include "gui/open_requests.hpp"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>
int main(int argc,char** argv){
    QApplication app(argc,argv);QCoreApplication::setApplicationName("Kleiotide");QCoreApplication::setApplicationVersion(QString::fromLatin1(kleiotide::version));
    QStringList files;for(const auto& argument:app.arguments().mid(1))files.push_back(QFileInfo(argument).absoluteFilePath());
    const auto identity=QCoreApplication::applicationFilePath()+"\n"+QDir::homePath();
    const auto key="kleiotide-"+QString::fromLatin1(QCryptographicHash::hash(identity.toUtf8(),QCryptographicHash::Sha256).toHex().left(24));
    kleiotide::OpenRequests requests;
    if(!requests.listen(key)){
        if(kleiotide::OpenRequests::forward(key,files))return 0;
        if(!requests.listen(key)){QMessageBox::critical(nullptr,"Could not start Kleiotide","Could not initialize local file-opening communication.");return 1;}
    }
    auto* initial=new kleiotide::MainWindow;initial->setAttribute(Qt::WA_DeleteOnClose);initial->show();QPointer<kleiotide::MainWindow> first=initial;bool useInitial=!files.isEmpty();
    requests.opened=[&](const QStringList& paths){auto* window=useInitial&&first?first.data():new kleiotide::MainWindow;useInitial=false;
        window->setAttribute(Qt::WA_DeleteOnClose);window->show();window->raise();window->activateWindow();window->open_paths(paths);};
    requests.activate=[&]{auto* window=new kleiotide::MainWindow;window->setAttribute(Qt::WA_DeleteOnClose);window->show();window->raise();window->activateWindow();};
    if(!files.isEmpty())requests.queue(files);
    else QTimer::singleShot(0,initial,[initial]{initial->offer_recovery(true);});
    return app.exec();
}
