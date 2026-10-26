#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/alignment_overview.hpp"
#include "core/fasta.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "core/statistics.hpp"
#include "../tests/abif_fixture.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QStatusBar>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class F>void wait(F done){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<20000){QApplication::processEvents();QThread::msleep(1);}require(done(),"Async timeout");QApplication::processEvents();}
QAction* action(MainWindow& w,const QString& text){for(auto* a:w.findChildren<QAction*>())if(a->objectName()==text||a->text().section('\t',0,0)==text)return a;throw std::runtime_error("Missing action: "+text.toStdString());}
template<class F>void dialog(MainWindow& w,const QString& name,F f){QString failure;QTimer::singleShot(0,[&]{auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());try{require(d,"Expected modal dialog");f(d);}catch(const std::exception& e){failure=e.what();if(d)d->reject();}});action(w,name)->trigger();if(!failure.isEmpty())throw std::runtime_error(failure.toStdString());}
void save(MainWindow& w,const QString& path,const QString& command="&Save"){
    dialog(w,command,[&](QDialog* d){auto* file=qobject_cast<QFileDialog*>(d);require(file,"Save filename dialog");file->selectFile(path);QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);});
    auto* view=dynamic_cast<AlignmentView*>(w.centralWidget());wait([&]{return view->isEnabled();});
}
}
int main(int argc,char** argv){QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);try{
    QTemporaryDir dir;const auto fasta=dir.filePath("mixed.faa");{std::ofstream out(fasta.toStdString());out<<">overlap\nACGT\n>protein\nMeF*\n>mask\n120A\n";}
    MainWindow w;w.resize(1280,850);w.show();auto* view=dynamic_cast<AlignmentView*>(w.centralWidget());
    require(action(w,"fastaImportNucleotide")->isChecked(),"Legacy import remains default");w.open_path(fasta);wait([&]{return view->isEnabled();});
    require(w.current_document().sequence(1).kind==RowKind::Nucleotide&&w.current_document().sequence(2).kind==RowKind::Mask,"Default .faa extension does not silently reclassify existing mask workflow");
    action(w,"fastaImportProtein")->trigger();w.open_path(fasta);wait([&]{return view->isEnabled();});
    require(w.current_document().sequence(1).kind==RowKind::Protein&&w.current_document().sequence(2).kind==RowKind::Protein&&w.current_document().sequence(3).kind==RowKind::Mask,"Declared protein file import preserves overlapping alphabet and mixed masks");
    for(int style=0;style<3;++style){view->set_residue_display(static_cast<ResidueDisplay>(style));view->set_identity_dots(true);QApplication::processEvents();const auto pixel=view->viewport()->grab(QRect(view->name_area_width(),view->ruler_height(),view->cell_width(),view->row_height())).toImage();
        require(pixel.pixelColor(pixel.width()/2,pixel.height()-4)==QColor(Qt::white),"Protein rows never get nucleotide background colors");}
    view->set_identity_dots(false);view->set_residue_display(ResidueDisplay::Letters);view->select_ids({1});
    dialog(w,"setSequenceType",[&](QDialog* d){require(d->objectName()=="sequenceTypeDialog","Type dialog");d->findChild<QComboBox*>("sequenceTypeChoice")->setCurrentIndex(0);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();});
    require(w.current_document().sequence(1).kind==RowKind::Nucleotide&&w.current_document().sequence(1).symbols.text()=="ACGT","Type assignment preserves actual residues");view->select_ids({1,2});action(w,"Align selected sequences to templates…")->trigger();require(w.statusBar()->currentMessage().contains("Protein targets"),"Mixed nucleotide/protein targets rejected before MAFFT setup");action(w,"Align whole alignment…")->trigger();require(w.statusBar()->currentMessage().contains("cannot include protein"),"Whole mixed alignment rejected before launching MAFFT");action(w,"&Undo")->trigger();require(w.current_document().sequence(1).kind==RowKind::Protein,"Type undo works through actual menu");action(w,"&Redo")->trigger();require(w.current_document().sequence(1).kind==RowKind::Nucleotide,"Type redo");action(w,"&Undo")->trigger();
    view->select_ids({1,2});const auto state=w.current_document().state_id();dialog(w,"setSequenceType",[&](QDialog* d){d->findChild<QComboBox*>("sequenceTypeChoice")->setCurrentIndex(0);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();require(!d->findChild<QLabel*>("sequenceTypeError")->text().isEmpty(),"Incompatible batch reports inline");d->reject();});require(w.current_document().state_id()==state&&w.current_document().sequence(1).kind==RowKind::Protein,"Failed batch leaves all selected types unchanged");
    view->select_ids({2});action(w,"Copy selected sequences")->trigger();require(QApplication::clipboard()->text()==">protein\nMeF*\n","Protein FASTA copy remains canonical and unwrapped");action(w,"Add from &clipboard")->trigger();require(w.current_document().rows().size()==4&&w.current_document().rows().back().kind==RowKind::Protein,"Protein clipboard import uses explicit choice");
    dialog(w,"Add &new sequence…",[&](QDialog* d){d->findChild<QLineEdit*>("newSequenceName")->setText("new protein");d->findChild<QPlainTextEdit*>("newSequenceSymbols")->setPlainText("WQX*");require(d->findChild<QComboBox*>("newSequenceType")->currentIndex()==1,"New sequence type follows current import choice");d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});require(w.current_document().rows().back().kind==RowKind::Protein,"Explicit new-protein dialog");
    const auto id=w.current_document().rows().back().id;view->select_ids({});view->set_mode(AlignmentView::Mode::Insert);view->select_fragment({{id},0,1});QApplication::clipboard()->setText("EF");action(w,"Paste fragment")->trigger();require(w.current_document().sequence(id).symbols.text()=="WEFQX*","Protein raw insertion accepts amino acids");action(w,"&Undo")->trigger();
    const auto native=dir.filePath("typed.kleio");save(w,native);require(QFileInfo::exists(native),"Normal Save prefers native session for explicit types");{std::ifstream input(native.toStdString(),std::ios::binary);const auto session=read_session(input);require(session.formatVersion==3&&session.document.sequence(1).kind==RowKind::Protein&&session.document.sequence(3).kind==RowKind::Mask,"Actual GUI save retains native types");}
    const auto exported=dir.filePath("typed.fasta");save(w,exported,"Export FASTA…");{std::ifstream input(exported.toStdString());const auto rows=read_fasta(input,{},FastaImport::ProteinAndMasks);require(rows.size()==5&&rows[1].symbols=="MeF*","Explicit sequence-only export retains canonical symbols");}
    auto* overview=dynamic_cast<AlignmentOverview*>(w.findChild<QWidget*>("alignmentOverview"));wait([&]{return overview&&overview->ready();});
    if(argc>1)require(w.grab().save(QString::fromLocal8Bit(argv[1])),"Mixed-type screenshot");w.close();
    MainWindow restored;restored.show();action(restored,"fastaImportMask")->trigger();restored.open_path(native);auto* rv=dynamic_cast<AlignmentView*>(restored.centralWidget());wait([&]{return rv->isEnabled();});require(restored.current_document().sequence(1).kind==RowKind::Protein,"Session types override FASTA import preference");restored.close();
    const auto tracePath=dir.filePath("trace.ab1");{std::ofstream out(tracePath.toStdString(),std::ios::binary);out<<test::synthetic_abif();}
    MainWindow trace;trace.show();action(trace,"fastaImportProtein")->trigger();trace.open_path(tracePath);auto* tv=dynamic_cast<AlignmentView*>(trace.centralWidget());wait([&]{return tv->isEnabled();});require(trace.current_document().sequence(1).kind==RowKind::Nucleotide&&trace.current_document().sequence(1).trace,"AB1 remains nucleotide under protein import choice");tv->select_ids({1});
    dialog(trace,"setSequenceType",[&](QDialog* d){d->findChild<QComboBox*>("sequenceTypeChoice")->setCurrentIndex(1);d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();require(d->findChild<QLabel*>("sequenceTypeError")->text().contains("Trace-linked"),"Trace type protection explains rejection");d->reject();});require(!trace.current_document().can_undo(),"Rejected trace conversion changes no history");trace.close();
    std::cout<<"PASS 0.16: explicit FASTA/file/clipboard/new-sequence types, neutral protein display, atomic type controls/undo/redo, strict editing, native-save default/roundtrip/export, saved-type precedence and AB1 protection\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
