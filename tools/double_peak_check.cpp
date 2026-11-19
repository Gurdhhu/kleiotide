#include "test_preferences.hpp"
#include "../tests/double_peak_fixture.hpp"
#include "core/session.hpp"
#include "core/version.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "gui/main_window.hpp"
#include "gui/table_io.hpp"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTimer>

#include <iostream>
#include <sstream>

using namespace kleiotide;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class Widget> Widget* child(QObject& parent, const char* name) {
    auto* widget = parent.findChild<Widget*>(name);
    require(widget, name);
    return widget;
}

void inspect_menu(const QMenu& menu) {
    const auto actions = menu.actions();
    for (int i = 2; i + 1 < actions.size(); ++i) {
        if (actions[i]->text() != "Double peak calling…") continue;
        require(actions[i]->isEnabled() && actions[i - 2]->text() == "Delete trace…" &&
            actions[i - 1]->isSeparator() && actions[i + 1]->isSeparator(),
            "Double peak calling has its own section immediately after Delete trace");
        return;
    }
    throw std::runtime_error("No double peak command in menu");
}

void save_report(QDialog& dialog, const QTemporaryDir& files, const QString& extension) {
    const auto file = files.filePath("report-" + extension + "." + extension);
    QTimer picker;
    picker.setInterval(5);
    QElapsedTimer deadline;
    deadline.start();
    QObject::connect(&picker, &QTimer::timeout, &dialog, [&] {
        auto* modal = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (deadline.elapsed() > 5000) {
            if (modal && modal != &dialog) modal->reject();
            return;
        }
        auto* chooser = qobject_cast<QFileDialog*>(modal);
        if (!chooser) return;
        for (const auto& filter : chooser->nameFilters())
            if (filter.endsWith("*." + extension + ")")) chooser->selectNameFilter(filter);
        chooser->setDirectory(files.path());
        auto* input = chooser->findChild<QLineEdit*>("fileNameEdit");
        if (!input) {
            chooser->reject();
            return;
        }
        input->setText("report-" + extension);
        QMetaObject::invokeMethod(chooser, "accept", Qt::DirectConnection);
    });
    picker.start();
    child<QPushButton>(dialog, "doublePeakSave")->click();
    require(QFile::exists(file), "Save adopts the selected format and adds its extension");

    if (extension == "xlsx") {
        QStringList sheets;
        read_xlsx_table(file, 0, &sheets);
        const auto rules = read_xlsx_table(file, 0);
        const auto results = read_xlsx_table(file, 1);
        require(sheets == QStringList{"Calculation methods", "Double peaks"} && rules.size() > 1 &&
            rules[1] == QStringList{"Kleiotide version", QString::fromLatin1(version)} &&
            results.size() == 5 && results[1].size() > 8 && results[1][0] == "No" && results[1][2] == "1" &&
            results[1][4] == "R" && results[1][6].toDouble() == 60.0 &&
            results[1][8] == "4",
            "XLSX contains rules, version, checkbox choices and measured values");
    } else {
        QFile input(file);
        require(input.open(QIODevice::ReadOnly), "Read saved report");
        const char sep = extension == "csv" ? ',' : '\t';
        const auto bytes = input.readAll();
        require(bytes.contains(QByteArray("Kleiotide version") + sep + version) &&
            bytes.contains("# Double peaks") && bytes.contains("reviewed read") &&
            bytes.contains("Peak pair"), "Delimited report contains rules and all preview columns");
    }
}
}

