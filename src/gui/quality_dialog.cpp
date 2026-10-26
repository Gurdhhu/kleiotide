#include "gui/quality_dialog.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
#include <array>
namespace kleiotide {
namespace {
QString text(const std::string& value){return QString::fromUtf8(value.data(),static_cast<qsizetype>(value.size()));}
struct PreviewResult {QualityPlan plan;QString error;};
class QualityDialog final:public QDialog {
public:
    QualityDialog(std::shared_ptr<Document> source,std::vector<SequenceId> selected,QualityOperation operation,QWidget* parent)
        :QDialog(parent),source_(std::move(source)),selected_(std::move(selected)),operation_(operation){
        const bool trim=operation==QualityOperation::TrimEnds;setObjectName("traceQualityDialog");setWindowTitle(trim?"Trim low quality ends":"Low-quality calls to N");resize(1040,660);
        auto* layout=new QVBoxLayout(this);auto* form=new QFormLayout;layout->addLayout(form);
        scope_=new QComboBox(this);scope_->setObjectName("qualityScope");scope_->addItem(QString("Selected sequence names: %1").arg(selected_.size()));scope_->addItem("All sequences with traces");form->addRow("Apply to",scope_);
        QSettings saved("Kleiotide","Kleiotide");threshold_=new QSpinBox(this);threshold_->setObjectName("qualityThreshold");threshold_->setRange(0,255);threshold_->setValue(saved.value("quality/threshold",20).toInt());form->addRow(trim?"Minimum mean Phred Q":"Convert calls with Phred Q below",threshold_);
        window_=new QSpinBox(this);window_->setObjectName("qualityWindow");window_->setRange(1,10000);window_->setValue(saved.value("quality/window",10).toInt());if(trim)form->addRow("Window length (non-gap symbols)",window_);else window_->hide();
        edited_=new QCheckBox("Use original-call Q for substituted symbols",this);edited_->setObjectName("qualityUseOriginalQ");edited_->setToolTip("Edited means the current symbol differs from its linked imported call, including earlier cleanup replacements. Use that original call's Q as evidence, not a new measurement for the edited symbol. Inserted calls still have no Q.");layout->addWidget(edited_);
        auto* policy=new QLabel(trim?
            "Scoring: arithmetic mean Phred Q. Keep the interval from the first through the last complete window meeting the threshold; internal low-quality islands remain. Trimmed calls become open gaps (~), preserving every column and existing gap states. Rows without a passing window are left unchanged.":
            "Only calls strictly below the threshold become N (n for lowercase calls). Existing N/n and all gap states keep their positions. Original trace signals and quality values are never changed.",this);policy->setWordWrap(true);layout->addWidget(policy);
        auto* protectedPolicy=new QLabel("Calls without usable Q are always retained. By default, substituted symbols are protected too. Trimming expands its retained interval to include protected calls; each passing window needs Q for every non-gap symbol. No selection does not imply all sequences.",this);protectedPolicy->setWordWrap(true);if(!trim)protectedPolicy->setText("Calls without usable Q and, by default, substituted symbols are left unchanged. Inserted calls never acquire a measured Q. No selection does not imply all sequences.");layout->addWidget(protectedPolicy);
        table_=new QTableWidget(this);table_->setObjectName("qualityPreview");table_->setColumnCount(10);table_->setHorizontalHeaderLabels({"Sequence","Result","Calls","Q used","No Q","Edited protected","Changed","Trim left","Trim right","Retained calls"});table_->setEditTriggers(QAbstractItemView::NoEditTriggers);table_->setSelectionBehavior(QAbstractItemView::SelectRows);table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);table_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Interactive);table_->setColumnWidth(0,190);table_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);for(int c=7;c<10;++c)table_->setColumnHidden(c,!trim);layout->addWidget(table_,1);
        status_=new QLabel(this);status_->setObjectName("qualityStatus");status_->setWordWrap(true);layout->addWidget(status_);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel,this);layout->addWidget(buttons);apply_=buttons->button(QDialogButtonBox::Apply);apply_->setObjectName("qualityApply");preview_=buttons->addButton("Preview",QDialogButtonBox::ActionRole);preview_->setObjectName("qualityPreviewButton");preview_->setDefault(true);cancel_=buttons->button(QDialogButtonBox::Cancel);
        connect(buttons,&QDialogButtonBox::rejected,this,&QualityDialog::reject);connect(preview_,&QPushButton::clicked,this,[this]{preview();});
        connect(apply_,&QPushButton::clicked,this,[this]{if(!plan_||running_)return;try{const auto before=source_->state_id();apply_trace_quality(*source_,*plan_);changed=source_->state_id()!=before;QSettings settings("Kleiotide","Kleiotide");settings.setValue("quality/threshold",threshold_->value());settings.setValue("quality/window",window_->value());accept();}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));plan_.reset();apply_->setEnabled(false);}});
        connect(scope_,&QComboBox::currentIndexChanged,this,[this]{invalidate();});connect(threshold_,&QSpinBox::valueChanged,this,[this]{invalidate();});connect(window_,&QSpinBox::valueChanged,this,[this]{invalidate();});connect(edited_,&QCheckBox::toggled,this,[this]{invalidate();});invalidate();
    }
    bool changed=false;
