#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/abif.hpp"
#include "core/session.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTableView>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>void wait(F done){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Async timeout");}
QAction* action(MainWindow& w,const QString& name){for(auto* a:w.findChildren<QAction*>())if(a->text()==name)return a;throw std::runtime_error("Missing menu action");}
void rename(MainWindow& w,const QString& name,const std::function<void(QDialog&)>& configure,bool apply=true){bool found=false;QTimer::singleShot(0,[&]{auto* q=qobject_cast<QDialog*>(QApplication::activeModalWidget());require(q&&q->objectName()=="batchRenameDialog","Batch rename dialog");found=true;configure(*q);if(apply){auto* button=q->findChild<QPushButton*>("renameApplyButton");require(button->isEnabled(),"Valid preview enables Apply");button->click();}else q->reject();});action(w,name)->trigger();require(found,"Dialog opened");}
void preview(QDialog& q){q.findChild<QPushButton*>("renamePreviewButton")->click();}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir dir;const auto path=dir.filePath("renaming.kleio");std::istringstream signal(test::synthetic_abif("ACGT"));Document d;d.load({read_abif(signal,"trace name","fixture.ab1"),{"second name","AC"},{"Жук👩‍🔬é","G"},{"duplicate","C"},{"duplicate","T"},{"duplicate_2","G"},{"mask name","12",RowKind::Mask}});d.replace(1,2,0,"~-");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,d,"{\"version\":1}");}
    MainWindow w;w.resize(1280,850);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});const auto original=w.current_document().sequence(1).trace;
    for(auto* a:w.findChildren<QAction*>())require(!a->text().contains("First non-gap"),"Retired selector is absent");
    for(auto* a:w.findChildren<QAction*>())require(a->text()!="Alphabetically","Alphabetical sequence sort is absent");
    auto* dots=w.findChild<QAction*>("identityDots");const auto icon=dots->icon().pixmap(40).toImage();bool yellow=false,blue=false,red=false,black=false;std::size_t bottomDots=0;for(int y=0;y<icon.height();++y)for(int x=0;x<icon.width();++x){const auto c=icon.pixelColor(x,y);if(c.alpha()<150)continue;if(y<icon.height()/2){yellow|=c.red()>c.green()+20&&c.green()>c.blue()+40;blue|=c.blue()>c.red()+40&&c.blue()>c.green()+40;red|=c.red()>c.blue()+40&&c.red()>c.green()+40;black|=c.red()<80&&c.green()<80&&c.blue()<80;}else if(c.red()<100&&c.green()<100&&c.blue()<100)++bottomDots;}require(yellow&&blue&&red&&black&&bottomDots,"ACGT colors above neutral dots");dots->trigger();
    v->select_ids({1});v->select_fragment({{1},1,1});const auto sanitizeState=w.current_document().state_id();
    rename(w,"Sanitize names",[&](QDialog& q){require(!q.findChild<QComboBox*>("renameScope")->currentData().toBool(),"Sanitize defaults to all rows despite selected names");preview(q);require(q.findChild<QTableView*>("renamePreview")->model()->rowCount()==7&&w.current_document().state_id()==sanitizeState,"All-row read-only sanitize preview");},false);require(w.current_document().state_id()==sanitizeState,"Sanitize Cancel leaves names intact");
    rename(w,"Sanitize names",[](QDialog& q){preview(q);});QSet<QString> safe;QRegularExpression allowed("^[A-Za-z0-9_]+$");for(const auto& row:w.current_document().rows()){const auto name=QString::fromUtf8(row.name.c_str());require(allowed.match(name).hasMatch()&&!safe.contains(name.toCaseFolded()),"Sanitize GUI produces ASCII-only unique names");safe.insert(name.toCaseFolded());}require(v->selected_ids()==std::vector<SequenceId>({1})&&v->fragment()->start==1&&w.current_document().sequence(1).trace==original,"Sanitize preserves selection and evidence");action(w,"&Undo")->trigger();require(w.current_document().sequence(1).name=="trace name"&&w.current_document().sequence(3).name==QString::fromUtf8("Жук👩‍🔬é").toUtf8().toStdString()&&!w.current_document().can_undo(),"Single sanitize undo restores original Unicode names");
    v->select_ids({1,7});v->select_fragment({{1},1,1});
    rename(w,"Spaces to underscores",[](QDialog& q){require(!q.findChild<QPushButton*>("renameApplyButton")->isEnabled(),"Apply initially disabled");preview(q);require(q.findChild<QTableView*>("renamePreview")->model()->rowCount()==2,"Selected preview scope");});
    require(w.current_document().sequence(1).name=="trace_name"&&w.current_document().sequence(7).name=="mask_name"&&w.current_document().sequence(2).name=="second name","Scoped batch apply");require(v->selected_ids()==std::vector<SequenceId>({1,7})&&v->fragment()->start==1,"Selections preserved");action(w,"&Undo")->trigger();require(w.current_document().sequence(1).name=="trace name"&&w.current_document().sequence(7).name=="mask name"&&!w.current_document().can_undo(),"Single undo restores batch");action(w,"&Redo")->trigger();action(w,"&Undo")->trigger();
    rename(w,"Spaces to underscores",[](QDialog& q){q.findChild<QComboBox*>("renameScope")->setCurrentIndex(1);preview(q);});require(w.current_document().sequence(2).name=="second_name","All-row scope");action(w,"&Undo")->trigger();
    const auto before=w.current_document().state_id();rename(w,"Append text…",[](QDialog& q){q.findChild<QLineEdit*>("renameText")->setText(" [new]");preview(q);},false);require(w.current_document().state_id()==before,"Cancel is read-only");
    rename(w,"Replace text…",[](QDialog& q){q.findChild<QLineEdit*>("renameText")->setText("trace name");preview(q);require(!q.findChild<QPushButton*>("renameApplyButton")->isEnabled()&&q.findChild<QLabel*>("renameStatus")->text().contains("nonblank"),"Invalid resulting name stays in preview");},false);
    rename(w,"Prepend text…",[](QDialog& q){auto* field=q.findChild<QLineEdit*>("renameText");field->setText("X_");preview(q);field->setText("Y_");require(!q.findChild<QPushButton*>("renameApplyButton")->isEnabled()&&q.findChild<QTableView*>("renamePreview")->model()->rowCount()==0,"Changed options invalidate preview");preview(q);});require(w.current_document().sequence(1).name=="Y_trace name","Only latest preview applied");action(w,"&Undo")->trigger();
    v->select_ids({4,5});rename(w,"Using old name / new name table…",[&](QDialog& q){q.findChild<QPlainTextEdit*>("renameTable")->setPlainText("duplicate\tTaxon\nmissing\tother");preview(q);const auto status=q.findChild<QLabel*>("renameStatus")->text();require(status.contains("missing")&&status.contains("multiple rows"),"Table reports missing/multiple matches");if(argc>2)require(q.grab().save(QString::fromLocal8Bit(argv[2])),"Dialog screenshot");});require(w.current_document().sequence(4).name=="Taxon"&&w.current_document().sequence(5).name=="Taxon","All duplicate source rows mapped");action(w,"&Undo")->trigger();
    v->select_all();rename(w,"Make names unique…",[](QDialog& q){preview(q);});require(w.current_document().sequence(4).name=="duplicate"&&w.current_document().sequence(5).name=="duplicate_3"&&w.current_document().sequence(6).name=="duplicate_2","Uniqueness avoids existing suffixes");action(w,"&Undo")->trigger();
    v->select_ids({1});rename(w,"Names from true lengths…",[](QDialog& q){preview(q);});require(w.current_document().sequence(1).name=="4","True length ignores both gap types");action(w,"&Undo")->trigger();rename(w,"Names from aligned lengths…",[](QDialog& q){preview(q);});require(w.current_document().sequence(1).name=="6","Aligned length retains gaps");action(w,"&Undo")->trigger();
    v->select_ids({3});rename(w,"Reverse names",[](QDialog& q){preview(q);});require(w.current_document().sequence(3).name==QString::fromUtf8("é👩‍🔬куЖ").toUtf8().toStdString(),"Unicode reversal in actual menu");action(w,"&Undo")->trigger();rename(w,"Append filename…",[](QDialog& q){require(q.findChild<QLineEdit*>("renameText")->text()=="renaming","Filename is prefilled and editable");preview(q);},false);
    v->select_ids({1,7});rename(w,"Prepend text…",[](QDialog& q){q.findChild<QLineEdit*>("renameText")->setText("sample_");preview(q);});require(w.current_document().sequence(1).trace==original&&w.current_document().sequence(7).kind==RowKind::Mask&&w.current_document().sequence(1).symbols.text()=="AC~-GT","Rename keeps signal/provenance/symbols/kinds");
    action(w,"&Save")->trigger();wait([&]{return v->isEnabled();});MainWindow restored;restored.show();restored.open_path(path);auto* rv=dynamic_cast<AlignmentView*>(restored.centralWidget());wait([&]{return rv->isEnabled();});require(restored.current_document().sequence(1).name=="sample_trace name"&&restored.current_document().sequence(7).name=="sample_mask name"&&restored.current_document().sequence(1).trace->original->channels==original->original->channels,"Native batch-name/evidence round trip");require(rv->identity_dots()&&rv->selected_ids()==std::vector<SequenceId>({1,7})&&rv->fragment()->start==1,"Views/selections retained");
    if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Window screenshot");restored.close();w.close();std::cout<<"PASS 0.13: selector removal, ACGT/dot icon, scoped/all batch previews, cancel/validation/invalidation, literal/Unicode/unique/length/table names, atomic undo/redo and native evidence/selection persistence\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
