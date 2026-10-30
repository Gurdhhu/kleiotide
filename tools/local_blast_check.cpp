#include "core/session.hpp"
#include "gui/app_settings.hpp"
#include "gui/local_blast.hpp"
#include "test_preferences.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableView>
#include <QTemporaryDir>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace kleiotide;
namespace {
void need(bool b, const char *text) {
  if (!b)
    throw std::runtime_error(text);
}
template <class T> T *child(QObject &p, const char *name) {
  auto *w = p.findChild<T *>(name);
  need(w, name);
  return w;
}
} // namespace
int main(int argc, char **argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication app(argc, argv);
  try {
    TestPreferences preferences;
    QTemporaryDir temp;
    need(temp.isValid(), "Temporary directory");
    const bool real = qEnvironmentVariableIsSet("KLEIOTIDE_TEST_REAL_BLAST");
    auto settings = AppSettings::instance().values();
    settings.blastnLauncher = QStandardPaths::findExecutable("blastn");
    settings.blastpLauncher = QStandardPaths::findExecutable("blastp");
    settings.makeblastdbLauncher =
        QStandardPaths::findExecutable("makeblastdb");
    need(AppSettings::instance().apply(settings), "Local tool settings");
    const QString protein = "MQIFVKTLTGKTITLEVEPSDTIENVKAKIQDKEGIPPDQQRLIFAGKQL"
                            "EDGRTLSDYNIQKESTLHLVLRLRGG";
    const QString letters = "ACDEFGHIKLMNPQRSTVWY";
    const QStringList codons = {"GCT", "TGT", "GAT", "GAA", "TTT", "GGT", "CAT",
                                "ATT", "AAA", "CTG", "ATG", "AAT", "CCT", "CAA",
                                "CGT", "TCT", "ACT", "GTT", "TGG", "TAT"};
    QString dna;
    for (auto c : protein)
      dna += codons[letters.indexOf(c)];
    auto document = std::make_shared<Document>();
    document->load(
        {{"DNA alias A", dna.toStdString()},
         {"DNA alias B", dna.toStdString()},
         {"protein alias", protein.toStdString(), RowKind::Protein}});
    const auto fasta = temp.filePath("source.fasta");
    {
      QFile f(fasta);
      need(f.open(QIODevice::WriteOnly), "Write source");
      f.write((">DNA subject with spaces\n" + dna.left(100) + "--" +
               dna.mid(100) + "\n")
                  .toUtf8());
    }
    const auto session = temp.filePath("source.kleio");
    {
      Document d;
      d.load({{"protein subject α", protein.toStdString(), RowKind::Protein},
              {"DNA skip", dna.toStdString()},
              {"mask", "001100", RowKind::Mask}});
      std::ostringstream out(std::ios::binary);
      write_session(out, d);
      QFile f(session);
      need(f.open(QIODevice::WriteOnly), "Write protein source session");
      f.write(QByteArray::fromStdString(out.str()));
    }
    QWidget owner, other;
    owner.show();
    other.show();
    QString failure;
    QElapsedTimer deadline;
    QTimer driver;
    driver.setInterval(10);
    int stage = 0;
    QString sourcePath, targetPath;
    int completedBuilds = 0;
    QObject::connect(&driver, &QTimer::timeout, [&] {
      try {
        if (deadline.elapsed() >= 30000) {
          auto *active = QApplication::activeModalWidget();
          throw std::runtime_error(
              (QString("Database build UI timeout: ") +
               (active ? active->windowTitle() : "no modal widget"))
                  .toStdString());
        }
        if (auto *file = qobject_cast<QFileDialog *>(
                QApplication::activeModalWidget())) {
          if (!file->property("selectionScheduled").toBool()) {
            file->setProperty("selectionScheduled", true);
            const auto path = file->windowTitle().startsWith("Choose source")
                                  ? sourcePath
                                  : targetPath;
            file->setDirectory(QFileInfo(path).absolutePath());
            file->selectFile(QFileInfo(path).fileName());
            QTimer::singleShot(150, file, [file, path] {
              if (auto *edit = file->findChild<QLineEdit *>("fileNameEdit"))
                edit->setText(path);
              QMetaObject::invokeMethod(file, "accept", Qt::QueuedConnection);
            });
          }
          return;
        }
        auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!d || d->objectName() != "localDatabaseBuild")
          return;
        auto *report = d->findChild<QPlainTextEdit *>();
        const auto text = report ? report->toPlainText() : QString{};
        if (text.contains("Saved: ")) {
          ++completedBuilds;
          driver.stop();
          d->reject();
        } else if (auto *close = d->findChild<QPushButton *>();
                   close && close->text() == "Close")
          throw std::runtime_error(text.toStdString());
      } catch (const std::exception &e) {
        failure = e.what();
        driver.stop();
        if (auto *d =
                qobject_cast<QDialog *>(QApplication::activeModalWidget()))
          d->reject();
      }
    });
    for (bool p : {false, true}) {
      sourcePath = p ? session : fasta;
      targetPath = temp.filePath(p ? "protein.kblast" : "nucleotide.kblast");
      if (real) {
        deadline.restart();
        driver.start();
        create_local_database(p, &owner);
        need(failure.isEmpty(), qPrintable(failure));
        need(QFileInfo::exists(targetPath),
             "Database creation UI saved output");
      } else {
        QDir().mkpath(targetPath + ".files");
        QFile index(targetPath + ".files/db" + (p ? ".pin" : ".nin"));
        need(index.open(QIODevice::WriteOnly), "Fixture index");
        index.close();
        QFile f(targetPath);
        need(f.open(QIODevice::WriteOnly), "Fixture manifest");
        f.write(
            QJsonDocument(
                QJsonObject{
                    {"format", "Kleiotide local BLAST database"},
                    {"version", 1},
                    {"protein", p},
                    {"prefix", QFileInfo(targetPath).fileName() + ".files/db"},
                    {"title", p ? "Protein fixture" : "Nucleotide fixture"},
                    {"names",
                     QJsonObject{{"kseq_1", p ? "protein subject α"
                                              : "DNA subject with spaces"}}}})
                .toJson());
        f.close();
        remember_local_database(targetPath);
      }
      need(local_database_paths(p).size() == 1 &&
               local_database_paths(!p).size() == (p ? 1 : 0),
           "Database lists retain molecule types");
    }

    for (const auto &program :
         QStringList{"blastn", "blastp", "blastx", "tblastn", "tblastx"}) {
      stage = 0;
      deadline.restart();
      QTimer search;
      search.setInterval(10);
      QObject::connect(&search, &QTimer::timeout, [&] {
        try {
          need(deadline.elapsed() < 30000, "Local BLAST workflow timeout");
          auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
          if (!d || d->objectName() != "localBlastDialog")
            return;
          if (stage == 0) {
            stage = 1;
            need(d->windowModality() == Qt::WindowModal &&
                     d->parentWidget() == &owner,
                 "Search blocks only its originating window");
            child<QComboBox>(*d, "localBlastProgram")->setCurrentText(program);
            auto *databases = child<QComboBox>(*d, "localBlastDatabase");
            need(databases->count() == 1, "Correct target database list");
            auto info =
                read_local_database(databases->currentData().toString());
            need(info.protein == (program == "blastp" || program == "blastx"),
                 "Target type for all five programs");
            need(child<QLineEdit>(*d, "localBlastLauncher")
                     ->text()
                     .contains(program),
                 "Program-specific executable path");
            if (argc > 1) {
              QDir().mkpath(QString::fromLocal8Bit(argv[1]));
              d->grab().save(QString::fromLocal8Bit(argv[1]) +
                             "/local-options-" + program + ".png");
            }
            if (!real) {
              search.stop();
              d->reject();
              return;
            }
            child<QPushButton>(*d, "localBlastStart")->click();
          } else if (stage == 1 &&
                     child<QPushButton>(*d, "localBlastStart")->isEnabled()) {
            auto text = child<QLabel>(*d, "localBlastStatus")->text();
            need(text.startsWith("Complete:"), qPrintable(text));
            auto *table = child<QTableView>(*d, "localBlastResults");
            need(table->model()->columnCount() == 15 &&
                     table->model()->rowCount() > 0,
                 "Local result metrics and real hits");
            need(table->model()->index(0, 4).data().toString() ==
                     (program == "blastp" || program == "blastx"
                          ? "protein subject α"
                          : "DNA subject with spaces"),
                 "Full original subject names restored");
            if (argc > 1)
              d->grab().save(QString::fromLocal8Bit(argv[1]) +
                             "/local-results-" + program + ".png");
            stage = 2;
            search.stop();
            d->reject();
          }
        } catch (const std::exception &e) {
          failure = e.what();
          search.stop();
          if (auto *d =
                  qobject_cast<QDialog *>(QApplication::activeModalWidget()))
            d->reject();
        }
      });
      search.start();
      local_blast_dialog(document, {1, 2, 3},
                         program == "blastp" || program == "tblastn", &owner);
      need(failure.isEmpty(), qPrintable(failure));
      need(stage == (real ? 2 : 1), "Local workflow completed");
    }
    if (real)
      need(completedBuilds == 2, "Both real database builds completed");
    std::cout << (real ? "PASS actual installed BLAST+ database creation from "
                         "FASTA/.kleio and GUI searches with "
                         "blastn/blastp/blastx/tblastn/tblastx, exact subject "
                         "names, metrics and typed registry\n"
                       : "PASS local BLAST UI: five program choices, typed "
                         "database registry, executable selection and window "
                         "modality (no BLAST+ dependency)\n");
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
