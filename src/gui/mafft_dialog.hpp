#pragma once
#include "gui/mafft_run.hpp"
#include <QDialog>
class QLabel;
class QPlainTextEdit;
class QPushButton;
namespace kleiotide {
QLabel* mafft_download_note(QWidget* parent = nullptr);
std::optional<MafftConfiguration> choose_mafft(QWidget*,std::optional<Document::ColumnRange>,std::optional<MafftTemplateRequest> = {},bool selectedOnly=false,RowKind molecule=RowKind::Nucleotide);
std::optional<MafftTemplateRequest> choose_mafft_templates(QWidget*,const Document&,const std::vector<SequenceId>&,bool fragments);
class MafftProgress final:public QDialog {
public:
    explicit MafftProgress(QWidget* parent);
    void set_status(const QString&);
    void append_log(const QString&);
    void completed(const QString& error);
    std::function<void()> cancel_requested;
protected:
    void reject() override;
private:
    QLabel* status_;
    QPlainTextEdit* log_;
    QPushButton* button_;
    bool running_=true;
};
}
