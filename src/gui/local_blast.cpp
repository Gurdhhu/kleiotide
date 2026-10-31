#include "gui/local_blast.hpp"
#include "core/fasta.hpp"
#include "core/session.hpp"
#include "core/translation.hpp"
#include "gui/app_settings.hpp"
#include "gui/blast_data.hpp"
#include "gui/file_output.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSaveFile>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableView>
#include <QTemporaryDir>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
namespace kleiotide {
namespace {
[[noreturn]] void bad(const QString &message) {
  throw std::runtime_error(message.toStdString());
}
QProcessEnvironment local_environment() {
  auto env = QProcessEnvironment::systemEnvironment();
  env.insert("BLAST_USAGE_REPORT", "false");
  return env;
}
} // namespace
QString local_blast_executable(const QString &name) {
  const auto &p = AppSettings::instance().values();
  QString chosen;
  if (name == "blastn")
    chosen = p.blastnLauncher;
  else if (name == "blastp")
    chosen = p.blastpLauncher;
  else if (name == "blastx")
    chosen = p.blastxLauncher;
  else if (name == "tblastn")
    chosen = p.tblastnLauncher;
  else if (name == "tblastx")
    chosen = p.tblastxLauncher;
  else if (name == "makeblastdb")
    chosen = p.makeblastdbLauncher;
  if (!chosen.isEmpty())
    return chosen;
  QString binary = name;
#ifdef Q_OS_WIN
  binary += ".exe";
#endif
  for (const auto &path :
       {p.blastnLauncher, p.blastpLauncher, p.makeblastdbLauncher})
    if (!path.isEmpty()) {
      auto sibling = QFileInfo(path).dir().filePath(binary);
      if (QFileInfo(sibling).isExecutable())
        return sibling;
    }
  return QStandardPaths::findExecutable(binary);
}
LocalDatabase read_local_database(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly) || file.size() > 16 * 1024 * 1024)
    bad("Cannot read database description, or it exceeds 16 MiB.");
  QJsonParseError error;
  auto j = QJsonDocument::fromJson(file.readAll(), &error).object();
  if (error.error != QJsonParseError::NoError ||
      j["format"] != "Kleiotide local BLAST database" || j["version"] != 1 ||
      !j["protein"].isBool())
    bad("Invalid local database description.");
  LocalDatabase db;
  db.manifest = QFileInfo(path).absoluteFilePath();
  db.prefix = QFileInfo(path).dir().absoluteFilePath(j["prefix"].toString());
  db.title = j["title"].toString();
  db.protein = j["protein"].toBool();
  db.names = j["names"].toObject();
  if (j["prefix"].toString().isEmpty() ||
      !QFileInfo::exists(db.prefix + (db.protein ? ".pin" : ".nin")))
    bad("Database files are missing. Keep the .kblast file beside its .files "
        "folder.");
  return db;
}
QStringList local_database_paths(bool protein) {
  QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                     "Kleiotide", "Kleiotide");
  QStringList result;
  for (const auto &path :
       settings.value("local-blast/databases").toStringList())
    try {
      if (read_local_database(path).protein == protein)
        result << path;
    } catch (...) {
    }
  return result;
}
void remember_local_database(const QString &path) {
  auto db = read_local_database(path);
  QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                     "Kleiotide", "Kleiotide");
  auto paths = settings.value("local-blast/databases").toStringList();
  paths.removeAll(db.manifest);
  paths.prepend(db.manifest);
  while (paths.size() > 200)
    paths.removeLast();
  settings.setValue("local-blast/databases", paths);
  settings.sync();
  if (settings.status() != QSettings::NoError)
    bad("Database created, but its recent location could not be saved.");
}
QJsonObject prepare_local_database(const QString &path, bool protein,
                                   const QString &output) {
  std::ifstream input(
      std::filesystem::path(std::u8string(
          reinterpret_cast<const char8_t *>(path.toUtf8().constData()))),
      std::ios::binary);
  if (!input)
    bad("Cannot open the source alignment.");
  Document document;
  if (QFileInfo(path).suffix().compare("kleio", Qt::CaseInsensitive) == 0)
    document = std::move(read_session(input).document);
  else
    document.load(read_fasta(input, {},
                             protein ? FastaImport::ProteinAndMasks
                                     : FastaImport::NucleotideAndMasks));
  QFile fasta(output);
  if (!fasta.open(QIODevice::WriteOnly))
    bad("Cannot write the temporary database FASTA.");
  QJsonObject names;
  for (const auto &row : document.rows())
    if (row.kind == (protein ? RowKind::Protein : RowKind::Nucleotide)) {
      QByteArray sequence;
      for (char c : row.symbols.text())
        if (!is_gap(c)) {
          c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
          sequence += c == '?' ? (protein ? 'X' : 'N') : c;
        }
      if (sequence.isEmpty())
        continue;
      auto id = "kseq_" + QString::number(names.size() + 1);
      names[id] = QString::fromStdString(row.name);
      auto record = '>' + id.toUtf8() + '\n' + sequence + '\n';
      if (fasta.write(record) != record.size())
        bad("Cannot write database sequences.");
      if (names.size() > 100000)
        bad("A local database supports at most 100,000 named sequences.");
    }
  if (names.isEmpty())
    bad("The source contains no nonempty sequences of the chosen molecule.");
  fasta.close();
  return names;
}
namespace {
class BuildDatabase final : public QDialog {
  QProcess process;
  QPlainTextEdit *report;
  QPushButton *close;
  QString target, dataFolder;
  bool protein, running = true, closing = false, preparing = true;
  std::unique_ptr<QTemporaryDir> temp;
  QJsonObject names;
  QTimer heartbeat;
  qint64 started = QDateTime::currentMSecsSinceEpoch();

public:
  BuildDatabase(QString source, bool p, QString output, QWidget *parent)
      : QDialog(parent), target(std::move(output)),
        dataFolder(target + ".files"), protein(p) {
    setWindowTitle("Create local BLAST database");
    setObjectName("localDatabaseBuild");
    setWindowModality(Qt::WindowModal);
    resize(750, 450);
    auto *layout = new QVBoxLayout(this);
    auto *message =
        new QLabel("Preparing sequences and building the local database…");
    message->setWordWrap(true);
    layout->addWidget(message);
    report = new QPlainTextEdit;
    report->setReadOnly(true);
    layout->addWidget(report, 1);
    auto *buttons = new QDialogButtonBox;
    close = buttons->addButton("Cancel", QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);
    connect(close, &QPushButton::clicked, this, &BuildDatabase::reject);
    process.setProcessEnvironment(local_environment());
    temp = std::make_unique<QTemporaryDir>(
        QFileInfo(target).dir().filePath(".kleiotide-db-XXXXXX"));
    if (!temp->isValid()) {
      preparing = false;
      stop("Cannot create a temporary database folder.");
      return;
    }
    connect(&process, &QProcess::readyReadStandardError, this, [this] {
      append(QString::fromUtf8(process.readAllStandardError()));
    });
    connect(&process, &QProcess::readyReadStandardOutput, this, [this] {
      append(QString::fromUtf8(process.readAllStandardOutput()));
    });
    connect(&process, &QProcess::errorOccurred, this, [this](auto error) {
      if (error == QProcess::FailedToStart)
        stop("Cannot start makeblastdb: " + process.errorString());
    });
    connect(
        &process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        this, [this, message](int code, auto state) {
          if (!running)
            return;
          if (code || state != QProcess::NormalExit) {
            stop("Database creation failed; see the program output above.");
            return;
          }
          try {
            QFile::remove(temp->filePath("source.fa"));
            if (QFileInfo::exists(target) || QFileInfo::exists(dataFolder))
              bad("The output path was created by another operation. Choose a "
                  "new name.");
            if (!QDir().rename(temp->path(), dataFolder))
              bad("Cannot place the completed database folder.");
            QSaveFile file(target);
            auto prefix = QFileInfo(dataFolder).fileName() + "/db";
            auto bytes =
                QJsonDocument(
                    QJsonObject{{"format", "Kleiotide local BLAST database"},
                                {"version", 1},
                                {"protein", protein},
                                {"title", QFileInfo(target).completeBaseName()},
                                {"prefix", prefix},
                                {"names", names}})
                    .toJson();
            if (!file.open(QIODevice::WriteOnly) ||
                file.write(bytes) != bytes.size() || !file.commit()) {
              QDir(dataFolder).removeRecursively();
              bad("Cannot save the database description.");
            }
            remember_local_database(target);
            message->setText(QString("Created %1 database with %2 sequences. "
                                     "It is now available in Local BLAST.")
                                 .arg(protein ? "protein" : "nucleotide")
                                 .arg(names.size()));
            stop("Saved: " + target);
          } catch (const std::exception &e) {
            stop(e.what());
          }
        });
    heartbeat.setInterval(1000);
    connect(&heartbeat, &QTimer::timeout, this, [this, message] {
      if (!running)
        return;
      const auto seconds =
          (QDateTime::currentMSecsSinceEpoch() - started) / 1000;
      message->setText(QString("Creating local database · elapsed %1 min %2 s")
                           .arg(seconds / 60)
                           .arg(seconds % 60));
      if (seconds >=
          qint64(AppSettings::instance().values().localBlastMinutes) * 60) {
        process.kill();
        stop("Database creation reached the configured local time limit.");
      }
    });
    heartbeat.start();
    struct Prepared {
      QJsonObject names;
      QString error;
    };
    auto *watcher = new QFutureWatcher<Prepared>(this);
    const auto fasta = temp->filePath("source.fa");
    connect(watcher, &QFutureWatcher<Prepared>::finished, this,
            [this, watcher] {
              auto result = watcher->result();
              watcher->deleteLater();
              preparing = false;
              if (closing) {
                running = false;
                QDialog::reject();
                return;
              }
              if (!result.error.isEmpty()) {
                stop(result.error);
                return;
              }
              if (!running)
                return;
              names = std::move(result.names);
              const auto executable = local_blast_executable("makeblastdb");
              if (executable.isEmpty()) {
                stop("Set the makeblastdb executable in Settings → BLAST → "
                     "Local BLAST.");
                return;
              }
              process.setProgram(executable);
              process.setArguments({"-in", temp->filePath("source.fa"),
                                    "-dbtype", protein ? "prot" : "nucl",
                                    "-parse_seqids", "-blastdb_version", "4",
                                    "-out", temp->filePath("db"), "-title",
                                    QFileInfo(target).completeBaseName()});
              append(executable + " " + process.arguments().join(' '));
              process.start();
            });
    watcher->setFuture(QtConcurrent::run([source, p, fasta] {
      Prepared result;
      try {
        result.names = prepare_local_database(source, p, fasta);
      } catch (const std::exception &e) {
        result.error = e.what();
      }
      return result;
    }));
  }
  ~BuildDatabase() override {
    process.disconnect(this);
    if (process.state() != QProcess::NotRunning) {
      process.kill();
      process.waitForFinished(1000);
    }
  }
  void append(const QString &text) { report->appendPlainText(text.left(8192)); }
  void stop(const QString &message) {
    running = false;
    heartbeat.stop();
    close->setText("Close");
    append(message);
  }
  void reject() override {
    if (preparing) {
      closing = true;
      close->setEnabled(false);
      return;
    }
    if (running) {
      closing = true;
      if (process.state() != QProcess::NotRunning) {
        running = false;
        process.kill();
        process.waitForFinished(1000);
        QDialog::reject();
      } else {
        close->setEnabled(false);
        append("Finishing the current file read before closing…");
      }
      return;
    }
    QDialog::reject();
  }
};
class WideDatabaseCombo final:public QComboBox{public:void showPopup()override{int width=this->width();for(int i=0;i<count();++i)width=std::max(width,fontMetrics().horizontalAdvance(itemText(i))+48);if(screen())width=std::min(width,screen()->availableGeometry().width()-32);view()->setMinimumWidth(width);view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);view()->setTextElideMode(Qt::ElideNone);QComboBox::showPopup();}};
class LocalSearch final : public QDialog {
  std::shared_ptr<Document> source;
  std::vector<SequenceId> selected;
  QTabWidget *tabs;
  QWidget *controls;
  QComboBox *program, *database, *scope, *task, *matrix, *strand, *queryCode,
      *dbCode;
  QLineEdit *launcher, *expect;
  QSpinBox *maxHits, *word, *reward, *penalty, *gapOpen, *gapExtend,
      *composition, *threads, *minutes;
  QDoubleSpinBox *identity, *coverage;
  QCheckBox *mask, *soft;
  QLabel *status;
  QPlainTextEdit *diagnostics;
  QTableView *table;
  QStandardItemModel model;
  QPushButton *start, *cancel, *tsv, *xlsx;
  QProcess process;
  QTimer timer;
  std::unique_ptr<QTemporaryDir> temp;
  QByteArray output, errorOutput;
  BlastOptions options;
  LocalDatabase db;
  std::vector<BlastQuery> queries;
  std::vector<BlastHit> hits;
  BlastTable rows;
  bool running = false, checking = false;
  qint64 started = 0;

public:
  LocalSearch(std::shared_ptr<Document> d, std::vector<SequenceId> ids,
              bool protein, QWidget *parent)
      : QDialog(parent), source(std::move(d)), selected(std::move(ids)) {
    setObjectName("localBlastDialog");
    setWindowTitle("Local BLAST");
    setWindowModality(Qt::WindowModal);
    resize(1040, 800);
    auto *layout = new QVBoxLayout(this);
    tabs = new QTabWidget;
    layout->addWidget(tabs, 1);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    controls = new QWidget;
    scroll->setWidget(controls);
    tabs->addTab(scroll, "Search options");
    auto *fields = new QVBoxLayout(controls);
    auto section = [&](const QString &text) {
      auto *box = new QGroupBox(text);
      auto *form = new QFormLayout(box);
      form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
      form->setRowWrapPolicy(QFormLayout::WrapLongRows);
      fields->addWidget(box);
      return form;
    };
    auto *form = section("Queries and local database");
    auto combo = [&](const char *id, const QString &title,
                     const QStringList &values) {
      auto *w = new QComboBox;
      w->setObjectName(id);
      w->addItems(values);
      w->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      form->addRow(title, w);
      return w;
    };
    auto integer = [&](const char *id, const QString &title, int low, int high,
                       int value) {
      auto *w = new QSpinBox;
      w->setObjectName(id);
      w->setRange(low, high);
      w->setValue(value);
      w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      form->addRow(title, w);
      return w;
    };
    program = combo("localBlastProgram", "Search type",
                    {"blastn", "blastp", "blastx", "tblastn", "tblastx"});
    program->setToolTip(
        "blastn: DNA → DNA; blastp: protein → protein; blastx: translated DNA "
        "→ protein; tblastn: protein → translated DNA; tblastx: translated DNA "
        "→ translated DNA.");
    scope = combo("localBlastScope", "Query scope",
                  {"Selected names", "All matching rows"});
    scope->setCurrentIndex(selected.empty() ? 1 : 0);
    database = new WideDatabaseCombo;
    database->setObjectName("localBlastDatabase");
    database->setMinimumContentsLength(25);
    database->setSizeAdjustPolicy(
        QComboBox::AdjustToMinimumContentsLengthWithIcon);
    database->setMaximumWidth(440);
    auto *dbRow = new QWidget;
    auto *dbLayout = new QHBoxLayout(dbRow);
    dbLayout->setContentsMargins(0, 0, 0, 0);
    dbLayout->addWidget(database, 1);
    auto *browseDb = new QPushButton("Browse…");
    browseDb->setToolTip("Open a .kblast database created by Kleiotide.");
    dbLayout->addWidget(browseDb);
    form->addRow("Local database", dbRow);
    launcher = new QLineEdit;
    launcher->setObjectName("localBlastLauncher");
    launcher->setMinimumWidth(270);
    auto *exeRow = new QWidget;
    auto *exeLayout = new QHBoxLayout(exeRow);
    exeLayout->setContentsMargins(0, 0, 0, 0);
    exeLayout->addWidget(launcher, 1);
    auto *browseExe = new QPushButton("Browse…");
    exeLayout->addWidget(browseExe);
    auto *exeLabel = new QLabel;
    form->addRow(exeLabel, exeRow);
    launcher->setToolTip(
        "Path to the NCBI executable for the selected search type.");
    task = combo("localBlastTask", "Task", {});
    auto *taskForm = form;
    auto *scoring = section("Scoring and sequence interpretation");
    form = scoring;
    word = integer("localBlastWord", "Word size (0 = task default)", 0, 256, 0);
    word->setSpecialValueText("Task default");
    reward = integer("localBlastReward", "Match reward", 1, 5, 1);
    penalty = integer("localBlastPenalty", "Mismatch penalty", -10, -1, -2);
    gapOpen =
        integer("localBlastGapOpen", "Gap open (both 0 = default)", 0, 100, 0);
    gapExtend = integer("localBlastGapExtend", "Gap extension", 0, 100, 0);
    matrix = combo("localBlastMatrix", "Scoring matrix",
                   {"BLOSUM62", "BLOSUM45", "BLOSUM50", "BLOSUM80", "BLOSUM90",
                    "PAM30", "PAM70", "PAM250"});
    composition =
        integer("localBlastComposition", "Composition statistics", 0, 3, 2);
    strand =
        combo("localBlastStrand", "Query strand", {"both", "plus", "minus"});
    queryCode = combo("localBlastQueryCode", "Query genetic code", {});
    dbCode = combo("localBlastDbCode", "Database genetic code", {});
    for (const auto &code : genetic_codes())
      for (auto *w : {queryCode, dbCode})
        w->addItem(QString::number(code.id) + " — " +
                       QString::fromStdString(code.name),
                   int(code.id));
    queryCode->setMaximumWidth(330);
    dbCode->setMaximumWidth(330);
    form = section("Targets, filters and resources");
    const auto &preferences = AppSettings::instance().values();
    maxHits = integer("localBlastHits", "Maximum target sequences", 1, 5000,
                      preferences.localBlastHits);
    expect =
        new QLineEdit(QString::number(preferences.localBlastExpect, 'g', 12));
    expect->setObjectName("localBlastExpect");
    expect->setMaximumWidth(170);
    form->addRow("E-value threshold", expect);
    auto percentage = [&](const char *id, const QString &title,
                          double initial) {
      auto *w = new QDoubleSpinBox;
      w->setObjectName(id);
      w->setRange(0, 100);
      w->setDecimals(2);
      w->setValue(initial);
      w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      form->addRow(title, w);
      return w;
    };
    identity = percentage("localBlastIdentity", "Minimum identity (%)",
                          preferences.localBlastIdentity);
    coverage = percentage("localBlastCover", "Minimum query cover (%)",
                          preferences.localBlastCoverage);
    mask = new QCheckBox("Low-complexity filtering");
    soft = new QCheckBox("Mask during seed lookup only");
    soft->setChecked(true);
    form->addRow(mask);
    form->addRow(soft);
    threads = integer("localBlastThreads", "CPU threads", 1, 256,
                      preferences.localBlastThreads);
    minutes = integer("localBlastMinutes", "Maximum running time (min)", 1,
                      1440, preferences.localBlastMinutes);
    fields->addStretch();
    auto *results = new QWidget;
    auto *resultLayout = new QVBoxLayout(results);
    tabs->addTab(results, "Results");
    auto *note =
        new QLabel("Identity uses the best-scoring HSP. Query cover includes "
                   "all HSP spans. Filters apply to the received hits.");
    note->setWordWrap(true);
    resultLayout->addWidget(note);
    table = new QTableView;
    table->setObjectName("localBlastResults");
    table->setModel(&model);
    table->setSortingEnabled(true);
    table->setWordWrap(false);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setDefaultSectionSize(24);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    resultLayout->addWidget(table, 1);
    status = new QLabel("Choose a local database and search type.");
    status->setObjectName("localBlastStatus");
    status->setWordWrap(true);
    resultLayout->addWidget(status);
    diagnostics = new QPlainTextEdit;
    diagnostics->setObjectName("localBlastDiagnostics");
    diagnostics->setReadOnly(true);
    diagnostics->setMaximumBlockCount(400);
    diagnostics->setMaximumHeight(130);
    resultLayout->addWidget(diagnostics);
    auto *buttons = new QDialogButtonBox;
    start = buttons->addButton("Search local database",
                               QDialogButtonBox::ActionRole);
    start->setObjectName("localBlastStart");
    cancel = buttons->addButton("Close", QDialogButtonBox::RejectRole);
    tsv = buttons->addButton("Save TSV…", QDialogButtonBox::ActionRole);
    xlsx = buttons->addButton("Save XLSX…", QDialogButtonBox::ActionRole);
    auto *report = buttons->addButton("Save diagnostic report…",
                                      QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);
    tsv->setEnabled(false);
    xlsx->setEnabled(false);
    start->setToolTip("Search the chosen database on this computer.");
    report->setToolTip("Save executable details, timing and program messages.");
    auto refill = [this] {
      database->clear();
      const bool targetProtein = program->currentText() == "blastp" ||
                                 program->currentText() == "blastx";
      for (const auto &path : local_database_paths(targetProtein)) {
        auto databaseInfo = read_local_database(path);
        database->addItem(
            databaseInfo.title + " — " + QFileInfo(path).fileName(), path);
        database->setItemData(database->count() - 1, path, Qt::ToolTipRole);
      }
    };
    auto configure = [this, exeLabel, taskForm, scoring, refill] {
      const auto p = program->currentText();
      const bool nucleotide = p == "blastn";
      const auto &pref = AppSettings::instance().values();
      launcher->setText(local_blast_executable(p));
      exeLabel->setText("NCBI " + p + " executable");
      task->clear();
      task->addItems(
          nucleotide ? QStringList{"megablast", "blastn", "dc-megablast",
                                   "blastn-short"}
                     : QStringList{"blastp", "blastp-short", "blastp-fast"});
      taskForm->setRowVisible(task, nucleotide || p == "blastp");
      for (auto *w : {reward, penalty})
        scoring->setRowVisible(w, nucleotide);
      scoring->setRowVisible(matrix, !nucleotide);
      scoring->setRowVisible(composition, !nucleotide && p != "tblastx");
      scoring->setRowVisible(strand,
                             p == "blastn" || p == "blastx" || p == "tblastx");
      scoring->setRowVisible(queryCode, p == "blastx" || p == "tblastx");
      scoring->setRowVisible(dbCode, p == "tblastn" || p == "tblastx");
      scoring->setRowVisible(gapOpen, p != "tblastx");
      scoring->setRowVisible(gapExtend, p != "tblastx");
      mask->setText(nucleotide ? "Low-complexity filtering (DUST)"
                               : "Low-complexity filtering (SEG)");
      mask->setChecked(nucleotide ? pref.localBlastNmask
                                  : pref.localBlastPmask);
      word->setValue(0);
      refill();
    };
    connect(program, &QComboBox::currentTextChanged, this,
            [configure] { configure(); });
    connect(
        task, &QComboBox::currentTextChanged, this, [this](const QString &t) {
          word->setValue(0);
          gapOpen->setValue(0);
          gapExtend->setValue(0);
          reward->setValue(t == "blastn" ? 2 : 1);
          penalty->setValue(t == "blastn" || t == "blastn-short" ? -3 : -2);
          matrix->setCurrentText(t == "blastp-short" ? "PAM30" : "BLOSUM62");
          composition->setValue(t == "blastp-short" ? 0 : 2);
        });
    program->setCurrentText(protein ? "blastp" : "blastn");
    configure();
    connect(browseDb, &QPushButton::clicked, this, [this, refill] {
      auto path =
          QFileDialog::getOpenFileName(this, "Open local BLAST database", {},
                                       "Kleiotide local database (*.kblast)");
      if (path.isEmpty())
        return;
      try {
        const auto info = read_local_database(path);
        if (info.protein != (program->currentText() == "blastp" ||
                             program->currentText() == "blastx"))
          bad("Choose a database of the target molecule for this search type.");
        remember_local_database(path);
        refill();
      } catch (const std::exception &e) {
        QMessageBox::warning(this, "Local database", e.what());
      }
    });
    connect(browseExe, &QPushButton::clicked, this, [this] {
      auto path = QFileDialog::getOpenFileName(
          this, "Choose NCBI " + program->currentText(), launcher->text(),
          "Executables (*.exe);;All files (*)");
      if (!path.isEmpty())
        launcher->setText(path);
    });
    connect(start, &QPushButton::clicked, this, [this] { begin(); });
    connect(cancel, &QPushButton::clicked, this, &LocalSearch::reject);
    connect(tsv, &QPushButton::clicked, this, [this] { save(false); });
    connect(xlsx, &QPushButton::clicked, this, [this] { save(true); });
    connect(report, &QPushButton::clicked, this, [this] {
      auto path =
          choose_export_path(this, "Save local BLAST diagnostics", {},
                             "Text (*.txt)", OutputFormat::Table, source.get());
      if (path.isEmpty())
        return;
      auto bytes = diagnostics->toPlainText().toUtf8();
      auto error = write_export_atomic(
          path, OutputFormat::Table, source.get(),
          [&](auto &out) { out.write(bytes.constData(), bytes.size()); });
      if (!error.isEmpty())
        QMessageBox::warning(this, "Save diagnostics", error);
    });
    process.setProcessEnvironment(local_environment());
    connect(&process, &QProcess::readyReadStandardError, this, [this] {
      auto chunk = process.readAllStandardError();
      errorOutput += chunk;
      if (errorOutput.size() > 65536)
        errorOutput = errorOutput.right(65536);
      record(QString::fromUtf8(chunk));
    });
    connect(&process, &QProcess::readyReadStandardOutput, this, [this] {
      output += process.readAllStandardOutput();
      if (output.size() > 64 * 1024 * 1024)
        stop(
            "BLAST output exceeds 64 MiB. Reduce target count or query scope.");
    });
    connect(&process, &QProcess::errorOccurred, this, [this](auto error) {
      if (running && error == QProcess::FailedToStart)
        stop("Cannot start the executable: " + process.errorString());
    });
    connect(
        &process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        this, [this](int code, auto exit) {
          if (!running)
            return;
          output += process.readAllStandardOutput();
          if (code || exit != QProcess::NormalExit) {
            stop("Local BLAST failed: " + QString::fromUtf8(errorOutput));
            return;
          }
          try {
            if (checking) {
              checking = false;
              const auto version = QString::fromUtf8(output);
              if (!version.contains(options.program + ":", Qt::CaseInsensitive))
                bad("Select the NCBI " + options.program +
                    " executable. Version output: " + version.left(2048));
              record(version.trimmed());
              output.clear();
              errorOutput.clear();
              process.setArguments(
                  blast_arguments(options, temp->filePath("query.fa")));
              record("Arguments: " + process.arguments().join(' '));
              process.start();
              return;
            }
            hits = parse_blast(output, queries);
            for (auto &hit : hits) {
              auto key = hit.accession;
              if (key.startsWith("lcl|"))
                key = key.mid(4);
              if (db.names.contains(key))
                hit.accession = db.names[key].toString();
              else if (db.names.contains(hit.title.section(' ', 0, 0)))
                hit.accession =
                    db.names[hit.title.section(' ', 0, 0)].toString();
              else
                bad("A hit identifier is missing from the database's "
                    "sequence-name map.");
            }
            running = false;
            timer.stop();
            controls->setEnabled(true);
            start->setEnabled(true);
            cancel->setText("Close");
            refresh();
            status->setText(
                QString("Complete: %1 displayed hit rows; elapsed %2 s.")
                    .arg(rows.size())
                    .arg((QDateTime::currentMSecsSinceEpoch() - started) /
                         1000));
            record("Search complete.");
          } catch (const std::exception &e) {
            stop(e.what());
          }
        });
    timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, this, [this] {
      const auto elapsed =
          (QDateTime::currentMSecsSinceEpoch() - started) / 1000;
      status->setText(QString("%1 · elapsed %2 min %3 s")
                          .arg(checking ? "Checking executable"
                                        : "Searching local database")
                          .arg(elapsed / 60)
                          .arg(elapsed % 60));
      if (elapsed >= qint64(options.waitMinutes) * 60 ||
          (checking && elapsed >= 10))
        stop(checking ? "Executable version check timed out."
                      : "Local search reached its time limit.");
    });
    auto refilter = [this] {
      if (running || hits.empty())
        return;
      bool ok = false;
      double e = expect->text().toDouble(&ok);
      if (ok && e > 0) {
        options.expect = e;
        options.identity = identity->value();
        options.coverage = coverage->value();
        refresh();
      }
    };
    connect(expect, &QLineEdit::textChanged, this, [refilter] { refilter(); });
    connect(identity, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [refilter] { refilter(); });
    connect(coverage, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [refilter] { refilter(); });
    if (auto *screen = this->screen()) {
      auto size = screen->availableGeometry().size();
      resize(std::min(1040, size.width() - 40),
             std::min(850, size.height() - 60));
    }
  }
  ~LocalSearch() override {
    process.disconnect(this);
    if (process.state() != QProcess::NotRunning) {
      process.kill();
      process.waitForFinished(1000);
    }
  }
  void record(const QString &message) {
    if (!message.isEmpty())
      diagnostics->appendPlainText(
          QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "  " +
          message.left(8192));
  }
  void stop(const QString &message) {
    running = false;
    checking = false;
    timer.stop();
    if (process.state() != QProcess::NotRunning) {
      process.kill();
      process.waitForFinished(1000);
    }
    controls->setEnabled(true);
    start->setEnabled(true);
    cancel->setText("Close");
    status->setText(message);
    record(message);
    tsv->setEnabled(!rows.empty());
    xlsx->setEnabled(!rows.empty());
  }
  void reject() override {
    if (running) {
      stop("Cancelled. Completed results are retained.");
      return;
    }
    QDialog::reject();
  }
  void begin() {
    try {
      if (database->currentIndex() < 0)
        bad("Create or select a local database first.");
      options = BlastOptions{};
      options.local = true;
      options.program = program->currentText();
      options.protein =
          options.program == "blastp" || options.program == "tblastn";
      db = read_local_database(database->currentData().toString());
      if (db.protein !=
          (options.program == "blastp" || options.program == "blastx"))
        bad("The database molecule does not match the selected search.");
      options.database = db.prefix;
      options.launcher = launcher->text().trimmed();
      options.task = task->currentText();
      options.maxHits = maxHits->value();
      bool ok = false;
      options.expect = expect->text().toDouble(&ok);
      if (!ok)
        bad("Enter a numeric E value, such as 0.05 or 1e-10.");
      options.identity = identity->value();
      options.coverage = coverage->value();
      options.word = word->value();
      options.reward = reward->value();
      options.penalty = penalty->value();
      options.gapOpen = gapOpen->value();
      options.gapExtend = gapExtend->value();
      options.matrix = matrix->currentText();
      options.composition = composition->value();
      options.strand = strand->currentText();
      options.mask = mask->isChecked();
      options.softMask = soft->isChecked();
      options.threads = threads->value();
      options.waitMinutes = minutes->value();
      options.queryCode = queryCode->currentData().toInt();
      options.databaseCode = dbCode->currentData().toInt();
      if (auto error = options.error(); !error.isEmpty())
        bad(error);
      auto ids = selected;
      if (scope->currentIndex() == 1) {
        ids.clear();
        for (const auto &row : source->rows())
          ids.push_back(row.id);
      }
      queries = blast_queries(*source, ids, options.protein, true);
      if (queries.empty())
        bad("No nonempty sequences of the query molecule in this scope.");
      temp = std::make_unique<QTemporaryDir>();
      if (!temp->isValid())
        bad("Cannot create search workspace.");
      QFile input(temp->filePath("query.fa"));
      const auto fasta = blast_fasta(queries).toLatin1();
      if (!input.open(QIODevice::WriteOnly) ||
          input.write(fasta) != fasta.size())
        bad("Cannot write local query FASTA.");
      input.close();
      running = true;
      checking = true;
      output.clear();
      errorOutput.clear();
      hits.clear();
      rows.clear();
      model.clear();
      tsv->setEnabled(false);
      xlsx->setEnabled(false);
      diagnostics->clear();
      record("Executable: " + options.launcher);
      record("Database: " + db.manifest);
      controls->setEnabled(false);
      start->setEnabled(false);
      cancel->setText("Cancel job");
      tabs->setCurrentIndex(1);
      started = QDateTime::currentMSecsSinceEpoch();
      timer.start();
      process.setProgram(options.launcher);
      process.setArguments({"-version"});
      process.start();
    } catch (const std::exception &e) {
      QMessageBox::warning(this, "Local BLAST", QString::fromUtf8(e.what()));
    }
  }
  void refresh() {
    rows = blast_table(queries, hits, options);
    model.clear();
    model.setHorizontalHeaderLabels(blast_columns(true));
    for (int c = 0; c < 15; ++c)
      table->setColumnWidth(c, c == 1             ? 180
                               : c == 4           ? 240
                               : c == 3 || c == 5 ? 125
                                                  : 110);
    table->setSortingEnabled(false);
    for (const auto &row : rows) {
      QList<QStandardItem *> items;
      for (int c = 0; c < row.size(); ++c) {
        auto *item = new QStandardItem;
        item->setData(blast_numeric_column(c, true)
                          ? QVariant(row[c].toDouble())
                          : QVariant(row[c]),
                      Qt::DisplayRole);
        item->setData(row[c], Qt::ToolTipRole);
        items << item;
      }
      model.appendRow(items);
    }
    table->setSortingEnabled(true);
    table->sortByColumn(-1, Qt::AscendingOrder);
    tsv->setEnabled(!rows.empty());
    xlsx->setEnabled(!rows.empty());
  }
  void save(bool excel) {
    try {
      auto path = choose_export_path(
          this, excel ? "Save local BLAST XLSX" : "Save local BLAST TSV", {},
          excel ? "Excel workbook (*.xlsx)" : "Tab-separated table (*.tsv)",
          OutputFormat::Table, source.get());
      if (path.isEmpty())
        return;
      const auto bytes = excel ? blast_xlsx(rows, true) : blast_tsv(rows, true);
      auto error = write_export_atomic(
          path, OutputFormat::Table, source.get(),
          [&](auto &out) { out.write(bytes.constData(), bytes.size()); });
      if (!error.isEmpty())
        bad(error);
      status->setText("Saved: " + path);
    } catch (const std::exception &e) {
      QMessageBox::warning(this, "Save local results", e.what());
    }
  }
};
} // namespace
void create_local_database(bool protein, QWidget *parent) {
  const auto input = QFileDialog::getOpenFileName(
      parent,
      "Choose source for local " + QString(protein ? "protein" : "nucleotide") +
          " database",
      {},
      "Sequence files (*.fasta *.fas *.fa *.fna *.faa *.kleio);;All files (*)");
  if (input.isEmpty())
    return;
  auto target =
      QFileDialog::getSaveFileName(parent, "Create a local BLAST database file",
                                   {}, "Kleiotide local database (*.kblast)",
                                   nullptr, QFileDialog::DontConfirmOverwrite);
  if (target.isEmpty())
    return;
  if (!target.endsWith(".kblast", Qt::CaseInsensitive))
    target += ".kblast";
  if (QFileInfo::exists(target) || QFileInfo::exists(target + ".files")) {
    QMessageBox::warning(
        parent, "Create database",
        "Choose a new filename. An existing database will not be replaced.");
    return;
  }
  BuildDatabase dialog(input, protein, target, parent);
  dialog.exec();
}
void local_blast_dialog(std::shared_ptr<Document> document,
                        const std::vector<SequenceId> &selected, bool protein,
                        QWidget *parent) {
  LocalSearch dialog(std::move(document), selected, protein, parent);
  dialog.exec();
}
} // namespace kleiotide
