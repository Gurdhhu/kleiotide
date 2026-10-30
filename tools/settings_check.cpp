#include "test_preferences.hpp"
#include "gui/app_settings.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/mafft_dialog.hpp"
#include "gui/recovery.hpp"
#include "core/session.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
template<class F>void wait(F f){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}need(f(),"Async timeout");QApplication::processEvents();}
QAction* action(MainWindow& w,const char* id){auto* a=w.findChild<QAction*>(id);need(a,"Action exists");return a;}
AlignmentView* view(MainWindow& w){auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());need(v,"Alignment view exists");return v;}
template<class F>void settings(MainWindow& w,F inspect){QString error;QTimer::singleShot(0,[&]{auto* d=dynamic_cast<QDialog*>(QApplication::activeModalWidget());try{need(d&&d->objectName()=="appSettings","Settings dialog");inspect(d);}catch(const std::exception& e){error=e.what();if(d)d->reject();}});action(w,"appSettingsAction")->trigger();if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
void overwrite(MainWindow& w){auto& v=*view(w);v.select_fragment({{w.current_document().rows()[0].id},0,1});QKeyEvent key(QEvent::KeyPress,Qt::Key_T,Qt::NoModifier,"T");QApplication::sendEvent(&v,&key);QApplication::processEvents();}
void close_dirty(MainWindow* w){QTimer::singleShot(0,[]{if(auto* d=dynamic_cast<QMessageBox*>(QApplication::activeModalWidget()))d->done(QMessageBox::Discard);});w->close();QApplication::processEvents();}
MainWindow* recover(MainWindow& owner,bool copy){std::vector<MainWindow*> previous;for(auto* widget:QApplication::topLevelWidgets())if(auto* w=dynamic_cast<MainWindow*>(widget))previous.push_back(w);QString error;QTimer::singleShot(0,[&]{auto* d=dynamic_cast<QDialog*>(QApplication::activeModalWidget());try{need(d&&d->objectName()=="recoveryChooser","Recovery chooser");auto* b=d->findChild<QPushButton*>(copy?"openRecoveryCopy":"recoverSelected");need(b,"Recovery button");b->click();}catch(const std::exception& e){error=e.what();if(d)d->reject();}});owner.offer_recovery();if(!error.isEmpty())throw std::runtime_error(error.toStdString());MainWindow* result=nullptr;for(auto* widget:QApplication::topLevelWidgets())if(auto* w=dynamic_cast<MainWindow*>(widget);w&&std::find(previous.begin(),previous.end(),w)==previous.end())result=w;need(result,"Recovery opens separate window");wait([&]{return view(*result)->isEnabled();});return result;}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);app.setApplicationName("Kleiotide-settings-test-"+QUuid::createUuid().toString(QUuid::WithoutBraces));try{kleiotide::TestPreferences testPreferences;
    QTemporaryDir temp;need(temp.isValid(),"Test directory");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());
    auto& service=AppSettings::instance();service.reload();auto defaults=AppPreferences::defaults();defaults.recoveryEnabled=false;if(argc>2)defaults.mafftLauncher=QString::fromLocal8Bit(argv[2]);need(service.apply(defaults),"Initial defaults");
    const auto fasta=temp.filePath("fresh.fasta");{QFile file(fasta);need(file.open(QIODevice::WriteOnly),"Write FASTA");file.write(">one\nAAACCCGGGTAA\n>two\nAAACCCGGGTAA\n");}
    auto existing=std::make_unique<MainWindow>();existing->show();existing->open_path(fasta);wait([&]{return view(*existing)->isEnabled();});overwrite(*existing);
    const auto state=existing->current_document().state_id();const auto text=existing->current_document().rows()[0].symbols.text();need(existing->windowTitle().contains("*"),"Edited source is unsaved");
    settings(*existing,[&](QDialog* d){action(*existing,"checkpointNow")->trigger();need(view(*existing)->isEnabled(),"Modal dialogs defer checkpointing");d->findChild<QSpinBox*>("settingsFontSize")->setValue(14);d->findChild<QComboBox*>("settingsPalette")->setCurrentIndex(int(ResiduePalette::Chromas));auto* code=d->findChild<QComboBox*>("settingsCode");code->setCurrentIndex(code->findData(2));d->findChild<QComboBox*>("settingsViewMode")->setCurrentIndex(1);d->findChild<QComboBox*>("settingsEditMode")->setCurrentIndex(1);d->findChild<QSpinBox*>("settingsBulkMiB")->setValue(32);d->findChild<QSpinBox*>("settingsOverviewMiB")->setValue(1);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();d->reject();});
    need(service.values().font.pointSize()==14&&service.values().code==2,"Apply persists even when subsequently cancelled");
    need(view(*existing)->font().pointSize()==11&&view(*existing)->residue_palette()==ResiduePalette::Classic&&existing->current_document().translation_settings().code==1&&existing->current_document().state_id()==state&&existing->current_document().rows()[0].symbols.text()==text,"Defaults never change an existing document");
    need(existing->current_document().bulk_edit_memory_limit()==32*1024*1024,"Operational limit updates existing window");
    MainWindow fresh;fresh.show();need(fresh.current_document().translation_settings().code==2&&!fresh.current_document().can_undo()&&!fresh.windowTitle().contains("*"),"New blank document uses clean default code");fresh.open_path(fasta);wait([&]{return view(fresh)->isEnabled();});need(view(fresh)->font().pointSize()==14&&view(fresh)->residue_palette()==ResiduePalette::Chromas&&view(fresh)->mode()==AlignmentView::Mode::View&&fresh.current_document().translation_settings().code==2,"Fresh import uses global appearance/mode/code");
    Document native;native.load({{"saved","ACGT"}});QFont savedFont=defaults.font;savedFont.setPointSize(9);QJsonObject savedView{{"version",1},{"font",savedFont.toString()},{"residuePalette","okabe-ito"},{"mode",2}};
    const auto nativePath=temp.filePath("saved.kleio");{std::ofstream out(std::filesystem::path(nativePath.toStdU16String()),std::ios::binary);write_session(out,native,QJsonDocument(savedView).toJson().toStdString());}fresh.open_path(nativePath);wait([&]{return view(fresh)->isEnabled();});need(view(fresh)->font().pointSize()==9&&view(fresh)->residue_palette()==ResiduePalette::OkabeIto&&fresh.current_document().translation_settings().code==1,"Saved session overrides new defaults");
    const auto legacy=temp.filePath("legacy.kleio");{std::ofstream out(std::filesystem::path(legacy.toStdU16String()),std::ios::binary);write_session(out,native);}fresh.open_path(legacy);wait([&]{return view(fresh)->isEnabled();});need(view(fresh)->font().pointSize()==11&&view(fresh)->residue_palette()==ResiduePalette::Classic&&view(fresh)->mode()==AlignmentView::Mode::Rewrite,"Legacy session defaults remain independent of globals");
    settings(*existing,[&](QDialog* d){d->findChild<QLineEdit*>("settingsBlastEmail")->setText("settings@example.org");d->findChild<QComboBox*>("settingsBlastNdb")->setCurrentText("refseq_rna");d->findChild<QSpinBox*>("settingsBlastHits")->setValue(25);d->findChild<QSpinBox*>("settingsLocalBlastThreads")->setValue(3);d->findChild<QLineEdit*>("settingsMakeblastdb")->setText("makeblastdb-path");d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();d->reject();});service.reload();need(service.values().blastEmail=="settings@example.org"&&service.values().blastNucleotideDatabase=="refseq_rna"&&service.values().blastMaxHits==25&&service.values().localBlastThreads==3&&service.values().makeblastdbLauncher=="makeblastdb-path","BLAST defaults persist across reload");
    const auto persisted=service.values();
    settings(*existing,[&](QDialog* d){d->findChild<QPushButton*>("settingsUseAppearance")->click();need(d->findChild<QSpinBox*>("settingsFontSize")->value()==11&&d->findChild<QComboBox*>("settingsPalette")->currentIndex()==int(ResiduePalette::Classic),"Current appearance fills draft");d->findChild<QPushButton*>("settingsUseCode")->click();need(d->findChild<QComboBox*>("settingsCode")->currentData().toUInt()==1,"Current code fills draft");d->reject();});need(service.values()==persisted,"Draft copy does not commit implicitly");
    settings(*existing,[&](QDialog* d){d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::RestoreDefaults)->click();d->reject();});need(service.values()==persisted,"Restore defaults is draft-only until Apply/OK");
    settings(*existing,[&](QDialog* d){if(argc>1){QDir().mkpath(QString::fromLocal8Bit(argv[1]));auto* tabs=d->findChild<QTabWidget*>();for(int i=0;i<tabs->count();++i){tabs->setCurrentIndex(i);QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);QApplication::processEvents();need(d->grab().save(QDir(QString::fromLocal8Bit(argv[1])).filePath("settings-"+QString::number(i)+".png")),"Settings capture");}}d->reject();});
    if(argc>2){QTimer::singleShot(0,[&]{auto* d=dynamic_cast<QDialog*>(QApplication::activeModalWidget());d->findChild<QSpinBox*>("mafftThreads")->setValue(4);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});auto chosen=choose_mafft(existing.get(),{});need(chosen&&chosen->options.threads==4&&service.values().mafftThreads==0,"Per-job MAFFT overrides do not replace global defaults");
        settings(*existing,[&](QDialog* d){QTimer dismissFailure;dismissFailure.setInterval(10);QObject::connect(&dismissFailure,&QTimer::timeout,d,[]{if(auto* progress=dynamic_cast<QDialog*>(QApplication::activeModalWidget()))if(progress->objectName()=="mafftProgress")if(auto* close=progress->findChild<QPushButton*>("mafftCancel");close&&close->text()=="Close")close->click();});dismissFailure.start();d->findChild<QPushButton*>("settingsMafftTest")->click();dismissFailure.stop();need(d->findChild<QPlainTextEdit*>("settingsMafftReport")->toPlainText().startsWith("Small alignment succeeded."),"MAFFT Test performs an alignment");d->reject();});}
    action(*existing,"checkpointNow")->trigger();need(!view(*existing)->isEnabled(),"Checkpoint gates source mutations");wait([&]{return view(*existing)->isEnabled();});need(existing->current_document().state_id()==state&&existing->current_document().rows()[0].symbols.text()==text&&existing->windowTitle().contains("*"),"Checkpoint retains document/dirty state");need(!QDir(RecoveryStore::default_directory()).entryList({"*.kleio"},QDir::Files).empty(),"Checkpoint committed");
    existing.reset();need(RecoveryStore::available().size()==1,"Unclean destruction leaves recoverable checkpoint");
    auto* copied=recover(fresh,true);need(copied->current_document().rows()[0].symbols.text()==text&&copied->windowTitle().contains("*"),"Recovered copy preserves edits and remains unsaved");close_dirty(copied);need(RecoveryStore::available().size()==1,"Open copy keeps old recovery");
    auto* resumed=recover(fresh,false);need(RecoveryStore::available().empty(),"Recover claims original checkpoint without deleting it");action(*resumed,"checkpointNow")->trigger();wait([&]{return view(*resumed)->isEnabled();});need(QDir(RecoveryStore::default_directory()).entryList({"*.kleio"},QDir::Files).size()==1,"New valid checkpoint replaces claimed recovery safely");close_dirty(resumed);need(RecoveryStore::available().empty(),"Confirmed clean close/discard retires recovery");
    fresh.close();QDir(RecoveryStore::default_directory()).removeRecursively();std::cout<<"PASS Settings/recovery workflow: precedence, Apply/Cancel/reset, per-job MAFFT, real test fixture, checkpoint gating, crash copy and safe recovery lifecycle\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
