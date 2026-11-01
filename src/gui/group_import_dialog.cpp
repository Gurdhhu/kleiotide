#include "gui/groups_dialog.hpp"
#include "core/group_import.hpp"
#include "gui/table_io.hpp"
#include "gui/spart.hpp"
#include <QMessageBox>
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QFormLayout>
#include <QSignalBlocker>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <atomic>
#include <memory>
namespace kleiotide {
namespace {
QString display(std::string_view value,std::size_t limit){const auto n=std::min(value.size(),limit);auto result=QString::fromUtf8(value.data(),static_cast<qsizetype>(n));if(n<value.size())result+=QStringLiteral("…");return result;}
class ImportModel final:public QAbstractTableModel {
public:
    explicit ImportModel(QObject* parent):QAbstractTableModel(parent){}
    std::shared_ptr<const GroupImportPlan> plan;
    void set(std::shared_ptr<const GroupImportPlan> value){beginResetModel();plan=std::move(value);endResetModel();}
    int rowCount(const QModelIndex& parent={})const override{return parent.isValid()||!plan?0:static_cast<int>(plan->rows.size());}
    int columnCount(const QModelIndex& parent={})const override{return parent.isValid()?0:4;}
    QVariant headerData(int n,Qt::Orientation orientation,int role)const override{if(role!=Qt::DisplayRole)return {};if(orientation==Qt::Vertical)return n+1;return QStringList{"Line","Sequence title","Group","Result"}.value(n);}
    QVariant data(const QModelIndex& index,int role)const override{
        if(!index.isValid()||!plan)return {};
        const auto& row=plan->rows.at(static_cast<std::size_t>(index.row()));
        if(role==Qt::ForegroundRole&&row.error)return QColor(Qt::darkRed);
        if(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole)return {};
        const auto limit=role==Qt::ToolTipRole?4096u:1024u;
        switch(index.column()){case 0:return QVariant::fromValue<qulonglong>(row.line);case 1:return display(row.title,limit);case 2:return display(row.name,limit);case 3:return display(row.status,limit);default:return {};}
    }
};
struct ImportResult {std::shared_ptr<GroupImportPlan> plan;QString error;};
class ImportDialog final:public QDialog {
public:
    ImportDialog(Document& source,QWidget* parent,std::vector<SpartPartition> partitions={}):QDialog(parent),source_(source){
        setObjectName("groupImportDialog");setWindowTitle("Import sequence groups");resize(960,680);auto* layout=new QVBoxLayout(this);
        auto* note=new QLabel("Paste or load a TSV, CSV or XLSX table and choose its sequence-name and group-name columns. Two-column tables use the first column for names and the second for groups. Surrounding parentheses and TSV quotes are removed from cells. Empty group means Ungrouped. Unmatched titles are skipped; errors block Apply.",this);note->setWordWrap(true);layout->addWidget(note);
        auto* load=new QPushButton("Load table…",this);load->setObjectName("groupImportLoad");load->setToolTip("Load a TSV, CSV or XLSX table and choose its columns.");layout->addWidget(load);
        input_=new QPlainTextEdit(this);input_->setObjectName("groupImportText");input_->setPlaceholderText("Sequence_name\tGroup_name");input_->setLineWrapMode(QPlainTextEdit::NoWrap);layout->addWidget(input_);
        header_=new QCheckBox("First row is a header",this);header_->setObjectName("groupImportHeader");duplicates_=new QCheckBox("Apply to all matching duplicate sequence titles",this);duplicates_->setObjectName("groupImportDuplicates");layout->addWidget(header_);layout->addWidget(duplicates_);
        auto* mapping=new QFormLayout;sequenceColumn_=new QComboBox(this);sequenceColumn_->setObjectName("groupImportSequenceColumn");groupColumn_=new QComboBox(this);groupColumn_->setObjectName("groupImportGroupColumn");
        for(auto* combo:{sequenceColumn_,groupColumn_}){combo->addItems({"Column 1","Column 2"});combo->setEnabled(false);combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setMinimumContentsLength(24);}
        groupColumn_->setCurrentIndex(1);mapping->addRow("Sequence name column",sequenceColumn_);mapping->addRow("Group name column",groupColumn_);layout->addLayout(mapping);
        model_=new ImportModel(this);auto* table=new QTableView(this);table->setObjectName("groupImportPreview");table->setModel(model_);table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->setSelectionBehavior(QAbstractItemView::SelectRows);table->setAlternatingRowColors(true);table->setWordWrap(false);table->verticalHeader()->setDefaultSectionSize(24);table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table->horizontalHeader()->setStretchLastSection(true);table->setColumnWidth(0,60);table->setColumnWidth(1,280);table->setColumnWidth(2,180);layout->addWidget(table,1);
        summary_=new QLabel(this);summary_->setObjectName("groupImportSummary");summary_->setWordWrap(true);summary_->setTextFormat(Qt::PlainText);layout->addWidget(summary_);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel,this);layout->addWidget(buttons);apply_=buttons->button(QDialogButtonBox::Apply);apply_->setObjectName("groupImportApply");apply_->setEnabled(false);
        debounce_=new QTimer(this);debounce_->setSingleShot(true);debounce_->setInterval(200);connect(debounce_,&QTimer::timeout,this,[this]{start();});
        connect(input_,&QPlainTextEdit::textChanged,this,[this]{schedule();});connect(header_,&QCheckBox::toggled,this,[this]{schedule();});connect(duplicates_,&QCheckBox::toggled,this,[this]{schedule();});connect(sequenceColumn_,&QComboBox::currentIndexChanged,this,[this]{schedule();});connect(groupColumn_,&QComboBox::currentIndexChanged,this,[this]{schedule();});connect(buttons,&QDialogButtonBox::rejected,this,&ImportDialog::reject);
        connect(apply_,&QPushButton::clicked,this,[this]{try{if(!model_->plan)return;const auto before=source_.state_id();apply_group_import(source_,*model_->plan);changed=source_.state_id()!=before;accept();}catch(const std::exception& e){clearPreview();summary_->setText(QString::fromUtf8(e.what()));}});
        connect(load,&QPushButton::clicked,this,[this]{if(auto content=load_table_file(this,"Load group table")){input_->setPlainText(*content);schedule();}});

