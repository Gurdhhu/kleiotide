#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include <QAction>
#include <QApplication>
#include <QBackingStore>
#include <QElapsedTimer>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QThread>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QAction* action(MainWindow& window,const QString& title){for(auto* a:window.findChildren<QAction*>())if(a->text()==title||a->objectName()==title)return a;throw std::runtime_error("Missing action");}
void mouse(AlignmentView& view,QEvent::Type type,QPoint point,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent event(type,QPointF(point),QPointF(view.viewport()->mapToGlobal(point)),button,buttons,Qt::NoModifier);QApplication::sendEvent(view.viewport(),&event);}
QImage pixels(AlignmentView& view){auto* device=view.backingStore()->paintDevice();require(device->devType()==QInternal::Image,"Raster backing store required");const auto* image=static_cast<QImage*>(device);const auto origin=view.viewport()->mapTo(view.window(),QPoint(0,0));const auto dpr=image->devicePixelRatio();return image->copy(QRect(qRound(origin.x()*dpr),qRound(origin.y()*dpr),qRound(view.viewport()->width()*dpr),qRound(view.viewport()->height()*dpr)));}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir directory;Document source;std::vector<Record> records;
    for(int r=0;r<2000;++r){std::string symbols;for(int c=0;c<4000;++c)symbols+="ACGT~~N-"[(r+c)%8];records.push_back({"Sequence "+std::to_string(r),std::move(symbols)});}source.load(std::move(records));
    const auto path=directory.filePath("selection.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,source);}
    MainWindow window;window.resize(3440,1440);window.show();window.open_path(path);auto* view=dynamic_cast<AlignmentView*>(window.centralWidget());
    QElapsedTimer timeout;timeout.start();while(!view->isEnabled()&&timeout.elapsed()<20000){app.processEvents();QThread::msleep(1);}require(view->isEnabled(),"Load timeout");app.processEvents();
    std::size_t notifications=0;const auto notify=view->selection_changed;view->selection_changed=[&]{++notifications;if(notify)notify();};
    for(auto style:{ResidueDisplay::Letters,ResidueDisplay::Backgrounds}){
        view->set_residue_display(style);view->unselect_all();view->set_mode(AlignmentView::Mode::View);app.processEvents();
        const QPoint anchor(view->name_area_width()+3*view->cell_width(),view->ruler_height()+view->row_height()/2);
        mouse(*view,QEvent::MouseButtonPress,anchor,Qt::LeftButton,Qt::LeftButton);app.processEvents();
        std::vector<double> frames,events;QPoint last;
        for(int i=0;i<160;++i){const int phase=i<80?i:159-i;last=QPoint(anchor.x()+10+(view->viewport()->width()-anchor.x()-60)*phase/80,anchor.y()+10+(view->viewport()->height()-anchor.y()-60)*phase/80);
            QElapsedTimer timer;timer.start();mouse(*view,QEvent::MouseMove,last,Qt::NoButton,Qt::LeftButton);events.push_back(timer.nsecsElapsed()/1e6);app.processEvents();frames.push_back(timer.nsecsElapsed()/1e6);
            if(i%20==0){const auto before=pixels(*view);view->viewport()->repaint();const auto after=pixels(*view);if(before!=after){before.save("validation/selection0193-stale.png");after.save("validation/selection0193-full.png");std::cerr<<"Frame "<<i<<" style "<<static_cast<int>(style)<<'\n';}require(before==after,"Selection repaint mismatch");}
        }
        mouse(*view,QEvent::MouseButtonRelease,last,Qt::LeftButton,Qt::NoButton);app.processEvents();
        std::sort(frames.begin(),frames.end());std::sort(events.begin(),events.end());std::cout<<(style==ResidueDisplay::Letters?"letters":"backgrounds")<<" 3440x1440 / 2000x4000: mouse p95 "<<events[152]<<" ms; event+paint median "<<frames[80]<<" ms, p95 "<<frames[152]<<" ms, max "<<frames.back()<<" ms\n";
    }
    if(app.arguments().contains("--benchmark-only")){window.close();return 0;}
    for(auto mode:{AlignmentView::Mode::Rewrite,AlignmentView::Mode::Insert})for(auto style:{ResidueDisplay::Letters,ResidueDisplay::Backgrounds}){
        view->set_mode(mode);view->set_residue_display(style);auto font=view->font();font.setPointSize(6);view->setFont(font);app.processEvents();
        for(int i=0;i<40;++i){std::vector<SequenceId> ids{1};if(i%3)ids.push_back(2);if(i%4)ids.push_back(4);view->select_fragment({ids,static_cast<std::size_t>(i%7),static_cast<std::size_t>(i%5+1)});app.processEvents();const auto before=pixels(*view);view->viewport()->repaint();const auto after=pixels(*view);if(before!=after){before.save("validation/selection0193-stale.png");after.save("validation/selection0193-full.png");std::cerr<<"Small font frame "<<i<<" style "<<static_cast<int>(style)<<" mode "<<static_cast<int>(mode)<<'\n';}require(before==after,"Blue/yellow, single/block, noncontiguous rows, font-6 repaint mismatch");const auto count=notifications;view->select_fragment(*view->fragment());require(notifications==count,"Unchanged selection emits no repeated updates");}
    }
    view->set_mode(AlignmentView::Mode::Rewrite);view->select_ids({3,4});
    auto& document=const_cast<Document&>(window.current_document());
    auto away=[&]{view->select_fragment({{1},1,1});view->horizontalScrollBar()->setValue(0);view->verticalScrollBar()->setValue(0);app.processEvents();};
    auto visible=[&](std::size_t row,std::size_t column){const auto range=view->visible_column_range();const auto top=static_cast<std::size_t>(view->verticalScrollBar()->value());return column>=range.first&&column<range.second&&row>=top&&row<top+static_cast<std::size_t>((view->viewport()->height()-view->ruler_height())/view->row_height());};
    auto check=[&](const char* label,std::size_t row,std::size_t column,const auto& edit){
        view->set_mode(AlignmentView::Mode::Rewrite);view->select_fragment({{row+1},column,3});const auto before=document.state_id();edit();require(document.state_id()!=before,label);away();action(window,"&Undo")->trigger();app.processEvents();if(document.state_id()!=before||!visible(row,column)){std::cerr<<label<<" target "<<row<<','<<column<<" scroll "<<view->verticalScrollBar()->value()<<','<<view->horizontalScrollBar()->value()<<" state "<<document.state_id()<<" expected "<<before<<'\n';}require(document.state_id()==before&&visible(row,column),"Undo reveals edited row and column");require(view->selected_ids()==std::vector<SequenceId>({3,4}),"Undo navigation preserves name selection");
        away();action(window,"&Redo")->trigger();app.processEvents();require(visible(row,column),"Redo reveals edited area");action(window,"&Undo")->trigger();
    };
    check("Rewrite",1000,2000,[&]{view->fill_fragment('N');});
    check("Paste",1100,2101,[&]{require(view->paste_residues("AGTCGA"),"Paste edit");});
    check("Insert",1200,2200,[&]{view->set_mode(AlignmentView::Mode::Insert);view->select_fragment({{1201},2199,1});require(view->paste_residues("AGT"),"Insert edit");});
    check("Delete",1300,2300,[&]{QKeyEvent event(QEvent::KeyPress,Qt::Key_Delete,Qt::NoModifier);QApplication::sendEvent(view,&event);});
    check("Lock gaps",1400,2404,[&]{view->lock_fragment();});
    check("Slide",1500,2500,[&]{view->select_fragment({{1501,1502},2500,3});view->slide_fragment(5);});
    require(view->fragment()&&view->fragment()->ids==std::vector<SequenceId>({1501,1502})&&view->fragment()->start==2500&&view->fragment()->width==3,"Navigation preserves restored block selection");
    check("Whole-row transform",1600,1,[&]{document.replace(1601,0,4000,std::string(2800,'A')+"T"+std::string(1199,'A'));view->refresh();if(view->changed)view->changed();});
    // The transform above differs before column 2800 as well; verify precise
    // prefix navigation separately with an otherwise identical whole-row edit.
    {auto after=document.sequence(1701).symbols.text();after[2900]=after[2900]=='C'?'T':'C';document.replace(1701,0,after.size(),after);view->refresh();away();action(window,"&Undo")->trigger();app.processEvents();require(visible(1700,2900),"Whole-row patches navigate to first actual change");}
    away();const int horizontal=view->horizontalScrollBar()->value(),vertical=view->verticalScrollBar()->value();document.rename({{1801,"Renamed"}});view->refresh();if(view->changed)view->changed();action(window,"&Undo")->trigger();require(view->horizontalScrollBar()->value()==horizontal&&view->verticalScrollBar()->value()==vertical,"Metadata Undo does not jump");
    window.close();std::cout<<"PASS 0.19.3: selection pixels and undo/redo navigation for rewrite, paste, insert, deletion, gap lock, block slide and full-row patches; selections preserved; metadata does not jump\n";
}catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}return 0;}
