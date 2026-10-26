#include "gui/alignment_view.hpp"
#include "gui/main_window.hpp"
#include "core/fasta.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QAction* find(const QList<QAction*>& actions,const QString& text){for(auto* a:actions){auto name=a->text();name.remove('&');if(name==text)return a;if(a->menu())if(auto* child=find(a->menu()->actions(),text))return child;}return nullptr;}
QAction* action(MainWindow&w,const QString& text){auto* a=find(w.menuBar()->actions(),text);require(a,"Missing sort action");return a;}
void settle(QApplication& app,AlignmentView& v){QElapsedTimer t;t.start();while(!v.isEnabled()&&t.elapsed()<10000)app.processEvents();app.processEvents();require(v.isEnabled(),"Async operation timed out");}
int main(int argc,char**argv){
    QApplication app(argc,argv);
    try{
        Document d;d.load({{"DNA","ACGT~~"},{"mask","ACGT01"},{"two","A~CG~~"},{"three","TT"},{"last","NN"}});
        AlignmentView v;v.resize(1000,340);v.set_document(&d);v.show();app.processEvents();std::vector<SequenceId> ids;for(const auto& row:d.rows())ids.push_back(row.id);
        auto point=[&](int row,int column){return QPointF(v.name_area_width()+(column+.5)*v.cell_width(),v.ruler_height()+(row+.5)*v.row_height());};
        auto mouse=[&](QEvent::Type type,QPointF p,Qt::MouseButton button,Qt::MouseButtons buttons,Qt::KeyboardModifiers mods=Qt::NoModifier){QMouseEvent e(type,p,QPointF(v.viewport()->mapToGlobal(p.toPoint())),button,buttons,mods);QApplication::sendEvent(v.viewport(),&e);app.processEvents();};
        auto click=[&](QPointF p,Qt::KeyboardModifiers mods=Qt::NoModifier){mouse(QEvent::MouseButtonPress,p,Qt::LeftButton,Qt::LeftButton,mods);mouse(QEvent::MouseButtonRelease,p,Qt::LeftButton,Qt::NoButton,mods);};
        auto drag=[&](QPointF a,QPointF b){mouse(QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);};
        const auto rendered=v.viewport()->grab().toImage();const auto ratio=rendered.devicePixelRatio();bool gray=false,yellow=false;
        for(int row=0;row<2;++row)for(int y=v.ruler_height()+row*v.row_height();y<v.ruler_height()+(row+1)*v.row_height();++y)for(int x=v.name_area_width();x<v.name_area_width()+v.cell_width();++x){const auto color=rendered.pixelColor(qRound(x*ratio),qRound(y*ratio));if(row==0&&color==QColor(180,125,0))yellow=true;if(row==1){if(color==QColor(65,70,75))gray=true;require(color!=QColor(180,125,0),"Mask A never uses nucleotide color");}}
        require(gray&&yellow,"Cached mask and nucleotide glyph colors");
        v.select_ids({ids[1],ids[3]});drag(QPointF(15,point(1,0).y()),QPointF(15,v.ruler_height()+5*v.row_height()));
        require(d.rows()[3].id==ids[1]&&d.rows()[4].id==ids[3]&&v.selected_ids().size()==2,"Noncontiguous selected rows drag to end");d.undo();v.refresh();
        drag(QPointF(15,point(3,0).y()),QPointF(15,v.ruler_height()-4));require(d.rows()[0].id==ids[1]&&d.rows()[1].id==ids[3],"Drag selected group to top");d.undo();v.refresh();
        mouse(QEvent::MouseButtonPress,QPointF(15,point(1,0).y()),Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,QPointF(15,v.ruler_height()+5*v.row_height()),Qt::NoButton,Qt::LeftButton);
        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(&v,&escape);mouse(QEvent::MouseButtonRelease,QPointF(15,v.ruler_height()+5*v.row_height()),Qt::LeftButton,Qt::NoButton);
        require(d.rows()[1].id==ids[1]&&v.selected_ids().empty(),"Escape cancels row drag and clears names in any mode");
        click(QPointF(15,point(1,0).y()));require(v.selected_ids()==std::vector<SequenceId>{ids[1]},"Plain click still selects one name");click(QPointF(15,point(3,0).y()),Qt::ControlModifier);require(v.selected_ids().size()==2,"Ctrl selection remains usable");
        v.unselect_all();v.set_mode(AlignmentView::Mode::Rewrite);drag(point(0,1),point(1,2));require(v.fragment()&&v.fragment()->ids.size()==2,"Mixed mask/DNA rectangle");v.slide_fragment(2);require(d.sequence(ids[1]).symbols.text()=="A~~CGT01","Mixed mask block pushing");d.undo();v.refresh();
        v.set_mode(AlignmentView::Mode::Rewrite);click(point(1,0));QKeyEvent rewrite(QEvent::KeyPress,Qt::Key_NumberSign,Qt::NoModifier,"#");QApplication::sendEvent(&v,&rewrite);require(d.sequence(ids[1]).symbols.text()=="#CGT01","Mask overwrite arbitrary symbol");d.undo();v.refresh();
        v.set_mode(AlignmentView::Mode::Insert);click(point(1,2));require(v.paste_residues("9*xy")&&d.sequence(ids[1]).symbols.text()=="ACG9*xyT01","Mask paste insertion");d.undo();v.refresh();
        if(argc>1)require(v.grab().save(QString::fromLocal8Bit(argv[1])),"Mask/drag screenshot");
        Document tall;std::vector<Record> many;for(int i=0;i<100;++i)many.push_back({std::to_string(i),"AC"});tall.load(many);v.set_document(&tall);v.select_ids({tall.rows()[0].id});
        mouse(QEvent::MouseButtonPress,QPointF(15,v.ruler_height()+v.row_height()/2),Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,QPointF(15,v.viewport()->height()+40),Qt::NoButton,Qt::LeftButton);
        QEventLoop loop;QTimer::singleShot(180,&loop,&QEventLoop::quit);loop.exec();require(v.verticalScrollBar()->value()>0,"Row drag edge autoscroll");mouse(QEvent::MouseButtonRelease,QPointF(15,v.viewport()->height()+40),Qt::LeftButton,Qt::NoButton);
        QTemporaryDir dir;const auto path=dir.filePath("sort_mask.fasta");QFile file(path);require(file.open(QIODevice::WriteOnly),"Sort fixture");file.write(">z\nAAA~~\n>b\nCC\n>A\naaa~~\n>a\nA--\n>x\nG\n>mask\nACGT01\n");file.close();
        MainWindow w;w.show();auto* view=dynamic_cast<AlignmentView*>(w.centralWidget());w.open_path(path);settle(app,*view);
        Document expected;std::istringstream in(">z\nAAA~~\n>b\nCC\n>A\naaa~~\n>a\nA--\n>x\nG\n>mask\nACGT01\n");expected.load(read_fasta(in));
        auto exported=[&]{view->select_all();action(w,"Copy selected sequences")->trigger();return QApplication::clipboard()->text();};const auto original=exported();
        const std::pair<const char*,Document::Sort> methods[]={{"By title",Document::Sort::Title},{"By length (descending)",Document::Sort::LengthDescending},{"By length (ascending)",Document::Sort::LengthAscending},{"By true length (descending)",Document::Sort::TrueLengthDescending},{"By true length (ascending)",Document::Sort::TrueLengthAscending},{"By summed residue frequencies in selected columns",Document::Sort::ResidueFrequency},{"Reverse order",Document::Sort::Reverse}};
        for(const auto& [label,method]:methods){
            std::optional<Document::ColumnRange> columns;if(method==Document::Sort::ResidueFrequency){view->set_mode(AlignmentView::Mode::Rewrite);view->select_fragment({{view->selected_ids().front()},0,2});columns=Document::ColumnRange{0,2};}
            const auto before=expected.state_id();expected.reorder(expected.sorted_order(method,columns));std::ostringstream out;write_fasta(out,expected);
            action(w,QString::fromLatin1(label))->trigger();settle(app,*view);require(exported()==QString::fromStdString(out.str()),"Sort menu resulting FASTA");
            if(expected.state_id()!=before){expected.undo();action(w,"Undo")->trigger();require(exported()==original,"Sort menu atomic undo");action(w,"Redo")->trigger();require(exported()==QString::fromStdString(out.str()),"Sort redo");action(w,"Undo")->trigger();}
        }
        QKeyEvent clearSelection(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(view,&clearSelection);
        view->set_mode(AlignmentView::Mode::View);action(w,"By summed residue frequencies in selected columns")->trigger();require(exported()==original,"Missing selected columns causes no edit");
        view->select_all();action(w,"Reverse complement")->trigger();const auto complemented=exported();require(complemented.endsWith(">mask\nACGT01\n"),"Reverse complement menu skips mask");action(w,"Undo")->trigger();
        const auto unicodePath=dir.filePath("unicode.fasta");QFile unicodeFile(unicodePath);require(unicodeFile.open(QIODevice::WriteOnly),"Unicode title fixture");
        const auto unicode=QString::fromUtf8(">Бета\nA\n>Альфа\nC\n>альфа\nG\n");unicodeFile.write(unicode.toUtf8());unicodeFile.close();w.open_path(unicodePath);settle(app,*view);
        action(w,"By title")->trigger();settle(app,*view);
        require(exported()==QString::fromUtf8(">Альфа\nC\n>альфа\nG\n>Бета\nA\n"),"Title sorting case-folds Unicode and preserves equal-title order");action(w,"Undo")->trigger();require(exported()==unicode,"Unicode title undo");
        v.set_document(nullptr);std::cout<<"PASS stage04: gray masks, mixed block edits, typed/pasted mask symbols, selected-row dragging/click/autoscroll, all sort menu actions and undo/redo\n";
    }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
