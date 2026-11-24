#pragma once
#include "core/overview.hpp"
#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <memory>
namespace kleiotide {
class AlignmentView;
class AlignmentOverview final:public QWidget {
public:
    AlignmentOverview(AlignmentView* view,QWidget* parent=nullptr);
    void set_document(const Document* document);
    void refresh();
    void update_palette();
    void set_limits(std::size_t workspace,int delayMs);
    bool ready() const;
    QSize sizeHint() const override { return {500,112}; }
    const std::vector<OverviewBin>& displayed_bins() const {return bins_;}
protected:
    bool event(QEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
private:
    void show_tooltip(const QPoint&,const QPoint&);
    void begin_scan();
    void advance_scan();
    void rebuild_plot();
    QRect plot_rect() const;
    void navigate(const QPoint& point);
    void finish_navigation();
    const Document* document_=nullptr;
    AlignmentView* view_;
    std::unique_ptr<OverviewScan> scan_;
    std::vector<OverviewBin> bins_;
    QPixmap plot_;
    QTimer debounce_,work_;
    std::uint64_t requested_=0;
    qreal dpr_=0;
    bool accessible_=false;
    bool navigationDrag_=false;
    QString error_;
    std::size_t workspace_=16*1024*1024;
};
}
