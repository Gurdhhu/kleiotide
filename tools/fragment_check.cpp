#include "gui/alignment_view.hpp"
#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>
#include <QEventLoop>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char**argv){
 QApplication app(argc,argv);
 try {
  Document d;d.load({{"one","A~~CG~~~T"},{"two","A~~CG~T~~"},{"three","A~"}});
  AlignmentView v;v.resize(900,280);v.set_document(&d);v.set_mode(AlignmentView::Mode::Rewrite);v.show();app.processEvents();
  const QFontMetrics metrics(v.font(),v.viewport());const int cw=std::max(4,metrics.horizontalAdvance('M')+2),rh=std::max(12,metrics.height()+4);
  auto point=[&](int row,int col){return QPointF(230+(col+0.5)*cw,v.ruler_height()+(row+.5)*rh);};
  auto mouse=[&](QEvent::Type type,QPointF p,Qt::MouseButton button,Qt::MouseButtons buttons, Qt::KeyboardModifiers mods=Qt::NoModifier){
   QMouseEvent event(type,p,QPointF(v.viewport()->mapToGlobal(p.toPoint())),button,buttons,mods);
   QApplication::sendEvent(v.viewport(),&event);app.processEvents();
  };
  auto drag=[&](QPointF a,QPointF b){mouse(QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);};
  auto key=[&](int code,Qt::KeyboardModifiers mods=Qt::NoModifier){QKeyEvent event(QEvent::KeyPress,code,mods);QApplication::sendEvent(&v,&event);app.processEvents();};
  drag(point(0,3),point(0,4));require(v.fragment()&&v.fragment()->start==3&&v.fragment()->width==2&&v.fragment()->ids.size()==1,"Mouse rectangle");
  key(Qt::Key_Right,Qt::ControlModifier);require(d.rows()[0].symbols.text()=="A~~~CG~~T"&&v.fragment()->start==4,"Arrow slide");
  key(Qt::Key_Right,Qt::ShiftModifier);require(d.rows()[0].symbols.text()=="A~~~~~CGT"&&v.fragment()->start==6,"Maximum slide");
  key(Qt::Key_Left,Qt::ShiftModifier);require(d.rows()[0].symbols.text()=="ACG~~~~~T"&&v.fragment()->start==1,"Maximum left");
  d.undo();v.refresh();app.processEvents();require(v.fragment()->start==6,"Undo selection position");
  drag(point(0,6),point(0,4));require(d.rows()[0].symbols.text()=="A~~~CG~~T"&&v.fragment()->start==4,"Drag rigid block");
  d.undo();v.refresh();require(v.fragment()->start==6,"Drag undo");
  bool menuTriggered = false;
  QTimer::singleShot(0, [&] {
   if(auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) {
    for(auto* item:menu->actions())if(item->text()=="Replace selected symbols with Ns") {item->trigger();menuTriggered=true;break;}
    menu->close();
   }
  });
  QContextMenuEvent context(QContextMenuEvent::Mouse,point(0,6).toPoint(),v.viewport()->mapToGlobal(point(0,6).toPoint()));
  QApplication::sendEvent(v.viewport(),&context);require(menuTriggered && d.rows()[0].symbols.text()=="A~~~~~NNT","Fragment context menu");
  d.undo();v.refresh();
  v.fill_fragment('N');require(d.rows()[0].symbols.text()=="A~~~~~NNT","Fill fragment");d.undo();v.refresh();
  v.select_ids({d.rows()[0].id,d.rows()[2].id});drag(point(0,1),point(0,2));require(v.fragment()->ids.size()==1,"Residue drag is independent from noncontiguous named rows");
  key(Qt::Key_Escape);v.unselect_all();
  mouse(QEvent::MouseButtonPress,point(0,1),Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,point(0,1),Qt::LeftButton,Qt::NoButton);
  mouse(QEvent::MouseButtonPress,point(2,3),Qt::LeftButton,Qt::LeftButton,Qt::ShiftModifier);mouse(QEvent::MouseButtonRelease,point(2,3),Qt::LeftButton,Qt::NoButton,Qt::ShiftModifier);
  require(v.fragment()->ids.size()==3&&v.fragment()->start==1&&v.fragment()->width==3,"Shift-click rectangular row/column selection");
  mouse(QEvent::MouseButtonPress,point(1,2),Qt::LeftButton,Qt::LeftButton,Qt::ShiftModifier);mouse(QEvent::MouseButtonRelease,point(1,2),Qt::LeftButton,Qt::NoButton,Qt::ShiftModifier);
  require(v.fragment()->ids.size()==2&&v.fragment()->width==2,"Shift-click preserves initial anchor and shrinks rectangle");
  key(Qt::Key_Escape);drag(point(0,3),point(1,4));require(v.fragment()->ids.size()==2&&v.fragment()->width==2,"Multiple-row rectangle");
  key(Qt::Key_Escape);const QPointF rulerA(230+2.5*cw,v.ruler_height()/2.0),rulerB(230+5.5*cw,v.ruler_height()/2.0);
  drag(rulerA,rulerB);require(v.fragment()->ids.size()==3&&v.fragment()->start==2&&v.fragment()->width==4,"Ruler columns across all rows");
  const auto image=v.viewport()->grab().toImage();v.viewport()->repaint();require(image==v.viewport()->grab().toImage(),"Selection repaint");
  const auto before0=d.rows()[0].symbols.text(),before1=d.rows()[1].symbols.text(),before2=d.rows()[2].symbols.text();
  v.set_right_click(AlignmentView::RightClick::InsertOthers);mouse(QEvent::MouseButtonPress,point(2,1),Qt::RightButton,Qt::RightButton);mouse(QEvent::MouseButtonRelease,point(2,1),Qt::RightButton,Qt::NoButton);
  require(d.rows()[0].symbols.text()==before0.substr(0,1)+"~"+before0.substr(1)&&d.rows()[1].symbols.text()==before1.substr(0,1)+"~"+before1.substr(1)&&d.rows()[2].symbols.text()==before2,"Right click others");
  d.undo();v.refresh();v.set_right_click(AlignmentView::RightClick::DeleteGap);mouse(QEvent::MouseButtonPress,point(2,1),Qt::RightButton,Qt::RightButton);mouse(QEvent::MouseButtonRelease,point(2,1),Qt::RightButton,Qt::NoButton);
  require(d.rows()[2].symbols.text()=="A","Right click delete");
  key(Qt::Key_Escape);require(!v.fragment(),"Escape clears selection");
  // Pushing via Shift+arrows/drag must cross locks and grow the document.
  d.load({{"push","A-CG~~T"},{"other","TT"}});v.set_document(&d);v.set_mode(AlignmentView::Mode::Rewrite);app.processEvents();
  drag(point(0,2),point(0,3));key(Qt::Key_Left,Qt::ControlModifier);
  require(d.rows()[0].symbols.text()=="A-CG~~T"&&v.fragment()->start==2,"Leading content cannot be lost");
  drag(point(0,2),point(0,7));require(v.fragment()->start==7&&d.rows()[0].symbols.text()=="A-~~~~~CGT"&&d.columns()==10,"Drag pushes suffix beyond end");
  d.undo();v.refresh();require(v.fragment()->start==2&&d.rows()[0].symbols.text()=="A-CG~~T","Extended drag undo");
  key(Qt::Key_Right,Qt::ControlModifier);key(Qt::Key_Right,Qt::ControlModifier);key(Qt::Key_Right,Qt::ControlModifier);
  require(v.fragment()->start==5&&d.rows()[0].symbols.text()=="A-~~~CGT","Arrow pushes terminal residue");
  // Vertical autoscroll while selecting, then release must stop the timer.
  std::vector<Record> records;for(int i=0;i<40;++i)records.push_back({std::to_string(i),"ACGT~~~~"});d.load(records);v.set_document(&d);v.set_mode(AlignmentView::Mode::Rewrite);app.processEvents();
  mouse(QEvent::MouseButtonPress,point(0,1),Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,QPointF(250,v.viewport()->height()-2),Qt::NoButton,Qt::LeftButton);
  QEventLoop loop;QTimer::singleShot(120,&loop,&QEventLoop::quit);loop.exec();
  require(v.verticalScrollBar()->value()>0,"Drag edge autoscroll");mouse(QEvent::MouseButtonRelease,QPointF(250,v.viewport()->height()-2),Qt::LeftButton,Qt::NoButton);
  if(argc>1)v.viewport()->grab().save(QString::fromLocal8Bit(argv[1]));
  std::cout<<"PASS fragment GUI: rectangle, group/ruler selection, arrows, drag, undo, fill, right-click gaps, autoscroll\n";
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