        summary_->setText("Paste or load a table to preview its column mapping.");
        if(!partitions.empty()){sanitation_=new QCheckBox("Assume SPART name sanitation",this);sanitation_->setObjectName("spartAssumeSanitation");sanitation_->setToolTip("Compare lowercase, SPART-sanitized names; alignment names stay unchanged.");sanitation_->setChecked(true);layout->insertWidget(0,sanitation_);connect(sanitation_,&QCheckBox::toggled,this,[this]{schedule();});auto* partition=new QComboBox(this);partition->setObjectName("spartPartition");for(const auto& p:partitions)partition->addItem(p.name);auto* choice=new QFormLayout;choice->addRow("SPART partition",partition);layout->insertLayout(0,choice);load->hide();input_->setReadOnly(true);header_->hide();sequenceColumn_->hide();groupColumn_->hide();connect(partition,&QComboBox::currentIndexChanged,this,[this,partitions](int i){if(i>=0)input_->setPlainText(table_text(partitions[std::size_t(i)].assignments));});input_->setPlainText(table_text(partitions[0].assignments));}

    }
    ~ImportDialog()override{if(cancelled_)cancelled_->store(true);}
    bool changed=false;
protected:
    void reject()override{clearPreview();QDialog::reject();}
private:
    void clearPreview(){++generation_;requested_=false;debounce_->stop();if(cancelled_)cancelled_->store(true);model_->set({});apply_->setEnabled(false);}
    void schedule(){clearPreview();requested_=true;summary_->setText("Waiting to validate table…");debounce_->start();}
    bool updateColumns(const GroupImportPlan& plan){
        const auto count=static_cast<int>(std::max(std::size_t(2),plan.columnCount));
        const auto oldSequence=sequenceColumn_->currentIndex(),oldGroup=groupColumn_->currentIndex();
        const auto sequence=plan.columnCount<=2||oldSequence>=count?0:oldSequence;
        const auto group=plan.columnCount<=2||oldGroup>=count?1:oldGroup;
        QSignalBlocker blockSequence(sequenceColumn_),blockGroup(groupColumn_);
        sequenceColumn_->clear();groupColumn_->clear();
        for(int i=0;i<count;++i){auto label=QString("Column %1").arg(i+1);if(static_cast<std::size_t>(i)<plan.columnNames.size())label+=" — "+display(plan.columnNames[static_cast<std::size_t>(i)],80);sequenceColumn_->addItem(label);groupColumn_->addItem(label);}
        sequenceColumn_->setCurrentIndex(sequence);groupColumn_->setCurrentIndex(group);sequenceColumn_->setEnabled(plan.columnCount>2);groupColumn_->setEnabled(plan.columnCount>2);
        return sequence!=oldSequence||group!=oldGroup;
    }
    void start(){
        if(running_||!requested_)return;
        requested_=false;const auto bytes=input_->toPlainText().toUtf8();
        if(bytes.size()>static_cast<qsizetype>(group_import_max_bytes)){summary_->setText("The group table exceeds 16 MiB. Split it into smaller tables.");return;}
        const bool sanitize=sanitation_&&sanitation_->isChecked();auto snapshot=std::make_shared<GroupImportSource>(group_import_source(source_));if(sanitize)for(auto& row:snapshot->rows)row.name=spart_sanitize(QString::fromStdString(row.name)).toStdString();const auto content=bytes.toStdString();const bool header=header_->isChecked(),duplicates=duplicates_->isChecked();const auto generation=generation_;const GroupImportColumns columns{static_cast<std::size_t>(sequenceColumn_->currentIndex()),static_cast<std::size_t>(groupColumn_->currentIndex())};
        cancelled_=std::make_shared<std::atomic_bool>(false);const auto cancel=cancelled_;running_=true;summary_->setText("Validating table… You can edit it or Cancel.");auto* watcher=new QFutureWatcher<ImportResult>(this);
        connect(watcher,&QFutureWatcher<ImportResult>::finished,this,[this,watcher,generation]{auto result=watcher->result();watcher->deleteLater();running_=false;
            if(generation==generation_){if(!result.error.isEmpty())summary_->setText(result.error);else if(result.plan->revision!=source_.state_id())summary_->setText("The alignment changed. Edit the table or toggle a setting to refresh its preview.");else if(updateColumns(*result.plan)){schedule();}else{model_->set(std::move(result.plan));const auto& p=*model_->plan;summary_->setText(QString("%1 matched sequences · %2 errors · %3 unmatched titles%4").arg(p.assignments.size()).arg(p.errors).arg(p.warnings).arg(p.errors?". Check the selected columns and the row errors below.":""));apply_->setEnabled(!p.errors&&!p.assignments.empty());}}
            if(requested_&&!debounce_->isActive())start();
        });
        watcher->setFuture(QtConcurrent::run([snapshot,content,header,duplicates,cancel,columns,sanitize]{ImportResult result;try{auto canonical=content;if(sanitize){auto rows=parse_text_table(QString::fromUtf8(content.data(),qsizetype(content.size())));for(std::size_t row=header?1:0;row<rows.size();++row)if(columns.sequence<std::size_t(rows[row].size()))rows[row][qsizetype(columns.sequence)]=spart_sanitize(rows[row][qsizetype(columns.sequence)]);canonical=table_text(rows).toUtf8().toStdString();}result.plan=std::make_shared<GroupImportPlan>(plan_group_import(*snapshot,canonical,header,duplicates,[cancel]{return cancel->load();},columns));}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;}));
    }
    QCheckBox* sanitation_=nullptr;Document& source_;ImportModel* model_;QPlainTextEdit* input_;QCheckBox *header_,*duplicates_;QComboBox *sequenceColumn_,*groupColumn_;QLabel* summary_;QPushButton* apply_;QTimer* debounce_;std::shared_ptr<std::atomic_bool> cancelled_;std::uint64_t generation_=0;bool running_=false,requested_=false;
};
}
bool import_spart_groups(Document& source,QWidget* parent){const auto path=QFileDialog::getOpenFileName(parent,"Import groups from SPART",{},"SPART (*.spart *.spart.xml *.xml);;All files (*)");if(path.isEmpty())return false;try{QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>16*1024*1024)throw std::runtime_error("Cannot read SPART file, or it exceeds 16 MiB.");auto partitions=parse_spart(file.readAll());ImportDialog dialog(source,parent,std::move(partitions));dialog.exec();return dialog.changed;}catch(const std::exception& e){QMessageBox::warning(parent,"Import SPART",QString::fromUtf8(e.what()));return false;}}
bool import_groups(Document& source,QWidget* parent){ImportDialog dialog(source,parent);dialog.exec();return dialog.changed;}
}
