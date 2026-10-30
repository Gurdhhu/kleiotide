#include "gui/settings_dialog.hpp"
#include "gui/blast_data.hpp"
#include "gui/mafft_dialog.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>
namespace kleiotide {
void show_settings(
    QWidget *parent, const AppPreferences &appearance,
    const TranslationSettings &currentCode,
    const std::function<void(const AppPreferences &)> &applyAppearance) {
  auto &service = AppSettings::instance();
  auto seed = service.values();
  QDialog dialog(parent);
  dialog.setObjectName("appSettings");
  dialog.setWindowTitle("Settings");
  dialog.resize(780, 670);
  QVBoxLayout layout(&dialog);
  QLabel scope(
      "Appearance, editing and translation defaults apply to new clean "
      "sessions. Existing sessions keep their own values. Resource and "
      "recovery settings apply to subsequent work in all windows.");
  scope.setWordWrap(true);
  layout.addWidget(&scope);
  QTabWidget tabs;
  layout.addWidget(&tabs, 1);
  auto page = [&](const QString &name) {
    auto *scroll = new QScrollArea(&tabs);
    scroll->setWidgetResizable(true);
    auto *body = new QWidget(scroll);
    auto *form = new QFormLayout(body);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    scroll->setWidget(body);
    tabs.addTab(scroll, name);
    return form;
  };
  auto spin = [&](QFormLayout *form, const char *id, const QString &label,
                  int low, int high, const QString &suffix = {}) {
    auto *s = new QSpinBox(&dialog);
    s->setObjectName(id);
    s->setRange(low, high);
    s->setSuffix(suffix);
    form->addRow(label, s);
    return s;
  };
  auto combo = [&](QFormLayout *form, const char *id, const QString &label,
                   const QStringList &options) {
    auto *c = new QComboBox(&dialog);
    c->setObjectName(id);
    c->addItems(options);
    form->addRow(label, c);
    return c;
  };
  auto check = [&](QFormLayout *form, const char *id, const QString &text) {
    auto *c = new QCheckBox(text, &dialog);
    c->setObjectName(id);
    form->addRow(c);
    return c;
  };
  auto note = [&](QFormLayout *form, const QString &text) {
    auto *l = new QLabel(text, &dialog);
    l->setWordWrap(true);
    form->addRow(l);
  };
  auto *look = page("Appearance and editing");
  QFontComboBox family;
  family.setObjectName("settingsFont");
  family.setFontFilters(QFontComboBox::MonospacedFonts);
  look->addRow("Alignment font", &family);
  auto *points = spin(look, "settingsFontSize", "Size", 6, 48, " pt");
  auto *bold = check(look, "settingsBold", "Bold");
  auto *italic = check(look, "settingsItalic", "Italic");
  auto *palette = combo(look, "settingsPalette", "Residue palette", {});
  for (int i = 0; i < residue_palette_count; ++i)
    palette->addItem(palette_label(static_cast<ResiduePalette>(i)));
  auto *display =
      combo(look, "settingsDisplay", "Residue display",
            {"Colored letters", "Monochrome", "Colored backgrounds"});
  auto *dots = check(look, "settingsDots", "Identity dots");
  QLabel preview;
  preview.setObjectName("settingsPreview");
  preview.setMinimumHeight(65);
  look->addRow("Preview", &preview);
  auto *mode =
      combo(look, "settingsViewMode", "Initial mode", {"Edit", "View"});
  auto *editing = combo(look, "settingsEditMode", "Remembered edit mode",
                        {"Overwrite", "Insert"});
  auto *right = combo(look, "settingsRightClick", "Editing right-click",
                      {"Context menu", "Insert gap", "Delete open gap",
                       "Insert gaps in others", "Delete open gaps in others"});
  auto *overview = check(look, "settingsOverview", "Show alignment overview");
  auto *traces = check(look, "settingsTrace", "Show traces when available");
  QPushButton useLook("Use current session appearance as defaults");
  useLook.setObjectName("settingsUseAppearance");
  look->addRow(&useLook);
  QPushButton applyLook("Apply to current session");
  applyLook.setObjectName("settingsApplyAppearance");
  applyLook.setEnabled(bool(applyAppearance));
  look->addRow(&applyLook);
  note(look, "Apply to current session copies this page's appearance and "
             "editing choices to this window immediately. It does not change "
             "global defaults or the session's genetic code. Cancel does not "
             "undo this explicit application.");
  auto *memory = page("Memory and performance");
  auto *bulk = spin(memory, "settingsBulkMiB",
                    "Additional bulk-edit working limit", 1, 65536, " MiB");
  auto *undo =
      spin(memory, "settingsUndoMiB", "Undo retention target", 1, 8192, " MiB");
  auto *workspace = spin(memory, "settingsOverviewMiB",
                         "Overview workspace limit", 1, 1024, " MiB");
  auto *delay = spin(memory, "settingsScanDelay", "Overview update delay", 50,
                     5000, " ms");
  note(memory,
       "These limits do not cap total process RAM. The document, original "
       "trace evidence, other analyses, allocator overhead and external MAFFT "
       "use additional memory. Raising a limit does not reserve RAM or "
       "guarantee success. Lowering the Undo target may prune older commands; "
       "the newest command is retained even if it alone exceeds the target. "
       "Running jobs retain their captured limits.");
  auto *translation = page("Translation and files");
  auto *code = combo(translation, "settingsCode", "Default genetic code", {});
  auto fillCodes = [&] {
    code->clear();
    for (const auto &c : genetic_codes())
      code->addItem(QString::number(c.id) + " — " +
                        QString::fromStdString(c.name),
                    int(c.id));
    if (seed.customCode)
      code->addItem("Custom: " + QString::fromStdString(seed.customCode->name),
                    0);
  };
  QPushButton useCode("Use current session's genetic code");
  useCode.setObjectName("settingsUseCode");
  translation->addRow(&useCode);
  note(translation, "Only the genetic-code definition becomes a default. "
                    "Reading frame and coding-region coordinates stay in each "
                    "document. Custom codes are stored in full.");
  auto *imports =
      combo(translation, "settingsFastaImport", "Default FASTA import type",
            {"Nucleotides + masks", "Proteins + masks", "Masks only"});
  auto pathRow = [&](QFormLayout *form, const QString &label, QLineEdit *field,
                     QPushButton *button) {
    auto *row = new QWidget;
    auto *line = new QHBoxLayout(row);
    line->setContentsMargins(0, 0, 0, 0);
    line->addWidget(field, 1);
    line->addWidget(button);
    form->addRow(label, row);
  };
  auto *mafft = page("MAFFT");
  QLineEdit launcher;
  launcher.setObjectName("settingsMafftLauncher");
  QPushButton browse("Browse…");
  pathRow(mafft, "Default launcher", &launcher, &browse);
  auto *threads =
      spin(mafft, "settingsMafftThreads", "Default threads", 0, 256);
  threads->setSpecialValueText("Automatic (safe)");
  note(
      mafft,
      "Automatic resolves to a positive logical-thread count, at most 8. Align "
      "dialogs can override this for one job without changing the global "
      "default. Keep the full Windows MAFFT runtime beside its launcher.");
  QPushButton test("Test a small alignment");
  test.setObjectName("settingsMafftTest");
  mafft->addRow(&test);
  QPlainTextEdit testReport;
  testReport.setObjectName("settingsMafftReport");
  testReport.setReadOnly(true);
  testReport.setMinimumHeight(160);
  mafft->addRow(&testReport);
  auto *blastPage = page("BLAST");
  auto *remoteBox = new QGroupBox("NCBI searches over HTTPS");
  auto *blast = new QFormLayout(remoteBox);
  blast->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  blastPage->addRow(remoteBox);
  QLineEdit blastEmail;
  blastEmail.setObjectName("settingsBlastEmail");
  blast->addRow("NCBI contact email", &blastEmail);
  auto *ndb = combo(blast, "settingsBlastNdb", "Nucleotide database",
                    blast_databases(false));
  auto *ntask = combo(blast, "settingsBlastNtask", "Nucleotide task",
                      {"megablast", "blastn"});
  auto *blastHits =
      spin(blast, "settingsBlastHits", "Max target sequences", 1, 5000);
  QLineEdit blastExpect;
  blastExpect.setObjectName("settingsBlastExpect");
  blastExpect.setMaximumWidth(180);
  blast->addRow("E-value threshold (e.g. 0.05 or 1e-10)", &blastExpect);
  auto filter = [&](const char *id, const QString &text) {
    auto *w = new QDoubleSpinBox;
    w->setObjectName(id);
    w->setRange(0, 100);
    w->setDecimals(2);
    blast->addRow(text, w);
    return w;
  };
  auto *blastIdentity =
      filter("settingsBlastIdentity", "Local identity filter (%)");
  auto *blastCover =
      filter("settingsBlastCoverage", "Local query-cover filter (%)");
  auto *blastNmask = check(blast, "settingsBlastNmask",
                           "Nucleotide low-complexity filtering (DUST)");
  auto *blastWait = spin(blast, "settingsBlastWait",
                         "Maximum monitoring time per batch", 1, 1440, " min");
  note(blast, "New NCBI searches use HTTPS. These defaults populate new "
              "dialogs; per-search changes do not rewrite them.");
  blastEmail.setToolTip(
      "Contact email required when submitting an NCBI search.");
  auto *localBox = new QGroupBox("Local BLAST+ searches and database creation");
  auto *local = new QFormLayout(localBox);
  blastPage->addRow(localBox);
  QLineEdit blastn, blastp, blastx, tblastn, tblastx, makeblastdb;
  blastn.setObjectName("settingsBlastn");
  blastp.setObjectName("settingsBlastp");
  blastx.setObjectName("settingsBlastx");
  tblastn.setObjectName("settingsTblastn");
  tblastx.setObjectName("settingsTblastx");
  makeblastdb.setObjectName("settingsMakeblastdb");
  QPushButton browseBlastn("Browse…"), browseBlastp("Browse…"),
      browseBlastx("Browse…"), browseTblastn("Browse…"),
      browseTblastx("Browse…"), browseMakeblastdb("Browse…");
  auto executable = [&](const QString &name, QLineEdit *field,
                        QPushButton *button) {
    pathRow(local, "NCBI " + name + " executable", field, button);
    field->setToolTip("Path to the installed NCBI " + name +
                      " executable; blank searches beside the other BLAST+ "
                      "tools and on PATH.");
    QObject::connect(button, &QPushButton::clicked, &dialog, [&, name, field] {
      auto path = QFileDialog::getOpenFileName(
          &dialog, "Choose NCBI " + name, field->text(),
          "Executables (*.exe);;All files (*)");
      if (!path.isEmpty())
        field->setText(path);
    });
  };
  executable("blastn", &blastn, &browseBlastn);
  executable("blastp", &blastp, &browseBlastp);
  executable("blastx", &blastx, &browseBlastx);
  executable("tblastn", &tblastn, &browseTblastn);
  executable("tblastx", &tblastx, &browseTblastx);
  executable("makeblastdb", &makeblastdb, &browseMakeblastdb);
  auto *localThreads =
      spin(local, "settingsLocalBlastThreads", "CPU threads", 1, 256);
  auto *localMinutes = spin(local, "settingsLocalBlastMinutes",
                            "Maximum running time", 1, 1440, " min");
  auto *localHits = spin(local, "settingsLocalBlastHits",
                         "Maximum target sequences", 1, 5000);
  QLineEdit localExpect;
  localExpect.setObjectName("settingsLocalBlastExpect");
  localExpect.setMaximumWidth(180);
  local->addRow("E-value threshold", &localExpect);
  auto localFilter = [&](const char *id, const QString &label) {
    auto *field = new QDoubleSpinBox;
    field->setObjectName(id);
    field->setRange(0, 100);
    field->setDecimals(2);
    local->addRow(label, field);
    return field;
  };
  auto *localIdentity =
      localFilter("settingsLocalBlastIdentity", "Minimum identity (%)");
  auto *localCover =
      localFilter("settingsLocalBlastCover", "Minimum query cover (%)");
  auto *localNmask =
      check(local, "settingsLocalBlastNmask", "Nucleotide DUST filtering");
  auto *localPmask =
      check(local, "settingsLocalBlastPmask", "Protein SEG filtering");
  note(local, "Install NCBI BLAST+ separately. Create databases from FASTA or "
              ".kleio files in the BLAST menu. Local searches use these tools "
              "on this computer.");
  auto *recovery = page("Recovery");
  auto *enabled = check(recovery, "settingsRecovery",
                        "Enable periodic recovery checkpoints");
  auto *seconds = spin(recovery, "settingsRecoverySeconds",
                       "Checkpoint interval", 15, 3600, " s");
  auto *quota = spin(recovery, "settingsRecoveryQuota",
                     "Retained recovery disk quota", 16, 65536, " MiB");
  const auto recoveryPath =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
      "/recovery";
  note(recovery, "Up to two completed generations per document are retained "
                 "within the disk quota. Writing temporarily pauses mutations "
                 "in that window; it does not copy the whole document. Busy "
                 "jobs defer checkpointing. Recovery never overwrites the "
                 "source file and does not mark work as saved. A replacement "
                 "can temporarily use disk space beyond the retained quota.");
  QLineEdit location(QDir::toNativeSeparators(recoveryPath));
  location.setReadOnly(true);
  location.setObjectName("settingsRecoveryLocation");
  recovery->addRow("Recovery folder", &location);
  QPushButton openRecovery("Open recovery folder");
  recovery->addRow(&openRecovery);
  note(recovery, "Turning recovery off or restoring preference defaults does "
                 "not delete existing checkpoints. File → Recover session… "
                 "lets you open or discard available checkpoints.");
  QLabel error;
  error.setObjectName("settingsError");
  error.setWordWrap(true);
  error.setStyleSheet("color:#A03020");
  layout.addWidget(&error);
  QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Apply |
                           QDialogButtonBox::Cancel |
                           QDialogButtonBox::RestoreDefaults);
  layout.addWidget(&buttons);
  auto chosenFont = [&] {
    auto f = family.currentFont();
    f.setPointSize(points->value());
    f.setBold(bold->isChecked());
    f.setItalic(italic->isChecked());
    return f;
  };
  auto render = [&] {
    QPixmap pixmap(360, 65);
    pixmap.fill(Qt::white);
    QPainter p(&pixmap);
    p.setFont(chosenFont());
    const QFontMetrics fm(chosenFont());
    int x = 6;
    for (char c : std::string("ACGTRY N~-")) {
      const auto paint =
          residue_paint(c, static_cast<ResiduePalette>(palette->currentIndex()),
                        static_cast<ResidueDisplay>(display->currentIndex()));
      const int width =
          std::max(12, fm.horizontalAdvance(QChar::fromLatin1(c)) + 4);
      p.fillRect(x, 4, width, 56, paint.background);
      p.setPen(paint.foreground);
      p.drawText(QRect(x, 4, width, 56), Qt::AlignCenter,
                 QString(QChar::fromLatin1(c)));
      x += width;
    }
    preview.setPixmap(pixmap);
  };
  auto read = [&] {
    auto p = seed;
    p.font = chosenFont();
    p.palette = static_cast<ResiduePalette>(palette->currentIndex());
    p.display = static_cast<ResidueDisplay>(display->currentIndex());
    p.identityDots = dots->isChecked();
    p.viewOnly = mode->currentIndex() == 1;
    p.insert = editing->currentIndex() == 1;
    p.rightClick = right->currentIndex();
    p.overviewVisible = overview->isChecked();
    p.traceVisible = traces->isChecked();
    p.code = code->currentData().toUInt();
    p.fastaImport = imports->currentIndex();
    p.bulkMiB = bulk->value();
    p.undoMiB = undo->value();
    p.overviewMiB = workspace->value();
    p.scanDelayMs = delay->value();
    p.mafftLauncher = launcher.text().trimmed();
    p.mafftThreads = threads->value();
    p.blastEmail = blastEmail.text().trimmed();
    p.blastnLauncher = blastn.text().trimmed();
    p.blastpLauncher = blastp.text().trimmed();
    p.blastxLauncher = blastx.text().trimmed();
    p.tblastnLauncher = tblastn.text().trimmed();
    p.tblastxLauncher = tblastx.text().trimmed();
    p.makeblastdbLauncher = makeblastdb.text().trimmed();
    p.localBlastThreads = localThreads->value();
    p.localBlastMinutes = localMinutes->value();
    p.localBlastHits = localHits->value();
    p.localBlastExpect = localExpect.text().toDouble();
    p.localBlastIdentity = localIdentity->value();
    p.localBlastCoverage = localCover->value();
    p.localBlastNmask = localNmask->isChecked();
    p.localBlastPmask = localPmask->isChecked();
    p.blastNucleotideDatabase = ndb->currentText();
    p.blastNucleotideTask = ntask->currentText();
    p.blastMaxHits = blastHits->value();
    p.blastWaitMinutes = blastWait->value();
    p.blastExpect = blastExpect.text().toDouble();
    p.blastIdentity = blastIdentity->value();
    p.blastCoverage = blastCover->value();
    p.blastExclude = false;
    p.blastNucleotideMask = blastNmask->isChecked();
    p.blastUseToolkit = false;
    p.recoveryEnabled = enabled->isChecked();
    p.recoverySeconds = seconds->value();
    p.recoveryQuotaMiB = quota->value();
    return p;
  };
  auto fill = [&](const AppPreferences &p) {
    seed = p;
    family.setCurrentFont(p.font);
    points->setValue(p.font.pointSize());
    bold->setChecked(p.font.bold());
    italic->setChecked(p.font.italic());
    palette->setCurrentIndex(int(p.palette));
    display->setCurrentIndex(int(p.display));
    dots->setChecked(p.identityDots);
    mode->setCurrentIndex(p.viewOnly ? 1 : 0);
    editing->setCurrentIndex(p.insert ? 1 : 0);
    right->setCurrentIndex(p.rightClick);
    overview->setChecked(p.overviewVisible);
    traces->setChecked(p.traceVisible);
    fillCodes();
    code->setCurrentIndex(code->findData(int(p.code)));
    imports->setCurrentIndex(p.fastaImport);
    bulk->setValue(p.bulkMiB);
    undo->setValue(p.undoMiB);
    workspace->setValue(p.overviewMiB);
    delay->setValue(p.scanDelayMs);
    launcher.setText(p.mafftLauncher);
    threads->setValue(p.mafftThreads);
    blastEmail.setText(p.blastEmail);
    blastn.setText(p.blastnLauncher);
    blastp.setText(p.blastpLauncher);
    blastx.setText(p.blastxLauncher);
    tblastn.setText(p.tblastnLauncher);
    tblastx.setText(p.tblastxLauncher);
    makeblastdb.setText(p.makeblastdbLauncher);
    localThreads->setValue(p.localBlastThreads);
    localMinutes->setValue(p.localBlastMinutes);
    localHits->setValue(p.localBlastHits);
    localExpect.setText(QString::number(p.localBlastExpect, 'g', 12));
    localIdentity->setValue(p.localBlastIdentity);
    localCover->setValue(p.localBlastCoverage);
    localNmask->setChecked(p.localBlastNmask);
    localPmask->setChecked(p.localBlastPmask);
    ndb->setCurrentText(p.blastNucleotideDatabase);
    ntask->setCurrentText(p.blastNucleotideTask);
    blastHits->setValue(p.blastMaxHits);
    blastWait->setValue(p.blastWaitMinutes);
    blastExpect.setText(QString::number(p.blastExpect, 'g', 12));
    blastIdentity->setValue(p.blastIdentity);
    blastCover->setValue(p.blastCoverage);
    blastNmask->setChecked(p.blastNucleotideMask);
    enabled->setChecked(p.recoveryEnabled);
    seconds->setValue(p.recoverySeconds);
    quota->setValue(p.recoveryQuotaMiB);
    render();
  };
  fill(seed);
  QObject::connect(&family, &QFontComboBox::currentFontChanged, &dialog,
                   [&] { render(); });
  for (auto *s : {points})
    QObject::connect(s, &QSpinBox::valueChanged, &dialog, [&] { render(); });
  for (auto *c : {bold, italic})
    QObject::connect(c, &QCheckBox::toggled, &dialog, [&] { render(); });
  for (auto *c : {palette, display})
    QObject::connect(c, &QComboBox::currentIndexChanged, &dialog,
                     [&] { render(); });
  QObject::connect(&useLook, &QPushButton::clicked, &dialog, [&] {
    auto p = read();
    p.font = appearance.font;
    p.palette = appearance.palette;
    p.display = appearance.display;
    p.identityDots = appearance.identityDots;
    p.overviewVisible = appearance.overviewVisible;
    p.traceVisible = appearance.traceVisible;
    fill(p);
  });
  QObject::connect(&applyLook, &QPushButton::clicked, &dialog, [&] {
    const auto p = read();
    if (const auto failure = p.error(); !failure.isEmpty()) {
      error.setText(failure);
      return;
    }
    if (applyAppearance)
      applyAppearance(p);
  });
  QObject::connect(&useCode, &QPushButton::clicked, &dialog, [&] {
    auto p = read();
    p.code = currentCode.code;
    if (currentCode.custom)
      p.customCode = *currentCode.custom;
    fill(p);
  });
  QObject::connect(&browse, &QPushButton::clicked, &dialog, [&] {
    const auto path = QFileDialog::getOpenFileName(
        &dialog, "Select MAFFT launcher", launcher.text(),
        "MAFFT launchers (mafft mafft.bat *.exe *.cmd);;All files (*)");
    if (!path.isEmpty())
      launcher.setText(path);
  });
  QObject::connect(&test, &QPushButton::clicked, &dialog, [&] {
    auto source = std::make_shared<Document>();
    source->load({{"test1", "ACGTACGTAGCTAGCTACGTTGCA"},
                  {"test2", "ACGTACGTAGCTAGCTACGTTGCA"}});
    MafftConfiguration c;
    c.launcher = QFileInfo(launcher.text().trimmed()).absoluteFilePath();
    c.options.threads = threads->value() ? threads->value() : -1;
    MafftProgress progress(&dialog);
    progress.setAttribute(Qt::WA_DeleteOnClose, false);
    MafftRun run(source, c, {}, &progress);
    run.status = [&](const QString &s) { progress.set_status(s); };
    run.log = [&](const QString &s) { progress.append_log(s); };
    progress.cancel_requested = [&] { run.cancel(); };
    run.finished = [&](std::shared_ptr<Document> result, const QString &report,
                       const QString &failure, bool cancelled) {
      testReport.setPlainText((cancelled ? "Cancelled.\n"
                               : result  ? "Small alignment succeeded.\n"
                                         : "Small alignment failed.\n") +
                              report);
      progress.completed(cancelled ? QString{} : failure);
    };
    run.start();
    progress.exec();
  });
  QObject::connect(&openRecovery, &QPushButton::clicked, &dialog, [&] {
    if (QDir().mkpath(recoveryPath))
      QDesktopServices::openUrl(QUrl::fromLocalFile(recoveryPath));
  });
  auto apply = [&] {
    QString message;
    if (!service.apply(read(), &message)) {
      error.setText(message);
      return false;
    }
    seed = service.values();
    error.clear();
    return true;
  };
  QObject::connect(buttons.button(QDialogButtonBox::Apply),
                   &QPushButton::clicked, &dialog, [&] { apply(); });
  QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
    if (apply())
      dialog.accept();
  });
  QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog,
                   &QDialog::reject);
  QObject::connect(buttons.button(QDialogButtonBox::RestoreDefaults),
                   &QPushButton::clicked, &dialog, [&] {
                     fill(AppPreferences::defaults());
                     error.clear();
                   });
  if (auto *screen = dialog.screen()) {
    const auto available = screen->availableGeometry().size();
    dialog.resize(std::min(780, std::max(360, available.width() - 48)),
                  std::min(670, std::max(360, available.height() - 80)));
  }
  dialog.exec();
}
} // namespace kleiotide
