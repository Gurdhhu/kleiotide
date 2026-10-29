#include "gui/menu_shortcuts.hpp"
#include "gui/file_output.hpp"
#include "core/version.hpp"
#include "gui/site_highlighter.hpp"
#include "gui/main_window.hpp"
#include "gui/app_settings.hpp"
#include "gui/settings_dialog.hpp"
#include "gui/recovery.hpp"
#include <QListWidget>
#include <QProgressDialog>
#include "gui/translation_ui.hpp"
#include "gui/alignment_view.hpp"
#include "gui/alignment_overview.hpp"
#include "gui/mafft_dialog.hpp"
#include "core/fasta.hpp"
#include "core/trace_linking.hpp"
#include "gui/trace_link_dialog.hpp"
#include "gui/blast_dialog.hpp"
#include "core/abif.hpp"
#include "core/session.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScrollBar>
#include <QSignalBlocker>
#include "gui/chromatogram_view.hpp"
#include "gui/rename_dialog.hpp"
#include "gui/groups_dialog.hpp"
#include "core/statistics.hpp"
#include "core/row_selection.hpp"
#include "gui/sequence_selection.hpp"
#include "gui/motif_navigator.hpp"
#include <QAction>
#include <QActionGroup>
#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QClipboard>
#include <QCheckBox>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QSpinBox>
#include <QLabel>
#include <QInputDialog>
#include <QFutureWatcher>
#include <QKeySequence>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <numeric>
#include <sstream>

