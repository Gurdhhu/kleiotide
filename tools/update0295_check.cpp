#include "test_preferences.hpp"
#include "core/chromatogram.hpp"
#include "core/session.hpp"
#include "gui/alignment_view.hpp"
#include "gui/alignment_overview.hpp"
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
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QStyle>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <array>
#include <cmath>
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
Record long_trace() {
    auto evidence=std::make_shared<Chromatogram>();
    auto link=std::make_shared<TraceLink>();
    link->original=evidence;
    evidence->calls.assign(600,'A');
    evidence->quality.assign(600,10);
    for(std::int32_t i=0;i<600;++i) {
        evidence->peaks.push_back(static_cast<std::uint32_t>(20*i+10));
        link->originalCalls.push_back(i);
    }
    for(auto& channel:evidence->channels)channel.resize(12000,1);
    return {"long trace",evidence->calls,RowKind::Nucleotide,link};
}
template<class Check>bool wait_for(Check ready) {
    QElapsedTimer timer;
    timer.start();
    while(!ready()&&timer.elapsed()<10000) {
        QApplication::processEvents();
        QThread::msleep(1);
    }
    return ready();
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
QRect device_pixels(const QImage& image,const QRect& logical) {
    const auto scale=image.devicePixelRatio();
    const int left=static_cast<int>(std::ceil(logical.x()*scale));
    const int top=static_cast<int>(std::ceil(logical.y()*scale));
    const int right=static_cast<int>(std::floor((logical.x()+logical.width())*scale));
    const int bottom=static_cast<int>(std::floor((logical.y()+logical.height())*scale));
    return QRect(left,top,right-left,bottom-top).intersected(image.rect());
}
void check_manual_cell(const QImage& before,const QImage& after,const QRect& cell) {
    const auto interior=device_pixels(after,cell.adjusted(2,2,-2,-2));
    int palePixels=0,glyphPixels=0;
    for(int y=interior.top();y<=interior.bottom();++y)
        for(int x=interior.left();x<=interior.right();++x){
            const auto lightness=after.pixelColor(x,y).lightness();
            if(lightness>180&&after.pixel(x,y)!=before.pixel(x,y))++palePixels;
            if(lightness<120)++glyphPixels;
        }
    require(palePixels>0&&palePixels*4>=interior.width()*interior.height(),
            "Manual mark adds a pale interior to a residue-colored cell");
    require(glyphPixels>0,"Dark residue letters are visible against the pale fill");

    const auto topEdge=device_pixels(after,QRect(cell.x()+2,cell.y(),cell.width()-4,2));
    bool darkBorder=false;
    for(int y=topEdge.top();y<=topEdge.bottom();++y)
        for(int x=topEdge.left();x<=topEdge.right();++x)
            if(after.pixelColor(x,y).lightness()<160)darkBorder=true;
    require(darkBorder,"Manual mark retains a darker border");
}
void check_column_shade(const QImage& before,const QImage& after,const QRect& band,
                        const QColor& background,const QColor& shade) {
    QImage sample(1,1,QImage::Format_RGB32);
    sample.fill(background);
    QPainter painter(&sample);
    painter.fillRect(sample.rect(),shade);
    painter.end();
    const auto expected=sample.pixelColor(0,0);
    const auto area=device_pixels(after,band.adjusted(2,2,-2,-2));
    int backgroundPixels=0,tintedPixels=0;
    for(int y=area.top();y<=area.bottom();++y)
        for(int x=area.left();x<=area.right();++x){
            if(before.pixelColor(x,y)!=background)continue;
            ++backgroundPixels;
            const auto pixel=after.pixelColor(x,y);
            if(std::abs(pixel.red()-expected.red())<=2&&std::abs(pixel.green()-expected.green())<=2&&
               std::abs(pixel.blue()-expected.blue())<=2)++tintedPixels;
        }
    // Inspect background pixels through the join between rows, skipping glyphs
    // and the outer edges of the captured band.
    require(expected!=background&&backgroundPixels>0&&tintedPixels*20>=backgroundPixels*19,
            "Column highlight is even shading across rows in every display mode");
}
}

