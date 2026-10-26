#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "core/abif.hpp"
#include "core/session.hpp"
#include "core/fasta.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QElapsedTimer>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QScrollBar>
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
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>void wait(F done){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Async timeout");}
void key(AlignmentView& v,int code,const QString& text={}){QKeyEvent e(QEvent::KeyPress,code,Qt::NoModifier,text);QApplication::sendEvent(&v,&e);QApplication::processEvents();}
QImage cell(AlignmentView& v,int row,int column){return v.viewport()->grab(QRect(v.name_area_width()+column*v.cell_width(),v.ruler_height()+row*v.row_height(),v.cell_width(),v.row_height())).toImage();}
QColor background(AlignmentView& v,int row,int column){auto image=cell(v,row,column);return image.pixelColor(image.width()/2,image.height()-3);}
QAction* named(MainWindow& w,const QString& id){auto* a=w.findChild<QAction*>(id);require(a,"Named action");return a;}
QAction* action(MainWindow& w,const QString& text){for(auto* a:w.findChildren<QAction*>())if(a->text().section('\t',0,0)==text)return a;throw std::runtime_error("Missing action");}
ChromatogramPanel* traces(MainWindow& w){for(auto* q:w.findChildren<QWidget*>())if(auto* p=dynamic_cast<ChromatogramPanel*>(q))return p;throw std::runtime_error("Trace panel");}
void raster_checks(){Document d;d.load({{"reference","ACGTURYN?~-"},{"identical","ACGTURYN?~-"},{"mask","ACGTURYN?~-",RowKind::Mask}});AlignmentView v;v.resize(850,320);v.set_document(&d);v.show();QApplication::processEvents();const auto original=cell(v,0,0),mask=cell(v,2,0);const auto state=d.state_id();
    for(int palette=0;palette<residue_palette_count;++palette)for(int style=0;style<3;++style){v.set_residue_palette(static_cast<ResiduePalette>(palette));v.set_residue_display(static_cast<ResidueDisplay>(style));v.set_identity_dots(false);key(v,Qt::Key_Escape);
        for(int col=0;col<11;++col){const auto paint=residue_paint(d.sequence(1).symbols.text()[static_cast<std::size_t>(col)],v.residue_palette(),v.residue_display());require(background(v,0,col)==(paint.background.alpha()?paint.background:QColor(Qt::white)),"Raster cell background follows style");}require(cell(v,2,0)==mask,"Mask raster remains neutral in all modes");
        v.set_identity_dots(true);require(background(v,1,0)==background(v,0,0)&&cell(v,1,0)!=cell(v,0,0),"Dots retain underlying cell color and differ from letters");
        v.set_mode(AlignmentView::Mode::Insert);v.select_fragment({{1},1,1});require(background(v,0,1)==QColor(225,235,251),"Single Insert caret remains blue");v.select_fragment({{1},1,3});require(background(v,0,2)==QColor(250,235,191),"Wide Insert selection remains yellow");
        v.set_mode(AlignmentView::Mode::Rewrite);require(background(v,0,2)==QColor(225,235,251),"Single-row Overwrite selection remains blue");v.select_fragment({{1,2},1,3});require(background(v,1,2)==QColor(250,235,191),"Multirow Overwrite block remains yellow");
        key(v,Qt::Key_Escape);v.select_fragment({{2},0,4});require(v.selected_residue_text()==QString("ACGT"),"Every display copies actual residues");key(v,Qt::Key_Escape);
    }
    v.set_identity_dots(false);v.set_residue_display(ResidueDisplay::Letters);v.set_residue_palette(ResiduePalette::Classic);require(cell(v,0,0)==original&&d.state_id()==state&&!d.can_undo(),"Restore legacy pixels; displays never edit data/history");
    v.set_residue_palette(ResiduePalette::OkabeIto);v.set_residue_display(ResidueDisplay::Backgrounds);v.set_mode(AlignmentView::Mode::Rewrite);v.select_fragment({{1},1,2});key(v,Qt::Key_A,"A");require(d.sequence(1).symbols.text()=="AATURYN?~-"&&v.caret()->second==2,"Overwrite works in colored backgrounds");d.undo();v.refresh();require(d.sequence(1).symbols.text()=="ACGTURYN?~-","Undo works with styled selection");
}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{raster_checks();QTemporaryDir dir;const auto path=dir.filePath("display.kleio");std::string calls;for(int i=0;i<25;++i)calls+="ACGT";std::istringstream input(test::synthetic_abif(calls));Document d;d.load({read_abif(input,"linked trace","fixture.ab1"),{"same",calls},{"mask","ACGT0123",RowKind::Mask}});{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,d,"{\"version\":1}");}
    MainWindow w;w.resize(1280,850);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});auto* toolbar=w.findChild<QToolBar*>("editingToolbar");auto* mono=named(w,"residueDisplay.monochrome");auto* bg=named(w,"residueDisplay.backgrounds");auto* letters=named(w,"residueDisplay.letters");auto* mb=qobject_cast<QToolButton*>(toolbar->widgetForAction(mono));auto* bb=qobject_cast<QToolButton*>(toolbar->widgetForAction(bg));auto* pb=w.findChild<QToolButton*>("residuePaletteButton");require(mb&&bb&&pb&&!mb->toolTip().isEmpty()&&!bb->toolTip().isEmpty()&&!pb->toolTip().isEmpty(),"Toolbar icons/tooltips present");
    require(named(w,"residuePalette.classic")->text()=="Kleio"&&named(w,"residuePalette.chromas")->text()=="Chromas-style","Requested palette labels");require(w.findChild<QWidget*>("alignmentOverview")->toolTip().contains("Color: conserved column (blue) -> variable (orange), gray = no information"),"Requested overview explanation");
    require(v->residue_palette()==ResiduePalette::Classic&&v->residue_display()==ResidueDisplay::Letters&&letters->isChecked(),"Old sessions use Kleio letter defaults");const auto evidence=w.current_document().sequence(1).trace->original;const auto state=w.current_document().state_id();
    mb->click();require(v->residue_display()==ResidueDisplay::Monochrome&&mono->isChecked()&&mb->isChecked()&&!bg->isChecked(),"Monochrome toolbar/menu synchronized");mb->click();require(v->residue_display()==ResidueDisplay::Letters&&letters->isChecked()&&!mb->isChecked(),"Monochrome toggles back to letters");bg->trigger();require(bb->isChecked()&&v->residue_display()==ResidueDisplay::Backgrounds,"Menu enables backgrounds");mono->trigger();require(mb->isChecked()&&!bb->isChecked()&&v->residue_display()==ResidueDisplay::Monochrome,"Display modes are exclusive");bg->trigger();
    const auto icon=named(w,"identityDots")->icon().pixmap(20).toImage();
    for(int i=0;i<residue_palette_count;++i){const auto p=static_cast<ResiduePalette>(i);auto* a=named(w,"residuePalette."+palette_id(p));require(pb->menu()->actions().contains(a),"Toolbar and View share palette actions");a->trigger();require(a->isChecked()&&v->residue_palette()==p&&traces(w)->canvas()->residue_palette()==p&&pb->toolTip().contains(palette_label(p)),"Palette updates menu/toolbar/grid/trace");for(char c:std::string("ACGT"))require(w.findChild<QLabel*>("traceLegend")->text().contains(residue_ink(c,p).name()),"Trace legend matches actual channel inks");}
    require(named(w,"identityDots")->icon().pixmap(20).toImage()!=icon,"Identity icon reflects chosen palette");
    v->select_ids({1});v->select_fragment({{2},1,3});named(w,"identityDots")->trigger();action(w,"Copy selected sequences")->trigger();std::istringstream copied(QApplication::clipboard()->text().toUtf8().toStdString());const auto copiedRows=read_fasta(copied);require(copiedRows.size()==1&&copiedRows.front().symbols==calls,"FASTA copy preserves actual letters");require(w.current_document().state_id()==state&&w.current_document().sequence(1).trace->original==evidence&&!w.current_document().can_undo(),"View settings preserve data/history/evidence");
    for(int p=0;p<residue_palette_count;++p){named(w,"residuePalette."+palette_id(static_cast<ResiduePalette>(p)))->trigger();if(argc>1){QApplication::processEvents();require(w.grab().save(QString::fromLocal8Bit(argv[1])+"_"+QString::number(p)+".png"),"Palette screenshot");}}named(w,"residuePalette.chromas")->trigger();action(w,"&Save")->trigger();wait([&]{return v->isEnabled();});
    MainWindow restored;restored.show();restored.open_path(path);auto* rv=dynamic_cast<AlignmentView*>(restored.centralWidget());wait([&]{return rv->isEnabled();});require(rv->residue_palette()==ResiduePalette::Chromas&&rv->residue_display()==ResidueDisplay::Backgrounds&&rv->identity_dots(),"Session restores palette/display/dots together");require(named(restored,"residuePalette.chromas")->isChecked()&&named(restored,"residueDisplay.backgrounds")->isChecked()&&traces(restored)->canvas()->residue_palette()==ResiduePalette::Chromas,"Restored controls and trace stay in sync");require(rv->selected_ids()==std::vector<SequenceId>({1})&&rv->fragment()->ids==std::vector<SequenceId>({2})&&restored.current_document().sequence(1).trace->original->channels==evidence->channels,"Independent selections and original signals persist");
    const auto invalid=dir.filePath("unknown.kleio");{std::ofstream out(invalid.toStdString(),std::ios::binary);write_session(out,d,"{\"version\":1,\"residuePalette\":\"future\",\"residueDisplay\":\"future\"}");}restored.open_path(invalid);wait([&]{return rv->isEnabled();});require(rv->residue_palette()==ResiduePalette::Classic&&rv->residue_display()==ResidueDisplay::Letters,"Unknown session values fall back safely");restored.close();w.close();std::cout<<"PASS 0.14: palette/style raster matrix, selection colors, masks/dots, editing/undo/copy, toolbar/menu/trace synchronization, native preferences and safe defaults\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
