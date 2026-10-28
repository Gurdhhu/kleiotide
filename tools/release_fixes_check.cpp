#include "test_preferences.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/alignment_overview.hpp"
#include "gui/app_settings.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class T>T* child(QObject& p,const char* name){auto* v=p.findChild<T*>(name);need(v,name);return v;}
template<class F>void until(QApplication& app,F done){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<10000){app.processEvents();QThread::msleep(1);}need(done(),"GUI timeout");}
void put(const QString& path,const QByteArray& data){QFile f(path);need(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Write fixture");}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    TestPreferences preferences;QTemporaryDir tmp;MainWindow window;window.resize(1600,900);window.show();
    auto* view=dynamic_cast<AlignmentView*>(window.centralWidget());need(view,"Alignment view");
    auto* settings=child<QAction>(window,"appSettingsAction");auto* file=qobject_cast<QMenu*>(settings->parent());need(file,"File menu");const auto actions=file->actions();const auto i=actions.indexOf(settings);need(i>0&&actions[i-1]->isSeparator()&&actions[i+1]->isSeparator()&&actions[i+2]->text().contains("xit"),"Settings has its own section immediately before Exit");
    need(child<QAction>(window,"checkpointNow")->text().contains("testing only"),"Manual checkpoint is labeled testing only");
    const auto* toolbars=child<QMenu>(window,"toolbarsMenu");need(toolbars->actions().contains(child<QAction>(window,"alignmentOverviewView"))&&toolbars->actions().contains(child<QAction>(window,"chromatogramView")),"View panels grouped under Toolbars");
    need(child<QToolButton>(window,"rewriteToggle")->text()=="Overwrite","Overwrite label");
    const auto fasta=tmp.filePath("a Я.fasta"),ab1=tmp.filePath("trace Я.ab1");put(fasta,">existing\nACGT\n");put(ab1,QByteArray::fromStdString(test::synthetic_abif("ACGT")));
    window.open_path(fasta);until(app,[&]{return view->isEnabled()&&window.current_document().rows().size()==1;});
    QMimeData mime;mime.setUrls({QUrl::fromLocalFile(ab1),QUrl::fromLocalFile(fasta)});QDragEnterEvent enter(QPoint(50,150),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&window,&enter);need(enter.isAccepted(),"Local FASTA/AB1 batch accepted");
    QDropEvent drop(QPointF(50,150),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&window,&drop);need(drop.isAccepted(),"Drop appended through import path");until(app,[&]{return view->isEnabled()&&window.current_document().rows().size()==3;});
    need(window.current_document().rows()[0].name=="existing"&&window.current_document().rows()[1].trace&&window.current_document().rows()[2].name=="existing","Drop preserves existing rows and AB1 evidence");
    for(auto* a:window.findChildren<QAction*>())if(a->shortcut()==QKeySequence::Undo){a->trigger();break;}need(window.current_document().rows().size()==1,"Batch drop is one Undo");
    QMimeData remote;remote.setUrls({QUrl("https://example.org/a.fasta")});QDragEnterEvent rejected(QPoint(0,0),Qt::CopyAction,&remote,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&window,&rejected);need(!rejected.isAccepted(),"Remote URLs are not imported");
    const auto baseline=AppSettings::instance().values();const auto priorPalette=view->residue_palette();bool applied=false;QString failure;QTimer timer;timer.setInterval(2);
    QObject::connect(&timer,&QTimer::timeout,[&]{if(auto* dialog=window.findChild<QDialog*>("appSettings")){try{auto* palette=child<QComboBox>(*dialog,"settingsPalette");palette->setCurrentIndex((int(priorPalette)+1)%residue_palette_count);child<QPushButton>(*dialog,"settingsApplyAppearance")->click();need(view->residue_palette()!=priorPalette&&AppSettings::instance().values().palette==baseline.palette,"Explicit session appearance does not write defaults");dialog->reject();applied=true;}catch(const std::exception& e){failure=e.what();dialog->reject();}timer.stop();}});
    timer.start();settings->trigger();need(applied&&failure.isEmpty(),qPrintable(failure.isEmpty()?"Session appearance action":failure));need(view->residue_palette()!=priorPalette,"Explicit appearance remains after Cancel");
    auto* bar=child<QToolBar>(window,"editingToolbar");auto* translation=child<QToolBar>(window,"translationToolbar");auto* toggle=child<QAction>(window,"translationToggle");child<QAction>(window,"restoreDefaultLayout")->trigger();app.processEvents();need(translation->isHidden(),"Layout restore respects disabled translation");toggle->setChecked(true);
    for(int repeat=0;repeat<4;++repeat){child<QAction>(window,"restoreDefaultLayout")->trigger();app.processEvents();need(translation->geometry().top()>bar->geometry().bottom(),"Translation stays on a separate lower row after restoring layout");need(!translation->isFloatable(),"Translation cannot float");window.resize(repeat%2?1000:1800,900);app.processEvents();need(translation->geometry().top()>bar->geometry().bottom(),"Translation row survives narrow windows");}
    window.resize(std::max(1400,bar->sizeHint().width()+500),900);app.processEvents();auto* button=bar->widgetForAction(toggle);auto* palette=child<QToolButton>(window,"residuePaletteButton");need(button&&button->geometry().left()>=palette->geometry().right()&&button->geometry().left()-palette->geometry().right()<50,"Translation toggle stays beside palette before stretch");
    auto* overview=child<AlignmentOverview>(window,"alignmentOverview");until(app,[&]{return overview->ready();});view->select_fragment({{window.current_document().rows()[0].id},1,2});app.processEvents();
    if(argc>1)need(window.grab().save(QString::fromLocal8Bit(argv[1])+"/release_fixes.png"),"Preview capture");
    bool about=false;QTimer dismiss;dismiss.setInterval(2);QObject::connect(&dismiss,&QTimer::timeout,[&]{if(auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){about=box->text().contains("https://github.com/Gurdhhu/kleiotide")&&box->text().contains("Suggested citation");box->accept();dismiss.stop();}});dismiss.start();child<QAction>(window,"aboutKleiotide")->trigger();need(about,"About contains GitHub link and versioned citation");
    std::cout<<"PASS 0.27.1: File/View menus, Overwrite, local batch drop with evidence and one Undo, session-only appearance, separate translation row after restore/resize, adjacent toggle, About links/citation\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
