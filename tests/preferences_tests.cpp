#include "gui/app_settings.hpp"
#include "core/alignment.hpp"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {void need(bool v,const char* m){if(!v)throw std::runtime_error(m);}}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir dir;need(dir.isValid(),"Temporary settings directory");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());
    QSettings::setPath(QSettings::IniFormat,QSettings::SystemScope,dir.filePath("system"));
    QSettings old(QSettings::defaultFormat(),QSettings::UserScope,"Kleiotide","Kleiotide");old.setValue("mafft/launcher","old-launcher.bat");old.setValue("mafft/threads",-1);old.sync();
    AppSettings settings;if(settings.values().mafftLauncher!="old-launcher.bat"||settings.values().mafftThreads!=0)throw std::runtime_error("Legacy migration: path="+settings.values().mafftLauncher.toStdString()+", threads="+std::to_string(settings.values().mafftThreads)+", store="+old.fileName().toStdString()+", keys="+old.allKeys().join(",").toStdString()+", validation="+settings.values().error().toStdString());
    auto p=settings.values();p.font.setPointSize(14);p.font.setBold(true);p.palette=ResiduePalette::Chromas;p.bulkMiB=3000;p.code=2;p.mafftThreads=3;p.recoverySeconds=45;p.blastEmail="default@example.org";p.blastnLauncher="C:/NCBI/blastn.exe";p.blastProteinDatabase="swissprot";p.blastMaxHits=25;p.blastExpect=1e-10;p.blastWaitMinutes=45;
    int emissionCount=0;QObject::connect(&settings,&AppSettings::changed,[&]{++emissionCount;});QString error;
    need(settings.apply(p,&error)&&emissionCount==1,"Validated preferences commit once");AppSettings restarted;need(restarted.values()==p,"Restart retains typed preferences");
    auto bad=p;bad.bulkMiB=-1;need(!settings.apply(bad,&error)&&settings.values()==p&&emissionCount==1,"Invalid values cannot commit");
    bad=p;bad.blastExpect=0;need(!settings.apply(bad),"Invalid BLAST threshold refused");bad=p;bad.blastEmail="not an email";need(!settings.apply(bad),"Invalid BLAST contact refused");
    auto custom=genetic_code(1);custom.id=0;custom.name="My custom code";custom.aminoAcids[0]='A';p.customCode=custom;p.code=0;
    need(settings.apply(p),"Custom code accepted");restarted.reload();need(restarted.values().translation()==p.translation(),"Full custom code definition survives restart");
    QJsonObject tampered{{"version",1},{"bulkMiB",-5}};old.setValue("preferences/v1",QJsonDocument(tampered).toJson(QJsonDocument::Compact));old.sync();restarted.reload();need(restarted.values().bulkMiB==2048,"Tampered limits fall back safely");
    auto blob=QJsonDocument::fromJson(old.value("preferences/v1").toByteArray()).object();blob["bulkMiB"]=2048;QFont missing=p.font;missing.setFamily("Kleiotide nonexistent font 1234");blob["font"]=missing.toString();old.setValue("preferences/v1",QJsonDocument(blob).toJson());old.sync();restarted.reload();need(restarted.values().font.family()!=missing.family()&&restarted.values().font.pointSize()==14,"Unavailable family falls back while retaining size/style");
    Document fresh;fresh.load({},p.translation());need(!fresh.can_undo()&&fresh.translation_settings()==p.translation(),"New-document code is a clean baseline, not an Undo command");
    std::cout<<"PASS settings: migration, restart, custom code, invalid/tampered values, font fallback and clean translation defaults\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
