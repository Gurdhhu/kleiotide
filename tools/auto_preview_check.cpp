#include "../tests/abif_fixture.hpp"
#include "core/abif.hpp"
#include "gui/app_settings.hpp"
#include "gui/collapse_dialog.hpp"
#include "gui/merge_dialog.hpp"
#include "gui/quality_dialog.hpp"
#include "gui/rename_dialog.hpp"
#include "gui/trace_link_dialog.hpp"
#include "test_preferences.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <sstream>
using namespace kleiotide;
void need(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
template <class T> T *child(QDialog *d, const char *id) {
  auto *w = d->findChild<T *>(id);
  need(w, id);
  return w;
}
int main(int argc, char **argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication app(argc, argv);
  try {
    TestPreferences prefs;
    (void)AppSettings::instance();
    QString error, mode;
    int stage = 0; qint64 unchangedAt=0; qlonglong unchangedRevision=0;
    QElapsedTimer time;
    QTimer driver;
    driver.setInterval(20);
    QObject::connect(&driver, &QTimer::timeout, [&] {
      try {
        need(time.elapsed() < 15000, "Automatic preview timeout");
        auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!d || qobject_cast<QFileDialog *>(d))
          return;
        if (mode == "rename") {
          if (stage == 0) {
            stage = 1;
            need(!child<QPushButton>(d, "renamePreviewButton")->isVisible(),
                 "Preview button removed");
            child<QLineEdit>(d, "renameText")->setText("_new");
          } else if (child<QPushButton>(d, "renameApplyButton")->isEnabled()) {
            driver.stop();
            child<QPushButton>(d, "renameApplyButton")->click();
          }
        }
        if (mode == "quality") {
          if (stage == 0) {
            stage = 1;
            child<QSpinBox>(d, "qualityThreshold")->setValue(30);
          } else if (child<QPushButton>(d, "qualityApply")->isEnabled()) {
            need(!child<QPushButton>(d, "qualityPreviewButton")->isVisible(),
                 "Quality preview button removed");
            driver.stop();
            child<QPushButton>(d, "qualityApply")->click();
          }
        }
        if (mode == "collapse") {
          if (stage == 0) {
            stage = 1;
            child<QComboBox>(d, "collapseScope")->setCurrentIndex(1);
          } else if (child<QPushButton>(d, "collapseApply")->isEnabled()) {
            need(!child<QPushButton>(d, "collapsePreview")->isVisible(),
                 "Collapse preview button removed");
            if(stage==1){child<QTableView>(d,"collapseSets")->selectRow(0);child<QTableView>(d,"collapseMembers")->selectRow(1);child<QPushButton>(d,"collapseKeep")->click();child<QLineEdit>(d,"collapseSeparator")->setText(" | ");stage=2;time.restart();}else if(stage==2&&time.elapsed()>600){driver.stop();child<QPushButton>(d,"collapseApply")->click();}
          }
        }
        if (mode == "merge") {
          if (stage == 0) {
            stage = 1;
            child<QLineEdit>(d, "mergeName")->setText("automatic_merge");
            QMetaObject::invokeMethod(
                child<QLineEdit>(d, "mergeName"), "textEdited",
                Qt::DirectConnection,
                Q_ARG(QString, QString("automatic_merge")));
            child<QSpinBox>(d, "mergeQualityMargin")->setValue(11);
          } else if (child<QPushButton>(d, "mergeApply")->isEnabled()) {
            need(!child<QPushButton>(d, "mergePreview")->isVisible(),
                 "Merge preview button removed");
            driver.stop();
            child<QPushButton>(d, "mergeApply")->click();
          }
        }
        if (mode == "trace") {
          auto *table = child<QTableView>(d, "traceAssignments");
          if (stage == 0) {
            stage = 1;
            child<QPushButton>(d, "traceLinkAdd")->click();
          } else if (stage == 1 &&
                     child<QPushButton>(d, "traceLinkApply")->isEnabled()) {
            need(table->model()->index(1, 2).data().toString() ==
                     "Not attempted",
                 "Unassigned stays unattempted");
            stage = 11;unchangedAt=time.elapsed();unchangedRevision=d->property("traceAssignmentsRevision").toLongLong();
            table->model()->setData(table->model()->index(0, 1),0,Qt::EditRole);
          } else if(stage==11){
            need(d->property("traceAssignmentsRevision").toLongLong()==unchangedRevision&&child<QPushButton>(d,"traceLinkApply")->isEnabled(),"Unchanged assignment never invalidates or recalculates the preview");
            if(time.elapsed()-unchangedAt<600)return;
            stage=2;
            table->model()->setData(table->model()->index(1, 1), 0,
                                    Qt::EditRole);
          } else if (stage == 2 &&
                     child<QPushButton>(d, "traceLinkApply")->isEnabled()) {
            need(table->model()->index(1, 3).data().toString() == "Perfect",
                 "Manual assignment mapped automatically");
            driver.stop();
            child<QPushButton>(d, "traceLinkApply")->click();
          }
        }
      } catch (const std::exception &e) {
        error = e.what();
        driver.stop();
        if (auto *d =
                qobject_cast<QDialog *>(QApplication::activeModalWidget()))
          d->reject();
      }
    });
    auto start = [&](QString name) {
      mode = name;
      stage = 0;
      time.restart();
      driver.start();
    };
    auto d = std::make_shared<Document>();
    d->load({{"alpha", "ACGT"}, {"beta", "ACGT"}});
    start("rename");
    auto patch =
        show_batch_rename(*d, {1, 2}, {}, RenameOperation::Append, nullptr);
    need(patch && patch->at(0).second == "alpha_new", qPrintable(error));
    start("merge");
    need(merge_sequences_dialog(d, {1, 2}, MergeMode::Aligned, nullptr)
             .has_value(),
         qPrintable(error));
    need(d->rows().back().name == "automatic_merge",
         "Merge confirmation appends named result");
    auto c = std::make_shared<Document>();
    c->load({{"alpha", "ACGT"}, {"beta", "ACGT"}});
    start("collapse");
    need(collapse_sequences_dialog(c, {1, 2}, nullptr) && c->rows().size() == 1 && c->rows()[0].id==2,
         qPrintable(error));
    std::istringstream input(test::synthetic_abif("ACGT"));
    auto record = read_abif(input, "alpha", "a.ab1");
    auto original = std::make_shared<Chromatogram>(*record.trace->original);
    original->quality = {40, 10, 40, 10};
    auto link = std::make_shared<TraceLink>(*record.trace);
    link->original = original;
    record.trace = link;
    auto q = std::make_shared<Document>();
    q->load({record});
    start("quality");
    need(edit_trace_quality(q, {1}, QualityOperation::CallsToN, nullptr) &&
             q->rows()[0].symbols.text() == "ANGN",
         qPrintable(error));
    QTemporaryDir temp;
    auto path = temp.filePath("alpha.ab1");
    {
      QFile f(path);
      need(f.open(QIODevice::WriteOnly), "AB1 fixture");
      f.write(QByteArray::fromStdString(
          test::synthetic_abif("ACGTTGCAAGTCCTAGATCGACCT")));
    }
    QTimer picker;
    picker.setInterval(10);
    QObject::connect(&picker, &QTimer::timeout, [&] {
      if (auto *f =
              qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
        if (auto *text = f->findChild<QLineEdit *>("fileNameEdit"))
          text->setText(path);
        QMetaObject::invokeMethod(f, "accept", Qt::QueuedConnection);
      }
    });
    picker.start();
    auto t = std::make_shared<Document>();
    t->load({{"alpha", "ACGTTGCAAGTCCTAGATCGACCT"},
             {"beta", "ACGTTGCAAGTCCTAGATCGACCT"}});
    start("trace");
    need(link_trace_dialog(t, {1, 2}, nullptr) && t->sequence(2).trace,
         qPrintable(error));
    need(error.isEmpty(), qPrintable(error));
    std::cout << "PASS automatic rename, merge, quality, collapse and trace "
                 "assignment/mapping without clicking Preview; explicit "
                 "confirmation required\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
