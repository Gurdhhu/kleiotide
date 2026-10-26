#include "gui/abif_batch_export.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/abif_export.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTableWidget>
#include <QTemporaryFile>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <sstream>
#include <unordered_set>
namespace kleiotide {
namespace {
QString text(const std::string& s){return QString::fromUtf8(s.data(),static_cast<qsizetype>(s.size()));}
QString filename(QString title){
    title.replace(QRegularExpression("[<>:\"/\\\\|?*\\x00-\\x1f]"),"_");title=title.trimmed();
    while(title.endsWith('.')||title.endsWith(' '))title.chop(1);
    if(title.size()>120){title.truncate(120);if(title.back().isHighSurrogate())title.chop(1);}
    if(title.isEmpty())title="trace";
    if(QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)",QRegularExpression::CaseInsensitiveOption).match(title).hasMatch())title.prepend('_');
    return title;
}
QString filename_suffix(QString suffix){
    suffix.replace(QRegularExpression("[<>:\"/\\\\|?*\\x00-\\x1f]"),"_");
    if(suffix.size()>80){suffix.truncate(80);if(suffix.back().isHighSurrogate())suffix.chop(1);}
    return suffix;
}
class BatchDialog final:public QDialog {
public:
    BatchDialog(std::shared_ptr<const Document> source,std::vector<SequenceId> selected,QWidget* parent):QDialog(parent),source_(std::move(source)),selected_(std::move(selected)){
        setObjectName("abifBatchDialog");setWindowTitle("Batch export edited traces (AB1)");resize(850,570);auto* layout=new QVBoxLayout(this);auto* form=new QFormLayout;layout->addLayout(form);
        scope_=new QComboBox(this);scope_->setObjectName("abifBatchScope");scope_->addItems({"All traces","Selected sequence names only"});
        if(!abif_batch_items(*source_,selected_,true,{}).empty())scope_->setCurrentIndex(1);
        form->addRow("Export",scope_);auto* folder=new QHBoxLayout;directory_=new QLineEdit(this);directory_->setObjectName("abifBatchDirectory");directory_->setPlaceholderText("Choose an output folder…");browse_=new QPushButton("Browse…",this);folder->addWidget(directory_);folder->addWidget(browse_);form->addRow("Output folder",folder);
        suffix_=new QLineEdit("_edited",this);suffix_->setObjectName("abifBatchSuffix");suffix_->setMaxLength(80);suffix_->setClearButtonEnabled(true);suffix_->setToolTip("Added after each sequence title, before .ab1. Include your separator (for example _trimmed), or leave empty for no suffix. Invalid filename characters become underscores.");form->addRow("Filename suffix",suffix_);
        auto* policy=new QLabel("One edited AB1 per trace-linked sequence, in alignment order. Sequences without traces are ignored. Filenames use the current sequence titles; numbered suffixes avoid duplicates and existing files. The original files are never overwritten. Calls, quality and signal follow the same rules as single-trace export.",this);policy->setWordWrap(true);layout->addWidget(policy);
        table_=new QTableWidget(0,2,this);table_->setObjectName("abifBatchPreview");table_->setHorizontalHeaderLabels({"Sequence","Output filename"});auto* header=table_->horizontalHeader();header->setMinimumSectionSize(48);header->setSectionResizeMode(0,QHeaderView::Interactive);header->setSectionResizeMode(1,QHeaderView::Stretch);header->resizeSection(0,preferredNameWidth_);header->setToolTip("Drag the divider to resize columns. Output filenames have priority in narrow windows.");table_->viewport()->installEventFilter(this);connect(header,&QHeaderView::sectionResized,this,[this](int column,int,int width){if(column==0&&!adjustingColumns_){preferredNameWidth_=width;fit_columns();}});table_->setEditTriggers(QAbstractItemView::NoEditTriggers);layout->addWidget(table_);
        progress_=new QProgressBar(this);progress_->setObjectName("abifBatchProgress");progress_->hide();layout->addWidget(progress_);
        status_=new QLabel(this);status_->setObjectName("abifBatchStatus");status_->setTextFormat(Qt::PlainText);status_->setWordWrap(true);layout->addWidget(status_);
        report_=new QPlainTextEdit(this);report_->setObjectName("abifBatchReport");report_->setReadOnly(true);report_->setMaximumHeight(160);report_->hide();layout->addWidget(report_);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel,this);layout->addWidget(buttons);cancel_=buttons->button(QDialogButtonBox::Cancel);write_=buttons->addButton("Export traces",QDialogButtonBox::AcceptRole);write_->setObjectName("abifBatchWrite");
        connect(buttons,&QDialogButtonBox::rejected,this,&BatchDialog::reject);connect(write_,&QPushButton::clicked,this,[this]{save();});connect(scope_,&QComboBox::currentIndexChanged,this,[this]{preview();});connect(directory_,&QLineEdit::textChanged,this,[this]{preview();});connect(suffix_,&QLineEdit::textChanged,this,[this]{preview();});connect(browse_,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getExistingDirectory(this,"Export edited traces to folder",directory_->text());if(!path.isEmpty())directory_->setText(path);});preview();
    }
protected:
    bool eventFilter(QObject* watched,QEvent* event)override{if(watched==table_->viewport()&&event->type()==QEvent::Resize)fit_columns();return QDialog::eventFilter(watched,event);}
    void reject()override{if(running_){cancelled_->store(true);cancel_->setEnabled(false);status_->setText("Cancelling… Completed exports will be kept; the unfinished file will be removed.");return;}QDialog::reject();}
private:
    void fit_columns(){
        if(adjustingColumns_)return;
        const auto available=table_->viewport()->width();if(available<=0)return;
        // Preserve the user's preferred name width when space returns. When
        // constrained, reserve 280 px (or two-thirds of a very narrow table)
        // for the output name; the source-name column yields first.
        const auto outputMinimum=std::min(280,std::max(48,available*2/3));
        const auto width=std::max(48,std::min(preferredNameWidth_,available-outputMinimum));
        adjustingColumns_=true;table_->horizontalHeader()->resizeSection(0,width);adjustingColumns_=false;
    }
    void preview(){
        if(running_)return;
        items_=abif_batch_items(*source_,selected_,scope_->currentIndex()==1,directory_->text(),suffix_->text());table_->setRowCount(static_cast<int>(items_.size()));
        for(std::size_t i=0;i<items_.size();++i){table_->setItem(static_cast<int>(i),0,new QTableWidgetItem(items_[i].title));table_->setItem(static_cast<int>(i),1,new QTableWidgetItem(QFileInfo(items_[i].path).fileName()));}
        const auto valid=!directory_->text().isEmpty()&&QFileInfo(directory_->text()).isDir();write_->setEnabled(valid&&!items_.empty());
        status_->setText(items_.empty()?"No trace-linked sequences in this scope. Select sequence names, or choose All traces.":QString("%1 traces. %2").arg(items_.size()).arg(valid?"Ready to export the files shown above.":"Choose an existing output folder."));
    }
    void save(){
        if(running_||items_.empty())return;
        // Freeze the reviewed names. Later conflicts are reported, never overwritten.
        running_=true;scope_->setEnabled(false);directory_->setEnabled(false);suffix_->setEnabled(false);browse_->setEnabled(false);write_->setEnabled(false);progress_->setRange(0,static_cast<int>(items_.size()));progress_->setValue(0);progress_->show();status_->setText("Exporting edited traces…");cancelled_=std::make_shared<std::atomic_bool>(false);
        auto* watcher=new QFutureWatcher<AbifBatchResult>(this);const auto source=source_;const auto items=items_;const auto cancel=cancelled_;
        connect(watcher,&QFutureWatcher<AbifBatchResult>::finished,this,[this,watcher]{const auto result=watcher->result();watcher->deleteLater();running_=false;cancel_->setEnabled(true);cancel_->setText("Close");progress_->setValue(static_cast<int>(result.written.size()+result.errors.size()));status_->setText(QString("%1%2 exported · %3 failed · %4 not processed. Completed files are kept.").arg(result.cancelled?"Cancelled: ":"Finished: ").arg(result.written.size()).arg(result.errors.size()).arg(items_.size()-static_cast<std::size_t>(result.written.size()+result.errors.size())));QStringList lines;for(const auto& path:result.written)lines<<"Exported: "+path;lines.append(result.errors);lines.append(result.notes);report_->setPlainText(lines.join('\n'));report_->show();});
        watcher->setFuture(QtConcurrent::run([this,source,items,cancel]{return write_abif_batch(*source,items,*cancel,[this](std::size_t done){QMetaObject::invokeMethod(this,[this,done]{progress_->setValue(static_cast<int>(done));},Qt::QueuedConnection);});}));
    }
    std::shared_ptr<const Document> source_;std::vector<SequenceId> selected_;std::vector<AbifBatchItem> items_;std::shared_ptr<std::atomic_bool> cancelled_;QComboBox* scope_;QLineEdit *directory_,*suffix_;QPushButton *browse_,*write_,*cancel_;QTableWidget* table_;QProgressBar* progress_;QLabel* status_;QPlainTextEdit* report_;bool running_=false,adjustingColumns_=false;int preferredNameWidth_=240;
};
}
std::vector<AbifBatchItem> abif_batch_items(const Document& source,const std::vector<SequenceId>& selected,bool onlySelected,const QString& directory,const QString& suffix){
    const std::unordered_set<SequenceId> ids(selected.begin(),selected.end());QSet<QString> names;const auto ending=filename_suffix(suffix);
    // Case-insensitive collision avoidance also makes exports portable to Windows.
    const QDir folder(directory);if(!directory.isEmpty())for(const auto& existing:folder.entryList(QDir::AllEntries|QDir::Hidden|QDir::System|QDir::NoDotAndDotDot))names.insert(existing.toCaseFolded());
    std::vector<AbifBatchItem> items;for(const auto& row:source.rows())if(row.trace&&(!onlySelected||ids.contains(row.id))){const auto stem=filename(text(row.name))+ending;auto name=stem+".ab1";std::size_t ordinal=2;while(names.contains(name.toCaseFolded()))name=stem+"_"+QString::number(ordinal++)+".ab1";names.insert(name.toCaseFolded());items.push_back({row.id,text(row.name),folder.filePath(name)});}return items;
}
AbifBatchResult write_abif_batch(const Document& source,const std::vector<AbifBatchItem>& items,const std::atomic_bool& cancel,const std::function<void(std::size_t)>& progress){
    AbifBatchResult result;std::size_t done=0;
    for(const auto& item:items){if(cancel.load()){result.cancelled=true;break;}try{
        if(QFileInfo::exists(item.path)||QFileInfo(item.path).isSymLink())throw std::runtime_error("The output path now exists; nothing was overwritten.");
        const auto plan=prepare_abif_export(source.sequence(item.id),[&cancel]{return cancel.load();});std::ostringstream output(std::ios::binary);write_abif_export(output,plan,[&cancel]{return cancel.load();});const auto bytes=output.str();
        QTemporaryFile file(QFileInfo(item.path).absolutePath()+"/.kleiotide-ab1-XXXXXX");if(!file.open())throw std::runtime_error(file.errorString().toStdString());if(file.write(bytes.data(),static_cast<qint64>(bytes.size()))!=static_cast<qint64>(bytes.size())||!file.flush())throw std::runtime_error(file.errorString().toStdString());
        if(cancel.load()){result.cancelled=true;break;}file.close();if(!file.rename(item.path))throw std::runtime_error(file.errorString().toStdString());file.setAutoRemove(false);result.written<<item.path;if(plan.calls.size()<5)result.notes<<QString("Note: %1 — BioEdit requires at least five calls; this standard AB1 uses inline calls.").arg(item.title);
    }catch(const std::exception& e){if(cancel.load()){result.cancelled=true;break;}result.errors<<QString("Failed: %1 — %2").arg(item.title,QString::fromUtf8(e.what()));}if(progress)progress(++done);
    }return result;
}
void export_abif_batch_dialog(std::shared_ptr<const Document> source,std::vector<SequenceId> selected,QWidget* parent){BatchDialog dialog(std::move(source),std::move(selected),parent);dialog.exec();}
void MainWindow::export_traces(){if(busy_)return;const auto selected=view_->selected_ids();set_busy(true,"Exporting edited traces…");export_abif_batch_dialog(document_,selected,this);set_busy(false);}
}
