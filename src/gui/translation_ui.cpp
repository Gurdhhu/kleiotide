#include "gui/translation_ui.hpp"
#include "gui/file_output.hpp"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QRegularExpression>
#include <algorithm>
#include <sstream>
#include <set>
namespace kleiotide {
namespace {
QMenu* top_menu(MainWindow* w,const QString& name){for(auto* a:w->menuBar()->actions())if(a->text().remove('&')==name)return a->menu();return nullptr;}
QString summary(const TranslationResult& t){return QString("%1 stops · %2 possible · %3 context-dependent · %4 unknown · %5 partial-gap codons\n%6 boundary fragments omitted · %7 coding-phase errors") .arg(t.stops).arg(t.possible).arg(t.context).arg(t.unknown).arg(t.partialGaps).arg(t.map.boundaryFragments).arg(t.map.phaseErrors);}
QString code_name(const GeneticCode& code){
    switch(code.id){
    case 4:return "Mold/protozoa mito. + Mycoplasma";
    case 6:return "Ciliate/Dasyclad./Hexamita";
    case 9:return "Echinoderm/flatworm mito.";
    case 11:return "Bacteria/archaea/plastids";
    case 22:return "Scenedesmus mito.";
    case 25:return "SR1/Gracilibacteria";
    case 26:return "Pachysolen nuclear";
    default:{auto name=QString::fromStdString(code.name).simplified();name.replace("Mitochondrial","mito.").replace("Alternative","Alt.");return name;}
    }
}
QIcon translation_icon(){QPixmap p(24,24);p.fill(Qt::transparent);QPainter q(&p);q.setPen(QColor(30,75,130));auto f=q.font();f.setBold(true);f.setPixelSize(10);q.setFont(f);q.drawText(QRect(0,0,24,12),Qt::AlignCenter,"ATG");q.setPen(QColor(130,40,150));q.drawText(QRect(0,11,24,13),Qt::AlignCenter,"M");return QIcon(p);}
}
void TranslationUI::fill_codes(QComboBox* box,const TranslationSettings& s){
    QSignalBlocker block(box);box->setEditable(false);box->clear();
    for(const auto& c:genetic_codes()){
        box->addItem(QString::number(c.id)+" · "+code_name(c),c.id);
        box->setItemData(box->count()-1,"NCBI table "+QString::number(c.id)+"\n"+QString::fromStdString(c.name).simplified(),Qt::ToolTipRole);
    }
    if(s.custom){box->addItem("Custom · "+QString::fromStdString(s.custom->name),0);box->setItemData(box->count()-1,QString::fromStdString(s.custom->name),Qt::ToolTipRole);}
    box->addItem("Custom code…",-1);box->setItemData(box->count()-1,"Create or import a custom genetic code",Qt::ToolTipRole);
    box->setCurrentIndex(box->findData(s.code));box->setMaxVisibleItems(15);box->setToolTip(box->currentData(Qt::ToolTipRole).toString());
}
TranslationUI::TranslationUI(MainWindow* w):QObject(w),w_(w){
    enabled_=new QAction(translation_icon(),"Translation",this);enabled_->setObjectName("translationToggle");enabled_->setCheckable(true);enabled_->setToolTip("Color nucleotide codons by their translated amino acids. Original nucleotide sequences remain unchanged.");top_menu(w,"View")->addAction(enabled_);auto* editingBar=w->findChild<QToolBar*>("editingToolbar");editingBar->insertAction(w->findChild<QAction*>("editingToolbarStretchAction"),enabled_);
    w->addToolBarBreak();bar_=w->addToolBar("Translation");bar_->setObjectName("translationToolbar");bar_->setIconSize(QSize(28,28));bar_->setMovable(false);bar_->setFloatable(false);bar_->toggleViewAction()->setVisible(false);
    auto space=[&]{auto* spacer=new QWidget(bar_);spacer->setFixedWidth(8);spacer->setObjectName("translationControlSpacing");bar_->addWidget(spacer);};
    auto toggle=[&](const char* text,const char* id,const char* tip){auto* a=bar_->addAction(text);a->setObjectName(id);a->setCheckable(true);a->setToolTip(tip);QPixmap icon(24,24);icon.fill(Qt::transparent);QPainter painter(&icon);auto f=painter.font();f.setBold(true);f.setPixelSize(10);painter.setFont(f);painter.setPen(QColor(70,45,120));painter.drawText(icon.rect(),Qt::AlignCenter,QString::fromLatin1(std::string_view(id)=="translationLetters"?"AA":"123"));a->setIcon(QIcon(icon));return a;};
    letters_=toggle("AA code","translationLetters","Show amino-acid labels over codons, including incomplete trailing codons. This preview uses View mode; choose Edit to return to nucleotide letters.");
    space();
    ruler_=toggle("1 2 3","translationRuler","Show coding positions below alignment column numbers; dots indicate noncoding positions.");ruler_->setChecked(true);space();
    bar_->addSeparator();auto* coding=new QToolButton(bar_);coding->setText("Set selection\ncoding");coding->setToolButtonStyle(Qt::ToolButtonTextOnly);coding->setMinimumWidth(105);coding->setToolTip("Set selection coding: assign coding phases or noncoding status to the selected alignment columns.");coding->setObjectName("codingColumns");coding->setPopupMode(QToolButton::InstantPopup);auto* menu=new QMenu(coding);coding->setMenu(menu);bar_->addWidget(coding);space();
    auto mutation = [this](QMenu* owner, const QString& label, std::function<void()> callback) {
        auto* action = owner->addAction(label);
        connect(action, &QAction::triggered, this, [this, callback = std::move(callback)] {
            if (!w_->busy_) callback();
        });
        mutationActions_.push_back(action);
        return action;
    };
    const char* names[] = {"Noncoding", "Coding 1–2–3", "Coding 2–3–1", "Coding 3–1–2"};
    for (int phase : {1, 2, 3, 0}) mutation(menu, names[phase], [this, phase] { annotate(phase); });
    menu->setTitle("Set selection coding");
    menu->addSeparator();
    mutation(menu, "Reset selected columns to frame", [this] { annotate(-1); });
    mutation(menu, "Reset all coding annotations", [this] {
        auto settings = w_->document_->translation_settings();
        settings.regions.clear();
        w_->document_->set_translation_settings(std::move(settings));
        w_->update_state();
    });
    code_=new QComboBox(bar_);code_->setObjectName("translationGeneticCode");code_->setMaximumWidth(300);code_->setToolTip("NCBI genetic code used by the translation layer. Initiator codons are not recoded to methionine. Context-dependent stop codons retain their sense amino acid and are counted separately.");bar_->addWidget(code_);space();
    frame_=new QComboBox(bar_);frame_->setObjectName("translationFrame");frame_->addItems({"Frame 1","Frame 2","Frame 3"});frame_->setToolTip("First complete codon begins at alignment column 1, 2 or 3. Manual coding annotations override the frame.");bar_->addWidget(frame_);space();

    count_=new QLabel(bar_);count_->setObjectName("translationStopCount");count_->setTextFormat(Qt::PlainText);bar_->addWidget(count_);bar_->hide();
    auto* translation = top_menu(w, "Alignment")->addMenu("Translation");
    translation->addAction(enabled_);
    translation->addAction(letters_);
    translation->addAction(ruler_);
    translation->addMenu(menu);
    translation->addSeparator();
    mutation(translation, "Add translated sequences…", [this] { add_dialog(); });
    mutation(translation, "Translate to a new protein window…", [this] { add_dialog(true); });
    copy_ = translation->addAction("Copy translated fragment", [this] { copy_fragment(); });
    copy_->setObjectName("copyTranslatedFragment");
    copy_->setEnabled(false);
    copy_->setToolTip("Copy amino acids from one nucleotide fragment while Translation is active; partial codon selections expand automatically.");
    mutation(translation, "Export protein sequences…", [this] { export_proteins(); });
    translation->addSeparator();
    generateMask_ = mutation(translation, "Generate translation mask", [this] { generate_mask(); });
    generateMask_->setObjectName("generateTranslationMask");
    annotateMask_ = mutation(translation, "Annotate from mask…", [this] { annotate_mask_dialog(); });
    annotateMask_->setObjectName("annotateFromMask");
    w->view_->annotate_mask_requested = [this](SequenceId id) { annotate_mask_dialog(id); };
    mutation(top_menu(w, "Sequence"), "Add translated sequences…", [this] { add_dialog(); });
    w->view_->add_translation_requested = [this] { add_dialog(); };
    w->view_->copy_translation_requested = [this] { copy_fragment(); };
    connect(enabled_,&QAction::toggled,this,[this](bool on){bar_->setVisible(on);if(!on&&letters_->isChecked())letters_->setChecked(false);refresh();});
    connect(letters_,&QAction::toggled,this,[this](bool on){if(on){rememberedMode_=static_cast<int>(w_->view_->mode());w_->view_->set_mode(AlignmentView::Mode::View);}else if(w_->view_->mode()==AlignmentView::Mode::View)w_->view_->set_mode(static_cast<AlignmentView::Mode>(rememberedMode_));display();});
    connect(ruler_,&QAction::toggled,this,[this]{display();});
    auto set = [this] {
        if (refreshing_ || !document_) return;
        if (w_->busy_) {
            QSignalBlocker codeBlock(code_), frameBlock(frame_);
            const auto& current = w_->document_->translation_settings();
            code_->setCurrentIndex(code_->findData(current.code));
            frame_->setCurrentIndex(static_cast<int>(current.frame) - 1);
            return;
        }
        try {
            auto settings = w_->document_->translation_settings();
            settings.code = code_->currentData().toUInt();
            settings.frame = static_cast<unsigned>(frame_->currentIndex() + 1);
            w_->document_->set_translation_settings(std::move(settings));
            w_->update_state();
        } catch (const std::exception& error) {
            QMessageBox::warning(w_, "Translation", QString::fromUtf8(error.what()));
        }
    };
    connect(code_,&QComboBox::currentIndexChanged,this,[this,set]{
        if(refreshing_||code_->currentIndex()<0)return;
        if(w_->busy_){set();return;}
        if(code_->currentData().toInt()==-1){
            {QSignalBlocker blocker(code_);code_->setCurrentIndex(code_->findData(w_->document_->translation_settings().code));}
            QTimer::singleShot(0,this,[this]{custom_code();});return;
        }
        code_->setToolTip(code_->currentData(Qt::ToolTipRole).toString());set();
    });connect(frame_,&QComboBox::currentIndexChanged,this,set);
    timer_=new QTimer(this);timer_->setInterval(0);connect(timer_,&QTimer::timeout,this,[this]{run();});refresh();
}
void TranslationUI::set_available(bool available) {
    bar_->setEnabled(available);
    for (auto* action : mutationActions_) action->setEnabled(available);
    update_selection();
}
void TranslationUI::export_proteins() {
    if (w_->busy_) return;
    const auto source = w_->document_;
    if (std::none_of(source->rows().begin(), source->rows().end(), [](const auto& row) {
        return row.kind == RowKind::Protein;
    })) {
        QMessageBox::information(w_, "Export proteins", "No protein sequences in this alignment.");
        return;
    }
    auto path = choose_output_filename(w_, "Export protein sequences", {},
        "FASTA (*.faa *.fasta)", nullptr, QFileDialog::DontConfirmOverwrite);
    path = approve_output_path(w_, path, OutputFormat::Fasta, source.get());
    if (path.isEmpty()) return;
    w_->set_busy(true, "Exporting protein sequences…");
    auto* watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, path] {
        const auto error = watcher->result();
        watcher->deleteLater();
        w_->set_busy(false);
        if (!error.isEmpty()) QMessageBox::warning(w_, "Export proteins", error);
        else w_->statusBar()->showMessage("Exported protein sequences: " + path, 8000);
    });
    watcher->setFuture(QtConcurrent::run([source, path] {
        return write_output_atomic(path, [&](std::ostream& output) {
            for (const auto& row : source->rows()) if (row.kind == RowKind::Protein)
                write_fasta_record(output, row.name, logical_protein(row));
        });
    }));
}
void TranslationUI::display(){w_->view_->set_translation(result_,enabled_->isChecked(),letters_->isChecked(),ruler_->isChecked());update_selection();}
void TranslationUI::refresh() {
    if (refreshing_) return;
    refreshing_ = true;
    set_available(!w_->busy_);
    const auto* document = w_->document_.get();
    const bool newDocument = document_ != document;
    const bool contentChanged = newDocument || content_ != document->content_id();
    const bool settingsChanged = newDocument || !(settings_ == document->translation_settings());
    if (contentChanged || settingsChanged) result_.reset();
    if (newDocument) {
        profiles_.reset();
        scan_.reset();
        proteinScan_.reset();
    }
    bool nucleotide = false;
    for (const auto& row : document->rows()) nucleotide |= row.kind == RowKind::Nucleotide;
    enabled_->setEnabled(nucleotide);
    if (!nucleotide) {
        QSignalBlocker block(enabled_);
        enabled_->setChecked(false);
        bar_->hide();
    }
    document_ = document;
    content_ = document->content_id();
    settings_ = document->translation_settings();
    if (settingsChanged) {
        fill_codes(code_, settings_);
        QSignalBlocker block(frame_);
        frame_->setCurrentIndex(static_cast<int>(settings_.frame) - 1);
    }
    if (letters_->isChecked() && w_->view_->mode() != AlignmentView::Mode::View) {
        QSignalBlocker block(letters_);
        letters_->setChecked(false);
    }
    if (contentChanged) {
        profiles_.reset();
        w_->view_->set_protein_profiles({});
        const bool protein = std::any_of(document->rows().begin(), document->rows().end(),
            [](const auto& row) { return row.kind == RowKind::Protein; });
        if (protein) proteinScan_ = std::make_unique<ProteinProfileScan>(*document);
        else proteinScan_.reset();
    }
    if (enabled_->isChecked() && (contentChanged || settingsChanged || !result_)) {
        if (!scan_ || contentChanged || settingsChanged) {
            scan_ = std::make_unique<TranslationScan>(*document, settings_);
            result_.reset();
            count_->setText("Translating…");
        }
    } else if (!enabled_->isChecked()) scan_.reset();
    if (scan_ || proteinScan_) timer_->start();
    display();
    refreshing_ = false;
}
void TranslationUI::run(){if(document_!=w_->document_.get()||content_!=w_->document_->content_id()||settings_!=w_->document_->translation_settings()){refresh();return;}QElapsedTimer time;time.start();try{do{if(proteinScan_){if(proteinScan_->step(*document_,2048)){profiles_=std::make_shared<ProteinProfiles>(proteinScan_->result());proteinScan_.reset();w_->view_->set_protein_profiles(profiles_);}}else if(scan_){if(scan_->step(*document_,2048)){result_=std::make_shared<TranslationResult>(scan_->result());scan_.reset();count_->setText(QString("Stops: %1%2").arg(result_->stops).arg((settings_.regions.empty()?QString{}:QString(" · Custom annotation"))+(selected_genetic_code(settings_).contextStops?QString(" · Context code"):QString{})));count_->setToolTip(summary(*result_));display();}}else{timer_->stop();break;}}while(time.elapsed()<4);}catch(const std::exception& e){timer_->stop();scan_.reset();proteinScan_.reset();count_->setText(QString::fromUtf8(e.what()));}}
void TranslationUI::annotate(int phase){if(w_->busy_)return;const auto& f=w_->view_->fragment();if(!f){QMessageBox::information(w_,"Coding columns","Select the alignment columns to annotate first. Sequence name selection does not affect coding positions.");return;}try{w_->document_->set_translation_settings(annotate_coding(w_->document_->translation_settings(),f->start,f->width,phase));w_->update_state();}catch(const std::exception& e){QMessageBox::warning(w_,"Coding columns",QString::fromUtf8(e.what()));}}
void TranslationUI::update_selection(){copy_->setEnabled(!w_->busy_&&w_->view_->can_copy_translation());generateMask_->setEnabled(!w_->busy_&&w_->document_->columns()!=0);annotateMask_->setEnabled(!w_->busy_&&w_->document_->columns()!=0);}
void TranslationUI::generate_mask(){
    if(w_->busy_||!w_->document_->columns())return;
    try{const auto text=translation_mask(w_->document_->translation_settings(),w_->document_->columns());
        w_->document_->append({{"translation_mask",text,RowKind::Mask}});w_->view_->refresh();w_->view_->select_ids({w_->document_->rows().back().id});w_->update_state();
    }catch(const std::exception& e){QMessageBox::warning(w_,"Generate translation mask",QString::fromUtf8(e.what()));}
}
void TranslationUI::annotate_mask_dialog(SequenceId mask){
    if(w_->busy_||!w_->document_->columns())return;
    auto apply=[this](std::string_view text){w_->document_->set_translation_settings(annotate_from_mask(w_->document_->translation_settings(),text,w_->document_->columns()));w_->update_state();};
    if(mask){try{const auto& row=w_->document_->sequence(mask);if(row.kind!=RowKind::Mask)return;apply(row.symbols.text());}catch(const std::exception& e){QMessageBox::warning(w_,"Annotate using this mask",QString::fromUtf8(e.what()));}return;}
    QDialog dialog(w_);dialog.setObjectName("annotationMaskDialog");dialog.setWindowTitle("Annotate from mask");dialog.resize(650,330);QVBoxLayout layout(&dialog);QFormLayout form;
    QComboBox source;source.setObjectName("annotationMaskSource");source.addItem("Paste a mask",QVariant::fromValue<qulonglong>(0));
    for(const auto& row:w_->document_->rows())if(row.kind==RowKind::Mask)source.addItem(QString::fromStdString(row.name),QVariant::fromValue<qulonglong>(row.id));
    const auto selected=w_->view_->selected_ids();for(auto id:selected){const auto index=source.findData(QVariant::fromValue<qulonglong>(id));if(index>0){source.setCurrentIndex(index);break;}}
    QPlainTextEdit text;text.setObjectName("annotationMaskText");text.setPlaceholderText("123123###123123");form.addRow("Source",&source);form.addRow("Mask",&text);layout.addLayout(&form);
    QLabel help("1, 2 and 3 mark codon positions; every other symbol marks a noncoding column. Short masks leave remaining columns noncoding. Masks longer than the alignment cannot be applied. Pasted line breaks are ignored; spaces count as noncoding columns. This replaces the current annotation and can be undone.");help.setWordWrap(true);layout.addWidget(&help);
    QLabel status;status.setObjectName("annotationMaskStatus");status.setWordWrap(true);layout.addWidget(&status);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText("Apply");layout.addWidget(&buttons);
    auto input=[&]{std::string symbols;for(auto c:text.toPlainText().toUcs4()){if(c=='\r'||c=='\n')continue;symbols+=c>='1'&&c<='3'?static_cast<char>(c):'#';}return symbols;};
    auto validate=[&]{try{const auto symbols=input();(void)annotate_from_mask(w_->document_->translation_settings(),symbols,w_->document_->columns());status.setText(QString("%1 mask columns / %2 alignment columns").arg(symbols.size()).arg(w_->document_->columns()));buttons.button(QDialogButtonBox::Ok)->setEnabled(!symbols.empty());}catch(const std::exception& e){status.setText(QString::fromUtf8(e.what()));buttons.button(QDialogButtonBox::Ok)->setEnabled(false);}};
    auto choose=[&]{const auto id=source.currentData().toULongLong();text.setReadOnly(id!=0);if(id)text.setPlainText(QString::fromStdString(w_->document_->sequence(id).symbols.text()));else text.clear();validate();};
    connect(&source,&QComboBox::currentIndexChanged,&dialog,choose);connect(&text,&QPlainTextEdit::textChanged,&dialog,validate);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);connect(&buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{apply(input());dialog.accept();}catch(const std::exception& e){status.setText(QString::fromUtf8(e.what()));}});choose();dialog.exec();
}
void TranslationUI::copy_fragment(){
    if(w_->busy_||!result_)return;
    auto expanded=w_->view_->expanded_translation_fragment();if(!expanded)return;
    w_->view_->select_fragment(*expanded);
    const auto& f=*expanded;const auto* row=result_->row(f.ids.front());if(!row)return;
    auto first=std::lower_bound(result_->map.codons.begin(),result_->map.codons.end(),f.start,[](const auto& span,auto column){return span.first()<column;});
    std::string text;for(auto i=first;i!=result_->map.codons.end()&&i->last()<f.start+f.width;++i)text+=row->calls[static_cast<std::size_t>(i-result_->map.codons.begin())].amino;
    if(!text.empty())QApplication::clipboard()->setText(QString::fromStdString(text));
}
QJsonObject TranslationUI::state()const{return {{"enabled",enabled_->isChecked()},{"letters",letters_->isChecked()},{"ruler",ruler_->isChecked()},{"rememberedMode",rememberedMode_}};}
void TranslationUI::restore(const QJsonObject& s){ruler_->setChecked(s["ruler"].toBool(true));enabled_->setChecked(s["enabled"].toBool());letters_->setChecked(s["letters"].toBool());rememberedMode_=std::clamp(s["rememberedMode"].toInt(2),0,2);refresh();}
void TranslationUI::custom_code(){
    if(w_->busy_)return;
    if(auto code=edit_custom_code(w_,w_->document_->translation_settings())){auto settings=w_->document_->translation_settings();settings.code=0;settings.custom=std::move(code);w_->document_->set_translation_settings(std::move(settings));w_->update_state();}
}
std::shared_ptr<const GeneticCode> TranslationUI::edit_custom_code(QWidget* parent,const TranslationSettings& settings){std::shared_ptr<const GeneticCode> result;QDialog dialog(parent);dialog.setObjectName("customGeneticCodeDialog");dialog.setWindowTitle("Custom genetic code");dialog.resize(620,320);QVBoxLayout layout(&dialog);QFormLayout form;QLineEdit name(&dialog);name.setObjectName("customGeneticCodeName");name.setText("Custom code");QPlainTextEdit amino(&dialog);amino.setPlainText(QString::fromStdString(selected_genetic_code(settings).aminoAcids));amino.setMaximumHeight(75);QLineEdit context(&dialog);form.addRow("Name",&name);form.addRow("64 amino acids",&amino);form.addRow("Context stop codons",&context);layout.addLayout(&form);QLabel help("Assignments use NCBI TCAG order: TTT, TTC, TTA, TTG, TCT … GGG. Use one-letter amino acids and * for stops. Optional context stop codons are comma-separated (for example TGA). Imported files may contain 64 amino-acid symbols, or 64 codon TAB amino-acid rows.",&dialog);help.setWordWrap(true);layout.addWidget(&help);QPushButton import("Import code…",&dialog);layout.addWidget(&import);connect(&import,&QPushButton::clicked,&dialog,[&]{const auto path=choose_open_file(&dialog,"Import genetic code");if(path.isEmpty())return;QFile f(path);if(!f.open(QIODevice::ReadOnly)){QMessageBox::warning(&dialog,"Import","Could not read the code file.");return;}const auto bytes=f.read(1024*1024);auto text=QString::fromUtf8(bytes);QString compact=text;compact.remove(QRegularExpression("\\s+"));if(compact.size()==64){amino.setPlainText(compact.toUpper());return;}QString assignments(64,QChar('?'));const QString bases="TCAG";for(const auto& line:text.split('\n')){const auto fields=line.simplified().split(' ');if(fields.size()!=2||fields[0].size()!=3||fields[1].size()!=1)continue;const auto codon=fields[0].toUpper();const auto a=bases.indexOf(codon[0]),b=bases.indexOf(codon[1]),c=bases.indexOf(codon[2]);if(a>=0&&b>=0&&c>=0)assignments[a*16+b*4+c]=fields[1][0].toUpper();}amino.setPlainText(assignments);});QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout.addWidget(&buttons);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);connect(&buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{auto c=std::make_shared<GeneticCode>();c->id=0;c->name=name.text().trimmed().toStdString();auto aas=amino.toPlainText().toUpper();aas.remove(QRegularExpression("\\s+"));c->aminoAcids=aas.toStdString();const QString bases="TCAG";for(auto word:context.text().toUpper().split(QRegularExpression("[,;\\s]+"),Qt::SkipEmptyParts)){word.replace('U','T');if(word.size()!=3||bases.indexOf(word[0])<0||bases.indexOf(word[1])<0||bases.indexOf(word[2])<0)throw std::invalid_argument("Context stops must be concrete three-base codons.");const auto at=bases.indexOf(word[0])*16+bases.indexOf(word[1])*4+bases.indexOf(word[2]);c->contextStops|=std::uint64_t(1)<<at;}validate_genetic_code(*c);result=c;dialog.accept();}catch(const std::exception& e){QMessageBox::warning(&dialog,"Genetic code",QString::fromUtf8(e.what()));}});dialog.exec();return result;}
void TranslationUI::add_dialog(bool compact){if(w_->busy_)return;auto ids=w_->view_->selected_ids();if(ids.empty()){if(!compact){QMessageBox::information(w_,"Translate sequences","Select nucleotide sequence names first.");return;}for(const auto& r:w_->document_->rows())if(r.kind==RowKind::Nucleotide)ids.push_back(r.id);}std::erase_if(ids,[&](auto id){return w_->document_->sequence(id).kind!=RowKind::Nucleotide;});if(ids.empty()){QMessageBox::information(w_,"Translate sequences","The selection contains no nucleotide sequences. Proteins and masks are excluded.");return;}
    QDialog dialog(w_);dialog.setObjectName("translationDialog");dialog.setWindowTitle(compact?"Translate to protein window":"Add translated sequences");dialog.resize(710,480);QVBoxLayout layout(&dialog);QFormLayout form;auto dialogSettings=w_->document_->translation_settings();QComboBox code(&dialog);code.setObjectName("translationDialogCode");fill_codes(&code,w_->document_->translation_settings());QComboBox frame(&dialog);frame.setObjectName("translationDialogFrame");frame.addItems({"1","2","3"});frame.setCurrentIndex(static_cast<int>(settings_.frame)-1);QCheckBox annotations("Use manual coding annotations",&dialog);annotations.setChecked(!settings_.regions.empty());annotations.setEnabled(!settings_.regions.empty());form.addRow("Genetic code",&code);form.addRow("Reading frame",&frame);form.addRow(&annotations);layout.addLayout(&form);QLabel description(compact?"A new window will contain compact protein sequences; nucleotide data stay in this window.":"Independent protein snapshots will be placed directly below their source sequences. Single-letter amino acids are centered in each codon, surrounded by ordinary editable gaps. Source groups are inherited; traces remain linked only to nucleotide sequences.",&dialog);description.setWordWrap(true);layout.addWidget(&description);QLabel stats(&dialog);stats.setObjectName("translationPreviewStats");stats.setWordWrap(true);layout.addWidget(&stats);QTableWidget table(static_cast<int>(ids.size()),4,&dialog);table.setHorizontalHeaderLabels({"Source sequence","Translated name","AA preview","Stops"});table.horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);table.horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);table.horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);table.setColumnWidth(3,55);table.setEditTriggers(QAbstractItemView::NoEditTriggers);for(std::size_t i=0;i<ids.size();++i)table.setItem(static_cast<int>(i),0,new QTableWidgetItem(QString::fromStdString(w_->document_->sequence(ids[i]).name)));layout.addWidget(&table);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);buttons.button(QDialogButtonBox::Ok)->setText(compact?"Open protein window":"Add translations");layout.addWidget(&buttons);std::unique_ptr<TranslationScan> preview;std::optional<TranslationResult> ready;QTimer timer(&dialog);timer.setInterval(0);const auto source=w_->document_;const auto content=source->content_id();
    std::function<void()> restart;restart=[&]{
        if(code.currentData().toInt()==-1){
            {QSignalBlocker blocker(code);code.setCurrentIndex(code.findData(dialogSettings.code));}
            QTimer::singleShot(0,&dialog,[&]{if(auto custom=edit_custom_code(&dialog,dialogSettings)){dialogSettings.custom=std::move(custom);dialogSettings.code=0;fill_codes(&code,dialogSettings);}restart();});return;
        }
        dialogSettings.code=code.currentData().toUInt();code.setToolTip(code.currentData(Qt::ToolTipRole).toString());ready.reset();buttons.button(QDialogButtonBox::Ok)->setEnabled(false);stats.setText("Calculating translation…");auto s=dialogSettings;s.code=code.currentData().toUInt();s.frame=static_cast<unsigned>(frame.currentIndex()+1);if(!annotations.isChecked())s.regions.clear();try{preview=std::make_unique<TranslationScan>(*source,s,ids);timer.start();}catch(const std::exception& e){stats.setText(QString::fromUtf8(e.what()));}};
    connect(&timer,&QTimer::timeout,&dialog,[&]{try{QElapsedTimer elapsed;elapsed.start();while(elapsed.elapsed()<4){if(preview->step(*source,2048)){ready=preview->result();preview.reset();timer.stop();stats.setText(summary(*ready));std::set<std::string> names;for(const auto& r:source->rows())names.insert(r.name);for(std::size_t i=0;i<ready->rows.size();++i){std::size_t stops=0;QString aas;const auto& row=ready->rows[i];for(auto c:row.calls)stops+=(c.flags&DefiniteStop)!=0;for(std::size_t j=0;j<std::min<std::size_t>(30,row.calls.size());++j)aas+=QChar::fromLatin1(row.calls[j].amino);if(row.calls.size()>30)aas+="…";auto base=source->sequence(row.id).name+"_TRANSLATED",name=base;for(unsigned n=2;names.contains(name);++n)name=base+"_"+std::to_string(n);names.insert(name);table.setItem(static_cast<int>(i),1,new QTableWidgetItem(QString::fromStdString(name)));table.setItem(static_cast<int>(i),2,new QTableWidgetItem(aas));table.setItem(static_cast<int>(i),3,new QTableWidgetItem(QString::number(stops)));}buttons.button(QDialogButtonBox::Ok)->setEnabled(!ready->map.phaseErrors&&!ready->map.codons.empty());break;}}}catch(const std::exception& e){timer.stop();stats.setText(QString::fromUtf8(e.what()));}});connect(&code,&QComboBox::currentIndexChanged,&dialog,restart);connect(&frame,&QComboBox::currentIndexChanged,&dialog,restart);connect(&annotations,&QCheckBox::toggled,&dialog,restart);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);connect(&buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{if(!ready||source!=w_->document_||content!=source->content_id())throw std::runtime_error("Alignment changed. Reopen the translation preview.");auto records=translated_records(*source,*ready,compact);if(compact){auto d=std::make_shared<Document>();std::vector<SequenceId> rowIds;for(std::size_t i=0;i<records.records.size();++i)rowIds.push_back(i+1);d->restore_session(std::move(records.records),rowIds,rowIds.size()+1,source->groups(),source->next_group_id());auto* window=new MainWindow;window->setAttribute(Qt::WA_DeleteOnClose);window->install_document(std::move(d),{},false);window->resultLabel_="Translated proteins";window->update_state();window->show();}else{auto added=source->insert_translations(std::move(records.records),records.sources);w_->view_->refresh();w_->view_->select_ids(added);w_->update_state();}dialog.accept();}catch(const std::exception& e){QMessageBox::warning(&dialog,"Translation",QString::fromUtf8(e.what()));}});restart();dialog.exec();
}
}
