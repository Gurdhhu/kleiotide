#include "gui/mafft_dialog.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QStandardItemModel>
#include <unordered_set>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextCursor>
#include <QVBoxLayout>

namespace kleiotide {
std::optional<MafftConfiguration> choose_mafft(QWidget* parent,std::optional<Document::ColumnRange> range,std::optional<MafftTemplateRequest> request,bool selectedOnly,RowKind molecule){
    const bool protein=molecule==RowKind::Protein;
    QSettings settings("Kleiotide","Kleiotide");QDialog dialog(parent);dialog.setWindowTitle(request?"Align selected targets to templates with MAFFT":range?"Realign selected columns with MAFFT":selectedOnly?"Align selected sequences with MAFFT":"Align whole alignment with MAFFT");dialog.resize(760,650);dialog.setObjectName("mafftSettings");
    QVBoxLayout layout(&dialog);QLabel scope(request?QString("%1 target row(s), %2 template row(s); %3.").arg(request->targets.size()).arg(request->templates.size()).arg(request->fragments?"fragment addition (--addfragments)":"full-sequence addition (--add)"):range?QString("Columns %1–%2 across all eligible rows.").arg(range->start+1).arg(range->start+range->width):selectedOnly?"Only selected sequence names; unselected rows are excluded from the result.":protein?"All protein rows in the current alignment.":"All nucleotide rows in the current alignment.");scope.setWordWrap(true);layout.addWidget(&scope);
    QLabel policy(request?"Target gaps are removed before alignment. Original columns and all non-target symbols are preserved; new insertion columns add open gaps to other rows. The result opens in a new window.":"Existing gaps in the aligned region are removed; new gaps are open (~). Mask rows are excluded; nucleotide and protein participants cannot be mixed. The result opens in a new window and the source stays available.");policy.setWordWrap(true);layout.addWidget(&policy);
    if(protein){auto* symbols=new QLabel("Protein scoring: B/Z/J/X and U/O/*/? are retained but scored as unknown (--anysymbol). Existing gaps are removed in the aligned scope; all resulting residues remain independently editable.",&dialog);symbols->setWordWrap(true);symbols->setObjectName("mafftProteinPolicy");layout.addWidget(symbols);}
    QWidget installation;QHBoxLayout installLayout(&installation);installLayout.setContentsMargins(0,0,0,0);QLineEdit launcher(settings.value("mafft/launcher",QStandardPaths::findExecutable("mafft")).toString());launcher.setObjectName("mafftLauncher");launcher.setPlaceholderText("MAFFT launcher: mafft.bat on Windows, mafft on Linux/macOS");QPushButton browse("Browse…");installLayout.addWidget(&launcher,1);installLayout.addWidget(&browse);layout.addWidget(&installation);
    QObject::connect(&browse,&QPushButton::clicked,&dialog,[&]{const auto path=QFileDialog::getOpenFileName(&dialog,"Select MAFFT launcher",launcher.text(),"MAFFT launchers (mafft mafft.bat *.exe *.cmd);;All files (*)");if(!path.isEmpty())launcher.setText(path);});
    QFormLayout form;layout.addLayout(&form);QComboBox algorithm;algorithm.setObjectName("mafftAlgorithm");algorithm.addItems({"Automatic — choose for this dataset","FFT-NS-1 — very fast","FFT-NS-2 — fast","FFT-NS-i — iterative refinement","L-INS-i — local pairwise, accurate","G-INS-i — global pairwise, similar lengths","E-INS-i — large internal gaps","PartTree — many sequences"});form.addRow("Method",&algorithm);
    QSpinBox threads;threads.setObjectName("mafftThreads");threads.setRange(-1,256);threads.setSpecialValueText("Automatic (physical cores)");threads.setValue(settings.value("mafft/threads",-1).toInt());form.addRow("Threads",&threads);
    QComboBox direction;direction.addItems({"Keep each input strand","Adjust strand — fast","Adjust strand — accurate"});direction.setObjectName("mafftDirection");direction.setToolTip("Reverse-complement oppositely oriented rows before alignment; accurate adjustment takes longer. Whole rows only.");if(range){direction.setEnabled(false);direction.setToolTip(request?"Templates and targets retain their input orientation to preserve the existing alignment.":"A fragment must retain the orientation of its surrounding sequence.");}if(request)direction.setToolTip("Adjust target strands against the fixed templates. Template strands and original column relationships must remain unchanged.");form.addRow("Nucleotide direction",&direction);
    QGroupBox advanced(protein?"Advanced protein settings":"Advanced nucleotide settings");QFormLayout advancedForm(&advanced);QSpinBox iterations;iterations.setRange(0,1000);iterations.setValue(1000);iterations.setObjectName("mafftIterations");advancedForm.addRow("Maximum refinement iterations",&iterations);
    QSpinBox retree;retree.setRange(1,10);retree.setValue(2);advancedForm.addRow("Guide-tree rebuilds (FFT-NS-i)",&retree);
    QDoubleSpinBox opening;opening.setRange(0,100);opening.setDecimals(3);opening.setValue(1.53);opening.setObjectName("mafftGapOpen");advancedForm.addRow("Gap opening cost (--op)",&opening);
    QDoubleSpinBox offset;offset.setRange(0,100);offset.setDecimals(3);offset.setValue(.123);offset.setObjectName("mafftOffset");offset.setToolTip("MAFFT's group-to-group offset parameter (--ep); E-INS-i normally uses 0.");advancedForm.addRow("Gap offset (--ep)",&offset);
    QSpinBox kimura;kimura.setRange(1,1000);kimura.setValue(200);advancedForm.addRow("Scoring distance (Kimura PAM)",&kimura);
    QDoubleSpinBox kappa;kappa.setRange(.001,100);kappa.setDecimals(3);kappa.setValue(2);advancedForm.addRow("Transition / transversion ratio",&kappa);
    QCheckBox composition(protein?"Use amino-acid composition (--fmodel)":"Use nucleotide composition (--fmodel)");advancedForm.addRow(&composition);
    QComboBox scoring;scoring.setObjectName("mafftProteinScoring");scoring.addItems({"BLOSUM","JTT PAM","Transmembrane PAM"});scoring.setToolTip("Amino-acid substitution model: general BLOSUM, evolutionary JTT, or a transmembrane-protein model.");advancedForm.addRow("Protein scoring model",&scoring);
    QComboBox blosum;blosum.setObjectName("mafftBlosum");for(int n:{30,45,62,80})blosum.addItem(QString::number(n),n);blosum.setCurrentIndex(2);blosum.setToolTip("BLOSUM matrix number (--bl). Lower numbers accommodate more divergent proteins; default 62.");advancedForm.addRow("BLOSUM matrix",&blosum);
    QSpinBox pam;pam.setObjectName("mafftProteinPam");pam.setRange(1,1000);pam.setValue(200);pam.setToolTip("Evolutionary scoring distance for JTT (--jtt) or transmembrane (--tm), in PAM units.");advancedForm.addRow("Protein PAM distance",&pam);
    auto proteinControls=[&]{advancedForm.setRowVisible(&scoring,protein);advancedForm.setRowVisible(&blosum,protein&&scoring.currentIndex()==0);advancedForm.setRowVisible(&pam,protein&&scoring.currentIndex()!=0);};proteinControls();QObject::connect(&scoring,&QComboBox::currentIndexChanged,&dialog,[&]{proteinControls();});
    if(protein){direction.setCurrentIndex(0);direction.setEnabled(false);form.setRowVisible(&direction,false);advancedForm.setRowVisible(&kimura,false);advancedForm.setRowVisible(&kappa,false);}
    QCheckBox memory("Memory-saving alignment (--memsave)");advancedForm.addRow(&memory);
    QCheckBox deterministic("Single-thread refinement for repeatable results");deterministic.setChecked(true);advancedForm.addRow(&deterministic);layout.addWidget(&advanced);
    QObject::connect(&algorithm,&QComboBox::currentIndexChanged,&dialog,[&](int i){iterations.setEnabled(i>=3&&i<=6);retree.setEnabled(i==3);iterations.setValue(1000);offset.setValue(i==6?0:.123);});iterations.setEnabled(false);retree.setEnabled(false);
    if(request){if(auto* model=qobject_cast<QStandardItemModel*>(algorithm.model()))model->item(7)->setEnabled(false);if(request->fragments){algorithm.setEnabled(false);algorithm.setToolTip("Fragment addition uses MAFFT Automatic (--auto), as recommended by MAFFT.");}}
    launcher.setToolTip("Location of your MAFFT launcher. Windows needs mafft.bat beside its complete runtime folder.");
    browse.setToolTip("Choose the locally installed MAFFT launcher.");
    if(!request||!request->fragments)algorithm.setToolTip("Choose the alignment strategy: fast progressive methods, slower iterative methods, or automatic selection by dataset size.");
    threads.setToolTip("Parallel workers: -1 chooses physical cores automatically; 1 uses one worker. More workers can require more memory.");
    iterations.setToolTip("Upper limit on refinement cycles. More cycles may improve alignment but take longer. Used by iterative methods.");
    retree.setObjectName("mafftRetree");retree.setToolTip("Number of guide-tree construction passes in FFT-NS-i's progressive stage. Additional passes cost time.");
    opening.setToolTip("Penalty for starting a gap (--op). Larger values discourage opening new gaps.");
    offset.setToolTip("Offset acting like a gap extension penalty (--ep). Larger values discourage long gaps; E-INS-i normally uses zero.");
    kimura.setObjectName("mafftKimura");kimura.setToolTip("Evolutionary distance of the nucleotide scoring matrix (Kimura PAM). Higher values model more divergent sequences.");
    kappa.setObjectName("mafftKappa");kappa.setToolTip("Relative weighting of transitions (A/G, C/T) versus transversions in the nucleotide scoring model.");
    composition.setObjectName("mafftComposition");composition.setToolTip(protein?"Adjust the scoring matrix using the dataset's amino-acid composition (--fmodel).":"Adjust the scoring matrix using the dataset's nucleotide composition (--fmodel).");
    memory.setObjectName("mafftMemory");memory.setToolTip("Use an alignment algorithm requiring less memory (--memsave), potentially taking longer.");
    deterministic.setObjectName("mafftDeterministic");deterministic.setToolTip("Disable parallel iterative refinement (--threadit 0) to avoid run-to-run variation from multithreading.");
    for(auto* f : {&form,&advancedForm})for(int row=0;row<f->rowCount();++row){
        const auto* item=f->itemAt(row,QFormLayout::FieldRole);
        if(item&&item->widget()){auto* label=f->labelForField(item->widget());if(label)label->setToolTip(item->widget()->toolTip());}
    }
    QLabel error;error.setWordWrap(true);error.setStyleSheet("color: #A03020");layout.addWidget(&error);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText("Align");layout.addWidget(&buttons);std::optional<MafftConfiguration> result;
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        MafftConfiguration config;config.templateRequest=request;config.launcher=QFileInfo(launcher.text().trimmed()).absoluteFilePath();
        if(!QFileInfo(config.launcher).isFile()||config.launcher.contains('\n')||config.launcher.contains('\r')||config.launcher.contains('"')){error.setText("Choose an existing MAFFT launcher file.");return;}
        auto& o=config.options;o.molecule=molecule;o.proteinScoring=static_cast<ProteinScoring>(scoring.currentIndex());o.blosum=blosum.currentData().toInt();o.proteinPam=pam.value();o.algorithm=static_cast<MafftAlgorithm>(algorithm.currentIndex());o.direction=static_cast<MafftDirection>(direction.currentIndex());o.threads=threads.value();o.iterations=iterations.value();o.retree=retree.value();o.gapOpen=opening.value();o.offset=offset.value();o.kimura=kimura.value();o.kappa=kappa.value();o.composition=composition.isChecked();o.memorySaving=memory.isChecked();o.deterministic=deterministic.isChecked();
        try{(void)o.arguments();}catch(const std::exception& e){error.setText(QString::fromUtf8(e.what())+" Use -1 or a positive thread count.");return;}
        settings.setValue("mafft/launcher",config.launcher);settings.setValue("mafft/threads",o.threads);result=config;dialog.accept();
    });dialog.exec();return result;
}
std::optional<MafftTemplateRequest> choose_mafft_templates(QWidget* parent,const Document& source,const std::vector<SequenceId>& selected,bool fragments){
    const std::unordered_set<SequenceId> selection(selected.begin(),selected.end());MafftTemplateRequest request;request.fragments=fragments;
    const auto molecule=mafft_molecule(source,selected);
    std::size_t skipped=0;
    for(const auto& row:source.rows())if(selection.contains(row.id)){if(row.kind==molecule&&row.symbols.residue_count())request.targets.push_back(row.id);else ++skipped;}
    QDialog dialog(parent);dialog.setObjectName("mafftTemplates");dialog.setWindowTitle(fragments?"Choose templates for fragment addition":"Choose templates for full-sequence addition");dialog.resize(760,510);QVBoxLayout layout(&dialog);
    QLabel scope(QString("Selected targets: %1. Masks and empty selected rows skipped: %2.").arg(request.targets.size()).arg(skipped));scope.setWordWrap(true);layout.addWidget(&scope);
    QLabel help("Check aligned rows of the same molecule type to use as templates. Targets and templates are separate. All other rows, including masks, retain their original column relationships.");help.setWordWrap(true);layout.addWidget(&help);
    QListWidget list;list.setObjectName("mafftTemplateRows");layout.addWidget(&list,1);
    for(std::size_t i=0;i<source.rows().size();++i){const auto& row=source.rows()[i];if(selection.contains(row.id)||row.kind!=molecule||!row.symbols.residue_count())continue;
        auto* item=new QListWidgetItem(QString("%1. %2").arg(i+1).arg(QString::fromUtf8(row.name.c_str())),&list);item->setData(Qt::UserRole,QVariant::fromValue(static_cast<qulonglong>(row.id)));item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(Qt::Checked);
    }
    QHBoxLayout controls;QPushButton all("Select all"),none("Clear");controls.addWidget(&all);controls.addWidget(&none);controls.addStretch();layout.addLayout(&controls);
    QObject::connect(&all,&QPushButton::clicked,&dialog,[&]{for(int i=0;i<list.count();++i)list.item(i)->setCheckState(Qt::Checked);});
    QObject::connect(&none,&QPushButton::clicked,&dialog,[&]{for(int i=0;i<list.count();++i)list.item(i)->setCheckState(Qt::Unchecked);});
    QLabel error;error.setWordWrap(true);error.setStyleSheet("color: #A03020");layout.addWidget(&error);
    QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText("Continue…");layout.addWidget(&buttons);
    auto validate=[&]{bool checked=false;for(int i=0;i<list.count();++i)checked|=list.item(i)->checkState()==Qt::Checked;const auto message=request.targets.empty()?"Select at least one target containing residues by its sequence name, then run this command again.":!checked?"Choose at least one separate template of the same molecule type containing residues.":"";error.setText(message);buttons.button(QDialogButtonBox::Ok)->setEnabled(request.targets.size()&&checked);};
    QObject::connect(&list,&QListWidget::itemChanged,&dialog,[&]{validate();});validate();
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,[&]{for(int i=0;i<list.count();++i)if(list.item(i)->checkState()==Qt::Checked)request.templates.push_back(list.item(i)->data(Qt::UserRole).toULongLong());dialog.accept();});
    if(dialog.exec()!=QDialog::Accepted)return {};
    return request;
}
MafftProgress::MafftProgress(QWidget* parent):QDialog(parent){
    setWindowTitle("MAFFT alignment");setObjectName("mafftProgress");resize(760,400);setAttribute(Qt::WA_DeleteOnClose);
    auto* layout=new QVBoxLayout(this);status_=new QLabel("Preparing alignment…",this);status_->setWordWrap(true);layout->addWidget(status_);
    log_=new QPlainTextEdit(this);log_->setObjectName("mafftLog");log_->setReadOnly(true);log_->setMaximumBlockCount(2000);layout->addWidget(log_,1);
    button_=new QPushButton("Cancel",this);button_->setObjectName("mafftCancel");layout->addWidget(button_,0,Qt::AlignRight);connect(button_,&QPushButton::clicked,this,[this]{reject();});
}
void MafftProgress::set_status(const QString& text){status_->setText(text);}
void MafftProgress::append_log(const QString& text){auto cursor=log_->textCursor();cursor.movePosition(QTextCursor::End);cursor.insertText(text);log_->setTextCursor(cursor);}
void MafftProgress::completed(const QString& error){running_=false;button_->setEnabled(true);button_->setText("Close");if(error.isEmpty()){accept();return;}status_->setText("Alignment was not applied. "+error);}
void MafftProgress::reject(){if(running_){button_->setEnabled(false);if(cancel_requested)cancel_requested();}else QDialog::reject();}
}
