#include "gui/file_output.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/distances.hpp"
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
#include <QToolButton>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSignalBlocker>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
#include <limits>
#include <stdexcept>
#include <unordered_map>
namespace kleiotide {
namespace {
QString utf8(const std::string& s){return QString::fromUtf8(s.data(),static_cast<qsizetype>(s.size()));}
QString number(double x){return utf8(distance_number(x));}
QString title(DistanceReport report){switch(report){case DistanceReport::Matrix:return "Pairwise distance matrix";case DistanceReport::Overall:return "Overall mean distance";case DistanceReport::WithinGroups:return "Within-group mean distance";case DistanceReport::BetweenGroups:return "Between-group mean distance";}return {};}
class DistanceModel final:public QAbstractTableModel {
public:
    DistanceModel(const Document& d,QObject* parent):QAbstractTableModel(parent){for(const auto& r:d.rows())source_[r.id]=&r;}
    void clear(){beginResetModel();result.reset();order.reset();endResetModel();}
    void set(std::shared_ptr<const DistanceResult> r,std::shared_ptr<const SimilarityOrder> o){beginResetModel();result=std::move(r);order=std::move(o);endResetModel();}
    void display(int value){beginResetModel();display_=value;endResetModel();}
    int rowCount(const QModelIndex& parent={})const override{return parent.isValid()?0:static_cast<int>(order?order->order.size():result?result->report==DistanceReport::Matrix?result->rows.size():result->means.size():0);}
    int columnCount(const QModelIndex& parent={})const override{return parent.isValid()?0:order?3:result?result->report==DistanceReport::Matrix?static_cast<int>(result->rows.size()):13:0;}
    QVariant headerData(int section,Qt::Orientation orientation,int role)const override{
        if(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole)return {};
        if(result&&result->report==DistanceReport::Matrix){const auto& row=result->rows.at(static_cast<std::size_t>(section));return role==Qt::ToolTipRole?QString("%1\nSequence ID %2").arg(utf8(row.name)).arg(row.id):QString::number(section+1)+": "+utf8(row.name);}
        if(orientation==Qt::Vertical)return section+1;
        if(order){const QStringList h{"Position","Sequence name","Type"};return h.value(section);}
        if(role==Qt::ToolTipRole&&section==3)return "Sample SD across m valid pair distances: sqrt(sum((d_i - mean)^2)/(m-1)). N/A when m < 2.";
        if(role==Qt::ToolTipRole&&section==4)return "Naive SE = SD/sqrt(m). Assumes independent pairs; shared sequences cause dependence. Not a bootstrap or dependence-adjusted uncertainty estimate. N/A when m < 2.";
        const QStringList h{"Group 1","Group 2","Mean distance","SD (pairs)","SE (naive)","Valid pairs","Excluded pairs","No overlap","Below minimum","Saturated","Mean comparable sites","Min comparable","Max comparable"};return h.value(section);
    }
    QVariant data(const QModelIndex& index,int role)const override{
        if(!index.isValid()||index.row()>=rowCount()||index.column()>=columnCount())return {};
        if(role==Qt::TextAlignmentRole)return int((index.column()<2?Qt::AlignLeft:Qt::AlignRight)|Qt::AlignVCenter);
        if(role!=Qt::DisplayRole&&role!=Qt::ToolTipRole)return {};
        if(order){const auto* row=source_.at(order->order[static_cast<std::size_t>(index.row())]);if(index.column()==0)return index.row()+1;if(index.column()==1)return utf8(row->name);return row->kind==RowKind::Nucleotide?"Nucleotide":row->kind==RowKind::Protein?"Protein":"Mask";}
        if(!result)return {};
        if(result->report==DistanceReport::Matrix){const auto& p=result->pair(static_cast<std::size_t>(index.row()),static_cast<std::size_t>(index.column()));if(role==Qt::ToolTipRole)return QString("Distance: %1\nComparable sites: %2\nObserved differences: %3\n%4").arg(p.valid()?number(p.value):"N/A").arg(p.comparable).arg(p.differences).arg(utf8(distance_failure_name(p.failure)));if(display_==1)return QVariant::fromValue<qulonglong>(p.comparable);if(display_==2)return p.comparable?QVariant::fromValue<qulonglong>(p.differences):QVariant("N/A");return p.valid()?number(p.value):QString("N/A");}
        const auto& m=result->means[static_cast<std::size_t>(index.row())];
        if(role==Qt::ToolTipRole&&(index.column()==3||index.column()==4))return m.valid<2?QVariant("N/A: at least two valid pair distances are required for sample SD and naive SE."):headerData(index.column(),Qt::Horizontal,role);
        if(role==Qt::ToolTipRole&&!m.valid)return "N/A: no valid pairs. A within-group estimate needs at least two eligible rows. Excluded-pair columns identify missing overlap, minimum-overlap failures and saturation.";
        switch(index.column()){
        case 0:return result->report==DistanceReport::Overall?QString("Overall"):utf8(result->groups[m.first].name);
        case 1:return result->report==DistanceReport::Overall?QString("Overall"):utf8(result->groups[m.second].name);
        case 2:return m.valid?number(m.value()):QString("N/A");
        case 3:return m.sample_sd()?number(*m.sample_sd()):QString("N/A");
        case 4:return m.naive_se()?number(*m.naive_se()):QString("N/A");
        case 5:return QVariant::fromValue<qulonglong>(m.valid);case 6:return QVariant::fromValue<qulonglong>(m.excluded());case 7:return QVariant::fromValue<qulonglong>(m.noOverlap);case 8:return QVariant::fromValue<qulonglong>(m.belowMinimum);case 9:return QVariant::fromValue<qulonglong>(m.saturated);
        case 10:return m.valid?number(static_cast<double>(m.comparableSum/static_cast<long double>(m.valid))):QString("N/A");case 11:return m.valid?QString::number(m.minimumComparable):QString("N/A");case 12:return m.valid?QString::number(m.maximumComparable):QString("N/A");default:return {};
        }
    }
    std::shared_ptr<const DistanceResult> result;std::shared_ptr<const SimilarityOrder> order;
private:
    std::unordered_map<SequenceId,const Sequence*> source_;int display_=0;
};
struct Job {std::shared_ptr<DistanceResult> result;std::shared_ptr<SimilarityOrder> order;std::string text;QString error,path;};
class DistanceDialog final:public QDialog {
public:
    DistanceDialog(std::shared_ptr<Document> source,std::vector<SequenceId> selected,std::optional<Document::ColumnRange> columns,DistanceReport report,bool sorting,QWidget* parent):QDialog(parent),source_(std::move(source)),ids_(std::move(selected)),columns_(columns),report_(report),sorting_(sorting){
        setObjectName(sorting?"similarityDialog":"distanceDialog");setWindowTitle(sorting?"Sort by decreasing similarity":title(report));resize(1080,740);auto* layout=new QVBoxLayout(this);auto* form=new QFormLayout;layout->addLayout(form);
        molecule_=new QComboBox(this);molecule_->setObjectName("distanceMolecule");molecule_->addItem("Nucleotide",static_cast<int>(RowKind::Nucleotide));molecule_->addItem("Protein",static_cast<int>(RowKind::Protein));for(const auto& row:source_->rows())if(row.kind!=RowKind::Mask){molecule_->setCurrentIndex(row.kind==RowKind::Protein?1:0);break;}form->addRow("Molecule type",molecule_);
        rows_=new QComboBox(this);rows_->setObjectName("distanceRows");rows_->addItems({"All rows of chosen molecule type",QString("Selected sequence names (%1, snapshot)").arg(ids_.size())});form->addRow("Rows",rows_);
        columnsBox_=new QComboBox(this);columnsBox_->setObjectName("distanceColumns");columnsBox_->addItems({"All columns of included sequences","Selected alignment columns (snapshot)"});form->addRow("Columns",columnsBox_);
        method_=new QComboBox(this);method_->setObjectName("distanceMethod");form->addRow("Distance method",method_);
        deletion_=new QComboBox(this);deletion_->setObjectName("distanceDeletion");deletion_->addItems({"Pairwise deletion","Complete deletion across included rows"});deletion_->setToolTip("Pairwise: use definite residues shared by each pair. Complete: first remove every column with a missing or ambiguous call in any included row, across the entire analysis scope.");form->addRow("Gaps / missing data",deletion_);
        minimum_=new QSpinBox(this);minimum_->setObjectName("distanceMinimum");minimum_->setRange(1,1000000000);minimum_->setValue(1);minimum_->setToolTip("Pairs with fewer definite shared sites are unavailable, never zero. Increase this to avoid comparisons supported by very short overlaps.");form->addRow("Minimum comparable sites",minimum_);
        ungrouped_=new QCheckBox("Include ungrouped sequences (Ungrouped is a separate group for group means)",this);ungrouped_->setObjectName("distanceUngrouped");ungrouped_->setChecked(true);layout->addWidget(ungrouped_);
        auto* note=new QLabel(sorting?"N, ?, other ambiguities, gaps and missing cells are excluded from both difference and comparable-site counts; N–N is not a match. Only definite A/C/G/T (U=T) or the 20 standard amino acids are compared. Nearest-neighbor chain: retain the first eligible row, then choose the closest remaining row to the last one placed using observed difference counts. Ties prefer more comparable sites, then original order. Rows without any valid comparison go last, except the retained first row. Disconnected components restart in original order. Only eligible row slots are reordered; masks and other rows stay in place. Preview first;":"Only definite A/C/G/T (U=T) or the 20 standard amino acids are compared. Ambiguities, gaps, missing cells, protein U/O and stops are excluded. Masks and the other molecule type are excluded. N/A means no overlap, insufficient overlap, or model saturation. Means average valid pair distances equally; excluded pairs are counted. Analysis uses underlying sequence data, independent of the translation display.",this);note->setWordWrap(true);if(sorting){auto* toggle=new QToolButton;toggle->setText("Similarity calculation rules");toggle->setCheckable(true);toggle->setArrowType(Qt::RightArrow);toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);layout->addWidget(toggle,0,Qt::AlignLeft);note->hide();connect(toggle,&QToolButton::toggled,this,[note,toggle](bool on){note->setVisible(on);toggle->setArrowType(on?Qt::DownArrow:Qt::RightArrow);});}layout->addWidget(note);
        if(!sorting&&report!=DistanceReport::Matrix){auto* dispersion=new QLabel("m = valid pair distances. SD = sqrt(sum((d - mean)^2)/(m - 1)); SE (naive) = SD/sqrt(m). Both are N/A for m < 2. SE assumes independent pairs, but pairs sharing sequences are dependent; it is not a bootstrap uncertainty estimate.",this);dispersion->setObjectName("distanceDispersionExplanation");dispersion->setWordWrap(true);layout->addWidget(dispersion);}
        assumption_=new QLabel(this);assumption_->setWordWrap(true);assumption_->setObjectName("distanceAssumptions");layout->addWidget(assumption_);
        estimate_=new QLabel(this);estimate_->setObjectName("distanceEstimate");estimate_->setWordWrap(true);layout->addWidget(estimate_);
        display_=new QComboBox(this);display_->setObjectName("distanceDisplay");display_->addItems({"Show distances","Show comparable-site counts","Show observed difference counts"});display_->setVisible(!sorting&&report==DistanceReport::Matrix);layout->addWidget(display_);
        model_=new DistanceModel(*source_,this);table_=new QTableView(this);table_->setObjectName("distanceTable");table_->setModel(model_);table_->setAlternatingRowColors(true);table_->setEditTriggers(QAbstractItemView::NoEditTriggers);table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table_->horizontalHeader()->setDefaultSectionSize(120);layout->addWidget(table_,1);
        status_=new QLabel("Choose settings, then Calculate.",this);status_->setObjectName("distanceStatus");status_->setWordWrap(true);layout->addWidget(status_);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close,this);layout->addWidget(buttons);close_=buttons->button(QDialogButtonBox::Close);calculate_=buttons->addButton(sorting?"Preview order":"Calculate",QDialogButtonBox::ActionRole);calculate_->setObjectName("distanceCalculate");calculate_->setDefault(true);cancelButton_=buttons->addButton("Cancel calculation",QDialogButtonBox::ActionRole);cancelButton_->setObjectName("distanceCancel");cancelButton_->setEnabled(false);
        copy_=buttons->addButton("Copy TSV",QDialogButtonBox::ActionRole);copy_->setObjectName("distanceCopy");copy_->setToolTip("Copy the full analysis with settings, sequence labels and counts. Clipboard limit: 64 MiB; larger tables can be exported to a file.");export_=buttons->addButton("Export TSV…",QDialogButtonBox::ActionRole);export_->setObjectName("distanceExport");copy_->setVisible(!sorting);export_->setVisible(!sorting);copy_->setEnabled(false);export_->setEnabled(false);
        apply_=buttons->addButton("Apply order",QDialogButtonBox::AcceptRole);apply_->setObjectName("similarityApply");apply_->setVisible(sorting);apply_->setEnabled(false);
        connect(buttons,&QDialogButtonBox::rejected,this,&DistanceDialog::reject);connect(apply_,&QPushButton::clicked,this,[this]{if(model_->order)accept();});connect(calculate_,&QPushButton::clicked,this,[this]{run(0);});connect(cancelButton_,&QPushButton::clicked,this,[this]{if(cancel_)cancel_->store(true);cancelButton_->setEnabled(false);status_->setText("Cancelling…");});connect(copy_,&QPushButton::clicked,this,[this]{run(1);});connect(export_,&QPushButton::clicked,this,[this]{const auto path=choose_export_path(this,"Export distances","distances.tsv","Tab-separated table (*.tsv);;All files (*)",OutputFormat::Table,source_.get());if(!path.isEmpty())run(2,path);});
        connect(molecule_,&QComboBox::currentIndexChanged,this,[this]{fill_methods();invalidate();});for(auto* box:{rows_,columnsBox_,method_,deletion_})connect(box,&QComboBox::currentIndexChanged,this,[this]{invalidate();});connect(minimum_,&QSpinBox::valueChanged,this,[this]{invalidate();});connect(ungrouped_,&QCheckBox::toggled,this,[this]{invalidate();});connect(display_,&QComboBox::currentIndexChanged,this,[this](int i){model_->display(i);});
        timer_=new QTimer(this);timer_->setInterval(100);connect(timer_,&QTimer::timeout,this,[this]{if(cancel_&&cancel_->load())return;const auto total=total_->load(),done=done_->load();status_->setText(total?QString("Processing comparisons: %1 / %2 (%3%)").arg(done).arg(total).arg(static_cast<int>(100.L*static_cast<long double>(done)/static_cast<long double>(total))):QString("Preparing columns / writing table…"));});fill_methods();invalidate();
    }
    std::shared_ptr<const SimilarityOrder> order()const{return model_->order;}
protected:
    void reject()override{if(running_){closing_=true;cancel_->store(true);close_->setEnabled(false);status_->setText("Cancelling…");return;}QDialog::reject();}
private:
    void fill_methods(){const QSignalBlocker block(method_);const auto previous=method_->currentData();method_->clear();for(auto m:{DistanceMethod::Differences,DistanceMethod::PDistance,DistanceMethod::JukesCantor,DistanceMethod::Kimura2,DistanceMethod::Poisson}){
        if(sorting_&&m!=DistanceMethod::Differences)continue;
        const bool protein=molecule_->currentIndex()==1;if((protein&&(m==DistanceMethod::JukesCantor||m==DistanceMethod::Kimura2))||(!protein&&m==DistanceMethod::Poisson))continue;method_->addItem(utf8(distance_method_name(m)),static_cast<int>(m));}
        const auto index=method_->findData(previous);method_->setCurrentIndex(index>=0?index:sorting_?0:method_->findData(static_cast<int>(DistanceMethod::PDistance)));method_->setEnabled(!sorting_);
    }
    DistanceOptions options()const{DistanceOptions o;o.scope.molecule=static_cast<RowKind>(molecule_->currentData().toInt());o.scope.selectedRows=rows_->currentIndex()==1;o.scope.ids=ids_;o.scope.selectedColumns=columnsBox_->currentIndex()==1;if(o.scope.selectedColumns){if(!columns_)throw std::invalid_argument("No columns were selected when this window opened. Close and select a fragment or columns first.");o.scope.begin=columns_->start;o.scope.end=columns_->start+columns_->width;}o.method=static_cast<DistanceMethod>(method_->currentData().toInt());o.deletion=static_cast<DistanceDeletion>(deletion_->currentIndex());o.minimumSites=static_cast<std::size_t>(minimum_->value());o.includeUngrouped=ungrouped_->isChecked();return o;}
    void invalidate(){if(running_)return;model_->clear();copy_->setEnabled(false);export_->setEnabled(false);apply_->setEnabled(false);status_->setText("Settings changed. Calculate to refresh results.");try{const auto o=options();const auto e=estimate_distances(*source_,o,sorting_?DistanceReport::Overall:report_);calculate_->setEnabled(true);estimate_->setText(QString("%1 eligible sequences · %2 columns · %3 unordered pairs · %4").arg(e.rows).arg(e.end-e.begin).arg(e.pairs).arg(sorting_?QString("Two streaming passes; no full distance matrix"):QString("Result storage ≈ %1 MiB (128 MiB limit)").arg(static_cast<double>(e.bytes)/1048576.0,0,'f',2)));switch(o.method){case DistanceMethod::Differences:assumption_->setText("Observed differences are counts, without length normalization. Short overlaps can give smaller counts; use the minimum-overlap setting.");break;case DistanceMethod::PDistance:assumption_->setText("p-distance = differences / comparable sites; shown as a fraction, not a percentage. No multiple-hit correction.");break;case DistanceMethod::JukesCantor:assumption_->setText("Jukes–Cantor assumes equal base frequencies and uniform substitution rates. N/A at p ≥ 0.75; units: substitutions/site.");break;case DistanceMethod::Kimura2:assumption_->setText("Kimura 2-parameter distinguishes transitions/transversions, with equal base frequencies and uniform rates among sites. N/A outside the logarithm domains; units: substitutions/site.");break;case DistanceMethod::Poisson:assumption_->setText("Poisson correction uses −ln(1−p), with uniform rates among sites. N/A at p = 1; units: substitutions/site.");break;}}catch(const std::exception& e){calculate_->setEnabled(false);estimate_->setText(QString::fromUtf8(e.what()));}}
    void running(bool on){running_=on;for(auto* w:std::array<QWidget*,7>{molecule_,rows_,columnsBox_,method_,deletion_,minimum_,ungrouped_})w->setEnabled(!on);if(sorting_)method_->setEnabled(false);calculate_->setEnabled(!on);copy_->setEnabled(!on&&bool(model_->result));export_->setEnabled(!on&&bool(model_->result));apply_->setEnabled(!on&&bool(model_->order));cancelButton_->setEnabled(on);}
    void run(int operation,const QString& path={}){
        if(running_||((operation==1||operation==2)&&!model_->result))return;
        DistanceOptions o;try{o=options();if(!operation)(void)estimate_distances(*source_,o,sorting_?DistanceReport::Overall:report_);}catch(const std::exception& e){status_->setText(QString::fromUtf8(e.what()));return;}
        if(!operation)model_->clear();
        running(true);cancel_=std::make_shared<std::atomic_bool>(false);done_=std::make_shared<std::atomic<std::uint64_t>>(0);total_=std::make_shared<std::atomic<std::uint64_t>>(0);timer_->start();status_->setText("Preparing calculation…");const auto cancel=cancel_;const auto done=done_,total=total_;const auto source=source_;const auto result=model_->result;const auto report=report_;const auto sorting=sorting_;
        auto* watcher=new QFutureWatcher<Job>(this);connect(watcher,&QFutureWatcher<Job>::finished,this,[this,watcher,operation]{auto job=watcher->result();watcher->deleteLater();timer_->stop();running(false);if(closing_){QDialog::reject();return;}if(!job.error.isEmpty()){status_->setText(job.error);return;}
            // A committed export is complete even if Cancel was clicked before
            // the GUI received the worker's completion signal.
            if(operation==2){status_->setText("Exported TSV: "+job.path);return;}
            if(cancel_->load()){status_->setText("Cancelled. Alignment data unchanged.");return;}
            if(operation==1){QApplication::clipboard()->setText(utf8(job.text));status_->setText("Copied full analysis as TSV, including settings and counts.");return;}model_->set(job.result,job.order);copy_->setEnabled(bool(job.result));export_->setEnabled(bool(job.result));apply_->setEnabled(bool(job.order));if(job.order){table_->setColumnWidth(1,400);status_->setText(QString("Preview: %1 eligible rows · %2 without any valid comparison. Apply changes the row order; Cancel leaves it unchanged.").arg(job.order->eligible).arg(job.order->uncomparable));}else{status_->setText(QString("Calculated %1 sequences · %2 columns. N/A is explained on hover. Copy/Export include the complete analysis and settings.").arg(job.result->rows.size()).arg(job.result->end-job.result->begin));if(job.result->report!=DistanceReport::Matrix&&job.result->means.empty())status_->setText("No between-group pairs: at least two groups with eligible rows are required.");}});
        watcher->setFuture(QtConcurrent::run([source,o,report,sorting,operation,path,result,cancel,done,total]{Job job;try{const auto stop=[cancel]{return cancel->load();};const auto progress=[done,total](std::uint64_t a,std::uint64_t b){done->store(a);total->store(b);};if(!operation){if(sorting)job.order=std::make_shared<SimilarityOrder>(similarity_order(*source,o,stop,progress));else job.result=std::make_shared<DistanceResult>(calculate_distances(*source,o,report,stop,progress));}else if(operation==1)job.text=distance_tsv(*result,64*1024*1024,stop);else{const auto error=write_export_atomic(path,OutputFormat::Table,source.get(),[&](auto& out){write_distance_tsv(*result,[&](std::string_view chunk){out.write(chunk.data(),static_cast<std::streamsize>(chunk.size()));},stop);},stop);if(!error.isEmpty())throw std::runtime_error(error.toStdString());job.path=path;}}catch(const std::exception& e){job.error=QString::fromUtf8(e.what());}return job;}));
    }
    std::shared_ptr<Document> source_;std::vector<SequenceId> ids_;std::optional<Document::ColumnRange> columns_;DistanceReport report_;bool sorting_=false,running_=false,closing_=false;
    QComboBox *molecule_,*rows_,*columnsBox_,*method_,*deletion_,*display_;QSpinBox* minimum_;QCheckBox* ungrouped_;QLabel *status_,*estimate_,*assumption_;DistanceModel* model_;QTableView* table_;QPushButton *calculate_,*copy_,*export_,*cancelButton_,*close_,*apply_;QTimer* timer_;
    std::shared_ptr<std::atomic_bool> cancel_;std::shared_ptr<std::atomic<std::uint64_t>> done_,total_;
};
}
void MainWindow::show_distances(int report,bool sorting){
    if(busy_)return;
    std::optional<Document::ColumnRange> columns;if(const auto& f=view_->fragment())columns=Document::ColumnRange{f->start,f->width};const auto state=document_->state_id();DistanceDialog dialog(document_,view_->selected_ids(),columns,static_cast<DistanceReport>(report),sorting,this);set_busy(true,sorting?"Similarity sorting…":"Distance analysis…");const auto accepted=dialog.exec()==QDialog::Accepted;set_busy(false);if(accepted&&sorting&&dialog.order()){if(document_->state_id()!=state){QMessageBox::warning(this,"Similarity sorting","The alignment changed. Reopen the preview before applying.");return;}try{document_->reorder(dialog.order()->order);view_->refresh();update_state();}catch(const std::exception& e){QMessageBox::warning(this,"Similarity sorting",QString::fromUtf8(e.what()));}}
}
}
