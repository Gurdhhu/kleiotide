#pragma once
#include "core/mafft_template.hpp"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QElapsedTimer>
#include <atomic>
#include <memory>
class QProcess;
namespace kleiotide {
// Only validated options and generated input/output paths enter the batch command.
int safe_mafft_thread_count(int availableLogicalCores) noexcept;
QString mafft_batch_command(const QStringList& options,bool versionOnly=false,std::optional<bool> fragments={});
struct MafftConfiguration {QString launcher;MafftOptions options;std::optional<MafftTemplateRequest> templateRequest;std::vector<SequenceId> selectedRows;};
class MafftRun final:public QObject {
public:
    MafftRun(std::shared_ptr<const Document>,MafftConfiguration,std::optional<Document::ColumnRange>,QObject* parent=nullptr);
    ~MafftRun() override;
    void start();
    void cancel();
    std::function<void(const QString&)> status,log;
    std::function<void(std::shared_ptr<Document>,const QString&,const QString&,bool)> finished;
private:
    struct Data;
    enum class Phase { Prepare, Version, Align, Merge, Done };
    void launch(bool versionOnly);
    void process_finished(int,bool crashed);
    void read_log();
    void finish(const QString& error={});
    void stop_tree();
    std::shared_ptr<const Document> source_;
    MafftConfiguration configuration_;
    std::optional<Document::ColumnRange> range_;
    std::shared_ptr<Data> data_;
    QProcess* process_;
    Phase phase_=Phase::Prepare;
    QString version_,versionText_,report_,logTail_,attemptTail_,retryNote_;
    bool automaticThreads_=false,retriedThreads_=false;
    QElapsedTimer elapsed_;
    qint64 processGroup_=0;
    bool timedOut_=false;
#ifdef _WIN32
    void* jobHandle_=nullptr;
#endif
};
}
