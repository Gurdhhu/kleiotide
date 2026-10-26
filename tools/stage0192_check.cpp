#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
template<class F>void wait(F f){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(f(),"Async timeout");QApplication::processEvents();}
QAction* action(MainWindow& w,const QString& name){for(auto* a:w.findChildren<QAction*>())if(a->objectName()==name||a->text().section('\t',0,0)==name)return a;throw std::runtime_error("Missing action "+name.toStdString());}
void key(AlignmentView& v,int code,Qt::KeyboardModifiers modifiers=Qt::NoModifier){v.window()->activateWindow();v.setFocus();QApplication::processEvents();QKeyEvent press(QEvent::KeyPress,code,modifiers);QApplication::sendEvent(&v,&press);QKeyEvent release(QEvent::KeyRelease,code,modifiers);QApplication::sendEvent(&v,&release);QApplication::processEvents();}
QPoint point(AlignmentView& v,int col){return {v.name_area_width()+col*v.cell_width()+v.cell_width()/2,v.ruler_height()+v.row_height()/2};}
void mouse(AlignmentView& v,QEvent::Type type,QPoint p,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent e(type,QPointF(p),QPointF(v.viewport()->mapToGlobal(p)),button,buttons,Qt::NoModifier);QApplication::sendEvent(v.viewport(),&e);QApplication::processEvents();}
template<class F>void context(AlignmentView& v,QPoint p,F inspect){QObject timer;QString error;bool called=false;QTimer::singleShot(0,&timer,[&]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());called=true;try{require(menu,"Context menu");inspect(menu);}catch(const std::exception& e){error=e.what();}if(menu)menu->close();});QContextMenuEvent event(QContextMenuEvent::Mouse,p,v.viewport()->mapToGlobal(p));QApplication::sendEvent(v.viewport(),&event);require(called,"Context inspected");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
QAction* menu_action(QMenu* menu,const QString& title){for(auto* a:menu->actions())if(a->text().section('\t',0,0)==title)return a;return nullptr;}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir dir;Document source;source.load({{"first","~~ACG~~T"},{"second","~~ACT~~T"},{"mask","0-~123",RowKind::Mask}});source.apply_groups({{1,"Group",0x0072b2}},{{2,1},{3,1}});const auto path=dir.filePath("source.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,source);}
    MainWindow w;w.resize(1280,760);w.show();w.open_path(path);auto* view=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return view->isEnabled();});const auto initial=w.current_document().state_id();
    auto* controls=w.findChild<QToolBar*>("editingToolbar");auto* counts=w.findChild<QToolBar*>("alignmentSizeToolbar");auto* label=w.findChild<QLabel*>("alignmentSize");require(controls&&counts&&label&&counts->isVisible(),"Wide window shows counts");
    const int natural=controls->sizeHint().width();w.resize(natural+12,760);QApplication::processEvents();require(counts->isHidden(),"Counts hide before reducing control space");
    for(const char* id:{"editViewSwitch","rewriteInsertSwitch","rightClickMode","residuePaletteButton"}){auto* control=w.findChild<QWidget*>(id);require(control&&control->isVisible()&&control->visibleRegion().boundingRect().width()>=control->width()-1,"Controls remain fully visible when counts no longer fit");}
    if(argc>2)require(w.grab().save(QString::fromLocal8Bit(argv[2])),"Narrow toolbar screenshot");
    w.resize(1280,760);QApplication::processEvents();require(counts->isVisible()&&label->text()=="3 sequences · 8 columns"&&w.width()-label->mapTo(&w,QPoint(label->width(),0)).x()<20,"Counts return at upper right on widening");
    for(auto mode:{AlignmentView::Mode::Rewrite,AlignmentView::Mode::Insert}){
        view->set_mode(mode);view->select_fragment({{1,2},2,3});mouse(*view,QEvent::MouseButtonPress,point(*view,3),Qt::LeftButton,Qt::LeftButton);mouse(*view,QEvent::MouseMove,point(*view,5),Qt::NoButton,Qt::LeftButton);mouse(*view,QEvent::MouseButtonRelease,point(*view,5),Qt::LeftButton,Qt::NoButton);
        require(view->fragment()->start==4&&w.current_document().sequence(1).symbols.text()=="~~~~ACGT","Mouse slide moves block");
        action(w,"&Undo")->trigger();require(view->fragment()&&view->fragment()->start==2&&view->fragment()->width==3&&view->fragment()->ids==std::vector<SequenceId>{1,2}&&w.current_document().state_id()==initial,"Menu Undo returns selection with mouse-dragged block");
        action(w,"&Redo")->trigger();require(view->fragment()->start==4,"Menu Redo restores moved selection");key(*view,Qt::Key_Z,Qt::ControlModifier);require(view->fragment()->start==2&&w.current_document().state_id()==initial,"Ctrl+Z restores block and selection after mouse movement");
    }
    view->set_mode(AlignmentView::Mode::Rewrite);view->select_ids({2,3});view->select_fragment({{1},2,2});
    const std::pair<const char*,QKeySequence> bindings[]={{"copySequences",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_C)},{"cutSequences",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_X)},{"pasteSequences",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_V)}};
    for(const auto& [id,binding]:bindings)require(action(w,id)->shortcut()==binding&&action(w,id)->isShortcutVisibleInContextMenu(),"New sequence shortcut appears in Edit menu");
    key(*view,Qt::Key_C,Qt::ControlModifier|Qt::ShiftModifier);auto* clipboard=QApplication::clipboard();require(clipboard->mimeData()->hasFormat("application/x-kleiotide-sequences")&&clipboard->text().startsWith(">second\n"),"Ctrl+Shift+C copies selected names, independently of fragment");
    {const auto bytes=clipboard->mimeData()->data("application/x-kleiotide-sequences");std::istringstream in(std::string(bytes.constData(),static_cast<std::size_t>(bytes.size())),std::ios::binary);auto copied=read_session(in).document;require(copied.rows().size()==2&&copied.rows()[1].kind==RowKind::Mask&&copied.groups().size()==1,"Sequence shortcut preserves types and group metadata");}
    key(*view,Qt::Key_X,Qt::ControlModifier|Qt::ShiftModifier);require(w.current_document().rows().size()==1&&w.current_document().sequence(1).symbols.text()=="~~ACG~~T","Ctrl+Shift+X cuts whole selected rows");
    key(*view,Qt::Key_V,Qt::ControlModifier|Qt::ShiftModifier);require(w.current_document().rows().size()==3&&w.current_document().rows()[1].name=="second"&&w.current_document().rows()[2].kind==RowKind::Mask&&w.current_document().rows()[2].groupId,"Ctrl+Shift+V pastes whole rows and metadata");
    action(w,"&Undo")->trigger();action(w,"&Undo")->trigger();require(w.current_document().state_id()==initial,"Whole-sequence Cut and Paste each form one undo step");
    view->select_ids({2,3});view->select_fragment({{1},2,2});key(*view,Qt::Key_C,Qt::ControlModifier);require(clipboard->text()=="AC"&&!clipboard->mimeData()->hasFormat("application/x-kleiotide-sequences"),"Plain Ctrl+C remains fragment-only");
    key(*view,Qt::Key_X,Qt::ControlModifier);require(w.current_document().rows().size()==3&&w.current_document().sequence(1).symbols.text()=="~~G~~T","Plain Ctrl+X still cuts a fragment");action(w,"&Undo")->trigger();
    view->select_ids({2,3});context(*view,{30,view->ruler_height()+view->row_height()+view->row_height()/2},[&](QMenu* menu){const char* names[]={"Copy selected sequences","Cut selected sequences","Paste sequences"};for(int i=0;i<3;++i){auto* item=menu_action(menu,names[i]);require(item&&item->property("shortcutHint").toString()==bindings[i].second.toString(QKeySequence::NativeText),"Name context menu shows whole-sequence shortcuts");}});
    view->unselect_all();view->select_fragment({{1},2,2});clipboard->setText("unchanged");key(*view,Qt::Key_C,Qt::ControlModifier|Qt::ShiftModifier);key(*view,Qt::Key_X,Qt::ControlModifier|Qt::ShiftModifier);require(clipboard->text()=="unchanged"&&w.current_document().state_id()==initial,"No selected names means whole-sequence Copy/Cut do nothing");
    clipboard->setText(">new\nACGTACGTACGT\n");key(*view,Qt::Key_V,Qt::ControlModifier|Qt::ShiftModifier);require(w.current_document().rows().size()==4&&w.current_document().columns()==12&&label->text()=="4 sequences · 12 columns","Whole-sequence paste also accepts FASTA and refreshes dimensions");action(w,"&Undo")->trigger();require(w.current_document().state_id()==initial,"FASTA row paste is undoable");
    if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Wide toolbar screenshot");w.close();std::cout<<"PASS 0.19.2: controls before counts at narrow widths, mouse Undo/Redo/Ctrl+Z selection, whole-sequence shortcuts/metadata/menu hints, fragment-key independence and atomic undo\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
