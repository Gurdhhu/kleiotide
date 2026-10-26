#pragma once
#include "core/row_selection.hpp"
#include "core/motif_index.hpp"
#include <QWidget>
#include <atomic>
#include <memory>
class QLineEdit;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
namespace kleiotide {
class MotifNavigator final:public QWidget {
public:
    explicit MotifNavigator(QWidget* parent=nullptr);
    ~MotifNavigator() override;
    void set_document(std::shared_ptr<const Document>,const std::vector<SequenceId>& selected);
    void set_available(bool);
    void focus_query();
    void next(bool backwards=false);
    void cancel();
    void close_results();
    bool searching()const noexcept{return searching_;}
    const std::optional<MotifHit>& current_hit()const{return hit_;}
    std::shared_ptr<const MotifIndex> results()const{return index_;}
    std::function<void(bool)> searching_changed;
    std::function<void(std::optional<MotifHit>)> highlighted;
    std::function<void(MotifHit)> select_requested;
    std::function<void(std::shared_ptr<const MotifIndex>)> results_changed;
private:
    void invalidate(const QString&,bool clear=false);
    void show_current(bool wrapped=false);
    void search(bool backwards,bool restart);
    void update_controls();
    std::shared_ptr<const Document> document_;
    std::vector<SequenceId> selected_;
    std::uint64_t revision_=0;
    std::optional<MotifHit> hit_;
    std::shared_ptr<const MotifIndex> index_;
    std::uint64_t current_=0;
    std::shared_ptr<std::atomic_bool> cancelled_;
    bool available_=true,searching_=false,dirty_=true;
    QLineEdit* query_=nullptr;
    QCheckBox *ignoreGaps_=nullptr,*reverse_=nullptr,*compatible_=nullptr,*wrap_=nullptr,*regex_=nullptr,*excludeNOnly_=nullptr;
    QComboBox* scope_=nullptr;
    QLabel* result_=nullptr;
    QLabel* counts_=nullptr;
    QPushButton *find_=nullptr,*previous_=nullptr,*next_=nullptr,*cancel_=nullptr;
};
}
