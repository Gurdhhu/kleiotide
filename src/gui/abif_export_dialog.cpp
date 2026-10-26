#include "gui/abif_export_dialog.hpp"
#include "gui/file_output.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "core/abif_export.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
namespace kleiotide {
namespace {
QString text(const std::string& s){return QString::fromUtf8(s.data(),static_cast<qsizetype>(s.size()));}
struct Prepared {std::shared_ptr<AbifExport> plan;QString error;};
class ExportDialog final:public QDialog {
public:
    ExportDialog(std::shared_ptr<const Document> source,SequenceId preferred,QWidget* parent):QDialog(parent),source_(std::move(source)){
        setObjectName("abifExportDialog");setWindowTitle("Export edited trace (AB1)");resize(780,450);auto* layout=new QVBoxLayout(this);auto* form=new QFormLayout;layout->addLayout(form);choice_=new QComboBox(this);choice_->setObjectName("abifExportSequence");int active=0;
        for(std::size_t i=0;i<source_->rows().size();++i){const auto& row=source_->rows()[i];if(!row.trace)continue;choice_->addItem(QString("%1. %2").arg(i+1).arg(text(row.name)),QVariant::fromValue<qulonglong>(row.id));if(row.id==preferred)active=choice_->count()-1;}
        choice_->setCurrentIndex(active);form->addRow("Trace-linked sequence",choice_);
        auto* policy=new QLabel("Export the current edited sequence with its chromatogram signal. Alignment gaps are omitted. Both AB1 call sets contain the edited sequence so readers choosing either set see the edits. Unchanged calls keep their measured quality; substituted, inserted and unscored calls receive Q0.",this);policy->setWordWrap(true);layout->addWidget(policy);
        auto* positions=new QLabel("Inserted calls receive interpolated positions, which can coincide in crowded regions. Signal samples are preserved; a reverse-complemented sequence exports reversed and complemented channels. Unavailable instrument/run metadata uses labeled compatibility placeholders. Save a .kleio session for the complete editing session and original evidence.",this);positions->setWordWrap(true);layout->addWidget(positions);
        summary_=new QLabel(this);summary_->setObjectName("abifExportSummary");summary_->setWordWrap(true);summary_->setTextFormat(Qt::PlainText);layout->addWidget(summary_);layout->addStretch();
        status_=new QLabel(this);status_->setObjectName("abifExportStatus");status_->setWordWrap(true);status_->setTextFormat(Qt::PlainText);layout->addWidget(status_);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel,this);layout->addWidget(buttons);cancel_=buttons->button(QDialogButtonBox::Cancel);export_=buttons->addButton("Export AB1…",QDialogButtonBox::AcceptRole);export_->setObjectName("abifExportWrite");export_->setEnabled(false);
        connect(buttons,&QDialogButtonBox::rejected,this,&ExportDialog::reject);connect(export_,&QPushButton::clicked,this,[this]{save();});connect(choice_,&QComboBox::currentIndexChanged,this,[this]{prepare();});QTimer::singleShot(0,this,[this]{prepare();});
    }
    QString destination;
protected:
    void reject()override{if(running_){closing_=true;cancelled_->store(true);cancel_->setEnabled(false);status_->setText("Cancelling AB1 export…");return;}QDialog::reject();}
private:
    void controls(bool running){running_=running;choice_->setEnabled(!running);export_->setEnabled(!running&&plan_!=nullptr);}
    void prepare(){
        if(running_)return;
        plan_.reset();summary_->clear();if(choice_->currentIndex()<0){status_->setText("No trace-linked nucleotide sequences are available.");return;}controls(true);closing_=false;cancelled_=std::make_shared<std::atomic_bool>(false);status_->setText("Checking calls, peak positions and signal…");auto* watcher=new QFutureWatcher<Prepared>(this);const auto source=source_;const auto id=static_cast<SequenceId>(choice_->currentData().toULongLong());const auto cancel=cancelled_;
        connect(watcher,&QFutureWatcher<Prepared>::finished,this,[this,watcher]{auto result=watcher->result();watcher->deleteLater();controls(false);if(closing_){QDialog::reject();return;}if(!result.error.isEmpty()){status_->setText(result.error);return;}plan_=std::move(result.plan);controls(false);summary_->setText(QString("%1 calls · %2 signal samples per channel · %3 alignment gaps omitted\n%4 inserted · %5 substituted · %6 calls retain measured Q · %7 unchanged calls without Q\nOrientation: %8 · %9 coincident call positions").arg(plan_->calls.size()).arg(plan_->signal->channels[0].size()).arg(plan_->omittedGaps).arg(plan_->inserted).arg(plan_->substituted).arg(plan_->measuredQuality).arg(plan_->missingQuality).arg(plan_->reversed?"reverse complement":"forward").arg(plan_->coincident));status_->setText(QString("Ready. Export creates a separate AB1 copy; the source sequence and imported files remain unchanged.")+(plan_->calls.size()<5?" Note: BioEdit requires at least five calls.":""));});
        watcher->setFuture(QtConcurrent::run([source,id,cancel]{Prepared r;try{r.plan=std::make_shared<AbifExport>(prepare_abif_export(source->sequence(id),[cancel]{return cancel->load();}));}catch(const std::exception& e){r.error=QString::fromUtf8(e.what());}return r;}));
    }
    void save(){
        if(running_||!plan_)return;
        const auto& row=source_->sequence(static_cast<SequenceId>(choice_->currentData().toULongLong()));auto name=QFileInfo(text(row.trace->original->sourcePath)).completeBaseName();if(name.isEmpty())name=text(row.name);name.replace(QRegularExpression("[<>:\"/\\\\|?*\\x00-\\x1f]"),"_");name=name.trimmed();if(name.isEmpty())name="trace";
        auto path=choose_output_filename(this,"Export edited AB1 copy",name+"_edited.ab1","AB1 chromatogram (*.ab1 *.abi)",nullptr,QFileDialog::DontConfirmOverwrite);
        if(path.isEmpty())return;
        path=output_path(path,OutputFormat::Abif);
        if(const auto error=output_path_error(path,OutputFormat::Abif,source_.get());!error.isEmpty()){
            status_->setText(error);return;
        }
        path=approve_output_path(this,path,OutputFormat::Abif,source_.get());
        if(path.isEmpty())return;
        controls(true);closing_=false;cancelled_=std::make_shared<std::atomic_bool>(false);status_->setText("Writing edited AB1…");auto* watcher=new QFutureWatcher<QString>(this);const auto plan=plan_;const auto cancel=cancelled_;
        connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,path]{const auto error=watcher->result();watcher->deleteLater();controls(false);if(error.isEmpty()){destination=path;accept();return;}if(closing_){QDialog::reject();return;}status_->setText(error);});
        watcher->setFuture(QtConcurrent::run([plan,path,cancel]{
            return write_output_atomic(path,[&](std::ostream& output){
                write_abif_export(output,*plan,[cancel]{return cancel->load();});
            },[cancel]{return cancel->load();});
        }));
    }
    std::shared_ptr<const Document> source_;std::shared_ptr<AbifExport> plan_;QComboBox* choice_;QLabel *summary_,*status_;QPushButton *export_,*cancel_;bool running_=false,closing_=false;std::shared_ptr<std::atomic_bool> cancelled_;
};
}
QString export_abif_dialog(std::shared_ptr<const Document> source,SequenceId preferred,QWidget* parent){ExportDialog dialog(std::move(source),preferred,parent);dialog.exec();return dialog.destination;}
void MainWindow::export_trace(){
    if(busy_)return;
    auto preferred=traces_->canvas()->sequence_id();const auto selected=view_->selected_ids();if(selected.size()==1&&document_->sequence(selected[0]).trace)preferred=selected[0];set_busy(true,"Exporting edited trace…");const auto destination=export_abif_dialog(document_,preferred,this);set_busy(false);if(!destination.isEmpty())statusBar()->showMessage("Exported edited AB1: "+destination,12000);
}
}