namespace kleiotide {
namespace {
QIcon view_icon(int kind,ResiduePalette palette=ResiduePalette::Classic) {
    QIcon icon;
    for(int scale:{1,2,3}) {
        QPixmap image(20*scale,20*scale);image.setDevicePixelRatio(scale);image.fill(Qt::transparent);
        QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(QColor(70,80,95),1.1));
        if(kind==0) {
            auto font=QFontDatabase::systemFont(QFontDatabase::FixedFont);font.setPixelSize(7);font.setBold(true);p.setFont(font);
            const auto colors=palette_colors(palette);
            for(int i=0;i<4;++i){p.setPen(colors[i]);p.drawText(QRectF(i*5,1,5,10),Qt::AlignCenter,QString(QChar::fromLatin1("ACGT"[i])));
                p.setPen(Qt::NoPen);p.setBrush(QColor(65,70,75));p.drawEllipse(QPointF(2.5+i*5,15),1.1,1.1);}
        } else if(kind==1) {
            p.drawLine(QPointF(1,17),QPointF(19,17));
            const auto bases=palette_colors(palette);const QColor colors[]={bases[0],bases[1],bases[3]};
            for(int i=0;i<3;++i){QPainterPath path;path.moveTo(1,16);const double x=5+i*5;
                path.cubicTo(x-3,16,x-2,4+i*2,x,3+i*2);path.cubicTo(x+2,4+i*2,x+2,16,19,16);
                p.setPen(QPen(colors[i],1.4));p.drawPath(path);}
        } else if(kind==2) {
            p.drawRoundedRect(QRectF(1,2,18,16),2,2);p.setPen(Qt::NoPen);
            const int heights[]={6,10,7,12,9};for(int i=0;i<5;++i){p.setBrush(i==2?QColor(245,135,25):QColor(40,115,200));p.drawRect(QRectF(3+i*3,16-heights[i],2,heights[i]));}
        } else if(kind==6){
            auto f=QFontDatabase::systemFont(QFontDatabase::FixedFont);f.setPixelSize(8);f.setBold(true);p.setFont(f);
            const auto colors=palette_colors(palette);for(int i=0;i<3;++i){p.setPen(colors[i]);p.drawText(QRect(i*5,8,6,11),Qt::AlignCenter,QString(QChar::fromLatin1("ACG"[i])));}
            p.setPen(QPen(QColor(25,65,105),1.8));p.setBrush(QColor(230,245,255,100));p.drawEllipse(QRectF(3,1,10,10));p.drawLine(QPointF(12,10),QPointF(18,17));
        } else if(kind==3){auto font=QFontDatabase::systemFont(QFontDatabase::FixedFont);font.setPixelSize(14);font.setBold(true);p.setFont(font);p.setPen(Qt::black);p.drawText(QRect(0,0,20,20),Qt::AlignCenter,"Aa");
        } else {const auto colors=palette_colors(palette);for(int i=0;i<4;++i){const QRectF rect(1+(i%2)*9,1+(i/2)*9,8,8);p.fillRect(rect,colors[static_cast<std::size_t>(i)]);
                if(kind==4){auto font=QFontDatabase::systemFont(QFontDatabase::FixedFont);font.setPixelSize(7);font.setBold(true);p.setFont(font);p.setPen(black_or_white(colors[static_cast<std::size_t>(i)]));p.drawText(rect,Qt::AlignCenter,QString(QChar::fromLatin1("ACGT"[i])));}}}
        p.end();icon.addPixmap(image);
    }
    return icon;
}
std::filesystem::path native_path(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    const auto bytes = path.toUtf8();
    return std::filesystem::path(std::string(bytes.constData(), static_cast<std::size_t>(bytes.size())));
#endif
}
struct ReadResult {
    QStringList importNotes,sessionNotes;
    std::vector<Record> records;
    std::shared_ptr<Document> document;
    QString error;
    bool hasTrace=false,isSession=false;
    std::string viewState,report;
};

}
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), document_(std::make_shared<Document>()) {
    document_->load({},AppSettings::instance().values().translation());savedState_=document_->state_id();
    setWindowTitle("Kleiotide — alignment editor");
    resize(1180, 720);setAcceptDrops(true);
    view_ = new AlignmentView(this);
    view_->set_document(document_.get());
    highlighter_=new SiteHighlighter(view_,this);highlighter_->set_document(document_.get());
    setCentralWidget(view_);
    overview_=new AlignmentOverview(view_,this);
    auto* overviewDock=new QDockWidget("Alignment overview",this);
    overviewDock->setFeatures(QDockWidget::DockWidgetClosable);
    auto* titleBar=new QWidget(overviewDock);titleBar->setFixedHeight(0);overviewDock->setTitleBarWidget(titleBar);
    overviewDock->setWidget(overview_);addDockWidget(Qt::BottomDockWidgetArea,overviewDock);
    overview_->set_document(document_.get());
    traces_=new ChromatogramPanel(this);
    traceDock_=new QDockWidget("Chromatogram",this);traceDock_->setObjectName("chromatogramDock");
    traceDock_->setFeatures(QDockWidget::NoDockWidgetFeatures);
    auto* traceTitle=new QWidget(traceDock_);traceTitle->setFixedHeight(0);traceDock_->setTitleBarWidget(traceTitle);traceDock_->setWidget(traces_);
    addDockWidget(Qt::BottomDockWidgetArea,traceDock_);splitDockWidget(overviewDock,traceDock_,Qt::Vertical);traceDock_->hide();
    traces_->cell_requested=[this](SequenceId id,std::size_t column){if(!busy_)view_->navigate_to_cell(id,column);};
    traces_->range_requested=[this](SequenceId id,std::size_t start,std::size_t width){if(busy_)return;view_->select_trace_fragment(id,start,width);view_->navigate_to(start);};
    traces_->key_requested=[this](QKeyEvent* event){if(!busy_&&prepare_trace_edit())QApplication::sendEvent(view_,event);};
    view_->viewport_changed=[this]{overview_->update();const auto [begin,end]=view_->visible_column_range();traces_->follow_viewport(begin,end);};
    view_->changed = [this] { update_state(); };
    view_->selection_changed = [this] {
        if(displayedDocument_!=document_.get()||displayedState_!=document_->state_id()||displayedMode_!=static_cast<int>(view_->mode()))update_state();
        else update_selection_state();
    };
    view_->paste_requested = [this] { paste(false,true); };
    view_->copy_rows_requested=[this]{copy_selected(true);};view_->cut_rows_requested=[this]{cut_selected();};
    view_->copy_fragment_requested=[this]{copy_clipboard();};view_->cut_fragment_requested=[this]{cut_clipboard();};view_->paste_fragment_requested=[this]{paste_fragment();};
    view_->rename_requested = [this](SequenceId id) { rename_sequence(id); };
    view_->link_trace_requested=[this]{link_traces();};view_->delete_trace_requested=[this]{delete_traces();};view_->blast_requested=[this](bool protein){search_blast(protein);};
    view_->delete_rows_requested = [this] { delete_selected(); };
    view_->add_group_requested=[this]{group_action(1);};view_->remove_group_requested=[this]{group_action(2);};
    view_->message = [this](const QString& text) { statusBar()->showMessage(text, 5000); };
    motif_=new MotifNavigator(this);motifDock_=new QDockWidget("Find motif",this);motifDock_->setObjectName("motifDock");
    motifDock_->setFeatures(QDockWidget::DockWidgetClosable);motifDock_->setWidget(motif_);addDockWidget(Qt::TopDockWidgetArea,motifDock_);motifDock_->hide();
    motif_->searching_changed=[this](bool searching){set_busy(searching,searching?"Searching for motif…":QString{});};
    motif_->highlighted=[this](std::optional<MotifHit> hit){if(hit)view_->set_search_match(AlignmentView::Fragment{{hit->id},hit->match.start,hit->match.width});else view_->set_search_match({});};
    motif_->results_changed=[this](std::shared_ptr<const MotifIndex> results){view_->set_search_results(std::move(results));};
    motif_->select_requested=[this](MotifHit hit){view_->select_fragment({{hit.id},hit.match.start,hit.match.width});view_->setFocus();};
    connect(motifDock_,&QDockWidget::visibilityChanged,this,[this](bool visible){if(!visible)motif_->close_results();});
    auto* file = menuBar()->addMenu("&File");
    auto action = [&](QMenu* menu, const QString& text, const QKeySequence& shortcut, auto callback) {
        auto* a = menu->addAction(text);
        a->setShortcut(shortcut);a->setShortcutVisibleInContextMenu(true);
        connect(a, &QAction::triggered, this, callback);
        mutatingActions_.push_back(a);
        return a;
    };
    action(file, "&New", QKeySequence::New, [this] {
        auto document=std::make_shared<Document>();document->load({},AppSettings::instance().values().translation());
        install_document(std::move(document), {}, true);
    });
    auto* newWindow=file->addAction("New window");connect(newWindow,&QAction::triggered,this,[]{auto* window=new MainWindow;window->setAttribute(Qt::WA_DeleteOnClose);window->show();});
    action(file, "&Open…", QKeySequence::Open, [this] {
        const auto paths = QFileDialog::getOpenFileNames(this, "Open sequences, traces or session", {},
            "Sequences, traces and sessions (*.fasta *.fas *.fa *.fna *.faa *.txt *.ab1 *.abi *.kleio);;Kleiotide session (*.kleio);;Sanger chromatograms (*.ab1 *.abi);;FASTA (*.fasta *.fas *.fa *.fna *.faa *.txt);;All files (*)");
        if (!paths.isEmpty()) open_paths(paths);
    });
    action(file, "Add sequence from &file…", {}, [this] {
        const auto paths = QFileDialog::getOpenFileNames(this, "Add sequences, linked traces or session sequences", {},
            "Sequences, traces and sessions (*.fasta *.fas *.fa *.fna *.faa *.txt *.ab1 *.abi *.kleio);;Kleiotide session sequences (*.kleio);;Sanger chromatograms (*.ab1 *.abi);;FASTA (*.fasta *.fas *.fa *.fna *.faa *.txt);;All files (*)");
        if (!paths.isEmpty()) load_files(paths, true);
    });
    action(file, "Add from &clipboard", {}, [this] { paste(); });
    action(file, "New alignment from clipboard", {}, [this] { paste(true); });
    action(file, "Add &new sequence…", {}, [this] { add_new_sequence(); });
    auto* importType=file->addMenu("FASTA import type");importType->setToolTipsVisible(true);
    fastaImport_=static_cast<FastaImport>(AppSettings::instance().values().fastaImport);
    auto* importGroup=new QActionGroup(this);importGroup->setExclusive(true);
    const char* importLabels[]={"Nucleotides + masks (default)","Proteins + masks","Masks only"};
    const char* importIds[]={"fastaImportNucleotide","fastaImportProtein","fastaImportMask"};
    for(int i=0;i<3;++i){const auto type=static_cast<FastaImport>(i);auto* a=action(importType,importLabels[i],{},[this,type]{fastaImport_=type;});
        a->setObjectName(importIds[i]);a->setCheckable(true);a->setChecked(i==static_cast<int>(fastaImport_));importGroup->addAction(a);
        a->setToolTip("Applies to FASTA Open, Add and clipboard imports in this window. Alphabets overlap: ACGT can be either nucleotide or protein. Incompatible printable rows become masks. Saved sessions and AB1 files retain their declared types.");}
    file->addSeparator();
    action(file, "&Save", QKeySequence::Save, [this] { save(); });
    action(file, "Save &as…", QKeySequence::SaveAs, [this] { save(true); });
    action(file,"Export FASTA…",{},[this]{save(true,{},true);});
    auto* exportAbif=action(file,"Export edited trace (AB1)…",{},[this]{export_trace();});exportAbif->setObjectName("exportAbif");exportAbif->setToolTip("Export one trace-linked sequence and its signal as an edited AB1 copy, including inserted/substituted calls and the current orientation.");
    auto* exportBatch=action(file,"Batch export edited traces (AB1)…",{},[this]{export_traces();});exportBatch->setObjectName("exportAbifBatch");exportBatch->setToolTip("Export all traces or traces linked to selected sequence names into separate AB1 files.");
    file->addSeparator();
    action(file,"Recover session…",{},[this]{offer_recovery();})->setObjectName("recoverSession");
    action(file,"Write recovery checkpoint now (testing only)",{},[this]{write_recovery(true);})->setObjectName("checkpointNow");
    auto* recoveryStatus=file->addAction("Recovery status…");connect(recoveryStatus,&QAction::triggered,this,[this]{QMessageBox::information(this,"Recovery status",recoveryStatus_+"\n\n"+QDir::toNativeSeparators(RecoveryStore::default_directory()));});
    file->addSeparator();
    auto* settingsAction=action(file,"Settings…",{},[this]{
        show_settings(this,current_appearance(),document_->translation_settings(),[this](const AppPreferences& p){
            if(busy_)return;
            apply_appearance(p);update_state();
        });
    });settingsAction->setObjectName("appSettingsAction");
    file->addSeparator();
    auto* exit = file->addAction("E&xit"); connect(exit, &QAction::triggered, this, &QWidget::close);
    auto* edit = menuBar()->addMenu("&Edit");
    undoAction_ = action(edit, "&Undo", QKeySequence::Undo, [this] {
        const auto location=document_->undo_residue_location();
        document_->undo(); view_->refresh();
        if(location)view_->reveal_residues(*location);
        update_state();
    });
    redoAction_ = action(edit, "&Redo", QKeySequence::Redo, [this] {
        const auto location=document_->redo_residue_location();
        document_->redo(); view_->refresh();
        if(location)view_->reveal_residues(*location);
        update_state();
    });
    edit->addSeparator();
    auto* cutFragment=action(edit,"Cut fragment",QKeySequence::Cut,[this]{cut_clipboard();});cutFragment->setObjectName("cutFragment");
    auto* copyFragment=action(edit,"Copy fragment",QKeySequence::Copy,[this]{copy_clipboard();});copyFragment->setObjectName("copyFragment");
    auto* pasteFragment=action(edit,"Paste fragment",QKeySequence::Paste,[this]{paste_fragment();});pasteFragment->setObjectName("pasteFragment");
    edit->addSeparator();
    auto* cutSequences=action(edit,"Cut selected sequences",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_X),[this]{cut_selected();});cutSequences->setObjectName("cutSequences");
    auto* copySequences=action(edit,"Copy selected sequences",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_C),[this]{copy_selected(true);});copySequences->setObjectName("copySequences");
    auto* pasteSequences=action(edit,"Paste sequences",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_V),[this]{paste(false,true);});pasteSequences->setObjectName("pasteSequences");
    deleteAction_ = action(edit, "&Delete selected sequences", QKeySequence(Qt::CTRL | Qt::Key_Delete),
        [this] { delete_selected(); });
    action(edit, "Copy sequence titles", {}, [this] { copy_selected(false); });
    edit->addSeparator();
    auto* trimQuality=action(edit,"Trim low quality ends…",{},[this]{quality_edit(true);});trimQuality->setObjectName("trimQualityEnds");
    auto* maskQuality=action(edit,"Low-quality calls to N…",{},[this]{quality_edit(false);});maskQuality->setObjectName("lowQualityToN");
    edit->addSeparator();
    auto* findMotif=action(edit,"Find motif…",QKeySequence(Qt::CTRL|Qt::SHIFT|Qt::Key_F),[this]{motifDock_->show();motif_->focus_query();});findMotif->setObjectName("findMotif");
    auto* nextMotif=action(edit,"Find next motif",QKeySequence(Qt::Key_F3),[this]{motifDock_->show();motif_->next();});nextMotif->setObjectName("findNextMotif");
    auto* previousMotif=action(edit,"Find previous motif",QKeySequence(Qt::SHIFT|Qt::Key_F3),[this]{motifDock_->show();motif_->next(true);});previousMotif->setObjectName("findPreviousMotif");
    auto* rename = edit->addMenu("&Rename");
    action(rename, "Rename selected sequence…", {}, [this] { rename_one(); });
    action(rename, "Using old name / new name table…", {}, [this] { rename_from_table(); });
    rename->addSeparator();
    for(int i=0;i<static_cast<int>(RenameOperation::Table);++i){const auto op=static_cast<RenameOperation>(i);action(rename,rename_operation_label(op),{},[this,op]{batch_rename(op);});}
    auto* sequence = menuBar()->addMenu("&Sequence");
    auto* setType=action(sequence,"Set sequence type…",{},[this]{set_sequence_type();});setType->setObjectName("setSequenceType");
    sequence->addSeparator();
    action(sequence,"Link trace (AB1)…",{},[this]{link_traces();})->setObjectName("linkTrace");
    action(sequence,"Delete trace…",{},[this]{delete_traces();})->setObjectName("deleteTrace");sequence->addSeparator();
    auto* merge=action(sequence,"Merge aligned sequences…",{},[this]{merge_sequences(false);});merge->setObjectName("mergeSelectedSequences");merge->setToolTip("Merge two or more selected nucleotide rows at their current alignment columns, with IUPAC consensus and optional trace-quality resolution.");
    auto* overlapMerge=action(sequence,"Find overlap and merge…",{},[this]{merge_sequences(true);});overlapMerge->setObjectName("findOverlapAndMerge");overlapMerge->setToolTip("Find overlaps in the current orientation with an optional edit allowance, review partial contigs and append the chosen result.");
    auto* collapse=action(sequence,"Collapse identical sequences…",{},[this]{collapse_sequences();});collapse->setObjectName("collapseIdentical");collapse->setToolTip("Keep one representative per identical ungapped sequence, with combined titles or a count suffix. Preview retained groups/traces and Undo the entire operation once.");
    sequence->addSeparator();
    auto* groups=sequence->addMenu("Groups");
    const char* groupLabels[]={"Manage groups…","Add selected sequences to group…","Remove selected sequences from group","Import groups from table…"};
    const char* groupIds[]={"manageGroups","assignGroup","removeGroup","importGroups"};
    for(int i=0;i<4;++i){auto* a=action(groups,groupLabels[i],{},[this,i]{group_action(i);});a->setObjectName(groupIds[i]);}
    auto* identityGroups=action(groups,"Group by % identity…",{},[this]{group_by_identity();});identityGroups->setObjectName("groupByIdentity");
    auto* identicalGroups=action(groups,"Group identical sequences…",{},[this]{group_action(5);});identicalGroups->setObjectName("groupIdentical");
    auto sequence_transform = [this](Document::Transform operation) {
        const auto ids = view_->selected_ids();
        if (ids.empty()) { statusBar()->showMessage("Select sequence names first.", 5000); return; }
        try { document_->transform(ids, operation); view_->refresh(); update_state(); }
        catch(const std::bad_alloc&){QMessageBox::warning(this,"Sequence operation","Not enough memory to complete this edit. Try fewer sequences or a shorter alignment.");}
        catch (const std::exception& e) { QMessageBox::warning(this, "Sequence operation", QString::fromUtf8(e.what())); }
    };
    sequence->addSeparator();
    action(sequence, "Lock gaps (~ → -)", {}, [sequence_transform] { sequence_transform(Document::Transform::LockGaps); });
    action(sequence, "Unlock gaps (- → ~)", {}, [sequence_transform] { sequence_transform(Document::Transform::UnlockGaps); });
    auto* lockSelection=action(sequence,"Lock gaps in selection",{},[this]{view_->lock_fragment();});lockSelection->setObjectName("lockSelection");
    auto* unlockSelection=action(sequence,"Unlock gaps in selection",{},[this]{view_->unlock_fragment();});unlockSelection->setObjectName("unlockSelection");
    action(sequence, "Degap (remove open gaps)", {}, [sequence_transform] { sequence_transform(Document::Transform::Degap); });
    sequence->addSeparator();
    action(sequence, "Reverse complement", {}, [sequence_transform] { sequence_transform(Document::Transform::ReverseComplement); });
    action(sequence, "Uppercase", {}, [sequence_transform] { sequence_transform(Document::Transform::Uppercase); });
    action(sequence, "Lowercase", {}, [sequence_transform] { sequence_transform(Document::Transform::Lowercase); });
    auto* sort=sequence->addMenu("&Sort");sort->setToolTipsVisible(true);
    const std::pair<const char*,Document::Sort> sorts[]={
        {"By sequence type",Document::Sort::Type},{"By group",Document::Sort::Group},{"By title",Document::Sort::Title},{"By length (descending)",Document::Sort::LengthDescending},{"By length (ascending)",Document::Sort::LengthAscending},
        {"By true length (descending)",Document::Sort::TrueLengthDescending},{"By true length (ascending)",Document::Sort::TrueLengthAscending},
        {"By summed residue frequencies in selected columns",Document::Sort::ResidueFrequency},
        {"Reverse order",Document::Sort::Reverse}};
    for(const auto& [label,method]:sorts)action(sort,QString::fromLatin1(label),{},[this,method]{sort_sequences(method);});
    sequence->addSeparator();
    auto* similarity=action(sort,"By decreasing similarity…",{},[this]{show_distances(1,true);});similarity->setObjectName("sortSimilarity");similarity->setToolTip("N, ?, other ambiguous calls, gaps and missing cells are excluded from both difference and comparable-site counts. N–N is not a match. Rows without valid overlap go last (except the starting row).");
    auto* sequenceSelection = sequence->addMenu("&Select");
    auto* groupSelect=action(sequenceSelection,"Groups…",{},[this]{group_action(4);});groupSelect->setObjectName("selectGroups");
    auto* onePerGroup=action(sequenceSelection,"One sequence per group…",{},[this]{select_one_per_group();});onePerGroup->setObjectName("selectOnePerGroup");
    const char* typeLabels[]={"Nucleotide sequences","Protein sequences","Masks","Sequences with traces"};
    const char* typeIds[]={"selectNucleotide","selectProtein","selectMasks","selectTraces"};
    for(int i=0;i<4;++i){auto* a=action(sequenceSelection,typeLabels[i],{},[this,i]{std::vector<SequenceId> ids;for(const auto& row:document_->rows())if(i==3?bool(row.trace):row.kind==(i==0?RowKind::Nucleotide:i==1?RowKind::Protein:RowKind::Mask))ids.push_back(row.id);view_->select_ids(ids);});a->setObjectName(typeIds[i]);}
    sequenceSelection->addSeparator();
    action(sequenceSelection, "Select all", QKeySequence::SelectAll, [this] { view_->select_all(); });
    shortcut_hint(action(sequenceSelection, "Unselect all", {}, [this] { view_->unselect_all(); }),QKeySequence(Qt::Key_Escape));
    action(sequenceSelection, "Invert selection", {}, [this] { view_->invert_selection(); });
    sequenceSelection->addSeparator();
    action(sequenceSelection, "Names matching regular expression…", QKeySequence::Find, [this] { select_regex(false); });
    action(sequenceSelection, "Sequences matching regular expression…", {}, [this] { select_regex(true); });
    sequenceSelection->addSeparator();
    const std::pair<const char*,SelectionTool> selectors[]={
        {"Names starting with…",SelectionTool::TitlePrefix},{"Names containing…",SelectionTool::TitleContains},
        {"Names from exact list…",SelectionTool::TitleExact},{"Duplicate names…",SelectionTool::DuplicateNames},
        {"Duplicate sequences…",SelectionTool::DuplicateSequences},
        {"Sequences containing a motif…",SelectionTool::Motif},{"Random subset…",SelectionTool::Random}};
    for(const auto& [label,method]:selectors)action(sequenceSelection,QString::fromUtf8(label),{},[this,method]{select_by_rule(method);});
    auto* alignment = menuBar()->addMenu("&Alignment");

    auto* mafft=alignment->addMenu("MAFFT");
    action(mafft,"Align whole alignment…",{},[this]{align_mafft(false);});
    action(mafft,"Align selected sequences…",{},[this]{align_mafft_selected();});
    action(mafft,"Realign selected columns…",{},[this]{align_mafft(true);});
    action(mafft,"Align selected sequences to templates…",{},[this]{align_mafft_templates(false);});
    action(mafft,"Align selected fragments to templates…",{},[this]{align_mafft_templates(true);});
    auto* report=mafft->addAction("Last alignment report…");connect(report,&QAction::triggered,this,[this]{show_mafft_report();});
    action(alignment, "&Flush alignment", {}, [this] {
        try{document_->flush();}
        catch(const std::bad_alloc&){QMessageBox::warning(this,"Flush alignment","Not enough memory to complete this edit. The alignment was not changed.");return;}
        catch(const std::exception& e){QMessageBox::warning(this,"Flush alignment",QString::fromUtf8(e.what()));return;}
        view_->refresh();update_state();
    })->setObjectName("flushAlignment");
    auto* toolbar = addToolBar("Editing");
    toolbar->setObjectName("editingToolbar");
    toolbar->setIconSize(QSize(20,20));toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    action(alignment, "&Unalign (remove all gaps)", {}, [this] {
        std::vector<SequenceId> ids; for (const auto& row : document_->rows()) ids.push_back(row.id);
        try { document_->transform(ids, Document::Transform::Unalign); view_->refresh(); update_state(); }
        catch (const std::exception& e) { QMessageBox::warning(this, "Unalign", QString::fromUtf8(e.what())); }
    });
    action(alignment, "&Minimize alignment (remove all-gap columns)", {}, [this] {
        try { document_->minimize(); view_->refresh(); update_state(); }
        catch (const std::exception& e) { QMessageBox::warning(this, "Minimize", QString::fromUtf8(e.what())); }
    });
    auto* blast=menuBar()->addMenu("&BLAST");
    action(blast,"Nucleotide BLAST…",{},[this]{search_blast(false);})->setObjectName("nucleotideBlast");
    action(blast,"Protein BLAST…",{},[this]{search_blast(true);})->setObjectName("proteinBlast");
    auto* highlight=menuBar()->addMenu("&Highlight");highlighter_->add_menu(highlight,mutatingActions_);
    auto* statistics=menuBar()->addMenu("&Statistics");statistics->setObjectName("statisticsMenu");
    action(statistics,"Alignment statistics…",{},[this]{show_statistics();})->setObjectName("alignmentStatisticsAction");
    action(statistics,"Nucleotide composition…",{},[this]{show_composition();})->setObjectName("nucleotideComposition");
    statistics->addSeparator();
    const char* distanceLabels[]={"Pairwise distance matrix…","Overall mean distance…","Within-group mean distance…","Between-group mean distance…"};
    const char* distanceIds[]={"pairwiseDistances","overallDistance","withinGroupDistance","betweenGroupDistance"};
    for(int i=0;i<4;++i){auto* command=action(statistics,QString::fromUtf8(distanceLabels[i]),{},[this,i]{show_distances(i);});command->setObjectName(distanceIds[i]);}
    auto* view = menuBar()->addMenu("&View");
    motifDock_->toggleViewAction()->setText("Motif search");motifDock_->toggleViewAction()->setIcon(view_icon(6));
    auto* overviewToggle=overviewDock->toggleViewAction();overviewToggle->setObjectName("alignmentOverviewView");
    overviewToggle->setIcon(view_icon(2));overviewToggle->setToolTip("Show or hide the alignment overview: column conservation, non-gap height and navigation.");
    auto* traceToggle=traceDock_->toggleViewAction();traceToggle->setObjectName("chromatogramView");
    traceToggle->setIcon(view_icon(1));traceToggle->setToolTip("Show or hide the chromatogram for linked sequences. Open or add an AB1 file to enable this view.");
    auto* dots = view->addAction("Identity dots to top sequence");
    dots->setObjectName("identityDots");
    dots->setCheckable(true);
    dots->setIcon(view_icon(0));
    dots->setToolTip("Show case-insensitive exact residue matches to the current top row as dots when both rows have the same sequence type. Gaps, masks and the top row keep their symbols. Copying and saving retain actual letters.");
    connect(dots, &QAction::toggled, this, [this](bool enabled) { view_->set_identity_dots(enabled); });
    auto* display=view->addMenu("Residue display");auto* displayGroup=new QActionGroup(this);
    const char* displayLabels[]={"Colored residue letters","Monochrome residues","Colored residue backgrounds"};
    for(int i=0;i<3;++i){const auto style=static_cast<ResidueDisplay>(i);auto* option=display->addAction(QString::fromLatin1(displayLabels[i]));option->setObjectName("residueDisplay."+display_id(style));option->setCheckable(true);displayGroup->addAction(option);option->setChecked(i==0);
        option->setIcon(view_icon(i==1?3:i==2?4:5));option->setToolTip(i==1?"Toggle monochrome alignment symbols. Click again to restore colored letters.":i==2?"Toggle residue-colored cells with contrasting black/white symbols. Click again to restore colored letters.":"Show residue-colored letters on a white alignment background.");
        connect(option,&QAction::triggered,this,[this,style]{set_residue_display(style!=ResidueDisplay::Letters&&view_->residue_display()==style?ResidueDisplay::Letters:style);});}
    auto* paletteMenu=view->addMenu("Residue palette");auto* paletteGroup=new QActionGroup(this);
    for(int i=0;i<residue_palette_count;++i){const auto palette=static_cast<ResiduePalette>(i);auto* option=paletteMenu->addAction(palette_label(palette));option->setObjectName("residuePalette."+palette_id(palette));option->setCheckable(true);paletteGroup->addAction(option);option->setChecked(i==0);option->setIcon(view_icon(5,palette));
        option->setToolTip("Change alignment residue colors and trace-channel colors. High contrast, Okabe–Ito and Paul Tol text is darkened where needed for contrast on white; Kleio and Chromas-style keep their exact base colors.");connect(option,&QAction::triggered,this,[this,palette]{set_residue_palette(palette);});}
    auto* fontAction=action(view, "Alignment &font…", {}, [this] { choose_font(); });fontAction->setObjectName("alignmentFont");
    toolbar->setObjectName("editingToolbar");toolbar->setMovable(false);toolbar->setFloatable(false);
    auto makeSwitch=[&](const char* object,const QString& accessible){
        auto* frame=new QFrame(toolbar);frame->setObjectName(object);frame->setAccessibleName(accessible);
        frame->setStyleSheet("QFrame { background: #edf1f5; border: 1px solid #aab7c5; border-radius: 7px; } QToolButton { color: #354456; background: transparent; border: 1px solid transparent; border-radius: 4px; padding: 3px 10px; min-width: 30px; } QToolButton:checked { color: white; background: #285b8f; border-color: #285b8f; } QToolButton:!checked:hover { background: #dce5ef; } QToolButton:focus { border-color: #536b85; } QToolButton:disabled { color: #8a929a; } QToolButton:checked:disabled { color: #edf0f3; background: #8294a5; border-color: #8294a5; }");
        auto* layout=new QHBoxLayout(frame);layout->setContentsMargins(2,2,2,2);layout->setSpacing(0);
        auto* group=new QButtonGroup(frame);group->setExclusive(true);toolbar->addWidget(frame);
        return frame;
    };
    auto addSegment=[&](QFrame* frame,const char* object,const QString& text,const QString& tip){
        auto* button=new QToolButton(frame);button->setObjectName(object);button->setText(text);button->setCheckable(true);button->setFocusPolicy(Qt::StrongFocus);button->setAccessibleName(text+" mode");button->setToolTip(tip);frame->findChild<QButtonGroup*>()->addButton(button);frame->layout()->addWidget(button);return button;
    };
    auto addSpacing=[&](const char* object){auto* space=new QWidget(toolbar);space->setObjectName(object);space->setFixedWidth(12);toolbar->addWidget(space);};
    auto* modeSwitch=makeSwitch("editViewSwitch","Edit or View mode");
    auto* editToggle=addSegment(modeSwitch,"editToggle","Edit","Edit residues using Overwrite or Insert; drag selected residues/blocks to slide");
    auto* viewToggle=addSegment(modeSwitch,"viewToggle","View","View and select residues without editing them");editToggle->setChecked(true);
    addSpacing("editingModeSpacing");
    auto* editingSwitch=makeSwitch("rewriteInsertSwitch","Overwrite or Insert mode");
    auto* rewriteToggle=addSegment(editingSwitch,"rewriteToggle","Overwrite","Replace the selected single-row fragment. Arrows move the caret; Ctrl+Left/Right slides one column, Shift+Left/Right moves across adjacent open gaps.");
    auto* insertToggle=addSegment(editingSwitch,"insertToggle","Insert","Insert after one selected symbol. Drag selected residues/blocks to slide; Ctrl+Left/Right slides one column, Shift+Left/Right moves across adjacent open gaps.");rewriteToggle->setChecked(true);
    addSpacing("rightClickSpacing");
    auto* gapMode = new QComboBox(toolbar);gapMode->setObjectName("rightClickMode");
    gapMode->addItems({"Right click: menu", "Right click: insert gap", "Right click: delete open gap", "Right click: insert gaps in others", "Right click: delete open gaps in others"});
    gapMode->setToolTip("Gap actions are available in Edit only. Locked gaps require unlocking before deletion.");toolbar->addWidget(gapMode);
    connect(gapMode,&QComboBox::currentIndexChanged,this,[this](int index){gapMode_=index;view_->set_right_click(static_cast<AlignmentView::RightClick>(index));update_state();});
    auto* modes=edit->addMenu("Mode");modes->setObjectName("editModeMenu");
    auto modeAction=[&](QMenu* menu,QActionGroup* group,const QString& label,const char* id,auto callback){
        auto* a=action(menu,label,{},callback);a->setObjectName(id);a->setCheckable(true);group->addAction(a);return a;
    };
    auto* viewModes=new QActionGroup(this);viewModes->setExclusive(true);modes->addSection("View / Edit");
    auto* viewMode=modeAction(modes,viewModes,"View","mode.view",[this]{view_->set_mode(AlignmentView::Mode::View);view_->setFocus();});
    auto* editMode=modeAction(modes,viewModes,"Edit","mode.edit",[this]{view_->set_mode(static_cast<AlignmentView::Mode>(editMode_));view_->setFocus();});
    auto* editingModes=new QActionGroup(this);editingModes->setExclusive(true);modes->addSection("Overwrite / Insert");
    auto* rewriteMode=modeAction(modes,editingModes,"Overwrite","mode.rewrite",[this]{view_->set_mode(AlignmentView::Mode::Rewrite);view_->setFocus();});
    auto* insertMode=modeAction(modes,editingModes,"Insert","mode.insert",[this]{view_->set_mode(AlignmentView::Mode::Insert);view_->setFocus();});
    auto* rightMenu=modes->addMenu("Right-click");auto* rightModes=new QActionGroup(this);rightModes->setExclusive(true);
    const char* rightLabels[]={"Context menu","Insert gap","Delete open gap","Insert gaps in others","Delete open gaps in others"};
    for(int i=0;i<5;++i){auto* a=action(rightMenu,rightLabels[i],{},[this,i]{gapMode_=i;view_->set_right_click(static_cast<AlignmentView::RightClick>(i));update_state();view_->setFocus();});a->setObjectName("mode.right."+QString::number(i));a->setCheckable(true);rightModes->addAction(a);}
    for(const auto& pair:std::array<std::pair<QToolButton*,QAction*>,4>{{{viewToggle,viewMode},{editToggle,editMode},{rewriteToggle,rewriteMode},{insertToggle,insertMode}}}){
        pair.first->parentWidget()->findChild<QButtonGroup*>()->setExclusive(false);
        pair.second->setToolTip(pair.first->toolTip());pair.first->setDefaultAction(pair.second);
    }
    toolbar->addSeparator();
    for(auto* toggle:{dots,traceToggle,overviewToggle}) {
        toolbar->addAction(toggle);
        if(auto* button=qobject_cast<QToolButton*>(toolbar->widgetForAction(toggle)))button->setAccessibleName(toggle->text());
    }
    toolbar->addSeparator();
    for(const auto& id:{"monochrome","backgrounds"}){auto* option=findChild<QAction*>("residueDisplay."+QString::fromLatin1(id));toolbar->addAction(option);if(auto* button=qobject_cast<QToolButton*>(toolbar->widgetForAction(option)))button->setAccessibleName(option->text());}
    auto* paletteButton=new QToolButton(toolbar);paletteButton->setObjectName("residuePaletteButton");paletteButton->setAccessibleName("Residue palette");paletteButton->setIconSize(QSize(20,20));paletteButton->setMenu(paletteMenu);paletteButton->setPopupMode(QToolButton::InstantPopup);toolbar->addWidget(paletteButton);
    auto* stretch=new QWidget(toolbar);stretch->setObjectName("editingToolbarStretch");stretch->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);auto* stretchAction=toolbar->addWidget(stretch);stretchAction->setObjectName("editingToolbarStretchAction");
    auto* sizeToolbar=addToolBar("Alignment size");sizeToolbar->setObjectName("alignmentSizeToolbar");sizeToolbar->setMovable(false);sizeToolbar->setFloatable(false);sizeToolbar->toggleViewAction()->setVisible(false);sizeToolbar->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    auto* sizeLabel=new QLabel(sizeToolbar);sizeLabel->setObjectName("alignmentSize");sizeLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);sizeLabel->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);sizeLabel->setContentsMargins(8,0,8,0);sizeLabel->setTextFormat(Qt::PlainText);sizeLabel->setToolTip("Total sequences (including masks and proteins) and alignment columns. Independent of name and residue selections.");sizeToolbar->addWidget(sizeLabel);
    auto* toolbarMenu=view->addMenu("Toolbars");toolbarMenu->setObjectName("toolbarsMenu");
    toolbarMenu->addAction(motifDock_->toggleViewAction());toolbarMenu->addAction(overviewToggle);toolbarMenu->addAction(traceToggle);
    auto* editingVisibility=toolbar->toggleViewAction();editingVisibility->setText("Editing controls");editingVisibility->setObjectName("editingToolbarView");toolbarMenu->addAction(editingVisibility);
    action(view,"Restore default layout",{},[this,toolbar,sizeToolbar,overviewDock]{
        auto* translationBar=findChild<QToolBar*>("translationToolbar");const bool translationVisible=translationBar&&!translationBar->isHidden();
        if(translationBar){removeToolBarBreak(translationBar);removeToolBar(translationBar);}
        removeToolBar(toolbar);removeToolBar(sizeToolbar);
        addToolBar(Qt::TopToolBarArea,toolbar);addToolBar(Qt::TopToolBarArea,sizeToolbar);toolbar->show();
        if(translationBar){addToolBarBreak(Qt::TopToolBarArea);addToolBar(Qt::TopToolBarArea,translationBar);translationBar->setVisible(translationVisible);}
        addDockWidget(Qt::BottomDockWidgetArea,overviewDock);addDockWidget(Qt::BottomDockWidgetArea,traceDock_);splitDockWidget(overviewDock,traceDock_,Qt::Vertical);
        overviewDock->show();traceDock_->setVisible(traces_->has_traces());motifDock_->hide();update_toolbar_layout();
    })->setObjectName("restoreDefaultLayout");
    auto* help = menuBar()->addMenu("&Help");
    auto* about = help->addAction("About Kleiotide");
    about->setObjectName("aboutKleiotide");
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox box(this);box.setWindowTitle("About Kleiotide");box.setTextFormat(Qt::RichText);
        box.setText(QString("<b>Kleiotide %1</b><br>Alignment and chromatogram editor.<br><br>"
            "Copyright © 2026 Oleg Shchepin.<br>GNU GPL version 3 only; no warranty. See LICENSE.<br>"
            "Logo/icon designer: Elizaveta Shchepina (Storminka / Io Tikhaya).<br><br>"
            "<a href=\"https://github.com/Gurdhhu/kleiotide\">GitHub: Gurdhhu/kleiotide</a><br><br>"
            "<b>Suggested citation</b><br>Shchepin, O. (2026). Kleiotide: sequence alignment and chromatogram editor. "
            "Version %1. https://github.com/Gurdhhu/kleiotide<br>"
            "Cite the version used; a DOI can be added when an archived release is available.<br><br>"
            "Save .kleio sessions to retain metadata and original trace evidence. Use FASTA export for sequence text.")
            .arg(QString::fromLatin1(version)));box.setTextInteractionFlags(Qt::TextBrowserInteraction);for(auto* label:box.findChildren<QLabel*>())label->setOpenExternalLinks(true);box.setStandardButtons(QMessageBox::Ok);box.exec();
    });
    translation_=new TranslationUI(this);
    view->removeAction(fontAction);view->addSeparator();view->addAction(fontAction);
    sequence->removeAction(sequenceSelection->menuAction());sequence->removeAction(sort->menuAction());
    sequence->addSeparator();sequence->addMenu(sequenceSelection);sequence->addMenu(sort);
    apply_appearance(AppSettings::instance().values());apply_operational_preferences();update_state();
    connect(&AppSettings::instance(),&AppSettings::changed,this,[this]{apply_operational_preferences();});
    checkpointElapsed_.start();recoveryIdle_.start();recoveryAttempt_.start();
    recoveryTimer_=new QTimer(this);recoveryTimer_->setInterval(1000);
    connect(recoveryTimer_,&QTimer::timeout,this,[this]{
        const auto& p=AppSettings::instance().values();
        if(p.recoveryEnabled&&!busy_&&document_->state_id()!=savedState_&&checkpointElapsed_.elapsed()>=qint64(p.recoverySeconds)*1000&&
           (recoveryIdle_.elapsed()>=1500||checkpointElapsed_.elapsed()>=qint64(p.recoverySeconds)*2000))write_recovery();
    });recoveryTimer_->start();
}
MainWindow::~MainWindow()=default;
void MainWindow::retire_recovery(){
    if(recovery_){const auto error=recovery_->discard();if(!error.isEmpty())statusBar()->showMessage(error,20000);recovery_.reset();}
    checkpointElapsed_.start();recoveryAttempt_.start();recoveryStatus_="No recovery checkpoint has been written for this document.";
}
void MainWindow::clear_recovered_claim(){
    if(recoveryClaim_&&consumeRecovery_){const auto error=recoveryClaim_->discard();if(!error.isEmpty())statusBar()->showMessage(error,20000);}
    recoveryClaim_.reset();consumeRecovery_=false;
}
void MainWindow::write_recovery(bool forced){
    // A modal editor can still own a staged Apply callback outside the main
    // action list. Checkpoint only after that nested event loop has finished.
    if(QApplication::activeModalWidget()||(!forced&&QApplication::activePopupWidget()))return;
    const auto retryAfter=std::max<qint64>(15000,qint64(AppSettings::instance().values().recoverySeconds)*1000);
    if(busy_||(!forced&&recoveryAttempt_.isValid()&&recoveryAttempt_.elapsed()<retryAfter))return;
    if(document_->state_id()==savedState_){if(forced)statusBar()->showMessage("There are no unsaved document changes to checkpoint.",5000);return;}
    recoveryAttempt_.start();
    try{if(!recovery_)recovery_=std::make_shared<RecoveryStore>();}
    catch(const std::exception& e){recoveryStatus_="Recovery unavailable: "+QString::fromUtf8(e.what());statusBar()->showMessage(recoveryStatus_,20000);return;}
    const auto source=document_;const auto store=recovery_;const auto view=session_view();const auto report=mafftReport_.toUtf8().toStdString();
    const auto title=windowTitle();const auto path=path_;const auto quota=AppSettings::bytes(AppSettings::instance().values().recoveryQuotaMiB);
    const auto cancelled=std::make_shared<std::atomic_bool>(false);
    auto* progress=new QProgressDialog("Writing a recovery checkpoint. Editing in this window is temporarily paused.","Cancel",0,0,this);
    progress->setObjectName("recoveryProgress");progress->setWindowModality(Qt::WindowModal);progress->setMinimumDuration(500);
    progress->setAutoClose(false);progress->setAutoReset(false);QTimer::singleShot(500,progress,[progress]{progress->show();});
    connect(progress,&QProgressDialog::canceled,this,[cancelled]{*cancelled=true;});
    connect(this,&QObject::destroyed,[cancelled]{*cancelled=true;});
    set_busy(true,"Writing recovery checkpoint…");auto* watcher=new QFutureWatcher<QString>(this);
    connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,progress]{
        const auto error=watcher->result();watcher->deleteLater();progress->close();progress->deleteLater();
        if(error.isEmpty()){
            checkpointElapsed_.start();recoveryStatus_="Recovery checkpoint saved at "+QDateTime::currentDateTime().toString(Qt::ISODate)+". The document is still unsaved.";
            clear_recovered_claim();
        }else recoveryStatus_="Recovery checkpoint not completed: "+error;
        set_busy(false);statusBar()->showMessage(recoveryStatus_,20000);
    });
    watcher->setFuture(QtConcurrent::run([store,source,view,report,title,path,quota,cancelled]{
        try{return store->write(*source,view,report,title,path,quota,[cancelled]{return cancelled->load();});}
        catch(const std::exception& e){return QString::fromUtf8(e.what());}
    }));
}
void MainWindow::offer_recovery(bool startup){
    if(busy_)return;
    const auto entries=RecoveryStore::available();
    if(entries.empty()){if(!startup)QMessageBox::information(this,"Recover session","No available recovery checkpoints. Active windows keep their checkpoints locked.");return;}
    QDialog dialog(this);dialog.setObjectName("recoveryChooser");dialog.setWindowTitle("Recover session");dialog.resize(760,440);
    QVBoxLayout layout(&dialog);QLabel help("These checkpoints belong to windows that are no longer active. Recover opens unsaved work in a new window and retains the checkpoint until it is safely replaced or saved. Open copy leaves the old checkpoint available.");help.setWordWrap(true);layout.addWidget(&help);
    QListWidget list;list.setObjectName("recoveryEntries");for(const auto& entry:entries)list.addItem(entry.modified.toLocalTime().toString(Qt::ISODate)+" — "+entry.title);list.setCurrentRow(0);layout.addWidget(&list,1);
    QCheckBox previous("Use previous completed generation (if the newest cannot be opened)");previous.setObjectName("previousRecoveryGeneration");previous.setEnabled(!entries[0].previousPath.isEmpty());layout.addWidget(&previous);
    connect(&list,&QListWidget::currentRowChanged,&dialog,[&](int row){previous.setChecked(false);previous.setEnabled(row>=0&&!entries[std::size_t(row)].previousPath.isEmpty());});
    QLabel error;error.setWordWrap(true);error.setStyleSheet("color:#A03020");layout.addWidget(&error);
    QDialogButtonBox buttons;auto* recover=buttons.addButton("Recover",QDialogButtonBox::AcceptRole);recover->setObjectName("recoverSelected");auto* copy=buttons.addButton("Open copy",QDialogButtonBox::ActionRole);copy->setObjectName("openRecoveryCopy");auto* discard=buttons.addButton("Discard selected…",QDialogButtonBox::DestructiveRole);discard->setObjectName("discardRecovery");buttons.addButton(QDialogButtonBox::Cancel);layout.addWidget(&buttons);
    auto open=[&](bool consume){
        if(list.currentRow()<0)return;
        auto claim=std::make_unique<RecoveryClaim>(entries[std::size_t(list.currentRow())]);
        if(!claim->valid()){error.setText("That recovery is now in use by another window or process.");return;}
        const auto path=previous.isChecked()?claim->entry().previousPath:claim->entry().path;auto* window=new MainWindow;window->setAttribute(Qt::WA_DeleteOnClose);window->recoveryClaim_=std::move(claim);window->openingRecovery_=true;window->consumeRecovery_=consume;window->show();window->open_path(path);dialog.accept();
    };
    connect(recover,&QPushButton::clicked,&dialog,[&]{open(true);});connect(copy,&QPushButton::clicked,&dialog,[&]{open(false);});
    connect(discard,&QPushButton::clicked,&dialog,[&]{
        if(list.currentRow()<0)return;
        const auto index=list.currentRow();
        if(QMessageBox::question(&dialog,"Discard recovery?","Delete both checkpoint generations for the selected session? Original files are not changed.",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
        RecoveryClaim claim(entries[std::size_t(index)]);const auto failure=claim.discard();
        if(!failure.isEmpty())error.setText(failure);else{list.item(index)->setHidden(true);list.setCurrentRow(-1);error.setText("Selected recovery discarded.");}
    });
    connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();
}
AppPreferences MainWindow::current_appearance()const {
    auto p=AppSettings::instance().values();p.font=view_->font();p.palette=view_->residue_palette();p.display=view_->residue_display();p.identityDots=view_->identity_dots();p.viewOnly=view_->mode()==AlignmentView::Mode::View;p.insert=editMode_==1;p.rightClick=gapMode_;
    p.overviewVisible=!overview_->parentWidget()->isHidden();p.traceVisible=traces_->has_traces()?!traceDock_->isHidden():p.traceVisible;return p;
}
void MainWindow::apply_appearance(const AppPreferences& p){
    view_->setFont(p.font);set_residue_palette(p.palette);set_residue_display(p.display);findChild<QAction*>("identityDots")->setChecked(p.identityDots);
    editMode_=p.insert?1:2;gapMode_=p.rightClick;
    view_->set_mode(p.viewOnly?AlignmentView::Mode::View:p.insert?AlignmentView::Mode::Insert:AlignmentView::Mode::Rewrite);
    overview_->parentWidget()->setVisible(p.overviewVisible);traceDock_->setVisible(p.traceVisible&&traces_->has_traces());
}
void MainWindow::apply_operational_preferences(){
    if(busy_)return;
    const auto& p=AppSettings::instance().values();
    document_->set_bulk_edit_memory_limit(AppSettings::bytes(p.bulkMiB));document_->set_history_budget(AppSettings::bytes(p.undoMiB));
    overview_->set_limits(AppSettings::bytes(p.overviewMiB),p.scanDelayMs);
}
void MainWindow::set_residue_palette(ResiduePalette palette){
    view_->set_residue_palette(palette);traces_->set_residue_palette(palette);
    findChild<QAction*>("residuePalette."+palette_id(palette))->setChecked(true);
    findChild<QAction*>("identityDots")->setIcon(view_icon(0,palette));findChild<QAction*>("chromatogramView")->setIcon(view_icon(1,palette));
    findChild<QAction*>("residueDisplay.backgrounds")->setIcon(view_icon(4,palette));
    auto* button=findChild<QToolButton*>("residuePaletteButton");button->setIcon(view_icon(5,palette));button->setToolTip("Residue palette: "+palette_label(palette)+". Choose colors for alignment symbols/cells and trace channels.");
}
void MainWindow::set_residue_display(ResidueDisplay display){view_->set_residue_display(display);findChild<QAction*>("residueDisplay."+display_id(display))->setChecked(true);}
void MainWindow::update_state() {
    if(displayedDocument_!=document_.get()||displayedState_!=document_->state_id())recoveryIdle_.start();
    if(translation_)translation_->refresh();
    displayedMode_=static_cast<int>(view_->mode());
    displayedDocument_=document_.get();displayedState_=document_->state_id();
    const bool edit=view_->mode()!=AlignmentView::Mode::View;
    if(edit)editMode_=static_cast<int>(view_->mode());
    if(auto* editButton=findChild<QToolButton*>("editToggle"))if(auto* viewButton=findChild<QToolButton*>("viewToggle")){QSignalBlocker guardEdit(editButton),guardView(viewButton);(edit?editButton:viewButton)->setChecked(true);editButton->setEnabled(!busy_);viewButton->setEnabled(!busy_);}
    if(auto* rewrite=findChild<QToolButton*>("rewriteToggle"))if(auto* insert=findChild<QToolButton*>("insertToggle")){
        QSignalBlocker guardRewrite(rewrite),guardInsert(insert);(editMode_==1?insert:rewrite)->setChecked(true);
        if(auto* control=findChild<QWidget*>("rewriteInsertSwitch"))control->setEnabled(edit&&!busy_);
    }
    for(const auto& choice:std::array<std::pair<const char*,bool>,4>{{{"mode.view",!edit},{"mode.edit",edit},{"mode.rewrite",editMode_!=1},{"mode.insert",editMode_==1}}})
        if(auto* a=findChild<QAction*>(choice.first)){a->setChecked(choice.second);a->setEnabled(!busy_&&(edit||choice.first==std::string_view("mode.view")||choice.first==std::string_view("mode.edit")));}
    for(int i=0;i<5;++i)if(auto* a=findChild<QAction*>("mode.right."+QString::number(i))){a->setChecked(i==(edit?gapMode_:0));a->setVisible(edit||i==0);a->setEnabled(!busy_&&(edit||i==0));}
    if(auto* label=findChild<QLabel*>("alignmentSize")){
        label->setText(QString("%1 sequences · %2 columns").arg(static_cast<qulonglong>(document_->rows().size())).arg(static_cast<qulonglong>(document_->columns())));
        if(auto* bar=findChild<QToolBar*>("alignmentSizeToolbar"))bar->setMinimumWidth(label->sizeHint().width()+12);
    }
    if(auto* gap=findChild<QComboBox*>("rightClickMode")){QSignalBlocker guard(gap);
        if(!edit&&gap->count()!=1){gap->clear();gap->addItem("Right click: menu");}
        else if(edit&&gap->count()!=5){gap->clear();gap->addItems({"Right click: menu","Right click: insert gap","Right click: delete open gap","Right click: insert gaps in others","Right click: delete open gaps in others"});}
        gap->setCurrentIndex(edit?gapMode_:0);gap->setEnabled(edit&&!busy_);view_->set_right_click(edit?static_cast<AlignmentView::RightClick>(gapMode_):AlignmentView::RightClick::Menu);
    }
    update_toolbar_layout();
    if (overview_) overview_->set_document(document_.get());
    if(highlighter_)highlighter_->set_document(document_.get());
    if(traces_){
        traces_->set_document(document_.get());
        if(!traces_->has_traces())traceDock_->hide();
    }
    if(auto* toggle=findChild<QAction*>("chromatogramView"))toggle->setEnabled(traces_->has_traces());
    if(auto* exportAbif=findChild<QAction*>("exportAbif"))exportAbif->setEnabled(!busy_&&traces_->has_traces());
    if(auto* exportBatch=findChild<QAction*>("exportAbifBatch"))exportBatch->setEnabled(!busy_&&traces_->has_traces());
    const bool dirty = document_->state_id() != savedState_;
    const auto title = path_.isEmpty() ? (resultLabel_.isEmpty()?QString("Untitled"):resultLabel_) : QFileInfo(path_).fileName();
    setWindowTitle(title + (dirty ? " *" : "") + " — Kleiotide");
    update_selection_state();
}
void MainWindow::update_selection_state() {
    overview_->update();
    if(translation_)translation_->update_selection();
    const bool edit=view_->mode()!=AlignmentView::Mode::View;
    const auto selected=view_->selected_ids();
    if(motif_)motif_->set_document(document_,selected);
    if(traces_&&traces_->has_traces()){
        if(const auto& fragment=view_->fragment()){
            traces_->follow_selection(fragment->ids,fragment->start);
            const bool included=std::find(fragment->ids.begin(),fragment->ids.end(),traces_->canvas()->sequence_id())!=fragment->ids.end();
            const bool blue=fragment->ids.size()==1&&(view_->mode()==AlignmentView::Mode::Rewrite||(view_->mode()==AlignmentView::Mode::Insert&&view_->insertable_selection()));
            traces_->canvas()->set_selection(fragment->start,included?fragment->width:0,blue);
        }else{
            traces_->canvas()->set_selection(0,0,false);
            if(const auto caret=view_->caret())traces_->follow_selection({caret->first},caret->second);
            else traces_->follow_selection(selected,{});
        }
    }
    if (!busy_) {
        const auto& fragment=view_->fragment();const bool single=fragment&&fragment->ids.size()==1;
        const bool writable=view_->mode()==AlignmentView::Mode::Insert||view_->mode()==AlignmentView::Mode::Rewrite;
        const auto enable=[this](const char* id,bool enabled){if(auto* a=findChild<QAction*>(id))a->setEnabled(enabled);};
        const auto* fragmentRow=single?document_->find_sequence(fragment->ids.front()):nullptr;
        const bool copyable=fragmentRow&&fragment->start<fragmentRow->symbols.size();
        enable("copyFragment",copyable);
        enable("cutFragment",writable&&copyable&&view_->complete_selection());
        enable("pasteFragment",writable&&view_->complete_selection()&&view_->caret().has_value()&&(!fragment||(single&&(view_->mode()==AlignmentView::Mode::Rewrite||view_->insertable_selection()))));
        enable("copySequences",!selected.empty());enable("cutSequences",!selected.empty());enable("unlockSelection",edit&&fragment.has_value());enable("lockSelection",edit&&fragment.has_value());
        const bool canMerge=selected.size()>1&&std::all_of(selected.begin(),selected.end(),[this](SequenceId id){const auto* row=document_->find_sequence(id);return row&&row->kind==RowKind::Nucleotide;});
        enable("mergeSelectedSequences",canMerge);enable("findOverlapAndMerge",canMerge);
        undoAction_->setEnabled(document_->can_undo());
        redoAction_->setEnabled(document_->can_redo());
        deleteAction_->setEnabled(!selected.empty());
        if (fragment) {
            statusBar()->showMessage(QString("Selection: columns %1–%2 · %3 rows%4")
                .arg(static_cast<qulonglong>(fragment->start + 1))
                .arg(static_cast<qulonglong>(fragment->start + fragment->width))
                .arg(static_cast<qulonglong>(fragment->ids.size()))
                .arg(edit?" · Ctrl+Left/Right: one column; Shift+Left/Right: maximum; drag to slide":""));
        }else statusBar()->clearMessage();
    }
}
QMenu* MainWindow::createPopupMenu(){
    auto* menu=QMainWindow::createPopupMenu();
    if(menu)if(auto* bar=findChild<QToolBar*>("editingToolbar"))menu->removeAction(bar->toggleViewAction());
    return menu;
}
void MainWindow::update_toolbar_layout(){
    auto* controls=findChild<QToolBar*>("editingToolbar");auto* counts=findChild<QToolBar*>("alignmentSizeToolbar");auto* label=findChild<QLabel*>("alignmentSize");
    if(!controls||!counts||!label)return;
    // Natural toolbar widths stay independent of overflow and current visibility.
    // Drop the information display before allocating less space to the controls.
    const int required=controls->sizeHint().width()+label->sizeHint().width()+16;
    const bool showCounts=width()>=required;
    if(counts->isHidden()==showCounts)counts->setVisible(showCounts);
}
namespace {
QStringList dropped_sequences(const QMimeData* mime){
    if(!mime||!mime->hasUrls())return {};
    QStringList paths;const QStringList suffixes={"fasta","fas","fa","fna","ffn","faa","frn","fsa","mpfa","fnt","fsa_nt","fsa_aa","txt","ab1","abi","kleio"};
    for(const auto& url:mime->urls()){
        if(!url.isLocalFile())return {};
        const QFileInfo info(url.toLocalFile());if(!info.isFile()||!suffixes.contains(info.suffix().toLower()))return {};
        paths.append(info.absoluteFilePath());
    }
    return paths;
}
}
void MainWindow::dragEnterEvent(QDragEnterEvent* event){if(!busy_&&!dropped_sequences(event->mimeData()).isEmpty())event->acceptProposedAction();else event->ignore();}
void MainWindow::dropEvent(QDropEvent* event){const auto paths=dropped_sequences(event->mimeData());if(busy_||paths.isEmpty()){event->ignore();return;}event->acceptProposedAction();load_files(paths,true);}
void MainWindow::resizeEvent(QResizeEvent* event){QMainWindow::resizeEvent(event);update_toolbar_layout();}
void MainWindow::changeEvent(QEvent* event){
    QMainWindow::changeEvent(event);
    if(event->type()==QEvent::FontChange||event->type()==QEvent::StyleChange)QTimer::singleShot(0,this,[this]{update_toolbar_layout();});
}
void MainWindow::set_busy(bool busy, const QString& text) {
    busy_ = busy;
    view_->setEnabled(!busy);
    traces_->setEnabled(!busy);
    if(motif_)motif_->set_available(!busy);
    if(translation_)translation_->set_available(!busy);
    for (auto* a : mutatingActions_) a->setEnabled(!busy);
    for(const auto* id:{"editViewSwitch","rewriteInsertSwitch","rightClickMode"})if(auto* control=findChild<QWidget*>(id))control->setEnabled(!busy);
    if (busy) statusBar()->showMessage(text); else {apply_operational_preferences();update_state();}
}
bool MainWindow::maybe_save(std::function<void()> continuation) {
    if (busy_) return false;
    if (document_->state_id() == savedState_) return true;
    const auto choice = QMessageBox::question(this, "Unsaved changes",
        "Save the current alignment before continuing?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Save) {
        save(false, std::move(continuation));
        return false;
    }
    return choice == QMessageBox::Discard;
}
void MainWindow::open_path(const QString& path) {open_paths({path});}
void MainWindow::open_paths(const QStringList& paths) {
    if(paths.isEmpty()||busy_||!maybe_save([this,paths]{open_paths(paths);}))return;
    load_files(paths,false);
}
void MainWindow::install_document(std::shared_ptr<Document> document, const QString& path, bool saved) {
    if (busy_ || !maybe_save([this, document, path, saved] { install_document(document, path, saved); })) return;
    retire_recovery();clear_recovered_claim();
    document_ = std::move(document); path_ = path;nativePath_=false;resultLabel_.clear();mafftReport_.clear();
    savedState_ = saved ? document_->state_id() : 0;
    view_->set_document(document_.get());apply_appearance(AppSettings::instance().values());apply_operational_preferences();update_state();
    if(traces_->has_traces()&&AppSettings::instance().values().traceVisible){traceDock_->show();resizeDocks({traceDock_},{260},Qt::Vertical);}
}
void MainWindow::load_files(const QStringList& paths, bool append) {
    if (busy_ || paths.isEmpty()) return;
    set_busy(true, append ? "Reading sequences to add…" : "Opening alignment…");
    const auto cleanDefaults=AppSettings::instance().values();
    auto* watcher = new QFutureWatcher<std::shared_ptr<ReadResult>>(this);
    connect(watcher, &QFutureWatcher<std::shared_ptr<ReadResult>>::finished, this,
        [this, watcher, paths, append,cleanDefaults] {
        const auto result = watcher->result();
        watcher->deleteLater();
        if (!result->error.isEmpty()) {
            openingRecovery_=false;recoveryClaim_.reset();
            set_busy(false);
            QMessageBox::warning(this, "Could not read sequences", result->error);
            return;
        }
        try {
            if (append) {
                document_->append(std::move(result->records)); view_->refresh();
            } else {
                retire_recovery();if(!openingRecovery_)clear_recovered_claim();
                document_ = std::move(result->document);
                nativePath_=result->isSession;
                path_ = paths.size() == 1 && (result->isSession||!result->hasTrace) ? paths.front() : QString{};
                resultLabel_=paths.size()>1?QString("%1 imported files").arg(paths.size()):result->hasTrace?QFileInfo(paths.front()).fileName():QString{};mafftReport_=QString::fromUtf8(result->report.c_str());
                savedState_ = document_->state_id(); view_->set_document(document_.get());
                if(!result->isSession)apply_appearance(cleanDefaults);
            }
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Could not add sequences", QString::fromUtf8(e.what()));set_busy(false);return;
        }
        set_busy(false);
        if(result->hasTrace&&traces_->has_traces()&&(result->isSession||append||cleanDefaults.traceVisible)){
            if(append){for(auto it=document_->rows().rbegin();it!=document_->rows().rend();++it)if(it->trace){traces_->choose_sequence(it->id);view_->navigate_to_cell(it->id,0);break;}}
            traceDock_->show();resizeDocks({traceDock_},{260},Qt::Vertical);
        }
        if(result->isSession)restore_view(result->viewState);
        if(openingRecovery_){
            path_.clear();nativePath_=false;savedState_=0;resultLabel_="Recovered session";openingRecovery_=false;
            if(!consumeRecovery_)recoveryClaim_.reset();
            update_state();
        }
        if(!result->sessionNotes.isEmpty())QMessageBox::information(this,"Session sequence import",
            "Only sequences, masks and their linked traces were imported from these sessions. Other session metadata (groups, translation settings/annotations, protein snapshot provenance, appearance, selections and reports) was discarded. The current session settings are retained.\n\n"+result->sessionNotes.join("\n"));
        if(!result->importNotes.isEmpty())QMessageBox::information(this,"AB1 import notes",result->importNotes.join("\n\n"));
    });
    const auto importType=fastaImport_;
    watcher->setFuture(QtConcurrent::run([paths, append,importType,cleanDefaults] {
        auto result = std::make_shared<ReadResult>();
        try {
            for (const auto& path : paths) {
                std::ifstream input(native_path(path), std::ios::binary);
                if (!input) throw std::runtime_error("Could not open " + path.toUtf8().toStdString());
                char magic[8]{};input.read(magic,8);const auto bytes=input.gcount();const bool abif=bytes>=4&&std::string_view(magic,4)=="ABIF";const bool native=bytes==8&&std::string_view(magic,8)==session_magic;input.clear();input.seekg(0);
                if(native||QFileInfo(path).suffix().compare("kleio",Qt::CaseInsensitive)==0){
                    if(!append&&paths.size()!=1)throw std::runtime_error("Open session files individually, or use Add sequence from file to import their sequences and linked traces.");
                    if(append){
                        auto session=read_session(input);
                        for(const auto& row:session.document.rows()){
                            result->records.push_back({row.name,row.symbols.text(),row.kind,row.trace});
                            result->hasTrace=result->hasTrace||bool(row.trace);
                        }
                        result->sessionNotes.append(QFileInfo(path).fileName());continue;
                    }
                    auto session=read_session(input);result->document=std::make_shared<Document>(std::move(session.document));result->viewState=std::move(session.viewState);result->report=std::move(session.alignmentReport);result->isSession=true;
                    result->hasTrace=std::any_of(result->document->rows().begin(),result->document->rows().end(),[](const auto& row){return bool(row.trace);});continue;
                }
                std::vector<Record> records;
                try{
                    if(abif){std::string note;records.push_back(read_abif(input,QFileInfo(path).completeBaseName().toUtf8().toStdString(),path.toUtf8().toStdString(),&note));result->hasTrace=true;if(!note.empty())result->importNotes.push_back(QFileInfo(path).fileName()+":\n"+QString::fromUtf8(note.c_str()));}
                    else records=read_fasta(input,{},importType);
                }catch(const std::exception& e){throw std::runtime_error(path.toUtf8().toStdString()+": "+e.what());}
                result->records.insert(result->records.end(),
                    std::make_move_iterator(records.begin()), std::make_move_iterator(records.end()));
            }
            if (!append&&!result->isSession) {
                result->document = std::make_shared<Document>();
                result->document->load(std::move(result->records),cleanDefaults.translation());
            }
        } catch (const std::exception& e) { result->error = QString::fromUtf8(e.what()); }
        return result;
    }));
}
bool MainWindow::prepare_trace_edit(){
    const auto* canvas=traces_->canvas();if(!canvas->accepts_editing())return false;
    const auto id=canvas->sequence_id();const auto& fragment=view_->fragment();
    if(fragment&&std::find(fragment->ids.begin(),fragment->ids.end(),id)!=fragment->ids.end())return true;
    if(const auto caret=view_->caret();caret&&caret->first==id)return true;
    view_->navigate_to_cell(id,canvas->focused_column());return true;
}
void MainWindow::add_new_sequence() {
    if (busy_) return;
    QDialog dialog(this); dialog.setWindowTitle("Add sequence");dialog.setObjectName("addSequenceDialog");
    QFormLayout layout(&dialog);
    QLineEdit name(&dialog);name.setObjectName("newSequenceName");QPlainTextEdit symbols(&dialog);symbols.setObjectName("newSequenceSymbols");
    QComboBox kind(&dialog);kind.setObjectName("newSequenceType");kind.addItems({"Nucleotide","Protein (one-letter codes)","Mask"});
    kind.setCurrentIndex(fastaImport_==FastaImport::ProteinAndMasks?1:fastaImport_==FastaImport::Masks?2:0);
    layout.addRow("Name", &name);layout.addRow("Type",&kind);layout.addRow("Sequence", &symbols);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    try {
        auto raw = symbols.toPlainText().toUtf8().toStdString();
        std::string clean;
        for (char c : raw) if (c != ' ' && c != '\t' && c != '\n' && c != '\r') clean += c;
        const RowKind types[]={RowKind::Nucleotide,RowKind::Protein,RowKind::Mask};
        document_->append({{name.text().toUtf8().toStdString(), std::move(clean),types[kind.currentIndex()]}});
        view_->refresh(); update_state();
    } catch (const std::exception& e) { QMessageBox::warning(this, "Add sequence", QString::fromUtf8(e.what())); }
}
void MainWindow::link_traces(){
    if(busy_)return;
    const auto selected=view_->selected_ids();bool ok=false;
    const auto scope=QInputDialog::getItem(this,"Link trace (AB1)","Sequences to include",{"Selected nucleotide sequences","All nucleotide sequences without traces"},selected.empty()?1:0,false,&ok);if(!ok)return;
    std::vector<SequenceId> ids;for(const auto& row:document_->rows())if(row.kind==RowKind::Nucleotide&&(scope.startsWith("All")?!row.trace:std::find(selected.begin(),selected.end(),row.id)!=selected.end()))ids.push_back(row.id);
    if(ids.empty()){QMessageBox::information(this,"Link trace","No eligible nucleotide sequences in this scope.");return;}
    set_busy(true,"Linking traces…");const bool applied=link_trace_dialog(document_,ids,this);if(applied)view_->refresh();set_busy(false);update_state();
}
void MainWindow::delete_traces(){
    if(busy_)return;
    auto ids=view_->selected_ids();if(ids.empty()){QMessageBox::information(this,"Delete trace","Select sequence names first.");return;}
    std::vector<std::pair<SequenceId,std::shared_ptr<const TraceLink>>> links;for(auto id:ids)if(document_->sequence(id).trace)links.push_back({id,{}});if(links.empty())return;
    if(QMessageBox::question(this,"Delete trace links?","Remove the selected session trace associations? Sequence letters and original AB1 files are retained. Undo restores the links.",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
    try{apply_trace_links(*document_,links);view_->refresh();update_state();}catch(const std::exception& e){QMessageBox::warning(this,"Delete trace",e.what());}
}
void MainWindow::search_blast(bool protein){if(busy_)return;set_busy(true,"BLAST query/report window…");blast_dialog(document_,view_->selected_ids(),protein,this);set_busy(false);update_state();}
void MainWindow::set_sequence_type(){
    if(busy_)return;
    const auto ids=view_->selected_ids();
    if(ids.empty()){statusBar()->showMessage("Select sequence names first.",5000);return;}
    QDialog dialog(this);dialog.setWindowTitle("Set sequence type");dialog.setObjectName("sequenceTypeDialog");dialog.resize(580,240);QFormLayout layout(&dialog);
    QLabel note("Apply to selected sequence names. Symbols are preserved; no sequence is translated. Trace-linked rows must remain nucleotide. Protein rows use one-letter amino-acid codes and are excluded from nucleotide analyses. MAFFT accepts nucleotide or protein participants of one type at a time.",&dialog);note.setWordWrap(true);layout.addRow(&note);
    QComboBox kind(&dialog);kind.setObjectName("sequenceTypeChoice");kind.addItems({"Nucleotide","Protein (one-letter codes)","Mask"});
    const RowKind types[]={RowKind::Nucleotide,RowKind::Protein,RowKind::Mask};
    const auto initial=document_->sequence(ids.front()).kind;for(int i=0;i<3;++i)if(types[i]==initial)kind.setCurrentIndex(i);
    layout.addRow(QString("%1 selected rows →").arg(static_cast<qulonglong>(ids.size())),&kind);
    QLabel error(&dialog);error.setObjectName("sequenceTypeError");error.setWordWrap(true);layout.addRow(&error);
    QDialogButtonBox buttons(QDialogButtonBox::Apply|QDialogButtonBox::Cancel,&dialog);layout.addRow(&buttons);
    connect(&kind,&QComboBox::currentIndexChanged,&dialog,[&]{error.clear();});
    connect(buttons.button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,[&]{
        try{document_->set_kind(ids,types[kind.currentIndex()]);view_->refresh();update_state();dialog.accept();}
        catch(const std::exception& e){error.setText(QString::fromUtf8(e.what()));}
    });
    connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();
}
void MainWindow::delete_selected() {
    if (busy_) return;
    try{document_->erase_rows(view_->selected_ids()); view_->refresh(); update_state();}
    catch(const std::exception& e){QMessageBox::warning(this,"Delete selected sequences",QString::fromUtf8(e.what()));}
}
void MainWindow::align_mafft(bool columns){
    if(busy_)return;
    RowKind molecule;try{molecule=mafft_molecule(*document_);}catch(const std::exception& e){statusBar()->showMessage(QString::fromUtf8(e.what()),8000);return;}
    std::optional<Document::ColumnRange> range;
    if(columns){if(!view_->fragment()){statusBar()->showMessage("Select a column range first.",5000);return;}range=Document::ColumnRange{view_->fragment()->start,view_->fragment()->width};}
    const auto configuration=choose_mafft(this,range,{},false,molecule);if(!configuration)return;
    start_mafft(*configuration,range);
}
void MainWindow::align_mafft_selected(){
    if(busy_)return;
    const auto ids=view_->selected_ids();if(ids.empty()){statusBar()->showMessage("Select sequence names first.",5000);return;}
    RowKind molecule;try{molecule=mafft_molecule(*document_,ids);}catch(const std::exception& e){statusBar()->showMessage(QString::fromUtf8(e.what()),8000);return;}
    auto configuration=choose_mafft(this,{}, {},true,molecule);if(!configuration)return;
    configuration->selectedRows=ids;start_mafft(*configuration,{});
}
void MainWindow::align_mafft_templates(bool fragments){
    if(busy_)return;
    const auto ids=view_->selected_ids();
    if(ids.empty()){statusBar()->showMessage("Select target sequence names first.",5000);return;}
    RowKind molecule;try{molecule=mafft_molecule(*document_,ids);}catch(const std::exception& e){statusBar()->showMessage(QString::fromUtf8(e.what()),8000);return;}
    const auto request=choose_mafft_templates(this,*document_,ids,fragments);if(!request)return;
    const auto configuration=choose_mafft(this,{},request,false,molecule);if(configuration)start_mafft(*configuration,{});
}
void MainWindow::start_mafft(MafftConfiguration configuration,std::optional<Document::ColumnRange> range){
    const auto sourceTitle=path_.isEmpty()?(resultLabel_.isEmpty()?QString("Untitled"):resultLabel_):QFileInfo(path_).fileName();
    set_busy(true,"Preparing MAFFT alignment…");auto* progress=new MafftProgress(this);auto* run=new MafftRun(document_,std::move(configuration),range,this);
    progress->cancel_requested=[run]{run->cancel();};run->status=[progress,this](const QString& text){progress->set_status(text);statusBar()->showMessage(text);};run->log=[progress](const QString& text){progress->append_log(text);};
    run->finished=[this,run,progress,sourceTitle](std::shared_ptr<Document> result,const QString& report,const QString& error,bool cancelled){
        mafftReport_="Source: "+sourceTitle+"\n"+report;set_busy(false);progress->completed(error);
        if(result&&!cancelled&&error.isEmpty()){
            auto* window=new MainWindow;window->setAttribute(Qt::WA_DeleteOnClose);window->install_document(std::move(result),{},false);window->resultLabel_="MAFFT — "+sourceTitle;window->mafftReport_=mafftReport_;window->update_state();window->show();
            statusBar()->showMessage("MAFFT result opened in a new window.",5000);
        }else if(cancelled)statusBar()->showMessage("MAFFT cancelled; source unchanged.",5000);
        run->deleteLater();
    };progress->show();run->start();
}
void MainWindow::show_mafft_report(){
    if(mafftReport_.isEmpty()){statusBar()->showMessage("No MAFFT job has run in this window yet.",5000);return;}
    QDialog dialog(this);dialog.setWindowTitle("MAFFT alignment report");dialog.resize(820,540);QVBoxLayout layout(&dialog);QPlainTextEdit text(mafftReport_);text.setReadOnly(true);layout.addWidget(&text);QDialogButtonBox buttons(QDialogButtonBox::Close);layout.addWidget(&buttons);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();
}
void MainWindow::show_statistics() {
    if (busy_) return;
    auto cancelled=std::make_shared<std::atomic_bool>(false);
    auto* dialog=new QDialog(this); dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setObjectName("alignmentStatistics"); dialog->setWindowTitle("Alignment statistics"); dialog->resize(540,410);
    auto* layout=new QVBoxLayout(dialog);
    auto* status=new QLabel("Computing statistics for all nucleotide rows…",dialog);
    status->setObjectName("statisticsStatus"); status->setWordWrap(true); layout->addWidget(status);
    auto* form=new QFormLayout; layout->addLayout(form);
    const char* labels[]={"Total sequences","Unique sequences","Columns","Distinct patterns","Parsimony-informative sites","Singleton sites","Constant sites","Gap-only sites","N-only sites"};
    const char* descriptions[]={
        "All nucleotide rows, including duplicates and empty rows. Proteins and masks are excluded.",
        "Distinct aligned nucleotide state strings, ignoring titles and case. U = T; N, ? and gaps share IQ-TREE's missing state.",
        "Longest nucleotide row; shorter rows are padded with missing data. Protein/mask-only extensions are excluded.",
        "Distinct column state patterns across nucleotide rows, using the same normalization as unique sequences.",
        "At least two definite nucleotide states each occur at least twice. Partial ambiguities do not count as definite observations.",
        "Variable but not parsimony-informative, matching IQ-TREE's reported singleton count. Incompatible ambiguities can make a site variable.",
        "All calls share at least one possible nucleotide. Includes compatible ambiguities, N-only and gap-only columns, as in IQ-TREE.",
        "Every nucleotide row has a gap or missing trailing cell at this column.",
        "Only N or ? and gaps/trailing cells, with at least one N or ?. No definite or partial nucleotide information."
    };
    std::vector<QLabel*> values;
    for (int i=0;i<9;++i) {
        auto* value=new QLabel("…",dialog); value->setObjectName(QString("statisticsValue%1").arg(i));
        value->setTextInteractionFlags(Qt::TextSelectableByMouse); value->setToolTip(QString::fromUtf8(descriptions[i]));
        form->addRow(QString::fromUtf8(labels[i]),value);
        form->labelForField(value)->setToolTip(value->toolTip()); values.push_back(value);
    }
    auto* note=new QLabel("IQ-TREE nucleotide conventions. Proteins and masks excluded. Gap-only and N-only counts are subsets of constant sites.",dialog);
    note->setWordWrap(true); layout->addWidget(note);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close,dialog);
    auto* copy=buttons->addButton("Copy",QDialogButtonBox::ActionRole); copy->setObjectName("statisticsCopy"); copy->setEnabled(false);
    layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    connect(dialog,&QDialog::finished,this,[cancelled]{cancelled->store(true);});
    struct Result { AlignmentStatistics statistics; QString error; };
    auto* watcher=new QFutureWatcher<Result>(this); QPointer<QDialog> guarded=dialog;
    connect(watcher,&QFutureWatcher<Result>::finished,this,[this,watcher,guarded,status,values,copy,cancelled] {
        const auto result=watcher->result(); watcher->deleteLater(); set_busy(false);
        if (!guarded || cancelled->load()) return;
        if (!result.error.isEmpty()) { status->setText(result.error); return; }
        const auto& s=result.statistics;
        const std::size_t numbers[]={s.sequences,s.uniqueSequences,s.columns,s.patterns,s.informative,s.singleton,s.constant,s.gapOnly,s.nOnly};
        for (std::size_t i=0;i<values.size();++i) values[i]->setText(QString::number(static_cast<qulonglong>(numbers[i])));
        status->setText("All nucleotide rows at the time this window was opened.");
        const auto text=QString::fromStdString(statistics_text(s));
        connect(copy,&QPushButton::clicked,guarded.data(),[text]{QApplication::clipboard()->setText(text);}); copy->setEnabled(true);
    });
    set_busy(true,"Computing alignment statistics…"); dialog->show();
    watcher->setFuture(QtConcurrent::run([source=document_,cancelled] {
        Result result;
        try { result.statistics=alignment_statistics(*source,[cancelled]{return cancelled->load();}); }
        catch (const std::exception& e) { result.error=QString::fromUtf8(e.what()); }
        return result;
    }));
}
void MainWindow::sort_sequences(Document::Sort method){
    if(busy_)return;
    std::optional<Document::ColumnRange> columns;
    if(method==Document::Sort::ResidueFrequency){
        if(!view_->fragment()){statusBar()->showMessage("Select a column range first.",5000);return;}
        columns=Document::ColumnRange{view_->fragment()->start,view_->fragment()->width};
    }
    struct Result{std::vector<SequenceId> order;QString error;};
    const auto source=document_;set_busy(true,"Sorting sequences…");
    auto* watcher=new QFutureWatcher<std::shared_ptr<Result>>(this);
    connect(watcher,&QFutureWatcher<std::shared_ptr<Result>>::finished,this,[this,watcher]{
        const auto result=watcher->result();watcher->deleteLater();
        if(!result->error.isEmpty()){set_busy(false);QMessageBox::warning(this,"Sort sequences",result->error);return;}
        try{document_->reorder(result->order);view_->refresh();set_busy(false);}
        catch(const std::exception&e){set_busy(false);QMessageBox::warning(this,"Sort sequences",QString::fromUtf8(e.what()));}
    });
    watcher->setFuture(QtConcurrent::run([source,method,columns]{
        auto result=std::make_shared<Result>();try{
            if(method==Document::Sort::Title){
                std::vector<QString> keys;for(const auto& row:source->rows())keys.push_back(QString::fromUtf8(row.name.c_str()).toCaseFolded());
                std::vector<std::size_t> positions(keys.size());std::iota(positions.begin(),positions.end(),0);
                std::stable_sort(positions.begin(),positions.end(),[&](auto a,auto b){return keys[a]<keys[b];});
                for(auto at:positions)result->order.push_back(source->rows()[at].id);
            }else result->order=source->sorted_order(method,columns);
        }catch(const std::exception&e){result->error=QString::fromUtf8(e.what());}return result;
    }));
}
void MainWindow::rename_from_table() { batch_rename(RenameOperation::Table); }
void MainWindow::batch_rename(RenameOperation operation) {
    if(busy_)return;
    const auto patches=show_batch_rename(*document_,view_->selected_ids(),QFileInfo(path_).completeBaseName(),operation,this);
    if(!patches)return;
    try { document_->rename(*patches);view_->refresh();update_state();
        statusBar()->showMessage(QString("Renamed %1 sequences").arg(static_cast<qulonglong>(patches->size())),5000);
    }catch(const std::exception& e){QMessageBox::warning(this,"Batch rename",QString::fromUtf8(e.what()));}
}
void MainWindow::rename_one() {
    if (busy_) return;
    const auto ids = view_->selected_ids();
    if (ids.size() != 1) {
        QMessageBox::information(this, "Rename sequence", "Select one sequence name first."); return;
    }
    rename_sequence(ids.front());
}
void MainWindow::rename_sequence(SequenceId id) {
    if (busy_) return;
    QInputDialog dialog(this);dialog.setWindowTitle("Rename sequence");dialog.setLabelText("New name");
    dialog.setTextValue(QString::fromUtf8(document_->sequence(id).name.c_str()));
    dialog.setMinimumWidth(720);dialog.resize(720,dialog.sizeHint().height());
    if (dialog.exec()!=QDialog::Accepted) return;
    const auto name=dialog.textValue();
    try {
        document_->rename({{id, name.toUtf8().toStdString()}});
        view_->refresh(); update_state();
    } catch (const std::exception& e) { QMessageBox::warning(this, "Rename sequence", QString::fromUtf8(e.what())); }
}
void MainWindow::copy_clipboard() {copy_fragment_clipboard(false);}
void MainWindow::choose_font() {
    QDialog dialog(this); dialog.setWindowTitle("Alignment font");
    QFormLayout layout(&dialog);
    QFontComboBox family(&dialog); family.setObjectName("alignmentFontFamily");
    family.setFontFilters(QFontComboBox::MonospacedFonts); family.setCurrentFont(view_->font());
    QSpinBox size(&dialog); size.setObjectName("alignmentFontSize"); size.setRange(6,96);
    size.setValue(std::max(6, view_->font().pointSize()));
    QComboBox style(&dialog); style.setObjectName("alignmentFontStyle");
    style.addItems({"Regular", "Bold", "Italic", "Bold Italic"});
    style.setCurrentIndex((view_->font().bold() ? 1 : 0) + (view_->font().italic() ? 2 : 0));
    QLabel preview("ACGTN acgtn ~-", &dialog); preview.setMinimumHeight(60); preview.setWordWrap(true);
    auto chosen = [&] {
        auto font = family.currentFont(); font.setPointSize(size.value());
        font.setBold(style.currentIndex() % 2 == 1); font.setItalic(style.currentIndex() >= 2);
        font.setUnderline(false); font.setStrikeOut(false); return font;
    };
    auto update = [&] { preview.setFont(chosen()); };
    connect(&family, &QFontComboBox::currentFontChanged, &dialog, update);
    connect(&size, &QSpinBox::valueChanged, &dialog, update);
    connect(&style, &QComboBox::currentIndexChanged, &dialog, update); update();
    layout.addRow("Font", &family); layout.addRow("Size", &size); layout.addRow("Style", &style); layout.addRow("Preview", &preview);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(520,240);
    if (dialog.exec() == QDialog::Accepted) view_->setFont(chosen());
}
void MainWindow::select_regex(bool sequences) {
    if (busy_) return;
    bool accepted = false;
    const auto pattern = QInputDialog::getText(this, "Select by regular expression",
        sequences ? "Pattern in aligned sequences (gaps included; case-sensitive)" : "Pattern in full sequence names (case-sensitive)",
        QLineEdit::Normal, {}, &accepted);
    if (!accepted) return;
    const QRegularExpression expression(pattern);
    if (!expression.isValid()) {
        QMessageBox::warning(this, "Invalid regular expression", expression.errorString()); return;
    }
    run_row_selection([expression,sequences](const Document& document) {
        std::vector<SequenceId> ids;
        for (const auto& row : document.rows()) {
            const auto& source = sequences ? row.symbols.text() : row.name;
            if (expression.match(QString::fromUtf8(source.data(), static_cast<qsizetype>(source.size()))).hasMatch())
                ids.push_back(row.id);
        }
        return ids;
    });
}
void MainWindow::run_row_selection(std::function<std::vector<SequenceId>(const Document&)> select) {
    if(busy_)return;
    struct Result {std::vector<SequenceId> ids;QString error;};
    const auto snapshot=document_;set_busy(true,"Selecting sequences…");
    auto* watcher=new QFutureWatcher<Result>(this);
    connect(watcher,&QFutureWatcher<Result>::finished,this,[this,watcher]{
        const auto result=watcher->result();watcher->deleteLater();
        if(result.error.isEmpty())view_->select_ids(result.ids);
        set_busy(false);
        if(!result.error.isEmpty())QMessageBox::warning(this,"Sequence selection",result.error);
        else statusBar()->showMessage(QString("Selected %1 sequences").arg(static_cast<qulonglong>(result.ids.size())),5000);
    });
    watcher->setFuture(QtConcurrent::run([snapshot,select=std::move(select)]{Result result;try{result.ids=select(*snapshot);}catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;}));
}
void MainWindow::select_by_rule(SelectionTool tool) {
    if(busy_)return;
    if(tool==SelectionTool::Random){
        if(document_->rows().empty()){statusBar()->showMessage("There are no sequences to select.",5000);return;}
        bool accepted=false;const int maximum=static_cast<int>(std::min<std::size_t>(document_->rows().size(),std::numeric_limits<int>::max()));
        const int count=QInputDialog::getInt(this,"Select random subset","Number of sequences (sampled without replacement from all rows)",std::min(10,maximum),0,maximum,1,&accepted);
        if(!accepted)return;
        const auto seed=QRandomGenerator::global()->generate64();
        run_row_selection([count,seed](const Document& d){return select_random_rows(d,static_cast<std::size_t>(count),seed);});return;
    }
    QDialog dialog(this);dialog.setObjectName("sequenceSelectionDialog");QFormLayout layout(&dialog);
    auto* description=new QLabel("Search all rows and replace the selected sequence names. Residue selection stays unchanged.",&dialog);description->setWordWrap(true);layout.addRow(description);
    QComboBox method;method.setObjectName("selectionMethod");
    QPlainTextEdit list;list.setObjectName("selectionPatterns");list.setPlaceholderText("One title or title fragment per line. Blank lines are ignored.");
    QLineEdit motif;motif.setObjectName("selectionMotif");
    QCheckBox matchCase("Match case");matchCase.setObjectName("selectionMatchCase");matchCase.setChecked(true);
    QCheckBox additional("Select only additional copies (leave the first unselected)");additional.setObjectName("selectionAdditionalOnly");
    QCheckBox ignoreGaps("Ignore gaps");ignoreGaps.setObjectName("selectionIgnoreGaps");ignoreGaps.setChecked(true);
    QCheckBox reverse("Also match the reverse complement");reverse.setObjectName("selectionReverseComplement");
    QCheckBox compatible("Match compatible IUPAC ambiguity codes");compatible.setObjectName("selectionCompatibleIupac");
    const bool titles=tool==SelectionTool::TitlePrefix||tool==SelectionTool::TitleContains||tool==SelectionTool::TitleExact;
    if(titles){dialog.setWindowTitle("Select by title list");method.addItems({"Exact titles","Titles starting with","Titles containing"});method.setCurrentIndex(tool==SelectionTool::TitleExact?0:tool==SelectionTool::TitlePrefix?1:2);layout.addRow("Match",&method);layout.addRow("Titles / fragments",&list);layout.addRow(&matchCase);}
    else if(tool==SelectionTool::DuplicateNames){dialog.setWindowTitle("Select duplicate names");method.addItems({"Complete title","Title before first '('","Title before first '|'"});layout.addRow("Compare",&method);layout.addRow(&matchCase);layout.addRow(&additional);}
    else if(tool==SelectionTool::DuplicateSequences){dialog.setWindowTitle("Select duplicate sequences");description->setText("Compare sequences without regard to letter case. Each sequence type is compared separately. Residue selection stays unchanged.");layout.addRow(&ignoreGaps);layout.addRow(&additional);}
    else {dialog.setWindowTitle("Select sequences containing a motif");description->setText("Search nucleotide rows without regard to letter case; proteins and masks are excluded. Exact IUPAC codes match literally unless compatibility is enabled. Residue selection stays unchanged.");layout.addRow("Nucleotide motif",&motif);layout.addRow(&ignoreGaps);layout.addRow(&reverse);layout.addRow(&compatible);
        ignoreGaps.setToolTip("Ignore open and locked gaps in both the motif and sequences. Turn off to match exact aligned gap positions and types.");compatible.setToolTip("A match is possible when the two symbols share at least one nucleotide. N and ? can match any nucleotide; T and U are compatible. Gaps still match their exact type when included.");}
    QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout.addRow(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.resize(660,titles?380:260);
    while(dialog.exec()==QDialog::Accepted){
        if(titles){auto patterns=list.toPlainText().split('\n');if(std::none_of(patterns.begin(),patterns.end(),[](const QString& s){return !s.trimmed().isEmpty();})){QMessageBox::warning(this,"Title selection","Enter at least one title or fragment.");continue;}
            const auto type=static_cast<TitleMatch>(method.currentIndex());const auto caseSensitive=matchCase.isChecked();run_row_selection([patterns,type,caseSensitive](const Document& d){return select_titles(d,patterns,type,caseSensitive);});}
        else if(tool==SelectionTool::DuplicateNames){const auto type=static_cast<DuplicateTitle>(method.currentIndex());const auto caseSensitive=matchCase.isChecked(),extra=additional.isChecked();run_row_selection([type,caseSensitive,extra](const Document& d){return select_duplicate_titles(d,type,caseSensitive,extra);});}
        else if(tool==SelectionTool::DuplicateSequences){const auto gaps=ignoreGaps.isChecked(),extra=additional.isChecked();run_row_selection([gaps,extra](const Document& d){return select_duplicate_sequences(d,gaps,extra);});}
        else {const auto text=motif.text().toUtf8().toStdString();const MotifOptions options{ignoreGaps.isChecked(),reverse.isChecked(),compatible.isChecked()};
            try{(void)select_motif(Document{},text,options);}catch(const std::exception& e){QMessageBox::warning(this,"Invalid motif",QString::fromUtf8(e.what()));continue;}
            run_row_selection([text,options](const Document& d){return select_motif(d,text,options);});}
        return;
    }
}
void MainWindow::save(bool saveAs, std::function<void()> onSuccess, bool exportFasta) {
    if (busy_) return;
    bool native = !exportFasta && (nativePath_ || requires_session(*document_));
    QString destination = saveAs || exportFasta || (native && !nativePath_) ? QString{} : path_;
    const bool chooseDestination = destination.isEmpty();
    if (chooseDestination) {
        QString filter;
        const auto filters = native ? QString("Kleiotide session (*.kleio)") : exportFasta ?
            QString("FASTA (*.fasta *.fas *.fa *.fna)") :
            QString("FASTA (*.fasta *.fas *.fa *.fna);;Kleiotide session (*.kleio)");
        destination = choose_output_filename(this,
            native ? "Save editing session" : "Save FASTA (sequence text only)", {}, filters, &filter,
            QFileDialog::DontConfirmOverwrite);
        if (destination.isEmpty()) return;
        native = !exportFasta && (native || filter.startsWith("Kleiotide") ||
            QFileInfo(destination).suffix().compare("kleio", Qt::CaseInsensitive) == 0);
    }
    destination = approve_output_path(this, destination,
        native ? OutputFormat::Session : OutputFormat::Fasta, document_.get(), chooseDestination);
    if (destination.isEmpty()) return;
    const auto viewState = native ? session_view() : std::string{};
    const auto report = mafftReport_.toUtf8().toStdString();
    set_busy(true, native ? "Saving editing session…" : "Exporting sequence text…");
    auto* watcher = new QFutureWatcher<QString>(this);
    const auto source = document_;
    const auto state = source->state_id();
    connect(watcher, &QFutureWatcher<QString>::finished, this,
        [this, watcher, destination, state, native, exportFasta, onSuccess = std::move(onSuccess)] {
        const auto error = watcher->result();
        watcher->deleteLater();
        if (error.isEmpty() && !exportFasta) {
            path_ = destination;
            nativePath_ = native;
            savedState_ = state;
            retire_recovery();clear_recovered_claim();
        }
        set_busy(false);
        if (!error.isEmpty()) QMessageBox::warning(this, "Could not save file", error);
        else if (onSuccess) onSuccess();
    });
    watcher->setFuture(QtConcurrent::run([source, destination, native, viewState, report] {
        return write_output_atomic(destination, [&](std::ostream& output) {
            if (native) write_session(output, *source, viewState, report);
            else write_fasta(output, *source);
        });
    }));
}
std::string MainWindow::session_view()const {
    QJsonObject state;state["version"]=1;state["mode"]=static_cast<int>(view_->mode());state["editMode"]=editMode_;state["rightClick"]=gapMode_;state["names"]=view_->name_area_width();state["font"]=view_->font().toString();
    state["identityDots"]=view_->identity_dots();
    if(translation_)state["translation"]=translation_->state();
    state["residuePalette"]=palette_id(view_->residue_palette());state["residueDisplay"]=display_id(view_->residue_display());
    state["column"]=view_->horizontalScrollBar()->value();state["row"]=view_->verticalScrollBar()->value();state["traceVisible"]=!traceDock_->isHidden();
    if(auto* dock=findChild<QWidget*>("alignmentOverview"))state["overviewVisible"]=!dock->parentWidget()->isHidden();
    state["trace"]=QString::number(traces_->canvas()->sequence_id());state["traceColumn"]=QString::number(static_cast<qulonglong>(traces_->canvas()->focused_column()));
    state["zoom"]=traces_->findChild<QSpinBox*>("traceZoom")->value();state["gain"]=traces_->findChild<QSpinBox*>("traceGain")->value();state["spacing"]=traces_->findChild<QComboBox*>("traceSpacing")->currentIndex();
    auto ids=[](const std::vector<SequenceId>& rows){QJsonArray array;for(auto id:rows)array.append(QString::number(id));return array;};state["selected"]=ids(view_->selected_ids());
    if(const auto& f=view_->fragment()){QJsonObject selection;selection["ids"]=ids(f->ids);selection["columns"]=f->columns;selection["start"]=QString::number(static_cast<qulonglong>(f->start));selection["width"]=QString::number(static_cast<qulonglong>(f->width));state["fragment"]=selection;}
    if(highlighter_)state["highlight"]=highlighter_->state();
    return QJsonDocument(state).toJson(QJsonDocument::Compact).toStdString();
}
void MainWindow::restore_view(const std::string& bytes){
    apply_appearance(AppPreferences::defaults());
    const auto state=QJsonDocument::fromJson(QByteArray::fromStdString(bytes)).object();if(state["version"].toInt()!=1){set_residue_palette(ResiduePalette::Classic);set_residue_display(ResidueDisplay::Letters);return;}
    if(highlighter_)highlighter_->restore(state["highlight"].toObject());
    findChild<QAction*>("identityDots")->setChecked(state["identityDots"].toBool(false));
    set_residue_palette(palette_from_id(state["residuePalette"].toString()));set_residue_display(display_from_id(state["residueDisplay"].toString()));
    auto ids=[&](const QJsonArray& values){std::vector<SequenceId> result;for(const auto& v:values){const auto id=v.toString().toULongLong();for(const auto& row:document_->rows())if(row.id==id){if(std::find(result.begin(),result.end(),id)==result.end())result.push_back(id);break;}}return result;};
    QFont font;if(!state["font"].toString().isEmpty()&&font.fromString(state["font"].toString())&&font.pointSize()>=6&&font.pointSize()<=48)view_->setFont(font);
    view_->set_name_area_width(state["names"].toInt(230));gapMode_=std::clamp(state["rightClick"].toInt(0),0,4);editMode_=state["editMode"].toInt(2)==1?1:2;const int storedMode=state["mode"].toInt(2);view_->set_mode(storedMode==0?AlignmentView::Mode::View:storedMode==1?AlignmentView::Mode::Insert:AlignmentView::Mode::Rewrite);
    if(translation_)translation_->restore(state["translation"].toObject());
    view_->select_ids(ids(state["selected"].toArray()));const auto f=state["fragment"].toObject();const auto start=f["start"].toString().toULongLong(),width=f["width"].toString().toULongLong();auto rows=ids(f["ids"].toArray());
    if(!rows.empty()&&width&&start<document_->columns()&&width<=document_->columns()-start)view_->select_fragment({rows,static_cast<std::size_t>(start),static_cast<std::size_t>(width),f["columns"].toBool(false)});
    traces_->choose_sequence(state["trace"].toString().toULongLong());
    traces_->findChild<QComboBox*>("traceSpacing")->setCurrentIndex(std::clamp(state["spacing"].toInt(),0,1));
    traces_->findChild<QSpinBox*>("traceZoom")->setValue(state["zoom"].toInt(100));traces_->findChild<QSpinBox*>("traceGain")->setValue(state["gain"].toInt(100));
    view_->horizontalScrollBar()->setValue(state["column"].toInt());view_->verticalScrollBar()->setValue(state["row"].toInt());
    if(state.contains("overviewVisible"))if(auto* overview=findChild<QWidget*>("alignmentOverview"))overview->parentWidget()->setVisible(state["overviewVisible"].toBool(true));
    traceDock_->setVisible(traces_->has_traces()&&state["traceVisible"].toBool(true));traces_->canvas()->focus_column(static_cast<std::size_t>(std::min<qulonglong>(state["traceColumn"].toString().toULongLong(),document_->columns())));
}
void MainWindow::closeEvent(QCloseEvent* event) {
    if (busy_) { statusBar()->showMessage("Wait for the current operation to finish."); event->ignore(); return; }
    if (maybe_save([this] { close(); })) {retire_recovery();clear_recovered_claim();event->accept();} else event->ignore();
}
}

