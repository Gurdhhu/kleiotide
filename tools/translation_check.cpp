#include "test_preferences.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QKeyEvent>
#include <QBackingStore>
#include <QImage>
#include <QMouseEvent>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
template<class F>void until(QApplication& app,F ready,const char* msg){QElapsedTimer timeout;timeout.start();while(!ready()&&timeout.elapsed()<15000){app.processEvents();QThread::msleep(1);}need(ready(),msg);app.processEvents();}
QAction* action(MainWindow& w,const QString& name){for(auto* a:w.findChildren<QAction*>())if(a->objectName()==name||a->text()==name)return a;throw std::runtime_error("Missing action: "+name.toStdString());}
void key(AlignmentView* v,int k){QKeyEvent e(QEvent::KeyPress,k,Qt::NoModifier);QApplication::sendEvent(v,&e);}
QImage pixels(AlignmentView& view){auto* device=view.backingStore()->paintDevice();need(device->devType()==QInternal::Image,"Raster backing store");const auto* image=static_cast<QImage*>(device);const auto origin=view.viewport()->mapTo(view.window(),QPoint(0,0));const auto dpr=image->devicePixelRatio();return image->copy(QRect(qRound(origin.x()*dpr),qRound(origin.y()*dpr),qRound(view.viewport()->width()*dpr),qRound(view.viewport()->height()*dpr)));}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{kleiotide::TestPreferences testPreferences;QTemporaryDir dir;Document source;source.load({{"one","ATGAAATAA"},{"two","GCN~~~TGA"},{"mask","123456789",RowKind::Mask}});const auto file=dir.filePath("translation.kleio");{std::ofstream out(file.toStdString(),std::ios::binary);write_session(out,source);}MainWindow w;w.resize(1500,900);w.show();w.open_path(file);auto* view=dynamic_cast<AlignmentView*>(w.centralWidget());until(app,[&]{return view->isEnabled()&&w.current_document().rows().size()==3;},"Open");auto* count=w.findChild<QLabel*>("translationStopCount");action(w,"translationToggle")->setChecked(true);until(app,[&]{return count->text().startsWith("Stops:");},"Translation finishes");need(count->text().startsWith("Stops: 2"),"Whole translation stop count");const auto columns=view->cell_width();const auto state=w.current_document().state_id();action(w,"translationLetters")->setChecked(true);need(view->mode()==AlignmentView::Mode::View,"AA preview enters View");need(!w.findChild<QAction*>("translationThreeLetter"),"Three-letter control removed");need(view->cell_width()==columns&&w.current_document().state_id()==state,"Presentation preserves coordinates and document");view->select_fragment({{1},0,3});action(w,"copyFragment")->trigger();need(QApplication::clipboard()->text()=="ATG","AA preview copies underlying nucleotides");w.findChild<QToolButton*>("editToggle")->click();need(!action(w,"translationLetters")->isChecked()&&view->mode()==AlignmentView::Mode::Rewrite,"Edit returns nucleotide labels");
    auto* code=w.findChild<QComboBox*>("translationGeneticCode");need(code->count()==28&&code->itemData(27).toInt()==-1,"27 genetic codes plus custom-code command");need(!code->isEditable(),"Code names are not editable");code->setCurrentIndex(code->findData(2));until(app,[&]{return count->text().startsWith("Stops:");},"Changed code scan");need(count->text().startsWith("Stops: 1"),"Mitochondrial stop count");action(w,"&Undo")->trigger();until(app,[&]{return count->text().startsWith("Stops: 2");},"Code Undo recount");
    action(w,"translationToggle")->setChecked(false);view->select_fragment({{1},6,3});need(view->paste_residues("CAA"),"Edit while layer off");action(w,"translationToggle")->setChecked(true);until(app,[&]{return count->text().startsWith("Stops: 1");},"Re-enabling translates changed source");action(w,"&Undo")->trigger();until(app,[&]{return count->text().startsWith("Stops: 2");},"Undo source recount");
    view->select_ids({1,2});QTimer accept;accept.setInterval(1);bool accepted=false;QObject::connect(&accept,&QTimer::timeout,&w,[&]{if(auto* d=w.findChild<QDialog*>("translationDialog")){auto* buttons=d->findChild<QDialogButtonBox*>();if(buttons&&buttons->button(QDialogButtonBox::Ok)->isEnabled()){need(!d->findChild<QComboBox*>("translationDialogEncoding"),"Snapshot uses single-letter AA only");accepted=true;accept.stop();buttons->button(QDialogButtonBox::Ok)->click();}}});accept.start();action(w,"Add translated sequences…")->trigger();need(accepted&&w.current_document().rows().size()==5,"Preview adds snapshots");auto ids=view->selected_ids();need(ids.size()==2&&w.current_document().sequence(ids[0]).protein->spans.empty(),"Translated names selected and metadata");view->select_fragment({{ids[0]},1,1});need(view->fragment()->start==1&&view->fragment()->width==1,"AA is an independent ordinary cell");key(view,Qt::Key_Right);need(view->fragment()->start==2,"Right arrow reaches adjacent gap");key(view,Qt::Key_Left);need(view->fragment()->start==1,"Left arrow reaches AA");need(view->paste_residues("W"),"Protein rewrite");need(logical_protein(w.current_document().sequence(ids[0]))=="~W~~K~~*~","Ordinary single-AA editing");action(w,"&Undo")->trigger();need(logical_protein(w.current_document().sequence(ids[0]))=="~M~~K~~*~","Protein Undo");view->select_fragment({{ids[0]},1,1});action(w,"copyFragment")->trigger();view->set_mode(AlignmentView::Mode::Insert);action(w,"pasteFragment")->trigger();need(logical_protein(w.current_document().sequence(ids[0]))=="~MM~~K~~*~","Native protein fragment paste is ordinary single-symbol insertion");action(w,"&Undo")->trigger();view->set_mode(AlignmentView::Mode::Rewrite);action(w,"&Undo")->trigger();need(w.current_document().rows().size()==3,"Atomic snapshot Undo");action(w,"&Redo")->trigger();need(w.current_document().rows().size()==5,"Snapshot Redo");
    // The generated gaps and amino acids are ordinary independent cells.
    const auto protein=ids.front();view->set_mode(AlignmentView::Mode::Rewrite);view->select_fragment({{protein},0,1});need(view->fragment()->width==1&&*view->selected_residue_text()=="~","Select one surrounding gap");action(w,"cutFragment")->trigger();need(w.current_document().sequence(protein).symbols.text()=="M~~K~~*~","Cut removes gap alone");view->set_mode(AlignmentView::Mode::Insert);action(w,"pasteFragment")->trigger();need(w.current_document().sequence(protein).symbols.text()=="~M~~K~~*~","Paste restores one ordinary gap");action(w,"&Undo")->trigger();action(w,"&Undo")->trigger();
    view->select_fragment({{protein},1,1});view->slide_fragment(1);need(w.current_document().sequence(protein).symbols.text()=="~~M~K~~*~","Single AA slides independently");action(w,"&Undo")->trigger();
    view->select_ids({protein});view->select_fragment({{1},1,4});const auto beforeCopy=w.current_document().state_id();action(w,"copyFragment")->trigger();need(QApplication::clipboard()->text()=="TGAA"&&view->fragment()->start==1&&view->fragment()->width==4,"Ordinary copy does not expand");until(app,[&]{return action(w,"copyTranslatedFragment")->isEnabled();},"Current translation ready after protein edits");action(w,"copyTranslatedFragment")->trigger();need(QApplication::clipboard()->text()=="MK"&&view->fragment()->start==0&&view->fragment()->width==6,"Translated copy expands both incomplete codons");need(w.current_document().state_id()==beforeCopy&&view->selected_ids()==std::vector<SequenceId>{protein},"Copy expansion preserves sequence data and selected names");
    // Font-6 ruler and narrow toolbar keep editing controls usable.
    auto font=view->font();font.setPointSize(6);view->setFont(font);w.resize(850,650);app.processEvents();need(w.findChild<QToolButton*>("editToggle")->isVisible(),"Editing control remains visible");
    w.resize(1500,900);app.processEvents();w.grab().save("validation/translation0202.png");
    // Trace-open benchmark, cached pixels vs forced paints, no selection-triggered recount.
    std::istringstream ab1(test::synthetic_abif());Document large;auto trace=read_abif(ab1,"trace","fixture.ab1");std::vector<Record> rows{trace};for(int i=0;i<160;++i){std::string text;for(int j=0;j<1000;++j)text+="ATGGCNTAA";rows.push_back({"Row "+std::to_string(i),std::move(text)});}large.load(std::move(rows));const auto largePath=dir.filePath("large.kleio");{std::ofstream out(largePath.toStdString(),std::ios::binary);write_session(out,large);}MainWindow perf;perf.resize(3440,1440);perf.show();perf.open_path(largePath);auto* grid=dynamic_cast<AlignmentView*>(perf.centralWidget());until(app,[&]{return grid->isEnabled()&&perf.current_document().rows().size()==161;},"Large load");action(perf,"translationToggle")->setChecked(true);auto* stops=perf.findChild<QLabel*>("translationStopCount");until(app,[&]{return stops->text().startsWith("Stops:");},"Large translate");need(stops->text().startsWith("Stops: 160000"),"Large exact stops");for(bool aa:{false,true}){action(perf,"translationLetters")->setChecked(aa);app.processEvents();std::vector<double> times;for(int i=0;i<80;++i){QElapsedTimer time;time.start();grid->select_fragment({{1,2,3},static_cast<std::size_t>(i%20),static_cast<std::size_t>(i%30+1)});app.processEvents();times.push_back(time.nsecsElapsed()/1e6);need(stops->text().startsWith("Stops: 160000"),"Selection does not restart translation");if(i%10==0){const auto cached=pixels(*grid);grid->viewport()->repaint();need(cached==pixels(*grid),"Translation cached selection pixels");}grid->horizontalScrollBar()->setValue(i);app.processEvents();}std::sort(times.begin(),times.end());std::cout<<"3440x1440 translation "<<(aa?"AA":"NT")<<" + trace: selection median "<<times[40]<<" ms; p95 "<<times[76]<<" ms\n";}
    // Avoid save prompts for a validation window with deliberate edits.
    std::cout<<"PASS translation GUI: controls, code/undo, preview, protein editing, snapshots, narrow toolbar, font-6 ruler, wide trace-open cache and interaction\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
