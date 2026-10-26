#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/motif_navigator.hpp"
#include "core/session.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QThread>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F>void wait(F done){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Async timeout");QApplication::processEvents();}
QAction* action(MainWindow& w,const char* id){auto* a=w.findChild<QAction*>(id);require(a,"Named action");return a;}
void click(MainWindow& w,const char* id){auto* button=w.findChild<QPushButton*>(id);require(button&&button->isEnabled(),"Enabled search button");button->click();auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});}
void selected(AlignmentView& v,SequenceId id,std::size_t start,std::size_t width){require(v.fragment()&&v.fragment()->ids==std::vector<SequenceId>{id}&&v.fragment()->start==start&&v.fragment()->width==width,"Current search hit automatically selected");}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{QTemporaryDir dir;const auto path=dir.filePath("all_hits.kleio");Document d;d.load({{"forward","AAGAAG"},{"reverse","CTT"},{"palindrome","ATAT"},{"mask","AAG",RowKind::Mask},{"gapped","A-~AG"},{"unknown","NnNN"},{"mixed","NAN"}});
    {std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,d,"{\"version\":1}");}
    MainWindow w;w.resize(1280,850);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});const auto state=w.current_document().state_id();
    action(w,"findMotif")->trigger();auto* panel=dynamic_cast<MotifNavigator*>(w.findChild<QWidget*>("motifNavigator"));auto* query=w.findChild<QLineEdit*>("motifQuery");auto* counts=w.findChild<QLabel*>("motifCounts");auto* message=w.findChild<QLabel*>("motifResult");auto* regex=w.findChild<QCheckBox*>("motifRegex");auto* reverse=w.findChild<QCheckBox*>("motifReverse");
    auto* exclude=w.findChild<QCheckBox*>("motifExcludeNOnly");require(exclude&&!exclude->isChecked()&&!exclude->toolTip().isEmpty()&&!w.findChild<QPushButton*>("motifSelect"),"Optional N-only filter replaces redundant selection button");
    require(panel&&query&&counts&&regex&&reverse&&!regex->toolTip().isEmpty(),"Regex/count controls and tooltip");v->select_ids({3});query->setText("AAG");reverse->setChecked(true);click(w,"motifFind");
    require(panel->results()->total==4&&panel->results()->forward==3&&panel->results()->reverse==1&&counts->text().contains("Total: 4")&&counts->text().contains("Forward: 3")&&counts->text().contains("Reverse complement: 1"),"Complete strand counts");selected(*v,1,0,3);require(v->selected_ids()==std::vector<SequenceId>{3},"Name selection preserved");const auto all=panel->results();
    click(w,"motifNext");selected(*v,1,3,3);click(w,"motifNext");selected(*v,2,0,3);click(w,"motifNext");selected(*v,5,0,5);click(w,"motifNext");selected(*v,1,0,3);require(message->text().contains("Wrapped"),"Cached navigation wraps");click(w,"motifPrevious");selected(*v,5,0,5);
    v->select_fragment({{3},0,2});v->select_ids({1,2});require(v->search_results()==all&&panel->results()==all,"All highlights survive unrelated cell/name selection");
    for(int p=0;p<residue_palette_count;++p)for(int s=0;s<3;++s){v->set_residue_palette(static_cast<ResiduePalette>(p));v->set_residue_display(static_cast<ResidueDisplay>(s));v->set_identity_dots(true);QApplication::processEvents();
        const auto raster=v->viewport()->grab(QRect(v->name_area_width()+3*v->cell_width(),v->ruler_height(),3*v->cell_width(),v->row_height())).toImage();bool outline=false;
        for(int y=0;y<raster.height();++y)for(int x=0;x<raster.width();++x)outline|=raster.pixelColor(x,y)==QColor(147,84,185);require(outline,"Other matches outlined in every display style");
        if(static_cast<ResidueDisplay>(s)==ResidueDisplay::Backgrounds)require(raster.pixelColor(raster.width()/2,raster.height()-4)==QColor(231,164,255),"Bright background-mode search fill across all palettes");
        require(v->search_results()->coverage(4,0,3)==std::vector<unsigned char>(3,0),"Masks not highlighted");}
    v->set_residue_display(ResidueDisplay::Letters);v->set_residue_palette(ResiduePalette::Classic);v->set_identity_dots(false);QApplication::processEvents();
    auto shaded=v->viewport()->grab(QRect(v->name_area_width()+3*v->cell_width(),v->ruler_height(),v->cell_width(),v->row_height())).toImage();
    require(shaded.pixelColor(shaded.width()/2,shaded.height()-4)!=QColor(Qt::white),"Other matches have a light shade inside the outline");
    regex->setChecked(true);query->setText("AA(G|T)");require(v->search_results()==all&&!w.findChild<QCheckBox*>("motifCompatible")->isEnabled(),"Regex syntax preserves completed highlights and disables IUPAC expansion");click(w,"motifFind");selected(*v,1,0,3);require(panel->results()->total==4,"Regex all-hit counts");
    query->setText("AT");click(w,"motifFind");selected(*v,3,0,2);require(panel->results()->total==2&&panel->results()->forward==2&&panel->results()->reverse==2&&panel->results()->both==2&&counts->toolTip().contains("once"),"Both-strand counts and explanation");
    const auto both=panel->results();query->setText("[");click(w,"motifFind");require(panel->results()==both&&v->search_results()==both&&message->text().contains("Invalid regular expression"),"Invalid regex preserves all previous results");
    query->setText("AAG");w.findChild<QPushButton*>("motifFind")->click();w.findChild<QPushButton*>("motifCancel")->click();wait([&]{return v->isEnabled();});require(v->search_results()==both&&counts->text().contains("Total: 2"),"Cancelled full scan keeps previous highlights/counts");
    query->setText("^");click(w,"motifFind");require(!panel->results()->total&&counts->text().contains("empty regex matches ignored"),"Zero-length results skipped visibly");
    query->setText("AAG");w.findChild<QComboBox*>("motifScope")->setCurrentIndex(1);v->select_ids({2});click(w,"motifFind");selected(*v,2,0,3);require(panel->results()->total==1&&panel->results()->forward==0&&panel->results()->reverse==1,"Selected scope reverse-only counts");
    w.findChild<QComboBox*>("motifScope")->setCurrentIndex(0);click(w,"motifFind");if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Search screenshot");
    reverse->setChecked(false);regex->setChecked(false);w.findChild<QCheckBox*>("motifCompatible")->setChecked(true);query->setText("A");click(w,"motifFind");require(panel->results()->total==15,"Unfiltered compatible matches include N calls");
    const auto unfiltered=panel->results();exclude->setChecked(true);require(panel->results()==unfiltered,"Filter setting keeps previous results until Find");click(w,"motifFind");require(panel->results()->total==9&&counts->text().contains("Total: 9"),"N-only filter updates counts and highlights");selected(*v,1,0,1);click(w,"motifNext");selected(*v,1,1,1);
    regex->setChecked(true);query->setText("N{2}");click(w,"motifFind");require(!panel->results()->total,"Regex excludes N-only matches");exclude->setChecked(false);click(w,"motifFind");require(panel->results()->total==3,"Disabling filter restores N-only matches");
    regex->setChecked(false);w.findChild<QCheckBox*>("motifCompatible")->setChecked(false);query->setText("AAG");click(w,"motifFind");v->set_residue_display(ResidueDisplay::Backgrounds);QApplication::processEvents();if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Bright search screenshot");
    auto otherFill=[&]{const auto cell=v->viewport()->grab(QRect(v->name_area_width()+3*v->cell_width(),v->ruler_height(),v->cell_width(),v->row_height())).toImage();return cell.pixelColor(cell.width()/2,cell.height()-4);};
    query->setText("T");click(w,"motifFind");require(otherFill()!=QColor(231,164,255),"New search clears old background-mode tile shading");query->setText("AAG");click(w,"motifFind");require(otherFill()==QColor(231,164,255),"New background-mode results refresh cached hit tiles");
    require(w.current_document().state_id()==state&&!w.current_document().can_undo(),"Searching/selecting/highlighting never changes document history");
    w.findChild<QDockWidget*>("motifDock")->close();QApplication::processEvents();require(!v->search_results()&&!v->search_match()&&!panel->results(),"Closing panel removes all search highlighting/results");require(otherFill()!=QColor(231,164,255),"Closing panel clears cached match fill");w.close();
    std::cout<<"PASS 0.15.2: N-only filtering, brighter background-mode matches, automatic selection without redundant button; regex syntax/errors/cancel, complete strand counts, automatic first/next/previous selection, all-match outlines/shading across displays, persistence and close cleanup\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
