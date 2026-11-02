#include "gui/menu_shortcuts.hpp"
#include "gui/alignment_view.hpp"
#include <QContextMenuEvent>
#include <QApplication>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollBar>
#include <QToolTip>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace kleiotide {
namespace {
struct AAPaint {QColor fill,ink,onFill;};
const std::array<AAPaint,128>& aa_paints(){static const auto paints=[]{std::array<AAPaint,128> result{};for(unsigned i=0;i<128;++i){const auto rgb=amino_acid_color(static_cast<char>(i));if(rgb){const auto fill=QColor::fromRgb(rgb);result[i]={fill,on_white(fill),black_or_white(fill)};}else result[i]={Qt::transparent,QColor(65,70,75),QColor(25,25,25)};}return result;}();return paints;}
QString utf8(const std::string& s) { return QString::fromUtf8(s.data(), static_cast<qsizetype>(s.size())); }
int scroll_range(std::size_t length, int visible) {
    const auto v = static_cast<std::size_t>(std::max(visible, 1));
    return static_cast<int>(std::min<std::size_t>(length > v ? length - v : 0,
        static_cast<std::size_t>(std::numeric_limits<int>::max())));
}

}
AlignmentView::AlignmentView(QWidget* parent) : QAbstractScrollArea(parent) {
    auto f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(11);
    setFont(f);
    update_metrics();
    setFocusPolicy(Qt::StrongFocus);
    viewport()->setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    dragScroll_ = new QTimer(this);
    dragScroll_->setInterval(40);
    connect(dragScroll_, &QTimer::timeout, this, [this] {
        if(draggingRows_&&isEnabled()){
            auto* v=verticalScrollBar();if(dragPoint_.y()<rulerHeight_+rowHeight_)v->setValue(v->value()-1);
            else if(dragPoint_.y()>viewport()->height()-rowHeight_)v->setValue(v->value()+1);
            update_row_drop(dragPoint_);return;
        }
        if (!dragAnchor_ || !isEnabled()) { dragScroll_->stop(); return; }
        auto* h = horizontalScrollBar(); auto* v = verticalScrollBar();
        if (dragPoint_.x() < nameWidth_ + cellWidth_) h->setValue(h->value() - 3);
        else if (dragPoint_.x() > viewport()->width() - cellWidth_) h->setValue(h->value() + 3);
        if (!rulerDrag_) {
            if (dragPoint_.y() < rulerHeight_ + rowHeight_) v->setValue(v->value() - 1);
            else if (dragPoint_.y() > viewport()->height() - rowHeight_) v->setValue(v->value() + 1);
        }
        update_fragment_drag(dragPoint_);
    });
}
void AlignmentView::set_document(Document* document) {
    clear_row_drag();selectionAnchorId_.reset();
    resizingNames_=false;viewport()->unsetCursor();
    document_ = document;
    update_metrics();
    selectionState_ = document_ ? document_->state_id() : 0;
    searchMatch_.reset();
    searchResults_.reset();
    siteHighlights_.reset();
    fragmentHistory_.clear();
    clear_render_cache();
    clear_fragment();
    selected_.clear(); caretId_.reset(); caretColumn_ = 0; selectionAnchor_ = 0;
    horizontalScrollBar()->setValue(0); verticalScrollBar()->setValue(0);
    refresh();
}
int AlignmentView::visible_rows() const {
    return std::max(1, (viewport()->height() - rulerHeight_) / rowHeight_);
}
int AlignmentView::visible_columns() const {
    return std::max(1, (viewport()->width() - nameWidth_) / cellWidth_);
}
void AlignmentView::refresh() {
    if (!document_) return;
    if(searchRevision_!=document_->state_id()){searchMatch_.reset();searchResults_.reset();}
    if (selectionState_ != document_->state_id()) {
        if (auto it = fragmentHistory_.find(document_->state_id()); it != fragmentHistory_.end()) {
            fragment_ = it->second;
            auto row = std::find_if(document_->rows().begin(), document_->rows().end(), [&](const auto& value) { return value.id == fragment_->ids.front(); });
            if (row != document_->rows().end()) selectionOrigin_ = Cell{static_cast<std::size_t>(row-document_->rows().begin()), fragment_->start};
        }
        selectionState_ = document_->state_id();
    }
    if (cacheState_ != document_->state_id()) {
        tiles_.clear(); names_.clear();
        cacheState_ = document_->state_id();
    }
    auto& live=liveIds_;live.clear();
    for (const auto& row : document_->rows()) live.insert(row.id);
    std::erase_if(selected_, [&](auto id) { return !live.contains(id); });
    if (caretId_ && !live.contains(*caretId_)) caretId_.reset();
    if(selectionAnchorId_){auto it=std::find_if(document_->rows().begin(),document_->rows().end(),[&](const auto& row){return row.id==*selectionAnchorId_;});
        if(it!=document_->rows().end())selectionAnchor_=static_cast<std::size_t>(it-document_->rows().begin());else selectionAnchorId_.reset();}
    if (fragment_) {
        std::erase_if(fragment_->ids, [&](auto id) { return !live.contains(id); });
        const auto width = document_->columns();
        if (fragment_->ids.empty() || fragment_->start >= width) clear_fragment();
        else fragment_->width = std::min(fragment_->width, width - fragment_->start);
    }
    if (fragment_) sync_fragment_caret();
    if (caretId_) caretColumn_ = std::min(caretColumn_, document_->sequence(*caretId_).symbols.size());
    verticalScrollBar()->setRange(0, scroll_range(document_->rows().size(), visible_rows()));
    verticalScrollBar()->setPageStep(visible_rows());
    horizontalScrollBar()->setRange(0, scroll_range(document_->columns() + 1, visible_columns()));
    horizontalScrollBar()->setPageStep(visible_columns());
    viewport()->update();
    if (viewport_changed) viewport_changed();
}
void AlignmentView::reveal_residues(const Document::ResidueLocation& location) {
    if(!document_)return;
    const auto& rows=document_->rows();
    const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto& row){return row.id==location.id;});
    if(found==rows.end())return;
    const auto row=static_cast<int>(found-rows.begin());
    auto* vertical=verticalScrollBar();
    if(row<vertical->value())vertical->setValue(row);
    else if(row>=vertical->value()+visible_rows())vertical->setValue(row-visible_rows()+1);
    const auto start=std::min(location.start,document_->columns());
    const auto width=std::min(location.width,document_->columns()-start);
    const auto visible=static_cast<std::size_t>(visible_columns());
    auto* horizontal=horizontalScrollBar();const auto first=static_cast<std::size_t>(horizontal->value());
    if(start<first||width>visible||start>=first+visible)horizontal->setValue(static_cast<int>(std::min<std::size_t>(start,std::numeric_limits<int>::max())));
    else if(start+width>first+visible)horizontal->setValue(static_cast<int>(std::min<std::size_t>(start+width-visible,std::numeric_limits<int>::max())));
}
void AlignmentView::resizeEvent(QResizeEvent* event) {
    QAbstractScrollArea::resizeEvent(event);
    resize_name_area(nameWidth_);refresh();
}
void AlignmentView::resize_name_area(int width) {
    const auto next=std::clamp(width,80,std::max(80,viewport()->width()-4*cellWidth_));
    if(next==nameWidth_)return;
    nameWidth_=next;names_.clear();refresh();
}
void AlignmentView::clear_render_cache() {
    tiles_.clear(); names_.clear();
    cacheDpr_ = 0;
    cacheState_ = document_ ? document_->state_id() : 0;
}
void AlignmentView::update_metrics() {
    const QFontMetrics metrics(font(), viewport());
    cellWidth_ = std::max(4, metrics.horizontalAdvance(QLatin1Char('M')) + 2);
    rowHeight_ = std::max(12, metrics.height() + 4);
    auto rulerFont=font();rulerFont.setPointSize(std::max(6,font().pointSize()-2));
    rulerHeight_=std::max(rowHeight_,QFontMetrics(rulerFont,viewport()).height()+11)+(codonRuler_?16:0);
}
void AlignmentView::changeEvent(QEvent* event) {
    QAbstractScrollArea::changeEvent(event);
    if (event->type() == QEvent::FontChange) {
        update_metrics(); clear_render_cache(); refresh();
    }
}
void AlignmentView::ensure_render_cache() {
    const auto dpr = viewport()->devicePixelRatioF();
    // Keep roughly two viewport images, bounded to 32–128 MiB, including HiDPI.
    const qint64 budget = static_cast<qint64>(std::ceil(viewport()->width() * dpr))
        * static_cast<qint64>(std::ceil(viewport()->height() * dpr)) * 8;
    tiles_.setMaxCost(static_cast<int>(std::clamp<qint64>(budget, 32 * 1024 * 1024, 128 * 1024 * 1024)));
    if (qFuzzyCompare(cacheDpr_, dpr)) return;
    tiles_.clear(); names_.clear();
    cacheDpr_ = dpr;
    // Rasterize once per font/DPI/style, including dot and selection variants.
    for (std::size_t c = 0; c < glyphs_.size(); ++c) {
        const auto style=residue_paint(static_cast<char>(c),residuePalette_,residueDisplay_);
        auto render=[&](QPixmap& glyph,char symbol,QColor foreground,QColor background){
            glyph=QPixmap(static_cast<int>(std::ceil(cellWidth_*dpr)),static_cast<int>(std::ceil(rowHeight_*dpr)));
            glyph.setDevicePixelRatio(dpr);glyph.fill(background);
            if(c<32||c==127)return;
            QPainter painter(&glyph);painter.setFont(font());painter.setPen(foreground);
            painter.drawText(QRect(0,0,cellWidth_,rowHeight_),Qt::AlignCenter,QString(QChar::fromLatin1(symbol)));
        };
        render(glyphs_[c],static_cast<char>(c),style.foreground,style.background);
        render(maskGlyphs_[c],static_cast<char>(c),QColor(65,70,75),Qt::transparent);
        render(plainGlyphs_[c],static_cast<char>(c),QColor(32,32,32),Qt::transparent);
        render(searchGlyphs_[c],static_cast<char>(c),QColor(32,32,32),QColor(231,164,255));
        render(dotGlyphs_[c],'.',residueDisplay_==ResidueDisplay::Backgrounds?style.foreground:residueDisplay_==ResidueDisplay::Monochrome?QColor(32,32,32):QColor(65,70,75),style.background);
    }
}
const QPixmap& AlignmentView::row_tile(const Sequence& row, std::size_t tile,bool selectedStyle) {
    const auto key = QString::number(row.id) + ':' + QString::number(static_cast<qulonglong>(tile))+(selectedStyle?":selection":"");
    if (const auto* cached = tiles_.object(key)) return *cached;
    const int width = static_cast<int>(tileColumns_) * cellWidth_;
    auto* pixmap = new QPixmap(static_cast<int>(std::ceil(width * cacheDpr_)),
                              static_cast<int>(std::ceil(rowHeight_ * cacheDpr_)));
    pixmap->setDevicePixelRatio(cacheDpr_);
    pixmap->fill(Qt::transparent);
    {
        QPainter painter(pixmap);painter.setFont(font());
        const auto& symbols = row.symbols.text();
        const auto* reference = identityDots_ && !(translationEnabled_&&row.kind==RowKind::Nucleotide) && row.kind != RowKind::Mask &&
            !document_->rows().empty() && document_->rows().front().kind == row.kind &&
            row.id != document_->rows().front().id ? &document_->rows().front().symbols.text() : nullptr;
        const auto upper = [](char c) { return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c; };
        const auto start = tile * tileColumns_;
        const auto end = std::min(start + tileColumns_, symbols.size());
        const auto searchCells=!selectedStyle&&residueDisplay_==ResidueDisplay::Backgrounds&&searchResults_
            ?searchResults_->coverage(row.id,start,end):std::vector<unsigned char>{};
        const auto* translated=translationEnabled_&&translation_&&translation_->content==document_->content_id()?translation_->row(row.id):nullptr;
        for (auto c = start; c < end; ++c) {
            const auto x=static_cast<int>(c-start)*cellWidth_;
            if(translated&&(c>=translation_->map.at.size()||translation_->map.at[c]<0)){
                const auto symbol=static_cast<unsigned char>(symbols[c]);const bool partial=c<translation_->map.partial.size()&&translation_->map.partial[c];
                if(partial&&!selectedStyle)painter.setOpacity(0.55);
                if(symbol<glyphs_.size())painter.drawPixmap(QPoint(x,0),selectedStyle?plainGlyphs_[symbol]:glyphs_[symbol]);
                painter.setOpacity(1);
                if(partial){painter.setPen(QColor(165,65,20));painter.drawLine(x+1,rowHeight_-2,x+cellWidth_-2,rowHeight_-2);}continue;
            }
            if(translated||row.kind==RowKind::Protein){
                char amino=0;QString label;std::uint32_t rgb=0;bool flag=false;
                if(translated&&c<translation_->map.at.size()&&translation_->map.at[c]>=0){
                    const auto slot=static_cast<std::size_t>(translation_->map.at[c]);const auto& call=translated->calls[slot];const auto& span=translation_->map.codons[slot];amino=call.amino;flag=(call.flags&(PartialGap|PossibleStop|ContextStop))!=0;
                    if(aminoLetters_){if(c==span.columns[span.cells/2])label=QString(QChar::fromLatin1(amino));}
                    else label=QString(QChar::fromLatin1(symbols[c]));
                }else if(row.kind==RowKind::Protein){
                    amino=symbols[c];label=QString(QChar::fromLatin1(amino));
                }else {label=QString(QChar::fromLatin1(symbols[c]));if(translated&&c<translation_->map.partial.size()&&translation_->map.partial[c])flag=true;}
                rgb=amino_acid_color(amino);
                const bool background=translated||residueDisplay_==ResidueDisplay::Backgrounds;
                if(!selectedStyle&&rgb&&residueDisplay_!=ResidueDisplay::Monochrome&&background)painter.fillRect(x,0,cellWidth_,rowHeight_,QColor::fromRgb(rgb));
                const auto& paint=aa_paints()[static_cast<unsigned char>(amino)<128?static_cast<unsigned char>(amino):static_cast<unsigned char>('X')];painter.setPen(selectedStyle||residueDisplay_==ResidueDisplay::Monochrome?QColor(25,25,25):background?paint.onFill:paint.ink);painter.drawText(QRect(x,0,cellWidth_,rowHeight_),Qt::AlignCenter,label);
                if(flag){painter.setPen(QColor(130,50,0));painter.drawLine(x+1,rowHeight_-2,x+cellWidth_-2,rowHeight_-2);}
                continue;
            }
            const bool identical = reference && c < reference->size() && !is_gap(symbols[c]) &&
                upper(symbols[c]) == upper((*reference)[c]);
            const auto symbol=static_cast<unsigned char>(symbols[c]);
            if(symbol<glyphs_.size()){
                const auto plainSymbol=identical?static_cast<unsigned char>('.'):symbol;
                const auto& glyph=row.kind==RowKind::Mask?maskGlyphs_[symbol]:row.kind==RowKind::Protein?maskGlyphs_[plainSymbol]:selectedStyle?plainGlyphs_[plainSymbol]
                    :!searchCells.empty()&&(searchCells[c-start]&1)?searchGlyphs_[plainSymbol]:identical?dotGlyphs_[symbol]:glyphs_[symbol];
                painter.drawPixmap(QPoint(static_cast<int>(c-start)*cellWidth_,0),glyph);
            }
        }
    }
    const qint64 cost = static_cast<qint64>(pixmap->width()) * pixmap->height() * 4;
    if (cost > tiles_.maxCost()) {
        oversizedTile_ = std::move(*pixmap);
        delete pixmap;
        return oversizedTile_;
    }
    tiles_.insert(key, pixmap, static_cast<int>(cost));
    return *tiles_.object(key);
}
void AlignmentView::set_identity_dots(bool enabled) {
    if (identityDots_ == enabled) return;
    identityDots_ = enabled;
    tiles_.clear();
    viewport()->update();
}
void AlignmentView::set_residue_palette(ResiduePalette palette) {
    if(residuePalette_==palette)return;
    residuePalette_=palette;clear_render_cache();viewport()->update();
}
void AlignmentView::set_residue_display(ResidueDisplay display) {
    if(residueDisplay_==display)return;
    residueDisplay_=display;clear_render_cache();viewport()->update();
}
void AlignmentView::scrollContentsBy(int dx, int dy) {
    if (viewport_changed) viewport_changed();
    QToolTip::hideText();
    if (dx && dy) { viewport()->update(); return; }
    // Ruler labels extend to the left of their tick column. Repaint the ruler as
    // a whole so a newly entering label is never clipped to the exposed strip.
    if (dx) viewport()->update(QRect(nameWidth_, 0, std::max(0, viewport()->width() - nameWidth_), rulerHeight_));
    const QRect area = dx ? QRect(nameWidth_, rulerHeight_, std::max(0, viewport()->width() - nameWidth_), std::max(0, viewport()->height() - rulerHeight_))
                         : QRect(0, rulerHeight_, viewport()->width(), std::max(0, viewport()->height() - rulerHeight_));
    const qint64 shift = dx ? static_cast<qint64>(dx) * cellWidth_ : static_cast<qint64>(dy) * rowHeight_;
    if (area.isEmpty()) return;
    if (std::abs(shift) >= (dx ? area.width() : area.height())) viewport()->update(area);
    else viewport()->scroll(dx ? static_cast<int>(shift) : 0, dy ? static_cast<int>(shift) : 0, area);
    // A wide selection has a clipped left outline fixed at the body edge.
    // Redraw both its new position and any copy shifted into the body. Keep
    // this strip beside the exposed pixels rather than spanning the viewport.
    if(dx&&fragment_)viewport()->update(QRect(nameWidth_,rulerHeight_,
        static_cast<int>(std::min<qint64>(area.width(),std::abs(shift)+2)),area.height()));
}
void AlignmentView::paintEvent(QPaintEvent* event) {
    ensure_render_cache();
    QPainter p(viewport());
    p.fillRect(event->rect(), Qt::white);
    p.setFont(font());
    p.fillRect(0, 0, viewport()->width(), rulerHeight_, QColor(238, 240, 244));
    p.fillRect(0, rulerHeight_, nameWidth_, viewport()->height(), QColor(247, 248, 250));
    if (!document_) return;
    const auto firstRow = static_cast<std::size_t>(verticalScrollBar()->value());
    const auto firstCol = static_cast<std::size_t>(horizontalScrollBar()->value());
    const auto bodyDirty = event->region().intersected(QRect(nameWidth_, rulerHeight_,
        std::max(0, viewport()->width() - nameWidth_), std::max(0, viewport()->height() - rulerHeight_))).boundingRect();
    const auto namesDirty = event->region().intersected(QRect(0, rulerHeight_, nameWidth_,
        std::max(0, viewport()->height() - rulerHeight_))).boundingRect();
    const auto rowsDirty = bodyDirty.united(namesDirty);
    const int topRow = std::max(0, (rowsDirty.top() - rulerHeight_) / rowHeight_);
    const int bottomRow = rowsDirty.isEmpty() ? 0 : std::max(0, (rowsDirty.bottom() - rulerHeight_) / rowHeight_ + 1);
    const auto rowStart = firstRow + static_cast<std::size_t>(topRow);
    const auto lastRow = std::min(document_->rows().size(), firstRow + static_cast<std::size_t>(bottomRow));
    const auto columnEnd = firstCol + static_cast<std::size_t>(visible_columns() + 1);
    p.setPen(QColor(90, 95, 105));
    p.drawText(QRect(6, 0, nameWidth_ - 12, rulerHeight_), Qt::AlignVCenter, "Sequence name");
    p.save();
    p.setClipRect(nameWidth_, 0, std::max(0, viewport()->width() - nameWidth_), rulerHeight_);
    auto rulerFont = font(); rulerFont.setPointSize(std::max(6, font().pointSize()-2));
    p.setFont(rulerFont);
    const bool sitesCurrent=siteHighlights_&&siteRevision_==document_->content_id();
    const std::unordered_set<SequenceId> fragmentRows=fragment_?std::unordered_set<SequenceId>(fragment_->ids.begin(),fragment_->ids.end()):std::unordered_set<SequenceId>{};
    if(sitesCurrent)for(auto c=firstCol;c<std::min(columnEnd,siteHighlights_->size());++c)if((*siteHighlights_)[c]){
        const int x=nameWidth_+static_cast<int>(c-firstCol)*cellWidth_;
        p.fillRect(x,0,cellWidth_,4,siteColor_.darker(160));
        auto shade=siteColor_;shade.setAlpha(70);p.fillRect(x,4,cellWidth_,rulerHeight_-4,shade);
    }
    for (std::size_t c = firstCol>=10?firstCol-10:0; c < columnEnd+10; ++c) {
        const int center = nameWidth_ + (static_cast<int>(c)-static_cast<int>(firstCol))*cellWidth_ + cellWidth_/2;
        const int length = (c+1)%10 == 0 ? 8 : (c+1)%5 == 0 ? 5 : 3;
        const int base=rulerHeight_-(codonRuler_?16:0);
        p.drawLine(center,base-length,center,base-1);
        if(codonRuler_&&c<document_->columns()){const auto phase=coding_phase(document_->translation_settings(),c);p.setPen(QColor(65,70,85));p.drawText(QRect(center-cellWidth_/2,base,cellWidth_,16),Qt::AlignCenter,phase?QString::number(phase):QString("·"));p.setPen(QColor(35,40,50));}
        if ((c+1)%10 == 0) {
            const auto label = QString::number(static_cast<qulonglong>(c+1));
            const int width = QFontMetrics(rulerFont).horizontalAdvance(label) + 4;
            p.drawText(QRect(center-width/2,1,width,rulerHeight_-10-(codonRuler_?16:0)), Qt::AlignCenter, label);
        }
    }
    p.restore(); p.setFont(font());
    for (std::size_t r = rowStart; r < lastRow; ++r) {
        const auto& row = document_->rows()[r];
        const int y = rulerHeight_ + static_cast<int>(r - firstRow) * rowHeight_;
        if (selected_.contains(row.id)) p.fillRect(0, y, nameWidth_, rowHeight_, QColor(204, 222, 249));
        if(const auto* group=document_->group(row.groupId))p.fillRect(1,y,10,rowHeight_,QColor::fromRgb(group->color));
        p.setPen(QColor(35, 40, 50));
        if (namesDirty.intersects(QRect(0, y, nameWidth_, rowHeight_))) {
            auto name = names_.constFind(row.id);
            if (name == names_.cend())
                name = names_.insert(row.id, QFontMetrics(font(), viewport()).elidedText(utf8(row.name), Qt::ElideRight, nameWidth_ - (row.trace?58:38)));
            p.drawText(QRect(16, y, nameWidth_ - (row.trace?58:38), rowHeight_), Qt::AlignVCenter, *name);
            p.save();auto badgeFont=font();badgeFont.setPixelSize(std::clamp(rowHeight_-8,8,12));p.setFont(badgeFont);
            const QRect badge(nameWidth_-20,y+3,16,std::max(6,rowHeight_-6));
            p.setBrush(QColor(233,237,243));p.setPen(QColor(185,193,204));p.drawRoundedRect(badge.adjusted(0,0,-1,-1),2,2);
            p.setPen(QColor(65,70,75));p.drawText(badge,Qt::AlignCenter,row.kind==RowKind::Protein?"P":row.kind==RowKind::Mask?"M":"N");p.restore();
            if(row.trace){const int x=nameWidth_-42,mid=y+rowHeight_/2;const QPen old=p.pen();p.setPen(QPen(QColor(38,93,151),1.3));
                const QPolygon wave({QPoint(x,mid+3),QPoint(x+3,mid+3),QPoint(x+6,mid-4),QPoint(x+9,mid+3),QPoint(x+12,mid-2),QPoint(x+15,mid+3)});p.drawPolyline(wave);p.setPen(old);}

        }
        p.save();
        p.setClipRect(nameWidth_, y, std::max(0, viewport()->width() - nameWidth_), rowHeight_);
        auto fragment_rect = [&](std::int64_t offset) {
            if (!fragment_ || !fragmentRows.contains(row.id))
                return QRect{};
            const auto begin = static_cast<std::int64_t>(fragment_->start) + offset;
            const auto finish = begin + static_cast<std::int64_t>(fragment_->width);
            const auto a = std::max(begin, static_cast<std::int64_t>(firstCol));
            const auto b = std::min(finish, static_cast<std::int64_t>(columnEnd));
            return a < b ? QRect(nameWidth_ + static_cast<int>(a - static_cast<std::int64_t>(firstCol)) * cellWidth_,
                                 y, static_cast<int>(b - a) * cellWidth_, rowHeight_) : QRect{};
        };
        const auto selectedRect = fragment_rect(0);
        if (!selectedRect.isEmpty()) p.fillRect(selectedRect, blue_fragment() ? QColor(225, 235, 251) : QColor(250, 235, 191));
        if (!fragment_ && caretId_ && *caretId_ == row.id && caretColumn_ >= firstCol && caretColumn_ < columnEnd) {
            p.fillRect(QRect(nameWidth_ + static_cast<int>(caretColumn_ - firstCol) * cellWidth_,
                             y, cellWidth_, rowHeight_), QColor(225, 235, 251));
        }
        auto paint_tiles=[&](const QRect& dirty,bool selection){
            if(dirty.isEmpty())return;
            // Scrolling a selected block can expose two separated edge strips.
            // Visit their tiles separately instead of their full-width bounds.
            const auto strips=event->region().intersected(dirty.intersected(QRect(nameWidth_,y,std::max(0,viewport()->width()-nameWidth_),rowHeight_)));
            std::vector<std::size_t> painted;
            for(const auto& strip:strips){
            const auto dirtyStart=firstCol+static_cast<std::size_t>(std::max(0,strip.left()-nameWidth_)/cellWidth_);
            const auto dirtyEnd=std::min(firstCol+static_cast<std::size_t>((strip.right()-nameWidth_)/cellWidth_+1),row.symbols.size());
            if(dirtyStart<dirtyEnd)for(auto tile=dirtyStart/tileColumns_;tile<=(dirtyEnd-1)/tileColumns_;++tile){
                // Several dirty strips can touch the same tile. A translucent
                // glyph must be composited exactly once, even at split edges.
                if(std::find(painted.begin(),painted.end(),tile)!=painted.end())continue;
                painted.push_back(tile);
                const auto offset=static_cast<qint64>(tile*tileColumns_)-static_cast<qint64>(firstCol);
                p.drawPixmap(QPoint(nameWidth_+static_cast<int>(offset)*cellWidth_,y),row_tile(row,tile,selection));
            }
            }
        };
        paint_tiles(bodyDirty,false);
        if((residueDisplay_==ResidueDisplay::Backgrounds||translationEnabled_||row.kind==RowKind::Protein)&&!selectedRect.isEmpty()){
            p.save();p.setClipRect(selectedRect,Qt::IntersectClip);
            p.fillRect(selectedRect,blue_fragment()?QColor(225,235,251):QColor(250,235,191));
            paint_tiles(bodyDirty.intersected(selectedRect),true);p.restore();
        }
        if(sitesCurrent&&row.kind==siteMolecule_&&(!siteRows_||siteRows_->contains(row.id))){
            // Paint only visible bands, after residue tiles and outside selection.
            // The opaque edge + white halo survives all residue palettes/styles.
            QRegion selectionRegion(selectedRect);
            if(!fragment_&&caretId_&&*caretId_==row.id&&caretColumn_>=firstCol&&caretColumn_<columnEnd)
                selectionRegion+=QRect(nameWidth_+static_cast<int>(caretColumn_-firstCol)*cellWidth_,y,cellWidth_,rowHeight_);
            p.save();p.setClipRegion(QRegion(QRect(nameWidth_,y,std::max(0,viewport()->width()-nameWidth_),rowHeight_)).subtracted(selectionRegion),Qt::IntersectClip);
            for(auto c=firstCol;c<std::min({columnEnd,row.symbols.size(),siteHighlights_->size()});){
                if(!(*siteHighlights_)[c]){++c;continue;}const auto begin=c;
                while(c<std::min({columnEnd,row.symbols.size(),siteHighlights_->size()})&&(*siteHighlights_)[c])++c;
                const QRect band(nameWidth_+static_cast<int>(begin-firstCol)*cellWidth_,y,static_cast<int>(c-begin)*cellWidth_,rowHeight_);
                auto shade=siteColor_;shade.setAlpha(residueDisplay_==ResidueDisplay::Backgrounds?115:80);if(translationEnabled_||row.kind==RowKind::Protein){p.setPen(QPen(siteColor_.darker(150),2));p.drawRect(band.adjusted(1,1,-1,-1));}else p.fillRect(band,shade);
                p.setPen(QPen(Qt::white,3));p.drawLine(band.left(),y+1,band.right(),y+1);p.drawLine(band.left(),y+rowHeight_-2,band.right(),y+rowHeight_-2);
                p.setPen(siteColor_.darker(180));p.drawLine(band.left(),y+1,band.right(),y+1);p.drawLine(band.left(),y+rowHeight_-2,band.right(),y+rowHeight_-2);
            }
            p.restore();
        }
        if (!fragment_ && caretId_ && *caretId_ == row.id && caretColumn_ >= firstCol && caretColumn_ < columnEnd) {
            p.setPen(QPen(mode_ == Mode::View ? QColor(145, 155, 170) : QColor(25, 85, 200), 1));
            p.drawRect(QRect(nameWidth_ + static_cast<int>(caretColumn_ - firstCol) * cellWidth_,
                             y, cellWidth_, rowHeight_).adjusted(0, 0, -1, -1));
        }
        if (!selectedRect.isEmpty()) {
            p.setPen(QPen(blue_fragment() ? QColor(25, 85, 200) : QColor(130, 85, 0), 1));
            p.drawRect(selectedRect.adjusted(0, 0, -1, -1));
        }
        if(searchResults_){
            // One bitmask per visible row, not a scan/draw call for every hit.
            // Dense and overlapping arithmetic runs are resolved by the index.
            const auto cells=searchResults_->coverage(row.id,firstCol,columnEnd);
            const bool bright=residueDisplay_==ResidueDisplay::Backgrounds;
            std::vector<QLine> edges;edges.reserve(4*cells.size());
            QRegion matchRegion;
            for(std::size_t i=0;i<cells.size();){if(!(cells[i]&1)){++i;continue;}const auto start=i;while(i<cells.size()&&(cells[i]&1))++i;
                const QRect band(nameWidth_+static_cast<int>(start)*cellWidth_,y,static_cast<int>(i-start)*cellWidth_,rowHeight_);
                if(bright)matchRegion+=band;
                // Bright fill and dark glyphs are already in cached row tiles;
                // avoid painting every matched cell twice while scrolling.
                if(!bright){p.save();p.setClipRegion(QRegion(band).subtracted(QRegion(selectedRect)),Qt::IntersectClip);
                    p.fillRect(band,QColor(147,84,185,30));p.restore();}
                edges.emplace_back(band.left(),y+1,band.right(),y+1);edges.emplace_back(band.left(),y+rowHeight_-2,band.right(),y+rowHeight_-2);}
            for(std::size_t i=0;i<cells.size();++i){const int x=nameWidth_+static_cast<int>(i)*cellWidth_;
                if(cells[i]&2)edges.emplace_back(x,y+1,x,y+rowHeight_-2);
                if(cells[i]&4)edges.emplace_back(x+cellWidth_-1,y+1,x+cellWidth_-1,y+rowHeight_-2);}
            if(!edges.empty()){
                // Keep the halo inside covered cells: a clipped neighboring
                // hit must not leave an outer white pixel after scrolling.
                if(bright){p.save();p.setClipRegion(matchRegion,Qt::IntersectClip);p.setPen(QPen(Qt::white,3));p.drawLines(edges.data(),static_cast<int>(edges.size()));p.restore();}
                p.setPen(QColor(147,84,185));p.drawLines(edges.data(),static_cast<int>(edges.size()));}
        }
        if(searchMatch_&&searchMatch_->ids.front()==row.id){
            const auto begin=searchMatch_->start,end=begin+searchMatch_->width;
            const auto a=std::max(begin,firstCol),b=std::min(end,columnEnd);
            if(a<b){const int left=nameWidth_+static_cast<int>(a-firstCol)*cellWidth_,right=nameWidth_+static_cast<int>(b-firstCol)*cellWidth_-1;
                // White halo keeps the purple cue visible over any residue fill.
                // Draw vertical edges only at actual hit boundaries, so clipping
                // does not introduce fixed edge pixels during partial scrolling.
                for(const auto& pen:{QPen(Qt::white,3),QPen(QColor(126,44,181),1)}){p.setPen(pen);
                    p.drawLine(left,y+2,right,y+2);p.drawLine(left,y+rowHeight_-3,right,y+rowHeight_-3);
                    if(begin>=firstCol)p.drawLine(left,y+2,left,y+rowHeight_-3);
                    if(end<=columnEnd)p.drawLine(right,y+2,right,y+rowHeight_-3);}
            }
        }
        if (draggingBlock_ && dragOffset_) {
            const auto preview = fragment_rect(dragOffset_);
            if(residueDisplay_==ResidueDisplay::Backgrounds&&!preview.isEmpty()){p.setPen(QPen(Qt::white,4));p.drawRect(preview.adjusted(0,0,-1,-1));}
            p.setPen(QPen(QColor(35, 95, 190), 2, Qt::DashLine));
            if (!preview.isEmpty()) p.drawRect(preview.adjusted(0, 0, -1, -1));
        }
        p.restore();
    }
    p.setPen(QColor(200, 204, 212));
    p.drawLine(nameWidth_ - 1, 0, nameWidth_ - 1, viewport()->height());
    if(draggingRows_){const int y=rulerHeight_+(static_cast<int>(rowDropIndex_)-verticalScrollBar()->value())*rowHeight_;
        p.setPen(QPen(QColor(35,95,190),2));p.drawLine(0,std::clamp(y,0,viewport()->height()-1),viewport()->width(),std::clamp(y,0,viewport()->height()-1));}
}
std::optional<AlignmentView::Cell> AlignmentView::cell_at(const QPoint& point) const {
    if (!document_ || point.y() < rulerHeight_ || point.x() < nameWidth_) return std::nullopt;
    const auto row = static_cast<std::size_t>((point.y() - rulerHeight_) / rowHeight_ + verticalScrollBar()->value());
    if (row >= document_->rows().size()) return std::nullopt;
    const auto column = static_cast<std::size_t>((point.x() - nameWidth_) / cellWidth_ + horizontalScrollBar()->value());
    return Cell{row, column};
}
void AlignmentView::notify_selection() {
    viewport()->update();
    if (selection_changed) selection_changed();
}
void AlignmentView::clear_fragment() {
    if (dragScroll_) dragScroll_->stop();
    fragment_.reset(); dragAnchor_.reset(); selectionOrigin_.reset(); dragRows_.clear();
    draggingBlock_ = false; rulerDrag_ = false; dragOffset_ = 0;
    if (selection_changed) selection_changed();
}
void AlignmentView::set_mode(Mode mode) {
    if (dragScroll_) dragScroll_->stop();
    dragAnchor_.reset(); dragRows_.clear(); draggingBlock_ = false; rulerDrag_ = false; dragOffset_ = 0;
    mode_ = mode;if(mode_==Mode::View)rightClick_=RightClick::Menu;
    if (fragment_) sync_fragment_caret();
    notify_selection();
}
bool AlignmentView::blue_fragment() const {
    return fragment_ && caretId_ && fragment_->ids.size() == 1 && complete_selection() &&
        (mode_ == Mode::Rewrite || (mode_ == Mode::Insert && insertable_selection()));
}
void AlignmentView::sync_fragment_caret() {
    caretId_.reset();
    if (!document_ || !fragment_ || fragment_->ids.size() != 1) return;
    if (!insertable_selection() && mode_ != Mode::Rewrite) return;
    const auto id = fragment_->ids.front();
    if (fragment_->start > document_->sequence(id).symbols.size()) return;
    caretId_ = id; caretColumn_ = fragment_->start;
}
void AlignmentView::select_fragment(Fragment fragment) {
    if (!document_ || fragment.ids.empty() || !fragment.width || fragment.start >= document_->columns() ||
        fragment.width > document_->columns() - fragment.start) { clear_fragment(); return; }
    for (auto id : fragment.ids) if(!liveIds_.contains(id))throw std::out_of_range("Unknown sequence ID.");
        if(fragment_&&*fragment_==fragment)return;
    const auto before=selection_paint_regions();const bool wasBlue=fragment_?blue_fragment():caretId_.has_value();
    fragment_ = std::move(fragment); sync_fragment_caret();
    const auto after=selection_paint_regions();
    // Interior cells shared by equal-colored selections do not change. Include
    // every row outline, since old borders can become interior pixels.
    const auto dirty=(wasBlue==blue_fragment()?before.first.xored(after.first):(before.first|after.first))|before.second|after.second;
    if(!dirty.isEmpty())viewport()->update(dirty);
    if (selection_changed) selection_changed();
}
std::pair<QRegion,QRegion> AlignmentView::selection_paint_regions() const {
    QRegion area,edges;if(!document_)return {area,edges};
    const auto firstRow=static_cast<std::size_t>(verticalScrollBar()->value());
    const auto firstCol=static_cast<std::size_t>(horizontalScrollBar()->value());
    const auto lastCol=firstCol+static_cast<std::size_t>(visible_columns()+1);
    const auto lastRow=std::min(document_->rows().size(),firstRow+static_cast<std::size_t>(visible_rows()+1));
    const std::unordered_set<SequenceId> ids=fragment_?std::unordered_set<SequenceId>(fragment_->ids.begin(),fragment_->ids.end()):std::unordered_set<SequenceId>{};
    for(auto row=firstRow;row<lastRow;++row){
        const auto id=document_->rows()[row].id;
        if(fragment_?!ids.contains(id):(!caretId_||*caretId_!=id))continue;
        const auto begin=fragment_?fragment_->start:caretColumn_;
        const auto end=begin+(fragment_?fragment_->width:1);
        const auto a=std::max(begin,firstCol),b=std::min(end,lastCol);if(a>=b)continue;
        const QRect rect(nameWidth_+static_cast<int>(a-firstCol)*cellWidth_,rulerHeight_+static_cast<int>(row-firstRow)*rowHeight_,static_cast<int>(b-a)*cellWidth_,rowHeight_);
        area+=rect;
        // A one-logical-pixel pen can reach outside its rectangle at fractional
        // device scales. Repaint both sides of every old/new outline.
        edges+=QRegion(rect.adjusted(-2,-2,2,2)).subtracted(QRegion(rect.adjusted(2,2,-2,-2)));
    }
    const QRect body(nameWidth_,rulerHeight_,std::max(0,viewport()->width()-nameWidth_),std::max(0,viewport()->height()-rulerHeight_));
    return {area.intersected(body),edges.intersected(body)};
}
bool AlignmentView::fragment_contains(const Cell& cell) const {
    return fragment_ && cell.column >= fragment_->start && cell.column - fragment_->start < fragment_->width
        && std::find(fragment_->ids.begin(), fragment_->ids.end(), document_->rows()[cell.row].id) != fragment_->ids.end();
}
void AlignmentView::notify_edit() {
    refresh(); if (changed) changed();
}
void AlignmentView::remember_fragment() {
    if (document_ && fragment_) fragmentHistory_[document_->state_id()] = *fragment_;
    while (fragmentHistory_.size() > 128) fragmentHistory_.erase(fragmentHistory_.begin());
}
void AlignmentView::slide_fragment(std::int64_t distance) {
    if (!document_ || !fragment_ || mode_==Mode::View) return;
    try {
        remember_fragment();
        if (distance == std::numeric_limits<std::int64_t>::max() || distance == -std::numeric_limits<std::int64_t>::max())
            distance = document_->adjacent_gap_distance(fragment_->ids, fragment_->start, fragment_->width, distance);
        const auto moved = document_->slide(fragment_->ids, fragment_->start, fragment_->width, distance);
        if (moved < 0) fragment_->start -= static_cast<std::size_t>(-moved);
        else fragment_->start += static_cast<std::size_t>(moved);
        if (selectionOrigin_) {
            if (moved < 0) selectionOrigin_->column -= std::min(selectionOrigin_->column, static_cast<std::size_t>(-moved));
            else selectionOrigin_->column += static_cast<std::size_t>(moved);
        }
        if (moved == 0 && message) message("No open gaps are available in that direction.");
        remember_fragment(); notify_edit();
    } catch (const std::exception& e) { if (message) message(QString::fromUtf8(e.what())); }
}
void AlignmentView::lock_fragment(){
    if(!document_||!fragment_||mode_==Mode::View)return;
    try{remember_fragment();document_->lock_fragment(fragment_->ids,fragment_->start,fragment_->width);remember_fragment();notify_edit();}
    catch(const std::exception& e){if(message)message(QString::fromUtf8(e.what()));}
}
void AlignmentView::unlock_fragment(){
    if(!document_||!fragment_||mode_==Mode::View)return;
    try{remember_fragment();document_->unlock_fragment(fragment_->ids,fragment_->start,fragment_->width);remember_fragment();notify_edit();}
    catch(const std::exception& e){if(message)message(QString::fromUtf8(e.what()));}
}
void AlignmentView::fill_fragment(char symbol) {
    if (!document_ || !fragment_) return;
    try {
        remember_fragment(); document_->fill_fragment(fragment_->ids, fragment_->start, fragment_->width, symbol);
        remember_fragment(); notify_edit();
    }
    catch (const std::exception& e) { if (message) message(QString::fromUtf8(e.what())); }
}
std::optional<QString> AlignmentView::selected_residue_text() const {
    if (!document_ || !fragment_ || fragment_->ids.size()!=1) return std::nullopt;
    const auto& text = document_->sequence(fragment_->ids.front()).symbols.text();
    if (fragment_->start >= text.size()) return std::nullopt;
    const auto count = std::min(fragment_->width, text.size()-fragment_->start);
    return QString::fromLatin1(text.data()+fragment_->start, static_cast<qsizetype>(count));
}
bool AlignmentView::paste_residues(const QString& text) {
    const auto bytes = text.toUtf8();
    std::string clean;
    for (char c : bytes) if (c!=' ' && c!='\t' && c!='\r' && c!='\n') clean += c;
    return write_residues(clean);
}
bool AlignmentView::write_residues(std::string_view text) {
    if (!document_ || (mode_ != Mode::Insert && mode_ != Mode::Rewrite) || !caretId_ || text.empty()) return false;
    if (fragment_ && (fragment_->ids.size()!=1 || (mode_==Mode::Insert && !insertable_selection()))) return false;
    const auto kind=document_->sequence(*caretId_).kind;
    for (char c : text) {
        if (!is_row_symbol(c,kind)) return false;
    }
    const auto id = *caretId_;
    const auto length = document_->sequence(id).symbols.size();
    if(caretColumn_>length)return false;
    const auto start = mode_ == Mode::Insert && fragment_ && caretColumn_ < length ? fragment_->start+fragment_->width : caretColumn_;
    const auto count = mode_ == Mode::Rewrite ?
        std::min(fragment_ ? fragment_->width : text.size(), length-start) : 0;
    remember_fragment();
    document_->replace(id,start,count,text);
    clear_fragment(); caretId_ = id; caretColumn_ = start + text.size() - (mode_ == Mode::Insert ? 1 : 0);
    // Insert keeps the last inserted cell selected for continuous typing.
    // Overwrite advances to the next symbol, or the unambiguous row end.
    if (caretColumn_ < document_->sequence(id).symbols.size())
        select_fragment({{id},caretColumn_,1});
    notify_edit(); ensure_caret_visible();
    return true;
}
void AlignmentView::delete_residues(bool backspace) {
    if (!document_ || (mode_!=Mode::Insert && mode_!=Mode::Rewrite)) return;
    if (fragment_) {
        const auto selection = *fragment_;
        std::vector<Document::Replacement> edits;
        for (auto id : selection.ids) {
            const auto size = document_->sequence(id).symbols.size();
            if (selection.start < size)
                edits.push_back({id,selection.start,std::min(selection.width,size-selection.start),{}});
        }
        remember_fragment();if(selection.columns)document_->erase_columns(selection.start,selection.width);else document_->replace_many(edits); clear_fragment(); caretId_.reset();
        if (selection.ids.size()==1) {
            const auto id=selection.ids.front(); caretId_=id;
            const auto target = backspace && selection.start ? selection.start - 1 : selection.start;
            caretColumn_=std::min(target,document_->sequence(id).symbols.size());
            if (caretColumn_<document_->sequence(id).symbols.size()) select_fragment({{id},caretColumn_,1});
        }
    } else if (caretId_) {
        const auto id=*caretId_; const auto size=document_->sequence(id).symbols.size();
        if (backspace) { if (!caretColumn_) return; --caretColumn_; }
        else if (caretColumn_>=size) return;
        document_->replace(id,caretColumn_,1,"");
        if (caretColumn_<document_->sequence(id).symbols.size()) select_fragment({{id},caretColumn_,1});
    } else return;
    notify_edit(); ensure_caret_visible();
}
std::pair<std::size_t,std::size_t> AlignmentView::visible_column_range() const {
    if (!document_) return {0,0};
    const auto begin=static_cast<std::size_t>(horizontalScrollBar()->value());
    return {std::min(begin,document_->columns()),std::min(document_->columns(),begin+static_cast<std::size_t>(visible_columns()))};
}
void AlignmentView::navigate_to(std::size_t column) {
    const auto half=static_cast<std::size_t>(visible_columns()/2);
    horizontalScrollBar()->setValue(static_cast<int>(std::min<std::size_t>(column>half?column-half:0,std::numeric_limits<int>::max())));
}
void AlignmentView::set_search_match(std::optional<Fragment> match){
    searchMatch_.reset();
    if(document_&&match&&match->ids.size()==1&&match->width){
        for(std::size_t i=0;i<document_->rows().size();++i){const auto& row=document_->rows()[i];
            if(row.id!=match->ids.front())continue;
            if(match->start>=row.symbols.size()||match->width>row.symbols.size()-match->start)break;
            searchMatch_=std::move(match);searchRevision_=document_->state_id();
            const auto first=static_cast<std::size_t>(verticalScrollBar()->value());
            if(i<first||i>=first+static_cast<std::size_t>(visible_rows()))verticalScrollBar()->setValue(static_cast<int>(std::min<std::size_t>(i,std::numeric_limits<int>::max())));
            navigate_to(searchMatch_->start);break;
        }
    }
    viewport()->update();
}
void AlignmentView::navigate_to_cell(SequenceId id,std::size_t column) {
    if(!document_)return;
    for(std::size_t index=0;index<document_->rows().size();++index)if(document_->rows()[index].id==id){
        if(column>document_->rows()[index].symbols.size())return;
        if(column==document_->rows()[index].symbols.size()){clear_fragment();caretId_=id;caretColumn_=column;notify_selection();}
        else select_fragment({{id},column,1});
        const auto first=static_cast<std::size_t>(verticalScrollBar()->value());
        if(index<first||index>=first+static_cast<std::size_t>(visible_rows()))verticalScrollBar()->setValue(static_cast<int>(index));
        navigate_to(column);return;
    }
}
void AlignmentView::update_fragment_drag(const QPoint& point) {
    if (!dragAnchor_ || !document_) return;
    dragPoint_ = point;
    const auto rowCount = document_->rows().size();
    const auto visibleEnd = rulerHeight_ + static_cast<int>(std::min<std::size_t>(rowCount - std::min(rowCount, static_cast<std::size_t>(verticalScrollBar()->value())), visible_rows() + 1)) * rowHeight_ - 1;
    const auto clamped = QPoint(std::clamp(point.x(), nameWidth_, std::max(nameWidth_, viewport()->width()-1)),
                               std::clamp(point.y(), rulerHeight_, std::max(rulerHeight_, std::min(visibleEnd, viewport()->height()-1))));
    const auto cell = cell_at(clamped); if (!cell) return;
    if (draggingBlock_) {
        auto offset = static_cast<std::int64_t>(cell->column) - static_cast<std::int64_t>(dragAnchor_->column);
        if (fragment_) offset = document_->slide_distance(fragment_->ids, fragment_->start, fragment_->width, offset);
        if(offset==dragOffset_)return;
        dragOffset_=offset;
        viewport()->update(); return;
    }
    const auto last = document_->columns(); if (!last) return;
    const auto a = std::min(dragAnchor_->column, last - 1), b = std::min(cell->column, last - 1);
    std::vector<SequenceId> ids;
    if (rulerDrag_ || cell->row == dragAnchor_->row) ids = dragRows_;
    else for (auto r = std::min(cell->row, dragAnchor_->row); r <= std::max(cell->row, dragAnchor_->row); ++r)
        ids.push_back(document_->rows()[r].id);
    select_fragment({std::move(ids), std::min(a, b), std::max(a, b) - std::min(a, b) + 1,rulerDrag_});
}
void AlignmentView::mousePressEvent(QMouseEvent* event) {
    if(event->button()==Qt::LeftButton)clear_row_drag();
    if(event->button()==Qt::LeftButton&&on_name_border(event->position().toPoint().x())){
        resizingNames_=true;nameResizeOffset_=event->position().toPoint().x()-(nameWidth_-1);
        viewport()->setCursor(Qt::SplitHCursor);QToolTip::hideText();event->accept();return;
    }
    if (document_ && event->button() == Qt::RightButton && mode_!=Mode::View && rightClick_ != RightClick::Menu) {
        if (const auto cell = cell_at(event->position().toPoint())) {
            try {
                const auto clicked = document_->rows()[cell->row].id;
                std::vector<SequenceId> ids;
                const bool others = rightClick_ == RightClick::InsertOthers || rightClick_ == RightClick::DeleteOthers;
                if (others) { for (const auto& row : document_->rows()) if (row.id != clicked) ids.push_back(row.id); }
                else ids.push_back(clicked);
                document_->edit_gaps(ids, cell->column,
                    rightClick_ == RightClick::InsertGap || rightClick_ == RightClick::InsertOthers);
                notify_edit();
            } catch (const std::exception& e) { if (message) message(QString::fromUtf8(e.what())); }
        }
        event->accept(); return;
    }
    if (!document_ || event->button() != Qt::LeftButton) {
        QAbstractScrollArea::mousePressEvent(event); return;
    }
    setFocus();
    const auto point = event->position().toPoint();
    if (point.y() >= rulerHeight_ && point.x() < nameWidth_) {
        const auto r = static_cast<std::size_t>((point.y() - rulerHeight_) / rowHeight_ + verticalScrollBar()->value());
        if (r >= document_->rows().size()) {
            selected_.clear(); notify_selection(); return;
        }
        const auto id = document_->rows()[r].id;
        if (event->modifiers().testFlag(Qt::ShiftModifier)) {
            if (!event->modifiers().testFlag(Qt::ControlModifier)) selected_.clear();
            const auto anchor = std::min(selectionAnchor_, document_->rows().size() - 1);
            for (auto i = std::min(anchor, r); i <= std::max(anchor, r); ++i)
                selected_.insert(document_->rows()[i].id);
        } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (selected_.contains(id)) selected_.erase(id); else selected_.insert(id);
            selectionAnchor_ = r;
        } else {
            if(selected_.contains(id))collapseNameSelection_=id;
            else{selected_.clear();selected_.insert(id);}
            selectionAnchor_ = r;
        }
        selectionAnchor_=std::min(selectionAnchor_,document_->rows().size()-1);selectionAnchorId_=document_->rows()[selectionAnchor_].id;
        if(selected_.contains(id)){rowDragStart_=point;rowDragIds_=selected_ids();}
        notify_selection();
    } else if (point.y() < rulerHeight_ && point.x() >= nameWidth_ && !document_->rows().empty()) {
        const auto column = static_cast<std::size_t>((point.x() - nameWidth_) / cellWidth_ + horizontalScrollBar()->value());
        if (column >= document_->columns()) return;
        const auto anchor = event->modifiers().testFlag(Qt::ShiftModifier) && selectionOrigin_ ? selectionOrigin_->column : column;
        clear_fragment();
        dragAnchor_ = Cell{static_cast<std::size_t>(verticalScrollBar()->value()), anchor};
        selectionOrigin_ = dragAnchor_;
        rulerDrag_ = true;
        for (const auto& row : document_->rows()) dragRows_.push_back(row.id);
        select_fragment({dragRows_, std::min(anchor,column), std::max(anchor,column)-std::min(anchor,column)+1,true});
        dragPoint_ = point; dragScroll_->start();
    } else if (auto cell = cell_at(point)) {
        if (cell->column < document_->columns()) {
            if (event->modifiers().testFlag(Qt::ShiftModifier) && fragment_) {
                if (!selectionOrigin_) {
                    const auto firstId = fragment_->ids.front();
                    std::size_t row = 0; while (row < document_->rows().size() && document_->rows()[row].id != firstId) ++row;
                    selectionOrigin_ = Cell{std::min(row, document_->rows().size()-1), fragment_->start};
                }
                const auto origin = *selectionOrigin_;
                std::vector<SequenceId> ids;
                if (origin.row == cell->row) ids = {document_->rows()[origin.row].id};
                else for (auto row = std::min(origin.row, cell->row); row <= std::max(origin.row, cell->row); ++row)
                    ids.push_back(document_->rows()[row].id);
                select_fragment({std::move(ids), std::min(origin.column, cell->column),
                    std::max(origin.column, cell->column) - std::min(origin.column, cell->column) + 1});
                return;
            }
            if (mode_!=Mode::View && fragment_contains(*cell) && !event->modifiers().testFlag(Qt::ShiftModifier)) {
                dragAnchor_ = cell; draggingBlock_ = true; dragOffset_ = 0;
            } else {
                clear_fragment(); dragAnchor_ = cell; selectionOrigin_ = cell;
                const auto id = document_->rows()[cell->row].id;
                dragRows_ = {id};
                select_fragment({dragRows_, cell->column, 1});
            }
            dragPoint_ = point; dragScroll_->start();
            viewport()->update(); return;
        }
        clear_fragment();
        caretId_ = document_->rows()[cell->row].id;
        caretColumn_ = std::min(cell->column, document_->rows()[cell->row].symbols.size());
        viewport()->update();
    }
}
void AlignmentView::mouseDoubleClickEvent(QMouseEvent* event) {
    clear_row_drag();
    const auto point = event->position().toPoint();
    if(event->button()==Qt::LeftButton&&on_name_border(point.x())){event->accept();return;}
    if (document_ && event->button() == Qt::LeftButton && point.x() >= 0 && point.x() < nameWidth_ && point.y() >= rulerHeight_) {
        const auto row = static_cast<std::size_t>((point.y() - rulerHeight_) / rowHeight_ + verticalScrollBar()->value());
        if (row < document_->rows().size()) {
            const auto id = document_->rows()[row].id;
            selected_.clear(); selected_.insert(id); selectionAnchor_ = row;selectionAnchorId_=id;
            notify_selection();
            if (rename_requested) rename_requested(id);
        } else { selected_.clear(); notify_selection(); }
        event->accept(); return;
    }
    if (document_ && event->button()==Qt::LeftButton) {
        if (const auto cell=cell_at(point)) {
            const auto& row=document_->rows()[cell->row]; const auto& text=row.symbols.text();
            if (cell->column<text.size()) {
                                const bool gaps=is_gap(text[cell->column]); auto begin=cell->column, end=begin+1;
                while (begin && is_gap(text[begin-1])==gaps) --begin;
                while (end<text.size() && is_gap(text[end])==gaps) ++end;
                clear_fragment(); selectionOrigin_=Cell{cell->row,begin};
                select_fragment({{row.id},begin,end-begin}); event->accept(); return;
            }
        }
    }
    QAbstractScrollArea::mouseDoubleClickEvent(event);
}
void AlignmentView::mouseMoveEvent(QMouseEvent* event) {
    const auto point = event->position().toPoint();
    if(rowDragStart_&&event->buttons().testFlag(Qt::LeftButton)){
        if(draggingRows_||(point-*rowDragStart_).manhattanLength()>=QApplication::startDragDistance()){
            draggingRows_=true;collapseNameSelection_.reset();viewport()->setCursor(Qt::ClosedHandCursor);
            QToolTip::hideText();dragPoint_=point;update_row_drop(point);dragScroll_->start();event->accept();return;
        }
    }
    if(resizingNames_&&event->buttons().testFlag(Qt::LeftButton)){
        resize_name_area(point.x()-nameResizeOffset_+1);QToolTip::hideText();event->accept();return;
    }
    if(!event->buttons().testFlag(Qt::LeftButton)&&on_name_border(point.x())){viewport()->setCursor(Qt::SplitHCursor);QToolTip::hideText();return;}
    viewport()->unsetCursor();
    if (dragAnchor_ && event->buttons().testFlag(Qt::LeftButton)) {
        QToolTip::hideText(); update_fragment_drag(point); event->accept(); return;
    }
    if (const auto cell = cell_at(point)) {
        const auto& row = document_->rows()[cell->row];
        if (cell->column < row.symbols.size()) {
            const bool gap = is_gap(row.symbols.text()[cell->column]);
            const auto position = row.symbols.residue_rank(cell->column + 1);
            auto info = utf8(row.name) + "\nType: " + QString::fromLatin1(row_kind_name(row.kind).data()) + (row.kind==RowKind::Nucleotide?QString{}:QString(" (excluded from nucleotide analyses)")) + "\nColumn " + QString::number(static_cast<qulonglong>(cell->column + 1)) +
                (gap ? "\nGap; residues before gap: " : "\nResidue ") +
                QString::number(static_cast<qulonglong>(position)) + " / " +
                QString::number(static_cast<qulonglong>(row.symbols.residue_count()));
            if(translationEnabled_&&translation_&&translation_->content==document_->content_id()&&cell->column<translation_->map.at.size()){const auto slot=translation_->map.at[cell->column];if(slot>=0)if(const auto* translated=translation_->row(row.id)){const auto& span=translation_->map.codons[static_cast<std::size_t>(slot)];const auto call=translated->calls[static_cast<std::size_t>(slot)];info+=QString("\nTranslation: %1; coding columns %2–%3%4\nCtrl+C copies underlying nucleotides").arg(QChar::fromLatin1(call.amino)).arg(span.first()+1).arg(span.last()+1).arg(span.cells<3?" (incomplete trailing codon)":"");if(call.flags&ContextStop)info+="\nContext-dependent termination; displayed as sense amino acid";if(call.flags&PossibleStop)info+="\nAmbiguity includes a possible stop";}}
            if(auto span=protein_span(row,cell->column))info+=QString("\nAmino-acid token: %1; display cells %2–%3").arg(QChar::fromLatin1(row.symbols.text()[span->first()])).arg(span->first()+1).arg(span->last()+1);
            QToolTip::showText(event->globalPosition().toPoint(), info, viewport());
            return;
        }
    } else if (document_ && point.y() >= rulerHeight_ && point.x() < nameWidth_) {
        const auto r = static_cast<std::size_t>((point.y() - rulerHeight_) / rowHeight_ + verticalScrollBar()->value());
        if (r < document_->rows().size()) {
            const auto& row=document_->rows()[r];
            QToolTip::showText(event->globalPosition().toPoint(), utf8(row.name)+"\nType: "+QString::fromLatin1(row_kind_name(row.kind).data())+(row.kind==RowKind::Protein?" (one-letter amino-acid codes)":"")+"\nGroup: "+(document_->group(row.groupId)?utf8(document_->group(row.groupId)->name):QString("Ungrouped"))+(row.trace?"\nLinked chromatogram: "+utf8(row.trace->original->sourcePath):""), viewport());
            return;
        }
    }
    QToolTip::hideText();
}
void AlignmentView::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button()==Qt::LeftButton&&rowDragStart_){
        if(draggingRows_){update_row_drop(event->position().toPoint());const auto ids=rowDragIds_;const auto target=rowDropIndex_;clear_row_drag();
            try{document_->move_rows(ids,target);notify_edit();}catch(const std::exception&e){if(message)message(QString::fromUtf8(e.what()));}}
        else{const auto collapse=collapseNameSelection_;clear_row_drag();if(collapse){selected_.clear();selected_.insert(*collapse);notify_selection();}}
        event->accept();return;
    }
    if(event->button()==Qt::LeftButton&&resizingNames_){
        resize_name_area(event->position().toPoint().x()-nameResizeOffset_+1);resizingNames_=false;
        if(!on_name_border(event->position().toPoint().x()))viewport()->unsetCursor();
        event->accept();return;
    }
    if (event->button() == Qt::LeftButton && dragAnchor_) {
        std::optional<std::int64_t> movement;
        if (draggingBlock_) {
            update_fragment_drag(event->position().toPoint());
            movement = dragOffset_;
        }
        // Finish the gesture before committing: edit notifications can resize
        // or repaint the view, and must not resume selection dragging.
        dragScroll_->stop(); dragAnchor_.reset(); dragRows_.clear(); rulerDrag_ = false;
        draggingBlock_ = false; dragOffset_ = 0;
        if(movement)slide_fragment(*movement);
        viewport()->update(); event->accept(); return;
    }
    QAbstractScrollArea::mouseReleaseEvent(event);
}
void AlignmentView::update_row_drop(const QPoint& point){
    if(!document_)return;
    const auto at=verticalScrollBar()->value()+static_cast<int>(std::floor(static_cast<double>(point.y()-rulerHeight_+rowHeight_/2)/rowHeight_));
    rowDropIndex_=static_cast<std::size_t>(std::clamp(at,0,static_cast<int>(document_->rows().size())));viewport()->update();
}
void AlignmentView::clear_row_drag(){
    if(draggingRows_&&dragScroll_)dragScroll_->stop();
    rowDragStart_.reset();collapseNameSelection_.reset();rowDragIds_.clear();draggingRows_=false;viewport()->unsetCursor();viewport()->update();
}
std::vector<SequenceId> AlignmentView::selected_ids() const {
    std::vector<SequenceId> result;
    if (document_&&!selected_.empty()) for (const auto& row : document_->rows()) if (selected_.contains(row.id)) result.push_back(row.id);
    return result;
}
void AlignmentView::select_all() {
    if (document_) for (const auto& row : document_->rows()) selected_.insert(row.id);
    notify_selection();
}
void AlignmentView::unselect_all() { selected_.clear(); notify_selection(); }
void AlignmentView::invert_selection() {
    if (document_) for (const auto& row : document_->rows()) {
        if (selected_.contains(row.id)) selected_.erase(row.id); else selected_.insert(row.id);
    }
    notify_selection();
}
void AlignmentView::select_ids(const std::vector<SequenceId>& ids) {
    selected_.clear();
    if (document_) {
        const std::unordered_set<SequenceId> requested(ids.begin(), ids.end());
        for (const auto& row : document_->rows()) if (requested.contains(row.id)) selected_.insert(row.id);
    }
    notify_selection();
}
void AlignmentView::extend_fragment(bool toEnd) {
    if(!document_||!fragment_)return;
    auto next=*fragment_;
    if(toEnd)next.width=document_->columns()-next.start;
    else{next.width+=next.start;next.start=0;}
    select_fragment(std::move(next));
    horizontalScrollBar()->setValue(toEnd?horizontalScrollBar()->maximum():0);
}
void AlignmentView::reveal_first_selected() {
    if(!document_||selected_.empty())return;
    const auto& rows=document_->rows();
    for(std::size_t i=0;i<rows.size();++i)if(selected_.contains(rows[i].id)){
        const auto first=static_cast<std::size_t>(verticalScrollBar()->value());
        if(i<first||i>=first+static_cast<std::size_t>(visible_rows()))verticalScrollBar()->setValue(static_cast<int>(std::min<std::size_t>(i,std::numeric_limits<int>::max())));
        return;
    }
}
void AlignmentView::contextMenuEvent(QContextMenuEvent* event) {
    if (event->pos().x() >= nameWidth_) {
        if (rightClick_ != RightClick::Menu || !fragment_) return;
        const auto cell = cell_at(event->pos());
        if (cell && !fragment_contains(*cell)) return;
        QMenu menu(this);
        auto* copy=menu.addAction("Copy",[this]{if(copy_fragment_requested)copy_fragment_requested();});copy->setEnabled(selected_residue_text().has_value());shortcut_hint(copy,QKeySequence::Copy);
        if(mode_==Mode::Insert||mode_==Mode::Rewrite){
            auto* cut=menu.addAction("Cut",[this]{if(cut_fragment_requested)cut_fragment_requested();});cut->setEnabled(selected_residue_text().has_value()&&complete_selection());shortcut_hint(cut,QKeySequence::Cut);
            auto* paste=menu.addAction("Paste",[this]{if(paste_fragment_requested)paste_fragment_requested();});paste->setEnabled(complete_selection()&&fragment_->ids.size()==1&&(mode_==Mode::Rewrite||insertable_selection()));shortcut_hint(paste,QKeySequence::Paste);
        }
        if(copy_translation_requested){auto* translated=menu.addAction("Copy translated fragment",[this]{copy_translation_requested();});translated->setEnabled(can_copy_translation());}
        menu.addSeparator();
        if (mode_!=Mode::View) {
            shortcut_hint(menu.addAction("Move left one column", [this] { slide_fragment(-1); }),QKeySequence(Qt::CTRL|Qt::Key_Left));
            shortcut_hint(menu.addAction("Move right one column", [this] { slide_fragment(1); }),QKeySequence(Qt::CTRL|Qt::Key_Right));
            shortcut_hint(menu.addAction("Move left as far as possible", [this] { slide_fragment(-std::numeric_limits<std::int64_t>::max()); }),QKeySequence(Qt::SHIFT|Qt::Key_Left));
            shortcut_hint(menu.addAction("Move right as far as possible", [this] { slide_fragment(std::numeric_limits<std::int64_t>::max()); }),QKeySequence(Qt::SHIFT|Qt::Key_Right));
            menu.addSeparator();
        }
        if (mode_!=Mode::View) {
            menu.addAction("Lock gaps in selection",[this]{lock_fragment();});
            menu.addAction("Unlock gaps in selection",[this]{unlock_fragment();});
            menu.addAction("Replace selected symbols with Ns", [this] { fill_fragment('N'); });
            menu.addAction("Replace selected symbols with open gaps (~)", [this] { fill_fragment('~'); });
            menu.addAction("Replace selected symbols with locked gaps (-)", [this] { fill_fragment('-'); });
            menu.addSeparator();
        }
        menu.addAction("Select to beginning",[this]{extend_fragment(false);})->setToolTip("Extend the selected rows to alignment column 1.");
        menu.addAction("Select to end",[this]{extend_fragment(true);})->setToolTip("Extend the selected rows to the final alignment column.");
        shortcut_hint(menu.addAction("Clear fragment selection", [this] { clear_fragment(); viewport()->update(); }),QKeySequence(Qt::Key_Escape));
        menu.exec(event->globalPos()); return;
    }
    if(document_&&event->pos().y()>=rulerHeight_){
        const auto row=static_cast<std::size_t>((event->pos().y()-rulerHeight_)/rowHeight_+verticalScrollBar()->value());
        if(row<document_->rows().size()&&!selected_.contains(document_->rows()[row].id)){
            selected_.clear();selected_.insert(document_->rows()[row].id);selectionAnchor_=row;notify_selection();}
    }
    QMenu menu(this);
    auto* cutRows=menu.addAction("Cut selected sequences",[this]{if(cut_rows_requested)cut_rows_requested();});cutRows->setEnabled(!selected_.empty());shortcut_hint(cutRows,QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_X));
    auto* copyRows=menu.addAction("Copy selected sequences",[this]{if(copy_rows_requested)copy_rows_requested();});copyRows->setEnabled(!selected_.empty());shortcut_hint(copyRows,QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_C));
    shortcut_hint(menu.addAction("Paste sequences", [this] { if (paste_requested) paste_requested(); }),QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_V));
    auto* remove = menu.addAction("Delete selected sequences", [this] { if (delete_rows_requested) delete_rows_requested(); });
    remove->setEnabled(!selected_.empty());shortcut_hint(remove,QKeySequence(Qt::CTRL|Qt::Key_Delete));
    menu.addSeparator();
    shortcut_hint(menu.addAction("Select all", [this] { select_all(); }),QKeySequence::SelectAll);
    shortcut_hint(menu.addAction("Unselect all", [this] { unselect_all(); }),QKeySequence(Qt::Key_Escape));
    menu.addAction("Invert selection", [this] { invert_selection(); });
    menu.addSeparator();
    if(add_translation_requested){auto* translate=menu.addAction("Add translated sequences…",[this]{add_translation_requested();});translate->setEnabled(!selected_.empty());}
    if(annotate_mask_requested){SequenceId mask=0;if(document_&&event->pos().y()>=rulerHeight_){auto row=static_cast<std::size_t>((event->pos().y()-rulerHeight_)/rowHeight_+verticalScrollBar()->value());if(row<document_->rows().size()&&document_->rows()[row].kind==RowKind::Mask)mask=document_->rows()[row].id;}
        auto* annotate=menu.addAction("Annotate using this mask",[this,mask]{if(mask)annotate_mask_requested(mask);});annotate->setEnabled(mask!=0);}
    menu.addSeparator();
    auto* link=menu.addAction("Link trace (AB1)…",[this]{if(link_trace_requested)link_trace_requested();});link->setEnabled(!selected_.empty());
    auto* unlink=menu.addAction("Delete trace…",[this]{if(delete_trace_requested)delete_trace_requested();});unlink->setEnabled(!selected_.empty());menu.addSeparator();
    menu.addAction("NCBI nucleotide BLAST…",[this]{if(blast_requested)blast_requested(false);})->setEnabled(!selected_.empty());
    menu.addSeparator();
    auto* addGroup=menu.addAction("Add to group…",[this]{if(add_group_requested)add_group_requested();});addGroup->setEnabled(!selected_.empty());
    auto* removeGroup=menu.addAction("Remove from group",[this]{if(remove_group_requested)remove_group_requested();});removeGroup->setEnabled(!selected_.empty());
    menu.exec(event->globalPos());
}
void AlignmentView::wheelEvent(QWheelEvent* event) {
    const auto angle = event->angleDelta();
    const auto pixel = event->pixelDelta();
    int delta = angle.y() != 0 ? angle.y() : angle.x();
    int step = delta == 0 ? 0 : -static_cast<int>(std::round(static_cast<double>(delta) / 120.0 * 3.0));
    if (delta != 0 && step == 0) step = delta > 0 ? -1 : 1;
    if (delta == 0) {
        const auto movement = pixel.y() != 0 ? pixel.y() : pixel.x();
        if (movement) step = movement > 0 ? -1 : 1;
    }
    auto* bar = event->modifiers().testFlag(Qt::ControlModifier) || (angle.y() == 0 && angle.x() != 0)
        ? horizontalScrollBar() : verticalScrollBar();
    if(event->modifiers().testFlag(Qt::ControlModifier))step*=2;
    bar->setValue(bar->value() + step);
    event->accept();
}
void AlignmentView::ensure_caret_visible() {
    if (!document_ || !caretId_) return;
    const auto& rows = document_->rows();
    const auto found = std::find_if(rows.begin(), rows.end(), [this](const auto& r) { return r.id == *caretId_; });
    if (found == rows.end()) return;
    const int r = static_cast<int>(std::distance(rows.begin(), found));
    auto* v = verticalScrollBar(); auto* h = horizontalScrollBar();
    if (r < v->value()) v->setValue(r);
    else if (r >= v->value() + visible_rows()) v->setValue(r - visible_rows() + 1);
    const int c = static_cast<int>(std::min<std::size_t>(caretColumn_, std::numeric_limits<int>::max()));
    if (c < h->value()) h->setValue(c);
    else if (c >= h->value() + visible_columns()) h->setValue(c - visible_columns() + 1);
}
void AlignmentView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        clear_row_drag(); clear_fragment(); selected_.clear(); caretId_.reset();
        notify_selection(); event->accept(); return;
    }
    if (!document_) { QAbstractScrollArea::keyPressEvent(event); return; }
    const bool horizontal=event->key()==Qt::Key_Left||event->key()==Qt::Key_Right;
    const auto modifiers=event->modifiers();
    if(mode_!=Mode::View&&fragment_&&horizontal&&
       !modifiers.testFlag(Qt::AltModifier)&&!modifiers.testFlag(Qt::MetaModifier)&&
       (modifiers==Qt::ShiftModifier||modifiers==Qt::ControlModifier)){
        const auto distance=modifiers==Qt::ShiftModifier?std::numeric_limits<std::int64_t>::max():1;
        slide_fragment(event->key()==Qt::Key_Left?-distance:distance);
        const auto& rows=document_->rows();const auto id=fragment_->ids.front();
        for(std::size_t i=0;i<rows.size();++i)if(rows[i].id==id){
            const auto first=static_cast<std::size_t>(verticalScrollBar()->value());
            if(i<first||i>=first+static_cast<std::size_t>(visible_rows()))verticalScrollBar()->setValue(static_cast<int>(i));
            break;
        }
        const auto first=static_cast<std::size_t>(horizontalScrollBar()->value());
        if(fragment_->start<first)horizontalScrollBar()->setValue(static_cast<int>(fragment_->start));
        else if(fragment_->start>=first+static_cast<std::size_t>(visible_columns()))horizontalScrollBar()->setValue(static_cast<int>(fragment_->start)-visible_columns()+1);
        event->accept();return;
    }
    try {
        const bool command = event->modifiers().testFlag(Qt::ControlModifier) ||
            event->modifiers().testFlag(Qt::AltModifier) || event->modifiers().testFlag(Qt::MetaModifier);
        if (!command && (event->key()==Qt::Key_Delete || event->key()==Qt::Key_Backspace)) {
            delete_residues(event->key()==Qt::Key_Backspace); event->accept(); return;
        }
        const bool arrow = event->key()==Qt::Key_Left || event->key()==Qt::Key_Right ||
            event->key()==Qt::Key_Up || event->key()==Qt::Key_Down ||
            event->key()==Qt::Key_Home || event->key()==Qt::Key_End;
        if (arrow) {
            // Any rectangular selection can collapse to an unambiguous caret.
            const bool stretch = fragment_ && ((fragment_->width>1&&!insertable_selection()) || fragment_->ids.size()>1);
            if (fragment_) {
                caretId_=fragment_->ids.front(); caretColumn_=fragment_->start;
                if (event->key()==Qt::Key_Right && stretch) caretColumn_+=fragment_->width;
            }
            if (!caretId_) { QAbstractScrollArea::keyPressEvent(event); return; }
            const auto size=document_->sequence(*caretId_).symbols.size();
            caretColumn_=std::min(caretColumn_,size);
            switch (event->key()) {
            case Qt::Key_Left: if (!stretch && caretColumn_) --caretColumn_; break;
            case Qt::Key_Right: if (!stretch){if(auto span=protein_span(document_->sequence(*caretId_),caretColumn_))caretColumn_=span->last()+1;else caretColumn_=std::min(caretColumn_+1,size);} break;
            case Qt::Key_Home: caretColumn_=0; break;
            case Qt::Key_End: caretColumn_=size; break;
            case Qt::Key_Up: case Qt::Key_Down: {
                const auto& rows=document_->rows();
                auto it=std::find_if(rows.begin(),rows.end(),[this](const auto& row){return row.id==*caretId_;});
                auto index=static_cast<std::size_t>(it-rows.begin());
                if (event->key()==Qt::Key_Up && index) --index;
                if (event->key()==Qt::Key_Down && index+1<rows.size()) ++index;
                caretId_=rows[index].id; caretColumn_=std::min(caretColumn_,rows[index].symbols.size()); break;
            }
            default: break;
            }
            const auto id=*caretId_; const auto column=caretColumn_;
            clear_fragment(); caretId_=id; caretColumn_=column;
            if (column<document_->sequence(id).symbols.size()) select_fragment({{id},column,1});
            else notify_selection();
            ensure_caret_visible(); viewport()->update(); event->accept(); return;
        }
        if (!command) {
            const auto typed=event->text().toUtf8();
            if (write_residues(std::string_view(typed.constData(),static_cast<std::size_t>(typed.size())))) {
                event->accept(); return;
            }
        }
        QAbstractScrollArea::keyPressEvent(event);
    } catch (const std::exception& e) { if (message) message(QString::fromUtf8(e.what())); }
}
}
namespace kleiotide {
void AlignmentView::set_translation(std::shared_ptr<const TranslationResult> t,bool enabled,bool amino,bool ruler){const bool translationChanged=translation_!=t||translationEnabled_!=enabled||aminoLetters_!=amino;const bool metrics=codonRuler_!=(enabled&&ruler);translation_=std::move(t);translationEnabled_=enabled;aminoLetters_=amino;codonRuler_=enabled&&ruler;if(translationChanged)tiles_.clear();if(metrics){update_metrics();clear_render_cache();refresh();}if(translationChanged||metrics)viewport()->update();}
bool AlignmentView::complete_selection()const{if(!document_||!fragment_)return true;const std::unordered_set<SequenceId> selected(fragment_->ids.begin(),fragment_->ids.end());for(const auto& row:document_->rows())if(selected.contains(row.id)&&!complete_protein_selection(row,fragment_->start,fragment_->width))return false;return true;}
bool AlignmentView::insertable_selection()const{if(!fragment_)return true;if(fragment_->ids.size()!=1)return false;if(fragment_->width==1)return true;auto span=protein_span(document_->sequence(fragment_->ids.front()),fragment_->start);return span&&span->first()==fragment_->start&&span->last()+1==fragment_->start+fragment_->width;}

}

