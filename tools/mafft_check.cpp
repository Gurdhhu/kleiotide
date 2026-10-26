#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/mafft_run.hpp"
#include "core/fasta.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace kleiotide;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void wait(QApplication& app,F done,int limit=15000){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<limit){app.processEvents();QThread::msleep(1);}app.processEvents();require(done(),"MAFFT test timed out");}
QAction* find(const QList<QAction*>& list,const QString& text){for(auto* a:list){auto label=a->text();label.remove('&');if(label==text)return a;if(a->menu())if(auto* child=find(a->menu()->actions(),text))return child;}return nullptr;}
QAction* action(MainWindow& w,const QString& text){auto* a=find(w.menuBar()->actions(),text);require(a,"MAFFT menu action missing");return a;}
std::string fasta(const Document& d){std::ostringstream out;write_fasta(out,d);return out.str();}
struct Outcome {std::shared_ptr<Document> result;QString report,error;bool cancelled=false;};
Outcome run(QApplication& app,std::shared_ptr<Document> source,const QString& path,std::optional<Document::ColumnRange> range={},MafftOptions options={},bool cancelWhenRunning=false){
    bool done=false;Outcome outcome;MafftRun job(source,{path,options},range);QElapsedTimer elapsed;elapsed.start();std::size_t heartbeat=0;QTimer tick;tick.setInterval(1);QObject::connect(&tick,&QTimer::timeout,[&]{++heartbeat;});tick.start();
    job.finished=[&](std::shared_ptr<Document> result,const QString& report,const QString& error,bool cancelled){outcome={std::move(result),report,error,cancelled};done=true;};
    job.log=[&](const QString& text){if(cancelWhenRunning&&text.contains("fixture-align-ready"))QTimer::singleShot(100,&job,[&]{job.cancel();});};job.start();wait(app,[&]{return done;},30000);require(heartbeat>0,"GUI event loop progresses during job");return outcome;
}
int main(int argc,char** argv){
    QApplication app(argc,argv);QCoreApplication::setApplicationName("Kleiotide-MAFFT-check");
    try{
        require(argc>=2,"Pass the process fixture executable");const auto fixture=QString::fromLocal8Bit(argv[1]);QTemporaryDir directory;require(directory.isValid(),"Temporary fixture folder");
        auto source=std::make_shared<Document>();source->load({{"same Я","AaCG~~TT"},{"same Я","AA~CG~TT"},{"mask","01234567"},{"empty",""}});const auto before=fasta(*source);
        auto result=run(app,source,fixture);require(result.result&&result.error.isEmpty()&&!result.cancelled,"Whole process result");require(fasta(*source)==before&&result.result->rows()[0].symbols.text()=="AaCGTT~"&&result.result->rows()[2].symbols.text()=="01234567","Source and masks unchanged, returned case preserved");require(result.report.contains("--preservecase")&&result.report.contains("7.525")&&result.report.contains("Masks excluded: 1"),"Job report records options/version/masks");
        require(safe_mafft_thread_count(-1)==1&&safe_mafft_thread_count(0)==1&&safe_mafft_thread_count(3)==3&&safe_mafft_thread_count(1024)==8,"Safe automatic thread bounds");
        const auto attempts=directory.filePath("attempts.txt");qputenv("KLEIOTIDE_FIXTURE_ATTEMPTS",attempts.toUtf8());
        for(const auto& mode:{"corefailure","coreempty"}){
            QFile::remove(attempts);qputenv("KLEIOTIDE_FIXTURE_MODE",mode);auto retried=run(app,source,fixture);
            require(retried.result&&retried.error.isEmpty()&&fasta(*source)==before,"Core-detection failure safely retries without source edits");
            QFile calls(attempts);require(calls.open(QIODevice::ReadOnly),"Attempt record");const auto lines=calls.readAll().trimmed().split('\n');
            const auto resolved=safe_mafft_thread_count(QThread::idealThreadCount());
            require(lines.size()==(resolved>1?2:1)&&lines.back()=="1","At most one automatic retry, using one thread");
            if(resolved>1)require(retried.report.contains("Retry:")&&retried.report.contains("--thread 1"),"Retry report records actual final options");
        }
        qputenv("KLEIOTIDE_FIXTURE_MODE","corealways");QFile::remove(attempts);const auto failedCore=run(app,source,fixture);
        require(!failedCore.result&&failedCore.error.contains("physical cores")&&fasta(*source)==before,"Repeated core-count failure stops with a useful diagnostic");
        QFile::remove(attempts);MafftOptions explicitThreads;explicitThreads.threads=2;const auto explicitFailure=run(app,source,fixture,{},explicitThreads);
        require(!explicitFailure.result&&!explicitFailure.report.contains("Retry:"),"Explicit thread requests do not silently change");
        qunsetenv("KLEIOTIDE_FIXTURE_MODE");qunsetenv("KLEIOTIDE_FIXTURE_ATTEMPTS");
        auto fragment=run(app,source,fixture,Document::ColumnRange{2,4});require(fragment.result&&fragment.result->rows()[0].symbols.text()=="AaCG~~TT"&&fragment.result->rows()[1].symbols.text()=="AACG~~TT","Column process merge and exact outside context");
        for(const auto& mode:{"badversion","failure","empty","corrupt"}){qputenv("KLEIOTIDE_FIXTURE_MODE",mode);const auto failure=run(app,source,fixture);require(!failure.result&&!failure.error.isEmpty()&&fasta(*source)==before,"Failed process cannot change source");}qunsetenv("KLEIOTIDE_FIXTURE_MODE");
        auto mixed=std::make_shared<Document>();mixed->load({{"selected1","AaCGTT"},{"unselected protein","MEF*",RowKind::Protein},{"selected2","AACGTT"},{"unselected mask","012345",RowKind::Mask}});mixed->apply_groups({{1,"Keep",0x0072b2}},{{1,1},{2,1}});
        bool subsetDone=false;std::shared_ptr<Document> subsetResult;QString subsetError;MafftRun subsetJob(mixed,{fixture,{}, {},{1,3}},{});subsetJob.finished=[&](auto doc,const QString& report,const QString& error,bool){subsetResult=doc;subsetError=error;require(report.contains("selected sequences only"),"Selected scope in report");subsetDone=true;};subsetJob.start();wait(app,[&]{return subsetDone;});
        require(subsetResult&&subsetError.isEmpty()&&subsetResult->rows().size()==2&&subsetResult->rows()[0].id==1&&subsetResult->rows()[1].id==3&&subsetResult->sequence(1).groupId==1&&mixed->rows().size()==4,"Selected-only MAFFT excludes unselected proteins/masks and preserves groups/IDs/source");
        const auto missing=run(app,source,directory.filePath("missing executable"));require(!missing.result&&missing.error.contains("Cannot start"),"Failed launch is actionable");
        const auto marker=directory.filePath("child.marker");qputenv("KLEIOTIDE_FIXTURE_MARKER",marker.toUtf8());qputenv("KLEIOTIDE_FIXTURE_MODE","hang");const auto cancelled=run(app,source,fixture,{}, {},true);qunsetenv("KLEIOTIDE_FIXTURE_MODE");require(cancelled.cancelled&&!cancelled.result&&cancelled.error.isEmpty(),"Cancel discards result without error");
        bool preparedCancel=false;MafftRun early(source,{fixture,{}},{});early.finished=[&](std::shared_ptr<Document> doc,const QString&,const QString& error,bool cancel){require(!doc&&error.isEmpty()&&cancel,"Preparation cancellation is clean");preparedCancel=true;};early.start();early.cancel();wait(app,[&]{return preparedCancel;});
        QFile childMarker(marker);require(childMarker.open(QIODevice::ReadOnly),"Child ran before cancellation");const auto markerBefore=childMarker.readAll();childMarker.close();QElapsedTimer stopped;stopped.start();while(stopped.elapsed()<140){app.processEvents();QThread::msleep(1);}require(childMarker.open(QIODevice::ReadOnly)&&childMarker.readAll()==markerBefore,"Cancellation stops descendant process");
        const auto batch=mafft_batch_command({"--nuc","--thread","-1","--op","1.53"});require(batch.contains("/v:off")&&batch.contains("%KLEIOTIDE_MAFFT_LAUNCHER%")&&batch.contains("%KLEIOTIDE_JOB_INPUT%"),"Batch paths use isolated quoted environment values");bool rejected=false;try{mafft_batch_command({"&calc.exe"});}catch(const std::exception&){rejected=true;}require(rejected,"Batch rejects shell-bearing options");
        if(argc>2){
            const auto real=QString::fromLocal8Bit(argv[2]);auto biological=std::make_shared<Document>();const std::string bases="ACGTTAGGCTAACCGTATCGATGCTAGCTAGGCTAACGTTAGCGATCGATCGTTAGCATGCTAGCTAACGTTACGATCGATGCTAGCATGCGATCGTACG";
            biological->load({{"dup Я",bases},{"dup Я",bases.substr(0,30)+"AaNNRYu?"+bases.substr(35)},{"short",bases.substr(10,65)},{"mask","012345"},{"empty",""}});const auto original=fasta(*biological);
            for(int method=0;method<8;++method){MafftOptions options;options.algorithm=static_cast<MafftAlgorithm>(method);options.threads=1;options.iterations=2;if(method==6)options.offset=0;const auto aligned=run(app,biological,real,{},options);if(!aligned.result)std::cerr<<aligned.error.toStdString()<<'\n';require(aligned.result&&fasta(*biological)==original,"Real MAFFT preset preserves every residue and source");}
            const auto alignedColumns=run(app,biological,real,Document::ColumnRange{12,30});require(alignedColumns.result&&alignedColumns.error.isEmpty(),"Real MAFFT column realignment");
            for(int direction=1;direction<=2;++direction){auto oriented=std::make_shared<Document>();oriented->load({{"forward",bases},{"reverse",bases}});oriented->transform({oriented->rows()[1].id},Document::Transform::ReverseComplement);MafftOptions options;options.threads=1;options.direction=static_cast<MafftDirection>(direction);const auto adjusted=run(app,oriented,real,{},options);require(adjusted.result&&adjusted.report.contains("Reversed row IDs: 2"),"Real MAFFT orientation tracks reverse complement");}
        }
        // Exercise the real settings dialog/menu and result-window lifecycle.
        const auto path=directory.filePath("input Я with spaces.fasta");QFile file(path);require(file.open(QIODevice::WriteOnly),"GUI source file");file.write(QByteArray::fromStdString(before));file.close();MainWindow window;window.show();window.open_path(path);auto* view=dynamic_cast<AlignmentView*>(window.centralWidget());wait(app,[&]{return view->isEnabled();});
        const auto toolbarState=fasta(window.current_document());auto* toolbar=window.findChild<QToolBar*>("editingToolbar");require(toolbar&&toolbar->isVisible(),"Editing toolbar starts visible");
        toolbar->hide();app.processEvents();require(!toolbar->isVisible(),"Hide toolbar fixture");action(window,"Editing controls")->trigger();require(toolbar->isVisible(),"View menu restores editing controls without restart");
        toolbar->hide();action(window,"Restore default layout")->trigger();app.processEvents();require(toolbar->isVisible()&&fasta(window.current_document())==toolbarState,"Restore layout retains document content");
        auto launchMenu=[&](const QString& label){QTimer::singleShot(0,[&]{auto* dialog=dynamic_cast<QDialog*>(QApplication::activeModalWidget());require(dialog&&dialog->objectName()=="mafftSettings","Friendly MAFFT settings dialog");auto* launcher=dialog->findChild<QLineEdit*>("mafftLauncher");require(launcher,"Launcher chooser");launcher->setText(fixture);if(argc>3)require(dialog->grab().save(QString::fromLocal8Bit(argv[3])),"Settings screenshot");dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});action(window,label)->trigger();wait(app,[&]{return view->isEnabled();});};
        launchMenu("Align whole alignment…");MainWindow* resultWindow=nullptr;for(auto* widget:app.topLevelWidgets())if(auto* w=dynamic_cast<MainWindow*>(widget);w&&w!=&window&&w->windowTitle().startsWith("MAFFT"))resultWindow=w;require(resultWindow,"Result opens separately");
        require(window.windowTitle().contains("input Я with spaces")&&!window.windowTitle().contains(" *"),"Original remains saved and unchanged");
        QTimer::singleShot(0,[]{auto* dialog=dynamic_cast<QDialog*>(QApplication::activeModalWidget());require(dialog&&dialog->findChild<QPlainTextEdit*>()->toPlainText().contains("Source revision:"),"Result report available");dialog->reject();});action(*resultWindow,"Last alignment report…")->trigger();delete resultWindow;
        view->select_ids({1,2});launchMenu("Align selected sequences…");resultWindow=nullptr;for(auto* widget:app.topLevelWidgets())if(auto* w=dynamic_cast<MainWindow*>(widget);w&&w!=&window&&w->windowTitle().startsWith("MAFFT"))resultWindow=w;require(resultWindow&&resultWindow->current_document().rows().size()==2,"Selected menu opens selected rows only in a new window");delete resultWindow;
        view->set_mode(AlignmentView::Mode::Rewrite);view->select_all();view->select_fragment({view->selected_ids(),2,4});launchMenu("Realign selected columns…");
        std::vector<QPointer<MainWindow>> windows;for(auto* widget:app.topLevelWidgets())if(auto* w=dynamic_cast<MainWindow*>(widget);w&&w!=&window)windows.push_back(w);
        for(auto& w:windows)if(w)delete w.data();
        qputenv("KLEIOTIDE_FIXTURE_MODE","hang");
        QTimer::singleShot(0,[&]{auto* dialog=dynamic_cast<QDialog*>(QApplication::activeModalWidget());require(dialog,"Cancellation settings dialog");dialog->findChild<QLineEdit*>("mafftLauncher")->setText(fixture);dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});
        action(window,"Align whole alignment…")->trigger();require(!view->isEnabled(),"Source is immutable while the job runs");
        require(action(window,"New window")->isEnabled(),"Other windows remain available during MAFFT");action(window,"New window")->trigger();
        QDialog* progress=nullptr;wait(app,[&]{for(auto* widget:app.topLevelWidgets())if(auto* dialog=dynamic_cast<QDialog*>(widget);dialog&&dialog->objectName()=="mafftProgress"){auto* log=dialog->findChild<QPlainTextEdit*>("mafftLog");if(log&&log->toPlainText().contains("fixture-align-ready")){progress=dialog;return true;}}return false;});
        progress->findChild<QPushButton*>("mafftCancel")->click();wait(app,[&]{return view->isEnabled();});qunsetenv("KLEIOTIDE_FIXTURE_MODE");require(window.windowTitle().contains("input Я with spaces")&&!window.windowTitle().contains(" *"),"GUI cancellation keeps source untouched");
        windows.clear();for(auto* widget:app.topLevelWidgets())if(auto* w=dynamic_cast<MainWindow*>(widget);w&&w!=&window)windows.push_back(w);for(auto& w:windows)if(w)delete w.data();
        std::cout<<"PASS MAFFT: validated whole/column jobs, case/masks, process errors/cancellation/tree, batch quoting, real presets/direction when provided, settings and separate result windows\n";
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
