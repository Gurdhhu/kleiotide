#include "gui/file_output.hpp"
#include "gui/table_export.hpp"
#include <QFileInfo>
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/composition.hpp"
#include <QAbstractTableModel>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
#include <limits>
namespace kleiotide {
namespace {
QString utf8(const std::string& s){return QString::fromUtf8(s.data(),static_cast<qsizetype>(s.size()));}
class CompositionModel final:public QAbstractTableModel {
public:
    explicit CompositionModel(QObject* parent):QAbstractTableModel(parent){for(const auto& h:composition_headers())headers_.push_back(utf8(h));}
    void set_result(std::shared_ptr<const NucleotideComposition> value){beginResetModel();result_=std::move(value);cacheRow_=-1;endResetModel();}
    int rowCount(const QModelIndex& parent={})const override{return parent.isValid()||!result_?0:static_cast<int>(result_->rows.size()+1);}
    int columnCount(const QModelIndex& parent={})const override{return parent.isValid()?0:static_cast<int>(headers_.size());}
    QVariant headerData(int section,Qt::Orientation orientation,int role)const override{
        if(role!=Qt::DisplayRole)return {};
        if(orientation==Qt::Horizontal)return headers_.value(section);
        return section==rowCount()-1?QVariant("Σ"):QVariant(section+1);
    }
    QVariant data(const QModelIndex& i,int role)const override{
        if(!i.isValid()||!result_||i.row()>=rowCount())return {};
        if(role==Qt::TextAlignmentRole)return int(i.column()==0?Qt::AlignLeft|Qt::AlignVCenter:Qt::AlignRight|Qt::AlignVCenter);
        if(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole)return {};
        if(cacheRow_!=i.row()){cache_.clear();const auto& r=i.row()==rowCount()-1?result_->pooled:result_->rows[static_cast<std::size_t>(i.row())];for(const auto& c:composition_cells(r))cache_.push_back(utf8(c));cacheRow_=i.row();}
        return cache_.value(i.column());
    }
private:
    QStringList headers_;std::shared_ptr<const NucleotideComposition> result_;
    mutable int cacheRow_=-1;mutable QStringList cache_;
};
struct CompositionJob {std::shared_ptr<NucleotideComposition> result;std::string tsv;QString error;};
class CompositionDialog final:public QDialog {
public:
    CompositionDialog(std::shared_ptr<Document> source,std::vector<SequenceId> ids,std::optional<Document::ColumnRange> columns,QWidget* parent):QDialog(parent),source_(std::move(source)),ids_(std::move(ids)),columns_(columns){
        setObjectName("compositionDialog");setWindowTitle("Nucleotide composition");resize(1120,620);auto* layout=new QVBoxLayout(this);auto* form=new QFormLayout;layout->addLayout(form);
        rows_=new QComboBox(this);rows_->setObjectName("compositionRows");rows_->addItems({"All nucleotide rows",QString("Selected sequence names (%1, snapshot)").arg(ids_.size())});form->addRow("Rows",rows_);
        columnsBox_=new QComboBox(this);columnsBox_->setObjectName("compositionColumns");columnsBox_->addItems({"All nucleotide alignment columns","Selected alignment columns (snapshot)"});form->addRow("Columns",columnsBox_);
        fractional_=new QCheckBox("Include partial IUPAC calls with equal fractional votes",this);fractional_->setObjectName("compositionFractional");fractional_->setChecked(true);layout->addWidget(fractional_);
        auto* note=new QLabel("Nucleotide rows only; proteins and masks are excluded. U counts as T. A/C/G/T and GC percentages divide by scored calls: definite calls plus fractional IUPAC calls when enabled. N/?, gaps and trailing missing cells never enter that denominator. Pooled counts sum all rows; they are not mean row percentages. Counts display 3 decimals; a zero denominator is N/A.",this);note->setWordWrap(true);layout->addWidget(note);
        model_=new CompositionModel(this);auto* table=new QTableView(this);table->setObjectName("compositionTable");table->setModel(model_);table->setAlternatingRowColors(true);table->setSelectionBehavior(QAbstractItemView::SelectRows);table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table->horizontalHeader()->setDefaultSectionSize(105);table->setColumnWidth(0,210);layout->addWidget(table,1);
        status_=new QLabel("Choose scope and ambiguity handling, then Calculate. Changing a setting clears the previous results.",this);status_->setObjectName("compositionStatus");status_->setWordWrap(true);layout->addWidget(status_);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close,this);layout->addWidget(buttons);calculate_=buttons->addButton("Calculate",QDialogButtonBox::ActionRole);calculate_->setObjectName("compositionCalculate");calculate_->setDefault(true);copy_=buttons->addButton("Copy",QDialogButtonBox::ActionRole);copy_->setObjectName("compositionCopy");export_=buttons->addButton("Save table…",QDialogButtonBox::ActionRole);export_->setObjectName("compositionExport");copy_->setEnabled(false);export_->setEnabled(false);close_=buttons->button(QDialogButtonBox::Close);
        connect(buttons,&QDialogButtonBox::rejected,this,&CompositionDialog::reject);connect(calculate_,&QPushButton::clicked,this,[this]{calculate();});connect(copy_,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(utf8(tsv_));});
        connect(export_,&QPushButton::clicked,this,[this]{const auto path=choose_table_path(this,"nucleotide_composition.xlsx",source_.get());if(path.isEmpty())return;const auto error=write_export_atomic(path,OutputFormat::Table,source_.get(),[&](auto& out){if(QFileInfo(path).suffix().compare("tsv",Qt::CaseInsensitive)==0)out.write(tsv_.data(),static_cast<std::streamsize>(tsv_.size()));else{TableSheet methods{"Calculation methods",{{"Calculation setting","Value"}}},table{"Nucleotide composition"};for(const auto& line:utf8(tsv_).split('\n')){if(line.startsWith("# "))methods.rows.push_back({line.mid(2),""});else if(!line.isEmpty())table.rows.push_back(line.split('\t'));}if(!table.rows.empty())for(int c=1;c<table.rows[0].size();++c)table.numericColumns.insert(c);const auto bytes=report_bytes(path,{methods,table});out.write(bytes.constData(),bytes.size());}});if(!error.isEmpty())QMessageBox::warning(this,"Export composition",error);});
        connect(rows_,&QComboBox::currentIndexChanged,this,[this]{invalidate();});connect(columnsBox_,&QComboBox::currentIndexChanged,this,[this]{invalidate();});connect(fractional_,&QCheckBox::toggled,this,[this]{invalidate();});
    }
protected:
    void reject()override{if(running_){closing_=true;cancel_->store(true);close_->setEnabled(false);status_->setText("Cancelling calculation…");return;}QDialog::reject();}
private:
    void invalidate(){model_->set_result({});tsv_.clear();copy_->setEnabled(false);export_->setEnabled(false);status_->setText("Settings changed. Calculate to refresh results.");}
    void calculate(){
        if(running_)return;
        AnalysisScope scope;scope.selectedRows=rows_->currentIndex()==1;scope.ids=ids_;scope.selectedColumns=columnsBox_->currentIndex()==1;
        if(scope.selectedRows&&scope.ids.empty()){status_->setText("No sequence names selected. Choose All nucleotide rows explicitly, or close and select names.");return;}
        if(scope.selectedColumns){if(!columns_){status_->setText("No columns selected when this window opened. Close and select a fragment or columns first.");return;}scope.begin=columns_->start;scope.end=scope.begin+columns_->width;}
        invalidate();running_=true;cancel_=std::make_shared<std::atomic_bool>(false);for(auto* w:std::array<QWidget*,4>{rows_,columnsBox_,fractional_,calculate_})w->setEnabled(false);status_->setText("Computing composition…");const auto fractional=fractional_->isChecked();const auto cancel=cancel_;const auto source=source_;
        auto* watcher=new QFutureWatcher<CompositionJob>(this);connect(watcher,&QFutureWatcher<CompositionJob>::finished,this,[this,watcher]{auto result=watcher->result();watcher->deleteLater();running_=false;for(auto* w:std::array<QWidget*,4>{rows_,columnsBox_,fractional_,calculate_})w->setEnabled(true);if(closing_){QDialog::reject();return;}if(!result.error.isEmpty()){status_->setText(result.error);return;}model_->set_result(result.result);tsv_=std::move(result.tsv);copy_->setEnabled(true);export_->setEnabled(true);status_->setText(QString("%1 nucleotide rows · %2 columns · Pooled row is last. Copy and Export include settings and the full table.").arg(result.result->rows.size()).arg(result.result->end-result.result->begin));});
        watcher->setFuture(QtConcurrent::run([source,scope,fractional,cancel]{CompositionJob result;try{const auto check=[cancel]{return cancel->load();};result.result=std::make_shared<NucleotideComposition>(nucleotide_composition(*source,scope,fractional,check));if(result.result->rows.size()>=static_cast<std::size_t>(std::numeric_limits<int>::max()))throw std::length_error("Too many rows to display.");result.tsv=composition_tsv(*result.result,check);}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;}));
    }
    std::shared_ptr<Document> source_;std::vector<SequenceId> ids_;std::optional<Document::ColumnRange> columns_;
    QComboBox *rows_,*columnsBox_;QCheckBox* fractional_;CompositionModel* model_;QLabel* status_;QPushButton *calculate_,*copy_,*export_,*close_;
    bool running_=false,closing_=false;std::shared_ptr<std::atomic_bool> cancel_;std::string tsv_;
};
}
void MainWindow::show_composition(){
    if(busy_)return;
    std::optional<Document::ColumnRange> columns;if(const auto& f=view_->fragment())columns=Document::ColumnRange{f->start,f->width};
    CompositionDialog dialog(document_,view_->selected_ids(),columns,this);set_busy(true,"Nucleotide composition…");dialog.exec();set_busy(false);
}
}
