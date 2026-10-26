#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/motif_navigator.hpp"
#include "core/abif.hpp"
#include "core/fasta.hpp"
#include "core/session.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QThread>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>void wait(F done){QElapsedTimer timer;timer.start();while(!done()&&timer.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Async timeout");QApplication::processEvents();}
QAction* named(MainWindow& w,const QString& id){auto* a=w.findChild<QAction*>(id);require(a,"Action exists");return a;}
QAction* action(MainWindow& w,const QString& text){for(auto* a:w.findChildren<QAction*>())if(a->text().section('\t',0,0)==text)return a;throw std::runtime_error("Missing action");}
void click(MainWindow& w,const char* id){auto* b=w.findChild<QPushButton*>(id);require(b&&b->isEnabled(),"Enabled navigator button");b->click();auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});}
void expect(AlignmentView& v,SequenceId id,std::size_t start,std::size_t width){require(v.search_match()&&v.search_match()->ids==std::vector<SequenceId>{id}&&v.search_match()->start==start&&v.search_match()->width==width,"Expected independent match highlight");}
std::string bytes(const QString& path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Read saved file");return f.readAll().toStdString();}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir dir;const auto fasta=dir.filePath("wrapped.fasta");const std::string longSequence=std::string(242,'a')+"RYN~-";
    {std::ofstream out(fasta.toStdString());out<<">long\n"<<longSequence.substr(0,17)<<"\n"<<longSequence.substr(17)<<"\n>empty\n\n>mask\n012\n345\n";}
    MainWindow text;text.show();text.open_path(fasta);auto* tv=dynamic_cast<AlignmentView*>(text.centralWidget());wait([&]{return tv->isEnabled();});tv->select_all();
    action(text,"Copy selected sequences")->trigger();const auto expected=">long\n"+longSequence+"\n>empty\n\n>mask\n012345\n";require(QApplication::clipboard()->text().toStdString()==expected,"Clipboard FASTA has no sequence wrapping");
    action(text,"&Save")->trigger();wait([&]{return tv->isEnabled();});require(bytes(fasta)==expected,"Actual Save writes one line per sequence");text.close();
    const auto path=dir.filePath("navigator.kleio");std::istringstream imported(test::synthetic_abif("AAAACGTT"));Document d;
    std::vector<Record> rows={read_abif(imported,"duplicate","fixture.ab1"),{"duplicate","~~AAAACGTT"},{"mask","AAAAAG",RowKind::Mask},{"reverse","CTTATAT"}};
    for(int i=0;i<75;++i)rows.push_back({"filler",std::string(500,'T')});rows.push_back({"far hit",std::string(400,'T')+"AAG"});d.load(std::move(rows));
    {std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,d,"{\"version\":1}");}
    MainWindow w;w.resize(1280,850);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});
    const auto state=w.current_document().state_id();const auto original=w.current_document().sequence(1).trace->original;
    named(w,"findMotif")->trigger();auto* dock=w.findChild<QDockWidget*>("motifDock");require(dock&&dock->isVisible(),"Find opens navigator");
    auto* panel=dynamic_cast<MotifNavigator*>(w.findChild<QWidget*>("motifNavigator"));require(panel,"Navigator instance");auto* query=w.findChild<QLineEdit*>("motifQuery");auto* result=w.findChild<QLabel*>("motifResult");
    require(named(w,"findMotif")->shortcut()==QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_F)&&named(w,"findNextMotif")->shortcut()==QKeySequence(Qt::Key_F3)&&named(w,"findPreviousMotif")->shortcut()==QKeySequence(Qt::SHIFT|Qt::Key_F3),"Find shortcuts");
    require(action(w,"Names matching regular expression…")->shortcut()==QKeySequence::Find,"Ctrl+F name-regex shortcut retained");
    v->select_ids({4});v->select_fragment({{1},4,2});query->setText("AAA");click(w,"motifFind");expect(*v,1,0,3);
    require(v->selected_ids()==std::vector<SequenceId>{4}&&v->fragment()->start==0&&v->fragment()->width==3,"Search selects first hit while preserving name selection");
    named(w,"findNextMotif")->trigger();wait([&]{return v->isEnabled();});expect(*v,1,1,3);click(w,"motifPrevious");expect(*v,1,0,3);click(w,"motifPrevious");expect(*v,2,3,3);require(result->text().contains("Wrapped"),"Wrap is reported");
    w.findChild<QCheckBox*>("motifWrap")->setChecked(false);click(w,"motifFind");click(w,"motifPrevious");expect(*v,1,0,3);require(result->text().contains("No previous"),"End stop retains current hit");
    w.findChild<QCheckBox*>("motifWrap")->setChecked(true);w.findChild<QComboBox*>("motifScope")->setCurrentIndex(1);v->select_ids({2});click(w,"motifFind");expect(*v,2,2,3);v->select_ids({});click(w,"motifFind");require(!v->search_match()&&result->text().contains("No matching"),"Empty selected scope is not all rows");
    w.findChild<QComboBox*>("motifScope")->setCurrentIndex(0);query->setText("AAG");w.findChild<QCheckBox*>("motifReverse")->setChecked(true);click(w,"motifFind");expect(*v,4,0,3);require(result->text().contains("reverse complement"),"Reverse strand label");
    w.findChild<QCheckBox*>("motifReverse")->setChecked(false);click(w,"motifFind");expect(*v,80,400,3);require(v->verticalScrollBar()->value()>0&&v->horizontalScrollBar()->value()>0,"Far hit scrolls into view without replacing editing selection");
    query->setText("AZ");click(w,"motifFind");require(v->search_match()&&result->text().contains("IUPAC"),"Invalid query reports inline and preserves completed highlights");
    query->setText("AAA");click(w,"motifFind");
    for(int palette=0;palette<residue_palette_count;++palette)for(int style=0;style<3;++style){v->set_residue_palette(static_cast<ResiduePalette>(palette));v->set_residue_display(static_cast<ResidueDisplay>(style));v->set_identity_dots(true);QApplication::processEvents();expect(*v,1,0,3);
        const auto cue=v->viewport()->grab(QRect(v->name_area_width(),v->ruler_height(),3*v->cell_width(),v->row_height())).toImage();bool purple=false;
        for(int y=0;y<cue.height();++y)for(int x=0;x<cue.width();++x)purple|=cue.pixelColor(x,y)==QColor(126,44,181);
        require(purple,"Visible purple search cue in all display combinations");}
    require(w.current_document().state_id()==state&&!w.current_document().can_undo()&&w.current_document().sequence(1).trace->original==original,"Search does not edit history or immutable evidence");
    w.findChild<QPushButton*>("motifFind")->click();require(!v->isEnabled()&&panel->searching()&&w.findChild<QPushButton*>("motifCancel")->isEnabled(),"Cancellable job locks edits only while running");w.findChild<QPushButton*>("motifCancel")->click();wait([&]{return v->isEnabled();});expect(*v,1,0,3);require(result->text().contains("cancelled"),"Cancel discards result and retains previous hit");
    w.findChild<QPushButton*>("motifFind")->click();panel->set_document(std::make_shared<Document>(),{});wait([&]{return v->isEnabled();});require(!v->search_match()&&!panel->current_hit(),"Stale search result cannot highlight a replaced document");click(w,"motifFind");
    require(v->fragment()->ids==std::vector<SequenceId>{1}&&v->fragment()->start==0&&v->fragment()->width==3&&v->selected_ids().empty(),"Automatic hit selection sets residue selection");
    v->set_mode(AlignmentView::Mode::Rewrite);QKeyEvent edit(QEvent::KeyPress,Qt::Key_C,Qt::NoModifier,"C");QApplication::sendEvent(v,&edit);QApplication::processEvents();require(!v->search_match()&&!panel->current_hit()&&w.current_document().can_undo(),"Editing invalidates search cursor/cue");action(w,"&Undo")->trigger();require(w.current_document().state_id()==state&&!v->search_match(),"Undo restores data without resurrecting stale hit");
    query->setText("AAG");click(w,"motifFind");if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Navigator screenshot");dock->close();QApplication::processEvents();require(!v->search_match()&&!v->search_results(),"Closing navigator clears all search cues");
    w.close();std::cout<<"PASS 0.15: unwrapped actual FASTA clipboard/save; navigator overlaps/strands/wrap/scope/validation/cancel; automatic hit selection, name selection retention, display matrix, far navigation, invalidation/undo and original evidence\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
