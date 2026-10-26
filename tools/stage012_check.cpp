#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/sequence_selection.hpp"
#include "core/abif.hpp"
#include "core/session.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
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
QAction* action(MainWindow& w,const QString& name){for(auto* a:w.findChildren<QAction*>())if(a->text()==name)return a;throw std::runtime_error("Missing menu action");}
void dialog(MainWindow& w,const QString& label,const std::function<void(QDialog&)>& fill,bool accept=true){
    bool found=false;QTimer::singleShot(0,[&]{if(auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget())){found=true;fill(*d);if(accept)d->accept();else d->reject();}});
    action(w,label)->trigger();require(found,"Selection dialog appeared");auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});
}
void expect(AlignmentView& v,std::vector<SequenceId> ids){require(v.selected_ids()==ids,"Expected row selection");require(v.fragment()&&v.fragment()->ids==std::vector<SequenceId>({1})&&v.fragment()->start==2&&v.fragment()->width==2,"Selection tool preserves residue block");}
void title_helpers(){Document d;d.load({{"ЖУК","A"},{"жук","A"},{"Alpha (one)","C"},{"Alpha (two)","C"},{"Alpha (one)","G"},{"Beta|1","T"},{"Beta |2","T"}});
    require(select_titles(d,{"жук"},TitleMatch::Exact,false)==std::vector<SequenceId>({1,2}),"Unicode case-insensitive title match");require(select_titles(d,{"Alpha","Alpha"},TitleMatch::Prefix,true)==std::vector<SequenceId>({3,4,5}),"Prefix lists avoid repeated matches");require(select_titles(d,{"one"},TitleMatch::Contains,true)==std::vector<SequenceId>({3,5}),"Contains literal match");require(select_duplicate_titles(d,DuplicateTitle::Full,false,false)==std::vector<SequenceId>({1,2,3,5}),"Unicode duplicate grouping");require(select_duplicate_titles(d,DuplicateTitle::BeforeParenthesis,true,true)==std::vector<SequenceId>({4,5}),"Parenthesis grouping extras");require(select_duplicate_titles(d,DuplicateTitle::BeforePipe,true,false)==std::vector<SequenceId>({3,5,6,7}),"Pipe grouping trims separator whitespace");}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    title_helpers();QTemporaryDir dir;const auto path=dir.filePath("selection.kleio");std::istringstream evidence(test::synthetic_abif("AACGTA"));Document d;d.load({read_abif(evidence,"Alpha (one)","fixture.ab1"),{"Alpha (two)","A-AcGTA"},{"Beta|one","CTT"},{"Beta|two","~~AAG"},{"ЖУК","~~~TT"},{"жук","~~TT"},{"Mask","AACGTA",RowKind::Mask},{"Alpha (one)","G"},{"gap","~~~"}});{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,d,"{\"version\":1}");}
    MainWindow w;w.resize(1280,850);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});const auto state=w.current_document().state_id();
    auto* toolbar=w.findChild<QToolBar*>("editingToolbar");auto* dots=w.findChild<QAction*>("identityDots");auto* trace=w.findChild<QAction*>("chromatogramView");auto* overview=w.findChild<QAction*>("alignmentOverviewView");require(toolbar&&dots&&trace&&overview,"View actions exist");
    auto button=[&](QAction* a){auto* b=qobject_cast<QToolButton*>(toolbar->widgetForAction(a));require(b&&b->defaultAction()==a&&!b->icon().isNull()&&!b->toolTip().isEmpty()&&!b->accessibleName().isEmpty(),"Shared icon/tooltip/accessibility button");return b;};
    auto* db=button(dots);auto* tb=button(trace);auto* ob=button(overview);auto* traceDock=w.findChild<QDockWidget*>("chromatogramDock");auto* overviewDock=qobject_cast<QDockWidget*>(w.findChild<QWidget*>("alignmentOverview")->parentWidget());
    require(db->x()>w.findChild<QComboBox*>("rightClickMode")->geometry().right(),"Buttons follow the dropdowns");require(db->icon().pixmap(20).toImage()!=tb->icon().pixmap(20).toImage()&&tb->icon().pixmap(20).toImage()!=ob->icon().pixmap(20).toImage(),"Distinct intuitive icons");
    db->click();require(dots->isChecked()&&db->isChecked()&&v->identity_dots(),"Toolbar enables identity view");dots->trigger();require(!db->isChecked()&&!v->identity_dots(),"Menu disables identity view");db->click();
    tb->click();QApplication::processEvents();require(!traceDock->isVisible()&&!trace->isChecked()&&!tb->isChecked(),"Toolbar hides trace");trace->trigger();QApplication::processEvents();require(traceDock->isVisible()&&tb->isChecked(),"Menu shows trace");traceDock->close();QApplication::processEvents();require(!tb->isChecked(),"Dock close updates toolbar");tb->click();
    ob->click();QApplication::processEvents();require(!overviewDock->isVisible()&&!ob->isChecked(),"Toolbar hides overview");overview->trigger();QApplication::processEvents();require(overviewDock->isVisible()&&ob->isChecked(),"Menu shows overview");
    v->select_fragment({{1},2,2});
    dialog(w,"Names starting with…",[](QDialog& q){q.findChild<QPlainTextEdit*>("selectionPatterns")->setPlainText("Alpha");require(q.findChildren<QCheckBox*>().size()==1,"Only relevant title options visible");});expect(*v,{1,2,8});
    dialog(w,"Names containing…",[](QDialog& q){q.findChild<QPlainTextEdit*>("selectionPatterns")->setPlainText("one");});expect(*v,{1,3,8});
    dialog(w,"Names from exact list…",[](QDialog& q){q.findChild<QPlainTextEdit*>("selectionPatterns")->setPlainText("Alpha (one)\nжук\n");q.findChild<QCheckBox*>("selectionMatchCase")->setChecked(false);});expect(*v,{1,5,6,8});
    dialog(w,"Names starting with…",[](QDialog& q){q.findChild<QPlainTextEdit*>("selectionPatterns")->setPlainText("Beta");},false);expect(*v,{1,5,6,8});
    dialog(w,"Duplicate names…",[](QDialog&){});expect(*v,{1,8});
    dialog(w,"Duplicate names…",[](QDialog& q){q.findChild<QComboBox*>("selectionMethod")->setCurrentIndex(1);q.findChild<QCheckBox*>("selectionAdditionalOnly")->setChecked(true);});expect(*v,{2,8});
    dialog(w,"Duplicate names…",[](QDialog& q){q.findChild<QComboBox*>("selectionMethod")->setCurrentIndex(2);});expect(*v,{1,3,4,8});
    dialog(w,"Duplicate sequences…",[](QDialog&){});expect(*v,{1,2,5,6});
    dialog(w,"Duplicate sequences…",[](QDialog& q){q.findChild<QCheckBox*>("selectionAdditionalOnly")->setChecked(true);});expect(*v,{2,6});
    dialog(w,"Sequences containing a motif…",[](QDialog& q){q.findChild<QLineEdit*>("selectionMotif")->setText("AAG");q.findChild<QCheckBox*>("selectionReverseComplement")->setChecked(true);});expect(*v,{3,4});
    dialog(w,"Sequences containing a motif…",[](QDialog& q){q.findChild<QLineEdit*>("selectionMotif")->setText("R");q.findChild<QCheckBox*>("selectionCompatibleIupac")->setChecked(true);});expect(*v,{1,2,4,8});
    dialog(w,"Random subset…",[](QDialog& q){auto* input=qobject_cast<QInputDialog*>(&q);require(input,"Random count dialog");input->setIntValue(3);});require(v->selected_ids().size()==3&&v->fragment()->width==2,"Random selection exact count");
    dialog(w,"Names from exact list…",[](QDialog& q){q.findChild<QPlainTextEdit*>("selectionPatterns")->setPlainText("not in this file");});expect(*v,{});
    require(w.current_document().state_id()==state&&!w.current_document().can_undo()&&w.current_document().sequence(1).trace->original->channels==d.sequence(1).trace->original->channels,"Selection/view settings never edit data or evidence");
    v->select_ids({3,4});tb->click();ob->click();action(w,"&Save")->trigger();wait([&]{return v->isEnabled();});
    MainWindow restored;restored.show();restored.open_path(path);auto* rv=dynamic_cast<AlignmentView*>(restored.centralWidget());wait([&]{return rv->isEnabled();});auto* rt=restored.findChild<QToolBar*>("editingToolbar");for(const auto& name:{"identityDots","chromatogramView","alignmentOverviewView"}){auto* a=restored.findChild<QAction*>(name);auto* b=qobject_cast<QToolButton*>(rt->widgetForAction(a));require(a->isChecked()==b->isChecked(),"Session restores menu/button synchronization");}
    require(rv->identity_dots()&&!restored.findChild<QAction*>("chromatogramView")->isChecked()&&!restored.findChild<QAction*>("alignmentOverviewView")->isChecked(),"Session restores all view choices");expect(*rv,{3,4});
    MainWindow empty;empty.show();require(!empty.findChild<QAction*>("chromatogramView")->isEnabled(),"Unavailable trace is disabled");empty.close();
    if(argc>1){require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Window screenshot");}
    restored.close();w.close();std::cout<<"PASS selection/toolbar: menu/dock/native view synchronization and icons/tooltips/accessibility; title/Unicode/duplicate/motif/random selection dialogs; cancel/zero matches/independent residues/unchanged data\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
