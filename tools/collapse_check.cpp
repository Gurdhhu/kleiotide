#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/collapse_dialog.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QKeySequence>
#include <QPushButton>
#include <QTableView>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
QAction* action(MainWindow& w,const char* name){auto* a=w.findChild<QAction*>(name);need(a,name);return a;}
QPushButton* button(QDialog& d,const char* name){auto* b=d.findChild<QPushButton*>(name);need(b,name);return b;}
QComboBox* combo(QDialog& d,const char* name){auto* c=d.findChild<QComboBox*>(name);need(c,name);return c;}
QTableView* table(QDialog& d,const char* name){auto* t=d.findChild<QTableView*>(name);need(t,name);return t;}
QString cell(QTableView* t,int r,int c){return t->model()->data(t->model()->index(r,c),Qt::DisplayRole).toString();}
std::string saved(const Document& d){std::ostringstream out(std::ios::binary);write_session(out,d);return out.str();}
template<class F>void until(QApplication& app,F f){QElapsedTimer elapsed;elapsed.start();while(!f()&&elapsed.elapsed()<20000){app.processEvents();QThread::msleep(1);}need(f(),"GUI timeout");}
template<class F>void workflow(MainWindow& w,F step){bool done=false;QString error;QElapsedTimer elapsed;elapsed.start();QTimer timer;timer.setInterval(2);QObject::connect(&timer,&QTimer::timeout,[&]{try{need(elapsed.elapsed()<20000,"Dialog timeout");if(auto* d=w.findChild<QDialog*>("collapseDialog")){if(step(*d)){done=true;timer.stop();}}}catch(const std::exception& e){error=e.what();timer.stop();if(auto* d=w.findChild<QDialog*>("collapseDialog"))d->reject();}});timer.start();action(w,"collapseIdentical")->trigger();need(error.isEmpty(),qPrintable(error));need(done,"Workflow finished");}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir directory;std::istringstream a(test::synthetic_abif()),b(test::synthetic_abif());auto trace1=read_abif(a,"trace1","one.ab1"),trace2=read_abif(b,"trace2","two.ab1");auto protein=std::make_shared<ProteinLayout>();protein->sourceName="original DNA";Document source;source.load({{"first","A--C"},{"unique","G"},{"second","a~C"},{"third","AC~"},{"protein1","M~EF*",RowKind::Protein},{"protein2","MEF*",RowKind::Protein,{},RowRepresentation::AlignedSymbols,0,protein},{"mask1","12~3",RowKind::Mask},{"mask2","123",RowKind::Mask},trace1,trace2});source.apply_groups({{1,"A",0x0072b2},{2,"B",0xe69f00}},{{1,1},{3,1},{4,2},{5,1},{6,2},{9,1},{10,2}});const auto path=directory.filePath("collapse.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,source);}
    MainWindow window;window.resize(1450,960);window.show();window.open_path(path);auto* view=dynamic_cast<AlignmentView*>(window.centralWidget());until(app,[&]{return view->isEnabled()&&window.current_document().rows().size()==10;});const auto before=saved(window.current_document());const auto retainedTrace=window.current_document().sequence(10).trace;const auto retainedProtein=window.current_document().sequence(6).protein;int stage=0;
    workflow(window,[&](QDialog& d){auto* apply=button(d,"collapseApply");if(!apply->isEnabled())return false;auto* sets=table(d,"collapseSets");auto* members=table(d,"collapseMembers");need(sets->model()->rowCount()==3,"Three biological duplicate sets; masks skipped");need(cell(sets,0,5)=="first;second;third","Default concatenation preview");
        sets->selectRow(0);members->selectRow(2);need(cell(members,2,6)=="AC~","Gap pattern is visible");button(d,"collapseKeep")->click();need(cell(sets,0,2).contains("third")&&cell(sets,0,3)=="B","Chosen representative/group preview");
        sets->selectRow(1);members->selectRow(1);button(d,"collapseKeep")->click();sets->selectRow(2);members->selectRow(1);button(d,"collapseKeep")->click();need(cell(sets,2,4).contains("two.ab1"),"Chosen trace path visible");
        combo(d,"collapseNaming")->setCurrentIndex(1);need(cell(sets,0,5)=="third_x3"&&!d.findChild<QLineEdit*>("collapseSeparator")->isEnabled(),"Count includes representative; separator inactive");combo(d,"collapseNaming")->setCurrentIndex(0);d.findChild<QLineEdit*>("collapseSeparator")->setText(" | ");need(cell(sets,0,5)=="third | first | second","Representative-first custom delimiter");d.layout()->activate();if(argc>1)need(d.grab().save(QString::fromLocal8Bit(argv[1])),"Preview screenshot");apply->click();stage=1;return true;});
    need(stage==1&&window.current_document().rows().size()==6,"Menu apply removed all duplicate members");need(window.current_document().sequence(4).name=="third | first | second"&&window.current_document().sequence(4).symbols.text()=="AC~"&&window.current_document().sequence(4).groupId==2,"Chosen gap layout, case and group retained");need(window.current_document().sequence(10).trace==retainedTrace&&window.current_document().sequence(6).protein==retainedProtein,"Chosen immutable evidence/provenance retained");const auto after=saved(window.current_document());
    // Find the standard Undo/Redo actions by shortcut rather than display labels.
    auto standard=[&](const QKeySequence& key){for(auto* a:window.findChildren<QAction*>())if(a->shortcut()==key)return a;throw std::runtime_error("Missing undo/redo action");};
    standard(QKeySequence::Undo)->trigger();need(saved(window.current_document())==before&&!window.current_document().can_undo(),"One Undo restores every row/name/group/trace");standard(QKeySequence::Redo)->trigger();need(saved(window.current_document())==after,"One Redo repeats exact collapse");standard(QKeySequence::Undo)->trigger();
    // Selected scope never consumes an unselected identical sequence.
    view->select_ids({1,3});stage=0;workflow(window,[&](QDialog& d){if(stage==0&&button(d,"collapsePreview")->isEnabled()){combo(d,"collapseScope")->setCurrentIndex(1);combo(d,"collapseNaming")->setCurrentIndex(1);button(d,"collapsePreview")->click();stage=1;}else if(button(d,"collapseApply")->isEnabled()){auto* sets=table(d,"collapseSets");need(sets->model()->rowCount()==1&&cell(sets,0,5)=="first_x2","Selected-only count preview");button(d,"collapseApply")->click();return true;}return false;});need(window.current_document().rows().size()==9&&window.current_document().sequence(1).name=="first_x2"&&window.current_document().sequence(4).name=="third","Unselected duplicate untouched");standard(QKeySequence::Undo)->trigger();need(saved(window.current_document())==before,"Selected-only Undo");
    stage=0;workflow(window,[&](QDialog& d){if(stage==0&&button(d,"collapsePreview")->isEnabled()){d.findChild<QCheckBox*>("collapseMasks")->setChecked(true);button(d,"collapsePreview")->click();stage=1;}else if(button(d,"collapseApply")->isEnabled()){need(table(d,"collapseSets")->model()->rowCount()==4,"Mask opt-in adds a separate mask set");d.reject();return true;}return false;});need(saved(window.current_document())==before,"Cancel is read-only");
    // A no-duplicate scope has no Apply action.
    view->select_ids({1,2});stage=0;workflow(window,[&](QDialog& d){if(stage==0&&button(d,"collapsePreview")->isEnabled()){combo(d,"collapseScope")->setCurrentIndex(1);button(d,"collapsePreview")->click();stage=1;}else if(button(d,"collapsePreview")->isEnabled()){need(!button(d,"collapseApply")->isEnabled()&&table(d,"collapseSets")->model()->rowCount()==0,"No matches disable Apply");d.reject();return true;}return false;});
    // Stale source detection is independent of the window lock.
    auto stale=std::make_shared<Document>();stale->load({{"one","A"},{"two","A"}});QString error;QTimer timer;timer.setInterval(2);stage=0;QObject::connect(&timer,&QTimer::timeout,[&]{if(auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());d&&d->objectName()=="collapseDialog"){if(stage==0&&button(*d,"collapseApply")->isEnabled()){stale->rename({{1,"external rename"}});stage=1;button(*d,"collapseApply")->click();}else if(stage==1&&d->findChild<QLabel*>("collapseStatus")->text().contains("alignment changed",Qt::CaseInsensitive)){stage=2;timer.stop();d->reject();}}});timer.start();need(!collapse_sequences_dialog(stale,{},&window)&&stage==2&&stale->rows().size()==2,"Stale preview cannot collapse");
    auto large=std::make_shared<Document>();std::vector<Record> rows;for(int i=0;i<100;++i)rows.push_back({"sequence"+std::to_string(i),std::string(100000,'A')});large->load(rows);std::size_t ticks=0;QTimer heartbeat;heartbeat.setInterval(1);QObject::connect(&heartbeat,&QTimer::timeout,[&]{++ticks;});heartbeat.start();QTimer cancel;cancel.setInterval(1);QObject::connect(&cancel,&QTimer::timeout,[&]{if(auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());d&&d->objectName()=="collapseDialog"){cancel.stop();d->reject();}});cancel.start();need(!collapse_sequences_dialog(large,{},&window)&&large->rows().size()==100&&!large->can_undo()&&ticks>0,"Cancellation stays responsive and read-only");
    std::cout<<"PASS collapse GUI: lazy preview, custom representative/gaps/group/trace/protein metadata, concatenation, count, selected scope, mask opt-in, no-op, Cancel, stale rejection and one-step Undo/Redo\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
