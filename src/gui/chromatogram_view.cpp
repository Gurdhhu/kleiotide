#include "gui/chromatogram_view.hpp"
#include <QComboBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
namespace kleiotide {
namespace {
QString position_text(const TracePosition& position,std::size_t column) {
    auto text=QString("Column %1 · %2").arg(static_cast<qulonglong>(column+1)).arg(QChar(position.current));
    if(position.originalCall){
        text+=QString(" · imported call %1 (%2)").arg(static_cast<qulonglong>(*position.originalCall+1)).arg(QChar(position.imported));
        if(position.edited)text+=" · substituted; original signal retained; no quality assigned to substitution";
        else if(position.quality)text+=QString(" · imported Q=%1").arg(*position.quality);
        else text+=" · quality unavailable";
    }else if(is_gap(position.current))text+=" · gap; no original peak or quality";
    else text+=" · inserted symbol; interpolated display anchor; no original peak or quality";
    return text;
}
}
TraceCanvas::TraceCanvas(QWidget* parent):QAbstractScrollArea(parent) {
    setObjectName("traceCanvas");setMinimumHeight(145);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setMouseTracking(true);setFocusPolicy(Qt::ClickFocus);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    connect(horizontalScrollBar(),&QScrollBar::valueChanged,this,[this]{invalidate_render_cache();});
}
double TraceCanvas::raw_from_oriented(double sample)const {return row_.trace->reversed?static_cast<double>(row_.trace->original->channels[0].size()-1)-sample:sample;}
double TraceCanvas::displayed(double sample)const {return aligned_?coordinates_->column_for_sample(sample):raw_from_oriented(sample);}
double TraceCanvas::original(double coordinate)const {return aligned_?coordinates_->sample_for_column(coordinate):raw_from_oriented(coordinate);}
double TraceCanvas::sample_x(double sample)const {return (displayed(sample)-origin())*pixels_per_unit()+80.-horizontalScrollBar()->value();}
double TraceCanvas::column_x(std::size_t column)const {return aligned_?(static_cast<double>(column)+.5-origin())*pixels_per_unit()+80.-horizontalScrollBar()->value():sample_x(position(column).sample);}
int TraceCanvas::label_gutter()const {auto font=this->font();font.setPointSize(9);return QFontMetrics(font).horizontalAdvance("Original Q:")+12;}
void TraceCanvas::set_alignment_spacing(bool enabled){if(aligned_==enabled)return;aligned_=enabled;update_scrollbar();center_column(column_);invalidate_render_cache();}
void TraceCanvas::set_alignment_columns(std::size_t columns){if(alignmentColumns_==columns)return;alignmentColumns_=columns;update_scrollbar();}
void TraceCanvas::set_selection(std::size_t start,std::size_t width,bool blue){if(selectionStart_==start&&selectionWidth_==width&&blue_==blue)return;selectionStart_=start;selectionWidth_=width;blue_=blue;viewport()->update(QRect(0,0,viewport()->width(),18));}
void TraceCanvas::set_sequence(const Sequence* row) {
    if(!row||!row->trace){active_=false;row_=Sequence{};coordinates_.reset();currentColumns_.clear();currentSamples_.clear();anchorOrdinals_.clear();originalColumns_.clear();dragAnchor_.reset();pickedCall_.reset();update_scrollbar();invalidate_render_cache();return;}
    if(active_&&row_.id==row->id&&row_.trace==row->trace&&row_.symbols.text()==row->symbols.text())return;
    const bool newSignal=!active_||row_.trace->original!=row->trace->original;
    const bool newRow=!active_||row_.id!=row->id;
    row_=*row;active_=true;coordinates_.emplace(row_);currentColumns_.clear();
    rebuild_positions();
    pickedCall_.reset();column_=newRow?0:std::min(column_,row_.symbols.size());
    if(newSignal){maximum_=1.;for(const auto& channel:row_.trace->original->channels)for(auto value:channel)maximum_=std::max(maximum_,static_cast<double>(value));}
    update_scrollbar();if(newRow)center_column(column_);invalidate_render_cache();
}
void TraceCanvas::rebuild_positions(){
    currentColumns_.clear();anchorOrdinals_.clear();
    const auto& link=*row_.trace;const auto& evidence=*link.original;
    originalColumns_.assign(evidence.calls.size(),{});currentSamples_.resize(link.originalCalls.size());
    for(std::size_t c=0;c<row_.symbols.size();++c)if(!is_gap(row_.symbols.text()[c])){
        const auto ordinal=currentColumns_.size();currentColumns_.push_back(c);const auto call=link.originalCalls[ordinal];
        if(call>=0){anchorOrdinals_.push_back(ordinal);originalColumns_[static_cast<std::size_t>(call)]=c;currentSamples_[ordinal]=evidence.peaks[static_cast<std::size_t>(call)];}
    }
    const double direction=link.reversed?-1.:1.;const auto& calls=link.originalCalls;
    // Same interpolation as trace_position(), once per synthetic run instead
    // of rescanning residue prefixes and neighboring calls for every label.
    for(std::size_t first=0;first<calls.size();){
        if(calls[first]>=0){++first;continue;}auto end=first+1;while(end<calls.size()&&calls[end]<0)++end;
        const double lo=first?evidence.peaks[static_cast<std::size_t>(calls[first-1])]:end<calls.size()?evidence.peaks[static_cast<std::size_t>(calls[end])]-direction*12.*static_cast<double>(end-first+1):0.;
        const double hi=end<calls.size()?evidence.peaks[static_cast<std::size_t>(calls[end])]:lo+direction*12.*static_cast<double>(end-first+2);
        for(auto i=first;i<end;++i){const double fraction=static_cast<double>(i-first+1)/static_cast<double>(end-first+1);currentSamples_[i]=std::clamp(lo+(hi-lo)*fraction,0.,static_cast<double>(evidence.channels[0].size()-1));}
        first=end;
    }
}
TracePosition TraceCanvas::position(std::size_t column)const {
    TracePosition result;if(!active_)return result;column=std::min(column,row_.symbols.size());
    const auto& link=*row_.trace;const auto& evidence=*link.original;
    const auto ordinal=static_cast<std::size_t>(std::lower_bound(currentColumns_.begin(),currentColumns_.end(),column)-currentColumns_.begin());
    result.current=column<row_.symbols.size()?row_.symbols.text()[column]:'~';
    if(!is_gap(result.current)){
        result.sample=currentSamples_[ordinal];const auto call=link.originalCalls[ordinal];
        if(call>=0){const auto index=static_cast<std::size_t>(call);result.originalCall=index;result.imported=link.reversed?trace_complement(evidence.calls[index]):evidence.calls[index];
            result.edited=std::toupper(static_cast<unsigned char>(result.current))!=std::toupper(static_cast<unsigned char>(result.imported));
            if(!result.edited&&!evidence.quality.empty())result.quality=evidence.quality[index];}
        return result;
    }
    const auto next=std::lower_bound(anchorOrdinals_.begin(),anchorOrdinals_.end(),ordinal);
    const auto left=next==anchorOrdinals_.begin()?0:*(next-1)+1;
    const auto right=next==anchorOrdinals_.end()?link.originalCalls.size():*next;
    const double direction=link.reversed?-1.:1.;
    const double lo=left?evidence.peaks[static_cast<std::size_t>(link.originalCalls[left-1])]:right<link.originalCalls.size()?evidence.peaks[static_cast<std::size_t>(link.originalCalls[right])]-direction*12.*static_cast<double>(right-left+1):0.;
    const double hi=right<link.originalCalls.size()?evidence.peaks[static_cast<std::size_t>(link.originalCalls[right])]:lo+direction*12.*static_cast<double>(right-left+2);
    result.sample=std::clamp(lo+(hi-lo)*.5,0.,static_cast<double>(evidence.channels[0].size()-1));return result;
}
void TraceCanvas::update_scrollbar() {
    const auto units=active_?(aligned_?static_cast<double>(std::max(alignmentColumns_,row_.symbols.size()))+1.5:static_cast<double>(row_.trace->original->channels[0].size())):0.;
    const int width=viewport()->width();
    horizontalScrollBar()->setPageStep(std::max(1,width));horizontalScrollBar()->setSingleStep(40);
    horizontalScrollBar()->setRange(0,std::max(0,static_cast<int>(std::min<double>(std::numeric_limits<int>::max(),std::ceil(units*pixels_per_unit()+80.)))-width));
}
void TraceCanvas::scroll_to(double position){horizontalScrollBar()->setValue(static_cast<int>(std::clamp(position,0.,static_cast<double>(horizontalScrollBar()->maximum()))));}
void TraceCanvas::center_column(std::size_t column) {
    if(!active_)return;
    scroll_to(horizontalScrollBar()->value()+column_x(column)-viewport()->width()/2.);
}
void TraceCanvas::focus_column(std::size_t column,bool center) {
    if(column_==column&&!pickedCall_){if(center)center_column(column);return;}
    if(!interacting_&&column_!=column)selectionOrigin_=column;
    column_=column;pickedCall_.reset();if(center)center_column(column);viewport()->update();
}
void TraceCanvas::set_zoom(int percent) {
    const double center=(horizontalScrollBar()->value()+viewport()->width()/2.-80.)/pixels_per_unit()+origin();
    zoom_=std::clamp(percent,25,400)/100.;update_scrollbar();
    scroll_to((center-origin())*pixels_per_unit()+80.-viewport()->width()/2.);invalidate_render_cache();
}
void TraceCanvas::set_gain(int percent) {gain_=std::clamp(percent,50,400)/100.;invalidate_render_cache();}
std::pair<double,double> TraceCanvas::visible_samples()const {
    if(!active_)return {0.,0.};
    const auto a=(horizontalScrollBar()->value()-80.)/pixels_per_unit()+origin(),b=(horizontalScrollBar()->value()+viewport()->width()-80.)/pixels_per_unit()+origin();
    return {std::max(0.,raw_from_oriented(original(a))),std::max(0.,raw_from_oriented(original(b)))};
}
void TraceCanvas::resizeEvent(QResizeEvent* event) {QAbstractScrollArea::resizeEvent(event);update_scrollbar();invalidate_render_cache();}
void TraceCanvas::changeEvent(QEvent* event){QAbstractScrollArea::changeEvent(event);if(event->type()==QEvent::FontChange)invalidate_render_cache();}
void TraceCanvas::invalidate_render_cache(){renderDirty_=true;viewport()->update();}
void TraceCanvas::render_trace_layers() {
    const auto dpr=viewport()->devicePixelRatioF();
    traceLayer_=QPixmap(qCeil(viewport()->width()*dpr),qCeil(viewport()->height()*dpr));traceLayer_.setDevicePixelRatio(dpr);traceLayer_.fill(Qt::white);
    currentLayer_=QPixmap(qCeil(viewport()->width()*dpr),qCeil(18*dpr));currentLayer_.setDevicePixelRatio(dpr);currentLayer_.fill(Qt::transparent);
    renderDirty_=false;
    QPainter painter(&traceLayer_);painter.setFont(font());
    if(!active_){painter.setPen(Qt::darkGray);painter.drawText(viewport()->rect(),Qt::AlignCenter,"Open or add an AB1 file to view its chromatogram.");return;}
    painter.setRenderHint(QPainter::Antialiasing);
    const auto& traceData=*row_.trace->original;const bool reverse=row_.trace->reversed;
    const auto count=traceData.channels[0].size();const auto [first,last]=visible_samples();
    const int baseline=viewport()->height()-20,top=50;
    const double scale=std::max(1,baseline-top)/maximum_*gain_;
    auto x=[&](double sample){return sample_x(sample);};
    painter.setPen(QColor(225,225,225));painter.drawLine(0,baseline,viewport()->width(),baseline);
    const std::size_t begin=static_cast<std::size_t>(std::max(0.,std::floor(first)-1.));
    const std::size_t end=std::min(count,static_cast<std::size_t>(std::max(0.,std::ceil(last)+1.)));
    painter.save();painter.setClipRect(0,top,viewport()->width(),std::max(1,baseline-top));
    for(std::size_t channel=0;channel<4;++channel){
        const std::size_t signal=reverse?3-channel:channel; // A<->T and C<->G.
        const auto& values=traceData.channels[signal];painter.setPen(QPen(residue_ink("ACGT"[channel],palette_),1.1));
        if(aligned_){
            // Inverse-map each display pixel. Keep every original extremum in its interval.
            auto value=[&](double oriented){const auto a=static_cast<std::size_t>(std::floor(oriented)),b=std::min(a+1,count-1);const auto fraction=oriented-static_cast<double>(a);
                return values[reverse?count-1-a:a]*(1.-fraction)+values[reverse?count-1-b:b]*fraction;};
            QPainterPath path;bool started=false;
            for(int pixel=0;pixel<viewport()->width();++pixel){
                const auto left=(horizontalScrollBar()->value()+pixel-80.)/pixels_per_unit()+origin();
                double a=raw_from_oriented(original(left)),b=raw_from_oriented(original(left+1./pixels_per_unit()));
                if(b<0.||a>static_cast<double>(count-1))continue;
                a=std::max(0.,a);b=std::min(static_cast<double>(count-1),b);
                const auto va=value(a),vb=value(b);double lo=std::min(va,vb),hi=std::max(va,vb);
                for(auto sample=static_cast<std::size_t>(std::ceil(a));sample<=static_cast<std::size_t>(std::floor(b));++sample){const auto v=values[reverse?count-1-sample:sample];lo=std::min(lo,static_cast<double>(v));hi=std::max(hi,static_cast<double>(v));}
                painter.drawLine(QPointF(pixel,baseline-hi*scale),QPointF(pixel,baseline-lo*scale));
                const QPointF point(pixel,baseline-vb*scale);if(!started){path.moveTo(point);started=true;}else path.lineTo(point);
            }painter.drawPath(path);
        }else if(zoom_>=1.){
            QPainterPath path;bool started=false;
            for(std::size_t display=begin;display<end;++display){const auto sample=reverse?count-1-display:display;
                const QPointF point(static_cast<double>(display)*zoom_+80.-horizontalScrollBar()->value(),baseline-values[sample]*scale);
                if(!started){path.moveTo(point);started=true;}else path.lineTo(point);
            }painter.drawPath(path);
        }else{
            // A min/max envelope retains narrow peaks when several samples share a pixel.
            for(int pixel=0;pixel<viewport()->width();++pixel){
                const auto a=std::min(count,static_cast<std::size_t>(std::max(0.,(horizontalScrollBar()->value()+pixel-80.)/zoom_)));
                const auto b=std::min(count,std::max(a+1,static_cast<std::size_t>(std::max(0.,std::ceil((horizontalScrollBar()->value()+pixel+1-80.)/zoom_)))));
                if(a>=b)continue;
                std::int32_t lo=values[reverse?count-1-a:a],hi=lo;
                for(auto d=a+1;d<b;++d){const auto value=values[reverse?count-1-d:d];lo=std::min(lo,value);hi=std::max(hi,value);}
                painter.drawLine(QPointF(pixel,baseline-hi*scale),QPointF(pixel,baseline-lo*scale));
            }
        }
    }painter.restore();
    const auto& columns=originalColumns_;
    const auto rawA=raw_from_oriented(first),rawB=raw_from_oriented(last);
    auto it=std::lower_bound(traceData.peaks.begin(),traceData.peaks.end(),std::min(rawA,rawB));
    auto font=painter.font();font.setPointSize(9);painter.setFont(font);double previousQuality=-100.;
    // Every visible imported call is drawn, even if adjacent letters overlap.
    const auto start=static_cast<std::size_t>(it-traceData.peaks.begin());
    const auto finish=static_cast<std::size_t>(std::upper_bound(traceData.peaks.begin(),traceData.peaks.end(),std::max(rawA,rawB))-traceData.peaks.begin());
    for(std::size_t k=start;k<finish;++k){const auto call=reverse?finish-1-(k-start):k;const double px=x(traceData.peaks[call]);
        const char imported=reverse?trace_complement(traceData.calls[call]):traceData.calls[call];
        painter.setPen(columns[call]?QColor(75,75,75):QColor(160,160,160));
        painter.drawText(QPointF(px-4,30),QString(QChar(imported)));
        if(!columns[call])painter.drawLine(QPointF(px-5,24),QPointF(px+5,24));
        if(!traceData.quality.empty()&&px-5>=previousQuality+4.){const auto quality=QString::number(traceData.quality[call]);previousQuality=px-5+QFontMetrics(font).horizontalAdvance(quality);painter.setPen(Qt::gray);painter.drawText(QPointF(px-5,45),quality);}
    }
    QPainter current(&currentLayer_);current.setFont(font);current.setRenderHint(QPainter::Antialiasing);
    for(const auto& label:current_labels()){
        const auto px=aligned_?column_x(label.column):sample_x(label.sample);current.setPen(label.edited?QColor(155,50,145):Qt::black);current.drawText(QPointF(px-4,12),QString(QChar(label.symbol)));
        if(label.edited)current.drawLine(QPointF(px-4,15),QPointF(px+4,15));
    }
    painter.drawText(5,viewport()->height()-4,QString("%1 · %2 strand").arg(aligned_?"Alignment columns (display warp)":"Original sample coordinates").arg(reverse?"reverse-complement":"forward"));
}
void TraceCanvas::paintEvent(QPaintEvent*) {
    const auto dpr=viewport()->devicePixelRatioF();
    if(renderDirty_||traceLayer_.size()!=QSize(qCeil(viewport()->width()*dpr),qCeil(viewport()->height()*dpr))||!qFuzzyCompare(traceLayer_.devicePixelRatioF(),dpr))render_trace_layers();
    QPainter painter(viewport());painter.drawPixmap(0,0,traceLayer_);if(!active_)return;
    painter.setRenderHint(QPainter::Antialiasing);auto labelFont=font();labelFont.setPointSize(9);painter.setFont(labelFont);
    if(selectionWidth_){const auto a=column_x(selectionStart_),b=column_x(selectionStart_+selectionWidth_-1);painter.fillRect(QRectF(std::min(a,b)-6,0,std::max(12.,std::abs(b-a)+12.),17),blue_?QColor(225,235,251):QColor(250,235,191));}
    painter.drawPixmap(0,0,currentLayer_);
    const auto focus=position(column_);const double px=pickedCall_?sample_x(row_.trace->original->peaks[*pickedCall_]):column_x(column_);
    painter.setPen(QPen(QColor(20,140,180),1.5,focus.originalCall||pickedCall_?Qt::SolidLine:Qt::DashLine));
    painter.drawLine(QPointF(px,0),QPointF(px,viewport()->height()-20));
    // Sticky row labels sit above residues, quality and caret graphics.
    painter.fillRect(QRect(0,0,label_gutter(),50),Qt::white);painter.setPen(Qt::darkGray);
    painter.drawText(5,12,"Current:");painter.drawText(5,30,"Imported:");painter.drawText(5,45,"Original Q:");
}
std::vector<TraceCanvas::Label> TraceCanvas::current_labels()const {
    std::vector<Label> labels;if(!active_)return labels;
    const auto& link=*row_.trace;
    auto add=[&](std::size_t c,std::size_t ordinal,bool gap){const auto call=gap?-1:link.originalCalls[ordinal];const auto sample=gap?position(c).sample:currentSamples_[ordinal];const auto px=aligned_?column_x(c):sample_x(sample);
        if(px>=-5&&px<=viewport()->width()+5){const auto symbol=row_.symbols.text()[c];const bool inserted=!gap&&call<0;bool edited=inserted;
            if(!gap&&call>=0){const auto imported=link.reversed?trace_complement(link.original->calls[static_cast<std::size_t>(call)]):link.original->calls[static_cast<std::size_t>(call)];edited=std::toupper(static_cast<unsigned char>(symbol))!=std::toupper(static_cast<unsigned char>(imported));}
            labels.push_back({c,sample,symbol,inserted,edited});}};
    if(aligned_){const auto first=static_cast<std::size_t>(std::clamp(std::floor((horizontalScrollBar()->value()-85.)/pixels_per_unit()+origin()),0.,static_cast<double>(row_.symbols.size())));
        const auto last=static_cast<std::size_t>(std::clamp(std::ceil((horizontalScrollBar()->value()+viewport()->width()-75.)/pixels_per_unit()+origin())+1.,0.,static_cast<double>(row_.symbols.size())));
        auto ordinal=static_cast<std::size_t>(std::lower_bound(currentColumns_.begin(),currentColumns_.end(),first)-currentColumns_.begin());
        for(auto c=first;c<last;++c){const bool gap=is_gap(row_.symbols.text()[c]);add(c,ordinal,gap);if(!gap)++ordinal;}
    }else{
        const auto first=(horizontalScrollBar()->value()-85.)/zoom_,last=(horizontalScrollBar()->value()+viewport()->width()-75.)/zoom_;
        const auto begin=std::lower_bound(currentSamples_.begin(),currentSamples_.end(),first,[&](double sample,double value){return raw_from_oriented(sample)<value;});
        const auto end=std::upper_bound(begin,currentSamples_.end(),last,[&](double value,double sample){return value<raw_from_oriented(sample);});
        for(auto it=begin;it!=end;++it){const auto ordinal=static_cast<std::size_t>(it-currentSamples_.begin());add(currentColumns_[ordinal],ordinal,false);}
    }
    return labels;
}
void TraceCanvas::select_call(std::size_t call) {
    if(!active_)return;
    pickedCall_=call;viewport()->update();
    if(const auto column=trace_column(row_,call)){
        column_=*column;if(cell_requested)cell_requested(row_.id,*column);
    }else if(evidence_selected)evidence_selected(QString("Imported call %1 was deleted from the alignment; its original signal remains visible.").arg(static_cast<qulonglong>(call+1)));
}
void TraceCanvas::step_call(int delta) {
    if(!active_)return;
    const auto& traceData=*row_.trace->original;const auto focus=position(column_);
    auto call=pickedCall_.value_or(focus.originalCall.value_or(static_cast<std::size_t>(std::lower_bound(traceData.peaks.begin(),traceData.peaks.end(),focus.sample)-traceData.peaks.begin())));
    const auto next=std::clamp<std::int64_t>(static_cast<std::int64_t>(call)+(row_.trace->reversed?-delta:delta),0,static_cast<std::int64_t>(traceData.calls.size()-1));
    select_call(static_cast<std::size_t>(next));scroll_to(horizontalScrollBar()->value()+sample_x(traceData.peaks[static_cast<std::size_t>(next)])-viewport()->width()/2.);
}
std::optional<std::size_t> TraceCanvas::column_at(double px)const {
    if(!active_)return {};
    if(aligned_){const auto column=(horizontalScrollBar()->value()+px-80.)/pixels_per_unit()+origin();return static_cast<std::size_t>(std::clamp(std::floor(column),0.,static_cast<double>(row_.symbols.size())));}
    const auto labels=current_labels();std::optional<std::size_t> best;double distance=std::numeric_limits<double>::max();
    for(const auto& label:labels){const auto delta=std::abs(column_x(label.column)-px);if(delta<distance){best=label.column;distance=delta;}}
    return best;
}
void TraceCanvas::mousePressEvent(QMouseEvent* event) {
    if(!active_||event->button()!=Qt::LeftButton){QAbstractScrollArea::mousePressEvent(event);return;}
    if(event->position().x()<label_gutter()&&event->position().y()<50){event->accept();return;}
    interacting_=true;setFocus();
    if(event->position().y()<18){if(const auto column=column_at(event->position().x())){
        if(!event->modifiers().testFlag(Qt::ShiftModifier)||!selectionOrigin_)selectionOrigin_=*column;
        dragAnchor_=*selectionOrigin_;pickedCall_.reset();
        if(*column==row_.symbols.size()){dragAnchor_.reset();if(cell_requested)cell_requested(row_.id,*column);}
        else if(range_requested){const auto start=std::min(*column,*dragAnchor_);range_requested(row_.id,start,std::min(row_.symbols.size()-start,std::max(*column,*dragAnchor_)-start+1));}
        else if(cell_requested)cell_requested(row_.id,*column);
        event->accept();return;
    }}
    const auto& peaks=row_.trace->original->peaks;const auto sample=original((horizontalScrollBar()->value()+event->position().x()-80.)/pixels_per_unit()+origin());
    auto it=std::lower_bound(peaks.begin(),peaks.end(),sample);std::size_t call=it==peaks.end()?peaks.size()-1:static_cast<std::size_t>(it-peaks.begin());
    if(call&&std::abs(static_cast<double>(peaks[call-1])-sample)<std::abs(static_cast<double>(peaks[call])-sample))--call;
    select_call(call);interacting_=false;event->accept();
}
void TraceCanvas::mouseMoveEvent(QMouseEvent* event){
    if(dragAnchor_&&event->buttons().testFlag(Qt::LeftButton))if(const auto column=column_at(event->position().x())){
        const auto start=std::min(*column,*dragAnchor_),end=std::min(row_.symbols.size(),std::max(*column,*dragAnchor_)+1);
        if(end>start&&range_requested)range_requested(row_.id,start,end-start);
        event->accept();return;
    }QAbstractScrollArea::mouseMoveEvent(event);
}
void TraceCanvas::mouseReleaseEvent(QMouseEvent* event){dragAnchor_.reset();interacting_=false;QAbstractScrollArea::mouseReleaseEvent(event);}
void TraceCanvas::keyPressEvent(QKeyEvent* event){
    if(active_&&key_requested&&(!pickedCall_||trace_column(row_,*pickedCall_))){key_requested(event);return;}
    QAbstractScrollArea::keyPressEvent(event);
}
void TraceCanvas::wheelEvent(QWheelEvent* event) {
    horizontalScrollBar()->setValue(horizontalScrollBar()->value()-(event->angleDelta().x()?event->angleDelta().x():event->angleDelta().y()));event->accept();
}
ChromatogramPanel::ChromatogramPanel(QWidget* parent):QWidget(parent) {
    setObjectName("chromatogramPanel");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(5,3,5,3);layout->setSpacing(3);
    auto* controls=new QHBoxLayout;choices_=new QComboBox(this);choices_->setObjectName("traceSequence");choices_->setMinimumWidth(180);choices_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    controls->addWidget(new QLabel("Trace:",this));controls->addWidget(choices_,1);
    for(const auto& [label,delta]:std::array<std::pair<const char*,int>,2>{{{"‹",-1},{"›",1}}}){
        auto* button=new QPushButton(label,this);button->setMaximumWidth(30);button->setToolTip(delta<0?"Previous imported call":"Next imported call");connect(button,&QPushButton::clicked,this,[this,delta]{canvas_->step_call(delta);});controls->addWidget(button);
    }
    auto* zoom=new QSpinBox(this);zoom->setObjectName("traceZoom");zoom->setRange(25,400);zoom->setSingleStep(25);zoom->setValue(100);zoom->setSuffix("%");zoom->setToolTip("Horizontal trace zoom; original samples remain unchanged.");
    controls->addWidget(new QLabel("Zoom",this));controls->addWidget(zoom);
    auto* gain=new QSpinBox(this);gain->setObjectName("traceGain");gain->setRange(50,400);gain->setSingleStep(25);gain->setValue(100);gain->setSuffix("%");gain->setToolTip("Display amplitude gain; original signal remains unchanged.");controls->addWidget(new QLabel("Gain",this));controls->addWidget(gain);
    auto* spacing=new QComboBox(this);spacing->setObjectName("traceSpacing");spacing->addItems({"Sample spacing","Alignment spacing"});
    spacing->setToolTip("Sample spacing shows original coordinates. Alignment spacing stretches/compresses display intervals to current alignment columns; original evidence stays unchanged.");controls->addWidget(spacing);
    connect(spacing,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){canvas_->set_alignment_spacing(index==1);});
    legend_=new QLabel(this);legend_->setObjectName("traceLegend");controls->addWidget(legend_);layout->addLayout(controls);
    source_=new QLabel(this);source_->setTextFormat(Qt::PlainText);source_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);source_->setObjectName("traceSource");source_->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(source_);
    canvas_=new TraceCanvas(this);layout->addWidget(canvas_,1);
    position_=new QLabel(this);position_->setObjectName("tracePosition");position_->setWordWrap(true);layout->addWidget(position_);
    connect(zoom,qOverload<int>(&QSpinBox::valueChanged),canvas_,&TraceCanvas::set_zoom);
    connect(gain,qOverload<int>(&QSpinBox::valueChanged),canvas_,&TraceCanvas::set_gain);
    connect(choices_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{update_row();if(const auto* row=current_row()){update_position(0);if(cell_requested)cell_requested(row->id,0);}});
    canvas_->cell_requested=[this](SequenceId id,std::size_t column){update_position(column);if(cell_requested)cell_requested(id,column);};
    canvas_->range_requested=[this](SequenceId id,std::size_t start,std::size_t width){if(range_requested)range_requested(id,start,width);};
    canvas_->key_requested=[this](QKeyEvent* event){if(key_requested)key_requested(event);};
    canvas_->setToolTip("Click or drag Current to select/edit using the toolbar mode. Imported calls and Original Q are immutable. Arrows move the caret; Ctrl+Left/Right slides the selection one column; Shift+Left/Right slides it as far as possible within existing gap/lock rules in Edit.");
    canvas_->evidence_selected=[this](const QString& text){position_->setText(text);};
    set_residue_palette(ResiduePalette::Classic);
}
void ChromatogramPanel::set_residue_palette(ResiduePalette palette){
    canvas_->set_residue_palette(palette);QString legend;
    for(char base:std::string("ACGT"))legend+=QString("<span style='color:%1'>%2</span>  ").arg(residue_ink(base,palette).name()).arg(QChar(base));
    legend_->setText(legend);legend_->setToolTip(palette_label(palette));
}
bool ChromatogramPanel::has_traces()const {return choices_->count()!=0;}
const Sequence* ChromatogramPanel::current_row() const {
    if(!document_||choices_->currentIndex()<0)return nullptr;
    const auto id=choices_->currentData().toULongLong();for(const auto& row:document_->rows())if(row.id==id&&row.trace)return &row;return nullptr;
}
void ChromatogramPanel::set_document(Document* document) {
    canvas_->set_alignment_columns(document?document->columns():0);
    if(document_==document&&document&&state_==document->state_id())return;
    const auto previous=choices_->currentData().toULongLong();const bool same=document_==document;
    document_=document;state_=document?document->state_id():0;
    QSignalBlocker block(choices_);choices_->clear();traceChoices_.clear();int selected=-1;
    std::size_t rowIndex=0;
    if(document)for(const auto& row:document->rows()){++rowIndex;if(!row.trace)continue;
        const int index=choices_->count();choices_->addItem(QString("%1 · %2").arg(static_cast<qulonglong>(rowIndex)).arg(QString::fromUtf8(row.name.c_str())),QVariant::fromValue<qulonglong>(row.id));
        traceChoices_.insert(row.id,index);
        if(same&&row.id==previous)selected=index;
    }
    choices_->setCurrentIndex(selected>=0?selected:choices_->count()?0:-1);update_row();
}
void ChromatogramPanel::update_row() {
    const auto* row=current_row();canvas_->set_sequence(row);
    if(!row){source_->setText("No linked chromatograms in this alignment.");position_->clear();return;}
    const auto& traceData=*row->trace->original;
    source_->setText(QString("%1 · %2 base calls · %3 signal samples/channel · %4 · original signal")
        .arg(QFileInfo(QString::fromUtf8(traceData.sourcePath.c_str())).fileName()).arg(static_cast<qulonglong>(traceData.calls.size()))
        .arg(static_cast<qulonglong>(traceData.channels[0].size())).arg(row->trace->reversed?"reverse complement":"forward"));
    source_->setToolTip(QString::fromUtf8(traceData.sourcePath.c_str())+"\nSample: "+QString::fromUtf8(traceData.sampleName.c_str())+QString("\nImported ABIF call/peak set: %1\nSignal samples are amplitude measurements in each of the four channels.\nSave a .kleio session to retain traces; FASTA export stores sequence text only.").arg(traceData.callSet));
    update_position(canvas_->focused_column());
}
void ChromatogramPanel::update_position(std::size_t column) {
    if(current_row()){canvas_->focus_column(column,false);position_->setText(position_text(canvas_->position(column),column));}
}
void ChromatogramPanel::choose_sequence(SequenceId id) {
    const auto found=traceChoices_.constFind(id);if(found==traceChoices_.cend())return;
    QSignalBlocker block(choices_);choices_->setCurrentIndex(*found);update_row();
}
void ChromatogramPanel::follow_selection(const std::vector<SequenceId>& ids,std::optional<std::size_t> column) {
    if(ids.empty())return;
    const auto current=canvas_->sequence_id();
    if(std::find(ids.begin(),ids.end(),current)==ids.end())for(auto id:ids){choose_sequence(id);if(canvas_->sequence_id()==id)break;}
    if(column&&std::find(ids.begin(),ids.end(),canvas_->sequence_id())!=ids.end()){
        update_position(*column);if(!canvas_->interaction_active())canvas_->center_column(*column);
    }
}
void ChromatogramPanel::follow_viewport(std::size_t begin,std::size_t end) {
    if(!has_traces()||begin>=end||canvas_->interaction_active())return;
    const auto focus=canvas_->focused_column();canvas_->center_column(focus>=begin&&focus<end?focus:begin+(end-begin)/2);
}
}
