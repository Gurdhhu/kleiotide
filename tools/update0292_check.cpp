#include "../tests/abif_fixture.hpp"
#include "core/session.hpp"
#include "gui/alignment_view.hpp"
#include "gui/app_settings.hpp"
#include "gui/blast_dialog.hpp"
#include "gui/main_window.hpp"
#include "gui/rename_dialog.hpp"
#include "gui/spart.hpp"
#include "gui/table_io.hpp"
#include "test_preferences.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QTableView>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok, const char *text) {
  if (!ok)
    throw std::runtime_error(text);
}
template <class F> void until(F f) {
  QElapsedTimer t;
  t.start();
  while (!f() && t.elapsed() < 15000) {
    QApplication::processEvents();
    QThread::msleep(1);
  }
  need(f(), "Workflow timeout");
}
QAction *action(MainWindow &w, const QString &name) {
  for (auto *a : w.findChildren<QAction *>())
    if (a->objectName() == name)
      return a;
  throw std::runtime_error("Missing action " + name.toStdString());
}
template <class T> T *child(QObject *parent, const char *id) {
  auto *value = parent->findChild<T *>(id);
  need(value, id);
  return value;
}
void footer(QDialog *d) {
  QApplication::processEvents();
  for (auto *box : d->findChildren<QDialogButtonBox *>())
    if (box->parentWidget() == d && box->isVisible()) {
      const auto bottom = box->mapTo(d, QPoint(0, box->height())).y();
      need(bottom <= d->height(), "Footer remains within client area");
      need(d->height() - bottom <= 28,
           "No expandable blank strip below dialog footer");
    }
}
} // namespace
int main(int argc, char **argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication app(argc, argv);
  try {
    TestPreferences prefs;
    (void)AppSettings::instance();
    QTemporaryDir temp;
    const auto output =
        argc > 1 ? QString::fromLocal8Bit(argv[1]) : temp.path();
    QDir().mkpath(output);
    Document document;
    document.load({{"alpha", "ACDE", RowKind::Protein},
                   {"beta", "ACDF", RowKind::Protein},
                   {"gamma", "ACDE", RowKind::Protein}});
    const auto input = temp.filePath("protein.kleio");
    {
      std::ofstream out(input.toStdString(), std::ios::binary);
      write_session(out, document);
    }
    MainWindow owner;
    owner.show();
    owner.open_path(input);
    auto *view = dynamic_cast<AlignmentView *>(owner.centralWidget());
    until([&] {
      return view->isEnabled() && owner.current_document().rows().size() == 3;
    });
    // Statistics molecule selector recomputes an immutable alignment snapshot.
    action(owner, "alignmentStatisticsAction")->trigger();
    auto *stats = child<QDialog>(&owner, "alignmentStatistics");
    auto *molecule = child<QComboBox>(stats, "statisticsMolecule");
    until([&] {
      return child<QPushButton>(stats, "statisticsCopy")->isEnabled();
    });
    need(molecule->currentIndex() == 1 &&
             child<QLabel>(stats, "statisticsValue0")->text() == "3",
         "Protein statistics are selected for protein alignment");
    molecule->setCurrentIndex(0);
    until([&] {
      return child<QPushButton>(stats, "statisticsCopy")->isEnabled();
    });
    need(child<QLabel>(stats, "statisticsValue0")->text() == "0",
         "Absent molecule statistics are empty");
    molecule->setCurrentIndex(1);
    until([&] {
      return child<QPushButton>(stats, "statisticsCopy")->isEnabled();
    });
    stats->close();
    QApplication::processEvents();
    // Method descriptions change independently of scope validation.
    QString failure;
    bool inspected = false;
    QTimer::singleShot(0, &owner, [&] {
      try {
        auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        need(d, "Distance dialog");
        auto *type = child<QComboBox>(d, "distanceMolecule");
        type->setCurrentIndex(0);
        auto *method = child<QComboBox>(d, "distanceMethod");
        method->setCurrentIndex(0);
        const auto before = child<QLabel>(d, "distanceAssumptions")->text();
        method->setCurrentIndex(1);
        need(before != child<QLabel>(d, "distanceAssumptions")->text() &&
                 child<QLabel>(d, "distanceAssumptions")
                     ->text()
                     .contains("p-distance"),
             "Method explanation refreshes without eligible nucleotide rows");
        inspected = true;
        d->reject();
      } catch (const std::exception &e) {
        failure = e.what();
        if (auto *d =
                qobject_cast<QDialog *>(QApplication::activeModalWidget()))
          d->reject();
      }
    });
    action(owner, "pairwiseDistances")->trigger();
    need(inspected && failure.isEmpty(), qPrintable(failure));
    // Both search windows launched by the real menu are modeless.
    action(owner, "nucleotideBlast")->trigger();
    auto *remote = child<QDialog>(&owner, "blastDialog");
    QApplication::processEvents();
    need(remote->windowModality() == Qt::NonModal && view->isEnabled(),
         "NCBI window leaves alignment editable");
    footer(remote);
    remote->grab().save(output + "/remote-options.png");
    const auto available = remote->screen()->availableGeometry();
    need(remote->frameGeometry().height() <= available.height() &&
             remote->width() <= available.width(),
         "NCBI dialog fits available screen");
    remote->close();
    delete remote;
    QApplication::processEvents();
    action(owner, "localProteinBlast")->trigger();
    auto *local = child<QDialog>(&owner, "localBlastDialog");
    QApplication::processEvents();
    need(local->windowModality() == Qt::NonModal && view->isEnabled(),
         "Local window leaves alignment editable");
    footer(local);
    local->grab().save(output + "/local-options.png");
    need(local->frameGeometry().height() <= available.height(),
         "Local BLAST dialog fits screen");
    local->close();
    delete local;
    QApplication::processEvents();
    // Compact prompts cannot grow below their footer.
    bool compact = false;
    QTimer::singleShot(40, &owner, [&] {
      auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try {
        need(d, "Grouping prompt");
        footer(d);
        if (!(d->height() < 350 && d->minimumHeight() == d->maximumHeight()))
          throw std::runtime_error(
              QString(
                  "Compact prompt: height=%1 min=%2 max=%3 name=%4 policy=%5")
                  .arg(d->height())
                  .arg(d->minimumHeight())
                  .arg(d->maximumHeight())
                  .arg(d->objectName())
                  .arg(d->property("followupPolicy").toBool())
                  .toStdString());
        d->grab().save(output + "/compact-group.png");
        compact = true;
      } catch (const std::exception &e) {
        failure = e.what();
      }
      if (d)
        d->reject();
    });
    action(owner, "groupIdentical")->trigger();
    need(compact && failure.isEmpty(), qPrintable(failure));
    // Copy table exports only the automatically updated preview and all columns
    // resize.
    auto source = std::make_shared<Document>();
    source->load({{"alpha", "ACGT"}, {"beta", "ACGT"}});
    QElapsedTimer previewTime;
    previewTime.start();
    QTimer driver;
    driver.setInterval(10);
    bool renamed = false;
    QObject::connect(&driver, &QTimer::timeout, [&] {
      try {
        need(previewTime.elapsed() < 10000, "Rename preview timeout");
        auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!d)
          return;
        auto *field = child<QLineEdit>(d, "renameText");
        if (field->text() != "_new") {
          field->setText("_new");
          return;
        }
        if (!child<QPushButton>(d, "renameApplyButton")->isEnabled())
          return;
        auto *table = child<QTableView>(d, "renamePreview");
        for (int c = 0; c < table->model()->columnCount(); ++c)
          need(table->horizontalHeader()->sectionResizeMode(c) ==
                   QHeaderView::Interactive,
               "Every preview column resizes");
        child<QPushButton>(d, "copyPreviewTable")->click();
        const auto copied = parse_text_table(QApplication::clipboard()->text());
        need(copied.size() == std::size_t(table->model()->rowCount() + 1) &&
                 copied[0].size() == table->model()->columnCount(),
             "Clipboard contains only preview rows and headers");
        renamed = true;
        driver.stop();
        d->reject();
      } catch (const std::exception &e) {
        failure = e.what();
        driver.stop();
        if (auto *d =
                qobject_cast<QDialog *>(QApplication::activeModalWidget()))
          d->reject();
      }
    });
    driver.start();
    show_batch_rename(*source, {1, 2}, {}, RenameOperation::Append, &owner);
    need(renamed && failure.isEmpty(), qPrintable(failure));
    // A real pending HTTP request operates on a snapshot and has a visible
    // status.
    QTcpServer server;
    need(server.listen(QHostAddress::LocalHost), "Loopback server");
    int requests = 0;
    bool snapshotQuery = false;
    QObject::connect(&server, &QTcpServer::newConnection, [&] {
      while (auto *socket = server.nextPendingConnection()) {
        auto data = std::make_shared<QByteArray>();
        QObject::connect(socket, &QTcpSocket::readyRead, [&, socket, data] {
          *data += socket->readAll();
          auto header = data->indexOf("\r\n\r\n");
          if (header < 0)
            return;
          int length = 0;
          for (auto line : data->left(header).split('\n'))
            if (line.toLower().startsWith("content-length:"))
              length = line.mid(15).trimmed().toInt();
          if (data->size() < header + 4 + length)
            return;
          ++requests;
          const bool post = data->startsWith("POST");
          if (post) {
            const auto body = data->mid(header + 4, length);
            snapshotQuery = body.contains("ACGT") && !body.contains("TTTT");
          }
          const QByteArray body =
              post ? "RID = SNAPSHOT-RID\nRTOE = 0\n" : "Status = WAITING";
          socket->write(
              "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: " +
              QByteArray::number(body.size()) + "\r\n\r\n" + body);
          socket->disconnectFromHost();
          QObject::connect(socket, &QTcpSocket::disconnected, socket,
                           &QObject::deleteLater);
        });
      }
    });
    BlastServices services;
    services.search =
        QUrl(QString("http://127.0.0.1:%1/search").arg(server.serverPort()));
    services.records = services.search;
    services.contactMs = 1;
    services.pollMs = 5;
    services.jobDirectory = temp.filePath("jobs");
    blast_dialog(source, {1}, false, &owner, services, true);
    auto *search = child<QDialog>(&owner, "blastDialog");
    source->replace(1, 0, 4, "TTTT");
    child<QLineEdit>(search, "blastEmail")->setText("test@example.org");
    QTimer yes;
    yes.setInterval(2);
    QObject::connect(&yes, &QTimer::timeout, [&] {
      if (auto *box =
              qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
        if (auto *button = box->button(QMessageBox::Yes))
          button->click();
    });
    yes.start();
    child<QPushButton>(search, "blastStart")->click();
    until([&] { return requests >= 2; });
    need(snapshotQuery && view->isEnabled() &&
             search->windowModality() == Qt::NonModal,
         "Pending search retains original query snapshot and interactive "
         "session");
    need(child<QLabel>(search, "blastStatusBadge")->text().contains("progress"),
         "Pending status badge");
    search->reject();
    need(
        child<QLabel>(search, "blastStatusBadge")->text().contains("Cancelled"),
        "Cancellation status badge");
    search->close();
    yes.stop();
    // Confirming the real Link trace action opens its session chromatogram.
    const std::string calls = "ACGTTGCACTAGATCGACCTTGGA";
    Document nucleotide;
    nucleotide.load({{"alpha", calls}});
    const auto sessionPath = temp.filePath("nucleotide.kleio");
    {
      std::ofstream out(sessionPath.toStdString(), std::ios::binary);
      write_session(out, nucleotide);
    }
    owner.open_path(sessionPath);
    until([&] {
      return view->isEnabled() && owner.current_document().rows().size() == 1;
    });
    view->select_ids({1});
    const auto ab1Path = temp.filePath("alpha.ab1");
    {
      QFile file(ab1Path);
      need(file.open(QIODevice::WriteOnly), "Trace fixture file");
      const auto bytes = test::synthetic_abif(calls);
      file.write(bytes.data(), qsizetype(bytes.size()));
    }
    int traceStage = 0;
    QTimer picker;
    picker.setInterval(5);
    QObject::connect(&picker, &QTimer::timeout, [&] {
      if (auto *file =
              qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
        file->selectFile(ab1Path);
        QMetaObject::invokeMethod(file, "accept", Qt::QueuedConnection);
      }
    });
    picker.start();
    QTimer linker;
    linker.setInterval(10);
    QObject::connect(&linker, &QTimer::timeout, [&] {
      try {
        auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (auto *scope = qobject_cast<QInputDialog *>(d)) {
          scope->accept();
          return;
        }
        if (!d || d->objectName() != "traceLinkDialog")
          return;
        if (traceStage == 0) {
          traceStage = 1;
          child<QPushButton>(d, "traceLinkAdd")->click();
        } else if (traceStage == 1 &&
                   child<QPushButton>(d, "traceLinkApply")->isEnabled()) {
          traceStage = 2;
          linker.stop();
          child<QPushButton>(d, "traceLinkApply")->click();
        }
      } catch (const std::exception &e) {
        failure = e.what();
        linker.stop();
        if (auto *d =
                qobject_cast<QDialog *>(QApplication::activeModalWidget()))
          d->reject();
      }
    });
    linker.start();
    action(owner, "linkTrace")->trigger();
    picker.stop();
    need(traceStage == 2 && failure.isEmpty(), qPrintable(failure));
    need(child<QDockWidget>(&owner, "chromatogramDock")->isVisible(),
         "Trace view activates after confirmed linking");
    // Ordinary group tables preserve exact names for subsequent import.
    Document groupSource;
    groupSource.load({{"=alpha, name", "ACGT"}});
    groupSource.apply_groups({{1, "-Group", 0x123456}}, {{1, 1}});
    for (const auto &extension : QStringList{"csv", "xlsx", "tsv"}) {
      const auto path = temp.filePath("groups." + extension);
      QTimer savePicker;
      savePicker.setInterval(5);
      QObject::connect(&savePicker, &QTimer::timeout, [&] {
        if (auto *file = qobject_cast<QFileDialog *>(
                QApplication::activeModalWidget())) {
          savePicker.stop();
          file->selectNameFilter(
              extension == "csv"    ? "Comma-separated table (*.csv)"
              : extension == "xlsx" ? "Excel workbook (*.xlsx)"
                                    : "Tab-separated table (*.tsv)");
          file->selectFile(path);
          QMetaObject::invokeMethod(file, "accept", Qt::QueuedConnection);
        }
      });
      savePicker.start();
      export_sequence_groups(groupSource, &owner);
      TextTable table;
      if (extension == "xlsx")
        table = read_xlsx_table(path);
      else {
        QFile file(path);
        need(file.open(QIODevice::ReadOnly), "Group table export exists");
        table = parse_text_table(QString::fromUtf8(file.readAll()),
                                 extension == "csv" ? ',' : '\t');
      }
      need(table.size() == 2 && table[1][0] == "=alpha, name" &&
               table[1][1] == "-Group",
           "Group export names roundtrip without SPART sanitation or escaped "
           "text prefixes");
    }
    need(spart_sanitize(" Alpha Beta ") == "_alpha_beta_",
         "SPART whitespace and lowercase sanitation");
    std::cout << "PASS 0.29.2 modeless snapshot searches, status, compact "
                 "geometry, protein stats, empty-scope explanations, "
                 "resize/copy preview and sanitation\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
  return 0;
}
