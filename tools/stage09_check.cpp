#include "gui/main_window.hpp"
#include "gui/chromatogram_view.hpp"
#include "gui/alignment_view.hpp"
#include "gui/open_requests.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <sstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F>void wait(F f){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<30000){QApplication::processEvents();QThread::msleep(1);}require(f(),"Timed out");}
QAction* action(MainWindow& w,const QString& title){for(auto* a:w.findChildren<QAction*>())if(a->text()==title)return a;throw std::runtime_error("Missing menu action");}
void write(const QString& path,const std::string& bytes){QFile f(path);require(f.open(QIODevice::WriteOnly),"Write fixture");require(f.write(bytes.data(),static_cast<qint64>(bytes.size()))==static_cast<qint64>(bytes.size()),"Fixture bytes");}
ChromatogramPanel* panel(MainWindow& w){for(auto* widget:w.findChildren<QWidget*>())if(auto* p=dynamic_cast<ChromatogramPanel*>(widget))return p;throw std::runtime_error("Missing trace panel");}
void rendering(const QString& picture){std::istringstream input(test::synthetic_abif("ACGTACGT"));auto record=read_abif(input,"dense","synthetic.ab1");auto data=std::make_shared<Chromatogram>(*record.trace->original);data->peaks={30,32,34,36,38,40,42,44};auto link=std::make_shared<TraceLink>(*record.trace);link->original=data;record.trace=link;Document d;d.load({record});
    ChromatogramPanel p;p.resize(1100,330);p.set_document(&d);p.show();QApplication::processEvents();auto* canvas=p.canvas();
    for(int zoom:{25,100,400}){p.findChild<QSpinBox*>("traceZoom")->setValue(zoom);require(canvas->current_labels().size()==8,"All dense calls retained at every zoom");require(!canvas->grab().isNull(),"Dense labels rendered");}
    p.findChild<QSpinBox*>("traceZoom")->setValue(100);d.replace(1,3,0,"WR");p.set_document(&d);p.follow_selection({1},3);require(canvas->current_labels().size()==10,"All inserted/current labels drawn");
    p.follow_selection({1},6);const auto labels=canvas->current_labels();require(labels.size()==10&&labels[3].symbol=='W'&&labels[3].inserted&&labels[4].symbol=='R'&&labels[4].inserted,"Unselected insertions persist");
    d.transform({1},Document::Transform::ReverseComplement);p.set_document(&d);p.follow_selection({1},0);require(canvas->current_labels().size()==10,"Reversed insertions persist");
    if(!picture.isEmpty())require(p.grab().save(picture),"Save render");
}
void workflows(const QString& picture){QTemporaryDir dir;const auto a=dir.filePath("first trace Я.ab1"),b=dir.filePath("second trace.ab1"),fasta=dir.filePath("mask.fasta");write(a,test::synthetic_abif());write(b,test::synthetic_abif("ACGTACGT"));write(fasta,">mask\n12*4\n");
    MainWindow w;w.resize(1180,820);w.show();auto* view=dynamic_cast<AlignmentView*>(w.centralWidget());auto* traces=panel(w);
    // Exercise the actual Open dialog with multiple selections.
    bool chosen=false;QTimer picker;picker.setInterval(5);QObject::connect(&picker,&QTimer::timeout,[&]{if(auto* d=qobject_cast<QFileDialog*>(QApplication::activeModalWidget())){if(chosen)return;chosen=true;require(d->fileMode()==QFileDialog::ExistingFiles,"Open supports multiple files");d->setDirectory(dir.path());d->selectFile("\"first trace Я.ab1\" \"second trace.ab1\"");QMetaObject::invokeMethod(d,"accept",Qt::QueuedConnection);}});picker.start();action(w,"&Open…")->trigger();picker.stop();wait([&]{return view->isEnabled();});require(chosen&&w.current_document().rows().size()==2,"One document from multi-file Open");
    const auto before=w.current_document().rows()[0].trace->original;
    view->set_mode(AlignmentView::Mode::Insert);view->select_fragment({{1},1,1});require(view->paste_residues("W"),"Insert through viewport");view->select_fragment({{1},4,1});require(traces->canvas()->current_labels().size()==7,"Inserted label persists after caret move");
    w.current_document();traces->findChild<QSpinBox*>("traceZoom")->setValue(200);traces->findChild<QSpinBox*>("traceGain")->setValue(150);view->set_name_area_width(310);
    const auto native=dir.filePath("portable session Я.kleio");bool saved=false;QTimer save;save.setInterval(5);QObject::connect(&save,&QTimer::timeout,[&]{if(auto* d=qobject_cast<QFileDialog*>(QApplication::activeModalWidget())){if(saved)return;saved=true;require(d->selectedNameFilter().contains("kleio"),"Trace Save defaults to session");d->selectFile(native);QMetaObject::invokeMethod(d,"accept",Qt::QueuedConnection);}});save.start();action(w,"&Save")->trigger();save.stop();wait([&]{return view->isEnabled();});require(saved&&QFile::exists(native)&&!w.windowTitle().contains('*'),"Native session saved cleanly");
    const auto edited=w.current_document().rows()[0].symbols.text();QFile::remove(a);QFile::remove(b);MainWindow reopened;reopened.show();reopened.open_path(native);auto* v=dynamic_cast<AlignmentView*>(reopened.centralWidget());wait([&]{return v->isEnabled();});auto* restored=panel(reopened);
    require(reopened.current_document().rows().size()==2&&reopened.current_document().rows()[0].symbols.text()==edited,"Edited multi-trace session reopened");require(reopened.current_document().rows()[0].trace->original->channels==before->channels,"Signals embedded; absent AB1 files irrelevant");
    require(v->mode()==AlignmentView::Mode::Insert&&v->fragment()&&v->fragment()->start==4&&v->name_area_width()==310,"Caret/mode/name width restored");require(restored->findChild<QSpinBox*>("traceZoom")->value()==200&&restored->findChild<QSpinBox*>("traceGain")->value()==150,"Trace display settings restored");
    require(v->paste_residues("N")&&reopened.current_document().can_undo(),"Continue editing reopened session");action(reopened,"&Undo")->trigger();require(!reopened.windowTitle().contains('*'),"Undo returns to reopened baseline");
    if(!picture.isEmpty())require(reopened.grab().save(picture),"Session screenshot with badges");reopened.close();w.close();
    MainWindow mixed;mixed.show();write(a,test::synthetic_abif());mixed.open_paths({fasta,a});auto* mv=dynamic_cast<AlignmentView*>(mixed.centralWidget());wait([&]{return mv->isEnabled();});require(mixed.current_document().rows().size()==2&&mixed.current_document().rows()[0].kind==RowKind::Mask&&mixed.current_document().rows()[1].trace,"Mixed FASTA/AB1 batch leaves mask intact");mixed.close();
}
void shell(){OpenRequests broker;const auto key="kleiotide-test-"+QUuid::createUuid().toString(QUuid::Id128);require(broker.listen(key),"Shell listener");QStringList result;int batches=0;broker.opened=[&](const QStringList& paths){result=paths;++batches;};broker.queue({"/tmp/first trace.ab1"});require(OpenRequests::forward(key,{"/tmp/second Я.ab1"}),"Secondary invocation forwarded");wait([&]{return batches==1;});require(result==QStringList({"/tmp/first trace.ab1","/tmp/second Я.ab1"}),"Separate shell launches coalesce into one batch");}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{rendering(argc>1?QString::fromLocal8Bit(argv[1]):QString{});workflows(argc>2?QString::fromLocal8Bit(argv[2]):QString{});shell();std::cout<<"PASS 0.9: dense labels, persistent insertions, multi-select Open, embedded sessions/reopen/edit/undo, metadata, mixed imports and shell request batching\n";}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
