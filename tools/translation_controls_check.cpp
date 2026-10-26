#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "core/session.hpp"
#include <QAction>
#include <QApplication>
#include <QBackingStore>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QPushButton>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTableView>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void until(QApplication& app,F ready,const char* message){QElapsedTimer t;t.start();while(!ready()&&t.elapsed()<10000){app.processEvents();QThread::msleep(1);}need(ready(),message);app.processEvents();}
QAction* action(MainWindow& w,const char* id){auto* a=w.findChild<QAction*>(id);need(a,id);return a;}
void check_context(AlignmentView& v,bool expected){
    bool found=false,enabled=false;QTimer::singleShot(0,&v,[&]{
        if(auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget())){for(auto* a:menu->actions()){need(!a->text().startsWith("Expand selection"),"No separate expansion action");if(a->text()=="Copy translated fragment"){found=true;enabled=a->isEnabled();}}menu->close();}
    });
    const QPoint point(v.name_area_width()+v.cell_width()/2,v.ruler_height()+v.row_height()/2);
    QContextMenuEvent e(QContextMenuEvent::Mouse,point,v.viewport()->mapToGlobal(point));QApplication::sendEvent(v.viewport(),&e);
    need(found&&enabled==expected,"Fragment context menu follows translation availability");
}
QColor background(AlignmentView& v,std::size_t row,std::size_t col){auto image=v.viewport()->grab().toImage();const auto dpr=image.devicePixelRatio();const auto x=v.name_area_width()+static_cast<int>(col)*v.cell_width()+1;const auto y=v.ruler_height()+static_cast<int>(row)*v.row_height()+1;return image.pixelColor(qRound(x*dpr),qRound(y*dpr));}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir dir;Document source;const std::string residues="ACDEFGHIKLMNPQRSTVWY";const std::string codons="GCTTGTGATGAATTTGGTCATATTAAACTTATGAATCCTCAACGTTCTACTGTTTGGTAT";
    std::vector<Record> rows{{"nucleotide",codons},{"protein",residues,RowKind::Protein}};
    for(int i=0;i<60;++i)rows.push_back({"gap_nt_"+std::to_string(i),std::string(codons.size(),'~')});
    for(int i=0;i<60;++i)rows.push_back({"gap_aa_"+std::to_string(i),std::string(residues.size(),'~'),RowKind::Protein});
    rows.push_back({"mask","12345",RowKind::Mask});source.load(std::move(rows));source.apply_groups({{1,"Study",0x336699}},{{1,1}});const auto path=dir.filePath("controls.kleio");{std::ofstream out(path.toStdString(),std::ios::binary);write_session(out,source);}
    MainWindow w;w.resize(1450,820);w.show();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());until(app,[&]{return v->isEnabled()&&w.current_document().rows().size()==123;},"Fixture load");
    auto* toggle=action(w,"translationToggle");auto* copy=action(w,"copyTranslatedFragment");v->select_fragment({{1},0,3});need(!copy->isEnabled()&&!v->can_copy_translation(),"Copy disabled on untranslated nucleotide alignment");check_context(*v,false);
    auto* code=w.findChild<QComboBox*>("translationGeneticCode");need(code&&!code->isEditable(),"Genetic code dropdown is read-only");need(code->count()==28&&code->itemText(code->count()-1)=="Custom code…","Custom code is last dropdown choice");
    need(code->itemText(code->findData(6)).size()<36&&code->itemData(code->findData(6),Qt::ToolTipRole).toString().contains("Dasycladacean"),"Short label retains full-name tooltip");
    for(auto* a:w.findChild<QToolBar*>("translationToolbar")->actions())need(a->text()!="Custom code…","No standalone custom-code button");
    auto* coding=w.findChild<QToolButton*>("codingColumns");need(coding&&coding->text()=="Set selection\ncoding","Coding button uses rectangular wrapped label");
    auto* count=w.findChild<QLabel*>("translationStopCount");toggle->setChecked(true);until(app,[&]{return count->text().startsWith("Stops:");},"Translation ready");
    need(copy->isEnabled(),"Copy enabled on complete translated nucleotide codon");check_context(*v,true);QApplication::clipboard()->setText("unchanged");copy->trigger();need(QApplication::clipboard()->text()=="A","Copy uses completed cached translation");
    v->select_fragment({{1},0,2});need(copy->isEnabled(),"Copy enabled on partial codon");copy->trigger();need(v->fragment()->start==0&&v->fragment()->width==3&&QApplication::clipboard()->text()=="A","Copy expands partial codon automatically");v->select_fragment({{1},2,2});copy->trigger();need(v->fragment()->start==0&&v->fragment()->width==6&&QApplication::clipboard()->text()=="AC","Copy expands both ends across two codons");v->select_fragment({{1,2},0,3});need(!copy->isEnabled(),"Copy disabled on multi-row selection");v->select_fragment({{2},0,3});need(!copy->isEnabled(),"Copy disabled on protein row");v->select_fragment({{123},0,3});need(!copy->isEnabled(),"Copy disabled on mask");
    // All standard residues remain colored despite a >98% gap fraction.
    v->set_mode(AlignmentView::Mode::View);v->unselect_all();QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(v,&escape);v->set_residue_display(ResidueDisplay::Backgrounds);
    need(!w.findChild<QAction*>("translationThreeLetter"),"No three-letter display action");
    const auto spacers=w.findChildren<QWidget*>("translationControlSpacing");need(spacers.size()>=5,"Translation controls have spacing");
    for(bool letters:{false,true}){action(w,"translationLetters")->setChecked(letters);app.processEvents();for(std::size_t i=0;i<residues.size();++i){const auto fill=QColor::fromRgb(amino_acid_color(residues[i]));need(background(*v,0,i*3)==fill,"Every translated amino acid has its family shade");need(background(*v,1,i)==fill,"Every imported amino acid has its family shade");need(color_contrast(black_or_white(fill),fill)>=4.5&&color_contrast(on_white(fill),Qt::white)>=4.5,"AA lettering remains readable on dark and light shades");}}
    // Command cancellation must restore the previous code without edits.
    const auto state=w.current_document().state_id();bool cancelled=false;QTimer cancel;cancel.setInterval(1);QObject::connect(&cancel,&QTimer::timeout,&w,[&]{if(auto* d=w.findChild<QDialog*>("customGeneticCodeDialog")){cancelled=true;cancel.stop();d->reject();}});cancel.start();code->setCurrentIndex(code->count()-1);until(app,[&]{return cancelled;},"Custom-code dialog opens from dropdown");need(code->currentData().toUInt()==1&&w.current_document().state_id()==state,"Cancel restores current code and document");
    // Accept creates a reusable custom entry and one undoable semantic edit.
    bool created=false;QTimer accept;accept.setInterval(1);QObject::connect(&accept,&QTimer::timeout,&w,[&]{if(auto* d=w.findChild<QDialog*>("customGeneticCodeDialog")){d->findChild<QLineEdit*>("customGeneticCodeName")->setText("Test custom");created=true;accept.stop();d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();}});accept.start();code->setCurrentIndex(code->count()-1);until(app,[&]{return created&&code->currentData().toInt()==0;},"Custom code accepted");need(code->itemData(code->count()-1).toInt()==-1&&code->itemText(code->currentIndex()).contains("Test custom"),"Saved custom code precedes final command");
    // The QAction has the original display name rather than an object ID.
    for(auto* a:w.findChildren<QAction*>())if(a->text()=="&Undo"){a->trigger();break;}
    until(app,[&]{return code->currentData().toInt()==1;},"Custom-code undo restores standard code");
    // A custom code created inside a snapshot preview stays local to that
    // translation; accepting snapshots must not change alignment defaults.
    v->select_ids({1});int stage=0;bool previewDone=false;QTimer preview;preview.setInterval(1);
    QObject::connect(&preview,&QTimer::timeout,&w,[&]{
        if(auto* custom=w.findChild<QDialog*>("customGeneticCodeDialog");custom&&stage==1){custom->findChild<QLineEdit*>("customGeneticCodeName")->setText("Preview-only code");stage=2;custom->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();return;}
        if(auto* dialog=w.findChild<QDialog*>("translationDialog")){
            auto* choices=dialog->findChild<QComboBox*>("translationDialogCode");auto* buttons=dialog->findChild<QDialogButtonBox*>();
            if(stage==0){need(!choices->isEditable(),"Snapshot code choices are read-only");stage=1;choices->setCurrentIndex(choices->count()-1);return;}
            if(stage==2&&choices->currentData().toInt()==0&&buttons->button(QDialogButtonBox::Ok)->isEnabled()){
                need(w.current_document().translation_settings().code==1,"Preview custom code does not change document defaults");previewDone=true;preview.stop();buttons->button(QDialogButtonBox::Ok)->click();
            }
        }
    });preview.start();for(auto* a:w.findChildren<QAction*>())if(a->text()=="Add translated sequences…"){a->trigger();break;}
    need(previewDone&&w.current_document().rows().size()==124,"Snapshot custom-code workflow completes");
    const auto& added=w.current_document().sequence(v->selected_ids().front());need(added.protein&&added.protein->settings.code==0&&added.protein->settings.custom->name=="Preview-only code","Snapshot retains its own custom code");
    need(w.current_document().translation_settings().code==1,"Alignment code remains standard after adding snapshot");
    for(auto* a:w.findChildren<QAction*>())if(a->text()=="&Undo"){a->trigger();break;}
    v->select_fragment({{1},0,3});toggle->setChecked(false);need(!copy->isEnabled(),"Layer off immediately disables copy");
    toggle->setChecked(true);until(app,[&]{return copy->isEnabled();},"Layer on restores eligible copy");v->select_fragment({{1},0,3});w.grab().save("validation/translation0202-controls.png");
    // Split codons expand across noncoding cells, but a purely noncoding
    // selection is not a translated amino-acid selection.
    Document intron;intron.load({{"split","A~TG"}});auto phases=annotate_coding({},0,1,1);phases=annotate_coding(phases,1,1,0);phases=annotate_coding(phases,2,2,2);intron.set_translation_settings(phases);TranslationScan split(intron,phases);while(!split.step(intron)){}AlignmentView splitView;splitView.set_document(&intron);splitView.set_translation(std::make_shared<TranslationResult>(split.result()),true,true,true);splitView.select_fragment({{1},1,1});need(!splitView.can_copy_translation(),"Noncoding-only selection is unavailable");splitView.select_fragment({{1},2,1});const auto expanded=splitView.expanded_translation_fragment();need(expanded&&expanded->start==0&&expanded->width==4,"Split codon copy expansion covers all coding cells");
    // Annotation mask workflows and menu placement.
    auto menu=[&](const char* title){for(auto* a:w.menuBar()->actions())if(a->text().remove('&')==title)return a->menu();return static_cast<QMenu*>(nullptr);};
    auto* viewMenu=menu("View");need(viewMenu&&viewMenu->actions().back()==action(w,"alignmentFont"),"Font command is last");const auto viewActions=viewMenu->actions();need(viewActions[viewActions.size()-2]->isSeparator(),"Font has its own section");bool motif=false;for(auto* a:viewActions)motif|=a->text()=="Motif search";need(motif,"View uses Motif search label");
    const auto seqActions=menu("Sequence")->actions();need(seqActions[seqActions.size()-2]->text().remove('&')=="Select"&&seqActions.back()->text().remove('&')=="Sort","Select and Sort are last sequence commands");
    action(w,"generateTranslationMask")->trigger();const auto generated=w.current_document().rows().back();need(generated.kind==RowKind::Mask&&generated.name=="translation_mask"&&generated.symbols.text()==translation_mask(w.current_document().translation_settings(),w.current_document().columns()),"Generate creates exact explicit mask");
    for(auto* a:w.findChildren<QAction*>())if(a->text()=="&Undo"){a->trigger();break;}need(w.current_document().rows().size()==123,"Generate mask undo removes one row");
    const auto oldSettings=w.current_document().translation_settings();bool maskApplied=false;QTimer maskDialog;maskDialog.setInterval(1);QObject::connect(&maskDialog,&QTimer::timeout,&w,[&]{if(auto* dialog=w.findChild<QDialog*>("annotationMaskDialog")){maskDialog.stop();auto* source=dialog->findChild<QComboBox*>("annotationMaskSource");source->setCurrentIndex(0);auto* text=dialog->findChild<QPlainTextEdit*>("annotationMaskText");text->setPlainText(QString(61,'1'));need(!dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled(),"Overlong mask disables Apply");text->setPlainText("123123###123\n123");auto* ok=dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok);need(ok->isEnabled(),"Short pasted mask accepted");maskApplied=true;ok->click();}});maskDialog.start();action(w,"annotateFromMask")->trigger();need(maskApplied&&translation_mask(w.current_document().translation_settings(),60)=="123123###123123"+std::string(45,'#'),"Pasted mask replaces annotation, ignores line breaks and pads noncoding");
    until(app,[&]{return count->text().contains("Custom annotation");},"Custom annotation label");for(auto* a:w.findChildren<QAction*>())if(a->text()=="&Undo"){a->trigger();break;}need(w.current_document().translation_settings()==oldSettings,"Mask annotation undo");
    v->annotate_mask_requested(123);need(translation_mask(w.current_document().translation_settings(),60)=="123"+std::string(57,'#'),"Mask context callback applies selected existing mask");for(auto* a:w.findChildren<QAction*>())if(a->text()=="&Undo"){a->trigger();break;}
    bool selectedMask=false;QTimer chooseMask;chooseMask.setInterval(1);QObject::connect(&chooseMask,&QTimer::timeout,&w,[&]{if(auto* dialog=w.findChild<QDialog*>("annotationMaskDialog")){chooseMask.stop();auto* choices=dialog->findChild<QComboBox*>("annotationMaskSource");choices->setCurrentIndex(choices->findData(QVariant::fromValue<qulonglong>(123)));need(dialog->findChild<QPlainTextEdit*>("annotationMaskText")->isReadOnly(),"Existing mask shown read-only");selectedMask=true;dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();}});chooseMask.start();action(w,"annotateFromMask")->trigger();need(selectedMask&&coding_phase(w.current_document().translation_settings(),4)==0,"Existing mask selection applied");for(auto* a:w.findChildren<QAction*>())if(a->text()=="&Undo"){a->trigger();break;}
    v->select_ids({1});bool renamed=false;QTimer renameTimer;renameTimer.setInterval(1);QObject::connect(&renameTimer,&QTimer::timeout,&w,[&]{if(auto* dialog=w.findChild<QDialog*>("batchRenameDialog")){renameTimer.stop();need(dialog->findChild<QLineEdit*>("renameSeparator")->isVisible(),"Group rename separator shown");dialog->findChild<QLineEdit*>("renameSeparator")->setText(" - ");dialog->findChild<QPushButton*>("renamePreviewButton")->click();const auto* model=dialog->findChild<QTableView*>("renamePreview")->model();need(model->rowCount()==1&&model->data(model->index(0,1),Qt::DisplayRole).toString()=="nucleotide - Study","Group suffix preview uses selected names");renamed=true;dialog->findChild<QPushButton*>("renameApplyButton")->click();}});renameTimer.start();for(auto* a:w.findChildren<QAction*>())if(a->text()=="Append group name…"){a->trigger();break;}need(renamed&&w.current_document().sequence(1).name=="nucleotide - Study","Group suffix GUI apply");for(auto* a:w.findChildren<QAction*>())if(a->text()=="&Undo"){a->trigger();break;}need(w.current_document().sequence(1).name=="nucleotide","Group suffix GUI undo");
    bool contextFound=false;v->select_ids({123});v->verticalScrollBar()->setValue(122);QTimer::singleShot(0,v,[&]{auto* popup=qobject_cast<QMenu*>(QApplication::activePopupWidget());need(popup,"Name context menu");const auto actions=popup->actions();for(qsizetype i=0;i<actions.size();++i)if(actions[i]->text()=="Annotate using this mask"){contextFound=actions[i]->isEnabled();need(i>1&&actions[i-1]->text()=="Add translated sequences…"&&actions[i-2]->isSeparator()&&i+1<actions.size()&&actions[i+1]->isSeparator(),"Translation name commands have own subsection");}popup->close();});const QPoint namePoint(10,v->ruler_height()+(122-v->verticalScrollBar()->value())*v->row_height()+v->row_height()/2);QContextMenuEvent nameContext(QContextMenuEvent::Mouse,namePoint,v->viewport()->mapToGlobal(namePoint));QApplication::sendEvent(v->viewport(),&nameContext);need(contextFound,"Mask context command enabled");
    Document tail;tail.load({{"tail","ATGGC"}});TranslationScan tailScan(tail,{});while(!tailScan.step(tail)){}AlignmentView tailView;tailView.set_document(&tail);tailView.set_translation(std::make_shared<TranslationResult>(tailScan.result()),true,true,true);tailView.select_fragment({{1},4,1});const auto tailSelection=tailView.expanded_translation_fragment();need(tailSelection&&tailSelection->start==3&&tailSelection->width==2,"Tail translation can be copied and expands only to actual columns");
    w.grab().save("validation/translation0203-controls.png");
    std::cout<<"PASS 0.20.3: translation controls, colors, automatic copy expansion, annotation masks and undo, context/menu organization and trailing codons\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
