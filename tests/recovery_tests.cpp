#include "gui/recovery.hpp"
#include "core/session.hpp"
#include <QApplication>
#include <QDir>
#include <QProcess>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <fstream>
#include <filesystem>
#include <map>
#include <sstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {void need(bool v,const char* m){if(!v)throw std::runtime_error(m);}Session read(const QString& path){std::ifstream in(std::filesystem::path(path.toStdU16String()),std::ios::binary);return read_session(in);}}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    if(argc==3&&QString::fromLocal8Bit(argv[1])=="--recovery-child"){
        Document child;child.load({{"child","ACGT"}});RecoveryStore store(QString::fromLocal8Bit(argv[2]));
        need(store.write(child,"{}",{},"Child","child-source.fasta",4*1024*1024).isEmpty(),"Child checkpoint");
        std::cout<<"ready"<<std::endl;return app.exec();
    }
    QTemporaryDir dir;need(dir.isValid(),"Temporary recovery directory");Document d;d.load({{"one",std::string(200000,'A')}});d.replace(d.rows()[0].id,8,1,"C");const auto state=d.state_id();
    QString latest;
    {
        RecoveryStore store(dir.path());need(store.write(d,"{}","report","Example","original.kleio",4*1024*1024).isEmpty(),"First atomic checkpoint");need(d.state_id()==state,"Checkpoint leaves source state unchanged");
        need(RecoveryStore::available(dir.path()).empty(),"Active window is excluded by lock");
        d.replace(d.rows()[0].id,9,1,"G");need(store.write(d,"{}","report","Example","original.kleio",4*1024*1024).isEmpty(),"Second generation");
        const auto currentState=d.state_id();
        const auto before=QDir(dir.path()).entryList({"*.kleio"},QDir::Files);need(before.size()==2,"Two completed generations retained");
        std::map<QString,QByteArray> bytes;for(const auto& name:before){QFile file(QDir(dir.path()).filePath(name));need(file.open(QIODevice::ReadOnly),"Read completed checkpoint");bytes[name]=file.readAll();}
        need(!store.write(d,"{}","report","Example","original.kleio",8).isEmpty(),"Quota refusal");
        need(!store.write(d,"{}","report","Example","original.kleio",4*1024*1024,[]{return true;}).isEmpty(),"Cancellation refusal");
        for(const auto& [name,expected]:bytes){QFile file(QDir(dir.path()).filePath(name));file.open(QIODevice::ReadOnly);need(file.readAll()==expected,"Refusal/cancellation retains both previous generations");}
        need(d.state_id()==currentState&&d.can_undo(),"Checkpoint does not alter live history");
    }
    const auto entries=RecoveryStore::available(dir.path());need(entries.size()==1&&entries[0].title=="original.kleio","Abandoned generations grouped by document identity");
    need(!entries[0].previousPath.isEmpty(),"Previous generation remains selectable");auto previous=read(entries[0].previousPath);need(previous.document.rows()[0].symbols.text()[9]=='A',"Previous generation is intact");
    auto recovered=read(entries[0].path);need(recovered.document.rows()[0].symbols.text()==d.rows()[0].symbols.text()&&recovered.alignmentReport=="report","Recovery session retains current data and report");
    {QFile corrupt(entries[0].path);need(corrupt.open(QIODevice::Append),"Corrupt latest fixture");corrupt.write("x");}bool rejected=false;try{read(entries[0].path);}catch(const std::runtime_error&){rejected=true;}need(rejected&&read(entries[0].previousPath).document.rows()[0].symbols.text()[9]=='A',"Strict validation rejects corrupt latest while previous remains readable");
    RecoveryClaim claim(entries[0]);need(claim.valid()&&RecoveryStore::available(dir.path()).empty(),"Recovery claim excludes other windows");need(claim.discard().isEmpty()&&QDir(dir.path()).entryList({"*.kleio"},QDir::Files).empty(),"Explicit discard removes only claimed generations");
    QProcess process;process.start(QCoreApplication::applicationFilePath(),{"--recovery-child",dir.path()});
    need(process.waitForStarted(10000),"Recovery child starts");QElapsedTimer deadline;deadline.start();QByteArray ready;
    while(!ready.contains("ready")&&deadline.elapsed()<30000&&process.state()!=QProcess::NotRunning){process.waitForReadyRead(100);ready+=process.readAllStandardOutput();}
    if(!ready.contains("ready"))throw std::runtime_error("Recovery child did not checkpoint: "+process.errorString().toStdString()+process.readAllStandardError().toStdString());
    need(RecoveryStore::available(dir.path()).empty(),"Another live process keeps its checkpoint locked");
    process.kill();need(process.waitForFinished(5000),"Abrupt child termination");
    const auto abandoned=RecoveryStore::available(dir.path());need(abandoned.size()==1&&read(abandoned[0].path).document.rows()[0].symbols.text()=="ACGT","Dead-process lock is reclaimed and committed data survives");
    RecoveryClaim dead(abandoned[0]);need(dead.valid()&&dead.discard().isEmpty(),"Discard reclaimed child checkpoint");
    std::ostringstream output;bool cancelled=false;try{write_session(output,d,{}, {},[]{return true;});}catch(const std::runtime_error&){cancelled=true;}need(cancelled,"Session writer honors cancellation");
    std::ostringstream partial;bool interrupted=false;try{write_session(partial,d,{}, {},[&]{return partial.tellp()>=std::streampos(65536);});}catch(const std::runtime_error&){interrupted=true;}need(interrupted&&partial.str().size()>=65536&&partial.str().size()<200000,"Cancellation interrupts a partially streamed large row");
    std::cout<<"PASS recovery: atomic generations, active locks, quota/cancellation preservation, reopen and explicit discard\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
