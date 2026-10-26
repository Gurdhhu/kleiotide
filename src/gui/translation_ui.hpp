#pragma once
#include "core/translation.hpp"
#include <QObject>
#include <QJsonObject>
#include <memory>
class QWidget;class QAction;class QToolBar;class QComboBox;class QLabel;class QTimer;
namespace kleiotide {class MainWindow;
class TranslationUI final:public QObject {
public:
    explicit TranslationUI(MainWindow*);
    void refresh();
    void update_selection();
    void set_available(bool);
    QJsonObject state()const;
    void restore(const QJsonObject&);
    void add_dialog(bool compact=false);
    void generate_mask();void annotate_mask_dialog(SequenceId mask=0);
private:
    std::shared_ptr<const GeneticCode> edit_custom_code(QWidget*,const TranslationSettings&);
    void export_proteins();
    void run();void display();void annotate(int);void custom_code();void copy_fragment();void fill_codes(QComboBox*,const TranslationSettings&);
    MainWindow* w_;QToolBar* bar_;QAction *enabled_,*letters_,*ruler_,*copy_,*generateMask_,*annotateMask_;QComboBox *code_,*frame_;QLabel* count_;QTimer* timer_;
    const Document* document_=nullptr;std::uint64_t content_=0;TranslationSettings settings_;
    std::unique_ptr<TranslationScan> scan_;std::unique_ptr<ProteinProfileScan> proteinScan_;
    std::shared_ptr<const TranslationResult> result_;std::shared_ptr<const ProteinProfiles> profiles_;
    std::vector<QAction*> mutationActions_;
    int rememberedMode_=2;bool refreshing_=false;
};}
