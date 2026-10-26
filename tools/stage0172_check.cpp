#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class F>void wait(F ready){QElapsedTimer timer;timer.start();while(!ready()&&timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(ready(),"Async timeout");QApplication::processEvents();}
QAction* action(MainWindow& w,const QString& id){for(auto* a:w.findChildren<QAction*>())if(a->objectName()==id||a->text().section('\t',0,0)==id)return a;throw std::runtime_error("Missing action "+id.toStdString());}
void key(AlignmentView& view,int code,Qt::KeyboardModifiers modifiers=Qt::NoModifier){view.activateWindow();view.setFocus();QApplication::processEvents();QKeyEvent press(QEvent::KeyPress,code,modifiers);QApplication::sendEvent(&view,&press);QKeyEvent release(QEvent::KeyRelease,code,modifiers);QApplication::sendEvent(&view,&release);QApplication::processEvents();}
QAction* menu_action(QMenu* menu,const QString& text){for(auto* a:menu->actions())if(a->text().section('\t',0,0)==text)return a;return nullptr;}
template<class F>void context(AlignmentView& view,QPoint point,F inspect){QString failure;bool called=false;QObject timer;
    QTimer::singleShot(0,&timer,[&]{called=true;auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());try{require(menu,"Context menu opened");inspect(menu);}catch(const std::exception& e){failure=e.what();}if(menu)menu->close();});
    QContextMenuEvent event(QContextMenuEvent::Mouse,point,view.viewport()->mapToGlobal(point));QApplication::sendEvent(view.viewport(),&event);require(called,"Context callback ran");if(!failure.isEmpty())throw std::runtime_error(failure.toStdString());
}
QPoint cell(AlignmentView& v,int row,int column){return {v.name_area_width()+column*v.cell_width()+v.cell_width()/2,v.ruler_height()+row*v.row_height()+v.row_height()/2};}
void save(MainWindow& w,const QString& path){QString failure;QTimer::singleShot(0,[&]{auto* file=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());if(!file){failure="Save dialog missing";return;}file->selectFile(path);QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);});action(w,"Save &as…")->trigger();require(failure.isEmpty(),"Save dialog");wait([&]{return w.centralWidget()->isEnabled();});}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{
    QTemporaryDir dir;std::istringstream ab1(test::synthetic_abif());Document d;d.load({{"DNA1","A-C~G--T"},{"DNA2","acgt"},{"protein","A-CGT",RowKind::Protein},{"mask","0-123",RowKind::Mask},read_abif(ab1,"trace","gone.ab1")});d.apply_groups({{1,"Old",0x0072b2}},{{1,1},{3,1}});
    const auto input=dir.filePath("source.kleio");{std::ofstream out(input.toStdString(),std::ios::binary);write_session(out,d);}MainWindow w;w.resize(1280,850);w.show();w.open_path(input);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});
    auto* toggle=w.findChild<QToolButton*>("editToggle");auto* viewButton=w.findChild<QToolButton*>("viewToggle");auto* mode=w.findChild<QWidget*>("rewriteInsertSwitch");auto* rewrite=w.findChild<QToolButton*>("rewriteToggle");auto* insert=w.findChild<QToolButton*>("insertToggle");auto* right=w.findChild<QComboBox*>("rightClickMode");
    require(toggle&&viewButton&&mode&&right&&toggle->isChecked()&&v->mode()==AlignmentView::Mode::Rewrite,"Edit/Rewrite default");require(rewrite&&insert&&rewrite->isChecked()&&rewrite->text()=="Rewrite"&&insert->text()=="Insert","Rewrite/Insert switch");
    auto setMode=[&](AlignmentView::Mode target){if(target==AlignmentView::Mode::View){viewButton->click();}else{if(!toggle->isChecked())toggle->click();(target==AlignmentView::Mode::Insert?insert:rewrite)->click();}require(v->mode()==target,"Mode controls set viewport mode");};
    require(toggle->text()=="Edit"&&viewButton->text()=="View"&&toggle->isVisible()&&viewButton->isVisible()&&!viewButton->isChecked(),"Both segments are visible with Edit selected");
    toggle->click();require(toggle->isChecked()&&!viewButton->isChecked()&&v->mode()==AlignmentView::Mode::Rewrite,"Clicking active segment keeps its state");
    const std::pair<const char*,QKeySequence> shortcuts[]={{"copyFragment",QKeySequence::Copy},{"cutFragment",QKeySequence::Cut},{"pasteFragment",QKeySequence::Paste}};
    for(const auto& [id,shortcut]:shortcuts){require(action(w,id)->shortcut()==shortcut,"Fragment shortcut binding");int count=0;for(auto* a:w.findChildren<QAction*>())if(a->shortcut()==shortcut)++count;require(count==1,"Exactly one action per clipboard shortcut");}
    for(auto id:{"copySequences","cutSequences","pasteSequences"})require(action(w,id)->shortcut().isEmpty(),"Whole sequences have no fragment shortcuts");
    const std::vector<SequenceId> names={2,3};v->select_ids(names);
    for(auto current:{AlignmentView::Mode::View,AlignmentView::Mode::Insert,AlignmentView::Mode::Rewrite}){
        setMode(current);right->setCurrentIndex(0);v->select_fragment({{1},0,3});const auto before=w.current_document().state_id();
        context(*v,cell(*v,0,1),[&](QMenu* menu){require(menu_action(menu,"Copy")&&menu_action(menu,"Copy")->isEnabled(),"Copy available in every mode with names selected");const bool writable=current==AlignmentView::Mode::Insert||current==AlignmentView::Mode::Rewrite;
            require(bool(menu_action(menu,"Cut"))==writable&&bool(menu_action(menu,"Paste"))==writable,"Cut/Paste listed only for Insert/Overwrite");if(writable){require(menu_action(menu,"Cut")->isEnabled(),"Cut accepts any one-row span");require(menu_action(menu,"Paste")->isEnabled()==(current==AlignmentView::Mode::Rewrite),"Insert wide selection cannot paste");}menu_action(menu,"Copy")->trigger();});
        require(QApplication::clipboard()->text()=="A-C"&&v->selected_ids()==names&&w.current_document().state_id()==before,"Fragment copy is independent of names and mode");
        QApplication::clipboard()->setText("sentinel");key(*v,Qt::Key_C,Qt::ControlModifier);require(QApplication::clipboard()->text()=="A-C","Actual Ctrl+C copies only fragment");
        v->select_fragment({{1,2},0,2});context(*v,cell(*v,0,0),[&](QMenu* menu){require(!menu_action(menu,"Copy")->isEnabled(),"Block Copy disabled");if(auto* a=menu_action(menu,"Cut"))require(!a->isEnabled(),"Block Cut disabled");if(auto* a=menu_action(menu,"Paste"))require(!a->isEnabled(),"Block Paste disabled");});
        QApplication::clipboard()->setText("AC");key(*v,Qt::Key_X,Qt::ControlModifier);key(*v,Qt::Key_V,Qt::ControlModifier);require(w.current_document().state_id()==before&&QApplication::clipboard()->text()=="AC","Block shortcuts leave data and clipboard unchanged");
    }
    setMode(AlignmentView::Mode::Insert);right->setCurrentIndex(1);setMode(AlignmentView::Mode::View);require(right->count()==1&&!right->isEnabled()&&!mode->isEnabled()&&right->currentText()=="Right click: menu","View has only menu right-click behavior");
    v->set_right_click(AlignmentView::RightClick::InsertGap);v->select_fragment({{1},0,1});const auto readonly=w.current_document().state_id();const auto point=cell(*v,0,0);
    QMouseEvent rightPress(QEvent::MouseButtonPress,QPointF(point),QPointF(v->viewport()->mapToGlobal(point)),Qt::RightButton,Qt::RightButton,Qt::NoModifier);QApplication::sendEvent(v->viewport(),&rightPress);require(w.current_document().state_id()==readonly,"View rejects direct right-click gap editing");
    context(*v,point,[&](QMenu* menu){require(menu_action(menu,"Copy")&&!menu_action(menu,"Lock gaps in selection")&&!menu_action(menu,"Unlock gaps in selection")&&!action(w,"lockSelection")->isEnabled(),"View context remains read-only");});toggle->click();require(v->mode()==AlignmentView::Mode::Insert&&right->count()==5&&right->currentIndex()==1,"Edit switch remembers prior mode and right-click action");right->setCurrentIndex(0);
    v->select_fragment({{1},2,2});key(*v,Qt::Key_X,Qt::ControlModifier);require(w.current_document().sequence(1).symbols.text()=="A-G--T"&&w.current_document().rows().size()==5&&QApplication::clipboard()->text()=="C~"&&v->selected_ids()==names,"Ctrl+X cuts fragment despite selected names");key(*v,Qt::Key_V,Qt::ControlModifier);require(w.current_document().sequence(1).symbols.text()=="A-C~G--T"&&w.current_document().rows().size()==5,"Ctrl+V inserts fragment at deletion boundary");action(w,"&Undo")->trigger();action(w,"&Undo")->trigger();
    v->select_fragment({{1},2,1});QApplication::clipboard()->setText("A");context(*v,cell(*v,0,2),[&](QMenu* menu){require(menu_action(menu,"Paste")->isEnabled(),"Single-symbol Insert paste enabled");menu_action(menu,"Paste")->trigger();});require(w.current_document().sequence(1).symbols.text()=="A-CA~G--T","Context Insert pastes after selected symbol");action(w,"&Undo")->trigger();
    setMode(AlignmentView::Mode::Rewrite);v->select_fragment({{1},2,2});QApplication::clipboard()->setText("TT");context(*v,cell(*v,0,2),[&](QMenu* menu){menu_action(menu,"Paste")->trigger();});require(w.current_document().sequence(1).symbols.text()=="A-TTG--T"&&v->fragment()->start==4,"Overwrite context substitutes selected span and advances caret");action(w,"&Undo")->trigger();
    v->select_fragment({{1},2,2});context(*v,cell(*v,0,2),[&](QMenu* menu){menu_action(menu,"Cut")->trigger();});require(w.current_document().sequence(1).symbols.text()=="A-G--T","Context Cut edits one row");action(w,"&Undo")->trigger();
    v->select_ids(names);v->select_fragment({{1},0,1});context(*v,{25,v->ruler_height()+v->row_height()+v->row_height()/2},[&](QMenu* menu){require(menu_action(menu,"Cut selected sequences")&&menu_action(menu,"Copy selected sequences")&&menu_action(menu,"Paste sequences"),"Name context has explicit sequence clipboard actions");menu_action(menu,"Copy selected sequences")->trigger();});require(QApplication::clipboard()->mimeData()->hasFormat("application/x-kleiotide-sequences"),"Name Copy preserves native metadata");
    const auto noImport=w.current_document().state_id();key(*v,Qt::Key_V,Qt::ControlModifier);require(w.current_document().state_id()==noImport&&w.current_document().rows().size()==5,"Fragment shortcut never imports a whole-sequence payload");
    context(*v,{25,v->ruler_height()+v->row_height()+v->row_height()/2},[&](QMenu* menu){menu_action(menu,"Cut selected sequences")->trigger();});require(w.current_document().rows().size()==3,"Name context cuts selected rows");action(w,"&Undo")->trigger();
    v->select_ids(names);action(w,"pasteSequences")->trigger();require(w.current_document().rows().size()==7&&w.current_document().rows()[3].name=="DNA2"&&w.current_document().rows()[4].kind==RowKind::Protein,"Explicit Paste sequences retains types and insertion order");action(w,"&Undo")->trigger();
    key(*v,Qt::Key_Escape);v->select_ids(names);QApplication::clipboard()->setText("sentinel");const auto namesOnly=w.current_document().state_id();key(*v,Qt::Key_C,Qt::ControlModifier);key(*v,Qt::Key_X,Qt::ControlModifier);key(*v,Qt::Key_V,Qt::ControlModifier);require(w.current_document().state_id()==namesOnly&&QApplication::clipboard()->text()=="sentinel","Name-only shortcuts never act on sequences");
    v->select_fragment({{1,4},1,3});action(w,"unlockSelection")->trigger();require(w.current_document().sequence(1).symbols.text()=="A~C~G--T"&&w.current_document().sequence(4).symbols.text()=="0~123"&&v->selected_ids()==names,"Unlock selection edits only selected cells independently of names");action(w,"&Undo")->trigger();
    context(*v,cell(*v,0,1),[&](QMenu* menu){menu_action(menu,"Unlock gaps in selection")->trigger();});require(w.current_document().sequence(1).symbols.text()=="A~C~G--T","Context unlock uses same rectangle");action(w,"&Undo")->trigger();
    action(w,"lockSelection")->trigger();require(w.current_document().sequence(1).symbols.text()=="A-C-G--T"&&w.current_document().sequence(4).symbols.text()=="0-123"&&v->selected_ids()==names,"Menu Lock changes selected open gaps only");action(w,"&Undo")->trigger();action(w,"&Redo")->trigger();require(w.current_document().sequence(1).symbols.text()=="A-C-G--T","Lock redo restores selected gap states");action(w,"&Undo")->trigger();
    context(*v,cell(*v,0,1),[&](QMenu* menu){require(menu_action(menu,"Lock gaps in selection")&&menu_action(menu,"Unlock gaps in selection"),"Both gap actions present");menu_action(menu,"Lock gaps in selection")->trigger();});require(w.current_document().sequence(1).symbols.text()=="A-C-G--T","Context Lock uses selected rectangle");action(w,"&Undo")->trigger();
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());require(dialog&&dialog->objectName()=="groupIdenticalDialog","Identical grouping preview");dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();});action(w,"groupIdentical")->trigger();require(w.current_document().groups().size()==1&&w.current_document().sequence(1).groupId==w.current_document().sequence(2).groupId&&!w.current_document().sequence(3).groupId,"Identical grouping ignores case, gap type, placement and count, but separates types");action(w,"&Undo")->trigger();require(w.current_document().groups()==d.groups(),"Grouping undo restores previous memberships");
    if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Toolbar screenshot");setMode(AlignmentView::Mode::Rewrite);setMode(AlignmentView::Mode::View);require(viewButton->isChecked()&&!toggle->isChecked()&&toggle->text()=="Edit"&&viewButton->text()=="View"&&!right->isEnabled(),"View segment highlight and disabled gap dropdown");if(argc>2)require(w.grab().save(QString::fromLocal8Bit(argv[2])),"View toolbar screenshot");const auto saved=dir.filePath("saved.kleio");save(w,saved);MainWindow restored;restored.show();restored.open_path(saved);auto* rv=dynamic_cast<AlignmentView*>(restored.centralWidget());wait([&]{return rv->isEnabled();});auto* rt=restored.findChild<QToolButton*>("editToggle");require(rv->mode()==AlignmentView::Mode::View&&!rt->isChecked()&&restored.findChild<QToolButton*>("rewriteToggle")->isChecked(),"Session retains View and remembered edit mode");rt->click();require(rv->mode()==AlignmentView::Mode::Rewrite,"Restored Edit switch returns to Rewrite");restored.close();w.close();
    std::cout<<"PASS 0.17.3: visible exclusive Edit/View segments, disabled View gap dropdown, lock/unlock selection/undo, mode switch/default/session, View gap protection, fragment shortcuts independent of names, explicit sequence clipboard/context, context mode/shape matrix, gap-insensitive groups, rectangular unlock/undo\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
