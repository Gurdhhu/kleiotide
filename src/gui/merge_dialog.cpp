#include "gui/file_output.hpp"
#include "gui/merge_dialog.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <atomic>
namespace kleiotide {
namespace {
QString text(const std::string& value){return QString::fromUtf8(value.data(),static_cast<qsizetype>(value.size()));}
struct Preview {MergePlan plan;QString error;};
class MergeDialog final:public QDialog {
public:
    MergeDialog(std::shared_ptr<Document> source,std::vector<SequenceId> selected,MergeMode mode,QWidget* parent)
        :QDialog(parent),source_(std::move(source)),selected_(std::move(selected)),mode_(mode){
        setObjectName("mergeDialog");setWindowTitle(mode==MergeMode::Aligned?"Merge selected sequences":"Find overlap and merge");resize(940,800);
        auto* layout=new QVBoxLayout(this);auto* form=new QFormLayout;layout->addLayout(form);
        std::string defaultName;allTraces_=selected_.size()>1;
        for(const auto& row:source_->rows())if(std::find(selected_.begin(),selected_.end(),row.id)!=selected_.end()){
            if(defaultName.empty())defaultName=row.name+"_merged";
            allTraces_=allTraces_&&bool(row.trace)&&row.kind==RowKind::Nucleotide;
        }
        name_=new QLineEdit(text(defaultName),this);name_->setObjectName("mergeName");name_->setMaxLength(65535);form->addRow("Result name",name_);
        minimum_=new QSpinBox(this);minimum_->setObjectName("mergeMinimumOverlap");minimum_->setRange(1,100000000);minimum_->setValue(20);minimum_->setVisible(mode==MergeMode::ExactOverlap);
        edits_=new QSpinBox(this);edits_->setObjectName("mergeMaximumEdits");edits_->setRange(0,100);edits_->setVisible(mode==MergeMode::ExactOverlap);edits_->setToolTip("Maximum substitutions plus inserted/deleted bases in each read-to-contig overlap. Terminal overhangs are free. Ambiguity codes other than N/? are matched literally: R versus A uses one difference even though their base sets overlap. Zero requires exact overlap.");
        if(mode==MergeMode::ExactOverlap){form->addRow("Minimum matching positions",minimum_);form->addRow("Maximum differences per overlap",edits_);}
        quality_=new QCheckBox("Resolve residue conflicts using original trace quality",this);quality_->setObjectName("mergeQuality");quality_->setVisible(mode==MergeMode::Aligned);
        margin_=new QSpinBox(this);margin_->setObjectName("mergeQualityMargin");margin_->setRange(0,255);margin_->setValue(10);margin_->setSuffix(" Q");margin_->setVisible(mode==MergeMode::Aligned);
        if(mode==MergeMode::Aligned){form->addRow(quality_);form->addRow("Required quality advantage",margin_);}
        contigs_=new QComboBox(this);contigs_->setObjectName("mergeContigChoice");form->addRow("Result contig",contigs_);
        notice_=new QFrame(this);notice_->setObjectName("mergeNotice");auto* noticeLayout=new QVBoxLayout(notice_);heading_=new QLabel(this);heading_->setObjectName("mergeHeading");auto font=heading_->font();font.setBold(true);font.setPointSize(font.pointSize()+2);heading_->setFont(font);heading_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);noticeLayout->addWidget(heading_);
        status_=new QLabel(this);status_->setObjectName("mergeStatus");status_->setWordWrap(true);status_->setTextFormat(Qt::PlainText);status_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);notice_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);noticeLayout->addWidget(status_);layout->addWidget(notice_);
        partial_=new QCheckBox("Use this partial contig; leave the excluded sequences unchanged",this);partial_->setObjectName("mergeAllowPartial");partial_->setVisible(false);layout->addWidget(partial_);
        auto* detailsToggle=new QToolButton(this);detailsToggle->setObjectName("mergeRulesToggle");detailsToggle->setText("Merging rules and quality policy");detailsToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);detailsToggle->setArrowType(Qt::RightArrow);detailsToggle->setCheckable(true);layout->addWidget(detailsToggle,0,Qt::AlignLeft);
        auto* details=new QLabel(mode==MergeMode::Aligned?
            "Uses the current alignment columns. Terminal regions containing only N, ? and gaps supply no evidence. Conflicting residues give an IUPAC union; internal gap conflicts retain the residue. Q resolution requires linked traces on every selected row, a concrete highest-Q original call and the chosen advantage over different calls. Missing Q, ties, edited/inserted calls or indels retain the ordinary consensus.":
            "Uses the current orientation after removing alignment gaps. N/? keep their positions but supply no matching evidence. Zero differences means exact overlap; otherwise each substitution or internal inserted/deleted base costs one. Terminal overhangs are free. The minimum counts informative matching positions. Ambiguity codes other than N/? are compared literally: R versus A costs one search difference but produces no consensus conflict. Counts describe different things: search events are summed across read additions; conflict positions are unique final columns with incompatible evidence. Each added read must meet these limits against the current contig consensus. Among overlaps meeting the minimum, fewer differences win; matching support breaks ties. Equally strong repeat placements are deferred. Components are searched separately; choose one and explicitly enable a partial result when reads are excluded.",this);
        details->setWordWrap(true);details->setVisible(false);layout->addWidget(details);connect(detailsToggle,&QToolButton::toggled,this,[details,detailsToggle](bool on){details->setVisible(on);detailsToggle->setArrowType(on?Qt::DownArrow:Qt::RightArrow);});
        auto* policy=new QLabel("Original rows and traces are retained. Apply appends one ungrouped consensus in one Undo step. Lowercase is preserved where one informative read supplies a position.",this);policy->setWordWrap(true);layout->addWidget(policy);
        auto* legendRow=new QHBoxLayout;auto* legend=new QLabel("Conflict sites: red background · Q-resolved sites: green background",this);legend->setWordWrap(true);legendRow->addWidget(legend,1);
        layout->addLayout(legendRow);
        sequence_=new QPlainTextEdit(this);sequence_->setObjectName("mergeSequence");sequence_->setReadOnly(true);sequence_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));sequence_->setMinimumHeight(100);sequence_->setMaximumHeight(160);layout->addWidget(sequence_);
        placements_=new QTableWidget(this);placements_->setObjectName("mergePlacements");placements_->setColumnCount(4);placements_->setHorizontalHeaderLabels({"Source sequence","Result / exclusion reason","Start","End"});placements_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Interactive);placements_->setColumnWidth(0,240);placements_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);placements_->setColumnWidth(2,70);placements_->setColumnWidth(3,70);placements_->setEditTriggers(QAbstractItemView::NoEditTriggers);placements_->setAlternatingRowColors(true);layout->addWidget(placements_,1);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel,this);layout->addWidget(buttons);
        apply_=buttons->button(QDialogButtonBox::Apply);apply_->setObjectName("mergeApply");apply_->setToolTip("Append the reviewed merged sequence to the alignment.");preview_=buttons->addButton("Preview",QDialogButtonBox::ActionRole);preview_->setObjectName("mergePreview");save_=buttons->addButton("Save FASTA…",QDialogButtonBox::ActionRole);save_->setObjectName("mergeSaveFasta");cancel_=buttons->button(QDialogButtonBox::Cancel);
        connect(buttons,&QDialogButtonBox::rejected,this,&MergeDialog::reject);connect(preview_,&QPushButton::clicked,this,[this]{preview();});connect(apply_,&QPushButton::clicked,this,[this]{apply();});connect(save_,&QPushButton::clicked,this,[this]{save_fasta();});
        connect(quality_,&QCheckBox::toggled,this,[this]{invalidate();});connect(minimum_,&QSpinBox::valueChanged,this,[this]{invalidate();});connect(margin_,&QSpinBox::valueChanged,this,[this]{invalidate();});connect(edits_,&QSpinBox::valueChanged,this,[this]{invalidate();});
        connect(name_,&QLineEdit::textChanged,this,[this]{controls();});connect(name_,&QLineEdit::textEdited,this,[this]{nameEdited_=true;});connect(partial_,&QCheckBox::toggled,this,[this]{controls();});
        connect(contigs_,&QComboBox::currentIndexChanged,this,[this](int index){if(index<0||!plan_||plan_->contigs.empty())return;choose_merge_contig(*plan_,static_cast<std::size_t>(index));partial_->setChecked(false);render();});
        invalidate();QTimer::singleShot(0,this,[this]{preview();});
    }
    std::optional<SequenceId> appended;
