#include "gui/mafft_run.hpp"
#include "gui/mafft_dialog.hpp"
#include "core/fasta.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QThread>
#include <QTimer>
#include <iostream>
#include <sstream>
using namespace kleiotide;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void wait(QApplication& app,F f,int ms=30000){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<ms){app.processEvents();QThread::msleep(1);}need(f(),"Timed out");}
std::string fasta(const Document& d){std::ostringstream o;write_fasta(o,d);return o.str();}
std::string ungap(std::string_view t){std::string r;for(char c:t)if(!is_gap(c))r+=c;return r;}
struct Outcome{std::shared_ptr<Document> result;QString error,report;bool cancelled=false;};
Outcome run(QApplication& app,const std::shared_ptr<Document>& source,MafftConfiguration config,std::optional<Document::ColumnRange> range={},bool cancel=false){bool done=false;Outcome o;MafftRun job(source,config,range);job.finished=[&](auto d,const auto& report,const auto& error,bool cancelled){o={d,error,report,cancelled};done=true;};job.start();if(cancel)job.cancel();wait(app,[&]{return done;});if(!o.result&&!o.error.isEmpty())std::cerr<<o.error.toStdString()<<'\n';return o;}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    need(argc>=2,"Pass MAFFT 7.526+ launcher");const auto path=QString::fromLocal8Bit(argv[1]);MafftOptions options;options.molecule=RowKind::Protein;options.threads=1;options.iterations=2;
    const std::string aa="MeFYWVILACGTNDBZJXUO*?AGPGKTLDNAEQLRDKLMETFHG";
    auto source=std::make_shared<Document>();source->load({{"dup Я",aa,RowKind::Protein},{"dup Я",aa.substr(0,18)+"~ME~F"+aa.substr(22),RowKind::Protein},{"short",aa.substr(8,29),RowKind::Protein},{"mask","0123456789",RowKind::Mask},{"empty","",RowKind::Protein}});source->apply_groups({{1,"Family",0x0072b2}},{{1,1},{2,1}});const auto before=fasta(*source);
    int jobs=0;
    for(int method=0;method<8;++method){options.algorithm=static_cast<MafftAlgorithm>(method);options.offset=method==6?0:.123;auto r=run(app,source,{path,options});need(r.result&&r.error.isEmpty(),"Real protein preset");for(auto id:{1,2,3})need(ungap(r.result->sequence(id).symbols.text())==ungap(source->sequence(id).symbols.text())&&r.result->sequence(id).kind==RowKind::Protein,"All protein symbols preserved");need(r.result->sequence(1).groupId==1&&r.result->sequence(4).symbols.text()=="0123456789"&&r.report.contains("--amino")&&!r.report.contains("--kimura")&&r.report.contains("scored as unknown"),"Metadata and transparent protein report");++jobs;}
    options.algorithm=MafftAlgorithm::Auto;options.offset=.123;
    for(int bl:{30,45,62,80}){options.blosum=bl;auto r=run(app,source,{path,options});need(r.result!=nullptr,"Real BLOSUM model");++jobs;}
    for(auto model:{ProteinScoring::Jtt,ProteinScoring::Transmembrane}){options.proteinScoring=model;options.proteinPam=200;auto r=run(app,source,{path,options});need(r.result!=nullptr,"Real PAM model");++jobs;}options.proteinScoring=ProteinScoring::Blosum;options.blosum=62;options.composition=true;auto composition=run(app,source,{path,options});need(composition.result!=nullptr,"Real protein composition");++jobs;options.composition=false;
    auto columns=run(app,source,{path,options},Document::ColumnRange{5,20});need(columns.result!=nullptr,"Real protein selected columns");for(auto id:{1,2,3}){const auto& old=source->sequence(id).symbols.text();const auto& now=columns.result->sequence(id).symbols.text();need(now.substr(0,5)==old.substr(0,5)&&now.ends_with(old.substr(std::min<std::size_t>(25,old.size()))),"Column context preserved");}++jobs;
    auto mixed=std::make_shared<Document>();mixed->load({{"dna","ACGTTAGC"},{"template","M-~EFYWVIL",RowKind::Protein},{"target","MEFYWVIL",RowKind::Protein},{"other","W?~UO*X---",RowKind::Protein},{"mask","1234567890",RowKind::Mask}});mixed->apply_groups({{1,"A",0x009988}},{{2,1},{3,1}});const auto mixedBefore=fasta(*mixed);
    auto subset=run(app,mixed,{path,options,{}, {2,3}});need(subset.result&&subset.result->rows().size()==2&&subset.result->rows()[0].id==2&&subset.result->rows()[1].id==3,"Protein-only subset in mixed document");++jobs;
    for(bool fragments:{false,true}){auto r=run(app,mixed,{path,options,MafftTemplateRequest{{3},{2},fragments}});need(r.result&&r.result->rows().size()==5,"Real protein template addition");need(r.result->sequence(3).kind==RowKind::Protein&&r.result->sequence(3).groupId==1&&ungap(r.result->sequence(3).symbols.text())=="MEFYWVIL","Target metadata/residues");for(auto id:{1,2,4,5}){auto expected=mixed->sequence(id).symbols.text();expected.resize(r.result->columns(),'~');need(r.result->sequence(id).symbols.text()==expected,"Nonparticipants/templates exact with virtual-end padding");}++jobs;}
    // An insertion must be retained, with a lossless projection of all original rows.
    auto insertion=std::make_shared<Document>();insertion->load({{"template","M-~EFYWVIL",RowKind::Protein},{"target","MEFWWWWWWWWWWWWWWWWWWWWWWWWWYWVIL",RowKind::Protein},{"mask","0123456789",RowKind::Mask},{"dna","AcG-T"}});
    for(bool fragments:{false,true}){auto r=run(app,insertion,{path,options,MafftTemplateRequest{{2},{1},fragments}});need(r.result!=nullptr,"Real insertion template job");need(ungap(r.result->sequence(2).symbols.text())==ungap(insertion->sequence(2).symbols.text()),"No inserted residues lost");const auto& mask=r.result->sequence(3).symbols.text();std::size_t previous=0;for(std::size_t c=0;c<10;++c){auto at=mask.find(static_cast<char>('0'+c),previous);need(at!=std::string::npos,"Original column retained in projected mask");for(auto id:{1,4}){const auto& original=insertion->sequence(id).symbols.text();if(c<original.size())need(r.result->sequence(id).symbols.text()[at]==original[c],"Original nucleotide/template relationship");}previous=at+1;}++jobs;}
    auto bad=run(app,mixed,{path,options});need(!bad.result&&bad.error.contains("cannot mix")&&!bad.report.contains("Launcher:"),"Mixed input rejected before external launch");auto wrong=options;wrong.molecule=RowKind::Nucleotide;auto wrongResult=run(app,source,{path,wrong});need(!wrongResult.result&&wrongResult.error.contains("do not match")&&!wrongResult.report.contains("Launcher:"),"Wrong type cannot launch");auto cancelled=run(app,source,{path,options},{},true);need(cancelled.cancelled&&!cancelled.result&&cancelled.error.isEmpty(),"Protein preparation cancellation");need(fasta(*source)==before&&fasta(*mixed)==mixedBefore,"All sources unchanged");
    // Exercise the actual settings controls without running another alignment.
    QString uiError;QTimer::singleShot(0,[&]{try{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());need(d&&d->objectName()=="mafftSettings","Protein settings dialog");need(!d->findChild<QComboBox*>("mafftDirection")->isVisible()&&!d->findChild<QSpinBox*>("mafftKimura")->isVisible(),"Nucleotide controls hidden");auto* score=d->findChild<QComboBox*>("mafftProteinScoring");need(score&&score->isVisible(),"Protein controls available");score->setCurrentIndex(1);d->findChild<QSpinBox*>("mafftProteinPam")->setValue(120);d->findChild<QLineEdit*>("mafftLauncher")->setText(path);d->findChild<QSpinBox*>("mafftThreads")->setValue(1);if(argc>2)need(d->grab().save(QString::fromLocal8Bit(argv[2])),"Protein settings screenshot");d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();}catch(const std::exception& e){uiError=e.what();if(auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget()))d->reject();}});
    auto config=choose_mafft(nullptr,{},{},false,RowKind::Protein);need(uiError.isEmpty()&&config&&config->options.molecule==RowKind::Protein&&config->options.proteinScoring==ProteinScoring::Jtt&&config->options.proteinPam==120,"Protein settings configuration");
    QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());auto* list=d->findChild<QListWidget*>("mafftTemplateRows");if(!list||list->count()!=2){uiError="Template chooser must show only eligible proteins";d->reject();return;}d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});auto request=choose_mafft_templates(nullptr,*mixed,{3},true);need(uiError.isEmpty()&&request&&request->templates==std::vector<SequenceId>({2,4}),"Same-molecule template chooser");
    std::cout<<"PASS "<<jobs<<" real protein MAFFT jobs: 8 presets, 4 BLOSUM, JTT/TM, composition, columns, selected-only, full/fragment templates and insertion projection; rejection, cancellation, settings and source preservation\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
