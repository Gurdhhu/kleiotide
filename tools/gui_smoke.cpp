#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include "gui/alignment_overview.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFontComboBox>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QSpinBox>
#include <QComboBox>
#include <QElapsedTimer>
#include <QFile>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>

using namespace kleiotide;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
QAction* find_action(const QList<QAction*>& actions, const QString& text) {
    for (auto* item : actions) {
        auto name = item->text(); name.remove('&');
        if (name == text) return item;
        if (item->menu()) if (auto* nested = find_action(item->menu()->actions(), text)) return nested;
    }
    return nullptr;
}
QAction* action(MainWindow& window, const QString& text) {
    if (auto* item = find_action(window.menuBar()->actions(), text)) return item;
    throw std::runtime_error("Action not found: " + text.toStdString());
}
void settle(QApplication& app, AlignmentView& view) {
    QElapsedTimer timer; timer.start();
    while (!view.isEnabled() && timer.elapsed() < 5000) app.processEvents();
    app.processEvents(); require(view.isEnabled(), "Background operation did not finish");
}
void enter_text(QApplication& app, QAction* item, const QString& text, bool table = false) {
    bool filled = false;
    QTimer::singleShot(0, [&] {
        auto* modal = QApplication::activeModalWidget();
        if (table) {
            auto* dialog = dynamic_cast<QDialog*>(modal);
            QPlainTextEdit* editor = nullptr;
            if (dialog) for (auto* child : dialog->children())
                if (auto* candidate = dynamic_cast<QPlainTextEdit*>(child)) editor = candidate;
            if (editor) { editor->setPlainText(text); filled = true;
                if(auto* preview=dialog->findChild<QPushButton*>("renamePreviewButton")){preview->click();dialog->findChild<QPushButton*>("renameApplyButton")->click();}
                else dialog->accept(); }
        } else {
            auto* input = dynamic_cast<QInputDialog*>(modal);
            if (input) { if(input->windowTitle()=="Rename sequence")require(input->width()>=720,"Wide rename dialog");input->setTextValue(text); filled = true; input->accept(); }
        }
        if (!filled && modal) modal->close();
    });
    item->trigger(); app.processEvents(); require(filled, "Expected input dialog was not opened");
}
void clipboard_checks(QApplication& app,const QString& path) {
    MainWindow w;w.show();auto* v=dynamic_cast<AlignmentView*>(w.centralWidget());w.open_path(path);settle(app,*v);
    v->select_all();const auto ids=v->selected_ids();v->unselect_all();v->set_mode(AlignmentView::Mode::Rewrite);
    auto* copy=action(w,"Copy fragment");auto* paste=action(w,"Paste fragment");
    require(copy->shortcut()==QKeySequence::Copy&&paste->shortcut()==QKeySequence::Paste,"Clipboard shortcuts");
    require(action(w,"Add from clipboard")->shortcut().isEmpty(),"No duplicate paste shortcut");
    auto* clipboard=QApplication::clipboard();
    auto shortcut=[&](int key){w.activateWindow();v->setFocus();app.processEvents();for(auto type:{QEvent::KeyPress,QEvent::KeyRelease}){QKeyEvent event(type,key,Qt::ControlModifier);QApplication::sendEvent(v,&event);}app.processEvents();};
    v->select_fragment({{ids[0]},1,2});shortcut(Qt::Key_C);require(clipboard->text()=="CG","Ctrl+C single fragment plain copy");
    v->select_fragment({{ids[2]},2,4});copy->trigger();require(clipboard->text()=="gt~~","Preserve case and gaps");
    clipboard->setText("unchanged");v->select_fragment({{ids[0],ids[1]},0,3});copy->trigger();require(clipboard->text()=="unchanged","Multiple fragments do nothing");
    v->unselect_all();copy->trigger();require(clipboard->text()=="unchanged","No fragment does nothing");
    v->select_ids({ids[0]});v->select_fragment({{ids[0]},1,2});action(w,"Copy selected sequences")->trigger();require(clipboard->text()==">Alpha full header\nACGT\n","Explicit whole-sequence copy");
    v->unselect_all();v->set_mode(AlignmentView::Mode::Insert);clipboard->setText("AA");paste->trigger();
    require(!w.windowTitle().contains('*'),"No caret does not paste");
    const QFontMetrics metrics(v->font(),v->viewport());const int cw=std::max(4,metrics.horizontalAdvance('M')+2),rh=std::max(12,metrics.height()+4);
    auto click=[&](int row,int col){QPointF p(230+(col+.5)*cw,v->ruler_height()+(row+.5)*rh);for(auto type:{QEvent::MouseButtonPress,QEvent::MouseButtonRelease}){QMouseEvent event(type,p,QPointF(v->viewport()->mapToGlobal(p.toPoint())),Qt::LeftButton,type==QEvent::MouseButtonPress?Qt::LeftButton:Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(v->viewport(),&event);}app.processEvents();};
    auto expect=[&](const QString& expected){v->select_ids({ids[0]});action(w,"Copy selected sequences")->trigger();require(clipboard->text()==expected,"Expected clipboard after paste/undo");v->unselect_all();};
    click(0,2);clipboard->setText("aU\r\n R? .-~\t");shortcut(Qt::Key_V);expect(">Alpha full header\nACGaUR?~-~T\n");
    action(w,"Undo")->trigger();expect(">Alpha full header\nACGT\n");
    action(w,"Redo")->trigger();expect(">Alpha full header\nACGaUR?~-~T\n");action(w,"Undo")->trigger();
    click(0,4);clipboard->setText("N");paste->trigger();clipboard->setText("t");paste->trigger();expect(">Alpha full header\nACGTNt\n");
    action(w,"Undo")->trigger();action(w,"Undo")->trigger();
    click(0,1);for(const auto& invalid:{QString("AC12GT"),QString("ACΩGT"),QString("  \r\n\t")}){clipboard->setText(invalid);paste->trigger();expect(">Alpha full header\nACGT\n");}
    for(auto mode:{AlignmentView::Mode::View}){v->set_mode(mode);click(0,1);clipboard->setText("AA");paste->trigger();expect(">Alpha full header\nACGT\n");}
    v->set_mode(AlignmentView::Mode::Insert);click(0,1);clipboard->setText(">appended\nNn~~\n");paste->trigger();v->select_all();require(v->selected_ids().size()==3,"Fragment shortcut cannot import FASTA");v->unselect_all();action(w,"Paste sequences")->trigger();v->select_all();require(v->selected_ids().size()==4,"Explicit FASTA sequence paste");action(w,"Undo")->trigger();v->select_all();require(v->selected_ids().size()==3,"FASTA append undo");
    AlignmentOverview* overview=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* item=dynamic_cast<AlignmentOverview*>(child))overview=item;
    require(overview,"Main-window overview dock");QElapsedTimer timer;timer.start();while(!overview->ready()&&timer.elapsed()<2000)app.processEvents();require(overview->ready()&&overview->displayed_bins().back().end==6,"Main-window overview after edit/undo");
    action(w,"Alignment overview")->trigger();require(!overview->isVisible(),"Hide overview");action(w,"Alignment overview")->trigger();require(overview->isVisible(),"Show overview");
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        QTemporaryDir dir;
        require(dir.isValid(), "Temporary directory");
        const auto path = dir.filePath("unicode_образец.fasta");
        QFile file(path); require(file.open(QIODevice::WriteOnly), "Fixture open");
        file.write(">Alpha full header\nACGT\n>Alpha full header\nTTTT\n>Beta\nacgt~~\n"); file.close();
        MainWindow window; window.show();
        auto* view = dynamic_cast<AlignmentView*>(window.centralWidget()); require(view, "Alignment view");
        action(window, "Add from clipboard"); action(window, "New alignment from clipboard"); action(window, "Alignment font…");
        window.open_path(path); settle(app, *view);
        view->select_all(); require(view->selected_ids().size() == 3, "Opened FASTA rows");
        auto search = [&](const QString& pattern, bool sequences, std::size_t expected) {
            enter_text(app, action(window, sequences ? "Sequences matching regular expression…" : "Names matching regular expression…"), pattern);
            settle(app, *view); require(view->selected_ids().size() == expected, "Regex selection result");
        };
        search("^Alpha", false, 2); search("(?i)^ACGT", true, 2); search("^TTTT$", true, 1);
        enter_text(app, action(window, "Rename selected sequence…"), "Образец α");
        search("^Образец α$", false, 1);
        action(window, "Undo")->trigger(); search("^Alpha", false, 2);
        view->unselect_all();
        enter_text(app, action(window, "Using old name / new name table…"), "Alpha full header\tBeta\nBeta\tGamma", true);
        search("^Beta$", false, 2); search("^Gamma$", false, 1);
        action(window, "Undo")->trigger(); search("^Alpha", false, 2); search("^Gamma$", false, 0);
        action(window, "Redo")->trigger(); search("^Beta$", false, 2); search("^Gamma$", false, 1);
        require(window.windowTitle().contains('*'), "Rename dirty state");
        search("^Gamma$", false, 1);
        action(window, "Lock gaps (~ → -)")->trigger(); search("^acgt--$", true, 1);
        search("^Gamma$", false, 1); action(window, "Uppercase")->trigger(); search("^ACGT--$", true, 1);
        action(window, "Undo")->trigger(); search("^acgt--$", true, 1);
        search("^Gamma$", false, 1); action(window, "Unlock gaps (- → ~)")->trigger(); search("^acgt~~$", true, 1);
        search("^Gamma$", false, 1); action(window, "Degap (remove open gaps)")->trigger(); search("^acgt$", true, 1);
        action(window, "Undo")->trigger(); search("^acgt~~$", true, 1);
        search("^Gamma$", false, 1); action(window, "Reverse complement")->trigger(); search("^~~acgt$", true, 1);
        action(window, "Undo")->trigger(); search("^acgt~~$", true, 1);
        action(window, "Unalign (remove all gaps)")->trigger(); search("^acgt$", true, 1);
        action(window, "Undo")->trigger(); search("^acgt~~$", true, 1);
        action(window, "Minimize alignment (remove all-gap columns)")->trigger(); search("^acgt$", true, 1);
        action(window, "Undo")->trigger(); search("^acgt~~$", true, 1);
        search("^Beta$", false, 2);
        action(window, "Copy sequence titles")->trigger();
        require(QApplication::clipboard()->text()=="Beta\nBeta\n", "Copy duplicate titles in row order");
        action(window, "Copy selected sequences")->trigger();
        require(QApplication::clipboard()->text()==">Beta\nACGT\n>Beta\nTTTT\n", "Selected FASTA export");
        const QFontMetrics metrics(view->font(),view->viewport());const int rh=std::max(12,metrics.height()+4);
        bool doubleRenamed=false;
        QTimer::singleShot(0,[&]{
            if(auto* input=qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {input->setTextValue("Двойной α");doubleRenamed=true;input->accept();}
        });
        QPointF namePoint(12,view->ruler_height()+rh*.5);
        QMouseEvent dbl(QEvent::MouseButtonDblClick,namePoint,QPointF(view->viewport()->mapToGlobal(namePoint.toPoint())),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view->viewport(),&dbl);require(doubleRenamed,"Double-click rename dialog");
        search("^Двойной α$",false,1);action(window,"Copy sequence titles")->trigger();
        require(QApplication::clipboard()->text()=="Двойной α\n","Unicode title export");
        action(window,"Undo")->trigger();search("^Beta$",false,2);
        view->select_all();QPointF emptyPoint(12,view->ruler_height()+rh*5);
        QMouseEvent empty(QEvent::MouseButtonPress,emptyPoint,QPointF(view->viewport()->mapToGlobal(emptyPoint.toPoint())),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view->viewport(),&empty);require(view->selected_ids().empty(),"Empty name-area click clears selection");
        bool fontChanged=false;
        QTimer::singleShot(0,[&]{
            if(auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
                auto* size=dialog->findChild<QSpinBox*>("alignmentFontSize");auto* style=dialog->findChild<QComboBox*>("alignmentFontStyle");
                auto* family=dialog->findChild<QFontComboBox*>("alignmentFontFamily");
                if(size&&style&&family){size->setValue(14);style->setCurrentIndex(1);fontChanged=true;if(argc>1)dialog->grab().save(QString::fromLocal8Bit(argv[1]));dialog->accept();}else dialog->reject();
            }
        });
        action(window,"Alignment font…")->trigger();require(fontChanged&&view->font().pointSize()==14&&view->font().bold()&&!view->font().underline()&&!view->font().strikeOut(),"Simplified font controls");
        clipboard_checks(app,path);
        std::cout << "PASS GUI smoke: existing workflows, contextual plain/FASTA copy, Insert paste/caret movement/atomic undo, invalid input, other modes, FASTA append, font controls\n";
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
