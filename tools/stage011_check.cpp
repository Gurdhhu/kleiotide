#include "gui/alignment_view.hpp"
#include "gui/main_window.hpp"
#include "core/abif.hpp"
#include "core/session.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QElapsedTimer>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QThread>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void key(AlignmentView& v,int code){QKeyEvent event(QEvent::KeyPress,code,Qt::NoModifier);QApplication::sendEvent(&v,&event);QApplication::processEvents();}
QPointF cell(AlignmentView& v,int row,int column){return {v.name_area_width()+(column+.5)*v.cell_width(),v.ruler_height()+(row+.5)*v.row_height()};}
void mouse(AlignmentView& v,QEvent::Type type,QPointF p,Qt::KeyboardModifiers mods=Qt::NoModifier){const bool release=type==QEvent::MouseButtonRelease;QMouseEvent event(type,p,QPointF(v.viewport()->mapToGlobal(p.toPoint())),type==QEvent::MouseMove?Qt::NoButton:Qt::LeftButton,release?Qt::NoButton:Qt::LeftButton,mods);QApplication::sendEvent(v.viewport(),&event);QApplication::processEvents();}
void click(AlignmentView& v,QPointF p,Qt::KeyboardModifiers mods=Qt::NoModifier){mouse(v,QEvent::MouseButtonPress,p,mods);mouse(v,QEvent::MouseButtonRelease,p,mods);}
void drag(AlignmentView& v,QPointF a,QPointF b){mouse(v,QEvent::MouseButtonPress,a);mouse(v,QEvent::MouseMove,b);mouse(v,QEvent::MouseButtonRelease,b);}
QImage glyph(AlignmentView& v,int row,int column){return v.viewport()->grab(QRect(v.name_area_width()+column*v.cell_width(),v.ruler_height()+row*v.row_height(),v.cell_width(),v.row_height())).toImage();}
template<class F>void wait(F done){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<15000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Async timeout");}
void deletion(){
    for(auto mode:{AlignmentView::Mode::Insert,AlignmentView::Mode::Rewrite}){
        for(bool backspace:{false,true})for(const auto [start,width]:{std::pair<std::size_t,std::size_t>{2,1},{1,3},{0,2},{5,1},{0,6}}){
            std::istringstream evidence(test::synthetic_abif("ACGTAC"));Document d;d.load({read_abif(evidence,"linked","fixture.ab1"),{"other","TTTTTT"}});d.replace(1,2,1,"-");d.replace(1,4,1,"~");
            AlignmentView v;v.resize(800,300);v.set_document(&d);v.set_mode(mode);v.show();v.select_ids({1,2});v.select_fragment({{1},start,width});
            const auto before=d.sequence(1).symbols.text();const auto link=d.sequence(1).trace;key(v,backspace?Qt::Key_Backspace:Qt::Key_Delete);
            auto expected=before;expected.erase(start,width);const auto target=std::min(backspace&&start?start-1:start,expected.size());
            require(d.sequence(1).symbols.text()==expected&&d.sequence(2).symbols.text()=="TTTTTT","Deletion affects selected cells only");
            require(v.caret()&&v.caret()->second==target,"Directional deletion caret");
            require(target==expected.size()?!v.fragment():v.fragment()&&v.fragment()->start==target&&v.fragment()->width==1,"Single symbol or end caret after deletion");
            validate_trace(d.sequence(1));require(v.selected_ids()==std::vector<SequenceId>({1,2}),"Deletion preserves name selection");
            d.undo();v.refresh();require(d.sequence(1).symbols.text()==before&&d.sequence(1).trace==link&&v.fragment()->start==start&&v.fragment()->width==width,"Undo restores exact text/provenance/selection");
        }
        Document d;d.load({{"dna","AC-G~T"},{"other","AC"},{"mask","12-3~4",RowKind::Mask}});AlignmentView v;v.set_document(&d);v.set_mode(mode);v.select_ids({2});v.select_fragment({{1,3},1,4});key(v,Qt::Key_Backspace);
        require(d.sequence(1).symbols.text()=="AT"&&d.sequence(2).symbols.text()=="AC"&&d.sequence(3).symbols.text()=="14"&&!v.caret(),"Backspace block keeps no ambiguous caret");d.undo();v.refresh();require(v.fragment()->ids==std::vector<SequenceId>({1,3}),"Block undo restores rows");
        v.navigate_to_cell(1,6);key(v,Qt::Key_Backspace);require(d.sequence(1).symbols.text()=="AC-G~"&&v.caret()->second==5,"End caret Backspace");
    }
}
void selections(){
    Document d;d.load({{"one","ACGT~~~T"},{"two","ACGT~~~T"},{"mask","12-3~4??",RowKind::Mask}});AlignmentView v;v.resize(800,300);v.set_document(&d);v.show();QApplication::processEvents();
    for(auto mode:{AlignmentView::Mode::View,AlignmentView::Mode::Insert,AlignmentView::Mode::Rewrite}){
        v.set_mode(mode);key(v,Qt::Key_Escape);v.select_ids({1,3});click(v,cell(v,0,1));
        require(v.fragment()->ids==std::vector<SequenceId>({1})&&v.selected_ids()==std::vector<SequenceId>({1,3}),"Click independent of selected names in every mode");
        key(v,Qt::Key_Escape);v.select_ids({1,3});drag(v,cell(v,0,1),cell(v,0,3));require(v.fragment()->ids==std::vector<SequenceId>({1})&&v.fragment()->width==3,"Horizontal drag stays one row");
        click(v,cell(v,2,4),Qt::ShiftModifier);require(v.fragment()->ids==std::vector<SequenceId>({1,2,3})&&v.fragment()->start==1&&v.fragment()->width==4,"Shift block uses spatial rows including unselected row");
        v.select_ids({2});require(v.fragment()->ids.size()==3,"Name command preserves residue block");v.unselect_all();require(v.fragment()->ids.size()==3,"Unselect names preserves residues");
        v.select_ids({1});click(v,{v.name_area_width()+2.5*v.cell_width(),v.ruler_height()/2.});require(v.fragment()->ids.size()==3&&v.fragment()->width==1,"Ruler includes every row irrespective of names");
        click(v,{12,cell(v,1,0).y()});require(v.selected_ids()==std::vector<SequenceId>({2})&&v.fragment()->ids.size()==3,"Name click preserves column selection");
        click(v,{12,cell(v,4,0).y()});require(v.selected_ids().empty()&&v.fragment()->ids.size()==3,"Blank names clears only names");
        v.select_ids({2});v.navigate_to_cell(1,3);v.select_trace_fragment(1,2,2);require(v.selected_ids()==std::vector<SequenceId>({2})&&v.fragment()->ids==std::vector<SequenceId>({1}),"Trace selection preserves names");
        key(v,Qt::Key_Escape);require(v.selected_ids().empty()&&!v.fragment()&&!v.caret(),"Escape clears both");
    }
}
void dots(){
    Document d;d.load({{"reference","AC-G~TRNYa"},{"match","ac~G-TANYA"},{"mask","AC-G~TRNYa",RowKind::Mask},{"short","AC"}});AlignmentView v;v.resize(800,300);v.set_document(&d);v.show();QApplication::processEvents();
    const auto top= glyph(v,0,0),letter=glyph(v,1,0),gap=glyph(v,1,2),different=glyph(v,1,6),mask=glyph(v,2,0),tail=glyph(v,1,9);
    const auto state=d.state_id();v.set_identity_dots(true);QApplication::processEvents();const auto dot=glyph(v,1,0);
    require(dot!=letter&&dot==glyph(v,1,1)&&dot==glyph(v,1,7)&&dot==glyph(v,1,9),"Exact case-insensitive and ambiguous matches draw same neutral dot");
    require(glyph(v,0,0)==top&&glyph(v,1,2)==gap&&glyph(v,1,6)==different&&glyph(v,2,0)==mask,"Top/gaps/masks/mismatches retain glyphs");
    require(glyph(v,3,0)==dot&&d.state_id()==state,"Ragged row and render-only state");
    v.select_fragment({{2},0,10});require(v.selected_residue_text()==QString("ac~G-TANYA"),"Dots copy actual substring");key(v,Qt::Key_Escape);
    d.replace(1,0,1,"T");v.refresh();require(glyph(v,1,0)==letter,"Editing reference invalidates cached dots");d.undo();v.refresh();require(glyph(v,1,0)==dot,"Undo restores dots");
    d.reorder({2,1,3,4});v.refresh();require(glyph(v,0,0)==letter&&glyph(v,1,0)==dot,"Reordering follows current top row");d.undo();v.refresh();
    d.reorder({3,1,2,4});v.refresh();require(glyph(v,2,0)==letter,"Mask top row disables nucleotide comparison");d.undo();v.refresh();
    v.set_identity_dots(false);require(glyph(v,1,0)==letter&&glyph(v,1,9)==tail,"Toggle restores letters");
}
void sessions(){
    QTemporaryDir dir;Document d;d.load({{"one","ACGT"},{"two","aCGT"}});const auto path=dir.filePath("display.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,d,"{\"version\":1}");}
    MainWindow w;w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});auto* toggle=w.findChild<QAction*>("identityDots");require(toggle&&!toggle->isChecked()&&!v->identity_dots(),"Old session defaults to letters");toggle->setChecked(true);
    v->select_ids({1});v->select_fragment({{2},1,2});for(auto* a:w.findChildren<QAction*>())if(a->text()=="&Save")a->trigger();wait([&]{return v->isEnabled();});
    MainWindow restored;restored.show();restored.open_path(path);auto* rv=dynamic_cast<AlignmentView*>(restored.centralWidget());wait([&]{return rv->isEnabled();});
    require(rv->identity_dots()&&restored.findChild<QAction*>("identityDots")->isChecked(),"Identity toggle restored");require(rv->selected_ids()==std::vector<SequenceId>({1})&&rv->fragment()->ids==std::vector<SequenceId>({2}),"Independent selections roundtrip");require(restored.current_document().sequence(2).symbols.text()=="aCGT","Native session stores real letters");
    for(auto* a:restored.findChildren<QAction*>())if(a->text()=="Copy fragment")a->trigger();require(QApplication::clipboard()->text()=="CG","Names do not change fragment copy");for(auto* a:restored.findChildren<QAction*>())if(a->text()=="Copy selected sequences")a->trigger();require(QApplication::clipboard()->text()==">one\nACGT\n","Explicit sequence copy");rv->unselect_all();for(auto* a:restored.findChildren<QAction*>())if(a->text()=="Copy fragment")a->trigger();require(QApplication::clipboard()->text()=="CG","Clearing names preserves substring copy");restored.close();w.close();
}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{deletion();selections();dots();sessions();std::cout<<"PASS 0.11: directional deletion/boundaries/provenance/undo, independent name/residue/ruler/trace selection in all modes, identity rendering/edit/reorder/cache/copy/native persistence\n";}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
