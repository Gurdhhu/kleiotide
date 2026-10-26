#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
template<class F>void wait(F ready){QElapsedTimer timer;timer.start();while(!ready()&&timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(ready(),"Async timeout");QApplication::processEvents();}
Record traced(std::string bases,std::vector<std::uint8_t> q){std::istringstream input(test::synthetic_abif(bases));auto r=read_abif(input,"trace","missing.ab1");auto data=std::make_shared<Chromatogram>(*r.trace->original);data->quality=std::move(q);auto link=std::make_shared<TraceLink>(*r.trace);link->original=data;r.trace=link;return r;}
QAction* action(MainWindow& w,const QString& id){for(auto* a:w.findChildren<QAction*>())if(a->objectName()==id||a->text().section('\t',0,0)==id)return a;throw std::runtime_error("Missing action "+id.toStdString());}
template<class F>void dialog(MainWindow& w,const QString& id,F inspect){QString failure;bool called=false;QObject context;QTimer::singleShot(0,&context,[&]{called=true;auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());try{require(d,"Expected dialog");inspect(d);}catch(const std::exception& e){failure=e.what();if(d)QMetaObject::invokeMethod(d,"reject",Qt::DirectConnection);}});action(w,id)->trigger();require(called,"Modal callback");if(!failure.isEmpty())throw std::runtime_error(id.toStdString()+": "+failure.toStdString());}
QPushButton* button(QDialog* d,const char* name){auto* b=d->findChild<QPushButton*>(name);require(b,"Expected quality button");return b;}
void preview(QDialog* d){auto* b=button(d,"qualityPreviewButton");require(b->isEnabled(),"Preview available");b->click();require(!d->findChild<QComboBox*>("qualityScope")->isEnabled(),"Settings disabled during asynchronous preview");wait([&]{return b->isEnabled();});require(d->findChild<QLabel*>("qualityStatus")->text().contains("will change"),"Preview completed with summary");}
void settings(QDialog* d,int q,int window=3){d->findChild<QSpinBox*>("qualityThreshold")->setValue(q);d->findChild<QSpinBox*>("qualityWindow")->setValue(window);}
void reject(QDialog* d){QMetaObject::invokeMethod(d,"reject",Qt::DirectConnection);}
void save(MainWindow& w,const QString& path){dialog(w,"Save &as…",[&](QDialog* d){auto* file=qobject_cast<QFileDialog*>(d);require(file,"Filename chooser");file->selectFile(path);QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);});wait([&]{return w.centralWidget()->isEnabled();});}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{
    QTemporaryDir dir;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());Document source;
    source.load({traced("ACGTACGTAC",{0,0,30,30,30,30,30,30,0,0}),traced("ACGTAA",{5,30,30,30,30,0}),traced("ACGT",{}),{"unlinked","ACGT"},{"protein","ACGT",RowKind::Protein},{"mask","0123",RowKind::Mask},traced("ACGTACGT",{0,0,30,30,30,30,0,0})});source.transform({2},Document::Transform::ReverseComplement);source.replace(7,0,1,"T");source.apply_groups({{1,"Group",0x0072b2}},{{1,1},{2,1}});source.rename({{1,"trim_demo"},{2,"reverse_demo"},{3,"missing_Q_demo"},{7,"edited_demo"}});
    const auto input=dir.filePath("quality.kleio");{std::ofstream out(input.toStdString(),std::ios::binary);write_session(out,source);}if(argc>3){std::ofstream demo(argv[3],std::ios::binary);write_session(demo,source);require(bool(demo),"Synthetic quality example saved");}MainWindow w;w.resize(1280,850);w.show();w.open_path(input);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});const auto baseline=w.current_document().state_id();const auto link=w.current_document().sequence(1).trace;
    dialog(w,"trimQualityEnds",[&](QDialog* d){require(d->objectName()=="traceQualityDialog"&&!v->isEnabled(),"Quality modal keeps source immutable");auto* scope=d->findChild<QComboBox*>("qualityScope");require(scope->currentIndex()==0&&!button(d,"qualityPreviewButton")->isEnabled()&&!button(d,"qualityApply")->isEnabled(),"No name selection never means all traces");scope->setCurrentIndex(1);settings(d,25);preview(d);auto* table=d->findChild<QTableWidget*>("qualityPreview");require(table->rowCount()==7&&table->item(0,6)->text()=="4"&&table->item(2,1)->text().contains("No usable Q")&&table->item(4,1)->text().contains("Protein"),"Preview includes counts, missing Q and ineligible rows");require(button(d,"qualityApply")->isEnabled(),"Valid preview enables Apply");if(argc>1)require(d->grab().save(QString::fromLocal8Bit(argv[1])),"Trim preview screenshot");reject(d);});require(w.current_document().state_id()==baseline&&v->isEnabled(),"Cancel leaves source and history unchanged");
    v->select_ids({1,4,5,6});v->select_fragment({{2},0,2});dialog(w,"trimQualityEnds",[&](QDialog* d){settings(d,25);preview(d);auto* table=d->findChild<QTableWidget*>("qualityPreview");require(table->rowCount()==4&&table->item(0,7)->text()=="2"&&table->item(0,8)->text()=="2","Selected scope previews exact trim boundaries");button(d,"qualityApply")->click();});require(w.current_document().sequence(1).symbols.text()=="~~GTACGT~~"&&w.current_document().sequence(2).symbols.text()=="TTACGT"&&w.current_document().sequence(1).groupId==1&&w.current_document().sequence(1).trace->original==link->original,"Apply edits selected trace names only, preserving groups and evidence");action(w,"&Undo")->trigger();require(w.current_document().state_id()==baseline&&w.current_document().sequence(1).trace==link,"One Undo restores exact source link");action(w,"&Redo")->trigger();require(w.current_document().sequence(1).symbols.text()=="~~GTACGT~~","Redo trim");action(w,"&Undo")->trigger();
    v->select_ids({2});dialog(w,"lowQualityToN",[&](QDialog* d){settings(d,20);require(!d->findChild<QSpinBox*>("qualityWindow")->isVisible(),"N dialog has no window parameter");preview(d);require(d->findChild<QTableWidget*>("qualityPreview")->item(0,6)->text()=="2","Reverse target low-Q count");d->findChild<QSpinBox*>("qualityThreshold")->setValue(21);require(!button(d,"qualityApply")->isEnabled()&&d->findChild<QTableWidget*>("qualityPreview")->rowCount()==0,"Changed settings invalidate and clear stale preview");preview(d);if(argc>2)require(d->grab().save(QString::fromLocal8Bit(argv[2])),"N preview screenshot");button(d,"qualityApply")->click();});require(w.current_document().sequence(2).symbols.text()=="NTACGN"&&w.current_document().sequence(2).trace->reversed,"N operation uses reverse-strand original Q");action(w,"&Undo")->trigger();
    v->select_ids({7});dialog(w,"lowQualityToN",[&](QDialog* d){settings(d,20);auto* include=d->findChild<QCheckBox*>("qualityUseOriginalQ");require(!include->isChecked(),"Edited calls protected by default on each dialog");preview(d);auto* table=d->findChild<QTableWidget*>("qualityPreview");require(table->item(0,5)->text()=="1"&&table->item(0,6)->text()=="3","Default preview protects manually substituted base");include->setChecked(true);require(!button(d,"qualityApply")->isEnabled(),"Inclusion invalidates preview");preview(d);require(table->item(0,6)->text()=="4","Explicit original-Q inclusion changes preview");button(d,"qualityApply")->click();});require(w.current_document().sequence(7).symbols.text()=="NNGTACNN"&&!trace_position(w.current_document().sequence(7),0).quality,"Explicit original evidence does not invent current Q");action(w,"&Undo")->trigger();require(w.current_document().sequence(7).symbols.text()=="TCGTACGT","Undo restores manual substitution");
    v->select_ids({3});dialog(w,"lowQualityToN",[&](QDialog* d){settings(d,255);preview(d);require(!button(d,"qualityApply")->isEnabled()&&d->findChild<QTableWidget*>("qualityPreview")->item(0,4)->text()=="4","Missing quality is protected, not zero");reject(d);});
    v->select_ids({1});dialog(w,"trimQualityEnds",[&](QDialog* d){settings(d,255,1);preview(d);require(!button(d,"qualityApply")->isEnabled()&&d->findChild<QTableWidget*>("qualityPreview")->item(0,1)->text().contains("No complete passing"),"No passing window cannot silently erase a row");reject(d);});
    const auto beforeCancel=w.current_document().state_id();dialog(w,"trimQualityEnds",[&](QDialog* d){settings(d,25);button(d,"qualityPreviewButton")->click();reject(d);wait([&]{return !d->isVisible();});});require(v->isEnabled()&&w.current_document().state_id()==beforeCancel,"Cancellation waits for preview completion and unlocks unchanged source");
    v->select_ids({1});dialog(w,"trimQualityEnds",[&](QDialog* d){settings(d,25);preview(d);button(d,"qualityApply")->click();});v->select_ids({2});dialog(w,"lowQualityToN",[&](QDialog* d){settings(d,20);preview(d);button(d,"qualityApply")->click();});const auto output=dir.filePath("quality_saved.kleio");save(w,output);MainWindow reopened;reopened.show();reopened.open_path(output);auto* rv=dynamic_cast<AlignmentView*>(reopened.centralWidget());wait([&]{return rv->isEnabled();});require(reopened.current_document().sequence(1).symbols.text()=="~~GTACGT~~"&&reopened.current_document().sequence(2).symbols.text()=="NTACGN"&&reopened.current_document().sequence(1).trace->original->quality==link->original->quality&&reopened.current_document().groups()==source.groups(),"Native session retains quality edits, groups and original Q");reopened.close();w.close();
    std::cout<<"PASS 0.18: explicit scope, asynchronous preview/cancel, missing and edited Q policies, no destructive all-bad trim, settings invalidation, reverse evidence, atomic Apply/Undo/Redo, group/provenance and native roundtrip\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
