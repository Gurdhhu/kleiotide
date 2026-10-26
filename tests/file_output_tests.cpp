#include "gui/file_output.hpp"
#include "core/abif.hpp"
#include "core/session.hpp"
#include "abif_fixture.hpp"
#include <QApplication>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void put(const QString& path,const QByteArray& bytes){QFile f(path);need(f.open(QIODevice::WriteOnly),"Create fixture");need(f.write(bytes)==bytes.size()&&f.flush(),"Write fixture");}
QByteArray get(const QString& path){QFile f(path);need(f.open(QIODevice::ReadOnly),"Read fixture");return f.readAll();}
class ShortDevice final:public QIODevice {
public:ShortDevice(){open(QIODevice::WriteOnly);}
protected:qint64 readData(char*,qint64)override{return -1;}qint64 writeData(const char*,qint64 n)override{return n>1?n-1:-1;}
};
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    Document d;d.load({{"row","ATGAAA"}});need(!requires_session(d),"Plain nucleotide FASTA stays available");
    for(int kind=0;kind<4;++kind){auto settings=TranslationSettings{};if(kind==0)settings.frame=2;if(kind==1)settings.code=2;if(kind==2)settings.regions={{1,2,0}};if(kind==3){auto custom=std::make_shared<GeneticCode>(genetic_code(1));custom->id=0;custom->name="custom";settings.custom=custom;settings.code=0;}d.set_translation_settings(settings);need(requires_session(d),"Every translation metadata kind requires a session");}
    d.set_translation_settings({});need(!requires_session(d),"Resetting metadata restores plain FASTA eligibility");
    need(output_path("one.txt",OutputFormat::Session)=="one.txt.kleio"&&output_path("one.KLEIO",OutputFormat::Session)=="one.KLEIO","Session suffix normalization");
    need(output_path("one.txt",OutputFormat::Abif)=="one.txt.ab1"&&output_path("one.ABI",OutputFormat::Abif)=="one.ABI","AB1 suffix normalization");
    QTemporaryDir directory;need(directory.isValid(),"Temporary folder");const auto path=directory.filePath("export Я.fasta");put(path,"original");
    for(const auto& name:{"????","bad?.fasta","bad*.fa","bad:name.fa","NUL.fasta","COM1.fa","trail. ","bad|name"})need(!output_filename_error(name,OutputNameRules::Windows).isEmpty(),"Invalid Windows output name is diagnosed");
    need(output_filename_error("correct Я.fasta",OutputNameRules::Windows).isEmpty(),"Unicode/space filename stays valid");
    int stage=0;bool warned=false,filterPreserved=false;QStringList initialFilters;QTimer filename;filename.setInterval(2);
    QObject::connect(&filename,&QTimer::timeout,[&]{
        if(auto* warning=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){warned=warning->windowTitle()=="Invalid output filename";warning->accept();stage=2;}
        else if(auto* chooser=qobject_cast<QFileDialog*>(QApplication::activeModalWidget())){
            if(stage==0){if(auto* model=chooser->findChild<QFileSystemModel*>())initialFilters=model->nameFilters();stage=1;chooser->selectFile("????");QMetaObject::invokeMethod(chooser,"accept",Qt::QueuedConnection);}
            else if(stage==2){if(auto* model=chooser->findChild<QFileSystemModel*>())filterPreserved=model->nameFilters()==initialFilters;stage=3;chooser->selectNameFilter("Session (*.kleio)");chooser->selectFile(directory.filePath("valid.kleio"));QMetaObject::invokeMethod(chooser,"accept",Qt::QueuedConnection);}
        }
    });
    QTimer timeout;timeout.setSingleShot(true);QObject::connect(&timeout,&QTimer::timeout,[&]{if(auto* widget=qobject_cast<QDialog*>(QApplication::activeModalWidget()))widget->reject();});
    QString selectedFilter="FASTA (*.fasta)";filename.start();timeout.start(5000);const auto chosen=choose_output_filename(nullptr,"Save test",directory.path(),"FASTA (*.fasta);;Session (*.kleio)",&selectedFilter,QFileDialog::DontConfirmOverwrite,OutputNameRules::Windows);filename.stop();timeout.stop();
    need(warned&&filterPreserved&&stage==3&&chosen==directory.filePath("valid.kleio")&&selectedFilter=="Session (*.kleio)","Wildcard is rejected before filtering; valid retry keeps dialog usable");
    need(write_output_atomic(path,[](auto& out){out<<">one\nACGT\n";}).isEmpty()&&get(path)==">one\nACGT\n","Complete atomic replacement");
    const auto files=QDir(directory.path()).entryList(QDir::Files|QDir::Hidden);
    const auto expected=get(path);auto error=write_output_atomic(path,[](auto& out){out<<"partial";throw std::runtime_error("Injected write failure");});
    need(!error.isEmpty()&&get(path)==expected,"Failed serialization preserves old file");
    error=write_output_atomic(path,[](auto& out){out<<"partial";out.setstate(std::ios::badbit);});need(!error.isEmpty()&&get(path)==expected,"Stream failure preserves old file");
    error=write_output_atomic(path,[](auto& out){out<<"cancelled";},[]{return true;});need(!error.isEmpty()&&get(path)==expected,"Cancellation preserves old file");
    need(QDir(directory.path()).entryList(QDir::Files|QDir::Hidden)==files,"Failed writes remove temporary files");
    ShortDevice device;bool rejected=false;try{write_output(device,[](auto& out){out.write("0123456789",10);});}catch(const std::exception&){rejected=true;}need(rejected,"Positive short write is detected");
    const auto commit=directory.filePath("blocked.fasta");error=write_output_atomic(commit,[&](auto& out){out<<"complete";need(QDir().mkdir(commit),"Inject rename/commit failure");});need(!error.isEmpty()&&QFileInfo(commit).isDir(),"Commit failure is reported");
    const auto opaque=directory.filePath("opaque.fasta");std::ostringstream bytes(std::ios::binary);write_session(bytes,d);put(opaque,QByteArray::fromStdString(bytes.str()));
    need(!output_path_error(opaque,OutputFormat::Fasta).isEmpty()&&!output_path_error(opaque,OutputFormat::Abif).isEmpty(),"Misnamed native sessions are protected by contents");
    need(output_path_error(opaque,OutputFormat::Session).isEmpty(),"Session may replace a session after approval");
    const auto trace=directory.filePath("original.data");put(trace,QByteArray::fromStdString(test::synthetic_abif()));
    std::istringstream input(test::synthetic_abif());Document source;source.load({read_abif(input,"trace",trace.toUtf8().toStdString())});
    need(!output_path_error(trace,OutputFormat::Fasta,&source).isEmpty()&&!output_path_error(trace,OutputFormat::Abif,&source).isEmpty(),"Imported trace cannot be overwritten");
    for(auto format:{OutputFormat::Fasta,OutputFormat::Table}){
        const auto original=get(trace);
        auto blocked=write_export_atomic(trace,format,&source,[](auto& out){out<<"must not overwrite";});
        need(!blocked.isEmpty()&&get(trace)==original,"Shared exporter refuses an imported trace before writing");
        const auto sessionBefore=get(opaque);
        blocked=write_export_atomic(opaque,format,&source,[](auto& out){out<<"must not overwrite";});
        need(!blocked.isEmpty()&&get(opaque)==sessionBefore,"Shared exporter protects session contents even with another suffix");
    }
    const auto table=directory.filePath("analysis.tsv");
    need(write_export_atomic(table,OutputFormat::Table,&source,[](auto& out){out<<"name\tvalue\nrow\t1\n";}).isEmpty(),"Table export commits atomically");
    const auto savedTable=get(table);error=write_export_atomic(table,OutputFormat::Table,&source,[](auto& out){out<<"partial";throw std::runtime_error("Injected table failure");});
    need(!error.isEmpty()&&get(table)==savedTable,"Failed table export preserves existing output");
    const auto alias=directory.filePath("alias.ab1");std::error_code ec;
#ifdef _WIN32
    std::filesystem::create_hard_link(std::filesystem::path(trace.toStdWString()),std::filesystem::path(alias.toStdWString()),ec);
#else
    std::filesystem::create_hard_link(trace.toStdString(),alias.toStdString(),ec);
#endif
    if(!ec)need(!output_path_error(alias,OutputFormat::Abif,&source).isEmpty(),"Hard links to imported evidence are protected");
    std::cout<<"PASS session eligibility, final-path protection, Unicode paths, complete/short/failed writes, cancellation and commit failures\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
