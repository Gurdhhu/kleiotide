#include "gui/alignment_overview.hpp"
#include "gui/alignment_view.hpp"
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <algorithm>
#include <cmath>
#include <limits>
namespace kleiotide {
namespace {
const QString colorExplanation=QStringLiteral("Color: conserved column (blue) -> variable (orange), gray = no information");
QColor color(const OverviewBin& bin){
    if(!bin.hasInformation)return QColor(166,174,181);
    // Majority fraction: 25% (four equally frequent bases) through 100%.
    if(bin.conservation>=1.0-1e-12)return QColor(38,93,151);
    if(bin.conservation>=.9)return QColor(77,141,182);
    if(bin.conservation>=.75)return QColor(140,178,180);
    if(bin.conservation>=.6)return QColor(239,181,78);
    return QColor(255,128,0);
}
}
AlignmentOverview::AlignmentOverview(AlignmentView* view,QWidget* parent):QWidget(parent),view_(view){
    setToolTip("Height = % non-gap symbols; "+colorExplanation+". Nucleotide rows only; proteins and masks are excluded.");
    setObjectName("alignmentOverview");setFixedHeight(112);setMouseTracking(true);
    debounce_.setSingleShot(true);debounce_.setInterval(150);work_.setInterval(4);
    connect(&debounce_,&QTimer::timeout,this,[this]{begin_scan();});
    connect(&work_,&QTimer::timeout,this,[this]{advance_scan();});
}
void AlignmentOverview::set_document(const Document* document){
    if(document_!=document){work_.stop();debounce_.stop();document_=document;requested_=std::numeric_limits<std::uint64_t>::max();scan_.reset();bins_.clear();plot_=QPixmap{};}
    refresh();
}
void AlignmentOverview::refresh(){
    if(!document_)return;
    if(requested_!=document_->content_id()){
        requested_=document_->content_id();work_.stop();scan_.reset();bins_.clear();plot_=QPixmap{};error_.clear();debounce_.start();
    }
    update();
}
bool AlignmentOverview::ready()const{return document_&&scan_&&scan_->complete()&&scan_->revision()==document_->content_id();}
void AlignmentOverview::begin_scan(){
    if(!document_)return;
    try{scan_=std::make_unique<OverviewScan>(*document_);work_.start();advance_scan();}
    catch(const std::exception&e){error_=QString::fromUtf8(e.what());work_.stop();update();}
}
void AlignmentOverview::advance_scan(){
    if(!document_||!scan_)return;
    if(scan_->revision()!=document_->content_id()){refresh();return;}
    try{
        QElapsedTimer timer;timer.start();
        do{if(scan_->step(*document_,4096)){work_.stop();rebuild_plot();break;}}while(timer.nsecsElapsed()<2000000);
    }catch(const std::exception&e){work_.stop();error_=QString::fromUtf8(e.what());}
    update();
}
QRect AlignmentOverview::plot_rect()const{return QRect(10,26,std::max(1,width()-20),65);}
void AlignmentOverview::rebuild_plot(){
    if(!ready())return;
    const auto rect=plot_rect();bins_=scan_->bins(static_cast<std::size_t>(rect.width()));dpr_=devicePixelRatioF();
    plot_=QPixmap(static_cast<int>(std::ceil(rect.width()*dpr_)),static_cast<int>(std::ceil(rect.height()*dpr_)));plot_.setDevicePixelRatio(dpr_);plot_.fill(QColor(246,247,249));
    QPainter p(&plot_);
    for(std::size_t i=0;i<bins_.size();++i){
        const int x=static_cast<int>(i*static_cast<std::size_t>(rect.width())/bins_.size()),end=static_cast<int>((i+1)*static_cast<std::size_t>(rect.width())/bins_.size());
        const int height=qRound(bins_[i].residues*54);const auto c=color(bins_[i]);
        if(height)p.fillRect(x,54-height,std::max(1,end-x),height,c);
        p.fillRect(x,58,std::max(1,end-x),6,c); // Gray baseline also identifies empty columns.
    }
}
void AlignmentOverview::resizeEvent(QResizeEvent*event){QWidget::resizeEvent(event);rebuild_plot();}
void AlignmentOverview::paintEvent(QPaintEvent*){
    QPainter p(this);p.fillRect(rect(),QColor(250,251,252));
    auto f=font();f.setPointSize(9);p.setFont(f);p.setPen(QColor(55,64,73));
    p.drawText(QRect(10,2,width()-20,20),Qt::AlignVCenter,"Alignment overview · Height = % non-gap symbols · "+colorExplanation);
    const auto area=plot_rect();p.fillRect(area,QColor(239,241,244));
    if(ready()){
        if(!qFuzzyCompare(dpr_,devicePixelRatioF()))rebuild_plot();
        p.drawPixmap(area.topLeft(),plot_);
    }else if(document_&&document_->columns()){
        const auto text=error_.isEmpty()?QString("Calculating overview… %1%").arg(scan_?qRound(scan_->progress()*100):0):"Overview unavailable: "+error_;
        p.drawText(area,Qt::AlignCenter,text);
    }else p.drawText(area,Qt::AlignCenter,"Open sequences to see the alignment overview");
    if(document_&&document_->columns()){
        const auto [begin,end]=view_->visible_column_range();const double scale=static_cast<double>(area.width())/document_->columns();
        const int x=area.left()+static_cast<int>(begin*scale),finish=area.left()+static_cast<int>(std::ceil(end*scale));
        p.setPen(QPen(QColor(25,44,66),2));p.drawRect(QRect(x,area.top(),std::max(4,finish-x),area.height()).adjusted(1,1,-1,-1));
        p.setPen(QColor(55,64,73));p.drawText(QRect(10,92,width()-20,18),Qt::AlignLeft|Qt::AlignVCenter,QString("1  ·  Click or drag to navigate  ·  Viewing %1–%2").arg(static_cast<qulonglong>(begin+1)).arg(static_cast<qulonglong>(end)));
        p.drawText(QRect(10,92,width()-20,18),Qt::AlignRight|Qt::AlignVCenter,QString::number(static_cast<qulonglong>(document_->columns())));
    }
}
void AlignmentOverview::navigate(const QPoint& point){
    if(!document_||!document_->columns())return;
    const auto area=plot_rect();const double fraction=static_cast<double>(std::clamp(point.x()-area.left(),0,area.width()-1))/std::max(1,area.width()-1);
    view_->navigate_to(static_cast<std::size_t>(fraction*(document_->columns()-1)));update();
}
void AlignmentOverview::mousePressEvent(QMouseEvent*event){if(event->button()==Qt::LeftButton&&plot_rect().contains(event->position().toPoint())){navigate(event->position().toPoint());event->accept();}else QWidget::mousePressEvent(event);}
void AlignmentOverview::mouseMoveEvent(QMouseEvent*event){
    if(event->buttons().testFlag(Qt::LeftButton)){navigate(event->position().toPoint());event->accept();return;}
    const auto point=event->position().toPoint();const auto area=plot_rect();
    if(ready()&&area.contains(point)&&!bins_.empty()){
        const auto index=std::min(bins_.size()-1,static_cast<std::size_t>(point.x()-area.left())*bins_.size()/static_cast<std::size_t>(area.width()));const auto&bin=bins_[index];
        const auto conservation=bin.hasInformation?QString("%1%").arg(bin.conservation*100,0,'f',1):QString("No information");
        QToolTip::showText(event->globalPosition().toPoint(),QString("Columns %1–%2\nNon-gap symbols: %3%\nColumn conservation: %4\nIUPAC calls vote equally for their possible bases.\nN, gaps and unknown ? do not vote; protein and mask rows are excluded.")
            .arg(static_cast<qulonglong>(bin.begin+1)).arg(static_cast<qulonglong>(bin.end)).arg(bin.residues*100,0,'f',1).arg(conservation),this);
    }else QToolTip::hideText();
}
}
