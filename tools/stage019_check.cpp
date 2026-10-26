#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/site_highlighter.hpp"
#include "core/session.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QTableView>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
template<class F>void wait(F f){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(f(),"Async timeout");QApplication::processEvents();}
QAction* action(MainWindow& w,const QString& id){for(auto* a:w.findChildren<QAction*>())if(a->objectName()==id||a->text().section('\t',0,0)==id)return a;throw std::runtime_error("Missing action: "+id.toStdString());}
template<class F>void modal(MainWindow& w,const QString& id,F inspect){bool called=false;QString error;QObject context;QTimer::singleShot(0,&context,[&]{called=true;auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());try{require(d,"Expected modal");inspect(d);}catch(const std::exception& e){error=e.what();if(d)d->reject();}});action(w,id)->trigger();require(called,"Modal inspected");if(!error.isEmpty())throw std::runtime_error(id.toStdString()+": "+error.toStdString());}
void accept(QDialog* d,QDialogButtonBox::StandardButton button=QDialogButtonBox::Ok){d->findChild<QDialogButtonBox*>()->button(button)->click();}
void ready(AlignmentView* v){wait([&]{return bool(v->site_highlights());});}
void flags(AlignmentView* v,std::initializer_list<std::size_t> cols){ready(v);auto bits=*v->site_highlights();for(std::size_t c=0;c<bits.size();++c)require(bool(bits[c])==(std::find(cols.begin(),cols.end(),c)!=cols.end()),"Wrong GUI highlights");}
void save(MainWindow& w,const QString& path){modal(w,"Save &as…",[&](QDialog* d){auto* f=qobject_cast<QFileDialog*>(d);require(f,"Save chooser");f->selectFile(path);QMetaObject::invokeMethod(f,"accept",Qt::DirectConnection);});wait([&]{return w.centralWidget()->isEnabled();});}
void calculate(QDialog* d){auto* b=d->findChild<QPushButton*>("compositionCalculate");b->click();wait([&]{return b->isEnabled();});require(d->findChild<QPushButton*>("compositionCopy")->isEnabled(),"Composition result ready");}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{
    QTemporaryDir temp;QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());
    std::vector<Record> rows;const std::vector<std::string> columns={"AAAA","AACC","AAAC","RARA","~-~-","N?N~","RYRY","AN~~","AG~~","UTTT"};for(int r=0;r<4;++r){Record row{"row"+std::to_string(r+1),{}};for(const auto& c:columns)row.symbols+=c[static_cast<std::size_t>(r)];rows.push_back(row);}rows.push_back({"mask","111111111111111",RowKind::Mask});rows.push_back({"protein1","DNXXUO",RowKind::Protein});rows.push_back({"protein2","BDXXUO",RowKind::Protein});Document input;input.load(rows);input.apply_groups({{1,"Group",0x0072b2}},{{1,1},{2,1}});const auto path=temp.filePath("sites.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,input);}
    MainWindow w;w.resize(1280,820);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return v->isEnabled();});const auto baseline=w.current_document().state_id();
    require(w.findChild<QMenu*>("highlightMenu")&&w.findChild<QMenu*>("statisticsMenu"),"New menus");require(action(w,"alignmentStatisticsAction")->associatedObjects().contains(w.findChild<QMenu*>("statisticsMenu")),"Summary moved to Statistics");
    action(w,"highlightConserved")->trigger();flags(v,{0,3,7,9});action(w,"highlightVariable")->trigger();flags(v,{0,1,2,3,6,7,8,9});action(w,"highlightClear")->trigger();require(!v->site_highlights()&&!action(w,"highlightVariable")->isChecked(),"Clear synchronizes rules");
    modal(w,"highlightConservation",[&](QDialog* d){d->findChild<QDoubleSpinBox*>("highlightMinimum")->setValue(100);accept(d);});flags(v,{0,7,9});
    modal(w,"highlightConservation",[&](QDialog* d){d->reject();});require(action(w,"highlightConservation")->isChecked(),"Cancel retains enabled rule");
    modal(w,"highlightConservation",[&](QDialog* d){d->findChild<QCheckBox*>("highlightThresholdEnabled")->setChecked(false);accept(d);});require(!v->site_highlights(),"Threshold can be disabled");
    modal(w,"highlightCoverage",[&](QDialog* d){d->findChild<QDoubleSpinBox*>("highlightMinimum")->setValue(0);d->findChild<QDoubleSpinBox*>("highlightMaximum")->setValue(0);d->findChild<QComboBox*>("highlightCoverageBasis")->setCurrentIndex(1);accept(d);});flags(v,{4,5,10,11,12,13,14});action(w,"highlightClear")->trigger();
    v->select_ids({1,4,6});v->select_fragment({{1,4},1,3});modal(w,"highlightScope",[&](QDialog* d){d->findChild<QComboBox*>("highlightRows")->setCurrentIndex(1);d->findChild<QComboBox*>("highlightColumns")->setCurrentIndex(1);accept(d,QDialogButtonBox::Apply);});action(w,"highlightVariable")->trigger();flags(v,{1,2});v->select_ids({2});flags(v,{1,2});
    v->select_fragment({{4},1,1});require(v->paste_residues("A"),"Edit relevant state");ready(v);flags(v,{2});action(w,"&Undo")->trigger();flags(v,{1,2});require(w.current_document().state_id()==baseline,"Analyses do not create undo steps");
    // Native persistence records the explicit snapshot, rule ranges and custom color.
    modal(w,"highlightColor",[&](QDialog* d){auto* color=qobject_cast<QColorDialog*>(d);require(color,"Color chooser");color->setCurrentColor(QColor("#b786d9"));color->accept();});require(v->site_highlight_color()==QColor("#b786d9"),"Chosen highlight color");
    const auto saved=temp.filePath("highlight_saved.kleio");save(w,saved);MainWindow reopened;reopened.show();reopened.open_path(saved);auto* rv=dynamic_cast<AlignmentView*>(reopened.centralWidget());wait([&]{return rv->isEnabled();});flags(rv,{1,2});require(rv->site_highlight_color()==QColor("#b786d9")&&action(reopened,"highlightVariable")->isChecked()&&reopened.current_document().groups()==input.groups(),"Native scope/color/rules/group roundtrip");reopened.close();
    // All display styles retain visible sites and byte-identical selection cells.
    modal(w,"highlightScope",[&](QDialog* d){d->findChild<QComboBox*>("highlightRows")->setCurrentIndex(0);d->findChild<QComboBox*>("highlightColumns")->setCurrentIndex(0);accept(d,QDialogButtonBox::Apply);});ready(v);v->select_fragment({{1},1,2});
    for(int palette=0;palette<residue_palette_count;++palette)for(auto style:{ResidueDisplay::Letters,ResidueDisplay::Monochrome,ResidueDisplay::Backgrounds}){v->set_residue_palette(static_cast<ResiduePalette>(palette));v->set_residue_display(style);v->set_identity_dots(true);action(w,"highlightClear")->trigger();auto before=v->viewport()->grab().toImage();action(w,"highlightVariable")->trigger();ready(v);auto after=v->viewport()->grab().toImage();const auto dpr=after.devicePixelRatio();const QRect selection(qRound((v->name_area_width()+v->cell_width())*dpr),qRound(v->ruler_height()*dpr),qRound(2*v->cell_width()*dpr),qRound(v->row_height()*dpr));require(before.copy(selection)==after.copy(selection),"Selection has visual precedence");const QRect site(selection.x(),qRound((v->ruler_height()+2*v->row_height())*dpr),selection.width(),selection.height());require(before.copy(site)!=after.copy(site),"Unselected highlights remain visible in each style");}
    if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Highlight screenshot");
    action(w,"alignmentStatisticsAction")->trigger();auto* stats=w.findChild<QDialog*>("alignmentStatistics");require(stats,"Summary opened");auto* statsCopy=stats->findChild<QPushButton*>("statisticsCopy");wait([&]{return statsCopy->isEnabled();});require(stats->findChild<QLabel*>("statisticsValue0")->text()=="4","Summary excludes proteins/mask");statsCopy->click();require(QApplication::clipboard()->text().contains("Constant sites: 6"),"Existing single-string Copy retained");stats->reject();QApplication::processEvents();
    v->select_ids({1,4,6});v->select_fragment({{1},1,3});
    modal(w,"nucleotideComposition",[&](QDialog* d){require(!v->isEnabled(),"Source held immutable");calculate(d);auto* table=d->findChild<QTableView*>("compositionTable");require(table->model()->rowCount()==5&&table->model()->columnCount()==17,"All nucleotide rows plus pooled row");d->findChild<QPushButton*>("compositionCopy")->click();const auto copied=QApplication::clipboard()->text();require(copied.contains("fractional equal votes")&&copied.contains("Pooled\t"),"Copy full TSV with methods");auto* rowScope=d->findChild<QComboBox*>("compositionRows");rowScope->setCurrentIndex(1);require(!d->findChild<QPushButton*>("compositionCopy")->isEnabled()&&table->model()->rowCount()==0,"Changed settings invalidate previous result");d->findChild<QComboBox*>("compositionColumns")->setCurrentIndex(1);calculate(d);require(table->model()->rowCount()==3&&table->model()->data(table->model()->index(0,10)).toString()=="3.000","Composition uses explicit row/column snapshot");d->findChild<QCheckBox*>("compositionFractional")->setChecked(false);calculate(d);require(table->model()->data(table->model()->index(0,10)).toString()=="2.000","Definite-only option");if(argc>2)require(d->grab().save(QString::fromLocal8Bit(argv[2])),"Composition screenshot");
        const auto exportPath=temp.filePath("composition.tsv");bool chooser=false;QTimer::singleShot(0,d,[&]{auto* file=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());if(file){chooser=true;file->selectFile(exportPath);QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);}});d->findChild<QPushButton*>("compositionExport")->click();require(chooser,"TSV export chooser");std::ifstream exported(exportPath.toStdString(),std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(exported)),{});d->findChild<QPushButton*>("compositionCopy")->click();require(QString::fromUtf8(bytes.c_str())==QApplication::clipboard()->text(),"Copy/export identical UTF-8 TSV");d->reject();});
    modal(w,"nucleotideComposition",[&](QDialog* d){d->findChild<QPushButton*>("compositionCalculate")->click();d->reject();wait([&]{return !d->isVisible();});});require(v->isEnabled()&&w.current_document().state_id()==baseline,"Cancellation waits safely without modifying document");w.close();
    std::cout<<"PASS 0.19: Highlight union/ranges/scope, invalidation/undo, color and native restore, visible style cues and selection precedence, Statistics menu, composition scope/ambiguity/copy/export/cancel\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
