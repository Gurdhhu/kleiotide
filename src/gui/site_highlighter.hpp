#pragma once
#include "core/site_summary.hpp"
#include <QColor>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <functional>
#include <memory>
class QAction;
class QMenu;
class QWidget;
namespace kleiotide {
class AlignmentView;
class SiteHighlighter final:public QObject {
public:
    SiteHighlighter(AlignmentView*,QWidget* owner);
    void add_menu(QMenu*,std::vector<QAction*>& guardedActions);
    void set_document(const Document*);
    QJsonObject state()const;
    void restore(const QJsonObject&);
private:
    void invalidate();
    void advance();
    void publish();
    void sync_actions();
    void choose_scope();
    void choose_threshold(bool conservation);
    AlignmentView* view_;
    QWidget* owner_;
    const Document* source_=nullptr;
    std::unique_ptr<SiteScan> scan_;
    std::uint64_t revision_=std::numeric_limits<std::uint64_t>::max();
    AnalysisScope scope_;
    HighlightRules rules_;
    QColor color_{198,173,242};
    QTimer work_,debounce_;
    std::vector<QAction*> categories_;
    QAction *scopeAction_=nullptr,*coverageAction_=nullptr,*conservationAction_=nullptr;
    QAction* scopeSummary_=nullptr;
};
}