protected:
    void reject()override{
        if(running_){closing_=true;cancelled_->store(true);cancel_->setEnabled(false);status_->setText("Cancelling preview…");return;}
        QDialog::reject();
    }
private:
    void invalidate(){plan_.reset();table_->setRowCount(0);apply_->setEnabled(false);const bool valid=scope_->currentIndex()==1||!selected_.empty();preview_->setEnabled(valid);status_->setText(valid?"Choose settings, then Preview. Apply uses only the displayed preview.":"No sequence names selected. Choose All sequences with traces explicitly, or cancel and select names first.");}
    void preview(){
        if(running_||(scope_->currentIndex()==0&&selected_.empty()))return;
        plan_.reset();running_=true;closing_=false;cancelled_=std::make_shared<std::atomic_bool>(false);apply_->setEnabled(false);preview_->setEnabled(false);for(auto* widget:std::array<QWidget*,4>{scope_,threshold_,window_,edited_})widget->setEnabled(false);status_->setText("Computing quality preview…");
        QualitySettings settings{operation_,scope_->currentIndex()==0?QualityScope::Selected:QualityScope::AllTraces,static_cast<unsigned>(threshold_->value()),static_cast<unsigned>(window_->value()),edited_->isChecked()};
        auto* watcher=new QFutureWatcher<PreviewResult>(this);const auto source=source_;const auto selected=selected_;const auto cancel=cancelled_;
        connect(watcher,&QFutureWatcher<PreviewResult>::finished,this,[this,watcher]{
            auto result=watcher->result();watcher->deleteLater();running_=false;for(auto* widget:std::array<QWidget*,4>{scope_,threshold_,window_,edited_})widget->setEnabled(true);preview_->setEnabled(true);
            if(closing_){QDialog::reject();return;}
            if(!result.error.isEmpty()){status_->setText(result.error);return;}
            plan_=std::move(result.plan);table_->setRowCount(static_cast<int>(plan_->rows.size()));
            for(std::size_t i=0;i<plan_->rows.size();++i){const auto& row=plan_->rows[i];const auto retained=row.calls?QString("%1–%2").arg(row.keepBegin+1).arg(row.keepEnd):QString("—");
                const QString values[]={QString("%1. %2").arg(row.row).arg(text(row.name)),text(row.status),QString::number(row.calls),QString::number(row.scored),QString::number(row.missingQ),QString::number(row.protectedEdited),QString::number(row.changed),QString::number(row.trimLeft),QString::number(row.trimRight),retained};
                for(int c=0;c<10;++c){auto* item=new QTableWidgetItem(values[c]);item->setToolTip(values[c]);table_->setItem(static_cast<int>(i),c,item);}
            }
            status_->setText(QString("%1 of %2 trace-linked rows will change · %3 symbols · %4 ineligible rows ignored. Counts and retained intervals use current non-gap symbols; original signals/Q remain intact.").arg(plan_->changedRows).arg(plan_->eligibleRows).arg(plan_->changedSymbols).arg(plan_->ignoredRows));apply_->setEnabled(plan_->changedRows!=0);
        });
        watcher->setFuture(QtConcurrent::run([source,selected,settings,cancel]{PreviewResult result;try{result.plan=plan_trace_quality(*source,settings,selected,[cancel]{return cancel->load();});}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;}));
    }
    std::shared_ptr<Document> source_;std::vector<SequenceId> selected_;QualityOperation operation_;
    QComboBox* scope_;QSpinBox *threshold_,*window_;QCheckBox* edited_;QTableWidget* table_;QLabel* status_;QPushButton *preview_,*apply_,*cancel_;
    std::optional<QualityPlan> plan_;std::shared_ptr<std::atomic_bool> cancelled_;bool running_=false,closing_=false;
};
}
bool edit_trace_quality(std::shared_ptr<Document> source,const std::vector<SequenceId>& selected,QualityOperation operation,QWidget* parent){QualityDialog dialog(std::move(source),selected,operation,parent);dialog.exec();return dialog.changed;}
void MainWindow::quality_edit(bool trim){
    if(busy_)return;
    const auto ids=view_->selected_ids();set_busy(true,"Trace-quality editing…");
    const bool changed=edit_trace_quality(document_,ids,trim?QualityOperation::TrimEnds:QualityOperation::CallsToN,this);
    if(changed)view_->refresh();
    set_busy(false);
}
}