int main(int argc, char** argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    try {
        TestPreferences settings;
        QTemporaryDir files;
        require(files.isValid(), "Temporary test files");
        Document document;
        document.load({test::double_peak_read("reviewed read", true),
                       test::double_peak_read("Q-free read", false)});
        const auto path = files.filePath("reads.kleio");
        std::ostringstream session(std::ios::binary);
        write_session(session, document);
        QFile input(path);
        require(input.open(QIODevice::WriteOnly), "Write input session");
        input.write(QByteArray::fromStdString(session.str()));
        input.close();

        MainWindow window;
        window.show();
        const auto command = child<QAction>(window, "doublePeakCalling");
        require(!command->isEnabled(), "Command is disabled for an alignment without traces");
        window.open_path(path);
        QElapsedTimer deadline;
        deadline.start();
        while (!command->isEnabled() && deadline.elapsed() < 10000)
            QApplication::processEvents();
        require(command->isEnabled(), "Command is available after loading a linked trace");
        auto* view = dynamic_cast<AlignmentView*>(window.centralWidget());
        require(view, "Alignment view");
        view->select_ids({1});
        inspect_menu(*qobject_cast<QMenu*>(command->parent()));

        QString failure;
        bool checkedMenu = false;
        QTimer::singleShot(0, [&] {
            auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
            try {
                require(menu, "Sequence name context menu");
                inspect_menu(*menu);
                checkedMenu = true;
            } catch (const std::exception& error) {
                failure = error.what();
            }
            if (menu) menu->close();
        });
        const QPoint name(12, view->ruler_height() + 4);
        QContextMenuEvent context(QContextMenuEvent::Mouse, name, view->viewport()->mapToGlobal(name));
        QApplication::sendEvent(view->viewport(), &context);
        require(checkedMenu && failure.isEmpty(), qPrintable(failure));

        const auto state = window.current_document().state_id();
        int stage = 0;
        QTimer driver;
        driver.setInterval(15);
        deadline.restart();
        QObject::connect(&driver, &QTimer::timeout, &window, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            try {
                require(deadline.elapsed() < 20000, "Double peak dialog timed out");
                if (!dialog || dialog->objectName() != "doublePeakDialog") return;
                auto* table = child<QTableView>(*dialog, "doublePeakPreview");
                auto* model = table->model();
                if (stage == 0 && model->rowCount() == 2) {
                    require(child<QComboBox>(*dialog, "doublePeakScope")->currentIndex() == 0 &&
                        child<QSpinBox>(*dialog, "doublePeakSecondary")->value() == 60,
                        "Selected scope and default threshold");
                    for (int column = 0; column < model->columnCount(); ++column)
                        require(!model->headerData(column, Qt::Horizontal, Qt::ToolTipRole).toString().isEmpty(),
                            "Every preview column has an explanatory tooltip");
                    require(model->headerData(8, Qt::Horizontal, Qt::ToolTipRole).toString().contains("recorded signal points") &&
                        model->index(0, 6).data(Qt::ToolTipRole).toString().contains("60 of 100 signal units above zero"),
                        "Tooltips explain displacement and show the actual heights");
                    require(!child<QComboBox>(*dialog, "doublePeakScope")->toolTip().isEmpty(),
                        "Scope has an explanatory tooltip");
                    for (const auto* setting : {"doublePeakSecondary", "doublePeakThird", "doublePeakDisplacement"})
                        require(!child<QSpinBox>(*dialog, setting)->toolTip().isEmpty(),
                            "Each threshold has an explanatory tooltip");
                    require(model->index(1, 10).data().toString() == "Detected peak pair does not include the current symbol",
                        "Review states the exact reason for the unchecked proposal");
                    require(model->index(0, 2).data().toString() == "1" &&
                        model->index(0, 4).data().toString() == "R" &&
                        model->index(0, 0).data(Qt::CheckStateRole).toInt() == Qt::Checked &&
                        model->index(1, 0).data(Qt::CheckStateRole).toInt() == Qt::Unchecked,
                        "Proposals expose alignment column, IUPAC and review status");
                    table->selectRow(1);
                    auto* trace = dynamic_cast<TraceCanvas*>(child<QWidget>(*dialog, "doublePeakTrace"));
                    require(trace && trace->sequence_id() == 1 && trace->focused_column() == 5,
                        "Selecting a proposal shows the corresponding chromatogram site");
                    require(model->setData(model->index(0, 0), Qt::Unchecked, Qt::CheckStateRole),
                        "A supported proposal can be unchecked");
                    child<QComboBox>(*dialog, "doublePeakScope")->setCurrentIndex(1);
                    ++stage;
                    return;
                }
                if (stage == 1 && model->rowCount() == 4) {
                    require(model->index(0, 0).data(Qt::CheckStateRole).toInt() == Qt::Unchecked &&
                        model->index(2, 0).data(Qt::CheckStateRole).toInt() == Qt::Checked,
                        "Scope refresh keeps explicit choices for matching proposals");
                    driver.stop();
                    child<QPushButton>(*dialog, "doublePeakCopy")->click();
                    const auto copied = QApplication::clipboard()->text();
                    require(copied.contains("Kleiotide version\t" + QString::fromLatin1(version)) &&
                        copied.contains("reviewed read") && copied.contains("Q-free read") &&
                        copied.contains("# Double peaks"),
                        "Clipboard includes rules, version and preview in TSV");
                    for (const auto& extension : {QString("xlsx"), QString("tsv"), QString("csv")})
                        save_report(*dialog, files, extension);
                    require(window.current_document().state_id() == state,
                        "Exporting and reviewing do not edit residues");
                    ++stage;
                    child<QPushButton>(*dialog, "doublePeakApply")->click();
                    if (dialog->isVisible()) dialog->reject();
                }
            } catch (const std::exception& error) {
                failure = error.what();
                driver.stop();
                if (dialog) dialog->reject();
            }
        });
        driver.start();
        command->trigger();
        require(failure.isEmpty(), qPrintable(failure));
        require(stage == 2 && window.current_document().state_id() != state &&
            window.current_document().sequence(1).symbols.text() == "A~CACgG" &&
            window.current_document().sequence(2).symbols.text() == "R~CACgG",
            "Only the checked, accepted proposal is applied");
        std::cout << "PASS double peak menus, live preview, trace navigation, copying, saving and applying\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
