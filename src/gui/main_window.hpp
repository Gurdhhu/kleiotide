#pragma once
#include "core/alignment.hpp"
#include "core/fasta.hpp"
#include "gui/residue_style.hpp"
#include <QMainWindow>
#include <QElapsedTimer>
#include <QStringList>
#include <functional>
#include <memory>
#include <vector>
class QAction;
class QCloseEvent;
class QDockWidget;
class QMenu;
class QTimer;
namespace kleiotide {
class AlignmentView;
class AlignmentOverview;
class ChromatogramPanel;
class MotifNavigator;
class SiteHighlighter;
class TranslationUI;
class RecoveryStore;
class RecoveryClaim;
struct AppPreferences;
enum class RenameOperation;
struct MafftConfiguration;
class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    void offer_recovery(bool startup=false);
    void open_path(const QString& path);
    void open_paths(const QStringList& paths);
    const Document& current_document() const {return *document_;}
protected:
    void closeEvent(QCloseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void changeEvent(QEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
    QMenu* createPopupMenu() override;
private:
    friend class TranslationUI;
    TranslationUI* translation_=nullptr;
    void load_files(const QStringList& paths, bool append);
    bool prepare_trace_edit();
    void paste(bool newDocument = false, bool atCaret = false);
    void paste_fragment();
    void save(bool saveAs = false, std::function<void()> onSuccess = {},bool exportFasta=false);
    void install_document(std::shared_ptr<Document> document, const QString& path, bool saved);
    void add_new_sequence();
    void set_sequence_type();
    void link_traces();
    void delete_traces();
    void search_blast(bool protein);
    void delete_selected();
    void collapse_sequences();
    void merge_sequences(bool findOverlap);
    void export_trace();
    void export_traces();
    void sort_sequences(Document::Sort method);
    void align_mafft(bool columns);
    void align_mafft_selected();
    void align_mafft_templates(bool fragments);
    void start_mafft(MafftConfiguration,std::optional<Document::ColumnRange>);
    void show_mafft_report();
    void show_statistics();
    void show_composition();
    void show_distances(int report=0,bool sorting=false);
    void rename_from_table();
    void batch_rename(RenameOperation);
    void rename_one();
    void rename_sequence(SequenceId id);
    void copy_selected(bool fasta);
    void copy_clipboard();
    void cut_clipboard();
    void copy_fragment_clipboard(bool cut);
    void cut_selected();
    void group_action(int operation);
    void group_by_identity();
    void select_one_per_group();
    void quality_edit(bool trim);
    void choose_font();
    void apply_appearance(const AppPreferences&);
    void apply_operational_preferences();
    AppPreferences current_appearance()const;
    void select_regex(bool sequences);
    enum class SelectionTool { TitlePrefix,TitleContains,TitleExact,DuplicateNames,DuplicateSequences,Motif,Random };
    void select_by_rule(SelectionTool);
    void run_row_selection(std::function<std::vector<SequenceId>(const Document&)>);
    bool maybe_save(std::function<void()> continuation = {});
    void set_busy(bool busy, const QString& text = {});
    void update_state();
    void update_selection_state();
    int displayedMode_=-1;
    const Document* displayedDocument_=nullptr;
    std::uint64_t displayedState_=0;
    void update_toolbar_layout();
    void set_residue_palette(ResiduePalette);
    void set_residue_display(ResidueDisplay);
    std::shared_ptr<Document> document_;
    ChromatogramPanel* traces_ = nullptr;
    QDockWidget* traceDock_ = nullptr;
    MotifNavigator* motif_=nullptr;
    QDockWidget* motifDock_=nullptr;
    AlignmentOverview* overview_ = nullptr;
    AlignmentView* view_ = nullptr;
    SiteHighlighter* highlighter_=nullptr;
    std::vector<QAction*> mutatingActions_;
    QAction* undoAction_ = nullptr;
    QAction* redoAction_ = nullptr;
    QAction* deleteAction_ = nullptr;
    QString path_;
    QString resultLabel_,mafftReport_;
    std::uint64_t savedState_ = 0;
    bool nativePath_=false;
    FastaImport fastaImport_=FastaImport::NucleotideAndMasks;
    std::string session_view() const;
    void restore_view(const std::string& state);
    void write_recovery(bool forced=false);
    void retire_recovery();
    void clear_recovered_claim();
    std::shared_ptr<RecoveryStore> recovery_;
    std::unique_ptr<RecoveryClaim> recoveryClaim_;
    QTimer* recoveryTimer_=nullptr;
    QElapsedTimer checkpointElapsed_,recoveryIdle_,recoveryAttempt_;
    QString recoveryStatus_="No recovery checkpoint has been written for this document.";
    bool openingRecovery_=false,consumeRecovery_=false;
    bool busy_ = false;
    int editMode_=2, gapMode_=0;
};
}

