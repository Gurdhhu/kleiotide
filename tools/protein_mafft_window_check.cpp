#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace kleiotide;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void wait(QApplication& app,F f){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<30000){app.processEvents();QThread::msleep(1);}need(f(),"Window test timed out");}
QAction* find(const QList<QAction*>& actions,const QString& text){for(auto* a:actions){auto name=a->text();name.remove('&');if(name==text)return a;if(a->menu())if(auto* found=find(a->menu()->actions(),text))return found;}return nullptr;}
QAction* action(MainWindow& w,const QString& text){auto* a=find(w.menuBar()->actions(),text);need(a,"Missing command");return a;}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    need(argc>1,"Pass MAFFT launcher");const auto launcher=QString::fromLocal8Bit(argv[1]);QTemporaryDir dir;Document protein;protein.load({{"one","M-~EFYWVIL",RowKind::Protein},{"two","MEFYWVIL",RowKind::Protein},{"mask","1234567890",RowKind::Mask}});protein.apply_groups({{1,"Family",0x0072b2}},{{1,1},{2,1}});const auto file=dir.filePath("protein.kleio");{std::ofstream out(file.toStdString(),std::ios::binary);write_session(out,protein);}
    MainWindow window;window.show();window.open_path(file);auto* view=dynamic_cast<AlignmentView*>(window.centralWidget());wait(app,[&]{return view->isEnabled()&&window.current_document().rows().size()==3;});
    auto settings=[&]{QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());need(d&&d->objectName()=="mafftSettings","Protein settings launched from menu");need(d->findChild<QComboBox*>("mafftProteinScoring")->isVisible()&&!d->findChild<QComboBox*>("mafftDirection")->isVisible(),"Protein control selection from declared types");d->findChild<QLineEdit*>("mafftLauncher")->setText(launcher);d->findChild<QSpinBox*>("mafftThreads")->setValue(1);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});};
    auto result=[&]{wait(app,[&]{return view->isEnabled();});MainWindow* found=nullptr;for(auto* w:app.topLevelWidgets())if(auto* main=dynamic_cast<MainWindow*>(w);main&&main!=&window&&main->windowTitle().startsWith("MAFFT"))found=main;need(found,"Menu result opens separately");return found;};
    for(const auto& label:{"Align whole alignment…","Align selected sequences…","Realign selected columns…"}){view->select_ids({1,2});view->select_fragment({{1,2},2,5});settings();action(window,label)->trigger();auto* output=result();need(output->current_document().sequence(1).kind==RowKind::Protein&&output->current_document().sequence(1).groupId==1,"Protein menu metadata retained");if(QString(label)=="Align selected sequences…")need(output->current_document().rows().size()==2,"Selected-only menu scope");delete output;}
    // Similarity sorting exposes the requested N policy in the actual dialog.
    QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());need(d&&d->objectName()=="similarityDialog","Similarity dialog");bool explanation=false;for(auto* l:d->findChildren<QLabel*>())explanation|=l->text().contains("N–N is not a match")&&l->text().contains("comparable-site counts");need(explanation,"Visible N-handling explanation");d->reject();});action(window,"By decreasing similarity…")->trigger();
    std::istringstream abif(test::synthetic_abif());protein.append({read_abif(abif,"trace","missing.ab1")});const auto mixed=dir.filePath("mixed.kleio");{std::ofstream out(mixed.toStdString(),std::ios::binary);write_session(out,protein);}window.open_path(mixed);wait(app,[&]{return view->isEnabled()&&window.current_document().rows().size()==4;});const auto trace=window.current_document().sequence(4).trace;const auto dna=window.current_document().sequence(4).symbols.text();
    action(window,"Align whole alignment…")->trigger();need(window.statusBar()->currentMessage().contains("cannot mix")&&view->isEnabled(),"Mixed menu scope rejected before settings");
    for(const auto& label:{"Align selected sequences to templates…","Align selected fragments to templates…"}){view->select_ids({2});QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());need(d&&d->objectName()=="mafftTemplates","Template chooser from menu");settings();d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});action(window,label)->trigger();auto* output=result();need(output->current_document().sequence(4).trace==trace,"Unselected trace evidence retained");const auto& now=output->current_document().sequence(4).symbols.text();need(now.substr(0,dna.size())==dna,"Unselected nucleotide symbols retained");need(output->current_document().sequence(2).kind==RowKind::Protein,"Protein template target type retained");delete output;}
    need(window.current_document().sequence(4).symbols.text()==dna&&!window.current_document().can_undo(),"Source remains unchanged");std::cout<<"PASS protein menu workflows: 5 real jobs, typed settings, new windows, subset scope, mixed rejection, trace retention and visible similarity N help\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
