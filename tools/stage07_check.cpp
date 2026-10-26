#include "gui/alignment_view.hpp"
#include "gui/main_window.hpp"
#include "gui/mafft_run.hpp"
#include "core/fasta.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void wait(F done){QElapsedTimer timer;timer.start();while(!done()&&timer.elapsed()<30000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Timed out");}
std::string fasta(const Document& d){std::ostringstream out;write_fasta(out,d);return out.str();}
std::string residues(std::string text){std::erase_if(text,[](char c){return is_gap(c);});return text;}
struct Outcome{std::shared_ptr<Document> result;QString report,error;bool cancelled=false;};
Outcome run(std::shared_ptr<Document> source,const QString& launcher,MafftTemplateRequest request,bool cancel=false,MafftAlgorithm algorithm=MafftAlgorithm::Auto,MafftDirection direction=MafftDirection::Keep){
    MafftOptions options;options.threads=1;options.algorithm=algorithm;options.direction=direction;options.iterations=2;MafftRun job(source,{launcher,options,request},{});Outcome outcome;bool done=false;
    job.finished=[&](std::shared_ptr<Document> result,const QString& report,const QString& error,bool cancelled){outcome={result,report,error,cancelled};done=true;};
    job.log=[&](const QString& text){if(cancel&&text.contains("fixture-align-ready"))QTimer::singleShot(10,&job,[&]{job.cancel();});};job.start();wait([&]{return done;});return outcome;
}
QAction* action(MainWindow& w,const QString& title){for(auto* a:w.findChildren<QAction*>())if(a->text()==title)return a;throw std::runtime_error("Missing action "+title.toStdString());}
void insertion(){
    Document d;d.load({{"one","ACGT"},{"mask","12*4"},{"empty",""}});AlignmentView v;v.set_document(&d);v.set_mode(AlignmentView::Mode::Insert);
    v.select_fragment({{1},0,1});require(v.paste_residues("t")&&d.sequence(1).symbols.text()=="AtCGT"&&v.fragment()->start==1,"Insert after first symbol");
    require(v.paste_residues("Gu")&&d.sequence(1).symbols.text()=="AtGuCGT"&&v.fragment()->start==3,"Repeated insertion stays contiguous before untouched suffix");
    d.undo();v.refresh();require(d.sequence(1).symbols.text()=="AtCGT"&&v.fragment()->start==1,"Undo restores last inserted selection");d.undo();v.refresh();require(d.sequence(1).symbols.text()=="ACGT"&&v.fragment()->start==0,"Undo restores initial selection");
    v.select_fragment({{1},3,1});require(v.paste_residues("N")&&v.paste_residues("a")&&d.sequence(1).symbols.text()=="ACGTNa","Insert after final symbol and repeated append");
    v.select_fragment({{2},1,1});require(v.paste_residues("xy")&&d.sequence(2).symbols.text()=="12xy*4"&&v.fragment()->start==3,"Mask insertion follows same rule");
    v.select_fragment({{3},0,1});require(v.paste_residues("A")&&v.paste_residues("C")&&d.sequence(3).symbols.text()=="AC","Insert at empty-row end and repeat");
    v.select_fragment({{1},0,2});require(!v.paste_residues("AA"),"Wider selection remains ambiguous");
    v.set_mode(AlignmentView::Mode::Rewrite);v.select_fragment({{1},0,2});require(v.paste_residues("G")&&d.sequence(1).symbols.text()=="GGTNa"&&v.fragment()->start==1,"Overwrite behavior unchanged");
}
void window(const QString& fixture,const QString& screenshot){
    QTemporaryDir dir;QFile file(dir.filePath("targets Я.fasta"));require(file.open(QIODevice::WriteOnly),"GUI fixture");file.write(">target\nACG\n>same title\nA-~C\n>same title\nA~-C\n>mask\n01234567\n>empty\n\n");file.close();
    MainWindow w;w.show();w.open_path(file.fileName());auto* view=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return view->isEnabled();});view->select_all();const auto ids=view->selected_ids();view->select_ids({ids[0],ids[3]});
    for(bool fragments:{false,true}){
        bool sawTemplates=false,sawSettings=false;QTimer timer;timer.setInterval(5);QObject::connect(&timer,&QTimer::timeout,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
            if(dialog->objectName()=="mafftTemplates"){
                sawTemplates=true;auto* rows=dialog->findChild<QListWidget*>("mafftTemplateRows");require(rows&&rows->count()==2,"Picker excludes selected targets, masks and empty rows");
                require(rows->item(0)->text()!=rows->item(1)->text(),"Row numbers disambiguate duplicate titles");if(!screenshot.isEmpty()&&!fragments)dialog->grab().save(screenshot);
                rows->item(1)->setCheckState(Qt::Unchecked);dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
            }else if(dialog->objectName()=="mafftSettings"){
                sawSettings=true;require(dialog->findChild<QComboBox*>("mafftDirection")->isEnabled(),"Template target strand adjustment enabled");if(fragments)require(!dialog->findChild<QComboBox*>("mafftAlgorithm")->isEnabled(),"Fragments use Automatic");
                dialog->findChild<QLineEdit*>("mafftLauncher")->setText(fixture);dialog->findChild<QSpinBox*>("mafftThreads")->setValue(1);dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
            }
        });timer.start();action(w,fragments?"Align selected fragments to templates…":"Align selected sequences to templates…")->trigger();timer.stop();wait([&]{return view->isEnabled();});require(sawTemplates&&sawSettings,"Both dialogs exercised");
        MainWindow* result=nullptr;for(auto* widget:QApplication::topLevelWidgets())if(auto* other=dynamic_cast<MainWindow*>(widget);other&&other!=&w&&other->windowTitle().startsWith("MAFFT"))result=other;
        require(result&&!w.windowTitle().contains('*'),"Result window and unchanged saved source");
        QTimer::singleShot(0,[&]{auto* report=qobject_cast<QDialog*>(QApplication::activeModalWidget());auto text=report->findChild<QPlainTextEdit*>()->toPlainText();require(text.contains("Template rows sent: 1")&&text.contains("Preservation checks: passed")&&text.contains(fragments?"--addfragments":"--add"),"Reviewable template report");report->reject();});action(*result,"Last alignment report…")->trigger();delete result;
    }
    view->unselect_all();bool invalid=false;QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());invalid=dialog&&!dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled();dialog->reject();});action(w,"Align selected sequences to templates…")->trigger();require(invalid,"No-target request cannot continue");
}
}
int main(int argc,char** argv){
    QApplication app(argc,argv);
    try{
        require(argc>=2,"Pass MAFFT process fixture");qputenv("KLEIOTIDE_FIXTURE_VERSION","7.526");insertion();const auto fixture=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();auto source=std::make_shared<Document>();source->load({{"target","a~CG"},{"dup","A-~C"},{"dup","a~-C"},{"mask","0123456789"},{"empty",""}});const auto before=fasta(*source);
        for(bool fragments:{false,true}){const auto o=run(source,fixture,{{1},{2,3},fragments});if(!o.result)std::cerr<<o.error.toStdString()<<'\n';require(o.result&&o.error.isEmpty()&&!o.cancelled,"Fixture addition succeeds");require(fasta(*source)==before&&o.result->columns()==13,"Full old axis retained plus three target insertions");require(o.result->rows()[1].symbols.text().substr(0,4)=="A-~C"&&o.result->rows()[3].symbols.text().substr(0,10)=="0123456789","Exact original gaps and mask projection");require(residues(o.result->rows()[0].symbols.text())=="aCG"&&o.report.contains("New insertion columns: 3"),"Exact target case and report");}
        qputenv("KLEIOTIDE_FIXTURE_VERSION","7.525");const auto older=run(source,fixture,{{1},{2,3},true});require(!older.result&&older.error.contains("7.526")&&fasta(*source)==before,"Older fragment engine stopped before alignment with upgrade message");qputenv("KLEIOTIDE_FIXTURE_VERSION","7.526");
        qputenv("KLEIOTIDE_FIXTURE_MODE","template-shift");const auto corrupt=run(source,fixture,{{1},{2,3},false});require(!corrupt.result&&!corrupt.error.isEmpty()&&fasta(*source)==before,"Shifted template columns rejected without changing source");qunsetenv("KLEIOTIDE_FIXTURE_MODE");
        const auto overlap=run(source,fixture,{{1},{1},false});require(!overlap.result&&!overlap.error.isEmpty(),"Runner rejects overlap");
        qputenv("KLEIOTIDE_FIXTURE_MODE","hang");const auto cancel=run(source,fixture,{{1},{2,3},false},true);qunsetenv("KLEIOTIDE_FIXTURE_MODE");require(cancel.cancelled&&!cancel.result&&fasta(*source)==before,"Template process cancellation preserves source");
        for(bool fragments:{false,true}){const auto command=mafft_batch_command({"--nuc"},false,fragments);require(command.contains("\"%KLEIOTIDE_JOB_TARGETS%\"")&&command.contains(fragments?"--addfragments":"--add"),"Windows target path quoted via environment");}
        if(argc>2){const auto real=QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();const std::string bases="ACGTTAGGCTAACCGTATCGATGCTAGCTAGGCTAACGTTAGCGATCGATCGTTAGCATGCTAGCTAACGTTACGATCGATGCTAGCATGCGATCGTACG";
            auto biological=std::make_shared<Document>();const auto reference=bases.substr(0,20)+"-~~"+bases.substr(20);biological->load({{"target",bases.substr(0,30)+"AaGCu"+bases.substr(30)},{"template",reference},{"template",reference},{"other",std::string(130,'N')},{"mask",std::string(140,'9')},{"empty",""}});const auto original=fasta(*biological);
            for(bool fragments:{false,true}){const auto o=run(biological,real,{{1},{2,3},fragments});if(!o.result)std::cerr<<o.error.toStdString()<<'\n';require(o.result&&o.error.isEmpty()&&fasta(*biological)==original,"Real MAFFT full/fragment source preservation");require(residues(o.result->rows()[0].symbols.text())==residues(biological->rows()[0].symbols.text()),"Real exact target identity/case/U");require(o.result->rows()[1].symbols.text().find("-~~")!=std::string::npos&&o.result->columns()>=140&&residues(o.result->rows()[4].symbols.text())==std::string(140,'9'),"Real template gap-only interval and entire mask tail restored");}
            for(bool fragments:{false,true}){const auto single=run(biological,real,{{1},{2},fragments});require(single.result&&single.error.isEmpty(),"Real single-template full/fragment addition");}
            const auto missing=run(biological,real,{{1,4},{2,3},true});
            require(fasta(*biological)==original&&(missing.result||!missing.error.isEmpty()),"N-only target result or process error always preserves source");
            if(missing.result)require(residues(missing.result->rows()[3].symbols.text())==std::string(130,'N'),"No loss of N-only target");
            else std::cout<<"NOTE MAFFT rejected the all-N target; error reported and source preserved.\n";
            for(bool fragments:{false,true})for(auto direction:{MafftDirection::Adjust,MafftDirection::Accurate}){
                auto reverse=std::make_shared<Document>();reverse->load({{"target",bases},{"template",bases},{"opposite template",bases},{"mask","012345",RowKind::Mask}});reverse->transform({1},Document::Transform::ReverseComplement);
                const auto before=fasta(*reverse);const auto adjusted=run(reverse,real,{{1},{2,3},fragments},false,MafftAlgorithm::Auto,direction);
                if(!adjusted.result)std::cerr<<adjusted.error.toStdString()<<'\n';
                require(adjusted.result&&adjusted.error.isEmpty()&&adjusted.report.contains("Reversed row IDs: 1")&&fasta(*reverse)==before,"Real template addition adjusts only target orientation");
                require(residues(adjusted.result->sequence(1).symbols.text())==bases&&residues(adjusted.result->sequence(2).symbols.text())==bases,"Reverse target becomes forward while templates remain forward");
            }
            auto multiple=std::make_shared<Document>();multiple->load({{"one",bases},{"two",bases.substr(10,70)},{"template",reference},{"template",reference},{"mask",std::string(140,'9')}});
            const auto several=run(multiple,real,{{1,2},{3,4},true});if(!several.result)std::cerr<<several.error.toStdString()<<'\n';require(several.result&&several.error.isEmpty(),"Real multiple informative targets");
            auto partial=std::make_shared<Document>();partial->load({{"target",bases.substr(5,75)},{"first",bases.substr(0,20)+"--~"+bases.substr(20)},{"second",bases.substr(0,30)+std::string(12,'~')+bases.substr(39)},{"untouched",std::string(155,'C')},{"mask",std::string(160,'9')}});const auto partialBefore=fasta(*partial);
            for(bool fragments:{false,true}){const auto mixed=run(partial,real,{{1},{2,3},fragments});require(mixed.result&&mixed.error.isEmpty()&&fasta(*partial)==partialBefore&&mixed.result->columns()>=160,"Real partial/gapped templates with long unrelated nucleotide and mask rows");}
            for(auto method:{MafftAlgorithm::Fast1,MafftAlgorithm::Fast2,MafftAlgorithm::Iterative,MafftAlgorithm::Local,MafftAlgorithm::Global,MafftAlgorithm::LargeGaps}){const auto o=run(biological,real,{{1},{2,3},false},false,method);if(!o.result)std::cerr<<o.error.toStdString()<<'\n';require(o.result&&o.error.isEmpty(),"Real full addition preset preserves template anchors");}
        }
        window(fixture,argc>3?QString::fromLocal8Bit(argv[3]):QString());std::cout<<"PASS stage 0.7: Insert-after/repeated typing/undo/masks, template ledger/jobs/errors/cancel/batch paths, real full/fragment additions/presets, template picker/settings/result/report\n";
    }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
