#include "gui/collapse_dialog.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/collapse.hpp"
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QToolButton>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <atomic>
#include <unordered_map>
namespace kleiotide {
namespace {
QString text(const std::string& s){return QString::fromUtf8(s.data(),static_cast<qsizetype>(s.size()));}
class CollapseModel final:public QAbstractTableModel {
public:
    explicit CollapseModel(const Document& source,QObject* parent):QAbstractTableModel(parent),source_(source){for(std::size_t i=0;i<source.rows().size();++i)indices_.emplace(source.rows()[i].id,i);}
    int rowCount(const QModelIndex& p={})const override{return p.isValid()?0:static_cast<int>(plan.clusters.size());}
    int columnCount(const QModelIndex& p={})const override{return p.isValid()?0:6;}
    QVariant headerData(int section,Qt::Orientation o,int role)const override{if(role!=Qt::DisplayRole)return {};if(o==Qt::Vertical)return section+1;const char* labels[]={"Copies","Type","Representative","Retained group","Retained trace","Result title"};return labels[section];}
    const Sequence& row(SequenceId id)const{return source_.rows().at(indices_.at(id));}
    std::size_t position(SequenceId id)const{return indices_.at(id)+1;}
    QString group(SequenceId id)const{const auto* g=source_.group(row(id).groupId);return g?text(g->name):QString("Ungrouped");}
    QString trace(SequenceId id)const{const auto& evidence=row(id).trace;if(!evidence)return "None";auto value=text(evidence->original->sampleName);if(value.isEmpty())value="Linked trace";if(!evidence->original->sourcePath.empty())value+=" — "+text(evidence->original->sourcePath);return value+(evidence->reversed?" (reverse)":"");}
    QString title(std::size_t index,qsizetype limit=4096)const{
        const auto& cluster=plan.clusters.at(index);const auto id=representatives.at(index);auto result=text(row(id).name);
        if(naming==CollapseNaming::CountSuffix)return result+"_x"+QString::number(cluster.members.size());
        for(auto member:cluster.members)if(member!=id){result+=separator+text(row(member).name);if(result.size()>limit)break;}
        return result.size()>limit?result.left(limit)+"…":result;
    }
    QVariant data(const QModelIndex& index,int role)const override{
        if(!index.isValid()||(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole))return {};
        const auto at=static_cast<std::size_t>(index.row());const auto id=representatives.at(at);const auto& representative=row(id);
        switch(index.column()){case 0:return QVariant::fromValue<qulonglong>(plan.clusters[at].members.size());case 1:return text(std::string(row_kind_name(representative.kind)));case 2:return QString("%1. %2").arg(position(id)).arg(text(representative.name));case 3:return group(id);case 4:return trace(id);case 5:return title(at,role==Qt::ToolTipRole?16384:4096);default:return {};}
    }
    void clear(){beginResetModel();plan={};representatives.clear();endResetModel();}
    void set(CollapsePlan value){beginResetModel();plan=std::move(value);representatives.clear();for(const auto& c:plan.clusters)representatives.push_back(c.members.front());endResetModel();}
    void renamed(){if(rowCount())emit dataChanged(index(0,5),index(rowCount()-1,5));}
    void keep(std::size_t at,SequenceId id){representatives.at(at)=id;emit dataChanged(index(static_cast<int>(at),0),index(static_cast<int>(at),5));}
    CollapsePlan plan;std::vector<SequenceId> representatives;CollapseNaming naming=CollapseNaming::Concatenate;QString separator=";";
private:const Document& source_;std::unordered_map<SequenceId,std::size_t> indices_;
};
class MemberModel final:public QAbstractTableModel {
public:
    MemberModel(CollapseModel& source,QObject* parent):QAbstractTableModel(parent),source_(source){}
    int rowCount(const QModelIndex& p={})const override{return p.isValid()||cluster_<0?0:static_cast<int>(source_.plan.clusters.at(static_cast<std::size_t>(cluster_)).members.size());}
    int columnCount(const QModelIndex& p={})const override{return p.isValid()?0:7;}
    QVariant headerData(int section,Qt::Orientation o,int role)const override{if(role!=Qt::DisplayRole)return {};if(o==Qt::Vertical)return section+1;const char* labels[]={"Row","Original title","Group","Trace","Residues","Gaps","Aligned symbols"};return labels[section];}
    SequenceId id(int row)const{return source_.plan.clusters.at(static_cast<std::size_t>(cluster_)).members.at(static_cast<std::size_t>(row));}
    QVariant data(const QModelIndex& index,int role)const override{if(!index.isValid()||(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole))return {};const auto sequence=id(index.row());const auto& row=source_.row(sequence);switch(index.column()){case 0:return QVariant::fromValue<qulonglong>(source_.position(sequence));case 1:return text(row.name);case 2:return source_.group(sequence);case 3:return source_.trace(sequence);case 4:return QVariant::fromValue<qulonglong>(row.symbols.residue_count());case 5:return QVariant::fromValue<qulonglong>(row.symbols.size()-row.symbols.residue_count());case 6:{const auto limit=role==Qt::ToolTipRole?4096U:160U;return text(row.symbols.text().substr(0,limit))+(row.symbols.size()>limit?"…":"");}default:return {};}}
    void set(int cluster){beginResetModel();cluster_=cluster;endResetModel();}
private:CollapseModel& source_;int cluster_=-1;
};
struct PreviewResult {CollapsePlan plan;QString error;};
struct ChoicesResult {std::vector<Document::CollapseGroup> choices;QString error;};
class CollapseDialog final:public QDialog {
public:
    CollapseDialog(std::shared_ptr<Document> source,std::vector<SequenceId> selected,QWidget* parent):QDialog(parent),source_(std::move(source)),selected_(std::move(selected)){
        setObjectName("collapseDialog");setWindowTitle("Collapse identical sequences");resize(1120,780);auto* layout=new QVBoxLayout(this);auto* form=new QFormLayout;layout->addLayout(form);
        scope_=new QComboBox(this);scope_->setObjectName("collapseScope");scope_->addItems({"All sequences",QString("Selected sequence names (%1, snapshot)").arg(selected_.size())});form->addRow("Scope",scope_);
        naming_=new QComboBox(this);naming_->setObjectName("collapseNaming");naming_->addItems({"Combine original titles","Representative title + count (_x13)"});form->addRow("Result titles",naming_);
        separator_=new QLineEdit(";",this);separator_->setObjectName("collapseSeparator");separator_->setMaxLength(64);separator_->setToolTip("Separator between complete original titles. The representative's title goes first; remaining titles follow alignment order.");form->addRow("Title separator",separator_);
        masks_=new QCheckBox("Include mask rows (compared separately)",this);masks_->setObjectName("collapseMasks");layout->addWidget(masks_);
        auto* rules=new QLabel("Matching ignores case and all open/locked gaps. Complete ungapped strings must match; shorter matching substrings stay separate. N and other ambiguities are literal symbols, not wildcards; U and T are distinct. Nucleotide, protein and mask rows are compared separately. Empty/gap-only rows match other empty/gap-only rows of their type.",this);rules->setWordWrap(true);auto* rulesToggle=new QToolButton;rulesToggle->setText("Comparison rules");rulesToggle->setCheckable(true);rulesToggle->setArrowType(Qt::RightArrow);rulesToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);layout->addWidget(rulesToggle,0,Qt::AlignLeft);rules->hide();layout->addWidget(rules);connect(rulesToggle,&QToolButton::toggled,this,[rules,rulesToggle](bool on){rules->setVisible(on);rulesToggle->setArrowType(on?Qt::DownArrow:Qt::RightArrow);});
        auto* policy=new QLabel("Choose a representative for each cluster. Its sequence, group and trace are retained; other members are removed. The count includes the representative (13 rows become name_x13).",this);policy->setWordWrap(true);layout->addWidget(policy);
        auto* splitter=new QSplitter(Qt::Vertical,this);auto* clusterPane=new QWidget(splitter);auto* clusterLayout=new QVBoxLayout(clusterPane);clusterLayout->setContentsMargins(0,0,0,0);clusterLayout->addWidget(new QLabel("Identical sequence clusters"));auto* memberPane=new QWidget(splitter);auto* memberLayout=new QVBoxLayout(memberPane);memberLayout->setContentsMargins(0,0,0,0);memberLayout->addWidget(new QLabel("Members of the selected cluster"));layout->addWidget(splitter,1);model_=new CollapseModel(*source_,this);sets_=new QTableView(clusterPane);sets_->setObjectName("collapseSets");sets_->setModel(model_);configure(sets_);sets_->horizontalHeader()->setSectionResizeMode(5,QHeaderView::Stretch);sets_->setColumnWidth(0,65);sets_->setColumnWidth(1,90);sets_->setColumnWidth(2,230);
        clusterLayout->addWidget(sets_);membersModel_=new MemberModel(*model_,this);members_=new QTableView(memberPane);members_->setObjectName("collapseMembers");members_->setModel(membersModel_);configure(members_);members_->horizontalHeader()->setSectionResizeMode(6,QHeaderView::Stretch);for(int c=0;c<6;++c)members_->setColumnWidth(c,c==1?200:c==3?160:c==2?120:75);memberLayout->addWidget(members_);splitter->setSizes({230,190});
        keep_=new QPushButton("Use selected member as representative",this);keep_->setObjectName("collapseKeep");keep_->setEnabled(false);layout->addWidget(keep_,0,Qt::AlignLeft);
        retained_=new QLabel("Select a duplicate set to inspect its members.",this);retained_->setObjectName("collapseRetained");retained_->setWordWrap(true);retained_->setTextFormat(Qt::PlainText);retained_->setMinimumHeight(retained_->fontMetrics().lineSpacing()*3);retained_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);layout->addWidget(retained_);
        status_=new QLabel(this);status_->setObjectName("collapseStatus");status_->setWordWrap(true);status_->setTextFormat(Qt::PlainText);status_->setStyleSheet("QLabel { background: #e7effa; color: #17212b; border: 2px solid #32669c; border-radius: 5px; padding: 10px; font-weight: bold; }");layout->insertWidget(1,status_);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel,this);layout->addWidget(buttons);apply_=buttons->button(QDialogButtonBox::Apply);apply_->setObjectName("collapseApply");preview_=buttons->addButton("Preview",QDialogButtonBox::ActionRole);preview_->setObjectName("collapsePreview");cancel_=buttons->button(QDialogButtonBox::Cancel);
        connect(buttons,&QDialogButtonBox::rejected,this,&CollapseDialog::reject);connect(preview_,&QPushButton::clicked,this,[this]{preview();});connect(apply_,&QPushButton::clicked,this,[this]{apply();});
        connect(scope_,&QComboBox::currentIndexChanged,this,[this]{invalidate();});connect(masks_,&QCheckBox::toggled,this,[this]{invalidate();});
        connect(naming_,&QComboBox::currentIndexChanged,this,[this]{model_->naming=static_cast<CollapseNaming>(naming_->currentIndex());separator_->setEnabled(!running_&&naming_->currentIndex()==0);model_->renamed();describe();});
        connect(separator_,&QLineEdit::textChanged,this,[this]{model_->separator=separator_->text();model_->renamed();describe();});
        connect(sets_->selectionModel(),&QItemSelectionModel::currentRowChanged,this,[this](const QModelIndex& at){show_members(at.isValid()?at.row():-1);});
        connect(members_->selectionModel(),&QItemSelectionModel::currentRowChanged,this,[this]{keep_->setEnabled(!running_&&members_->currentIndex().isValid());});
        connect(keep_,&QPushButton::clicked,this,[this]{const auto set=sets_->currentIndex().row(),member=members_->currentIndex().row();if(set<0||member<0||running_)return;model_->keep(static_cast<std::size_t>(set),membersModel_->id(member));describe();});
        invalidate();QTimer::singleShot(0,this,[this]{preview();});
    }
    bool changed=false;
