#include "core/abif.hpp"
#include "core/fasta.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "gui/main_window.hpp"
#include "gui/mafft_run.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QClipboard>
#include <QKeyEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QUrl>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void wait(F done){QElapsedTimer timer;timer.start();while(!done()&&timer.elapsed()<30000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Timed out");}
QAction* action(MainWindow& window,const QString& title){for(auto* a:window.findChildren<QAction*>())if(a->text()==title)return a;throw std::runtime_error("Missing menu action");}
void canvas_check(const QString& screenshot){
    auto bytes=test::synthetic_abif(std::string(100,'A')+"CGTRN");std::istringstream stream(bytes);Document d;d.load({read_abif(stream,"synthetic","synthetic.ab1"),{"other","ACGT"}});
    ChromatogramPanel panel;panel.resize(1050,300);panel.set_document(&d);panel.show();QApplication::processEvents();
    auto* canvas=panel.canvas();require(panel.has_traces()&&canvas->sequence_id()==1,"Trace available");
    panel.follow_selection({1},80);auto range=canvas->visible_samples();require(range.first<1630&&range.second>1630,"Alignment selects trace anchor");
    auto* zoom=panel.findChild<QSpinBox*>("traceZoom");zoom->setValue(200);range=canvas->visible_samples();require(range.first<1630&&range.second>1630,"Zoom preserves center");
    zoom->setValue(25);require(!canvas->grab().isNull(),"Decimated envelope render");zoom->setValue(100);
    canvas->focus_column(20);SequenceId clicked=0;std::size_t column=0;panel.cell_requested=[&](SequenceId id,std::size_t c){clicked=id;column=c;};
    const int px=430+80-canvas->horizontalScrollBar()->value();QMouseEvent click(QEvent::MouseButtonPress,QPointF(px,90),QPointF(px,90),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(canvas->viewport(),&click);require(clicked==1&&column==20,"Signal peak selects alignment column");
    d.replace(1,20,1,"T");panel.set_document(&d);panel.follow_selection({1},20);require(panel.findChild<QLabel*>("tracePosition")->text().contains("substituted"),"Edited evidence label");
    d.replace(1,21,0,"CC");panel.set_document(&d);panel.follow_selection({1},21);require(panel.findChild<QLabel*>("tracePosition")->text().contains("no original peak"),"Synthetic anchor label");
    d.transform({1},Document::Transform::ReverseComplement);panel.set_document(&d);panel.follow_selection({1},0);require(panel.findChild<QLabel*>("traceSource")->text().contains("reverse complement"),"Reverse strand view");
    d.undo();d.undo();d.undo();panel.set_document(&d);require(canvas->sequence_id()==1,"Trace undo refresh");
    d.replace(1,20,1,"");panel.set_document(&d);panel.follow_selection({1},20);canvas->step_call(-1);require(panel.findChild<QLabel*>("tracePosition")->text().contains("deleted"),"Deleted signal remains navigable");
    d.undo();panel.set_document(&d);panel.follow_selection({1},25);if(!screenshot.isEmpty())require(panel.grab().save(screenshot),"Save trace screenshot");
    d.erase_rows({1});panel.set_document(&d);require(!panel.has_traces()&&canvas->sequence_id()==0,"Detached/deleted row clears viewer");d.undo();panel.set_document(&d);require(panel.has_traces(),"Restore linked row");
}
void window_check(const QString& first,const QString& second,const QString& screenshot){
    MainWindow window;window.resize(1180,820);window.show();window.open_path(first);auto* view=dynamic_cast<AlignmentView*>(window.centralWidget());wait([&]{return view->isEnabled();});
    ChromatogramPanel* panel=nullptr;for(auto* widget:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ChromatogramPanel*>(widget))panel=candidate;
    require(panel&&panel->has_traces(),"AB1 open creates linked sequence");auto* dock=window.findChild<QDockWidget*>("chromatogramDock");require(dock&&dock->isVisible(),"Automatic trace dock");
    view->select_all();const auto ids=view->selected_ids();require(ids.size()==1,"One sequence per trace");view->unselect_all();view->select_fragment({{ids[0]},30,1});require(panel->canvas()->focused_column()==30,"Window alignment-to-trace selection");
    panel->canvas()->step_call(1);require(view->fragment()&&view->fragment()->start==31,"Window trace-to-alignment navigation");
    view->navigate_to(300);auto range=panel->canvas()->visible_samples();require(range.second>range.first,"Overview/alignment scrolling follows trace");
    action(window,"Chromatogram")->trigger();require(!dock->isVisible(),"View hides trace");view->select_fragment({{ids[0]},40,1});require(!dock->isVisible(),"Hidden pane stays hidden on selection");action(window,"Chromatogram")->trigger();require(dock->isVisible(),"View restores trace");
    if(!screenshot.isEmpty()){view->navigate_to_cell(ids[0],200);bool settled=false;QTimer::singleShot(250,&window,[&]{settled=true;});wait([&]{return settled;});require(window.grab().save(screenshot),"Save window screenshot");}
    const auto sourceBytes=[&]{QFile file(first);require(file.open(QIODevice::ReadOnly),"Original trace read");return file.readAll();}();
    require(sourceBytes.startsWith("ABIF"),"Original trace preserved");
    QTemporaryDir directory;const auto copiedPath=directory.filePath("trace Я.ab1");QFile copy(copiedPath);require(copy.open(QIODevice::WriteOnly),"Create private AB1 fixture");copy.write(sourceBytes);copy.close();
    auto* mime=new QMimeData;mime->setUrls({QUrl::fromLocalFile(copiedPath)});QApplication::clipboard()->setMimeData(mime);
    QKeyEvent paste(QEvent::KeyPress,Qt::Key_V,Qt::ControlModifier);QApplication::sendEvent(view,&paste);wait([&]{return view->isEnabled();});view->select_all();require(view->selected_ids().size()==2&&panel->canvas()->sequence_id()!=ids[0],"AB1 append via clipboard paths and Unicode title");
    action(window,"&Undo")->trigger();view->unselect_all();require(panel->has_traces()&&!window.windowTitle().contains('*'),"Append undo restores saved baseline");
    window.open_path(second);wait([&]{return view->isEnabled();});require(panel->has_traces()&&panel->canvas()->sequence_id()!=0,"Second supplied AB1 opens");
    view->select_all();require(view->selected_ids().size()==1,"Second trace replaces first clean baseline");
    const auto exported=directory.filePath("exported Я.fasta");bool chosen=false;QTimer picker;picker.setInterval(5);QObject::connect(&picker,&QTimer::timeout,[&]{if(auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget())){if(chosen)return;chosen=true;dialog->selectFile(exported);QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);}});picker.start();action(window,"Export FASTA…")->trigger();picker.stop();wait([&]{return view->isEnabled();});require(chosen&&QFile::exists(exported),"Explicit FASTA export offers separate destination");
    QFile exportFile(exported);require(exportFile.open(QIODevice::ReadOnly)&&exportFile.readAll().startsWith('>'),"FASTA text export");require(panel->has_traces(),"Export retains live attachment");
    bool selectedProtected=false,rejectedProtected=false;QTimer guard;guard.setInterval(5);QObject::connect(&guard,&QTimer::timeout,[&]{auto* active=QApplication::activeModalWidget();if(auto* dialog=qobject_cast<QFileDialog*>(active)){if(!selectedProtected){selectedProtected=true;dialog->setOption(QFileDialog::DontConfirmOverwrite,true);dialog->selectFile(copiedPath);QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);}}else if(auto* box=qobject_cast<QMessageBox*>(active)){if(box->windowTitle()=="Choose another filename"){rejectedProtected=true;box->done(QMessageBox::Ok);}else box->done(QMessageBox::Yes);}});QTimer watchdog;watchdog.setSingleShot(true);watchdog.setInterval(5000);QObject::connect(&watchdog,&QTimer::timeout,[&]{if(auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget())){std::cerr<<"Save guard timeout: "<<d->windowTitle().toStdString()<<std::endl;d->reject();}});watchdog.start();guard.start();action(window,"Export FASTA…")->trigger();guard.stop();watchdog.stop();require(rejectedProtected,"ABIF overwrite blocked");QFile preserved(copiedPath);require(preserved.open(QIODevice::ReadOnly)&&preserved.readAll()==sourceBytes,"Original bytes remain unchanged");
    window.close();
}
void mafft_check(const QString& path,const QString& launcher){
    auto source=std::make_shared<Document>();QFile traceFile(path);require(traceFile.open(QIODevice::ReadOnly),"Open MAFFT trace fixture");std::istringstream input(traceFile.readAll().toStdString());auto record=read_abif(input,"linked",path.toUtf8().toStdString());
    source->load({record,{"reference",record.symbols.substr(15,record.symbols.size()-30)}});
    for(int mode=0;mode<3;++mode){MafftOptions options;options.threads=1;MafftConfiguration config{launcher,options,{}};
        if(mode==2)config.templateRequest=MafftTemplateRequest{{1},{2},false};
        MafftRun run(source,config,mode==1?std::optional<Document::ColumnRange>({30,100}):std::nullopt);std::shared_ptr<Document> result;QString error;bool done=false;
        run.finished=[&](std::shared_ptr<Document> next,const QString&,const QString& e,bool){result=next;error=e;done=true;};run.start();wait([&]{return done;});
        if(!result)std::cerr<<error.toStdString()<<'\n';
        require(result&&result->sequence(1).trace==record.trace,"Real MAFFT trace provenance");validate_trace(result->sequence(1));
    }
}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{
    require(argc>=3,"Two AB1 paths required");canvas_check(argc>3?QString::fromLocal8Bit(argv[3]):QString());window_check(QString::fromLocal8Bit(argv[1]),QString::fromLocal8Bit(argv[2]),argc>4?QString::fromLocal8Bit(argv[4]):QString());
    if(argc>5)mafft_check(QString::fromLocal8Bit(argv[1]),QString::fromLocal8Bit(argv[5]));
    std::cout<<"PASS stage 0.8: original/synthetic/deleted anchors, zoom/render/reverse/undo, both AB1 imports, paired navigation, hide/show and real MAFFT provenance\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
