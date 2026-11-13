#include "test_preferences.hpp"
#include "core/chromatogram.hpp"
#include "core/session.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "gui/mafft_dialog.hpp"
#include "gui/main_window.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace kleiotide;
namespace {
void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
template<class T>T* child(QObject& parent,const char* name) {
    auto* value=parent.findChild<T*>(name);
    require(value,name);
    return value;
}
Record traced(std::string name,std::vector<std::uint8_t> quality) {
    auto evidence=std::make_shared<Chromatogram>();
    evidence->calls="ACGT";
    evidence->peaks={4,8,12,16};
    evidence->quality=std::move(quality);
    for(auto& channel:evidence->channels)channel.resize(24,1);
    auto link=std::make_shared<TraceLink>();
    link->original=evidence;
    link->originalCalls={0,1,2,3};
    return {std::move(name),"A~CGT",RowKind::Nucleotide,link};
}
template<class Open,class Check>void modal(Open open,Check check) {
    QTimer driver;
    driver.setInterval(5);
    bool done=false;
    QString error;
    QObject::connect(&driver,&QTimer::timeout,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if(!dialog)return;
        driver.stop();
        try { check(*dialog);done=true; }
        catch(const std::exception& e) { error=QString::fromUtf8(e.what()); }
        if(dialog->isVisible())dialog->reject();
    });
    driver.start();
    open();
    require(done&&error.isEmpty(),qPrintable(error));
}
void drag_grip(QDockWidget& dock,int amount) {
    auto* grip=dock.titleBarWidget();
    const QPoint local=grip->rect().center();
    const QPoint global=grip->mapToGlobal(local);
    QMouseEvent down(QEvent::MouseButtonPress,local,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(grip,&down);
    QMouseEvent move(QEvent::MouseMove,local-QPoint(0,amount),global-QPoint(0,amount),
                     Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(grip,&move);
    QMouseEvent up(QEvent::MouseButtonRelease,local,global-QPoint(0,amount),
                   Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(grip,&up);
    QApplication::processEvents();
}
}

int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        TestPreferences preferences;
        QTemporaryDir directory;
        Document source;
        source.load({traced("first",{0,19,20,45}),traced("second",{5,5,5,5}),
                     traced("missing Q",{}),{"plain","ACGT"}});
        source.replace(1,4,1,"A");
        source.replace(1,5,0,"A");
        source.transform({2},Document::Transform::ReverseComplement);
        const auto path=directory.filePath("preview.kleio");
        std::ostringstream bytes;
        write_session(bytes,source);
        QFile file(path);
        require(file.open(QIODevice::WriteOnly),"Create trace fixture");
        file.write(QByteArray::fromStdString(bytes.str()));
        file.close();

        MainWindow window;
        window.resize(1050,760);
        window.show();
        auto* view=dynamic_cast<AlignmentView*>(window.centralWidget());
        require(view,"Alignment view");
        window.open_path(path);
        QElapsedTimer timer;
        timer.start();
        while((!view->isEnabled()||window.current_document().rows().size()!=4)&&timer.elapsed()<10000) {
            app.processEvents();
            QThread::msleep(1);
        }
        require(view->isEnabled()&&window.current_document().rows().size()==4,"Open trace session");
        auto& document=window.current_document();
        const auto state=document.state_id();
        const auto trigger=[&](const char* name){child<QAction>(window,name)->trigger();};
        auto* traceDock=child<QDockWidget>(window,"chromatogramDock");
        auto* overviewDock=child<QDockWidget>(window,"alignmentOverviewDock");
        auto* canvas=dynamic_cast<TraceCanvas*>(child<QWidget>(window,"traceCanvas"));
        require(canvas,"Chromatogram canvas");
        require(canvas->sequence_id()==0,"Session without residue selection starts with blank trace");
        view->select_fragment({{1},2,1});
        require(canvas->sequence_id()==1&&canvas->focused_column()==2,"Traced residue shows corresponding chromatogram");
        view->select_fragment({{4},2,1});
        require(canvas->sequence_id()==0,"Untraced residue clears chromatogram");
        view->select_ids({1});
        view->clear_residue_selection();
        require(canvas->sequence_id()==0,"Name selection alone leaves chromatogram blank");
        child<QComboBox>(window,"traceSequence")->setCurrentIndex(1);
        require(canvas->sequence_id()==2&&view->fragment()&&view->fragment()->ids.front()==2,"Dropdown selects traced sequence");

        traceDock->show();
        overviewDock->show();
        app.processEvents();
        require(traceDock->geometry().bottom()<overviewDock->geometry().top(),"Overview below trace");
        const int previousTrace=traceDock->height(),previousOverview=overviewDock->height();
        drag_grip(*overviewDock,24);
        require(overviewDock->height()>previousOverview&&traceDock->height()==previousTrace,"Overview grip changes only overview height");
        const int changedOverview=overviewDock->height();
        drag_grip(*traceDock,24);
        require(traceDock->height()>previousTrace&&overviewDock->height()==changedOverview,"Trace grip changes only trace height");
        const int changedTrace=traceDock->height();
        const int alignmentHeight=view->height();
        trigger("chromatogramView");
        app.processEvents();
        require(traceDock->isHidden()&&overviewDock->height()==changedOverview&&view->height()>alignmentHeight,
                "Hiding trace returns its space to the alignment");
        trigger("chromatogramView");
        app.processEvents();
        require(traceDock->isVisible()&&traceDock->height()==changedTrace&&overviewDock->height()==changedOverview,
                "Reopening trace restores its own height");
        trigger("alignmentOverviewView");
        app.processEvents();
        require(overviewDock->isHidden()&&traceDock->height()==changedTrace&&view->height()>alignmentHeight,
                "Hiding overview returns its space to the alignment");
        trigger("alignmentOverviewView");
        app.processEvents();
        require(overviewDock->isVisible()&&overviewDock->height()==changedOverview&&traceDock->height()==changedTrace,
                "Reopening overview restores its own height");
        trigger("restoreDefaultLayout");
        app.processEvents();
        require(traceDock->isVisible()&&overviewDock->isVisible()&&
                traceDock->height()==previousTrace&&overviewDock->height()==previousOverview&&
                traceDock->geometry().bottom()<overviewDock->geometry().top(),"Restore default panel sizes and order");

        const auto quality=[&](int scope){
            modal([&]{trigger("highlightLowQuality");},[&](QDialog& dialog){
                require(dialog.objectName()=="highlightQualityDialog","Quality highlight dialog");
                child<QSpinBox>(dialog,"highlightQualityThreshold")->setValue(20);
                child<QComboBox>(dialog,"highlightQualityScope")->setCurrentIndex(scope);
                dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();
                require(dialog.result()==QDialog::Accepted,"Apply low-Q highlighting");
            });
        };
        const auto marked=[&](SequenceId id,std::size_t column){return view->base_highlighted(document.sequence(id),column);};
        quality(1);
        require(marked(1,0)&&marked(1,2),"Q0 and Q19 qualify at threshold 20");
        require(!marked(1,1)&&!marked(1,3)&&!marked(1,4)&&!marked(1,5),"Gap, threshold, edited and inserted calls skipped");
        require(!marked(2,0)&&!marked(3,0)&&!marked(4,0),"Selected trace scope and missing data");
        quality(0);
        require(marked(2,0)&&marked(2,4),"Original scores follow reverse-complemented calls");
        view->select_fragment({{1},2,2});
        trigger("unhighlightSelectedBases");
        require(marked(1,0)&&!marked(1,2),"Unhighlight suppresses only selected low-Q calls");
        view->set_residue_display(ResidueDisplay::Letters);
        view->clear_residue_selection();
        const auto grabMarkedCells=[&] {
            const QRect cells(view->name_area_width()+view->cell_width(),
                              view->ruler_height()+3*view->row_height(),
                              2*view->cell_width(),view->row_height());
            return view->viewport()->grab(cells).toImage();
        };
        const QImage before=grabMarkedCells();
        require(!before.isNull(),"Capture the two unmarked residues");
        view->select_fragment({{4},1,2});
        trigger("highlightSelectedBases");
        require(marked(4,1)&&marked(4,2)&&!marked(4,0),"Manual marks apply to selected cells in an untraced row");
        view->clear_residue_selection();
        const QImage after=grabMarkedCells();
        require(before.size()==after.size()&&before!=after,"Manual highlight draws a visible border");
        int glyphPixels=0;
        for(int y=0;y<before.height();++y) {
            for(int x=0;x<before.width();++x) {
                if(before.pixelColor(x,y)==Qt::white)continue;
                ++glyphPixels;
                require(before.pixel(x,y)==after.pixel(x,y),"Highlight border preserves every residue glyph pixel");
            }
        }
        require(glyphPixels>0,"Captured cells contain visible residue letters");
        view->select_fragment({{4},2,1});
        trigger("unhighlightSelectedBases");
        require(marked(4,1)&&!marked(4,2),"Partial unhighlight retains adjacent manual mark");
        trigger("highlightClear");
        require(!marked(4,1)&&!marked(1,0)&&document.state_id()==state,"Clear removes both layers without editing");

        const auto widthOnly=[](QDialog& dialog){
            const int width=dialog.width(),height=dialog.height();
            require(dialog.minimumHeight()==dialog.maximumHeight(),"MAFFT height fixed");
            require(dialog.maximumWidth()>dialog.minimumWidth(),"MAFFT width adjustable");
            const int desired=width<dialog.maximumWidth()?std::min(width+32,dialog.maximumWidth()):std::max(width-32,dialog.minimumWidth());
            require(desired!=width,"Room for width adjustment");
            dialog.resize(desired,height+30);
            require(dialog.width()==desired&&dialog.height()==height,"MAFFT width can change independently");
        };
        modal([&]{choose_mafft(&window,{});},widthOnly);
        modal([&]{choose_mafft_templates(&window,document,{1},false);},widthOnly);
        window.close();
        std::cout<<"PASS trace selection, independent panel heights and visibility, readable per-base highlights and MAFFT dialog sizing\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL "<<error.what()<<'\n';
        return 1;
    }
}