namespace kleiotide {
bool AlignmentView::can_copy_translation()const{return expanded_translation_fragment().has_value();}
std::optional<AlignmentView::Fragment> AlignmentView::expanded_translation_fragment()const{
    if(!document_||!translationEnabled_||!translation_||translation_->content!=document_->content_id()||translation_->settings!=document_->translation_settings()||!fragment_||fragment_->ids.size()!=1)return {};
    const auto& f=*fragment_;
    if(document_->sequence(f.ids.front()).kind!=RowKind::Nucleotide||!translation_->row(f.ids.front())||translation_->map.phaseErrors)return {};
    const auto end=f.start+f.width;const auto& map=translation_->map;
    if(f.start>=map.at.size()||end>map.at.size()||map.partial[f.start]||map.partial[end-1])return {};
    auto overlaps=[&](const ProteinSpan& span){for(unsigned i=0;i<span.cells;++i)if(span.columns[i]>=f.start&&span.columns[i]<end)return true;return false;};
    auto first=std::lower_bound(map.codons.begin(),map.codons.end(),f.start,[](const auto& span,auto column){return span.last()<column;});
    if(first!=map.codons.end()&&!overlaps(*first))++first;
    if(first==map.codons.end()||first->first()>=end||!overlaps(*first))return {};
    auto after=std::lower_bound(first,map.codons.end(),end,[](const auto& span,auto column){return span.first()<column;});
    if(after!=first&&!overlaps(*(after-1)))--after;
    if(after==first)return {};
    auto expanded=f;expanded.start=std::min(f.start,first->first());expanded.width=std::max(end,(after-1)->last()+1)-expanded.start;return expanded;
}
}
