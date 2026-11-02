#include "gui/file_output.hpp"
#include "gui/table_export.hpp"
#include "gui/identity_group_dialog.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/identity_groups.hpp"
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QSpinBox>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
#include <algorithm>
#include <QHBoxLayout>
#include <random>
#include <unordered_map>
namespace kleiotide {
namespace {
QString text(const std::string& s){return QString::fromUtf8(s.data(),static_cast<qsizetype>(s.size()));}
constexpr auto statusStyle="QLabel { background: #e7effa; color: #17212b; border: 2px solid #32669c; border-radius: 5px; padding: 10px; font-weight: bold; }";
class GroupingModel final:public QAbstractTableModel {
public:
    GroupingModel(const Document& d,QObject* parent):QAbstractTableModel(parent){for(const auto& row:d.rows())rows_[row.id]=&row;}
    std::shared_ptr<const IdentityGroupPlan> plan;
    void set(std::shared_ptr<const IdentityGroupPlan> value){beginResetModel();plan=std::move(value);names_.clear();if(plan)for(const auto& g:plan->groups)names_.emplace(g.id,text(g.name));endResetModel();}
    int rowCount(const QModelIndex& p={})const override{return p.isValid()||!plan?0:static_cast<int>(plan->members.size());}
    int columnCount(const QModelIndex& p={})const override{return p.isValid()?0:6;}
    QVariant headerData(int c,Qt::Orientation o,int role)const override{if(role!=Qt::DisplayRole)return {};if(o==Qt::Vertical)return c+1;const QStringList labels{"Sequence","Proposed group","Group centroid","Identity (%)","Overlap (%)","Result"};return labels.value(c);}
    QVariant data(const QModelIndex& at,int role)const override{
        if(!at.isValid()||!plan||(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole))return {};
        const auto& m=plan->members.at(static_cast<std::size_t>(at.row()));
        if(role==Qt::ToolTipRole&&m.group&&(at.column()==3||at.column()==4))return QString("%1 matches / %2 compared columns; %3 residue-to-residue columns.\nOverlap covers the shorter trimmed ungapped sequence; internal unknown calls count toward coverage but not identity.").arg(m.pair.matches).arg(m.pair.columns).arg(m.pair.paired);
        switch(at.column()){
        case 0:return text(rows_.at(m.id)->name);
        case 1:return m.group?names_.at(m.group):QString("Unchanged");
        case 2:return m.centroid?text(rows_.at(m.centroid)->name):QString("—");
        case 3:return m.group?QString::number(m.pair.identity(),'f',2):QString("—");
        case 4:return m.group?QString::number(m.overlap,'f',2):QString("—");
        case 5:return !m.skipped.empty()?text(m.skipped):m.id==m.centroid?QString("New group centroid"):QString("Group member");
        default:return {};
        }
    }
private:std::unordered_map<SequenceId,const Sequence*> rows_;std::unordered_map<GroupId,QString> names_;
};
void configure(QTableView* table){table->setAlternatingRowColors(true);table->setSelectionBehavior(QAbstractItemView::SelectRows);table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table->horizontalHeader()->setStretchLastSection(true);table->verticalHeader()->setDefaultSectionSize(24);}
struct Preview {std::shared_ptr<IdentityGroupPlan> plan;QString error;};
class IdentityDialog final:public QDialog {
public:
    IdentityDialog(std::shared_ptr<Document> source,std::vector<SequenceId> selected,QWidget* parent):QDialog(parent),source_(std::move(source)),selected_(std::move(selected)){
        setObjectName("identityGroupsDialog");setWindowTitle("Group by % identity");resize(1080,720);auto* layout=new QVBoxLayout(this);auto* form=new QFormLayout;layout->addLayout(form);
        scope_=new QComboBox(this);scope_->setObjectName("identityScope");scope_->addItems({"All sequences",QString("Selected sequence names (%1)").arg(selected_.size())});form->addRow("Scope",scope_);
        method_=new QComboBox(this);method_->setObjectName("identityMethod");method_->addItems({"Use existing alignment","Pairwise alignment with centroid candidate search (internal)"});form->addRow("Identity calculation",method_);
        candidates_=new QSpinBox(this);candidates_->setObjectName("identityCandidates");candidates_->setRange(0,4096);candidates_->setSpecialValueText("All (exhaustive)");candidates_->setValue(16);candidates_->setEnabled(false);candidates_->setToolTip("Fast search aligns at most this many centroids per query, ranked by shared unique words. It may miss qualifying centroids. Use 0 for the exhaustive 0.26 search. Word-ranked search may choose a different qualifying centroid than exhaustive mode.");form->addRow("Maximum centroid candidates (0 = exhaustive)",candidates_);
        threshold_=new QDoubleSpinBox(this);threshold_->setObjectName("identityThreshold");threshold_->setRange(0,100);threshold_->setDecimals(2);threshold_->setSuffix(" %");threshold_->setValue(97);form->addRow("Minimum identity",threshold_);
        overlap_=new QDoubleSpinBox(this);overlap_->setObjectName("identityOverlap");overlap_->setRange(0,100);overlap_->setDecimals(2);overlap_->setSuffix(" %");overlap_->setValue(80);overlap_->setToolTip("Minimum coverage of the shorter sequence after trimming terminal gaps and missing calls. At 0%, even a short high-identity overlap can qualify.");form->addRow("Minimum overlap (% of shorter sequence)",overlap_);
        prefix_=new QLineEdit("Identity_",this);prefix_->setObjectName("identityPrefix");prefix_->setMaxLength(128);prefix_->setToolTip("Numbered groups, for example Identity_1. Existing names are skipped.");form->addRow("Group name prefix",prefix_);
        status_=new QLabel(this);status_->setObjectName("identityStatus");status_->setWordWrap(true);status_->setTextFormat(Qt::PlainText);status_->setStyleSheet(statusStyle);layout->addWidget(status_);
        auto* detailsButton=new QToolButton(this);detailsButton->setText("Calculation details");detailsButton->setCheckable(true);detailsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);detailsButton->setArrowType(Qt::RightArrow);layout->addWidget(detailsButton,0,Qt::AlignLeft);
        auto* details=new QLabel("Sequences are processed by descending trimmed ungapped length; ties follow the current row order. Each sequence joins the first qualifying centroid, or starts a new group (including singletons). Centroids stay fixed. Fast pairwise search ranks centroids by shared unique 8-base words (3 residues for proteins), then aligns up to the chosen maximum. Ties use centroid creation order. With fewer centroids, all are word-ranked; at 0 (exhaustive), all centroids are tried in creation order. Fast search can choose a different qualifying centroid than exhaustive mode. It is heuristic and may miss qualifying groups; increase the limit or use exhaustive mode for sensitivity, especially with low identity, short overlaps or many ambiguous calls. Seedless queries try the earliest centroids up to the limit. Every tested pair still receives a full unbanded alignment. Nucleotide and protein rows are separate; masks and rows without informative residues are skipped.\n\nTerminal runs of gaps and N/? (protein X/?) are ignored. Identity is exact symbol matches / compared columns, ignoring case (nucleotide U=T). Internal single gaps count as differences; double gaps and columns containing missing calls are omitted. Other ambiguity codes match literally. Pairwise alignment removes existing gaps and uses affine semiglobal scoring (+2 match, −4 mismatch, −20 gap opening, −2 extension), with free end gaps and neutral missing calls. It uses current orientations and no external program. Existing-alignment comparisons keep compact and codon-spanned protein layouts separate.\n\nApply assigns eligible scoped sequences to the previewed groups. Old groups that become empty are deleted. Groups still used outside this scope or by skipped rows are retained. Undo restores the previous catalog and memberships. The row order, sequences and trace evidence are preserved.",this);details->setWordWrap(true);details->setVisible(false);layout->addWidget(details);connect(detailsButton,&QToolButton::toggled,this,[details,detailsButton](bool on){details->setVisible(on);detailsButton->setArrowType(on?Qt::DownArrow:Qt::RightArrow);});
        model_=new GroupingModel(*source_,this);table_=new QTableView(this);table_->setObjectName("identityPreview");table_->setModel(model_);configure(table_);for(int c=0;c<3;++c)table_->setColumnWidth(c,205);layout->addWidget(table_,1);
        auto* policy=new QLabel("Preview groups first, then Apply. Old groups that become empty are deleted; groups still used by untouched rows are retained. Save table exports the table without applying it.",this);policy->setWordWrap(true);layout->addWidget(policy);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel,this);layout->addWidget(buttons);apply_=buttons->button(QDialogButtonBox::Apply);apply_->setObjectName("identityApply");cancelButton_=buttons->button(QDialogButtonBox::Cancel);preview_=buttons->addButton("Preview groups",QDialogButtonBox::ActionRole);preview_->setObjectName("identityCalculate");stop_=buttons->addButton("Stop calculation",QDialogButtonBox::ActionRole);stop_->setObjectName("identityStop");save_=buttons->addButton("Save table…",QDialogButtonBox::ActionRole);save_->setObjectName("identitySavePreview");save_->setToolTip("Save grouping settings and results as XLSX, TSV or CSV.");
        connect(save_,&QPushButton::clicked,this,[this]{savePreview();});
        connect(buttons,&QDialogButtonBox::rejected,this,&IdentityDialog::reject);connect(preview_,&QPushButton::clicked,this,[this]{calculate();});connect(stop_,&QPushButton::clicked,this,[this]{cancelled_->store(true);stop_->setEnabled(false);status_->setText("Stopping calculation…");});
        connect(apply_,&QPushButton::clicked,this,[this]{try{if(!model_->plan)return;apply_identity_groups(*source_,*model_->plan);changed=true;accept();}catch(const std::exception& e){invalidate();status_->setText(QString::fromUtf8(e.what()));}});
        connect(scope_,&QComboBox::currentIndexChanged,this,[this]{invalidate();});connect(method_,&QComboBox::currentIndexChanged,this,[this]{invalidate();});connect(candidates_,&QSpinBox::valueChanged,this,[this]{invalidate();});connect(threshold_,&QDoubleSpinBox::valueChanged,this,[this]{invalidate();});connect(overlap_,&QDoubleSpinBox::valueChanged,this,[this]{invalidate();});connect(prefix_,&QLineEdit::textChanged,this,[this]{invalidate();});
        timer_=new QTimer(this);timer_->setInterval(100);connect(timer_,&QTimer::timeout,this,[this]{if(running_&&!cancelled_->load())status_->setText(QString("Grouping… %1 / %2 sequences processed · %3 centroid comparisons").arg(done_->load()).arg(total_->load()).arg(comparisons_->load()));});invalidate();
    }
    bool changed=false;
protected:
    void reject()override{if(running_){closing_=true;cancelled_->store(true);stop_->setEnabled(false);cancelButton_->setEnabled(false);status_->setText("Cancelling…");return;}QDialog::reject();}
private:
    void controls(bool running){running_=running;for(auto* w:std::vector<QWidget*>{scope_,method_,threshold_,overlap_,prefix_,candidates_})w->setEnabled(!running);candidates_->setEnabled(!running&&method_->currentIndex()==1);save_->setEnabled(!running&&model_->plan);preview_->setEnabled(!running&&(scope_->currentIndex()==0||!selected_.empty()));apply_->setEnabled(!running&&model_->plan&&model_->plan->assigned);stop_->setVisible(running);stop_->setEnabled(running);cancelButton_->setEnabled(true);}
    void invalidate(){model_->set({});controls(false);status_->setText(preview_->isEnabled()?"Choose settings and preview the proposed groups. Fast pairwise search may miss qualifying groups; use 0 candidates for exhaustive search.":"Select sequence names first, or choose All sequences.");}
    void savePreview(){
        try{
            if(!model_->plan)return;
            auto path=choose_table_path(this,"clustering_preview.xlsx",source_.get());if(path.isEmpty())return;
            const auto bytes=report_bytes(path,grouping_workbook(*source_,*model_->plan));
            const auto error=write_export_atomic(path,OutputFormat::Table,source_.get(),[&](auto& out){out.write(bytes.constData(),bytes.size());});if(!error.isEmpty())throw std::runtime_error("Cannot save preview table: "+error.toStdString());
            status_->setText("Preview table saved. Group memberships are unchanged until Apply.");
        }catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));}
    }
    void calculate(){
        if(running_)return;
        invalidate();IdentityGroupOptions options;options.alignment=static_cast<IdentityAlignment>(method_->currentIndex());options.maximumCandidates=static_cast<std::size_t>(candidates_->value());options.minimumIdentity=threshold_->value();options.minimumOverlap=overlap_->value();options.prefix=prefix_->text().toUtf8().toStdString();options.selectedOnly=scope_->currentIndex()==1;options.selected=selected_;
        closing_=false;cancelled_=std::make_shared<std::atomic_bool>(false);done_=std::make_shared<std::atomic<std::uint64_t>>(0);total_=std::make_shared<std::atomic<std::uint64_t>>(0);comparisons_=std::make_shared<std::atomic<std::uint64_t>>(0);controls(true);status_->setText("Preparing sequences…");timer_->start();auto* watcher=new QFutureWatcher<Preview>(this);const auto source=source_;const auto cancel=cancelled_;const auto done=done_,total=total_,comparisons=comparisons_;
        connect(watcher,&QFutureWatcher<Preview>::finished,this,[this,watcher]{auto result=watcher->result();watcher->deleteLater();timer_->stop();controls(false);if(closing_){QDialog::reject();return;}if(cancelled_->load()){status_->setText("Calculation stopped. Group memberships are unchanged.");return;}if(!result.error.isEmpty()){status_->setText(result.error);return;}if(result.plan->revision!=source_->state_id()){status_->setText("The alignment changed; refresh the grouping preview.");return;}model_->set(std::move(result.plan));controls(false);const auto& p=*model_->plan;status_->setText(QString("Preview: %1 new groups · %2 sequences assigned · %3 skipped · %4 comparisons · %5 queries shortlisted. Apply saves these memberships.").arg(p.clusters).arg(p.assigned).arg(p.members.size()-p.assigned).arg(p.comparisons).arg(p.shortlistedQueries));});
        watcher->setFuture(QtConcurrent::run([source,options,cancel,done,total,comparisons]{Preview result;try{result.plan=std::make_shared<IdentityGroupPlan>(plan_identity_groups(*source,options,[cancel]{return cancel->load();},[done,total,comparisons](std::size_t a,std::size_t b,std::uint64_t c){done->store(a);total->store(b);comparisons->store(c);}));}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;}));
    }
    std::shared_ptr<Document> source_;std::vector<SequenceId> selected_;GroupingModel* model_;QTableView* table_;QComboBox *scope_,*method_;QDoubleSpinBox *threshold_,*overlap_;QSpinBox* candidates_;QLineEdit* prefix_;QLabel* status_;QPushButton *apply_,*preview_,*stop_,*cancelButton_,*save_;QTimer* timer_;bool running_=false,closing_=false;std::shared_ptr<std::atomic_bool> cancelled_;std::shared_ptr<std::atomic<std::uint64_t>> done_,total_,comparisons_;
};
class RepresentativesModel final:public QAbstractTableModel {
public:
    RepresentativesModel(const Document& d,QObject* parent):QAbstractTableModel(parent),source_(d){for(std::size_t i=0;i<d.rows().size();++i)positions_[d.rows()[i].id]=i+1;for(const auto& g:d.groups()){ranks_[g.id]=ranks_.size();groups_[g.id]=text(g.name);}ranks_[0]=ranks_.size();groups_[0]="Ungrouped";}
    std::vector<SequenceId> ids;
    void set(std::vector<SequenceId> value){beginResetModel();ids=std::move(value);std::stable_sort(ids.begin(),ids.end(),[&](auto a,auto b){return ranks_.at(source_.rows()[positions_.at(a)-1].groupId)<ranks_.at(source_.rows()[positions_.at(b)-1].groupId);});endResetModel();}
    int rowCount(const QModelIndex& p={})const override{return p.isValid()?0:static_cast<int>(ids.size());}
    int columnCount(const QModelIndex& p={})const override{return p.isValid()?0:4;}
    QVariant headerData(int c,Qt::Orientation o,int role)const override{if(role!=Qt::DisplayRole)return {};if(o==Qt::Vertical)return c+1;const QStringList h{"Group","Row","Selected sequence","Ungapped residues"};return h.value(c);}
    QVariant data(const QModelIndex& at,int role)const override{if(!at.isValid()||(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole))return {};const auto& r=source_.rows()[positions_.at(ids.at(static_cast<std::size_t>(at.row())))-1];switch(at.column()){case 0:return groups_.at(r.groupId);case 1:return QVariant::fromValue<qulonglong>(positions_.at(r.id));case 2:return text(r.name);case 3:return QVariant::fromValue<qulonglong>(r.symbols.residue_count());default:return {};}}
private:const Document& source_;std::unordered_map<SequenceId,std::size_t> positions_;std::unordered_map<GroupId,QString> groups_;std::unordered_map<GroupId,std::size_t> ranks_;
};
}
bool identity_groups_dialog(std::shared_ptr<Document> d,const std::vector<SequenceId>& selected,QWidget* parent){IdentityDialog dialog(std::move(d),selected,parent);dialog.exec();return dialog.changed;}
void MainWindow::group_by_identity(){
    if(busy_)return;
    const auto ids=view_->selected_ids();set_busy(true,"Identity grouping…");try{if(identity_groups_dialog(document_,ids,this))view_->refresh();set_busy(false);}catch(const std::exception& e){set_busy(false);QMessageBox::warning(this,"Group by % identity",QString::fromUtf8(e.what()));}
}
void MainWindow::select_one_per_group(){
    if(busy_)return;
    const auto revision=document_->state_id();set_busy(true,"Choose one sequence per group…");
    try{
        QDialog dialog(this);dialog.setObjectName("onePerGroupDialog");dialog.setWindowTitle("One sequence per group");dialog.resize(820,580);QVBoxLayout layout(&dialog);QFormLayout form;layout.addLayout(&form);QComboBox method;method.setObjectName("representativeMethod");method.addItems({"First in current sorting order","Longest ungapped sequence","Random"});form.addRow("Select per group",&method);QCheckBox ungrouped("Also select from Ungrouped…");ungrouped.setObjectName("representativeUngrouped");QComboBox ungroupedMode;ungroupedMode.setObjectName("representativeUngroupedMode");ungroupedMode.addItems({"One sequence","All sequences"});ungroupedMode.setEnabled(false);QHBoxLayout ungroupedRow;ungroupedRow.addWidget(&ungrouped);ungroupedRow.addWidget(&ungroupedMode);ungroupedRow.addStretch();layout.addLayout(&ungroupedRow);
        QLabel rules("Selects one row from each nonempty named group. Longest counts all non-gap residues, including unknown calls; ties use the first row. Random draws are uniform within each group. Ungrouped can contribute one sequence using the same rule, or all sequences. Preview follows the group-manager order, with Ungrouped last. This replaces the sequence-name selection and leaves the alignment unchanged.");rules.setWordWrap(true);layout.addWidget(&rules);QLabel status;status.setObjectName("representativeStatus");status.setWordWrap(true);status.setStyleSheet(statusStyle);layout.addWidget(&status);
        RepresentativesModel model(*document_,&dialog);QTableView table;table.setObjectName("representativePreview");table.setModel(&model);configure(&table);table.setColumnWidth(0,180);table.setColumnWidth(1,65);table.setColumnWidth(2,320);layout.addWidget(&table,1);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout.addWidget(&buttons);auto* choose=buttons.button(QDialogButtonBox::Ok);choose->setText("Select sequences");choose->setObjectName("representativeApply");auto* redraw=buttons.addButton("Draw again",QDialogButtonBox::ActionRole);redraw->setObjectName("representativeRedraw");std::uint64_t seed=0;
        auto preview=[&]{try{if(document_->state_id()!=revision)throw std::runtime_error("The alignment changed; reopen the selection dialog.");if(method.currentIndex()==2){std::random_device random;seed=(std::uint64_t(random())<<32)^random();}model.set(select_group_representatives(*document_,static_cast<GroupRepresentative>(method.currentIndex()),ungrouped.isChecked()?(ungroupedMode.currentIndex()==0?UngroupedSelection::One:UngroupedSelection::All):UngroupedSelection::None,seed));redraw->setEnabled(method.currentIndex()==2);choose->setEnabled(!model.ids.empty());ungroupedMode.setEnabled(ungrouped.isChecked());status.setText(QString("%1 sequences will be selected. One per nonempty named group; Ungrouped follows the chosen option. Preview uses group-manager order.").arg(model.ids.size()));}catch(const std::exception& e){model.set({});choose->setEnabled(false);status.setText(e.what());}};
        connect(&method,&QComboBox::currentIndexChanged,&dialog,preview);connect(&ungrouped,&QCheckBox::toggled,&dialog,preview);connect(&ungroupedMode,&QComboBox::currentIndexChanged,&dialog,preview);connect(redraw,&QPushButton::clicked,&dialog,preview);connect(&buttons,&QDialogButtonBox::accepted,&dialog,[&]{if(document_->state_id()!=revision){choose->setEnabled(false);status.setText("The alignment changed; reopen the selection dialog.");return;}dialog.accept();});connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);preview();if(dialog.exec()==QDialog::Accepted){view_->select_ids(model.ids);view_->reveal_first_selected();}set_busy(false);
    }catch(const std::exception& e){set_busy(false);QMessageBox::warning(this,"One sequence per group",QString::fromUtf8(e.what()));}
}
}
