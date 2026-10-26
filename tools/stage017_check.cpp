#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QWheelEvent>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
template<class F>void wait(F done){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Async timeout");QApplication::processEvents();}
QAction* action(MainWindow& w,const QString& name){for(auto* a:w.findChildren<QAction*>())if(a->objectName()==name||a->text()==name)return a;throw std::runtime_error("Missing action: "+name.toStdString());}
template<class F,class Trigger>void modal(F callback,Trigger trigger){QString failure;QObject context;bool called=false;QTimer::singleShot(0,&context,[&]{called=true;auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());try{require(d,"Expected dialog");callback(d);}catch(const std::exception& e){failure=e.what();if(d)d->reject();}});trigger();require(called,"Expected modal dialog did not open");if(!failure.isEmpty())throw std::runtime_error(failure.toStdString());}
template<class F>void dialog(MainWindow& w,const QString& name,F f){try{modal(f,[&]{require(action(w,name)->isEnabled(),"Action disabled");action(w,name)->trigger();});}catch(const std::exception& e){throw std::runtime_error(name.toStdString()+": "+e.what());}}
void accept(QDialog* d,QDialogButtonBox::StandardButton b=QDialogButtonBox::Ok){auto* box=d->findChild<QDialogButtonBox*>();require(box&&box->button(b)&&box->button(b)->isEnabled(),"Enabled dialog button");box->button(b)->click();}
void save(MainWindow& w,const QString& path){dialog(w,"Save &as…",[&](QDialog* d){auto* file=qobject_cast<QFileDialog*>(d);require(file,"Native save filename dialog");file->selectFile(path);QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);});auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{QTemporaryDir dir;std::istringstream original(test::synthetic_abif());Document source;source.load({{"A","AACCGGTT"},{"protein","MEF*GG",RowKind::Protein},{"mask","120034",RowKind::Mask},read_abif(original,"trace","gone.ab1"),{"dup","ACGT"},{"dup","ACGT"}});source.apply_groups({{1,"Alps",0x0072b2},{2,"Caucasus",0xe69f00}},{{1,1},{2,2},{4,1}});
    const auto path=dir.filePath("groups.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,source);}MainWindow w;w.resize(1280,850);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});
    require(action(w,"cutFragment")->shortcut()==QKeySequence::Cut&&action(w,"pasteFragment")->shortcut()==QKeySequence::Paste,"Standard Ctrl+X/Ctrl+V shortcuts");v->select_ids({3});
    dialog(w,"assignGroup",[&](QDialog* d){require(d->objectName()=="assignGroupDialog","Assign group dialog");auto* choice=d->findChild<QComboBox*>("assignGroupChoice");choice->setCurrentIndex(choice->count()-1);d->findChild<QLineEdit*>("assignGroupName")->setText("Tatra");accept(d);});require(w.current_document().sequence(3).groupId==3,"Creating and assigning group is one operation");
    dialog(w,"manageGroups",[&](QDialog* d){auto* list=d->findChild<QListWidget*>("groupList");list->setCurrentRow(3);
        modal([&](QDialog* nested){auto* input=qobject_cast<QInputDialog*>(nested);require(input,"Rename group input");input->setTextValue("Tatras");input->accept();},[&]{d->findChild<QPushButton*>("groupRename")->click();});
        modal([&](QDialog* nested){auto* color=qobject_cast<QColorDialog*>(nested);require(color,"Group color chooser");color->setCurrentColor(QColor(0x22,0x44,0x66));color->accept();},[&]{d->findChild<QPushButton*>("groupColor")->click();});
        auto* rows=d->findChild<QListWidget*>("groupMembers");rows->item(0)->setCheckState(Qt::Checked);rows->item(1)->setCheckState(Qt::Checked);d->findChild<QPushButton*>("groupUp")->click();if(argc>2)require(d->grab().save(QString::fromLocal8Bit(argv[2])),"Group manager screenshot");accept(d,QDialogButtonBox::Apply);
    });require(w.current_document().group(3)->name=="Tatras"&&w.current_document().group(3)->color==0x224466&&w.current_document().sequence(1).groupId==3&&w.current_document().groups()[1].id==3,"Manager stages names/colors/members/order atomically");
    v->select_ids({});QTimer::singleShot(0,[&]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());require(menu,"Name context menu");bool found=false;for(auto* a:menu->actions())if(a->text()=="Remove from group"){require(a->isEnabled(),"Context row selected automatically");a->trigger();found=true;break;}require(found,"Remove from group context action");menu->close();});
    QContextMenuEvent context(QContextMenuEvent::Mouse,QPoint(30,v->ruler_height()+3*v->row_height()+v->row_height()/2),QPoint(30,200));QApplication::sendEvent(v->viewport(),&context);require(!w.current_document().sequence(4).groupId&&v->selected_ids()==std::vector<SequenceId>{4},"Right-click group action applies to clicked unselected name");action(w,"&Undo")->trigger();
    const auto grouped=w.current_document().state_id();dialog(w,"manageGroups",[&](QDialog* d){d->findChild<QListWidget*>("groupMembers")->item(0)->setCheckState(Qt::Unchecked);d->reject();});require(w.current_document().state_id()==grouped,"Manager Cancel discards staged edits");
    action(w,"By group")->trigger();wait([&]{return v->isEnabled();});std::vector<SequenceId> order;for(const auto& row:w.current_document().rows())order.push_back(row.id);require(order==std::vector<SequenceId>({4,1,2,3,5,6}),"Stable manager order, ungrouped last");action(w,"&Undo")->trigger();v->select_fragment({{1},0,2});
    dialog(w,"selectGroups",[&](QDialog* d){auto* list=d->findChild<QListWidget*>("selectGroupsList");list->item(0)->setCheckState(Qt::Checked);list->item(2)->setCheckState(Qt::Checked);accept(d);});require(v->selected_ids()==std::vector<SequenceId>({1,2,3,5,6})&&v->fragment()->width==2,"Group selection preserves independent residue selection");
    auto pixel=v->viewport()->grab().toImage();require(pixel.pixelColor(2,v->ruler_height()+v->row_height()/2)==QColor(0x22,0x44,0x66),"Group swatch remains visible over selected names");
    dialog(w,"importGroups",[&](QDialog* d){d->findChild<QPlainTextEdit*>("groupImportText")->setPlainText("Sequence_name\tGroup_name\nA\tNewGroup\nunknown\tSkip\ndup\tDuplicates\n");d->findChild<QCheckBox*>("groupImportHeader")->setChecked(true);require(!d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->isEnabled(),"Duplicate titles block Apply by default");d->findChild<QCheckBox*>("groupImportDuplicates")->setChecked(true);require(d->findChild<QLabel*>("groupImportSummary")->text().contains("1 unmatched"),"Unmatched titles are reported");accept(d,QDialogButtonBox::Apply);});require(w.current_document().sequence(5).groupId==w.current_document().sequence(6).groupId&&w.current_document().groups().size()==5,"TSV import handles explicit duplicate policy");action(w,"&Undo")->trigger();require(w.current_document().groups().size()==3,"TSV one-step undo");action(w,"&Redo")->trigger();
    const auto beforeConflict=w.current_document().state_id();dialog(w,"importGroups",[&](QDialog* d){d->findChild<QPlainTextEdit*>("groupImportText")->setPlainText("dup\tX\ndup\tY\n");d->findChild<QCheckBox*>("groupImportDuplicates")->setChecked(true);require(!d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->isEnabled(),"Conflicting assignments cannot apply");d->reject();});require(w.current_document().state_id()==beforeConflict,"Invalid import never mutates alignment");
    const auto trace=w.current_document().sequence(4).trace;v->select_fragment({{1,2},1,2});v->select_ids({2,4});action(w,"cutSequences")->trigger();require(w.current_document().rows().size()==4&&QApplication::clipboard()->mimeData()->hasFormat("application/x-kleiotide-sequences"),"Sequence Cut publishes typed payload then removes selected rows");
    Document target;target.load({{"anchor","A"},{"tail","12",RowKind::Mask}});target.apply_groups({{1,"Tatras",0xff0000}},{{1,1}});const auto targetPath=dir.filePath("target.kleio");{std::ofstream out(targetPath.toStdString(),std::ios::binary);write_session(out,target);}MainWindow other;other.show();other.open_path(targetPath);auto* ov=dynamic_cast<AlignmentView*>(other.centralWidget());wait([&]{return ov->isEnabled();});ov->select_fragment({{1,2},0,1});ov->select_ids({1});action(other,"pasteSequences")->trigger();
    require(other.current_document().rows().size()==4&&other.current_document().rows()[1].name=="protein"&&other.current_document().rows()[2].trace&&other.current_document().rows()[3].name=="tail","Paste inserts after selected sequence names, retaining order/types/traces");require(other.current_document().rows()[2].trace->original->channels==trace->original->channels&&other.current_document().group(other.current_document().rows()[1].groupId)->name=="Tatras (2)","Cross-window paste preserves signals and resolves color/name collision");action(other,"&Undo")->trigger();require(other.current_document().rows().size()==2&&other.current_document().groups().size()==1,"One paste undo removes inserted rows and groups");action(other,"&Redo")->trigger();save(other,dir.filePath("target_saved.kleio"));other.close();action(w,"&Undo")->trigger();require(w.current_document().sequence(4).trace==trace&&w.current_document().sequence(2).kind==RowKind::Protein,"Cut undo restores original trace object/types/memberships");
    v->setFocus();v->select_ids({});v->set_mode(AlignmentView::Mode::Insert);v->select_fragment({{1},2,2});action(w,"cutFragment")->trigger();require(w.current_document().sequence(1).symbols.text()=="AAGGTT"&&QApplication::clipboard()->text()=="CC","Single substring Cut");action(w,"pasteFragment")->trigger();require(w.current_document().sequence(1).symbols.text()=="AACCGGTT","Insert paste returns to deletion boundary");action(w,"&Undo")->trigger();action(w,"&Undo")->trigger();
    v->set_mode(AlignmentView::Mode::Rewrite);v->select_fragment({{1},2,2});action(w,"cutFragment")->trigger();v->select_fragment({{1},1,1});action(w,"pasteFragment")->trigger();require(w.current_document().sequence(1).symbols.text()=="ACCGGTT","Overwrite paste substitutes selected span");action(w,"&Undo")->trigger();action(w,"&Undo")->trigger();
    for(auto mode:{AlignmentView::Mode::Insert,AlignmentView::Mode::Rewrite}){
        v->set_mode(mode);v->select_fragment({{1,2},1,2});QApplication::clipboard()->setText("clipboard sentinel");const auto before=w.current_document().state_id();
        action(w,"cutFragment")->trigger();require(w.current_document().state_id()==before&&QApplication::clipboard()->text()=="clipboard sentinel","Block Cut changes neither document nor clipboard");
        QApplication::clipboard()->setText("AC");action(w,"pasteFragment")->trigger();require(w.current_document().state_id()==before&&v->fragment()->ids.size()==2,"Block raw Paste leaves document and selection unchanged");
        require(!v->paste_residue_rows({"AC","EF"}),"View rejects multirow clipboard data");
        auto* payload=new QMimeData;payload->setText("AC\nEF");payload->setData("application/x-kleiotide-residues",R"({"version":1,"rows":["AC","EF"]})");QApplication::clipboard()->setMimeData(payload);
        action(w,"pasteFragment")->trigger();v->select_fragment({{1},1,1});action(w,"pasteFragment")->trigger();require(w.current_document().state_id()==before,"Legacy multirow payload cannot paste into block or single row");
    }
    v->set_mode(AlignmentView::Mode::View);v->select_fragment({{1},1,1});const auto state=w.current_document().state_id();const auto clipboard=QApplication::clipboard()->text();action(w,"cutFragment")->trigger();require(w.current_document().state_id()==state&&QApplication::clipboard()->text()==clipboard,"Residue Cut is inactive outside Insert/Overwrite");
    const std::pair<const char*,std::vector<SequenceId>> selections[]={{"selectNucleotide",{1,4,5,6}},{"selectProtein",{2}},{"selectMasks",{3}},{"selectTraces",{4}}};
    for(const auto& [label,ids]:selections){action(w,label)->trigger();require(v->selected_ids()==ids&&v->fragment()->ids==std::vector<SequenceId>{1},"Type and trace selectors preserve residue selection");}
    for(auto* a:w.findChildren<QAction*>())require(a->text()!="Group by identical sequences","Obsolete identical sort removed");
    const auto oldGroups=w.current_document().groups();dialog(w,"groupIdentical",[&](QDialog* d){accept(d,QDialogButtonBox::Apply);});require(w.current_document().groups().size()==1&&w.current_document().sequence(5).groupId==w.current_document().sequence(6).groupId&&!w.current_document().sequence(1).groupId,"Group identical action replaces old memberships");action(w,"&Undo")->trigger();require(w.current_document().groups()==oldGroups,"One undo restores previous group catalog");
    v->select_ids({1});QApplication::processEvents();pixel=v->viewport()->grab().toImage();require(pixel.pixelColor(9,v->ruler_height()+v->row_height()/2)==QColor::fromRgb(w.current_document().group(w.current_document().sequence(1).groupId)->color),"Group swatch is ten pixels wide");
    Document large;std::vector<Record> records;for(int i=0;i<100;++i)records.push_back({std::to_string(i),std::string(2000,'A')});large.load(std::move(records));AlignmentView scroll;scroll.resize(700,300);scroll.set_document(&large);scroll.show();QApplication::processEvents();scroll.horizontalScrollBar()->setValue(50);scroll.verticalScrollBar()->setValue(20);
    auto wheel=[&](QPoint angle,QPoint pixels,Qt::KeyboardModifiers modifiers){QWheelEvent event(QPointF(350,100),QPointF(350,100),pixels,angle,Qt::NoButton,modifiers,Qt::NoScrollPhase,false);QApplication::sendEvent(scroll.viewport(),&event);};wheel({0,-120},{},Qt::ControlModifier);require(scroll.horizontalScrollBar()->value()==56&&scroll.verticalScrollBar()->value()==20,"Ctrl+wheel now moves six columns per notch");wheel({0,-120},{},Qt::NoModifier);require(scroll.verticalScrollBar()->value()==23,"Normal wheel remains three rows");wheel({-120,0},{},Qt::NoModifier);require(scroll.horizontalScrollBar()->value()==59,"Native horizontal wheel unchanged");wheel({},QPoint(0,-1),Qt::ControlModifier);require(scroll.horizontalScrollBar()->value()==61,"Pixel Ctrl+wheel also doubles");scroll.close();
    v->select_ids({1,2,3});if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Grouped alignment screenshot");save(w,dir.filePath("source_saved.kleio"));w.close();std::cout<<"PASS 0.17: group manager/colors/members/order, TSV preview/errors/undo, group selection/swatch, typed sequence cut/paste across windows with traces, single-row cut/paste and block rejection, type/trace selection, identical grouping/undo, mode guards, doubled Ctrl+wheel\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
