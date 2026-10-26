#include "gui/alignment_view.hpp"
#include <QApplication>
#include <QMouseEvent>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
QPoint point(AlignmentView& v,int column){return {v.name_area_width()+column*v.cell_width()+v.cell_width()/2,v.ruler_height()+v.row_height()/2};}
void mouse(AlignmentView& v,QEvent::Type type,QPoint p,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent e(type,QPointF(p),QPointF(v.viewport()->mapToGlobal(p)),button,buttons,Qt::NoModifier);QApplication::sendEvent(v.viewport(),&e);QApplication::processEvents();}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    for(auto mode:{AlignmentView::Mode::Rewrite,AlignmentView::Mode::Insert})for(bool changeDisplay:{false,true}){
        Document doc;doc.load({{"first","~~ACG~~T"},{"second","~~ACT~~T"}});AlignmentView view;view.resize(900,300);view.set_document(&doc);view.set_mode(mode);view.show();QApplication::processEvents();
        const auto before=doc.state_id();view.select_fragment({{1,2},2,3});
        mouse(view,QEvent::MouseButtonPress,point(view,3),Qt::LeftButton,Qt::LeftButton);
        mouse(view,QEvent::MouseMove,point(view,5),Qt::NoButton,Qt::LeftButton);
        mouse(view,QEvent::MouseButtonRelease,point(view,5),Qt::LeftButton,Qt::NoButton);
        require(doc.sequence(1).symbols.text()=="~~~~ACGT"&&view.fragment()->start==4,"Mouse drag commits the block and selection");
        doc.undo();
        // A display refresh must not acknowledge an undo on behalf of selection
        // history. This ordering can occur independently of mouse/key input.
        if(changeDisplay)view.set_identity_dots(true);
        view.refresh();QApplication::processEvents();
        require(doc.state_id()==before&&doc.sequence(1).symbols.text()=="~~ACG~~T","Undo restores residues");
        require(view.fragment()&&view.fragment()->start==2&&view.fragment()->width==3&&view.fragment()->ids==std::vector<SequenceId>{1,2},"Undo must restore mouse-drag selection, including after a display-cache refresh");
        doc.redo();if(changeDisplay)view.set_residue_display(ResidueDisplay::Backgrounds);view.refresh();
        require(view.fragment()->start==4&&view.fragment()->width==3,"Redo restores moved block selection");
        doc.undo();view.set_residue_palette(ResiduePalette::Chromas);view.refresh();require(view.fragment()->start==2,"Palette cache changes cannot suppress selection undo");
    }
    std::cout<<"PASS drag history: mouse block movement/undo/redo in both edit modes, with independent display-cache invalidation\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
