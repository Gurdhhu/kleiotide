#include "test_preferences.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
template<class F>void wait(F f){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(f(),"Async timeout");QApplication::processEvents();}
QAction* action(MainWindow& w,const QString& id){for(auto* a:w.findChildren<QAction*>())if(a->objectName()==id||a->text().section('\t',0,0)==id)return a;throw std::runtime_error("Missing action "+id.toStdString());}
void key(AlignmentView& v,int code,Qt::KeyboardModifiers mods=Qt::NoModifier){v.activateWindow();v.setFocus();QApplication::processEvents();QKeyEvent press(QEvent::KeyPress,code,mods);QApplication::sendEvent(&v,&press);QKeyEvent release(QEvent::KeyRelease,code,mods);QApplication::sendEvent(&v,&release);QApplication::processEvents();}
QPoint cell(AlignmentView& v,int row,int col){return {v.name_area_width()+col*v.cell_width()+v.cell_width()/2,v.ruler_height()+row*v.row_height()+v.row_height()/2};}
void mouse(AlignmentView& v,QEvent::Type type,QPoint point,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent e(type,QPointF(point),QPointF(v.viewport()->mapToGlobal(point)),button,buttons,Qt::NoModifier);QApplication::sendEvent(v.viewport(),&e);QApplication::processEvents();}
template<class F>void context(AlignmentView& v,QPoint at,F inspect){bool called=false;QString error;QObject context;QTimer::singleShot(0,&context,[&]{called=true;auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());try{require(menu,"Context menu");inspect(menu);}catch(const std::exception& e){error=e.what();}if(menu)menu->close();});QContextMenuEvent event(QContextMenuEvent::Mouse,at,v.viewport()->mapToGlobal(at));QApplication::sendEvent(v.viewport(),&event);require(called,"Context inspected");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
QAction* menu_action(QMenu* menu,const QString& text){for(auto* a:menu->actions())if(a->text().section('\t',0,0)==text)return a;return nullptr;}
template<class F>void modal(MainWindow& w,const QString& id,F inspect){bool called=false;QString error;QObject context;QTimer::singleShot(0,&context,[&]{called=true;auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());try{require(d,"Modal dialog");inspect(d);}catch(const std::exception& e){error=e.what();if(d)d->reject();}});action(w,id)->trigger();require(called,"Modal inspected");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
void save(MainWindow& w,const QString& path){modal(w,"Save &as…",[&](QDialog* d){auto* f=qobject_cast<QFileDialog*>(d);require(f,"Save chooser");f->selectFile(path);QMetaObject::invokeMethod(f,"accept",Qt::DirectConnection);});wait([&]{return w.centralWidget()->isEnabled();});}
QRect row_rect(AlignmentView& v,int r,int c){const auto scale=v.devicePixelRatioF();return {qRound((v.name_area_width()+c*v.cell_width())*scale),qRound((v.ruler_height()+r*v.row_height())*scale),qRound(v.cell_width()*scale),qRound(v.row_height()*scale)};}
void toolbar_dimensions(MainWindow& w,AlignmentView& v){
    auto* label=w.findChild<QLabel*>("alignmentSize");
    auto* controls=w.findChild<QToolBar*>("editingToolbar");
    auto* counts=w.findChild<QToolBar*>("alignmentSizeToolbar");
    require(label&&controls&&counts,"Alignment dimension widgets exist");
    const auto expected=QStringLiteral("4 sequences \u00b7 8 columns");
    auto check=[&](bool ok,const char* message){
        if(ok)return;
        const auto details=QString("%1 [window=%2, controls hint=%3, counter hint=%4, visible=%5, text='%6', label width=%7, visible width=%8]")
            .arg(QString::fromLatin1(message)).arg(w.width()).arg(controls->sizeHint().width())
            .arg(counts->sizeHint().width()).arg(counts->isVisible()).arg(label->text())
            .arg(label->width()).arg(label->visibleRegion().boundingRect().width());
        throw std::runtime_error(details.toStdString());
    };
    auto wide_width=[&]{return std::max(1280,controls->sizeHint().width()+counts->sizeHint().width()+64);};
    auto fully_visible=[&]{
        const int right=label->mapTo(&w,QPoint(label->width(),0)).x();
        const int gap=w.contentsRect().right()+1-right;
        const int margin=counts->contentsMargins().right()+label->contentsMargins().right()+
            counts->style()->pixelMetric(QStyle::PM_ToolBarItemMargin)+
            counts->style()->pixelMetric(QStyle::PM_ToolBarFrameWidth)+8;
        check(label->isVisible()&&label->visibleRegion().boundingRect().width()>=label->width()-1&&
              gap>=0&&gap<=std::max(20,margin),"Counter is fully visible and right-aligned when space allows");
    };
    // Window width alone does not establish whether Windows fonts/styles leave
    // room for the counter. Exercise both fitting and deliberately narrow layouts.
    // A larger font reproduces the old fixed-1280 assertion on Linux as well.
    const auto originalFont=w.font();
    for(bool enlarged:{false,true}){
        auto font=originalFont;
        if(enlarged)font.setPointSize(std::max(18,originalFont.pointSize()+6));
        w.setFont(font);QApplication::processEvents();
        w.resize(wide_width(),800);QApplication::processEvents();
        check(label->text()==expected,"Persistent toolbar dimension text");
        fully_visible();
        v.select_ids({1,2});v.select_fragment({{1,2},2,3});
        check(label->text()==expected,"Selection does not replace dimensions");fully_visible();
        for(int width:{1280,900,720,wide_width()}){
            w.resize(width,800);QApplication::processEvents();
            check(label->text()==expected,"Dimension text survives narrow layouts");
            if(w.width()+32<controls->sizeHint().width()+label->sizeHint().width())
                check(!counts->isVisible(),"Narrow layouts give editing controls priority");
            if(counts->isVisible())fully_visible();
        }
        std::cout<<"PASS toolbar dimensions: font "<<font.pointSize()<<" pt, fitting window "<<w.width()<<" px\n";
    }
    w.setFont(originalFont);QApplication::processEvents();
    w.resize(wide_width(),800);QApplication::processEvents();fully_visible();
}
void editing(AlignmentView::Mode mode){
    Document d;d.load({{"one","~~ACG~~T"},{"two","~~ACT~~T"}});AlignmentView v;v.resize(900,340);v.set_document(&d);v.set_mode(mode);v.show();QApplication::processEvents();const auto initial=d.state_id();
    v.select_fragment({{1},2,1});key(v,Qt::Key_Right);require(v.caret()==std::pair<SequenceId,std::size_t>{1,3}&&d.state_id()==initial,"Right arrow only moves caret");key(v,Qt::Key_Left);key(v,Qt::Key_Down);require(v.caret()==std::pair<SequenceId,std::size_t>{2,2},"Vertical caret movement");key(v,Qt::Key_Up);require(v.caret()==std::pair<SequenceId,std::size_t>{1,2},"Up arrow caret");
    v.select_fragment({{1,2},2,3});key(v,Qt::Key_Right,Qt::ControlModifier);require(d.sequence(1).symbols.text()=="~~~ACG~T"&&d.sequence(2).symbols.text()=="~~~ACT~T"&&v.fragment()->start==3&&v.fragment()->ids.size()==2,"Ctrl+Right slides solid block one column in either edit mode");d.undo();v.refresh();require(d.state_id()==initial&&v.fragment()->start==2,"One undo restores data and selection");d.redo();v.refresh();require(v.fragment()->start==3,"Redo restores slide selection");key(v,Qt::Key_Left,Qt::ControlModifier);require(d.sequence(1).symbols.text()=="~~ACG~~T"&&v.fragment()->start==2,"Ctrl+Left slides back");
    key(v,Qt::Key_Right,Qt::ShiftModifier);require(d.sequence(1).symbols.text()=="~~~~ACGT"&&v.fragment()->start==4,"Shift+Right retains maximum adjacent-gap slide");key(v,Qt::Key_Left,Qt::ShiftModifier);require(d.sequence(1).symbols.text()=="ACG~~~~T"&&v.fragment()->start==0,"Shift+Left retains maximum slide");
    d=Document{};d.load({{"one","~~ACG~~T"},{"two","~~ACT~~T"}});v.set_document(&d);v.set_mode(mode);v.select_fragment({{1,2},2,3});const auto before=d.state_id();mouse(v,QEvent::MouseButtonPress,cell(v,0,3),Qt::LeftButton,Qt::LeftButton);mouse(v,QEvent::MouseMove,cell(v,0,4),Qt::NoButton,Qt::LeftButton);require(d.state_id()==before,"Drag preview is read-only");mouse(v,QEvent::MouseButtonRelease,cell(v,0,4),Qt::LeftButton,Qt::NoButton);require(d.sequence(1).symbols.text()=="~~~ACG~T"&&v.fragment()->start==3,"Drag existing block slides in either edit mode");d.undo();v.refresh();
    v.select_fragment({{1},2,1});mouse(v,QEvent::MouseButtonPress,cell(v,0,2),Qt::LeftButton,Qt::LeftButton);mouse(v,QEvent::MouseButtonRelease,cell(v,0,3),Qt::LeftButton,Qt::NoButton);require(v.fragment()->start==3&&v.caret()->second==3,"Single-symbol sliding keeps editing caret synchronized");
    v.select_fragment({{1,2},1,2});context(v,cell(v,0,1),[&](QMenu* menu){const auto actions=menu->actions();int first=-1,last=-1;const char* names[]={"Move left one column","Move right one column","Move left as far as possible","Move right as far as possible"};const QKeySequence keys[]={QKeySequence(Qt::CTRL|Qt::Key_Left),QKeySequence(Qt::CTRL|Qt::Key_Right),QKeySequence(Qt::SHIFT|Qt::Key_Left),QKeySequence(Qt::SHIFT|Qt::Key_Right)};for(int i=0;i<4;++i){auto* a=menu_action(menu,names[i]);require(a&&a->property("shortcutHint").toString()==keys[i].toString(QKeySequence::NativeText),"Sliding context hints");if(i==0)first=static_cast<int>(actions.indexOf(a));last=static_cast<int>(actions.indexOf(a));}require(first>0&&last+1<actions.size()&&actions[first-1]->isSeparator()&&actions[last+1]->isSeparator(),"Sliding commands are a separate group");require(!menu_action(menu,"Cut")->isEnabled()&&!menu_action(menu,"Paste")->isEnabled(),"Block clipboard edits remain protected");});
    d=Document{};d.load({{"one","AC-G"},{"two","T~GC"}});v.set_document(&d);v.set_mode(mode);v.select_fragment({{1,2},0,1});key(v,Qt::Key_Right,Qt::ControlModifier);require(d.sequence(1).symbols.text()=="~AC-G"&&d.sequence(2).symbols.text()=="~TGC"&&d.columns()==5,"Sliding pushes residues/locked gaps, consumes open gaps and extends alignment");d.undo();v.refresh();
    v.set_mode(AlignmentView::Mode::View);v.select_fragment({{1,2},0,1});const auto readonly=d.state_id();key(v,Qt::Key_Right,Qt::ControlModifier);key(v,Qt::Key_Left,Qt::ShiftModifier);require(d.state_id()==readonly,"View never slides");v.close();
}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{kleiotide::TestPreferences testPreferences;
    editing(AlignmentView::Mode::Rewrite);editing(AlignmentView::Mode::Insert);
    QTemporaryDir dir;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,dir.path());Document d;d.load({{"one","~~ACG~~T"},{"two","~~ACT~~T"},{"outside","~~AGT~~T"},{"mask","11111111",RowKind::Mask}});const auto path=dir.filePath("source.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,d);}
    MainWindow w;w.resize(1280,800);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});auto* edit=w.findChild<QToolButton*>("editToggle");auto* view=w.findChild<QToolButton*>("viewToggle");auto* rewrite=w.findChild<QToolButton*>("rewriteToggle");auto* insert=w.findChild<QToolButton*>("insertToggle");auto* gap=w.findChild<QComboBox*>("rightClickMode");require(edit&&view&&rewrite&&insert&&gap&&!w.findChild<QComboBox*>("editingMode"),"Exactly two segmented mode switches; old dropdown removed");require(edit->isChecked()&&rewrite->isChecked()&&rewrite->text()=="Rewrite"&&insert->text()=="Insert","Edit/Rewrite default and visible labels");
    auto* first=w.findChild<QWidget*>("editViewSwitch");auto* second=w.findChild<QWidget*>("rewriteInsertSwitch");require(second->mapToGlobal(QPoint(0,0)).x()>=first->mapToGlobal(QPoint(first->width(),0)).x()+12&&gap->mapToGlobal(QPoint(0,0)).x()>=second->mapToGlobal(QPoint(second->width(),0)).x()+12,"Spacing between control groups");
    insert->click();require(insert->isChecked()&&!rewrite->isChecked()&&v->mode()==AlignmentView::Mode::Insert,"Insert segment changes mode");insert->click();require(insert->isChecked(),"Active mode stays selected");view->click();require(!second->isEnabled()&&!gap->isEnabled()&&gap->count()==1&&insert->isChecked(),"View disables editing controls and remembers Insert");edit->click();require(second->isEnabled()&&v->mode()==AlignmentView::Mode::Insert,"Edit restores Insert");rewrite->click();
    toolbar_dimensions(w,*v);auto* size=w.findChild<QLabel*>("alignmentSize");
    QApplication::clipboard()->setText(">added\nACGTACGTACGTACGT\n");action(w,"pasteSequences")->trigger();wait([&]{return w.current_document().rows().size()==5;});require(size->text()=="5 sequences · 16 columns","Dimensions update after adding rows/columns");action(w,"&Undo")->trigger();require(size->text()=="4 sequences · 8 columns","Dimensions update on undo");
    key(*v,Qt::Key_Escape);v->select_ids({1,2});modal(w,"highlightScope",[&](QDialog* dialog){dialog->findChild<QComboBox*>("highlightRows")->setCurrentIndex(1);dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();});auto before=v->viewport()->grab().toImage();action(w,"highlightVariable")->trigger();wait([&]{return bool(v->site_highlights());});auto after=v->viewport()->grab().toImage();require((*v->site_highlights())[4],"Selected population determines variable column");for(int row=0;row<4;++row)require((before.copy(row_rect(*v,row,4))!=after.copy(row_rect(*v,row,4)))==(row<2),"Only analysis-snapshot rows receive shading");v->select_ids({3});require(v->site_highlights()&&(*v->site_highlights())[4],"Changing name selection preserves explicit analysis snapshot");
    v->select_fragment({{1},2,1});context(*v,cell(*v,0,2),[&](QMenu* menu){for(const auto& pair:std::vector<std::pair<QString,QKeySequence>>{{"Copy",QKeySequence::Copy},{"Cut",QKeySequence::Cut},{"Paste",QKeySequence::Paste}}){auto* a=menu_action(menu,pair.first);require(a&&a->property("shortcutHint").toString()==pair.second.toString(QKeySequence::NativeText),"Fragment clipboard shortcut labels");}});
    context(*v,{30,v->ruler_height()+v->row_height()/2},[&](QMenu* menu){require(menu_action(menu,"Select all")->property("shortcutHint").toString()==QKeySequence(QKeySequence::SelectAll).toString(QKeySequence::NativeText)&&menu_action(menu,"Delete selected sequences")->property("shortcutHint").toString()==QKeySequence(Qt::CTRL|Qt::Key_Delete).toString(QKeySequence::NativeText),"Name-menu shortcut hints");require(menu_action(menu,"Copy selected sequences")->property("shortcutHint").toString()==QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_C).toString(QKeySequence::NativeText),"Whole-row Copy advertises Ctrl+Shift+C");});
    for(auto* a:w.findChildren<QAction*>())if(!a->shortcut().isEmpty())require(a->isShortcutVisibleInContextMenu(),"Every assigned menu shortcut is visible");
    action(w,"mode.insert")->trigger();require(v->mode()==AlignmentView::Mode::Insert&&insert->isChecked(),"Menu and toolbar share Insert state");
    action(w,"mode.right.2")->trigger();auto* right=w.findChild<QComboBox*>("rightClickMode");require(right->currentIndex()==2&&action(w,"mode.right.2")->isChecked(),"Right-click menu updates combo");
    right->setCurrentIndex(3);require(action(w,"mode.right.3")->isChecked()&&!action(w,"mode.right.2")->isChecked(),"Combo updates exclusive menu choices");
    action(w,"mode.view")->trigger();require(v->mode()==AlignmentView::Mode::View&&view->isChecked()&&!action(w,"mode.insert")->isEnabled()&&action(w,"mode.right.0")->isChecked()&&!action(w,"mode.right.3")->isVisible(),"View mode restricts editing and right-click actions");
    action(w,"mode.edit")->trigger();require(v->mode()==AlignmentView::Mode::Insert&&right->currentIndex()==3&&action(w,"mode.right.3")->isChecked(),"Edit restores remembered insertion and right-click choices");
    require(edit->defaultAction()==action(w,"mode.edit")&&insert->defaultAction()==action(w,"mode.insert"),"Toolbar uses the same actions as menu");
    insert->click();view->click();const auto saved=dir.filePath("saved.kleio");save(w,saved);MainWindow reopened;reopened.show();reopened.open_path(saved);auto* rv=dynamic_cast<AlignmentView*>(reopened.centralWidget());wait([&]{return rv->isEnabled()&&bool(rv->site_highlights());});require(rv->mode()==AlignmentView::Mode::View&&reopened.findChild<QToolButton*>("insertToggle")->isChecked(),"Native session remembers View and Insert");reopened.findChild<QToolButton*>("editToggle")->click();require(rv->mode()==AlignmentView::Mode::Insert,"Restored Edit returns to Insert");require(reopened.findChild<QComboBox*>("rightClickMode")->currentIndex()==3,"Session preserves remembered right-click action");reopened.close();
    const auto legacy=dir.filePath("legacy_slide.kleio");for(const auto& state:{std::string("{\"version\":1,\"mode\":3,\"editMode\":3}"),std::string("{\"version\":1,\"mode\":0,\"editMode\":3}")}){{std::ofstream out(legacy.toStdString(),std::ios::binary);write_session(out,d,state);}MainWindow old;old.show();old.open_path(legacy);auto* ov=dynamic_cast<AlignmentView*>(old.centralWidget());wait([&]{return ov->isEnabled();});require(old.findChild<QToolButton*>("rewriteToggle")->isChecked()&&ov->mode()==(state.find("\"mode\":0")!=std::string::npos?AlignmentView::Mode::View:AlignmentView::Mode::Rewrite),"Old Slide session migrates to Rewrite, preserving View");old.findChild<QToolButton*>("editToggle")->click();require(ov->mode()==AlignmentView::Mode::Rewrite,"Old remembered Slide becomes Rewrite");old.close();}
    edit->click();rewrite->click();if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Toolbar/highlight screenshot");w.close();std::cout<<"PASS 0.19.1: two switches/spacing, persistent dimensions, shared drag/Shift/Ctrl sliding and caret arrows, block pushing/extension, read-only View, shortcut hints, row-filtered highlights and old/new session modes\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
