#include "gui/blast_dialog.hpp"
#include "gui/app_settings.hpp"
#include "gui/blast_data.hpp"
#include "gui/file_output.hpp"
#include "gui/local_blast.hpp"
#include <QAbstractTableModel>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScreen>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableView>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <set>
namespace kleiotide {
namespace {
// Shared across all open BLAST dialogs; one server contact per ten seconds.
qint64 nextContact = 0;
QPointer<QProcess> remoteClient;
bool confirm_submission(QWidget *parent, const QString &title,
                        const QString &message) {
  QMessageBox box(QMessageBox::Question, title, message,
                  QMessageBox::Yes | QMessageBox::No, parent);
  box.setTextFormat(Qt::PlainText);
  box.setDefaultButton(QMessageBox::No);
  return box.exec() == QMessageBox::Yes;
}

class ScientificSpin final : public QDoubleSpinBox {
public:
  using QDoubleSpinBox::QDoubleSpinBox;
  QString textFromValue(double v) const override {
    return QString::number(v, 'g', 10);
  }
  double valueFromText(const QString &s) const override { return s.toDouble(); }
  QValidator::State validate(QString &s, int &pos) const override {
    QDoubleValidator validator(1e-100, 1e6, 100);
    validator.setNotation(QDoubleValidator::ScientificNotation);
    validator.setLocale(QLocale::c());
    return validator.validate(s, pos);
  }
};
class Table final : public QAbstractTableModel {
public:
  BlastTable rows;
  int rowCount(const QModelIndex &p = {}) const override {
    return p.isValid() ? 0 : int(rows.size());
  }
  int columnCount(const QModelIndex &p = {}) const override {
    return p.isValid() ? 0 : blast_columns().size();
  }
  QVariant headerData(int s, Qt::Orientation o, int role) const override {
    if (o == Qt::Horizontal && role == Qt::DisplayRole)
      return blast_columns().value(s);
    return {};
  }
  QVariant data(const QModelIndex &i, int role) const override {
    if (!i.isValid())
      return {};
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
      return rows[i.row()][i.column()];
    if (role == Qt::ForegroundRole && i.column() == 4)
      return QColor("#1466bc");
    if (role == Qt::UserRole) {
      const int c = i.column();
      if (blast_numeric_column(c))
        return rows[i.row()][c].toDouble();
      return rows[i.row()][c];
    }
    return {};
  }
  void replace(BlastTable values) {
    beginResetModel();
    rows = std::move(values);
    endResetModel();
  }
};
class Dialog final : public QDialog {
public:
  BlastServices services;
  std::shared_ptr<Document> source;
  std::vector<SequenceId> selected;
  bool protein;
  BlastOptions options;
  std::vector<BlastQuery> queries;
  std::vector<std::vector<BlastQuery>> batches;
  std::vector<BlastHit> hits;
  QJsonArray completed;
  QJsonObject metadataCache;
  std::size_t batch = 0, resultBytes = 0;
  QString rid, jobPath;
  QStringList accessions;
  bool running = false, metadata = false;
  int generation = 0, processGeneration = 0;
  QTimer timer, heartbeat;
  qint64 waitUntil = 0, monitorStarted = 0, phaseStarted = 0;
  int polls = 0, readyRetries = 0, emptyReplies = 0;
  bool checkingVersion = false, clientVersionChecked = false;
  QString phase, lastServer;
  QJsonArray diagnostics;
  QPlainTextEdit *details;
  QNetworkAccessManager network;
  QNetworkReply *reply = nullptr;
  QProcess process;
  std::unique_ptr<QTemporaryDir> temp;
  Table model;
  QTableView *table;
  QTabWidget *tabs;
  QSortFilterProxyModel proxy;
  QLabel *status, *summary;
  QPushButton *start, *resume, *cancel, *tsv, *xlsx, *fetch, *saveJob,
      *saveReport, *openRid;
  QComboBox *scope, *database, *task, *strand, *matrix;
  QLineEdit *email, *entrez, *launcher;
  QSpinBox *maxHits, *word, *reward, *penalty, *gapOpen, *gapExtend,
      *composition, *waitMinutes;
  QDoubleSpinBox *expect, *identity, *coverage;
  QCheckBox *exclude, *mask, *soft, *shortQuery, *useToolkit;
  QWidget *controls;
  Dialog(std::shared_ptr<Document> d, std::vector<SequenceId> ids, bool p,
         QWidget *parent, BlastServices service)
      : QDialog(parent), services(std::move(service)), source(std::move(d)),
        selected(std::move(ids)), protein(p) {
    if (services.search.host() != "127.0.0.1" &&
        services.search.host() != "localhost") {
      services.contactMs = std::max(10000, services.contactMs);
      services.pollMs = std::max(60000, services.pollMs);
    }
    setObjectName("blastDialog");
    setWindowModality(Qt::WindowModal);
    setWindowTitle(p ? "NCBI protein BLAST" : "NCBI nucleotide BLAST");
    resize(1000, 820);
    auto *outer = new QVBoxLayout(this);
    auto *bodyScroll = new QScrollArea;
    bodyScroll->setWidgetResizable(true);
    auto *body = new QWidget;
    bodyScroll->setWidget(body);
    tabs = new QTabWidget;
    tabs->addTab(bodyScroll, "Search options");
    outer->addWidget(tabs, 1);
    auto *resultPage = new QWidget;
    auto *resultsLayout = new QVBoxLayout(resultPage);
    tabs->addTab(resultPage, "Results");
    auto *layout = new QVBoxLayout(body);
    auto *notice =
        new QLabel("Searches NCBI over HTTPS. Query sequences are sent to "
                   "NCBI; original names and traces stay on this computer.");
    notice->setWordWrap(true);
    layout->addWidget(notice);
    controls = new QWidget;
    auto *controlLayout = new QVBoxLayout(controls);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(controls);
    auto section = [&](const QString &title) {
      auto *group = new QGroupBox(title);
      auto *f = new QFormLayout(group);
      f->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
      f->setRowWrapPolicy(QFormLayout::WrapLongRows);
      controlLayout->addWidget(group);
      return f;
    };
    auto *form = section("Query and destination");
    auto combo = [&](const char *id, const QString &label,
                     const QStringList &list) {
      auto *c = new QComboBox;
      c->setObjectName(id);
      c->addItems(list);
      c->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      form->addRow(label, c);
      return c;
    };
    auto spin = [&](const char *id, const QString &label, int low, int high,
                    int value) {
      auto *w = new QSpinBox;
      w->setObjectName(id);
      w->setRange(low, high);
      w->setValue(value);
      w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      form->addRow(label, w);
      return w;
    };
    auto real = [&](const char *id, const QString &label, double low,
                    double high, double value, int decimals) {
      QDoubleSpinBox *w =
          QString::fromLatin1(id) == "blastExpect"
              ? static_cast<QDoubleSpinBox *>(new ScientificSpin)
              : new QDoubleSpinBox;
      w->setObjectName(id);
      w->setDecimals(decimals);
      w->setRange(low, high);
      w->setValue(value);
      w->setMaximumWidth(
          fontMetrics().horizontalAdvance("0.000000000000e-100") + 40);
      form->addRow(label, w);
      return w;
    };
    scope = combo("blastScope", "Query scope",
                  {"Selected names", "All matching rows"});
    scope->setCurrentIndex(selected.empty() ? 1 : 0);
    database = combo("blastDatabase", "NCBI database", blast_databases(p));
    task =
        combo("blastTask", "Search strategy",
              p ? QStringList{"blastp"} : QStringList{"megablast", "blastn"});
    email = new QLineEdit;
    email->setObjectName("blastEmail");
    email->setMinimumWidth(240);
    email->setPlaceholderText("Set a default in Settings → BLAST");
    form->addRow("NCBI contact email *", email);
    form = section("Targets and result filters");
    maxHits = spin("blastMaxHits", "Maximum target sequences", 1, 5000, 10);
    expect = real("blastExpect", "E-value threshold", 1e-100, 1e6, .05, 100);
    identity = real("blastIdentity", "Minimum identity (%)", 0, 100, 0, 2);
    coverage = real("blastCoverage", "Minimum query cover (%)", 0, 100, 0, 2);
    form = section("Scoring");
    word = spin("blastWord", "Word size", 2, 256, p ? 3 : 28);
    reward = spin("blastReward", "Match reward", 1, 5, 1);
    penalty = spin("blastPenalty", "Mismatch penalty", -10, -1, -2);
    form->setRowVisible(reward, !p);
    form->setRowVisible(penalty, !p);
    gapOpen = spin("blastGapOpen", "Gap open (both 0 = default)", 0, 100, 0);
    gapExtend = spin("blastGapExtend", "Gap extension", 0, 100, 0);
    matrix = combo("blastMatrix", "Scoring matrix",
                   {"BLOSUM62", "BLOSUM45", "BLOSUM50", "BLOSUM80", "BLOSUM90",
                    "PAM250", "PAM30", "PAM70"});
    composition = spin("blastComposition", "Composition statistics", 0, 3, 2);
    form->setRowVisible(matrix, p);
    form->setRowVisible(composition, p);
    form = section("Filtering and monitoring");
    mask = new QCheckBox(p ? "Low-complexity filtering (SEG)"
                           : "Low-complexity filtering (DUST)");
    soft = new QCheckBox("Mask during seed lookup only");
    soft->setChecked(true);
    shortQuery = new QCheckBox("Automatically adjust short queries");
    form->addRow(mask);
    form->addRow(soft);
    form->addRow(shortQuery);
    waitMinutes = spin("blastWaitMinutes",
                       "Maximum monitoring time per batch (min)", 1, 1440, 120);
    connect(task, &QComboBox::currentTextChanged, this,
            [this](const QString &t) {
              word->setValue(protein ? 3 : t == "megablast" ? 28 : 11);
              reward->setValue(t == "blastn" ? 2 : 1);
              penalty->setValue(t == "blastn" ? -3 : -2);
              gapOpen->setValue(0);
              gapExtend->setValue(0);
            });
    const auto &defaults = AppSettings::instance().values();
    database->setCurrentText(p ? defaults.blastProteinDatabase
                               : defaults.blastNucleotideDatabase);
    task->setCurrentText(p ? "blastp"
                         : defaults.blastNucleotideTask == "blastn"
                             ? "blastn"
                             : "megablast");
    email->setText(defaults.blastEmail);
    maxHits->setValue(defaults.blastMaxHits);
    expect->setValue(defaults.blastExpect);
    identity->setValue(defaults.blastIdentity);
    coverage->setValue(defaults.blastCoverage);
    mask->setChecked(p ? defaults.blastProteinMask
                       : defaults.blastNucleotideMask);
    waitMinutes->setValue(defaults.blastWaitMinutes);
    const std::vector<std::pair<QWidget *, QString>> help = {
        {scope, "Choose selected names or every row of this molecule."},
        {database, "Choose an NCBI database."},
        {task, "Megablast finds close matches; blastn is more sensitive."},
        {email, "Required contact address for NCBI requests."},
        {maxHits, "Maximum targets retrieved before result filters."},
        {expect, "Smaller values require stronger matches."},
        {identity, "Minimum identity of the best-scoring HSP."},
        {coverage, "Minimum query coverage across all HSPs."},
        {word, "Larger seed words are faster and less sensitive."},
        {reward, "Score for matching nucleotide bases."},
        {penalty, "Score for mismatching nucleotide bases."},
        {gapOpen, "Cost to open a gap; both 0 uses the task default."},
        {gapExtend, "Cost for each additional gap residue."},
        {matrix, "Protein substitution scoring matrix."},
        {composition,
         "0: off; 1: traditional; 2: conditional; 3: unconditional."},
        {mask, "Mask simple repetitive regions."},
        {soft, "Allow alignment extension through masked regions."},
        {shortQuery, "Let NCBI adjust settings for very short queries."},
        {waitMinutes,
         "Stop monitoring at this limit; keep the RID for Resume."}};
    for (const auto &[widget, tip] : help)
      widget->setToolTip(tip);
    controlLayout->addStretch();
    auto refilter = [this] {
      if (running || hits.empty())
        return;
      options.identity = identity->value();
      options.coverage = coverage->value();
      options.expect = expect->value();
      refresh();
    };
    connect(identity, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [refilter](double) { refilter(); });
    connect(coverage, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [refilter](double) { refilter(); });
    connect(expect, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [refilter](double) { refilter(); });
    summary =
        new QLabel("Identity and query-cover filters are applied to received "
                   "hits. Retrieving 10 targets can show fewer than 10 after "
                   "filtering. Query cover = union of all HSP query spans; "
                   "other metrics = best bit-score HSP. Publication year is "
                   "the year of the first published reference, if provided.");
    summary->setToolTip(summary->text());
    summary->setText("Cover = union of HSP query spans; other metrics use the "
                     "best HSP. Filters apply locally.");
    summary->setWordWrap(true);
    resultsLayout->addWidget(summary);
    proxy.setSourceModel(&model);
    proxy.setSortRole(Qt::UserRole);
    table = new QTableView;
    table->setObjectName("blastResults");
    table->setModel(&proxy);
    table->setSortingEnabled(true);
    table->sortByColumn(-1, Qt::AscendingOrder);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    resultsLayout->addWidget(table, 1);
    connect(
        table, &QTableView::clicked, this, [this](const QModelIndex &index) {
          if (index.column() == 4) {
            const auto accession = index.data().toString();
            QDesktopServices::openUrl(
                QUrl("https://www.ncbi.nlm.nih.gov/" +
                     QString(protein ? "protein/" : "nuccore/") +
                     QString::fromLatin1(QUrl::toPercentEncoding(accession))));
          }
        });
    status = new QLabel("Configure a search. Nothing has been submitted.");
    status->setObjectName("blastStatus");
    status->setWordWrap(true);
    resultsLayout->addWidget(status);
    details = new QPlainTextEdit;
    details->setObjectName("blastDiagnostics");
    details->setReadOnly(true);
    details->setMaximumBlockCount(400);
    details->setMaximumHeight(120);
    details->setPlaceholderText(
        "Timestamped request and client diagnostics appear here. No network "
        "contact occurs until submission confirmation.");
    resultsLayout->addWidget(details);
    auto *buttons = new QDialogButtonBox;
    outer->addWidget(buttons);
    start = buttons->addButton("Search NCBI", QDialogButtonBox::ActionRole);
    start->setObjectName("blastStart");
    resume = buttons->addButton("Resume job…", QDialogButtonBox::ActionRole);
    resume->setObjectName("blastResume");
    saveJob =
        buttons->addButton("Save job copy…", QDialogButtonBox::ActionRole);
    fetch = buttons->addButton("Fetch GenBank metadata",
                               QDialogButtonBox::ActionRole);
    fetch->setObjectName("blastMetadata");
    tsv = buttons->addButton("Save TSV…", QDialogButtonBox::ActionRole);
    xlsx = buttons->addButton("Save XLSX…", QDialogButtonBox::ActionRole);
    cancel = buttons->addButton("Close", QDialogButtonBox::RejectRole);
    cancel->setObjectName("blastCancel");
    fetch->setEnabled(false);
    tsv->setEnabled(false);
    xlsx->setEnabled(false);
    saveJob->setEnabled(false);
    saveReport = buttons->addButton("Save diagnostic report…",
                                    QDialogButtonBox::ActionRole);
    saveReport->setObjectName("blastSaveReport");
    openRid =
        buttons->addButton("Open RID at NCBI", QDialogButtonBox::ActionRole);
    openRid->setObjectName("blastOpenRid");
    openRid->setEnabled(false);
    auto *recordsRow = new QDialogButtonBox;
    auto *exportRow = new QDialogButtonBox;
    for (auto *button : {fetch, saveJob, saveReport, openRid}) {
      buttons->removeButton(button);
      recordsRow->addButton(button, QDialogButtonBox::ActionRole);
    }
    for (auto *button : {tsv, xlsx}) {
      buttons->removeButton(button);
      exportRow->addButton(button, QDialogButtonBox::ActionRole);
    }
    outer->addWidget(recordsRow);
    outer->addWidget(exportRow);
    if (auto *screen = this->screen()) {
      const auto size = screen->availableGeometry().size();
      resize(std::min(1150, std::max(360, size.width() - 48)),
             std::min(850, std::max(360, size.height() - 64)));
    }
    connect(start, &QPushButton::clicked, this, [this] { begin(); });
    connect(resume, &QPushButton::clicked, this, [this] { load_job(); });
    connect(cancel, &QPushButton::clicked, this, &Dialog::reject);
    connect(tsv, &QPushButton::clicked, this, [this] { export_table(false); });
    connect(xlsx, &QPushButton::clicked, this, [this] { export_table(true); });
    connect(fetch, &QPushButton::clicked, this, [this] { begin_metadata(); });
    connect(saveJob, &QPushButton::clicked, this, [this] {
      const auto path =
          choose_export_path(this, "Save BLAST job copy", {}, "JSON (*.json)",
                             OutputFormat::Table, source.get());
      if (!path.isEmpty()) {
        auto before = jobPath;
        jobPath = path;
        persist();
        jobPath = before;
      }
    });
    start->setToolTip("Validate options, then review and confirm the queries "
                      "before sending anything to NCBI.");
    resume->setToolTip("Open a saved job. Continue monitoring the saved NCBI "
                       "request without submitting it again.");
    saveJob->setToolTip("Save query data, options, completed hits and any "
                        "current RID so this search can be resumed. This file "
                        "contains sequences and original names.");
    fetch->setToolTip(
        "Retry missing GenBank record details without repeating BLAST.");
    tsv->setToolTip("Save the currently filtered hits as a tab-separated "
                    "table, in query/hit order.");
    xlsx->setToolTip("Save the currently filtered hits as an Excel workbook "
                     "with numeric metric columns.");
    cancel->setToolTip(
        "While running, stop monitoring or stop the external client, retaining "
        "completed results. Submitted sequences cannot be retracted. When "
        "idle, close the dialog.");
    saveReport->setToolTip(
        "Save timestamps, elapsed time, RID, backend parameters and client "
        "diagnostics to help investigate delays or failures. Query sequences "
        "are omitted.");
    openRid->setToolTip(
        "Open the current Request ID on NCBI's website to check the same "
        "server job; this does not submit a new query.");
    connect(openRid, &QPushButton::clicked, this, [this] {
      QUrl url(services.search);
      QUrlQuery q;
      q.addQueryItem("CMD", "Get");
      q.addQueryItem("RID", rid);
      q.addQueryItem("PROGRAM", protein ? "blastp" : "blastn");
      url.setQuery(q);
      QDesktopServices::openUrl(url);
    });
    connect(saveReport, &QPushButton::clicked, this, [this] {
      auto path = choose_output_filename(this, "Save BLAST diagnostic report",
                                         {}, "Text (*.txt)", nullptr,
                                         QFileDialog::DontConfirmOverwrite);
      if (path.isEmpty())
        return;
      if (QFileInfo(path).suffix().isEmpty())
        path += ".txt";
      path = approve_output_path(this, path, OutputFormat::Table, source.get());
      if (path.isEmpty())
        return;
      QString text =
          "Kleiotide 0.29.0 BLAST diagnostics\nDatabase: " + options.database +
          "; task: " + options.task + "; backend: " +
          (options.toolkit() ? options.launcher : QString("Built-in HTTPS")) +
          "\nCurrent status: " + status->text() + "\n" +
          details->toPlainText() + "\n";
      auto error = write_export_atomic(
          path, OutputFormat::Table, source.get(), [&](auto &out) {
            const auto bytes = text.toUtf8();
            out.write(bytes.constData(), bytes.size());
          });
      if (!error.isEmpty())
        QMessageBox::warning(this, "Diagnostic report", error);
    });
    heartbeat.setInterval(1000);
    connect(&heartbeat, &QTimer::timeout, this, [this] {
      if (!running)
        return;
      const auto now = QDateTime::currentMSecsSinceEpoch();
      if (checkingVersion && now - phaseStarted >= 10000) {
        fail("BLAST+ did not respond to -version-full within 10 seconds. "
             "Verify the configured executable.");
        return;
      }
      if (!metadata && monitorStarted &&
          now - monitorStarted >= ((services.search.host() == "127.0.0.1" ||
                                    services.search.host() == "localhost") &&
                                           services.testTimeoutMs > 0
                                       ? services.testTimeoutMs
                                       : qint64(options.waitMinutes) * 60000)) {
        fail("Monitoring time limit reached. NCBI may still be searching. "
             "Resume reuses the saved RID.");
        return;
      }
      update_status();
    });
    heartbeat.start();
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [this] { next(); });
  }
  ~Dialog() override {
    running = false;
    ++generation;
    timer.stop();
    if (reply) {
      reply->disconnect(this);
      reply->abort();
      reply = nullptr;
    }
    process.disconnect(this);
    if (process.state() != QProcess::NotRunning) {
      process.kill();
      process.waitForFinished(1000);
    }
  }
  QByteArray processBytes, log;
  BlastOptions read() const {
    BlastOptions o;
    o.protein = protein;
    o.waitMinutes = waitMinutes->value();
    o.database = database->currentText();
    o.task = task->currentText();
    o.email = email->text().trimmed();
    o.maxHits = maxHits->value();
    o.expect = expect->value();
    o.identity = identity->value();
    o.coverage = coverage->value();
    o.word = word->value();
    o.reward = reward->value();
    o.penalty = penalty->value();
    o.gapOpen = gapOpen->value();
    o.gapExtend = gapExtend->value();
    o.matrix = matrix->currentText();
    o.composition = composition->value();
    o.mask = mask->isChecked();
    o.softMask = soft->isChecked();
    o.shortQuery = shortQuery->isChecked();
    return o;
  }
  void set_running(bool on) {
    running = on;
    if (on)
      tabs->setCurrentIndex(1);
    else
      waitUntil = 0;
    saveJob->setEnabled(!queries.empty());
    controls->setEnabled(!on);
    start->setEnabled(!on);
    resume->setEnabled(!on);
    fetch->setEnabled(!on && !hits.empty());
    tsv->setEnabled(!on && !model.rows.empty());
    xlsx->setEnabled(!on && !model.rows.empty());
    cancel->setText(on ? "Cancel job" : "Close");
  }
  void reject() override {
    if (running) {
      ++generation;
      timer.stop();
      running = false;
      if (reply) {
        reply->abort();
        reply = nullptr;
      }
      if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(1000);
      }
      record(
          "Cancelled by user; submitted remote queries cannot be retracted.");
      persist();
      set_running(false);
      status->setText(
          "Cancelled. Submitted queries cannot be retracted. Resume from " +
          jobPath);
      return;
    }
    QDialog::reject();
  }
  void fail(const QString &error) {
    timer.stop();
    ++generation;
    set_running(false);
    checkingVersion = false;
    if (reply) {
      reply->abort();
      reply = nullptr;
    }
    if (process.state() != QProcess::NotRunning) {
      process.kill();
      process.waitForFinished(1000);
    }
    record("Stopped: " + error);
    set_running(false);
    persist();
    status->setText(error +
                    "\nCompleted results are retained. Job: " + jobPath);
  }
  void refresh() {
    try {
      model.replace(blast_table(queries, hits, options));
    } catch (const std::exception &e) {
      status->setText(e.what());
    }
    tsv->setEnabled(!running && !model.rows.empty());
    xlsx->setEnabled(!running && !model.rows.empty());
  }
  QJsonObject state() const {
    QJsonArray qs;
    for (const auto &q : queries) {
      QJsonArray names, numbers;
      for (const auto &name : q.names)
        names.append(name);
      for (auto n : q.numbers)
        numbers.append(n);
      qs.append(QJsonObject{{"id", q.id},
                            {"sequence", q.sequence},
                            {"names", names},
                            {"numbers", numbers}});
    }
    return {{"format", "Kleiotide BLAST job"},
            {"version", 1},
            {"options", options.json()},
            {"queries", qs},
            {"batch", int(batch)},
            {"rid", rid},
            {"completed", completed},
            {"metadata", metadataCache},
            {"diagnostics", diagnostics}};
  }
  bool persist() {
    if (jobPath.isEmpty())
      return true;
    const auto error = write_output_atomic(jobPath, [&](auto &out) {
      const auto bytes = QJsonDocument(state()).toJson();
      if (bytes.size() > 128 * 1024 * 1024)
        throw std::length_error("Job file exceeds 128 MiB");
      out.write(bytes.constData(), bytes.size());
    });
    if (!error.isEmpty()) {
      status->setText("Could not save resumable job: " + error);
      return false;
    }
    return true;
  }
  void begin() {
    const auto previousOptions = options;
    const auto previousQueries = queries;
    const auto previousBatches = batches;
    bool committed = false;
    try {
      options = read();
      if (auto e = options.error(); !e.isEmpty()) {
        QMessageBox::warning(this, "BLAST options", e);
        options = previousOptions;
        return;
      }
      std::vector<SequenceId> ids = selected;
      if (scope->currentIndex() == 1) {
        ids.clear();
        for (const auto &row : source->rows())
          ids.push_back(row.id);
      }
      queries = blast_queries(*source, ids, protein);
      if (queries.empty()) {
        QMessageBox::warning(
            this, "BLAST query scope",
            "No nonempty sequences of the requested molecule in this scope.");
        options = previousOptions;
        queries = previousQueries;
        return;
      }
      std::size_t residues = 0, names = 0;
      for (const auto &q : queries) {
        residues += q.sequence.size();
        names += q.names.size();
      }
      if (residues > 20000000 || queries.size() > 20000)
        throw std::length_error("Public-server job exceeds 20 million residues "
                                "or 20,000 unique queries. Use local BLAST+.");
      batches = blast_batches(queries);
      const auto message =
          QString("Send %1 sequences (%2 unique queries, %3 residues) in %4 "
                  "batches to NCBI?\nDatabase: %5; task: %6; max targets: %7; "
                  "E value: %8\nBackend: %9\nBatching respects shared-server "
                  "limits; large jobs may take hours. Query names stay local. "
                  "A local resumable job stores query data.")
              .arg(names)
              .arg(queries.size())
              .arg(residues)
              .arg(batches.size())
              .arg(options.database, options.task)
              .arg(options.maxHits)
              .arg(options.expect)
              .arg(options.toolkit()
                       ? "NCBI BLAST+ -remote: " + options.launcher
                       : QString("Built-in HTTPS"));
      if (!confirm_submission(this, "Submit BLAST queries?", message)) {
        options = previousOptions;
        queries = previousQueries;
        batches = previousBatches;
        return;
      }
      committed = true;
      diagnostics = {};
      details->clear();
      monitorStarted = 0;
      clientVersionChecked = false;
      checkingVersion = false;
      polls = 0;
      readyRetries = 0;
      emptyReplies = 0;
      record("Confirmed " + options.task + " / " + options.database +
             "; backend " +
             (options.toolkit() ? options.launcher : QString("HTTPS")));
      hits.clear();
      completed = {};
      metadataCache = {};
      batch = 0;
      resultBytes = 0;
      rid.clear();
      metadata = false;
      model.replace({});
      const auto root = services.jobDirectory.isEmpty()
                            ? QStandardPaths::writableLocation(
                                  QStandardPaths::AppLocalDataLocation) +
                                  "/blast-jobs"
                            : services.jobDirectory;
      if (!QDir().mkpath(root))
        throw std::runtime_error("Cannot create job storage");
      jobPath =
          root + '/' + QUuid::createUuid().toString(QUuid::Id128) + ".json";
      if (!persist()) {
        QMessageBox::warning(this, "BLAST job storage", status->text());
        return;
      }
      set_running(true);
      ++generation;
      schedule(0);
    } catch (const std::exception &e) {
      if (!committed) {
        options = previousOptions;
        queries = previousQueries;
        batches = previousBatches;
        QMessageBox::warning(this, "BLAST search", QString::fromUtf8(e.what()));
        return;
      }
      fail(e.what());
    }
  }
  void record(const QString &text) {
    if (text.isEmpty())
      return;
    const auto line = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) +
                      "  " + text.left(8192);
    diagnostics.append(line);
    while (diagnostics.size() > 400)
      diagnostics.removeFirst();
    details->appendPlainText(line);
  }
  void set_phase(const QString &value) {
    phase = value;
    phaseStarted = QDateTime::currentMSecsSinceEpoch();
    waitUntil = 0;
    record(value);
    update_status();
  }
  void update_status() {
    if (!running)
      return;
    const auto now = QDateTime::currentMSecsSinceEpoch();
    QString text = phase;
    const auto elapsed = monitorStarted ? (now - monitorStarted) / 1000 : 0;
    if (waitUntil > now)
      text += QString(" Next contact in %1 s (rate-limited).")
                  .arg((waitUntil - now + 999) / 1000);
    if (metadata)
      text += QString(" Metadata records remaining: %1; all %2 search batches "
                      "completed.")
                  .arg(accessions.size())
                  .arg(batches.size());
    else
      text += QString(" Batch %1/%2; elapsed %3 min %4 s; completed %5.")
                  .arg(std::min(batch + 1, batches.size()))
                  .arg(batches.size())
                  .arg(elapsed / 60)
                  .arg(elapsed % 60)
                  .arg(batch);
    if (!lastServer.isEmpty())
      text += " Last server reply: " + lastServer;
    if (!rid.isEmpty())
      text += " · RID " + rid;

    text += "\nJob: " + jobPath;
    status->setText(text);
    openRid->setEnabled(!rid.isEmpty());
  }
  void schedule(int minimumMs) {
    const auto now = QDateTime::currentMSecsSinceEpoch();
    const auto at = std::max(now + minimumMs, nextContact);
    nextContact = at + services.contactMs;
    waitUntil = at;
    phase = metadata ? "Waiting to fetch GenBank metadata."
            : rid.isEmpty()
                ? "Waiting to submit the next batch."
                : "NCBI search is pending; checking the saved RID at most once "
                  "a minute (not a completion estimate).";
    update_status();
    timer.start(int(std::min<qint64>(at - now, 3600000)));
  }
  void request(QUrl url, const QByteArray &post,
               const std::function<void(QByteArray)> &done) {
    if (url.scheme() == "https" && !QSslSocket::supportsSsl()) {
      fail("No TLS backend is available. Reinstall with the Qt TLS plugins "
           "(Windows Schannel) deployed beside the application.");
      return;
    }
    set_phase(metadata         ? "Downloading GenBank metadata…"
              : post.isEmpty() ? "Checking NCBI / downloading a result…"
                               : "Submitting query batch to NCBI…");
    const auto current = generation;
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Kleiotide/0.29.0");
    req.setTransferTimeout(120000);
    if (!post.isEmpty())
      req.setHeader(QNetworkRequest::ContentTypeHeader,
                    "application/x-www-form-urlencoded");
    reply = post.isEmpty() ? network.get(req) : network.post(req, post);
    auto *active = reply;
    auto bytes = std::make_shared<QByteArray>();
    connect(active, &QNetworkReply::readyRead, this, [this, active, bytes] {
      *bytes += active->readAll();
      if (bytes->size() > 64 * 1024 * 1024)
        active->abort();
    });
    connect(active, &QNetworkReply::finished, this,
            [this, active, bytes, current, done] {
              *bytes += active->readAll();
              if (reply == active)
                reply = nullptr;
              const auto error = active->error();
              const auto errorText = active->errorString();
              active->deleteLater();
              if (!running || current != generation)
                return;
              if (error != QNetworkReply::NoError) {
                fail("Network failure (no automatic resubmission): " +
                     errorText);
                return;
              }
              if (bytes->size() > 64 * 1024 * 1024) {
                fail("Response exceeds 64 MiB");
                return;
              }
              record(QString("HTTP reply: %1 bytes").arg(bytes->size()));
              done(*bytes);
            });
  }
  void next() {
    if (!running)
      return;
    if (metadata) {
      metadata_next();
      return;
    }
    if (batch >= batches.size()) {
      set_running(false);
      refresh();
      status->setText(
          QString("Search complete: %1 hit rows. Fetching GenBank metadata…")
              .arg(model.rows.size()));
      if (!hits.empty())
        begin_metadata();
      else
        status->setText("Search complete: no hits returned. Job: " + jobPath);
      return;
    }
    if (batches.size() > 50 && rid.isEmpty() &&
        services.search.host() != "127.0.0.1" &&
        services.search.host() != "localhost") {
      const QTimeZone zone("America/New_York");
      if (!zone.isValid()) {
        fail("Cannot determine NCBI off-peak time; reduce to 50 batches or use "
             "local BLAST+.");
        return;
      }
      const auto eastern = QDateTime::currentDateTimeUtc().toTimeZone(zone);
      if (eastern.date().dayOfWeek() <= 5 && eastern.time().hour() >= 5 &&
          eastern.time().hour() < 21) {
        phase = "Waiting for NCBI off-peak window (weekends or 21:00–05:00 US "
                "Eastern). Cancel permits resuming later.";
        waitUntil = QDateTime::currentMSecsSinceEpoch() + 60000;
        update_status();
        timer.start(60000);
        return;
      }
    }
    if (!monitorStarted)
      monitorStarted = QDateTime::currentMSecsSinceEpoch();
    if (rid.isEmpty()) {
      QByteArray body;
      try {
        body = blast_parameters(options, batches[batch])
                   .toString(QUrl::FullyEncoded)
                   .replace("+", "%2B")
                   .toUtf8();
      } catch (const std::exception &e) {
        fail(e.what());
        return;
      }
      request(services.search, body, [this](const QByteArray &response) {
        const auto text = QString::fromUtf8(response);
        const auto match =
            QRegularExpression("RID\\s*=\\s*([A-Za-z0-9-]+)").match(text);
        if (!match.hasMatch()) {
          fail("No RID returned. Submission status unknown; inspect the saved "
               "job before starting another search.");
          return;
        }
        rid = match.captured(1);
        lastServer = "RID accepted at " +
                     QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        record(lastServer + "; RID " + rid);
        if (!persist()) {
          set_running(false);
          status->setText("Could not save RID " + rid +
                          ". Use Save job copy before closing.");
          return;
        }
        schedule(services.pollMs);
      });
    } else {
      QUrl url(services.search);
      QUrlQuery q;
      q.addQueryItem("CMD", "Get");
      q.addQueryItem("RID", rid);

      q.addQueryItem("FORMAT_TYPE", "JSON2_S");
      q.addQueryItem("tool", "Kleiotide");
      q.addQueryItem("email", options.email);
      url.setQuery(q.toString(QUrl::FullyEncoded).replace("+", "%2B"));
      request(url, {}, [this](const QByteArray &response) {
        const auto text = QString::fromUtf8(response);
        if (text.trimmed().isEmpty()) {
          record(
              "Empty NCBI reply; retrying the same RID without resubmission.");
          if (++emptyReplies > 3) {
            fail("NCBI repeatedly returned an empty response for RID " + rid);
            return;
          }
          schedule(services.pollMs);
          return;
        }
        emptyReplies = 0;
        const auto statusMatch =
            QRegularExpression("Status\\s*=\\s*(WAITING|FAILED|UNKNOWN|READY)",
                               QRegularExpression::CaseInsensitiveOption)
                .match(text);
        if (statusMatch.hasMatch()) {
          const auto state = statusMatch.captured(1).toUpper();
          ++polls;
          lastServer = state + " at " +
                       QDateTime::currentDateTimeUtc().toString(Qt::ISODate) +
                       QString(" (poll %1)").arg(polls);
          record(lastServer);
          if (state == "WAITING") {
            schedule(services.pollMs);
            return;
          }
          if (state == "FAILED" || state == "UNKNOWN") {
            fail("NCBI RID failed or expired: " + rid + "\n" + text.left(2048));
            return;
          }
          if (state == "READY" && !response.trimmed().startsWith('{')) {
            if (++readyRetries > 3) {
              fail("NCBI reports READY but has not returned JSON results. Use "
                   "Open RID at NCBI and save the diagnostic report; this job "
                   "can be resumed.");
              return;
            }
            schedule(services.pollMs);
            return;
          }
        }
        record("Result response received; parsing JSON.");
        accept_results(response);
      });
    }
  }
  void accept_results(const QByteArray &bytes) {
    try {
      if (resultBytes + std::size_t(bytes.size()) > 64 * 1024 * 1024)
        throw std::length_error(
            "Completed raw results exceed 64 MiB. Reduce targets or query "
            "scope; completed batches are retained.");
      auto parsed = parse_blast(bytes, batches[batch]);
      if (hits.size() + parsed.size() > 500000)
        throw std::length_error(
            "Job exceeds 500,000 hits. Reduce target count or query scope; "
            "completed results are retained.");
      hits.insert(hits.end(), parsed.begin(), parsed.end());
      completed.append(QJsonDocument::fromJson(bytes).object());
      resultBytes += bytes.size();
      ++batch;
      monitorStarted = 0;
      polls = 0;
      readyRetries = 0;
      lastServer.clear();
      rid.clear();
      openRid->setEnabled(false);
      if (!persist()) {
        set_running(false);
        refresh();
        status->setText("Results retained in memory, but the resumable job "
                        "could not be saved. Use Save job copy.");
        return;
      }
      refresh();
      if (batch == batches.size())
        next();
      else
        schedule(0);
    } catch (const std::exception &e) {
      record("Response excerpt: " + QString::fromUtf8(bytes.left(2048)));
      fail(e.what());
    }
  }
  void begin_metadata() {
    if (running || hits.empty())
      return;
    std::set<QString> seen;
    accessions.clear();
    for (auto &h : hits) {
      if (metadataCache.contains(h.accession)) {
        const auto cached = metadataCache[h.accession].toObject();
        h.species = cached["species"].toString();
        h.identifier = cached["identifier"].toString();
        h.taxonomy = cached["taxonomy"].toString();
        h.authors = cached["authors"].toString();
        h.date = cached["date"].toString();
      } else if (seen.insert(h.accession).second)
        accessions.append(h.accession);
    }
    metadata = true;
    set_running(true);
    ++generation;
    if (accessions.isEmpty())
      metadata_next();
    else
      schedule(0);
  }

  void metadata_next() {
    if (accessions.isEmpty()) {
      metadata = false;
      set_running(false);
      refresh();
      status->setText(
          QString(
              "Complete: %1 displayed hit rows. Job: %2. Unavailable fields "
              "remain blank; publication year is not a record-update date.")
              .arg(model.rows.size())
              .arg(jobPath));
      return;
    }
    const auto group = accessions.mid(0, 20);
    QUrl url(services.records);
    QUrlQuery q;
    q.addQueryItem("db", options.protein ? "protein" : "nuccore");
    q.addQueryItem("id", group.join(','));
    q.addQueryItem("rettype", options.protein ? "gp" : "gb");
    q.addQueryItem("retmode", "xml");
    q.addQueryItem("tool", "Kleiotide");
    q.addQueryItem("email", options.email);
    request(url, q.toString(QUrl::FullyEncoded).replace("+", "%2B").toUtf8(),
            [this, group](const QByteArray &bytes) {
              try {
                enrich_blast(hits, bytes);
                for (const auto &accession : group) {
                  for (const auto &h : hits)
                    if (h.accession == accession) {
                      metadataCache[accession] =
                          QJsonObject{{"species", h.species},
                                      {"identifier", h.identifier},
                                      {"taxonomy", h.taxonomy},
                                      {"authors", h.authors},
                                      {"date", h.date}};
                      break;
                    }
                }
                persist();
                accessions = accessions.mid(group.size());
                refresh();
                schedule(0);
              } catch (const std::exception &e) {
                fail(QString("Basic hits retained; metadata failed: ") +
                     e.what());
              }
            });
  }
  void load_job() {
    const auto path = QFileDialog::getOpenFileName(
        this, "Resume Kleiotide BLAST job", {}, "JSON (*.json)");
    if (path.isEmpty())
      return;
    try {
      QFile file(path);
      if (!file.open(QIODevice::ReadOnly) || file.size() > 128 * 1024 * 1024)
        throw std::runtime_error("Cannot read job or job too large");
      QJsonParseError error;
      const auto j = QJsonDocument::fromJson(file.readAll(), &error).object();
      if (error.error != QJsonParseError::NoError ||
          j["format"] != "Kleiotide BLAST job" || j["version"].toInt() != 1)
        throw std::runtime_error("Invalid BLAST job");
      auto nextOptions = BlastOptions::from_json(j["options"].toObject());
      if (nextOptions.local)
        throw std::runtime_error("Use Local BLAST for local jobs.");
      if (nextOptions.protein != protein)
        throw std::runtime_error(
            "Use the matching nucleotide/protein BLAST menu for this job");
      if (!nextOptions.error().isEmpty())
        throw std::runtime_error(nextOptions.error().toStdString());
      std::vector<BlastQuery> nextQueries;
      std::set<QString> ids;
      std::size_t residues = 0;
      for (const auto &value : j["queries"].toArray()) {
        const auto q = value.toObject();
        BlastQuery query;
        query.id = q["id"].toString();
        query.sequence = q["sequence"].toString();
        for (const auto &name : q["names"].toArray())
          query.names << name.toString();
        for (const auto &n : q["numbers"].toArray())
          query.numbers.push_back(n.toInt());
        if (!QRegularExpression("^q[0-9]+$").match(query.id).hasMatch() ||
            !ids.insert(query.id).second || query.names.isEmpty() ||
            query.sequence.isEmpty() || query.sequence.size() > 100000 ||
            !QRegularExpression("^[A-Z*]+$").match(query.sequence).hasMatch())
          throw std::runtime_error("Invalid saved query");
        residues += query.sequence.size();
        nextQueries.push_back(query);
        if (nextQueries.size() > 20000 || residues > 20000000)
          throw std::runtime_error("Saved job exceeds query limits");
      }
      std::set<int> numbers;
      bool someNumbered = false, someLegacy = false;
      int nameCount = 0;
      for (const auto &q : nextQueries) {
        nameCount += q.names.size();
        if (q.numbers.empty()) {
          someLegacy = true;
          continue;
        }
        someNumbered = true;
        if (q.numbers.size() != std::size_t(q.names.size()))
          throw std::runtime_error("Invalid saved query numbering");
        for (int n : q.numbers)
          if (n < 1 || !numbers.insert(n).second)
            throw std::runtime_error("Invalid saved query numbering");
      }
      if (someNumbered && (someLegacy || *numbers.rbegin() != nameCount))
        throw std::runtime_error("Invalid saved query numbering");
      auto nextBatches = blast_batches(nextQueries);
      const auto nextBatch = std::size_t(j["batch"].toInt(-1));
      auto nextCompleted = j["completed"].toArray();
      auto nextRid = j["rid"].toString();
      if (nextQueries.empty() || nextBatch > nextBatches.size() ||
          nextCompleted.size() != int(nextBatch) ||
          (!nextRid.isEmpty() &&
           (!QRegularExpression("^[A-Za-z0-9-]{1,100}$")
                 .match(nextRid)
                 .hasMatch() ||
            nextOptions.toolkit() || nextBatch == nextBatches.size())))
        throw std::runtime_error("Invalid saved batch state");
      std::size_t nextBytes = 0;
      std::vector<BlastHit> nextHits;
      for (std::size_t i = 0; i < nextBatch; ++i) {
        const auto bytes =
            QJsonDocument(nextCompleted[int(i)].toObject()).toJson();
        nextBytes += bytes.size();
        if (nextBytes > 64 * 1024 * 1024)
          throw std::runtime_error("Saved results exceed 64 MiB");
        auto parsed = parse_blast(bytes, nextBatches[i]);
        if (nextHits.size() + parsed.size() > 500000)
          throw std::runtime_error("Saved hit count exceeds limit");
        nextHits.insert(nextHits.end(), parsed.begin(), parsed.end());
      }
      const auto resumeMessage =
          QString("Resume saved %1 / %2 job, batch %3/%4 (%5 unique "
                  "queries)?\nBackend: %6\nSaved options are used. Completed "
                  "results are retained and the saved RID is reused.")
              .arg(nextOptions.database, nextOptions.task)
              .arg(std::min(nextBatch + 1, nextBatches.size()))
              .arg(nextBatches.size())
              .arg(nextQueries.size())
              .arg(nextOptions.toolkit()
                       ? "NCBI BLAST+ executable: " + nextOptions.launcher
                       : QString("Built-in HTTPS"));
      if (!confirm_submission(this, "Resume NCBI search?", resumeMessage))
        return;
      diagnostics = j["diagnostics"].toArray();
      while (diagnostics.size() > 400)
        diagnostics.removeFirst();
      details->clear();
      for (qsizetype n = 0; n < diagnostics.size(); ++n)
        diagnostics[n] = diagnostics[n].toString().left(8192);
      for (const auto &entry : diagnostics)
        details->appendPlainText(entry.toString());
      record(
          "Resume confirmed; " +
          QString(nextOptions.toolkit()
                      ? "an interrupted BLAST+ batch is resubmitted explicitly"
                      : "saved HTTP RID retained"));
      monitorStarted = 0;
      checkingVersion = false;
      clientVersionChecked = false;
      polls = 0;
      readyRetries = 0;
      emptyReplies = 0;
      lastServer.clear();
      options = std::move(nextOptions);
      queries = std::move(nextQueries);
      batches = std::move(nextBatches);
      batch = nextBatch;
      resultBytes = nextBytes;
      completed = std::move(nextCompleted);
      rid = std::move(nextRid);
      hits = std::move(nextHits);
      metadataCache = j["metadata"].toObject();
      jobPath = path;
      {
        QSignalBlocker blockIdentity(identity), blockCoverage(coverage),
            blockExpect(expect);
        identity->setValue(options.identity);
        coverage->setValue(options.coverage);
        expect->setValue(options.expect);
      }
      refresh();
      metadata = false;
      set_running(true);
      ++generation;
      schedule(rid.isEmpty() ? 0 : services.pollMs);
    } catch (const std::exception &e) {
      const auto message =
          QString("Could not resume job; current results retained: ") +
          e.what();
      status->setText(message);
      QMessageBox::warning(this, "Resume BLAST job", message);
    }
  }

  void export_table(bool excel) {
    try {
      const auto ext = excel ? ".xlsx" : ".tsv";
      auto path = choose_output_filename(
          this, excel ? "Save BLAST XLSX" : "Save BLAST TSV", {},
          excel ? "Excel workbook (*.xlsx)" : "Tab-separated table (*.tsv)",
          nullptr, QFileDialog::DontConfirmOverwrite);
      if (path.isEmpty())
        return;
      if (QFileInfo(path).suffix().isEmpty())
        path += ext;
      path = approve_output_path(this, path, OutputFormat::Table, source.get());
      if (path.isEmpty())
        return;
      const auto bytes = excel ? blast_xlsx(model.rows, false, protein)
                               : blast_tsv(model.rows);
      const auto error = write_export_atomic(
          path, OutputFormat::Table, source.get(),
          [&](auto &out) { out.write(bytes.constData(), bytes.size()); });
      status->setText(error.isEmpty() ? "Saved table: " + path : error);
    } catch (const std::exception &e) {
      status->setText(e.what());
    }
  }
};
} // namespace
void blast_dialog(std::shared_ptr<Document> d,
                  const std::vector<SequenceId> &ids, bool protein,
                  QWidget *parent, BlastServices services) {
  if (protein) {
    local_blast_dialog(std::move(d), ids, true, parent);
    return;
  }
  Dialog dialog(std::move(d), ids, false, parent, std::move(services));
  dialog.exec();
}
} // namespace kleiotide