protected:
    void reject()override{if(running_){closing_=true;cancelled_->store(true);cancel_->setEnabled(false);notice("working","Cancelling…","The source alignment has not been changed.");return;}QDialog::reject();}
private:
    void notice(const char* state,const QString& heading,const QString& message){
        heading_->setText(heading);status_->setText(message);
        status_->setMinimumHeight(status_->fontMetrics().lineSpacing()*static_cast<int>(message.count('\n')+1));
        const QString color=QString(state)=="error"?"#fde7e9":QString(state)=="partial"?"#fff1ce":QString(state)=="ready"?"#e1f3eb":"#e7effa";
        const QString border=QString(state)=="error"?"#b42332":QString(state)=="partial"?"#976100":QString(state)=="ready"?"#237a53":"#32669c";
        notice_->setStyleSheet(QString("QFrame#mergeNotice { background: %1; border: 2px solid %2; border-radius: 5px; } QFrame#mergeNotice QLabel { color: #17212b; background: transparent; border: none; }").arg(color,border));
        notice_->setProperty("mergeState",state);
        notice_->layout()->invalidate();notice_->layout()->activate();
        notice_->setMinimumHeight(notice_->layout()->minimumSize().height()+4);
        if(layout()){layout()->invalidate();layout()->activate();}
    }
    bool usable()const{return plan_&&plan_->ids.size()>1&&!plan_->symbols.empty()&&(plan_->omitted.empty()||partial_->isChecked())&&!name_->text().trimmed().isEmpty();}
    void controls(){
        name_->setEnabled(!running_);minimum_->setEnabled(!running_);edits_->setEnabled(!running_);contigs_->setEnabled(!running_&&plan_&&plan_->contigs.size()>1);partial_->setEnabled(!running_);
        quality_->setEnabled(!running_&&mode_==MergeMode::Aligned&&allTraces_);margin_->setEnabled(!running_&&quality_->isChecked());preview_->setEnabled(!running_&&selected_.size()>1);
        apply_->setEnabled(!running_&&usable());save_->setEnabled(!running_&&usable());
    }
    void invalidate(){plan_.reset();sequence_->clear();sequence_->setExtraSelections({});placements_->setRowCount(0);{QSignalBlocker guard(contigs_);contigs_->clear();}partial_->setChecked(false);partial_->setVisible(false);notice("working","Preview required","Choose settings, then Preview.");controls();}
    void render(){
        if(!plan_)return;
        const auto& p=*plan_;
        if(!nameEdited_&&!p.ids.empty())name_->setText(text(source_->sequence(p.ids.front()).name+"_merged"));
        auto summary=QString("%1 of %2 selected sequences → %3 columns · %4 overlapping informative positions\n%5 conflicting positions (%6 residue, %7 indel) · %8 resolved by Q")
            .arg(p.ids.size()).arg(p.selectedIds.size()).arg(p.symbols.size()).arg(p.overlappingPositions).arg(p.conflictingPositions).arg(p.residueConflicts).arg(p.gapConflicts).arg(p.qualityResolved);
        if(mode_==MergeMode::ExactOverlap)summary+=QString("\n%1 edit events used: %2 substitutions, %3 inserted/deleted bases; %4 substitutions are compatible IUPAC symbol differences")
            .arg(p.editsUsed).arg(p.substitutionsUsed).arg(p.indelBasesUsed).arg(p.compatibleDifferencesUsed);
        if(p.ids.empty())notice("error","No contig found","No two selected reads have an unambiguous overlap within these limits. Review the per-sequence results below; lower the minimum or increase the difference allowance and Preview again.");
        else if(!p.omitted.empty())notice("partial","Partial contig — review excluded reads",summary+"\n"+QString("%1 reads excluded. Enable the partial-contig option to Apply or Save this result.").arg(p.omitted.size()));
        else notice("ready","Ready to merge",summary);
        partial_->setVisible(!p.ids.empty()&&!p.omitted.empty());sequence_->setPlainText(text(p.symbols));
        QList<QTextEdit::ExtraSelection> highlights;
        for(const auto& conflict:p.conflicts){if(conflict.column>=p.symbols.size())continue;QTextEdit::ExtraSelection mark;mark.cursor=sequence_->textCursor();mark.cursor.setPosition(static_cast<int>(conflict.column));mark.cursor.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor);mark.format.setBackground(QColor(conflict.resolvedByQuality?"#1b7650":"#ae2435"));mark.format.setForeground(Qt::white);mark.format.setFontWeight(QFont::Bold);highlights.push_back(mark);}
        sequence_->setExtraSelections(highlights);
        placements_->setRowCount(static_cast<int>(p.selectedIds.size()));
        for(std::size_t i=0;i<p.selectedIds.size();++i){const auto id=p.selectedIds[i];const auto row=static_cast<int>(i);placements_->setItem(row,0,new QTableWidgetItem(text(source_->sequence(id).name)));
            const auto found=std::find_if(p.placements.begin(),p.placements.end(),[id](const auto& place){return place.id==id;});
            QString status="Merged",start="—",end="—";
            if(found!=p.placements.end()){start=QString::number(found->start+1);end=QString::number(found->start+found->length);}else{const auto omitted=std::find_if(p.omitted.begin(),p.omitted.end(),[id](const auto& item){return item.id==id;});status=omitted!=p.omitted.end()?"Excluded: "+text(omitted->reason):"Excluded";}
            placements_->setItem(row,1,new QTableWidgetItem(status));placements_->item(row,1)->setToolTip(status);placements_->setItem(row,2,new QTableWidgetItem(start));placements_->setItem(row,3,new QTableWidgetItem(end));
        }
        controls();
    }
    void preview(){
        if(running_||selected_.size()<2)return;
        invalidate();running_=true;closing_=false;cancelled_=std::make_shared<std::atomic_bool>(false);controls();notice("working","Computing merge preview…","Searching selected reads; Cancel leaves the source alignment unchanged.");
        MergeSettings settings;settings.mode=mode_;settings.minimumOverlap=static_cast<std::size_t>(minimum_->value());settings.resolveQuality=quality_->isChecked();settings.qualityMargin=static_cast<unsigned>(margin_->value());settings.maximumEdits=static_cast<unsigned>(edits_->value());
        const auto source=source_;const auto selected=selected_;const auto cancel=cancelled_;auto* watcher=new QFutureWatcher<Preview>(this);
        connect(watcher,&QFutureWatcher<Preview>::finished,this,[this,watcher]{auto result=watcher->result();watcher->deleteLater();running_=false;if(closing_){QDialog::reject();return;}if(!result.error.isEmpty()){notice("error","Merge preview failed",result.error);controls();return;}
            plan_=std::move(result.plan);{QSignalBlocker guard(contigs_);for(std::size_t c=0;c<plan_->contigs.size();++c){const auto& contig=plan_->contigs[c];contigs_->addItem(QString("Contig %1 — %2 sequences, %3 columns").arg(c+1).arg(contig.ids.size()).arg(contig.symbols.size()));}if(plan_->contigs.empty())contigs_->addItem(mode_==MergeMode::Aligned?"Current alignment":"No contig");contigs_->setCurrentIndex(0);}render();});
        watcher->setFuture(QtConcurrent::run([source,selected,settings,cancel]{Preview result;try{result.plan=plan_sequence_merge(*source,selected,settings,[cancel]{return cancel->load();});}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;}));
    }
    void save_fasta(){
        if(running_||!usable())return;
        if(plan_->revision!=source_->state_id()){invalidate();notice("error","Merge preview expired","The alignment changed. Preview again before saving.");return;}
        const auto path=choose_export_path(this,"Save merged sequence",{},"FASTA (*.fasta *.fas *.fa);;All files (*)",OutputFormat::Fasta,source_.get());if(path.isEmpty())return;
        const auto bytes=(">"+name_->text().trimmed()+"\n"+text(plan_->symbols)+"\n").toUtf8();
        const auto error=write_export_atomic(path,OutputFormat::Fasta,source_.get(),[&](auto& out){out.write(bytes.constData(),bytes.size());});
        if(!error.isEmpty()){notice("error","Save merged sequence failed",error);return;}
        render();status_->setText(status_->text()+"\nSaved FASTA: "+path);
    }
    void apply(){if(running_||!usable())return;try{appended=apply_sequence_merge(*source_,*plan_,name_->text().trimmed().toUtf8().toStdString());accept();}catch(const std::exception& e){invalidate();notice("error","Merge preview failed",QString::fromUtf8(e.what()));}}
    std::shared_ptr<Document> source_;std::vector<SequenceId> selected_;MergeMode mode_;std::optional<MergePlan> plan_;
    bool allTraces_=false,running_=false,closing_=false,nameEdited_=false;std::shared_ptr<std::atomic_bool> cancelled_;
    QLineEdit* name_;QSpinBox *minimum_,*margin_,*edits_;QCheckBox *quality_,*partial_;QComboBox* contigs_;QFrame* notice_;QLabel *heading_,*status_;
    QPlainTextEdit* sequence_;QTableWidget* placements_;QPushButton *apply_,*preview_,*cancel_,*save_;
};
}
std::optional<SequenceId> merge_sequences_dialog(std::shared_ptr<Document> source,const std::vector<SequenceId>& selected,MergeMode mode,QWidget* parent){MergeDialog dialog(std::move(source),selected,mode,parent);dialog.exec();return dialog.appended;}
void MainWindow::merge_sequences(bool findOverlap){
    if(busy_)return;
    const auto selected=view_->selected_ids();if(selected.size()<2)return;set_busy(true,"Preparing merge…");
    const auto id=merge_sequences_dialog(document_,selected,findOverlap?MergeMode::ExactOverlap:MergeMode::Aligned,this);
    if(id){view_->refresh();view_->select_ids({*id});view_->navigate_to_cell(*id,0);}set_busy(false);update_state();
}
}
