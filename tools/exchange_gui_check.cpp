#include "gui/app_settings.hpp"
#include "gui/groups_dialog.hpp"
#include "gui/rename_dialog.hpp"
#include "gui/spart.hpp"
#include "test_preferences.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
using namespace kleiotide;
namespace {
void need(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
template <class T> T *child(QObject &p, const char *id) {
  auto *w = p.findChild<T *>(id);
  need(w, id);
  return w;
}
} // namespace
int main(int argc, char **argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication app(argc, argv);
  try {
    TestPreferences preferences;
    (void)AppSettings::instance();
    QTemporaryDir temp;
    Document source;
    source.load({{"alpha", "ACGT"}, {"beta", "ACGT"}});
    QString failure;
    QElapsedTimer deadline;
    int stage = 0;
    QString input;
    const auto csv = temp.filePath("groups.csv");
    {
      QFile f(csv);
      need(f.open(QIODevice::WriteOnly), "CSV fixture");
      f.write("Other,Sequence name,Group name\r\nnote,\"(alpha)\",\"First "
              "group\"\r\n\"quoted, text\",beta,Second group\r\n");
    }
    const auto workbook = QFileInfo(QString::fromUtf8(__FILE__))
                              .dir()
                              .filePath("../examples/table_import_demo.xlsx");
    QString sheetName = "Groups";
    QTimer chooser;
    chooser.setInterval(5);
    QObject::connect(&chooser, &QTimer::timeout, [&] {
      if (auto *file =
              qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
        if (!file->property("selectionScheduled").toBool()) {
          file->setProperty("selectionScheduled", true);
          const auto path = input;
          file->setDirectory(QFileInfo(path).absolutePath());
          QTimer::singleShot(100, file, [file, path] {
            if (auto *edit = file->findChild<QLineEdit *>("fileNameEdit"))
              edit->setText(path);
            QMetaObject::invokeMethod(file, "accept", Qt::QueuedConnection);
          });
        }
      } else if (auto *sheet = qobject_cast<QInputDialog *>(
                     QApplication::activeModalWidget())) {
        sheet->setTextValue(sheetName);
        sheet->accept();
      }
    });
    chooser.start();
    for (bool excel : {false, true}) {
      Document d = source;
      input = excel ? workbook : csv;
      stage = 0;
      deadline.restart();
      QTimer drive;
      drive.setInterval(5);
      QObject::connect(&drive, &QTimer::timeout, [&] {
        try {
          if (deadline.elapsed() >= 15000) {
            auto *w = QApplication::activeModalWidget();
            throw std::runtime_error((QString("Group import timeout: ") +
                                      (w ? w->windowTitle() : "none"))
                                         .toStdString());
          }
          if (auto *file = qobject_cast<QFileDialog *>(
                  QApplication::activeModalWidget())) {
            if (!file->property("selectionScheduled").toBool()) {
              file->setProperty("selectionScheduled", true);
              file->setDirectory(QFileInfo(input).absolutePath());
              file->selectFile(QFileInfo(input).fileName());
              QTimer::singleShot(150, file, [file, path = input] {
                if (auto *edit = file->findChild<QLineEdit *>("fileNameEdit"))
                  edit->setText(path);
                QMetaObject::invokeMethod(file, "accept", Qt::QueuedConnection);
              });
            }
            return;
          }
          if (auto *sheet = qobject_cast<QInputDialog *>(
                  QApplication::activeModalWidget())) {
            sheet->setTextValue("Groups");
            sheet->accept();
            return;
          }
          auto *dialog =
              qobject_cast<QDialog *>(QApplication::activeModalWidget());
          if (!dialog || dialog->objectName() != "groupImportDialog")
            return;
          if (stage == 0) {
            stage = 1;
            child<QPushButton>(*dialog, "groupImportLoad")->click();
          } else if (stage == 1 &&
                     !child<QPlainTextEdit>(*dialog, "groupImportText")
                          ->toPlainText()
                          .isEmpty()) {
            stage = 2;
            child<QCheckBox>(*dialog, "groupImportHeader")->setChecked(true);
          } else if (stage == 2 &&
                     child<QComboBox>(*dialog, "groupImportSequenceColumn")
                             ->count() == 3) {
            stage = 3;
            child<QComboBox>(*dialog, "groupImportSequenceColumn")
                ->setCurrentIndex(1);
            child<QComboBox>(*dialog, "groupImportGroupColumn")
                ->setCurrentIndex(2);
          } else if (stage == 3 &&
                     child<QPushButton>(*dialog, "groupImportApply")
                         ->isEnabled()) {
            need(child<QTableView>(*dialog, "groupImportPreview")
                         ->model()
                         ->rowCount() == 2,
                 "Group preview rows");
            if (argc > 1) {
              QDir().mkpath(QString::fromLocal8Bit(argv[1]));
              dialog->grab().save(QString::fromLocal8Bit(argv[1]) +
                                  "/table-group-import.png");
            }
            stage = 4;
            drive.stop();
            child<QPushButton>(*dialog, "groupImportApply")->click();
          }
        } catch (const std::exception &e) {
          failure = e.what();
          drive.stop();
          QTimer::singleShot(0, qApp, [] {
            for (auto *w : QApplication::topLevelWidgets())
              if (auto *d = qobject_cast<QDialog *>(w))
                d->reject();
          });
          if (auto *dialog =
                  qobject_cast<QDialog *>(QApplication::activeModalWidget()))
            dialog->reject();
        }
      });
      drive.start();
      need(import_groups(d, nullptr) && stage == 4 && failure.isEmpty(),
           qPrintable(failure));
      need(d.group(d.sequence(1).groupId)->name == "First group" &&
               d.group(d.sequence(2).groupId)->name == "Second group",
           "CSV/XLSX chosen columns and name normalization");
    }
    input = workbook;
    sheetName = "Rename";
    for (auto op : {RenameOperation::Table, RenameOperation::AppendTable}) {
      stage = 0;
      deadline.restart();
      QTimer drive;
      drive.setInterval(5);
      QObject::connect(&drive, &QTimer::timeout, [&] {
        try {
          if (deadline.elapsed() >= 15000) {
            auto *w = QApplication::activeModalWidget();
            throw std::runtime_error(
                (QString("Rename timeout: ") + (w ? w->windowTitle() : "none"))
                    .toStdString());
          }
          if (auto *file = qobject_cast<QFileDialog *>(
                  QApplication::activeModalWidget())) {
            if (!file->property("selectionScheduled").toBool()) {
              file->setProperty("selectionScheduled", true);
              file->setDirectory(QFileInfo(workbook).absolutePath());
              file->selectFile(QFileInfo(workbook).fileName());
              QTimer::singleShot(150, file, [file, path = workbook] {
                if (auto *edit = file->findChild<QLineEdit *>("fileNameEdit"))
                  edit->setText(path);
                QMetaObject::invokeMethod(file, "accept", Qt::QueuedConnection);
              });
            }
            return;
          }
          if (auto *sheet = qobject_cast<QInputDialog *>(
                  QApplication::activeModalWidget())) {
            sheet->setTextValue("Rename");
            sheet->accept();
            return;
          }
          auto *dialog =
              qobject_cast<QDialog *>(QApplication::activeModalWidget());
          if (!dialog || dialog->objectName() != "batchRenameDialog")
            return;
          if (stage == 0) {
            stage = 1;
            child<QPushButton>(*dialog, "renameLoadTable")->click();
          } else if (stage == 1 &&
                     !child<QPlainTextEdit>(*dialog, "renameTable")
                          ->toPlainText()
                          .isEmpty()) {
            stage = 2;
            child<QCheckBox>(*dialog, "renameTableHeader")->setChecked(true);
            child<QComboBox>(*dialog, "renameOldColumn")->setCurrentIndex(1);
            child<QComboBox>(*dialog, "renameNewColumn")->setCurrentIndex(2);
            child<QPushButton>(*dialog, "renamePreviewButton")->click();
          } else if (stage == 2) {
            need(child<QPushButton>(*dialog, "renameApplyButton")->isEnabled(),
                 "Rename preview valid");
            if (argc > 1)
              dialog->grab().save(QString::fromLocal8Bit(argv[1]) +
                                  "/table-rename-" + QString::number(int(op)) +
                                  ".png");
            stage = 3;
            drive.stop();
            child<QPushButton>(*dialog, "renameApplyButton")->click();
          }
        } catch (const std::exception &e) {
          failure = e.what();
          drive.stop();
          QTimer::singleShot(0, qApp, [] {
            for (auto *w : QApplication::topLevelWidgets())
              if (auto *d = qobject_cast<QDialog *>(w))
                d->reject();
          });
          if (auto *dialog =
                  qobject_cast<QDialog *>(QApplication::activeModalWidget()))
            dialog->reject();
        }
      });
      drive.start();
      auto patches = show_batch_rename(source, {}, QString{}, op, nullptr);
      need(patches && patches->size() == 2 && stage == 3 && failure.isEmpty(),
           qPrintable(failure));
      need(patches->front().second == (op == RenameOperation::Table
                                           ? "renamed alpha"
                                           : "alpha renamed alpha"),
           "Both rename operations map selected XLSX columns");
    }
    source.rename({{1," alpha "}});
    const auto spart = temp.filePath("partitions.spart");
    {
      QFile file(spart);
      need(file.open(QIODevice::WriteOnly), "SPART fixture");
      file.write("begin "
                 "spart;Project_name=p;Date=2026;N_spartitions=2:Separate/"
                 "Together;N_individuals=2/2;N_subsets=2/"
                 "1;Individual_assignment=\n_alpha_:1/5\nbeta:2/5;end;");
    }
    input = spart;
    stage = 0;
    deadline.restart();
    QTimer drive;
    drive.setInterval(5);
    QObject::connect(&drive, &QTimer::timeout, [&] {
      try {
        need(deadline.elapsed() < 15000, "SPART import UI timeout");
        if (auto *file = qobject_cast<QFileDialog *>(
                QApplication::activeModalWidget())) {
          if (!file->property("selectionScheduled").toBool()) {
            file->setProperty("selectionScheduled", true);
            file->setDirectory(QFileInfo(spart).absolutePath());
            file->selectFile(QFileInfo(spart).fileName());
            QTimer::singleShot(150, file, [file, path = spart] {
              if (auto *edit = file->findChild<QLineEdit *>("fileNameEdit"))
                edit->setText(path);
              QMetaObject::invokeMethod(file, "accept", Qt::QueuedConnection);
            });
          }
          return;
        }
        auto *dialog =
            qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog || dialog->objectName() != "groupImportDialog")
          return;
        if (stage == 0) {
          stage = 1;
          auto *parts = child<QComboBox>(*dialog, "spartPartition");
          need(parts->count() == 2, "SPART partition dropdown");need(child<QCheckBox>(*dialog,"spartAssumeSanitation")->isChecked(),"SPART sanitation matching defaults on");
          parts->setCurrentIndex(1);
        } else if (stage == 1 && child<QPushButton>(*dialog, "groupImportApply")
                                     ->isEnabled()) {
          stage = 2;
          drive.stop();
          child<QPushButton>(*dialog, "groupImportApply")->click();
        }
      } catch (const std::exception &e) {
        failure = e.what();
        drive.stop();
        if (auto *dialog =
                qobject_cast<QDialog *>(QApplication::activeModalWidget()))
          dialog->reject();
      }
    });
    drive.start();
    need(import_spart_groups(source, nullptr) && stage == 2 &&
             failure.isEmpty(),
         qPrintable(failure));
    need(source.sequence(1).name==" alpha " && source.sequence(1).groupId == source.sequence(2).groupId &&
             source.group(source.sequence(1).groupId)->name == "5",
         "Chosen SPART partition applied");
    std::cout << "PASS CSV/XLSX group import with worksheet/header/column "
                 "mapping, both XLSX rename operations, and multi-partition "
                 "SPART selection/preview/apply\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
