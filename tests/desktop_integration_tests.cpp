#include "gui/open_requests.hpp"
#include "gui/tool_discovery.hpp"
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>

using namespace kleiotide;

int main(int argc, char** argv) try {
    QApplication app(argc,argv);
    QTemporaryDir dir;
    if (!dir.isValid()) throw std::runtime_error("Cannot create temporary tool folder");
    const auto session=dir.filePath("Sample 1 Я.kleio");
    OpenRequests requests;
    app.installEventFilter(&requests);
    QFileOpenEvent startup(session);
    QApplication::sendEvent(&app,&startup);
    requests.queue({session});
    if (requests.take_pending()!=QStringList{session}) throw std::runtime_error("Startup Finder and argv requests must coalesce");

    const auto second=dir.filePath("Second.kleio");
    QStringList opened;
    QEventLoop loop;
    requests.opened=[&](const QStringList& paths){opened=paths; loop.quit();};
    QFileOpenEvent warm(QUrl::fromLocalFile(second));
    QApplication::sendEvent(&app,&warm);
    QTimer::singleShot(4000,&loop,&QEventLoop::quit);
    loop.exec();
    if (opened!=QStringList{second}) throw std::runtime_error("Warm file URL was lost");

    for (const auto& tool : QStringList{"mafft","blastn","blastp","blastx","tblastn","tblastx","makeblastdb"}) {
        auto filename=tool;
#ifdef Q_OS_WIN
        filename+=tool=="mafft"?".bat":".exe";
#endif
        QFile stub(dir.filePath(filename));
        if(!stub.open(QIODevice::WriteOnly)) throw std::runtime_error("Could not create tool fixture");
        stub.close();
        stub.setPermissions(stub.permissions()|QFile::ExeOwner);
        const auto found=find_local_tool(tool,{dir.filePath("existing-launcher")});
        if(QFileInfo(found)!=QFileInfo(stub)) throw std::runtime_error("Executable discovery missed a sibling");
#ifndef Q_OS_WIN
        const auto old=qgetenv("PATH");
        qputenv("PATH",dir.path().toUtf8());
        const auto byPath=find_local_tool(tool);
        qputenv("PATH",old);
        if (byPath!=stub.fileName()) throw std::runtime_error("Executable discovery missed PATH");
#endif
    }
    std::cout<<"PASS Finder events and executable discovery\n";
    return 0;
} catch(const std::exception& error){
    std::cerr<<"FAIL "<<error.what()<<'\n';
    return 1;
}
