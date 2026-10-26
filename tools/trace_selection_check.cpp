#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "core/session.hpp"
#include <QAction>
#include <QApplication>
#include <QBackingStore>
#include <QComboBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QImage>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void mouse(AlignmentView& view,QEvent::Type type,QPoint point,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent event(type,QPointF(point),QPointF(view.viewport()->mapToGlobal(point)),button,buttons,Qt::NoModifier);QApplication::sendEvent(view.viewport(),&event);}
QImage pixels(TraceCanvas& canvas){auto* device=canvas.backingStore()->paintDevice();require(device->devType()==QInternal::Image,"Raster backing store required");const auto* image=static_cast<QImage*>(device);const auto origin=canvas.viewport()->mapTo(canvas.window(),QPoint(0,0));const auto dpr=image->devicePixelRatio();return image->copy(QRect(qRound(origin.x()*dpr),qRound(origin.y()*dpr),qRound(canvas.viewport()->width()*dpr),qRound(canvas.viewport()->height()*dpr)));}
void check_cache(TraceCanvas& canvas,int gain){QApplication::processEvents();const auto cached=pixels(canvas);canvas.set_gain(gain);canvas.viewport()->repaint();require(cached==pixels(canvas),"Cached/partial trace pixels differ from a fresh render");}
void check_positions(TraceCanvas& canvas,const Sequence& row){
    for(std::size_t c=0;c<=row.symbols.size();++c){const auto expected=trace_position(row,c),actual=canvas.position(c);require(std::abs(expected.sample-actual.sample)<1e-8&&expected.current==actual.current&&expected.originalCall==actual.originalCall&&expected.imported==actual.imported&&expected.edited==actual.edited&&expected.quality==actual.quality,"Cached trace position differs from model");}
}
void check_labels(TraceCanvas& canvas,const Sequence& row){
    const auto labels=canvas.current_labels();std::vector<std::size_t> expected;
    for(std::size_t c=0;c<row.symbols.size();++c){if(!canvas.alignment_spacing()&&is_gap(row.symbols.text()[c]))continue;const auto x=canvas.column_x(c);if(x>=-5&&x<=canvas.viewport()->width()+5)expected.push_back(c);}
    require(labels.size()==expected.size(),"All visible calls, including overlapping and inserted calls, are retained");
    for(std::size_t i=0;i<labels.size();++i){const auto& label=labels[i];const auto position=trace_position(row,expected[i]);const bool inserted=!is_gap(position.current)&&!position.originalCall;
        require(label.column==expected[i]&&label.symbol==position.current&&std::abs(label.sample-position.sample)<1e-8&&label.inserted==inserted&&label.edited==(inserted||position.edited),"Current label mapping and edited style");}
}
Record trace_record(std::size_t count){
    auto evidence=std::make_shared<Chromatogram>();evidence->sourcePath="synthetic.ab1";evidence->sampleName="Performance fixture";
    for(std::size_t c=0;c<count;++c){evidence->calls+="ACGT"[c%4];evidence->peaks.push_back(static_cast<std::uint32_t>(20+c*12));evidence->quality.push_back(35);}
    for(std::size_t channel=0;channel<4;++channel){auto& values=evidence->channels[channel];values.resize(count*12+40,1);
        for(std::size_t c=channel;c<count;c+=4)for(int d=-10;d<=10;++d){const auto sample=static_cast<std::size_t>(static_cast<int>(evidence->peaks[c])+d);values[sample]+=static_cast<std::int32_t>(500.*std::exp(-d*d/10.));}}
    auto link=std::make_shared<TraceLink>();link->original=evidence;for(std::size_t c=0;c<count;++c)link->originalCalls.push_back(static_cast<std::int32_t>(c));
    return {"Trace one",evidence->calls,RowKind::Nucleotide,link};
}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir directory;Document source;auto first=trace_record(1000),second=first;second.name="Trace two";source.load({first,second,{"No trace",first.symbols}});
    const auto path=directory.filePath("trace-selection.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,source);}
    MainWindow window;window.resize(3440,1440);window.show();window.open_path(path);auto* view=dynamic_cast<AlignmentView*>(window.centralWidget());QElapsedTimer timeout;timeout.start();while(!view->isEnabled()&&timeout.elapsed()<20000){app.processEvents();QThread::msleep(1);}require(view->isEnabled(),"Open timeout");app.processEvents();
    auto* dock=window.findChild<QDockWidget*>("chromatogramDock");ChromatogramPanel* panel=nullptr;for(auto* widget:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ChromatogramPanel*>(widget))panel=candidate;
    require(dock&&panel&&panel->has_traces(),"Linked trace panel");auto* spacing=window.findChild<QComboBox*>("traceSpacing");require(spacing,"Spacing selector");
    for(bool visible:{false,true})for(int aligned:{0,1}){
        dock->setVisible(visible);spacing->setCurrentIndex(aligned);view->set_mode(AlignmentView::Mode::View);app.processEvents();
        for(bool movingStart:{false,true}){
            const int base=view->name_area_width()+view->cell_width()*60;const int anchor=movingStart?base:base-view->cell_width()*50;
            const QPoint begin(anchor,view->ruler_height()+view->row_height()/2);mouse(*view,QEvent::MouseButtonPress,begin,Qt::LeftButton,Qt::LeftButton);app.processEvents();
            std::vector<double> frames,events;QPoint last;
            for(int i=0;i<80;++i){const int phase=i<40?i:79-i;last={movingStart?anchor-phase*view->cell_width():anchor+phase*view->cell_width(),begin.y()+(i%3?0:view->row_height())};QElapsedTimer timer;timer.start();mouse(*view,QEvent::MouseMove,last,Qt::NoButton,Qt::LeftButton);events.push_back(timer.nsecsElapsed()/1e6);app.processEvents();frames.push_back(timer.nsecsElapsed()/1e6);}
            mouse(*view,QEvent::MouseButtonRelease,last,Qt::LeftButton,Qt::NoButton);app.processEvents();std::sort(frames.begin(),frames.end());std::sort(events.begin(),events.end());
            std::cout<<(visible?"open":"hidden")<<' '<<(aligned?"aligned":"sample")<<' '<<(movingStart?"moving-start":"fixed-start")<<": input p95 "<<events[76]<<" ms; frame median "<<frames[40]<<" ms; p95 "<<frames[76]<<" ms; max "<<frames.back()<<" ms\n"<<std::flush;
        }
    }
    if(app.arguments().contains("--benchmark-only")){window.close();return 0;}
    dock->show();auto* canvas=panel->canvas();auto* zoom=window.findChild<QSpinBox*>("traceZoom");auto* gain=window.findChild<QSpinBox*>("traceGain");require(zoom&&gain,"Trace controls");
    for(int aligned:{0,1})for(int percent:{25,100,175,400}){
        spacing->setCurrentIndex(aligned);zoom->setValue(percent);gain->setValue(150);view->select_fragment({{1},60,1});app.processEvents();
        check_labels(*canvas,window.current_document().sequence(1));check_cache(*canvas,150);
        for(int i=0;i<8;++i){view->select_fragment({i%2?std::vector<SequenceId>{1}:std::vector<SequenceId>{1,2},60,static_cast<std::size_t>(i*3+1)});app.processEvents();check_cache(*canvas,150);}
        canvas->horizontalScrollBar()->setValue(canvas->horizontalScrollBar()->value()+37);check_cache(*canvas,150);canvas->focus_column(65,false);check_cache(*canvas,150);
    }
    view->select_fragment({{2},50,3});require(canvas->sequence_id()==2&&canvas->focused_column()==50,"Selection follows a different linked trace");view->select_fragment({{1,2},55,4});require(canvas->sequence_id()==2&&canvas->focused_column()==55,"Block retains current trace when included");
    view->select_fragment({{3},55,4});require(canvas->sequence_id()==2,"Untraced row leaves current evidence available");check_cache(*canvas,150);
    for(auto palette:{ResiduePalette::Chromas,ResiduePalette::Classic}){canvas->set_residue_palette(palette);check_cache(*canvas,150);}
    auto font=canvas->font();font.setPointSize(12);canvas->setFont(font);check_cache(*canvas,150);window.resize(2800,1200);app.processEvents();check_cache(*canvas,150);
    // Compare the cached index with the model after edits, gaps, deleted calls,
    // coincident peaks, reverse complement and long synthetic runs.
    Document edited;auto record=trace_record(100);auto coincident=std::make_shared<Chromatogram>(*record.trace->original);coincident->peaks[1]=coincident->peaks[0];auto link=std::make_shared<TraceLink>(*record.trace);link->original=coincident;record.trace=link;edited.load({record});const auto evidence=edited.rows()[0].trace->original;
    TraceCanvas test;test.resize(1000,260);test.show();std::mt19937 random(194);
    for(unsigned i=0;i<75;++i){const auto length=edited.rows()[0].symbols.size();if(i%7==0)edited.transform({1},Document::Transform::ReverseComplement);else{const auto at=random()%(length+1);const auto count=random()%(std::min<std::size_t>(length-at,3)+1);std::string text;for(unsigned j=0,n=random()%5;j<n;++j)text+="ACGN-~"[random()%6];edited.replace(1,at,count,text);}
        test.set_sequence(&edited.rows()[0]);check_positions(test,edited.rows()[0]);for(bool aligned:{false,true}){test.set_alignment_spacing(aligned);test.horizontalScrollBar()->setValue(static_cast<int>(random()%300));app.processEvents();check_labels(test,edited.rows()[0]);check_cache(test,100);}require(edited.rows()[0].trace->original==evidence,"Original evidence remains immutable");}
    edited.replace(1,0,edited.rows()[0].symbols.size(),"~~");test.set_sequence(&edited.rows()[0]);check_positions(test,edited.rows()[0]);edited.replace(1,1,0,std::string(2048,'N'));test.set_sequence(&edited.rows()[0]);check_positions(test,edited.rows()[0]);edited.transform({1},Document::Transform::ReverseComplement);test.set_sequence(&edited.rows()[0]);check_positions(test,edited.rows()[0]);
    test.set_sequence(nullptr);require(test.current_labels().empty(),"Removing trace clears cached labels");test.close();window.close();std::cout<<"PASS trace selection: open/hidden benchmark, cache pixels, trace following, positions/labels after edits and reverse complement, coincident peaks, gaps, synthetic runs, zoom/gain/palette/font/resize\n";
}catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}return 0;}
