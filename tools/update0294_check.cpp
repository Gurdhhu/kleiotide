#include "test_preferences.hpp"
#include "core/session.hpp"
#include <sstream>
#include "gui/alignment_view.hpp"
#include "gui/file_output.hpp"
#include "gui/main_window.hpp"
#include "gui/mafft_dialog.hpp"
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void wait(F f){QElapsedTimer elapsed;elapsed.start();while(!f()&&elapsed.elapsed()<10000){QApplication::processEvents();QThread::msleep(1);}need(f(),"Workflow timed out");QApplication::processEvents();}
QAction* action(QObject* parent,const QString& name){for(auto* a:parent->findChildren<QAction*>())if(a->objectName()==name||a->text()==name)return a;throw std::runtime_error("Action not found: "+name.toStdString());}
template<class T>T* child(QObject* parent,const char* name){auto* p=parent->findChild<T*>(name);need(p,name);return p;}
template<class F,class C>void modal(F launch,C check){QString error;bool done=false;QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());try{need(dialog,"Expected modal dialog");check(dialog);done=true;}catch(const std::exception& e){error=e.what();}if(dialog)dialog->reject();});launch();need(done&&error.isEmpty(),qPrintable(error));}
template<class F,class C>auto file_choice(F launch,C check){QString error;bool done=false;QTimer driver;driver.setInterval(10);QObject::connect(&driver,&QTimer::timeout,[&]{auto* d=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());if(!d)return;if(done){QMetaObject::invokeMethod(d,"accept",Qt::DirectConnection);return;}try{check(d);done=true;}catch(const std::exception& e){error=e.what();d->reject();driver.stop();}});driver.start();auto result=launch();driver.stop();need(done&&error.isEmpty(),qPrintable(error));return result;}
void mafft_link(QDialog* d){auto* note=child<QLabel>(d,"mafftDownload");need(note->openExternalLinks()&&note->text().contains("https://mafft.cbrc.jp/alignment/software/")&&note->text().contains("Download MAFFT"),"MAFFT official download link");}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);QStandardPaths::setTestModeEnabled(true);app.setApplicationName("Kleiotide-0294-check");try{
    const auto arguments=QCoreApplication::arguments();
    if(arguments.size()==4&&arguments[1]=="--folder-probe"){
        QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,arguments[2]);
        need(remembered_file_directory()==arguments[3],"Folder persisted in another process");return 0;
    }
    TestPreferences preferences;QTemporaryDir profile,files;need(profile.isValid()&&files.isValid(),"Temporary fixtures");QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,profile.path());
    const auto a=files.filePath("A"),b=files.filePath("B");QDir().mkpath(a);QDir().mkpath(b);const auto input=QDir(b).filePath("reads.fasta");{QFile f(input);need(f.open(QIODevice::WriteOnly),"Write input");f.write(">one\nACGT\n");}
    remember_file_directory(a);need(file_dialog_initial(QDir(b).filePath("results.xlsx"))==QDir(a).filePath("results.xlsx"),"Last folder overrides old path while retaining suggested filename");
    auto picked=file_choice([&]{return choose_open_file(nullptr,"Open test",{},"FASTA (*.fasta)");},[&](QFileDialog* d){need(d->directory().absolutePath()==a,"Open starts in remembered folder");d->setDirectory(b);d->selectFile(input);QMetaObject::invokeMethod(d,"accept",Qt::DirectConnection);});need(picked==input&&remembered_file_directory()==b,"Open updates shared folder");
    auto saved=file_choice([&]{return choose_output_filename(nullptr,"Save test","table.xlsx","Excel workbook (*.xlsx)");},[&](QFileDialog* d){need(d->directory().absolutePath()==b,"Save shares open folder");child<QLineEdit>(d,"fileNameEdit")->setText("new-table");QMetaObject::invokeMethod(d,"accept",Qt::DirectConnection);});need(saved==QDir(b).filePath("new-table.xlsx"),qPrintable("Remembered folder retains automatic suffix rules; actual="+saved+" expected="+QDir(b).filePath("new-table.xlsx")));
    auto cancelled=file_choice([&]{return choose_open_files(nullptr,"Cancel test");},[&](QFileDialog* d){d->setDirectory(a);d->reject();});need(cancelled.isEmpty()&&remembered_file_directory()==b,"Cancelled navigation does not change preference");
    auto folder=file_choice([&]{return choose_directory(nullptr,"Choose folder",a);},[&](QFileDialog* d){need(d->directory().absolutePath()==b,"Directory picker shares remembered folder");d->reject();});need(folder.isEmpty()&&remembered_file_directory()==b,"Cancelled folder picker unchanged");
    QProcess probe;probe.start(QCoreApplication::applicationFilePath(),{"--folder-probe",profile.path(),b});need(probe.waitForStarted(5000)&&probe.waitForFinished(10000)&&probe.exitCode()==0,"Last folder survives a new process");
    auto defaults=AppPreferences::defaults();defaults.recoveryEnabled=false;need(AppSettings::instance().apply(defaults)&&remembered_file_directory()==b,"Restore defaults does not discard file navigation");
    {QSettings settings(QSettings::defaultFormat(),QSettings::UserScope,"Kleiotide","Kleiotide");settings.setValue("files/lastDirectory",files.filePath("gone"));settings.sync();}need(remembered_file_directory().isEmpty()&&file_dialog_initial(input)==input,"Missing remembered folder falls back safely");remember_file_directory(b);
    auto document=std::make_shared<Document>();std::vector<Record> rows;for(int i=0;i<180;++i)rows.push_back({"row_"+std::to_string(i),std::string(i==179?200:80,'A'),i==2||i==120?RowKind::Protein:RowKind::Nucleotide});document->load(std::move(rows));document->apply_groups({{1,"Example",0x336699}},{{121,1},{131,1}});
    const auto sessionPath=QDir(a).filePath("selection.kleio");{std::ostringstream bytes;write_session(bytes,*document);QFile f(sessionPath);need(f.open(QIODevice::WriteOnly),"Write selection session");f.write(QByteArray::fromStdString(bytes.str()));}
    MainWindow owner;owner.resize(850,450);owner.show();owner.open_path(sessionPath);auto* view=dynamic_cast<AlignmentView*>(owner.centralWidget());need(view,"Alignment view");wait([&]{return view->isEnabled()&&owner.current_document().rows().size()==180;});need(remembered_file_directory()==a,"Direct file opening updates shared folder");const auto state=owner.current_document().state_id();const auto canUndo=owner.current_document().can_undo();view->select_ids({3});view->select_fragment({{1,2},10,4});
    action(&owner,"selectFragmentBeginning")->trigger();need(view->fragment()->start==0&&view->fragment()->width==14&&view->fragment()->ids==std::vector<SequenceId>({1,2}),"Extend block to beginning");
    action(&owner,"selectFragmentEnd")->trigger();need(view->fragment()->width==200&&view->selected_ids()==std::vector<SequenceId>({3}),"Extend block to global alignment end and retain name selection");
    view->select_fragment({{1},40,1});action(&owner,"selectFragmentEnd")->trigger();need(view->fragment()->start==40&&view->fragment()->width==160,"Single residue extension");
    view->select_fragment({{1,2},12,3,true});action(&owner,"selectFragmentBeginning")->trigger();need(view->fragment()->columns&&view->fragment()->width==15,"Column-selection scope preserved");
    action(&owner,"clearFragmentSelection")->trigger();need(!view->fragment()&&view->selected_ids()==std::vector<SequenceId>({3})&&!action(&owner,"selectFragmentEnd")->isEnabled(),"Clear affects only residues and disables empty actions");need(owner.current_document().state_id()==state&&owner.current_document().can_undo()==canUndo,"Selection commands do not edit history");
    QMenu* edit=nullptr;for(auto* item:owner.menuBar()->actions())if(item->text()=="&Edit")edit=item->menu();need(edit,"Edit menu");const auto items=edit->actions();int motif=items.indexOf(action(&owner,"findMotif"));need(items[motif-1]->isSeparator()&&items[motif+1]==action(&owner,"findNextMotif")&&items[motif+2]==action(&owner,"findPreviousMotif")&&items[motif+3]->isSeparator(),"Motif commands form their own section");int first=items.indexOf(action(&owner,"selectFragmentBeginning"));need(items[first-1]->isSeparator()&&items[first+1]==action(&owner,"selectFragmentEnd")&&items[first+2]==action(&owner,"clearFragmentSelection")&&items[first+3]->isSeparator()&&items[first+4]->menu(),"Selection subsection directly above Rename");
    view->horizontalScrollBar()->setValue(0);view->verticalScrollBar()->setValue(0);view->select_fragment({{1},2,3});QString popupError;bool popup=false;QTimer::singleShot(0,[&]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());try{need(menu,"Residue context menu");auto acts=menu->actions();int i=acts.indexOf(action(menu,"Select to beginning"));need(i>0&&acts[i-1]->isSeparator()&&acts[i+1]->text()=="Select to end"&&acts[i+2]->text().startsWith("Clear fragment selection"),"Context selection subsection");action(menu,"Select to end")->trigger();popup=true;}catch(const std::exception& e){popupError=e.what();}if(menu)menu->close();});const QPoint point(view->name_area_width()+2*view->cell_width()+1,view->ruler_height()+view->row_height()/2);QContextMenuEvent context(QContextMenuEvent::Mouse,point,view->viewport()->mapToGlobal(point));QApplication::sendEvent(view->viewport(),&context);need(popup&&popupError.isEmpty()&&view->fragment()->width==198,qPrintable(popupError));
    view->horizontalScrollBar()->setValue(5);view->verticalScrollBar()->setValue(100);action(&owner,"selectProtein")->trigger();need(view->verticalScrollBar()->value()==2&&view->horizontalScrollBar()->value()==5,"Type selection reveals uppermost row without horizontal movement");
    view->verticalScrollBar()->setValue(120);action(&owner,"Select all")->trigger();need(view->verticalScrollBar()->value()==0,"Select all reveals first row");view->select_ids({5});view->verticalScrollBar()->setValue(120);action(&owner,"Invert selection")->trigger();need(view->verticalScrollBar()->value()==0,"Invert reveals uppermost selected row");
    view->verticalScrollBar()->setValue(170);QTimer::singleShot(0,[]{auto* d=qobject_cast<QInputDialog*>(QApplication::activeModalWidget());if(d){d->setTextValue("^row_12[0-9]$");d->accept();}});action(&owner,"Names matching regular expression…")->trigger();wait([&]{return view->isEnabled();});need(view->selected_ids().size()==10&&view->verticalScrollBar()->value()==120,"Background selector scrolls to first result");
    view->verticalScrollBar()->setValue(0);QTimer::singleShot(0,[]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(d){auto* list=d->findChild<QListWidget*>("selectGroupsList");if(list)list->item(1)->setCheckState(Qt::Checked);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();}});action(&owner,"selectGroups")->trigger();need(view->verticalScrollBar()->value()==120,"Group selection reveals uppermost member");
    view->verticalScrollBar()->setValue(0);QTimer::singleShot(0,[]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(d)if(auto* button=d->findChild<QPushButton*>("representativeApply"))button->click();});action(&owner,"selectOnePerGroup")->trigger();need(view->verticalScrollBar()->value()==120,"Representative selection reveals uppermost member");
    for(int mode=0;mode<5;++mode)modal([&]{if(mode==0)choose_mafft(&owner,{});else if(mode==1)choose_mafft(&owner,{}, {},true);else if(mode==2)choose_mafft(&owner,Document::ColumnRange{0,20});else choose_mafft_templates(&owner,*document,{1},mode==4);},mafft_link);
    modal([&]{action(&owner,"appSettingsAction")->trigger();},mafft_link);
    // Resume is deliberately not routed through the ordinary remembered folder.
    remember_file_directory(b);const auto jobs=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/blast-jobs";QDir().mkpath(jobs);action(&owner,"nucleotideBlast")->trigger();auto* remote=child<QDialog>(&owner,"blastDialog");file_choice([&]{child<QPushButton>(remote,"blastResume")->click();return QString{};},[&](QFileDialog* d){need(d->directory().absolutePath()==jobs,"Resume still opens default BLAST jobs folder");d->reject();});need(remembered_file_directory()==b,"Resume leaves ordinary remembered folder unchanged");remote->close();delete remote;
    need(owner.current_document().state_id()==state,"Inspection workflows leave document unchanged");owner.close();
    std::cout<<"PASS persistent shared file folders/new process/cancel/Resume exception; selection extension and menu sections; automatic first-result scrolling; MAFFT download links\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
