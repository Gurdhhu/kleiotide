#include "gui/mafft_run.hpp"
#include "core/fasta.hpp"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QThread>
#include <algorithm>
#include <QFutureWatcher>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <csignal>
#include <unistd.h>
#endif

namespace kleiotide {
namespace {
std::filesystem::path native(const QString& text){
#ifdef _WIN32
    return std::filesystem::path(text.toStdWString());
#else
    return std::filesystem::path(text.toStdString());
#endif
}
}
int safe_mafft_thread_count(int availableLogicalCores) noexcept {
    return std::clamp(availableLogicalCores,1,8);
}
QString mafft_batch_command(const QStringList& options,bool versionOnly,std::optional<bool> fragments){
    for(const auto& arg:options)if(!QRegularExpression("\\A[-a-zA-Z0-9.]+\\z").match(arg).hasMatch())throw std::invalid_argument("Invalid batch option.");
    const auto addition=fragments?QString(*fragments?" --addfragments":" --add")+" \"%KLEIOTIDE_JOB_TARGETS%\"":QString();
    const auto args=versionOnly?QString("--version"):options.join(' ')+addition+" --out \"%KLEIOTIDE_JOB_OUTPUT%\" \"%KLEIOTIDE_JOB_INPUT%\"";
    return "/d /v:off /s /c \"\"%KLEIOTIDE_MAFFT_LAUNCHER%\" "+args+"\"";
}
struct MafftRun::Data {
    std::atomic_bool cancelled=false;
    std::unique_ptr<QTemporaryDir> temporary;
    std::shared_ptr<MafftAlignment> alignment;
    std::shared_ptr<MafftTemplateAlignment> templateAlignment;
    std::size_t insertedColumns=0;
    std::shared_ptr<const Document> workingSource;
    std::shared_ptr<Document> result;
    QString error;
    std::vector<SequenceId> reversed;
    std::size_t width=0;
};
MafftRun::MafftRun(std::shared_ptr<const Document> source,MafftConfiguration configuration,std::optional<Document::ColumnRange> range,QObject* parent)
    :QObject(parent),source_(std::move(source)),configuration_(std::move(configuration)),range_(range),data_(std::make_shared<Data>()),process_(new QProcess(this)){
#ifndef _WIN32
    process_->setChildProcessModifier([]{if(::setpgid(0,0)<0)_exit(127);});
#endif
    connect(process_,&QProcess::started,this,[this]{
        processGroup_=process_->processId();
#ifdef _WIN32
        if(!jobHandle_){
            auto job=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            const auto child=OpenProcess(PROCESS_SET_QUOTA|PROCESS_TERMINATE,FALSE,static_cast<DWORD>(processGroup_));
            if(job&&child&&SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))&&AssignProcessToJobObject(job,child))jobHandle_=job;
            else if(job)CloseHandle(job);
            if(child)CloseHandle(child);
        }
#endif
        process_->closeWriteChannel();if(data_->cancelled)stop_tree();
    });
    connect(process_,&QProcess::readyReadStandardError,this,[this]{read_log();});
    connect(process_,&QProcess::readyReadStandardOutput,this,[this]{const auto bytes=process_->readAllStandardOutput();if(phase_==Phase::Version)versionText_=(versionText_+QString::fromUtf8(bytes)).right(16384);});
    connect(process_,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)finish("Cannot start MAFFT: "+process_->errorString());});
    connect(process_,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus state){process_finished(code,state==QProcess::CrashExit);});
}
MafftRun::~MafftRun(){data_->cancelled=true;stop_tree();if(process_->state()!=QProcess::NotRunning){process_->kill();process_->waitForFinished(1000);}
#ifdef _WIN32
    if(jobHandle_)CloseHandle(static_cast<HANDLE>(jobHandle_));
#endif
}
void MafftRun::start(){
    elapsed_.start();
    automaticThreads_=configuration_.options.threads==-1;
    if(automaticThreads_)configuration_.options.threads=safe_mafft_thread_count(QThread::idealThreadCount());
    try{(void)configuration_.options.arguments();if(range_&&configuration_.options.direction!=MafftDirection::Keep)throw std::invalid_argument("Column alignment must keep the original strand.");if(!configuration_.selectedRows.empty()&&(range_||configuration_.templateRequest))throw std::invalid_argument("Selected-only alignment cannot combine with columns or templates.");if(range_&&configuration_.templateRequest)throw std::invalid_argument("Template addition operates on whole target rows.");if(configuration_.templateRequest&&(configuration_.options.algorithm==MafftAlgorithm::PartTree||(configuration_.templateRequest->fragments&&configuration_.options.algorithm!=MafftAlgorithm::Auto)))throw std::invalid_argument("Template addition does not support PartTree; fragment addition uses Automatic.");}
    catch(const std::exception& e){finish(QString::fromUtf8(e.what()));return;}
    if(status)status(configuration_.options.molecule==RowKind::Protein?"Preparing protein sequences…":"Preparing nucleotide sequences…");
    auto* watcher=new QFutureWatcher<void>(this);const auto data=data_;const auto source=source_;const auto range=range_;const auto request=configuration_.templateRequest;const auto selectedRows=configuration_.selectedRows;const auto molecule=configuration_.options.molecule;
    connect(watcher,&QFutureWatcher<void>::finished,this,[this,watcher]{watcher->deleteLater();if(data_->cancelled||!data_->error.isEmpty()){finish(data_->error);return;}launch(true);});
    watcher->setFuture(QtConcurrent::run([data,source,range,request,selectedRows,molecule]{try{
        data->workingSource=selectedRows.empty()?source:std::make_shared<Document>(source->subset(selectedRows));const auto working=data->workingSource;
        data->temporary=std::make_unique<QTemporaryDir>(QDir::tempPath()+"/kleiotide-mafft-XXXXXX");if(!data->temporary->isValid())throw std::runtime_error("Cannot create an alignment temporary folder.");
        const auto cancelled=[data]{return data->cancelled.load();};
        std::ofstream input(native(data->temporary->filePath("input.fa")),std::ios::binary);
        if(request){data->templateAlignment=std::make_shared<MafftTemplateAlignment>(*source,*request,cancelled);if(data->templateAlignment->molecule()!=molecule)throw std::invalid_argument("MAFFT settings do not match the participant molecule type.");data->templateAlignment->write_templates(input,cancelled);
            std::ofstream targets(native(data->temporary->filePath("targets.fa")),std::ios::binary);data->templateAlignment->write_targets(targets,cancelled);targets.close();if(!targets)throw std::runtime_error("Cannot save MAFFT targets.");
        }else{data->alignment=std::make_shared<MafftAlignment>(*working,range,cancelled);if(data->alignment->molecule()!=molecule)throw std::invalid_argument("MAFFT settings do not match the participant molecule type.");data->alignment->write_input(input,cancelled);}
        input.close();if(!input)throw std::runtime_error("Cannot save MAFFT input.");
    }catch(const std::exception& e){data->error=QString::fromUtf8(e.what());}}));
}
void MafftRun::launch(bool versionOnly){
    if(data_->cancelled){finish();return;}
    phase_=versionOnly?Phase::Version:Phase::Align;attemptTail_.clear();
    if(status)status(versionOnly?"Checking MAFFT installation…":"MAFFT is aligning sequences…");
    QStringList options;for(const auto& arg:configuration_.options.arguments())options.push_back(QString::fromStdString(arg));
    const auto& request=configuration_.templateRequest;
    if(!versionOnly){
        report_="MAFFT "+version_+"\nLauncher: "+configuration_.launcher+"\nArguments: "+options.join(' ');
        if(request)report_+=QString(request->fragments?" --addfragments targets.fa":" --add targets.fa");
        report_+=QString(" input.fa\nMolecule: ")+(configuration_.options.molecule==RowKind::Protein?"Protein (--amino); B/Z/J/X/U/O/*/? are retained and scored as unknown.\n":"Nucleotide (--nuc)\n");
        report_+="\nThread policy: "+QString(automaticThreads_?"Automatic (safe; bounded positive count)":"Explicit count")+"; resolved "+QString::number(configuration_.options.threads)+"\n"+retryNote_;
        report_+="Source revision: "+QString::number(source_->state_id())+"\nScope: ";
        if(request){
            report_+="selected targets against chosen templates\nTarget rows sent: "+QString::number(data_->templateAlignment->target_count())+"\nTemplate rows sent: "+QString::number(data_->templateAlignment->template_count());
            report_+="\nTarget IDs:";for(auto id:request->targets)report_+=' '+QString::number(id);
            report_+="\nTemplate IDs:";for(auto id:request->templates)report_+=' '+QString::number(id);
            report_+="\nOriginal columns: "+QString::number(source_->columns())+"\nTemplate gap-only columns restored: "+QString::number(data_->templateAlignment->restored_columns());
            report_+="\nBoundary policy: retain original gap intervals before new insertion columns; insertions immediately precede the next template anchor. Trailing insertions follow the complete original axis.\nNon-target rows retain original symbols, case and gap states; virtual ends are padded with open gaps. Masks are never sent to MAFFT.\n";
        }else report_+=(range_?QString("all eligible rows, columns %1–%2").arg(range_->start+1).arg(range_->start+range_->width):(configuration_.selectedRows.empty()?"whole alignment":"selected sequences only; unselected rows excluded"))+"\nRows sent: "+QString::number(data_->alignment->input_count())+"\nMasks excluded: "+QString::number(data_->alignment->mask_count())+"\n";
    }
    auto environment=QProcessEnvironment::systemEnvironment();process_->setWorkingDirectory(data_->temporary->path());process_->setStandardOutputFile(versionOnly?QString():data_->temporary->filePath("result.fa"));
#ifdef _WIN32
    const auto suffix=QFileInfo(configuration_.launcher).suffix().toLower();
    if(suffix=="bat"||suffix=="cmd"){
        environment.insert("KLEIOTIDE_MAFFT_LAUNCHER",QDir::toNativeSeparators(configuration_.launcher));
        environment.insert("KLEIOTIDE_JOB_INPUT",QDir::toNativeSeparators(data_->temporary->filePath("input.fa")));
        environment.insert("KLEIOTIDE_JOB_TARGETS",QDir::toNativeSeparators(data_->temporary->filePath("targets.fa")));
        environment.insert("KLEIOTIDE_JOB_OUTPUT",QDir::toNativeSeparators(data_->temporary->filePath("result.fa")));
        process_->setStandardOutputFile(versionOnly?QString():data_->temporary->filePath("launcher.stdout"));
        process_->setProgram(QDir::toNativeSeparators(qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/cmd.exe"));process_->setArguments({});process_->setNativeArguments(mafft_batch_command(options,versionOnly,request?std::optional<bool>(request->fragments):std::nullopt));
    }else
#endif
    {process_->setProgram(configuration_.launcher);if(request)options+=QStringList{request->fragments?"--addfragments":"--add",data_->temporary->filePath("targets.fa")};process_->setArguments(versionOnly?QStringList{"--version"}:options+QStringList{data_->temporary->filePath("input.fa")});}
    process_->setProcessEnvironment(environment);process_->start();
    if(versionOnly)QTimer::singleShot(10000,this,[this]{if(phase_==Phase::Version){timedOut_=true;data_->error="MAFFT version check timed out.";cancel();}});
}
void MafftRun::read_log(){
    const auto text=QString::fromUtf8(process_->readAllStandardError());if(phase_==Phase::Version)versionText_=(versionText_+text).right(16384);
    logTail_=(logTail_+text).right(65536);attemptTail_=(attemptTail_+text).right(65536);if(log&&!text.isEmpty())log(text.right(16384));
}
void MafftRun::process_finished(int code,bool crashed){
    if(phase_==Phase::Done)return;
    read_log();
#ifdef _WIN32
    if(jobHandle_){CloseHandle(static_cast<HANDLE>(jobHandle_));jobHandle_=nullptr;}
#endif
    processGroup_=0;if(data_->cancelled){finish(data_->error);return;}
    if(phase_==Phase::Version){
        const auto match=QRegularExpression("(?:^|[^0-9])(7\\.[0-9]+)(?:[^0-9]|$)").match(versionText_);
        if(!match.hasMatch()||crashed){finish("The selected launcher did not report a MAFFT 7 version.\n"+versionText_);return;}
        version_=match.captured(1);
        if(configuration_.templateRequest&&configuration_.templateRequest->fragments&&version_.mid(2).toInt()<526){
            finish("Fragment addition requires MAFFT 7.526 or later. The selected installation reports "+version_+". Please update MAFFT and try again.");return;
        }
        launch(false);return;
    }
#ifdef _WIN32
    // Batch launchers can report failure on stdout and still return zero.
    QFile diagnostic(data_->temporary->filePath("launcher.stdout"));
    if(diagnostic.open(QIODevice::ReadOnly)){
        diagnostic.seek(std::max<qint64>(0,diagnostic.size()-65536));
        const auto text=QString::fromUtf8(diagnostic.readAll());
        logTail_=(logTail_+text).right(65536);attemptTail_=(attemptTail_+text).right(65536);
        if(log&&!text.isEmpty())log(text.right(16384));
    }
#endif
    const QFileInfo output(data_->temporary->filePath("result.fa"));
    const bool missingOutput=!output.isFile()||output.size()==0;
    const bool coreFailure=attemptTail_.contains("Cannot count the number of physical cores",Qt::CaseInsensitive);
    if(automaticThreads_&&!retriedThreads_&&configuration_.options.threads>1&&
       !crashed&&(code!=0||missingOutput)&&coreFailure){
        retriedThreads_=true;
        retryNote_="Retry: core detection failed with "+QString::number(configuration_.options.threads)+" threads; one retry with --thread 1.\n";
        logTail_=(logTail_+"\n"+retryNote_).right(65536);if(log)log(retryNote_);
        if(output.exists()&&!QFile::remove(output.filePath())){finish("Cannot clear an incomplete MAFFT result before retry.");return;}
        configuration_.options.threads=1;launch(false);return;
    }
    if(code!=0||crashed||missingOutput){
        QString error=code!=0||crashed?"MAFFT failed (exit code "+QString::number(code)+(crashed?", crashed).":")."):"MAFFT did not create a nonempty output alignment (exit code 0).";
        if(coreFailure)error+="\nThe MAFFT/Cygwin package could not count physical cores. An explicit thread count was supplied; check this MAFFT installation's compatibility with Windows on this computer.";
        error+="\n"+attemptTail_.right(8192);finish(error);return;
    }
    phase_=Phase::Merge;if(status)status("Checking residues and building the result…");
    auto* watcher=new QFutureWatcher<void>(this);const auto data=data_;const auto source=source_;const bool allowReverse=configuration_.options.direction!=MafftDirection::Keep;
    connect(watcher,&QFutureWatcher<void>::finished,this,[this,watcher]{watcher->deleteLater();finish(data_->error);});
    watcher->setFuture(QtConcurrent::run([data,source,allowReverse]{try{
        const auto cancelled=[data]{return data->cancelled.load();};std::ifstream out(native(data->temporary->filePath("result.fa")),std::ios::binary);
        if(!out)throw std::runtime_error("MAFFT did not create an output alignment.");
        const auto molecule=data->templateAlignment?data->templateAlignment->molecule():data->alignment->molecule();
        const auto outputType=molecule==RowKind::Protein?FastaImport::ProteinAndMasks:FastaImport::NucleotideAndMasks;
        MafftResult result;
        if(data->templateAlignment){auto merged=data->templateAlignment->merge(read_fasta(out,cancelled,outputType),cancelled,allowReverse);data->insertedColumns=merged.insertedColumns;result=std::move(merged.alignment);}
        else result=data->alignment->merge(read_fasta(out,cancelled,outputType),allowReverse,cancelled);
        data->width=result.fragmentWidth;data->reversed=std::move(result.reversed);if(cancelled())return;
        data->result=std::make_shared<Document>(data->workingSource->derive(std::move(result.records)));
    }catch(const std::exception& e){data->error=QString::fromUtf8(e.what());}}));
}
void MafftRun::cancel(){if(phase_==Phase::Done)return;data_->cancelled=true;if(status)status("Cancelling MAFFT…");stop_tree();}
void MafftRun::stop_tree(){
    if(process_->state()==QProcess::NotRunning||processGroup_<=0)return;
#ifdef _WIN32
    // Let taskkill discover the tree before killing the launcher; the job object
    // also covers children created after assignment and termination fallbacks.
    auto* killer=new QProcess(this);connect(killer,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this,killer]{
        if(jobHandle_)TerminateJobObject(static_cast<HANDLE>(jobHandle_),1);process_->kill();killer->deleteLater();
    });
    killer->start(qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/taskkill.exe",{"/PID",QString::number(processGroup_),"/T","/F"});
    QTimer::singleShot(1000,this,[this]{if(data_->cancelled&&process_->state()!=QProcess::NotRunning){if(jobHandle_)TerminateJobObject(static_cast<HANDLE>(jobHandle_),1);process_->kill();}});
#else
    ::kill(-static_cast<pid_t>(processGroup_),SIGKILL);
    process_->kill();
#endif
}
void MafftRun::finish(const QString& error){
    if(phase_==Phase::Done)return;
    phase_=Phase::Done;
    report_+="\nElapsed: "+QString::number(elapsed_.elapsed()/1000.0,'f',2)+" s\nReturned region width: "+QString::number(data_->width)+"\nReversed row IDs:";
    for(auto id:data_->reversed)report_+=' '+QString::number(id);
    if(data_->templateAlignment)report_+="\nNew insertion columns: "+QString::number(data_->insertedColumns)+"\nPreservation checks: "+(data_->result?QString("passed — every original column retained; template relationships and exact target residues verified."):QString("no result applied."));
    if(!error.isEmpty())report_+="\nError: "+error;
    report_+="\n\nMAFFT log (last 64 KiB):\n"+logTail_;
    const bool cancelled=data_->cancelled&&!timedOut_;const auto failure=cancelled?QString():error;
    if(finished)finished(cancelled||!failure.isEmpty()?nullptr:data_->result,report_,failure,cancelled);
}
}
