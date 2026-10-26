#include "gui/main_window.hpp"
#include "gui/chromatogram_view.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>void wait(F f){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<30000){QApplication::processEvents();QThread::msleep(1);}require(f(),"Timeout");}
QAction* action(MainWindow& w,const QString& text){for(auto* a:w.findChildren<QAction*>())if(a->text().section('\t',0,0)==text)return a;throw std::runtime_error("Missing action");}
ChromatogramPanel* panel(MainWindow& w){for(auto* widget:w.findChildren<QWidget*>())if(auto* p=dynamic_cast<ChromatogramPanel*>(widget))return p;throw std::runtime_error("Missing trace panel");}
void mouse(TraceCanvas& c,QEvent::Type type,double x,Qt::KeyboardModifiers modifiers=Qt::NoModifier){const bool press=type==QEvent::MouseButtonPress,release=type==QEvent::MouseButtonRelease;QMouseEvent event(type,QPointF(x,8),QPointF(x,8),press||release?Qt::LeftButton:Qt::NoButton,release?Qt::NoButton:Qt::LeftButton,modifiers);QApplication::sendEvent(c.viewport(),&event);}
void click(TraceCanvas& c,std::size_t column,Qt::KeyboardModifiers mods=Qt::NoModifier){const auto x=c.column_x(column);mouse(c,QEvent::MouseButtonPress,x,mods);mouse(c,QEvent::MouseButtonRelease,x,mods);}
void drag(TraceCanvas& c,std::size_t a,std::size_t b){const auto x=c.column_x(a),y=c.column_x(b);mouse(c,QEvent::MouseButtonPress,x);mouse(c,QEvent::MouseMove,y);mouse(c,QEvent::MouseButtonRelease,y);}
void key(TraceCanvas& c,int code,const QString& text={},Qt::KeyboardModifiers mods=Qt::NoModifier){QKeyEvent event(QEvent::KeyPress,code,mods,text);QApplication::sendEvent(&c,&event);}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{
    QTemporaryDir dir;const auto path=dir.filePath("editing trace.ab1");std::string calls;for(unsigned i=0;i<40;++i)calls+="ACGT";QFile file(path);require(file.open(QIODevice::WriteOnly),"Fixture open");const auto bytes=test::synthetic_abif(calls);file.write(bytes.data(),static_cast<qint64>(bytes.size()));file.close();
    MainWindow w;w.resize(1180,850);w.show();w.open_path(path);auto* view=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return view->isEnabled();});auto* p=panel(w);auto* c=p->canvas();const auto evidence=w.current_document().sequence(1).trace->original;
    auto* toggle=w.findChild<QToolButton*>("editToggle");auto setMode=[&](int value){if(value==0){w.findChild<QToolButton*>("viewToggle")->click();}else{if(!toggle->isChecked())toggle->click();w.findChild<QToolButton*>(value==1?"insertToggle":"rewriteToggle")->click();}};auto* spacing=p->findChild<QComboBox*>("traceSpacing");
    setMode(0);click(*c,1);key(*c,Qt::Key_T,"T");require(w.current_document().sequence(1).symbols.text()==calls,"View mode remains read-only");
    setMode(1);click(*c,1);key(*c,Qt::Key_R,"R");require(w.current_document().sequence(1).symbols.text().substr(0,5)=="ACRGT"&&view->fragment()->start==2,"Direct Insert after selected call");require(!trace_position(w.current_document().sequence(1),2).originalCall,"No invented original peak");
    drag(*c,1,3);require(view->fragment()->width==3,"Drag selects current substring");const auto before=w.current_document().sequence(1).symbols.text();key(*c,Qt::Key_A,"A");require(w.current_document().sequence(1).symbols.text()==before,"Ambiguous Insert selection is not editable");
    click(*c,1);click(*c,4,Qt::ShiftModifier);require(view->fragment()->start==1&&view->fragment()->width==4,"Shift-click trace selection");
    setMode(2);drag(*c,2,4);QApplication::clipboard()->setText("TT");action(w,"Paste fragment")->trigger();require(w.current_document().sequence(1).symbols.text().substr(0,5)=="ACTTA"&&view->caret()->second==4,"Direct Overwrite paste and caret advance");
    action(w,"&Undo")->trigger();require(w.current_document().sequence(1).symbols.text()==before,"Undo direct replacement");action(w,"&Redo")->trigger();require(w.current_document().sequence(1).symbols.text().substr(0,5)=="ACTTA","Redo direct replacement");
    drag(*c,1,2);QApplication::clipboard()->setText("sentinel");action(w,"Copy fragment")->trigger();require(QApplication::clipboard()->text()=="CT","Trace selection copies substring");
    key(*c,Qt::Key_Delete);require(w.current_document().sequence(1).symbols.text().substr(0,3)=="ATA","Direct deletion");action(w,"&Undo")->trigger();
    key(*c,Qt::Key_Backspace);require(w.current_document().sequence(1).symbols.text().substr(0,3)=="ATA"&&view->caret()->second==0,"Direct Backspace selects preceding symbol");action(w,"&Undo")->trigger();
    view->select_ids({1});click(*c,1);require(view->selected_ids()==std::vector<SequenceId>({1})&&view->fragment()->ids==std::vector<SequenceId>({1}),"Trace click preserves independent name selection");view->unselect_all();
    spacing->setCurrentIndex(1);setMode(1);click(*c,1);key(*c,Qt::Key_AsciiTilde,"~");require(w.current_document().sequence(1).symbols.text().substr(0,5)=="AC~TT","Gap inserted directly");
    const auto labels=c->current_labels();require(std::any_of(labels.begin(),labels.end(),[](const auto& l){return l.symbol=='~';}),"Aligned Current displays gaps");require(std::abs(c->column_x(4)-c->column_x(3)-20.)<1e-8,"Uniform alignment spacing");
    setMode(3);click(*c,3);key(*c,Qt::Key_Right,{},Qt::ShiftModifier);require(view->fragment()->start==4,"Trace Shift-arrow sliding");action(w,"&Undo")->trigger();
    setMode(2);click(*c,1);key(*c,Qt::Key_G,"G");require(trace_position(w.current_document().sequence(1),1).edited&&!trace_position(w.current_document().sequence(1),1).quality,"Substitution retains evidence, not original quality as its own");
    view->select_ids({1});action(w,"Reverse complement")->trigger();require(w.current_document().sequence(1).trace->reversed,"Trace strand reverses"); // Action label is resolved below if the menu uses punctuation.
    require(w.current_document().sequence(1).trace->original==evidence,"Original signal pointer remains immutable");
    spacing->setCurrentIndex(0);c->horizontalScrollBar()->setValue(0);QApplication::processEvents();auto overlay=c->viewport()->grab(QRect(0,0,60,50)).toImage();c->horizontalScrollBar()->setValue(255);QApplication::processEvents();require(c->viewport()->grab(QRect(0,0,60,50)).toImage()==overlay,"Opaque labels remain unchanged over scrolling residues/caret");spacing->setCurrentIndex(1);
    if(argc>1){QApplication::processEvents();require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Screenshot");}
    const auto native=dir.filePath("edited session.kleio");bool chosen=false;QTimer timer;timer.setInterval(5);QObject::connect(&timer,&QTimer::timeout,[&]{if(auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget())){if(chosen)return;chosen=true;dialog->selectFile(native);QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);}});timer.start();action(w,"&Save")->trigger();timer.stop();wait([&]{return view->isEnabled();});require(chosen&&QFile::exists(native),"Save session");
    MainWindow restored;restored.show();restored.open_path(native);auto* v=dynamic_cast<AlignmentView*>(restored.centralWidget());wait([&]{return v->isEnabled();});auto* rp=panel(restored);require(rp->canvas()->alignment_spacing(),"Spacing choice restored");require(restored.current_document().sequence(1).symbols.text()==w.current_document().sequence(1).symbols.text()&&restored.current_document().sequence(1).trace->original->channels==evidence->channels,"Session edits/signal exact");
    restored.close();w.close();std::cout<<"PASS 0.10: opaque headers, direct modes/selection/typing/paste/copy/delete/undo/redo/gaps/sliding, display warp and native session restore\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