protected:
    void reject()override{if(running_){closing_=true;cancelled_->store(true);cancel_->setEnabled(false);status_->setText("Cancelling…");return;}QDialog::reject();}
private:
    static void configure(QTableView* table){table->setAlternatingRowColors(true);table->setSelectionBehavior(QAbstractItemView::SelectRows);table->setSelectionMode(QAbstractItemView::SingleSelection);table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table->horizontalHeader()->setDefaultSectionSize(140);}
    void controls(bool running){running_=running;scope_->setEnabled(!running);masks_->setEnabled(!running);naming_->setEnabled(!running);separator_->setEnabled(!running&&naming_->currentIndex()==0);sets_->setEnabled(!running);members_->setEnabled(!running);keep_->setEnabled(!running&&members_->currentIndex().isValid());preview_->setEnabled(!running&&(scope_->currentIndex()==0||selected_.size()>1));apply_->setEnabled(!running&&!model_->plan.clusters.empty());}
    void invalidate(){membersModel_->set(-1);model_->clear();keep_->setEnabled(false);apply_->setEnabled(false);preview_->setEnabled(scope_->currentIndex()==0||selected_.size()>1);retained_->setText("Select a duplicate set to inspect its members.");status_->setText(preview_->isEnabled()?"Choose scope, then Preview.":"Select at least two sequence names, or choose All sequences.");}
    void show_members(int at){membersModel_->set(at);keep_->setEnabled(false);if(at>=0){const auto& cluster=model_->plan.clusters[static_cast<std::size_t>(at)];const auto id=model_->representatives[static_cast<std::size_t>(at)];const auto found=std::find(cluster.members.begin(),cluster.members.end(),id);members_->selectRow(static_cast<int>(found-cluster.members.begin()));}describe();}
    void describe(){const auto at=sets_->currentIndex().row();if(at<0||static_cast<std::size_t>(at)>=model_->plan.clusters.size())return;const auto index=static_cast<std::size_t>(at);const auto id=model_->representatives[index];retained_->setText(QString("Retain row %1: %2 · Group: %3 · Trace: %4\nNew title: %5").arg(model_->position(id)).arg(text(model_->row(id).name).left(120),model_->group(id).left(80),model_->trace(id).left(120),model_->title(index,180)));}
    void preview(){
        if(running_||(scope_->currentIndex()==1&&selected_.size()<2))return;
        invalidate();controls(true);closing_=false;cancelled_=std::make_shared<std::atomic_bool>(false);status_->setText("Finding identical sequences…");const auto source=source_;const auto selected=selected_;const bool selectedOnly=scope_->currentIndex()==1,includeMasks=masks_->isChecked();const auto cancel=cancelled_;auto* watcher=new QFutureWatcher<PreviewResult>(this);
        connect(watcher,&QFutureWatcher<PreviewResult>::finished,this,[this,watcher]{auto result=watcher->result();watcher->deleteLater();controls(false);if(closing_){QDialog::reject();return;}if(!result.error.isEmpty()){status_->setText(result.error);return;}model_->set(std::move(result.plan));controls(false);status_->setText(QString("%1 duplicate sets · %2 rows removed · %3 rows retained from %4 in scope · %5 masks skipped. First row is the default representative; choose a set to change it.").arg(model_->plan.clusters.size()).arg(model_->plan.removedRows).arg(model_->plan.scopedRows-model_->plan.removedRows).arg(model_->plan.scopedRows).arg(model_->plan.masksSkipped));if(model_->rowCount())sets_->selectRow(0);});
        watcher->setFuture(QtConcurrent::run([source,selected,selectedOnly,includeMasks,cancel]{PreviewResult r;try{r.plan=plan_identical_collapse(*source,selectedOnly,selected,includeMasks,[cancel]{return cancel->load();});}catch(const std::exception& e){r.error=QString::fromUtf8(e.what());}return r;}));
    }
    void apply(){
        if(running_||model_->plan.clusters.empty())return;
        controls(true);closing_=false;cancelled_=std::make_shared<std::atomic_bool>(false);status_->setText("Preparing collapsed titles…");auto* watcher=new QFutureWatcher<ChoicesResult>(this);const auto source=source_;const auto plan=model_->plan;const auto representatives=model_->representatives;const auto naming=model_->naming;const auto separator=separator_->text().toUtf8().toStdString();const auto cancel=cancelled_;
        connect(watcher,&QFutureWatcher<ChoicesResult>::finished,this,[this,watcher]{auto result=watcher->result();watcher->deleteLater();controls(false);if(closing_){QDialog::reject();return;}if(!result.error.isEmpty()){status_->setText(result.error);return;}try{apply_identical_collapse(*source_,model_->plan,result.choices);changed=true;accept();}catch(const std::exception& e){invalidate();status_->setText(QString::fromUtf8(e.what()));}});
        watcher->setFuture(QtConcurrent::run([source,plan,representatives,naming,separator,cancel]{ChoicesResult r;try{r.choices=collapse_choices(*source,plan,representatives,naming,separator,[cancel]{return cancel->load();});}catch(const std::exception& e){r.error=QString::fromUtf8(e.what());}return r;}));
    }
    std::shared_ptr<Document> source_;std::vector<SequenceId> selected_;CollapseModel* model_;MemberModel* membersModel_;QComboBox *scope_,*naming_;QLineEdit* separator_;QCheckBox* masks_;QTableView *sets_,*members_;QLabel *retained_,*status_;QPushButton *keep_,*apply_,*preview_,*cancel_;bool running_=false,closing_=false;std::shared_ptr<std::atomic_bool> cancelled_;
};
}
bool collapse_sequences_dialog(std::shared_ptr<Document> source,const std::vector<SequenceId>& selected,QWidget* parent){CollapseDialog dialog(std::move(source),selected,parent);dialog.exec();return dialog.changed;}
void MainWindow::collapse_sequences(){
    if(busy_)return;
    const auto selected=view_->selected_ids();set_busy(true,"Collapse identical sequences…");const bool changed=collapse_sequences_dialog(document_,selected,this);if(changed)view_->refresh();set_busy(false);update_state();
}
}