int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        TestPreferences preferences;
        QTemporaryDir directory;
        Document source;
        source.load({traced("first",{0,19,20,45}),traced("second",{5,5,5,5}),
                     traced("missing Q",{}),{"plain","ACGT"},long_trace()});
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
        require(wait_for([&]{return view->isEnabled()&&window.current_document().rows().size()==5;}),
                "Open trace session");
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
        require(window.style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent)==0,
                "Only the panel grips provide resize targets");
        auto* restoreLayout=child<QAction>(window,"restoreDefaultLayout");
        auto* viewMenu=qobject_cast<QMenu*>(restoreLayout->parent());
        require(viewMenu,"View menu");
        const auto actions=viewMenu->actions();
        const auto layoutPosition=actions.indexOf(restoreLayout);
        require(layoutPosition>0&&layoutPosition+2<actions.size()&&
                actions[layoutPosition-1]->isSeparator()&&actions[layoutPosition+1]->isSeparator()&&
                actions[layoutPosition+2]==child<QAction>(window,"alignmentFont"),
                "Restore default layout has its own section before Alignment font");

        auto* overview=dynamic_cast<AlignmentOverview*>(child<QWidget>(window,"alignmentOverview"));
        require(overview,"Alignment overview widget");
        view->select_fragment({{5},3,35});
        const auto fragment=view->fragment();
        auto* traceScroll=canvas->horizontalScrollBar();
        const int traceAtSelection=traceScroll->value();
        const auto overviewMouse=[&](QEvent::Type type,const QPoint& point,
                                     Qt::MouseButton button,Qt::MouseButtons buttons){
            QMouseEvent event(type,point,overview->mapToGlobal(point),button,buttons,Qt::NoModifier);
            QApplication::sendEvent(overview,&event);
        };
        const QPoint left(12,40),middle(overview->width()/3,40),right(overview->width()-12,40);
        overviewMouse(QEvent::MouseButtonPress,left,Qt::LeftButton,Qt::LeftButton);
        overviewMouse(QEvent::MouseMove,middle,Qt::NoButton,Qt::LeftButton);
        const auto middleColumn=view->visible_column_range().first;
        overviewMouse(QEvent::MouseMove,right,Qt::NoButton,Qt::LeftButton);
        require(middleColumn>0&&view->visible_column_range().first>middleColumn&&view->fragment()==fragment,
                "Overview scrolls even while the entire selected fragment is offscreen");
        require(traceScroll->value()==traceAtSelection,"Overview drag defers trace scrolling");
        overviewMouse(QEvent::MouseButtonRelease,right,Qt::LeftButton,Qt::NoButton);
        require(traceScroll->value()>traceAtSelection&&canvas->focused_column()==3,
                "Trace follows the final viewport while retaining the selected column");
        view->horizontalScrollBar()->setValue(0);
        require(traceScroll->value()==traceAtSelection,"Normal scrolling still follows the viewport");

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
        require(marked(5,0),"All traces includes the long row");
        view->select_ids({1,2});
        quality(1);
        require(!marked(5,0),"Selected scope excludes the long row");
        view->select_fragment({{1},2,2});
        trigger("unhighlightSelectedBases");
        require(marked(1,0)&&!marked(1,2),"Unhighlight suppresses only selected low-Q calls");
        view->set_residue_display(ResidueDisplay::Backgrounds);
        view->clear_residue_selection();
        const auto rowTop=[&](int row){return view->ruler_height()+row*view->row_height();};
        const auto cellLeft=[&](int column){return view->name_area_width()+column*view->cell_width();};
        const auto grabAlignment=[&] {
            return view->viewport()->grab().toImage();
        };
        const QImage before=grabAlignment();
        require(!before.isNull(),"Capture the two unmarked residues");
        view->select_fragment({{4},1,2});
        trigger("highlightSelectedBases");
        require(marked(4,1)&&marked(4,2)&&!marked(4,0),"Manual marks apply to selected cells in an untraced row");
        view->clear_residue_selection();
        const QImage after=grabAlignment();
        require(before.size()==after.size()&&before!=after,"Manual highlight changes colored cells");
        for(int column:{1,2})
            check_manual_cell(before,after,QRect(cellLeft(column),rowTop(3),view->cell_width(),view->row_height()));
        view->select_fragment({{4},2,1});
        trigger("unhighlightSelectedBases");
        require(marked(4,1)&&!marked(4,2),"Partial unhighlight retains adjacent manual mark");
        view->clear_residue_selection();

        constexpr std::array styles{ResidueDisplay::Letters,ResidueDisplay::Monochrome,ResidueDisplay::Backgrounds};
        std::array<QImage,styles.size()> unshaded;
        for(std::size_t i=0;i<styles.size();++i){
            view->set_residue_display(styles[i]);
            unshaded[i]=grabAlignment();
        }
        child<QAction>(window,"highlightConserved")->setChecked(true);
        // The long A-only row and four short rows all have A at this column.
        require(wait_for([&]{const auto columns=view->site_highlights();return columns&&(*columns)[0];}),
                "Conserved column computed");
        const QRect band(cellLeft(0),rowTop(3),view->cell_width(),2*view->row_height());
        for(std::size_t i=0;i<styles.size();++i){
            const auto style=styles[i];
            view->set_residue_display(style);
            const bool coloredCells=style==ResidueDisplay::Backgrounds;
            const QColor background=coloredCells?residue_paint('A',view->residue_palette(),style).background:QColor(Qt::white);
            auto shade=view->site_highlight_color();
            shade.setAlpha(coloredCells?115:80);
            const auto shaded=grabAlignment();
            check_column_shade(unshaded[i],shaded,band,background,shade);
            shade.setAlpha(70);
            check_column_shade(unshaded[i],shaded,QRect(cellLeft(0),0,view->cell_width(),view->ruler_height()),
                               QColor(238,240,244),shade);
        }

        child<QAction>(window,"saveAlignment")->trigger();
        require(wait_for([&]{return view->isEnabled();}),"Save session with highlights");
        MainWindow reopened;
        auto* restoredView=dynamic_cast<AlignmentView*>(reopened.centralWidget());
        require(restoredView,"Reopened alignment view");
        reopened.open_path(path);
        require(wait_for([&]{return restoredView->isEnabled()&&reopened.current_document().rows().size()==5;}),
                "Reopen saved session");
        const auto& restored=reopened.current_document();
        require(restoredView->base_highlighted(restored.sequence(4),1)&&
                !restoredView->base_highlighted(restored.sequence(4),2)&&
                restoredView->base_highlighted(restored.sequence(1),0)&&
                !restoredView->base_highlighted(restored.sequence(1),2)&&
                !restoredView->base_highlighted(restored.sequence(5),0),
                "Manual marks, low-quality scope and exclusions survive the round trip");
        require(wait_for([&]{const auto columns=restoredView->site_highlights();return columns&&(*columns)[0];}),
                "Column rules and their shading survive the round trip");
        reopened.close();

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
        std::cout<<"PASS trace selection, panel grips, overview drag, highlight painting and persistence, and MAFFT dialog sizing\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL "<<error.what()<<'\n';
        return 1;
    }
}
