#include "gui/alignment_view.hpp"
#include <QApplication>
#include <QFontMetrics>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int ink_height(const QImage& image){
    int top=image.height(),bottom=-1;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(image.pixelColor(x,y)!=QColor(238,240,244)){top=std::min(top,y);bottom=std::max(bottom,y);}
    return bottom<0?0:bottom-top+1;
}
int main(int argc,char**argv){
    QApplication app(argc,argv);
    try{
        Document d;d.load({{"one long descriptive sequence name",std::string(5000,'A')},{"two",std::string(5000,'A')},{"three",std::string(5000,'A')}});
        AlignmentView v;v.resize(900,320);v.set_document(&d);v.show();app.processEvents();
        auto mouse=[&](QEvent::Type type,QPointF p,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent event(type,p,QPointF(v.viewport()->mapToGlobal(p.toPoint())),button,buttons,Qt::NoModifier);QApplication::sendEvent(v.viewport(),&event);app.processEvents();};
        auto resize=[&](int width){const auto start=QPointF(v.name_area_width()-1,10);const auto end=QPointF(width-1,10);mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,end,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,end,Qt::LeftButton,Qt::NoButton);};
        const auto original=d.state_id();v.set_mode(AlignmentView::Mode::Rewrite);v.select_ids({d.rows()[0].id});v.select_fragment({{d.rows()[0].id},10,2});v.horizontalScrollBar()->setValue(20);
        mouse(QEvent::MouseMove,QPointF(v.name_area_width()-1,10),Qt::NoButton,Qt::NoButton);require(v.viewport()->cursor().shape()==Qt::SplitHCursor,"Divider hover cursor");
        resize(420);require(v.name_area_width()==420&&v.horizontalScrollBar()->value()==20,"Drag wider retains left column");
        require(v.selected_ids().size()==1&&v.fragment()&&v.fragment()->start==10&&v.fragment()->width==2&&d.state_id()==original,"Resize preserves selection and document");
        resize(-200);require(v.name_area_width()==80,"Minimum name width");resize(10000);require(v.name_area_width()==v.viewport()->width()-4*v.cell_width(),"Keep residue area visible");resize(340);
        mouse(QEvent::MouseMove,QPointF(20,10),Qt::NoButton,Qt::NoButton);require(v.viewport()->cursor().shape()!=Qt::SplitHCursor,"Cursor leaves divider");
        v.unselect_all();v.horizontalScrollBar()->setValue(0);auto font=v.font();font.setPointSize(6);v.setFont(font);app.processEvents();
        auto rulerFont=font;rulerFont.setPointSize(6);const QFontMetrics fm(rulerFont,v.viewport());require(v.ruler_height()-10>=fm.height(),"Small ruler reserves full font height");
        require(v.row_height()==std::max(12,QFontMetrics(font,v.viewport()).height()+4),"Small-font rows remain compact");
        auto screenshot=v.viewport()->grab().toImage();const auto dpr=screenshot.devicePixelRatio();const int center=v.name_area_width()+9*v.cell_width()+v.cell_width()/2;
        const auto digits=screenshot.copy(QRect(qRound((center-20)*dpr),0,qRound(40*dpr),qRound((v.ruler_height()-9)*dpr)));
        QImage reference(qRound(40*dpr),qRound((fm.height()+8)*dpr),QImage::Format_RGB32);reference.setDevicePixelRatio(dpr);reference.fill(QColor(238,240,244));
        {QPainter painter(&reference);painter.setFont(rulerFont);painter.setPen(QColor(90,95,105));painter.drawText(QPoint(2,fm.ascent()+3),"10");}
        require(std::abs(ink_height(digits)-ink_height(reference))<=1&&ink_height(digits)>0,"Ruler digits have full rendered glyph height");
        if(argc>1)require(v.grab().save(QString::fromLocal8Bit(argv[1])),"Layout screenshot");
        auto point=[&](int row,int column){return QPointF(v.name_area_width()+(column+.5)*v.cell_width(),v.ruler_height()+(row+.5)*v.row_height());};
        const auto cell=point(1,3);mouse(QEvent::MouseButtonPress,cell,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,cell,Qt::LeftButton,Qt::NoButton);
        require(v.fragment()&&v.fragment()->ids.front()==d.rows()[1].id&&v.fragment()->start==3,"Cell hit after border/font change");
        v.set_mode(AlignmentView::Mode::Insert);const auto caret=point(2,2);mouse(QEvent::MouseButtonPress,caret,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,caret,Qt::LeftButton,Qt::NoButton);
        require(v.paste_residues("N")&&d.rows()[2].symbols.text().substr(0,4)=="AAAN","Caret paste after layout change");
        std::cout<<"PASS layout: divider drag/cursor/limits/preserved selection, compact font-6 rows, uncropped ruler, cell/caret hits\n";
    }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
