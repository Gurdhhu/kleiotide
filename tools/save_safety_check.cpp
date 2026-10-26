#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/file_output.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QAction* action(MainWindow& w,const QString& text){for(auto* a:w.findChildren<QAction*>())if(a->text()==text)return a;throw std::runtime_error(("Missing action: "+text).toStdString());}
AlignmentView* view(MainWindow& w){return dynamic_cast<AlignmentView*>(w.centralWidget());}
template<class F>void until(QApplication& app,F ready){QElapsedTimer timer;timer.start();while(!ready()&&timer.elapsed()<15000){app.processEvents();QThread::msleep(1);}need(ready(),"GUI timeout");}
void put(const QString& path,const QByteArray& bytes){QFile file(path);need(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.flush(),"Fixture write");}
QByteArray get(const QString& path){QFile file(path);need(file.open(QIODevice::ReadOnly),"Fixture read");return file.readAll();}
void open(QApplication& app,MainWindow& w,const QString& path){w.show();w.open_path(path);until(app,[&]{return view(w)->isEnabled()&&!w.current_document().rows().empty();});}
void session(const QString& path,const Document& d){const auto error=write_output_atomic(path,[&](auto& out){write_session(out,d);});need(error.isEmpty(),"Session fixture write");}
struct Picker {
    QTimer timer;
    QString path;
    QMessageBox::StandardButton replace=QMessageBox::No;
    int replacements=0,warnings=0;
    bool chosen=false;
    Picker(QString value,QMessageBox::StandardButton answer=QMessageBox::No):path(std::move(value)),replace(answer){
        timer.setInterval(2);
        QObject::connect(&timer,&QTimer::timeout,[this]{
            if(auto* message=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){
                if(message->windowTitle()=="Replace existing file?"){++replacements;message->button(replace)->click();}
                else if(message->windowTitle()=="Unsaved changes")message->button(QMessageBox::Save)->click();
                else{++warnings;message->accept();}
            }else if(auto* chooser=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());chooser&&!chosen){
                chosen=true;chooser->selectFile(path);QMetaObject::invokeMethod(chooser,"accept",Qt::QueuedConnection);
            }
        });timer.start();
    }
};
void check_busy(MainWindow& w){
    std::cout<<"Checking busy state: "<<w.windowTitle().toStdString()<<std::endl;
    need(!view(w)->isEnabled(),"Background operation holds the editor busy");
    auto* frame=w.findChild<QComboBox*>("translationFrame");auto* code=w.findChild<QComboBox*>("translationGeneticCode");
    need(frame&&!frame->isEnabled()&&code&&!code->isEnabled(),"Frame/code controls disabled immediately");
    const auto before=w.current_document().translation_settings();const auto state=w.current_document().state_id();
    frame->setCurrentIndex((frame->currentIndex()+1)%3);code->setCurrentIndex(code->findData(2));
    for(const auto& label:{"Noncoding","Coding 1–2–3","Reset all coding annotations","Reset selected columns to frame","Generate translation mask","Annotate from mask…","Add translated sequences…","Translate to a new protein window…","Export protein sequences…"}){
        auto* a=action(w,QString::fromUtf8(label));need(!a->isEnabled(),"Translation mutation action disabled while busy");a->trigger();
    }
    need(w.current_document().state_id()==state&&w.current_document().translation_settings()==before,"Busy handlers cannot mutate document, including programmatic signals");
}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{
    QTemporaryDir dir;need(dir.isValid(),"Temporary folder");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());const auto fasta=dir.filePath("source Я.fasta");put(fasta,">row\nATGAAATAG\n");
    // Every kind of translation metadata must survive ordinary Save/reopen.
    for(int kind=0;kind<4;++kind){MainWindow w;open(app,w,fasta);action(w,"Translation")->setChecked(true);
        if(kind==0)w.findChild<QComboBox*>("translationFrame")->setCurrentIndex(1);
        if(kind==1){auto* combo=w.findChild<QComboBox*>("translationGeneticCode");combo->setCurrentIndex(combo->findData(2));}
        if(kind==2){view(w)->select_fragment({{1},3,3});action(w,"Noncoding")->trigger();}
        if(kind==3){QTimer custom;custom.setInterval(2);QObject::connect(&custom,&QTimer::timeout,[&]{if(auto* dialog=w.findChild<QDialog*>("customGeneticCodeDialog")){custom.stop();dialog->findChild<QLineEdit*>("customGeneticCodeName")->setText("Retained custom code");dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();}});custom.start();auto* combo=w.findChild<QComboBox*>("translationGeneticCode");combo->setCurrentIndex(combo->count()-1);until(app,[&]{return w.current_document().translation_settings().code==0;});}
        const auto expected=w.current_document().translation_settings();need(!(expected==TranslationSettings{}),"Metadata changed");const auto path=dir.filePath(QString("metadata_%1.kleio").arg(kind));
        {Picker picker(path);action(w,"&Save")->trigger();need(picker.chosen,"Ordinary Save asks for a session when metadata exists");check_busy(w);until(app,[&]{return view(w)->isEnabled();});need(picker.warnings==0,"Session save succeeds");}
        need(!w.windowTitle().contains('*'),"Successful session save clears dirty state");MainWindow reopened;open(app,reopened,path);need(reopened.current_document().translation_settings()==expected,"Translation settings round-trip exactly");reopened.close();w.close();
    }
    MainWindow w;open(app,w,fasta);action(w,"Translation")->setChecked(true);w.findChild<QComboBox*>("translationFrame")->setCurrentIndex(1);
    const auto entered=dir.filePath("destination.txt"),final=entered+".kleio";put(final,"sentinel");
    {Picker picker(entered);action(w,"Save &as…")->trigger();need(picker.replacements==1&&get(final)=="sentinel"&&w.windowTitle().contains('*'),"Rejecting replacement protects normalized destination and dirty state");}
    {Picker picker(entered,QMessageBox::Yes);action(w,"Save &as…")->trigger();check_busy(w);until(app,[&]{return view(w)->isEnabled();});need(picker.replacements==1&&get(final).startsWith(QByteArray(session_magic.data(),8)),"Final path replacement is confirmed once");}
    // Explicit FASTA export must not mark changed metadata saved.
    w.findChild<QComboBox*>("translationFrame")->setCurrentIndex(2);const auto exportPath=dir.filePath("only_sequences.fasta");
    {Picker picker(exportPath);action(w,"Export FASTA…")->trigger();check_busy(w);until(app,[&]{return view(w)->isEnabled();});need(get(exportPath).startsWith(">row")&&w.windowTitle().contains('*'),"Explicit FASTA export leaves session changes dirty");}
    action(w,"Alignment statistics…")->trigger();check_busy(w);until(app,[&]{return view(w)->isEnabled();});
    if(auto* statistics=w.findChild<QDialog*>("alignmentStatistics"))statistics->reject();
    bool grouped=false;QTimer grouping;grouping.setInterval(2);QObject::connect(&grouping,&QTimer::timeout,[&]{
        if(auto* dialog=w.findChild<QDialog*>("identityGroupsDialog")){grouping.stop();
            dialog->findChild<QPushButton*>("identityCalculate")->click();check_busy(w);grouped=true;dialog->reject();}
    });grouping.start();action(w,"Group by % identity…")->trigger();need(grouped&&view(w)->isEnabled(),"Grouping remains locked through cancellation");
    if(argc>2){
        const auto pair=dir.filePath("pair.fasta");put(pair,">first\nATGAAATAG\n>second\nATGAAATAG\n");MainWindow aligning;open(app,aligning,pair);
        QTimer settings;settings.setInterval(2);QObject::connect(&settings,&QTimer::timeout,[&]{if(auto* dialog=aligning.findChild<QDialog*>("mafftSettings")){
            settings.stop();dialog->findChild<QLineEdit*>("mafftLauncher")->setText(QString::fromLocal8Bit(argv[2]));dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        }});qputenv("KLEIOTIDE_FIXTURE_MODE","hang");settings.start();action(aligning,"Align whole alignment…")->trigger();
        until(app,[&]{auto* log=aligning.findChild<QPlainTextEdit*>("mafftLog");return log&&log->toPlainText().contains("fixture-align-ready");});check_busy(aligning);
        aligning.findChild<QDialog*>("mafftProgress")->reject();until(app,[&]{return view(aligning)->isEnabled();});qunsetenv("KLEIOTIDE_FIXTURE_MODE");aligning.close();
    }
    // The real Save-before-close continuation persists annotations before closing.
    MainWindow closing;open(app,closing,fasta);view(closing)->select_fragment({{1},0,3});action(closing,"Noncoding")->trigger();const auto closePath=dir.filePath("close.kleio");
    {Picker picker(closePath);closing.close();until(app,[&]{return !closing.isVisible();});need(picker.chosen&&picker.warnings==0,"Save-before-close completes");}
    MainWindow closedCopy;open(app,closedCopy,closePath);need(closedCopy.current_document().translation_settings().regions.size()==1,"Save-before-close retains annotation");closedCopy.close();
    // Save-before-open must persist the old document before replacing it.
    const auto nextFasta=dir.filePath("next.fasta"),beforeOpen=dir.filePath("before_open.kleio");put(nextFasta,">next\nACGT\n");
    MainWindow opening;open(app,opening,fasta);opening.findChild<QComboBox*>("translationFrame")->setCurrentIndex(2);
    {Picker picker(beforeOpen);opening.open_path(nextFasta);until(app,[&]{return view(opening)->isEnabled()&&!opening.current_document().rows().empty()&&opening.current_document().rows()[0].name=="next";});need(picker.chosen&&picker.warnings==0,"Save-before-open completes");}
    MainWindow previous;open(app,previous,beforeOpen);need(previous.current_document().translation_settings().frame==3,"Save-before-open retains old metadata");previous.close();opening.close();
    // Cancelling the Save dialog leaves the original FASTA and dirty metadata intact.
    MainWindow cancelled;open(app,cancelled,fasta);cancelled.findChild<QComboBox*>("translationFrame")->setCurrentIndex(1);bool cancelledPicker=false;
    QTimer cancelPicker;cancelPicker.setInterval(2);QObject::connect(&cancelPicker,&QTimer::timeout,[&]{if(auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget())){cancelPicker.stop();cancelledPicker=true;dialog->reject();}});cancelPicker.start();action(cancelled,"&Save")->trigger();
    need(cancelledPicker&&view(cancelled)->isEnabled()&&cancelled.windowTitle().contains('*')&&cancelled.current_document().translation_settings().frame==2&&get(fasta)==">row\nATGAAATAG\n","Cancelled Save preserves input and unsaved metadata");
    // Protein export shares content protection and uses atomic streaming output.
    Document proteins;proteins.load({{"nt","ATGAAATAG"},{"protein","MKT",RowKind::Protein}});const auto proteinSession=dir.filePath("proteins.kleio");session(proteinSession,proteins);MainWindow proteinWindow;open(app,proteinWindow,proteinSession);
    const auto original=get(proteinSession);
    {Picker picker(proteinSession,QMessageBox::Yes);action(proteinWindow,"Export protein sequences…")->trigger();need(picker.warnings==1&&get(proteinSession)==original,"Protein export cannot overwrite session");}
    const auto proteinOutput=dir.filePath("proteins.faa");put(proteinOutput,"old output");
    {Picker picker(proteinOutput);action(proteinWindow,"Export protein sequences…")->trigger();need(picker.replacements==1&&get(proteinOutput)=="old output","Protein export replacement can be cancelled");}
    {Picker picker(proteinOutput,QMessageBox::Yes);action(proteinWindow,"Export protein sequences…")->trigger();check_busy(proteinWindow);until(app,[&]{return view(proteinWindow)->isEnabled();});need(get(proteinOutput)==">protein\nMKT\n"&&picker.warnings==0,"Protein FASTA export completes");}
    // Single AB1 export also confirms the normalized path, and protects source evidence.
    const auto ab1=dir.filePath("source.ab1");put(ab1,QByteArray::fromStdString(test::synthetic_abif()));Document traces;std::istringstream input(test::synthetic_abif());traces.load({read_abif(input,"trace",ab1.toUtf8().toStdString())});
    const auto traceSession=dir.filePath("trace.kleio");session(traceSession,traces);MainWindow traceWindow;open(app,traceWindow,traceSession);
    const auto ab1Entered=dir.filePath("copy.txt"),ab1Final=ab1Entered+".ab1";put(ab1Final,"trace sentinel");
    int stage=0;QTimer exportTimer;exportTimer.setInterval(2);QObject::connect(&exportTimer,&QTimer::timeout,[&]{if(auto* dialog=traceWindow.findChild<QDialog*>("abifExportDialog")){auto* button=dialog->findChild<QPushButton*>("abifExportWrite");if(stage==0&&button->isEnabled()){stage=1;Picker picker(ab1Entered);button->click();need(picker.replacements==1&&get(ab1Final)=="trace sentinel","AB1 normalized destination protected by confirmation");stage=2;exportTimer.stop();dialog->reject();}}});exportTimer.start();action(traceWindow,"Export edited trace (AB1)…")->trigger();need(stage==2,"AB1 path workflow completed");traceWindow.close();proteinWindow.close();
    {Picker picker(final,QMessageBox::Yes);action(w,"&Save")->trigger();until(app,[&]{return view(w)->isEnabled();});}w.close();
    Document limitedDocument;std::vector<Record> limitedRows{{"long",std::string(1000000,'A')}};for(int i=0;i<1000;++i)limitedRows.push_back({"empty"+std::to_string(i),""});limitedDocument.load(std::move(limitedRows));const auto limitedPath=dir.filePath("limited.kleio");session(limitedPath,limitedDocument);
    MainWindow limited;open(app,limited,limitedPath);bool memoryWarning=false;QTimer acknowledgeMemory;acknowledgeMemory.setInterval(2);
    QObject::connect(&acknowledgeMemory,&QTimer::timeout,[&]{if(auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){memoryWarning=box->text().contains("working-memory limit");box->accept();}});acknowledgeMemory.start();
    auto* flush=limited.findChild<QAction*>("flushAlignment");need(flush,"Flush action");flush->trigger();acknowledgeMemory.stop();need(memoryWarning&&limited.current_document().sequence(2).symbols.text().empty()&&!limited.current_document().can_undo()&&view(limited)->isEnabled(),"Oversized Flush reports a warning and preserves the live document/history");limited.close();
    std::cout<<"PASS save safety: all translation metadata, final-path overwrite Yes/No, Save-before-close/open and cancellation, FASTA export dirty state, busy controls/handlers during save/statistics/grouping/MAFFT, protein export protection, AB1 normalized path\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}}
