#pragma once
#include "core/overview.hpp"
#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <functional>
#include <memory>
namespace kleiotide {
class AlignmentView;
class AlignmentOverview final:public QWidget {
public:
    AlignmentOverview(AlignmentView* view,QWidget* parent=nullptr);
    void set_document(const Document* document);
    void refresh();
    bool ready() const;
    const std::vector<OverviewBin>& displayed_bins() const {return bins_;}
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
private:
    void begin_scan();
    void advance_scan();
    void rebuild_plot();
    QRect plot_rect() const;
    void navigate(const QPoint& point);
    const Document* document_=nullptr;
    AlignmentView* view_;
    std::unique_ptr<OverviewScan> scan_;
    std::vector<OverviewBin> bins_;
    QPixmap plot_;
    QTimer debounce_,work_;
    std::uint64_t requested_=0;
    qreal dpr_=0;
    QString error_;
};
}
