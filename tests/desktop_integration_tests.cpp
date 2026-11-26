#include "gui/open_requests.hpp"
#include "gui/tool_discovery.hpp"
#include "gui/update_check.hpp"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

namespace {
void verify_update_selection() {
    const auto asset = [](QString tag, QString name, const QByteArray& contents) {
        return QJsonObject{{"name",name},{"state","uploaded"},{"size",contents.size()},
            {"digest","sha256:"+QString::fromLatin1(QCryptographicHash::hash(contents,QCryptographicHash::Sha256).toHex())},
            {"browser_download_url","https://github.com/Gurdhhu/kleiotide/releases/download/"+tag+'/'+name}};
    };
    const QByteArray executable("MZ installer fixture");
    const QByteArray package("!<arch>\nDebian fixture");
    const QString target("v0.30.6-rc1");
    auto wrongAsset = asset("v0.30.7","Kleiotide_0.30.7_Windows_x64_Setup.exe",executable);
    wrongAsset["browser_download_url"] = "https://example.org/setup.exe";
    const auto feed = QJsonDocument(QJsonArray{
        QJsonObject{{"tag_name","qt-sources-6.5.3"}},
        QJsonObject{{"tag_name","v0.30.7"},{"assets",QJsonArray{wrongAsset}}},
        QJsonObject{{"tag_name","v0.30.9"},{"draft",true}},
        QJsonObject{{"tag_name",target},{"prerelease",true},{"assets",QJsonArray{
            asset(target,"Kleiotide_0.30.6_rc1_Windows_x64_Setup.exe",executable),
            asset(target,"kleiotide_0.30.6-1.deb13_amd64.deb",package)}}}
    }).toJson();
    const auto windows = find_release_update(feed,{0,30,5},UpdatePlatform::WindowsX64);
    const auto debian = find_release_update(feed,{0,30,5},UpdatePlatform::Debian13Amd64);
    const auto other = find_release_update(feed,{0,30,5},UpdatePlatform::Other);
    if (!windows.preview || windows.tag!=target || debian.tag!=target || !other.asset.isEmpty() || other.tag!="v0.30.7")
        throw std::runtime_error("Release or compatible asset selection");
    if (verified_update_payload(windows,executable)!=executable || verified_update_payload(debian,package)!=package)
        throw std::runtime_error("Verified installers retain the published bytes");
    auto corrupt = executable;
    corrupt[4]='?';
    try { (void)verified_update_payload(windows,corrupt); }
    catch (const std::exception&) { return; }
    throw std::runtime_error("Update verification accepted modified bytes");
}
}
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
    verify_update_selection();
    if (argc == 3) {
        QFile feed(QString::fromLocal8Bit(argv[1])), zip(QString::fromLocal8Bit(argv[2]));
        if (!feed.open(QIODevice::ReadOnly) || !zip.open(QIODevice::ReadOnly))
            throw std::runtime_error("Cannot open public release fixtures");
        const auto update = find_release_update(feed.readAll(),{0,30,1},UpdatePlatform::WindowsX64);
        const auto installer = verified_update_payload(update,zip.readAll());
        if (installer.isEmpty() || !update.asset.endsWith(".zip")) throw std::runtime_error("Public ZIP extraction failed");
        std::cout<<"PASS public GitHub asset checksum and setup ZIP extraction\n";
    }
    std::cout<<"PASS Finder events, executable discovery, GitHub release selection and update verification\n";
    return 0;
} catch(const std::exception& error){
    std::cerr<<"FAIL "<<error.what()<<'\n';
    return 1;
}
