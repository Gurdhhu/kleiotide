#include "gui/alignment_overview.hpp"
#include "gui/alignment_view.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QScrollBar>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void wait_ready(QApplication& app,AlignmentOverview& overview,int limit=10000){
    QElapsedTimer timer;timer.start();
    while(!overview.ready()&&timer.elapsed()<limit){app.processEvents(QEventLoop::AllEvents,5);QThread::msleep(1);}
    app.processEvents();require(overview.ready(),"Overview calculation timed out");
}
int main(int argc,char**argv){
    QApplication app(argc,argv);
    try{
        Document d;std::vector<Record> records;
        for(int row=0;row<24;++row){
            std::string s(5000,'A');for(int c=1000;c<2000;++c)s[c]=row<12?'~':'A';
            for(int c=2000;c<3000;++c)s[c]="ACGT"[row%4];for(int c=3000;c<4000;++c)s[c]='N';
            for(int c=4000;c<5000;++c)s[c]=row%2?'~':'-';records.push_back({"sequence_"+std::to_string(row+1),s});
        }
        d.load(std::move(records));QWidget window;QVBoxLayout layout(&window);layout.setContentsMargins(0,0,0,0);
        AlignmentView view;AlignmentOverview overview(&view);layout.addWidget(&view);layout.addWidget(&overview);
        window.resize(1180,720);view.set_document(&d);overview.set_document(&d);view.viewport_changed=[&]{overview.update();};window.show();
        wait_ready(app,overview);const auto& bins=overview.displayed_bins();require(!bins.empty(),"Overview bins");
        const QFontMetrics metrics(view.font(),view.viewport());const int cw=std::max(4,metrics.horizontalAdvance('M')+2),rh=view.ruler_height();
        const auto ruler=view.viewport()->grab().toImage();const auto dpr=ruler.devicePixelRatio();
        for(int col=0;col<10;++col){const int length=(col+1)%10==0?8:(col+1)%5==0?5:3;const auto x=230+col*cw+cw/2;
            const auto at=[&](int y){return ruler.pixelColor(qRound(x*dpr),qRound(y*dpr));};
            require(at(rh-1)==QColor(90,95,105)&&at(rh-length)==QColor(90,95,105)&&at(rh-length-1)!=QColor(90,95,105),"Ruler tick heights 1/5/10");
        }
        bool ungapped=false,half=false,variable=false,unknown=false,allgap=false;
        for(const auto& b:bins){
            if(b.end<=1000){require(b.residues==1&&b.conservation==1,"Ungapped conserved columns");ungapped=true;}
            if(b.begin>=1000&&b.end<=2000){require(b.residues==.5&&b.conservation==1,"Half gaps");half=true;}
            if(b.begin>=2000&&b.end<=3000){require(b.residues==1&&b.conservation==.25,"Variable region");variable=true;}
            if(b.begin>=3000&&b.end<=4000){require(b.residues==1&&b.conservation==0&&!b.hasInformation,"Ambiguous region");unknown=true;}
            if(b.begin>=4000){require(b.residues==0&&b.conservation==0&&!b.hasInformation,"All-gap region");allgap=true;}
        }
        require(ungapped&&half&&variable&&unknown&&allgap,"All overview bands sampled");
        const auto plot=overview.grab().toImage();const auto ratio=plot.devicePixelRatio();
        auto pixel=[&](int column,int y){const int x=10+column*(overview.width()-20)/5000;return plot.pixelColor(qRound(x*ratio),qRound(y*ratio));};
        require(pixel(500,36)==QColor(38,93,151),"Conserved residue column is full-height blue");
        require(pixel(1500,36)==QColor(246,247,249)&&pixel(1500,66)==QColor(38,93,151),"Half-residue bar height");
        require(pixel(2500,36)==QColor(255,128,0),"Variable residues are bright orange");
        require(pixel(3500,36)==QColor(166,174,181),"All N is gray at full height");
        require(pixel(4500,66)==QColor(246,247,249)&&pixel(4500,86)==QColor(166,174,181),"All gaps have gray baseline and no bar");
        const auto* cached=bins.data();d.rename({{d.rows()[0].id,"renamed"}});view.refresh();overview.refresh();require(overview.ready()&&overview.displayed_bins().data()==cached,"Rename reuses statistics");
        const auto mouse=[&](QEvent::Type type,QPointF p,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent event(type,p,QPointF(overview.mapToGlobal(p.toPoint())),button,buttons,Qt::NoModifier);QApplication::sendEvent(&overview,&event);app.processEvents();};
        mouse(QEvent::MouseButtonPress,QPointF(overview.width()-11,50),Qt::LeftButton,Qt::LeftButton);require(view.horizontalScrollBar()->value()==view.horizontalScrollBar()->maximum(),"Click right navigates to end");
        mouse(QEvent::MouseMove,QPointF(10,50),Qt::NoButton,Qt::LeftButton);require(view.horizontalScrollBar()->value()==0,"Drag left navigates to start");
        mouse(QEvent::MouseButtonRelease,QPointF(10,50),Qt::LeftButton,Qt::NoButton);
        mouse(QEvent::MouseButtonPress,QPointF(overview.width()/2,50),Qt::LeftButton,Qt::LeftButton);auto [begin,end]=view.visible_column_range();require(begin<=2500&&end>2500,"Click centers visible range");
        d.replace(d.rows()[0].id,5000,0,"A");view.refresh();overview.refresh();require(!overview.ready(),"Edit invalidates plot");wait_ready(app,overview);require(overview.displayed_bins().back().end==5001,"Growth refresh");
        d.undo();view.refresh();overview.refresh();wait_ready(app,overview);require(overview.displayed_bins().back().end==5000,"Undo refresh");
        d.replace(d.rows()[0].id,0,1,"C");overview.refresh();d.replace(d.rows()[0].id,0,1,"G");overview.refresh();wait_ready(app,overview);require(overview.displayed_bins()[0].conservation<1,"Rapid edits publish latest statistics");
        window.resize(900,720);app.processEvents();require(overview.ready()&&overview.displayed_bins().size()==static_cast<std::size_t>(overview.width()-20),"Resize rebins cached columns");
        window.resize(1180,720);app.processEvents();view.navigate_to(0);
        if(argc>1)require(window.grab().save(QString::fromLocal8Bit(argv[1])),"Overview screenshot");
        Document palette;std::vector<Record> paletteRows;
        for(int row=0;row<20;++row){std::string s="AAAAAAANN~";s[1]=row<19?'A':'C';s[2]=row<16?'A':'C';s[3]=row<14?'A':'C';s[4]=row<10?'A':'C';s[5]="ACGT"[row%4];s[6]=row==0?'A':'N';paletteRows.push_back({std::to_string(row),s});}
        palette.load(std::move(paletteRows));view.set_document(&palette);overview.set_document(&palette);wait_ready(app,overview);
        const auto paletteImage=overview.grab().toImage();const auto pdpr=paletteImage.devicePixelRatio();
        const QColor expected[]={QColor(38,93,151),QColor(77,141,182),QColor(140,178,180),QColor(239,181,78),QColor(255,128,0),QColor(255,128,0),QColor(38,93,151),QColor(166,174,181),QColor(166,174,181)};
        for(int col=0;col<9;++col){const int x=10+static_cast<int>((col+.5)*(overview.width()-20)/10);require(paletteImage.pixelColor(qRound(x*pdpr),qRound(86*pdpr))==expected[col],"Discrete conservation steps and N neutrality");}
        Document partial;partial.load({{"one","ARRBN~"},{"two","AGRN~-"},{"mask","CCGT01"}});
        view.set_document(&partial);overview.set_document(&partial);wait_ready(app,overview);
        const auto& partialBins=overview.displayed_bins();require(partialBins.size()==6&&partialBins[0].residues==1&&partialBins[1].conservation==.75&&partialBins[2].conservation==.5&&std::abs(partialBins[3].conservation-1.0/3)<1e-12,"Partial IUPAC contributes and masks do not");
        const auto partialImage=overview.grab().toImage();const auto partialDpr=partialImage.devicePixelRatio();
        auto partialPixel=[&](int col,int y){const int x=10+static_cast<int>((col+.5)*(overview.width()-20)/6);return partialImage.pixelColor(qRound(x*partialDpr),qRound(y*partialDpr));};
        require(partialPixel(0,36)==QColor(38,93,151)&&partialPixel(1,36)==QColor(140,178,180)&&partialPixel(2,36)==QColor(255,128,0)&&partialPixel(3,36)==QColor(255,128,0),"Partial IUPAC overview colors");
        require(partialPixel(4,36)==QColor(246,247,249)&&partialPixel(4,66)==QColor(166,174,181)&&partialPixel(5,66)==QColor(246,247,249),"Masks excluded from residue height denominator");
        Document replacement;replacement.load({{"short","ACGT"}});view.set_document(&replacement);overview.set_document(&replacement);wait_ready(app,overview);require(overview.displayed_bins().size()==4,"Document replacement");
        overview.set_document(nullptr);app.processEvents();require(!overview.ready(),"Cleared overview");
        if(argc>3){
            const auto rows=std::stoull(argv[2]),columns=std::stoull(argv[3]);require(rows>0&&columns>0,"Benchmark dimensions");
            Document large;std::vector<Record> input;input.reserve(rows);
            for(std::size_t r=0;r<rows;++r){std::string s(columns,'A');for(auto c=r%17;c<columns;c+=17)s[c]='~';input.push_back({std::to_string(r),std::move(s)});}large.load(std::move(input));
            view.set_document(&large);overview.set_document(&large);QElapsedTimer elapsed;elapsed.start();qint64 previous=0,maxLag=0;std::size_t ticks=0;
            QTimer heartbeat;heartbeat.setInterval(1);QObject::connect(&heartbeat,&QTimer::timeout,[&]{const auto now=elapsed.nsecsElapsed();maxLag=std::max(maxLag,now-previous);previous=now;++ticks;view.horizontalScrollBar()->setValue(static_cast<int>(ticks%100));});heartbeat.start();
            wait_ready(app,overview,60000);heartbeat.stop();require(ticks>10,"GUI processes heartbeat during calculation");
            std::cout<<"Overview "<<rows<<" x "<<columns<<": "<<elapsed.elapsed()<<" ms, heartbeat ticks "<<ticks<<", max timer gap "<<maxLag/1e6<<" ms\n";
            overview.set_document(nullptr);view.set_document(nullptr);
        }
        view.viewport_changed={};overview.set_document(nullptr);view.set_document(nullptr);
        std::cout<<"PASS overview: statistics, resize, cached rename, click/drag navigation, edits/growth/undo, rapid edits, document replacement\n";
    }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
