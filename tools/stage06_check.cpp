#include "gui/alignment_view.hpp"
#include "gui/main_window.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void require(bool condition,const char* message) { if(!condition)throw std::runtime_error(message); }
void key(QWidget& widget,int code,const QString& text={},Qt::KeyboardModifiers mods=Qt::NoModifier) {
    widget.window()->activateWindow();widget.setFocus();if(code!=Qt::Key_F)QApplication::processEvents();
    QKeyEvent press(QEvent::KeyPress,code,mods,text);QApplication::sendEvent(&widget,&press);
    QKeyEvent release(QEvent::KeyRelease,code,mods,text);QApplication::sendEvent(&widget,&release);QApplication::processEvents();
}
void mouse(AlignmentView& v,QEvent::Type type,QPointF point,Qt::MouseButton button,Qt::MouseButtons buttons,Qt::KeyboardModifiers mods=Qt::NoModifier) {
    QMouseEvent event(type,point,QPointF(v.viewport()->mapToGlobal(point.toPoint())),button,buttons,mods);
    QApplication::sendEvent(v.viewport(),&event);QApplication::processEvents();
}
QPointF cell(AlignmentView& v,int row,int col) {return {v.name_area_width()+(col+.5)*v.cell_width(),v.ruler_height()+(row+.5)*v.row_height()};}
void click(AlignmentView& v,QPointF p,Qt::KeyboardModifiers mods=Qt::NoModifier) {
    mouse(v,QEvent::MouseButtonPress,p,Qt::LeftButton,Qt::LeftButton,mods);mouse(v,QEvent::MouseButtonRelease,p,Qt::LeftButton,Qt::NoButton,mods);
}
void drag(AlignmentView& v,QPointF a,QPointF b) {
    mouse(v,QEvent::MouseButtonPress,a,Qt::LeftButton,Qt::LeftButton);mouse(v,QEvent::MouseMove,b,Qt::NoButton,Qt::LeftButton);mouse(v,QEvent::MouseButtonRelease,b,Qt::LeftButton,Qt::NoButton);
}
QColor background(AlignmentView& v,int row,int col) {
    const auto image=v.viewport()->grab().toImage();const auto dpr=image.devicePixelRatio();
    return image.pixelColor(qRound((v.name_area_width()+col*v.cell_width()+v.cell_width()/2)*dpr),qRound((v.ruler_height()+(row+1)*v.row_height()-3)*dpr));
}
QAction* action(QWidget& w,const QString& name) {
    for(auto* a:w.findChildren<QAction*>()) {auto text=a->text();text.remove('&');if(text==name)return a;}
    throw std::runtime_error("Missing action: "+name.toStdString());
}
template<class Predicate> void wait(Predicate done,const char* message) {
    QElapsedTimer timer;timer.start();while(!done()&&timer.elapsed()<10000)QApplication::processEvents();require(done(),message);
}
void selection_checks() {
    Document d;d.load({{"one","AA~-CGTN~~~TT"},{"two","AC--CG?~~~~TT"},{"mask","12~-34??~~xx"}});
    AlignmentView v;v.resize(800,350);v.set_document(&d);v.show();QApplication::processEvents();
    for(auto mode:{AlignmentView::Mode::View,AlignmentView::Mode::Insert,AlignmentView::Mode::Rewrite}) {
        v.set_mode(mode);key(v,Qt::Key_Escape);
        drag(v,cell(v,0,4),cell(v,0,7));require(v.fragment()&&v.fragment()->width==4&&v.fragment()->ids.size()==1,"Single-row drag in every mode");
        require(v.selected_residue_text()==QString("CGTN"),"Substring text in every mode");
        click(v,cell(v,2,8),Qt::ShiftModifier);require(v.fragment()->ids.size()==3&&v.fragment()->start==4&&v.fragment()->width==5,"Shift-click rectangle in every mode");
        require(!v.selected_residue_text(),"Multiple-row block is not a text substring");
        key(v,Qt::Key_Escape);
        const auto a=QPointF(v.name_area_width()+2.5*v.cell_width(),v.ruler_height()/2.),b=QPointF(v.name_area_width()+5.5*v.cell_width(),v.ruler_height()/2.);
        drag(v,a,b);require(v.fragment()->ids.size()==3&&v.fragment()->start==2&&v.fragment()->width==4,"Ruler drag in every mode");
        click(v,{v.name_area_width()+7.5*v.cell_width(),v.ruler_height()/2.},Qt::ShiftModifier);
        require(v.fragment()->start==2&&v.fragment()->width==6,"Shift-click ruler extends from initial column");
        key(v,Qt::Key_Escape);
        mouse(v,QEvent::MouseButtonDblClick,cell(v,0,5),Qt::LeftButton,Qt::LeftButton);
        require(v.fragment()->start==4&&v.fragment()->width==4,"Double-click selects continuous residues");
        mouse(v,QEvent::MouseButtonDblClick,cell(v,0,2),Qt::LeftButton,Qt::LeftButton);
        require(v.fragment()->start==2&&v.fragment()->width==2,"Double-click combines locked and open gaps");
        v.select_all();key(v,Qt::Key_Escape);require(v.selected_ids().empty()&&!v.fragment(),"Escape clears selected names in every mode");
    }
    v.set_mode(AlignmentView::Mode::Insert);click(v,cell(v,0,4));
    require(background(v,0,4)==QColor(225,235,251),"Insert single-cell caret is blue");
    drag(v,cell(v,0,4),cell(v,0,6));require(background(v,0,5)==QColor(250,235,191),"Insert stretch is yellow");
    const auto state=d.state_id();key(v,Qt::Key_A,"A");require(d.state_id()==state&&!v.paste_residues("GG"),"Ambiguous Insert caret blocks typing and paste");
    v.set_mode(AlignmentView::Mode::Rewrite);require(v.fragment()->width==3&&background(v,0,5)==QColor(225,235,251),"Mode change retains selection and Overwrite stretch is blue");
    drag(v,cell(v,0,4),cell(v,1,6));require(background(v,0,5)==QColor(250,235,191),"Overwrite multiple-row block is yellow");
    key(v,Qt::Key_A,"A");require(d.state_id()==state&&!v.paste_residues("GG"),"Overwrite block rejects ambiguous input");
    v.set_mode(AlignmentView::Mode::View);key(v,Qt::Key_Delete);key(v,Qt::Key_A,"A");require(d.state_id()==state,"View selection cannot type or delete");
}
void edit_checks() {
    Document d;d.load({{"one","ACGTTA"},{"two","AC"},{"mask","12-3~4"}});AlignmentView v;v.resize(800,350);v.set_document(&d);v.show();
    v.set_mode(AlignmentView::Mode::Insert);click(v,cell(v,0,1));key(v,Qt::Key_T,"t");require(d.sequence(1).symbols.text()=="ACtGTTA"&&v.fragment()->start==2,"Insert typed symbol after selected cell and select inserted cell");
    require(v.paste_residues("G G\nT"),"Insert sequence-like paste");require(d.sequence(1).symbols.text()=="ACtGGTGTTA"&&v.fragment()->start==5,"Paste inserts after caret and selects last inserted symbol");
    d.undo();d.undo();v.refresh();require(d.sequence(1).symbols.text()=="ACGTTA","Undo insertion and paste");
    v.set_mode(AlignmentView::Mode::Rewrite);v.select_fragment({{1},1,3});key(v,Qt::Key_A,"a");
    require(d.sequence(1).symbols.text()=="AaTA"&&v.fragment()->start==2,"One symbol replaces a selected stretch and advances");
    key(v,Qt::Key_C,"C");require(d.sequence(1).symbols.text()=="AaCA"&&v.fragment()->start==3,"Continued typing overwrites the next symbol");
    d.undo();d.undo();v.refresh();v.select_fragment({{1},1,2});require(v.paste_residues("ttGG"),"Overwrite paste selection");
    require(d.sequence(1).symbols.text()=="AttGGTTA"&&v.fragment()->start==5,"Longer paste replaces exact selection without overwriting suffix");
    const auto version=d.state_id();require(!v.paste_residues("ACZ"),"Invalid nucleotide paste rejected atomically");require(d.state_id()==version,"Invalid paste has no edit");
    d.undo();v.refresh();v.select_fragment({{1},2,4});require(v.paste_residues("N"),"Shorter Overwrite paste");require(d.sequence(1).symbols.text()=="ACN"&&!v.fragment(),"Replacement at row end leaves append caret");
    require(v.paste_residues("TT"),"Overwrite at row end can append");require(d.sequence(1).symbols.text()=="ACNTT","Append at unambiguous end caret");
    d=Document{};d.load({{"one","AC-G~T"},{"two","AC"},{"mask","12-3~4"}});v.set_document(&d);
    for(auto mode:{AlignmentView::Mode::Insert,AlignmentView::Mode::Rewrite}) {
        v.set_mode(mode);v.select_fragment({{1,2,3},1,4});key(v,Qt::Key_Delete);
        require(d.sequence(1).symbols.text()=="AT"&&d.sequence(2).symbols.text()=="A"&&d.sequence(3).symbols.text()=="14","Block deletion clamps ragged rows and edits masks");
        d.undo();v.refresh();require(d.sequence(1).symbols.text()=="AC-G~T"&&d.sequence(3).symbols.text()=="12-3~4"&&v.fragment()->ids.size()==3,"One undo restores block and exact selected range");
        key(v,Qt::Key_Backspace);require(d.sequence(1).symbols.text()=="AT","Backspace deletes selected block");d.undo();v.refresh();
        v.select_fragment({{1},1,3});key(v,Qt::Key_Delete);require(d.sequence(1).symbols.text()=="A~T"&&v.fragment()->start==1,"Delete one-row stretch, caret at next symbol");d.undo();v.refresh();
    }
    v.set_mode(AlignmentView::Mode::Rewrite);v.select_fragment({{3},1,3});require(v.paste_residues("x9"),"Mask Overwrite accepts non-nucleotide printable symbols");require(d.sequence(3).symbols.text()=="1x9~4","Mask substring replacement");
}
void window_checks(const QString& screenshot) {
    QTemporaryDir dir;require(dir.isValid(),"Temporary directory");const auto path=dir.filePath("statistics.fasta");QFile file(path);require(file.open(QIODevice::WriteOnly),"FASTA fixture");
    file.write(">one\nAAAN~RUCB\n>two\nAAR N-YUYD\n>three\nC AR~~Yt?V\n>four\nCCN-.Rt?H\n>mask\n01234567890123456789\n");file.close();
    MainWindow w;w.show();w.activateWindow();QApplication::processEvents();w.open_path(path);auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());require(v,"Main viewport");wait([&]{return v->isEnabled();},"Async file load");
    v->select_all();const auto ids=v->selected_ids();require(ids.size()==5,"Fixture rows");v->unselect_all();
    for(auto mode:{AlignmentView::Mode::View,AlignmentView::Mode::Insert,AlignmentView::Mode::Rewrite}) {
        v->set_mode(mode);v->select_fragment({{ids[0]},0,3});QApplication::clipboard()->setText("sentinel");v->setFocus();key(*v,Qt::Key_C,{},Qt::ControlModifier);
        require(QApplication::clipboard()->text()=="AAA","Actual Ctrl+C copies substring in every mode");
        v->select_fragment({{ids[0],ids[1]},0,3});QApplication::clipboard()->setText("sentinel");key(*v,Qt::Key_C,{},Qt::ControlModifier);require(QApplication::clipboard()->text()=="sentinel","Ctrl+C block leaves clipboard unchanged");
    }
    for(auto* menu:w.findChildren<QMenu*>())require(menu->title()!="Select sequences","Edit selection submenu removed");
    require(action(w,"Names matching regular expression…")->shortcut()==QKeySequence::Find,"Ctrl+F bound to name regex selection");
    bool found=false;QTimer::singleShot(0,[&]{if(auto* dialog=qobject_cast<QInputDialog*>(QApplication::activeModalWidget())){found=true;dialog->setTextValue("^one$");dialog->accept();}});
    v->setFocus();key(*v,Qt::Key_F,{},Qt::ControlModifier);wait([&]{return v->isEnabled();},"Async regex selection finishes");require(found&&v->selected_ids().size()==1&&v->selected_ids()[0]==ids[0],"Actual Ctrl+F opens and applies regex dialog");
    key(*v,Qt::Key_Escape);require(v->selected_ids().empty(),"Escape clears regex result");
    v->set_mode(AlignmentView::Mode::Rewrite);v->select_fragment({{ids[0]},1,3});QApplication::clipboard()->setText("CG");
    key(*v,Qt::Key_V,{},Qt::ControlModifier);require(v->fragment()&&v->fragment()->start==3,"Actual Ctrl+V Overwrite advances caret");
    v->select_fragment({{ids[0]},0,9});require(v->selected_residue_text()==QString("ACG~RUCB"),"Actual Ctrl+V replaces the selected substring");
    action(w,"Undo")->trigger();v->set_mode(AlignmentView::Mode::Insert);v->select_fragment({{ids[0]},1,3});
    QApplication::clipboard()->setText("AA");key(*v,Qt::Key_V,{},Qt::ControlModifier);require(!w.windowTitle().contains('*'),"Actual Ctrl+V refuses an ambiguous Insert caret");
    bool tips=false;QTimer::singleShot(0,[&]{if(auto* dialog=QApplication::activeModalWidget()){
        tips=true;for(const char* name:{"mafftLauncher","mafftAlgorithm","mafftThreads","mafftDirection","mafftIterations","mafftRetree","mafftGapOpen","mafftOffset","mafftKimura","mafftKappa","mafftComposition","mafftMemory","mafftDeterministic"}){auto* widget=dialog->findChild<QWidget*>(QString::fromLatin1(name));tips=tips&&widget&&!widget->toolTip().isEmpty();}
        if(auto* settings=qobject_cast<QDialog*>(dialog))settings->reject();
    }});action(w,"Align whole alignment…")->trigger();require(tips,"Every MAFFT parameter has hover help");
    action(w,"Alignment statistics…")->trigger();auto* stats=w.findChild<QDialog*>("alignmentStatistics");require(stats,"Statistics window appears");
    auto* copy=stats->findChild<QPushButton*>("statisticsCopy");wait([&]{return copy->isEnabled();},"Async statistics complete");
    const int expected[]={4,4,9,8,1,3,5,1,1};
    for(int i=0;i<9;++i)require(stats->findChild<QLabel*>(QString("statisticsValue%1").arg(i))->text()==QString::number(expected[i]),"Displayed IQ-TREE statistics");
    copy->click();const auto text=QApplication::clipboard()->text();require(text.contains("Total sequences: 4")&&text.contains("Distinct patterns: 8")&&text.contains("N-only sites: 1")&&!text.contains('\n'),"Statistics Copy is a single paste-ready string");
    if(!screenshot.isEmpty())stats->grab().save(screenshot);stats->reject();QApplication::processEvents();
    action(w,"Alignment statistics…")->trigger();stats=w.findChild<QDialog*>("alignmentStatistics");require(stats,"Reopened statistics");stats->reject();wait([&]{return v->isEnabled();},"Closing statistics cancels safely and releases source");
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try { selection_checks();edit_checks();window_checks(argc>1?QString::fromLocal8Bit(argv[1]):QString{});
        std::cout<<"PASS stage 0.6: all-mode selection/colors, typing/paste/replacement/deletion/undo, double-click, Esc/Ctrl+F/Ctrl+C, MAFFT help, async statistics/copy/cancel\n";
    } catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
