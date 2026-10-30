#include "gui/app_settings.hpp"
#include "gui/blast_data.hpp"
#include "gui/dialog_policy.hpp"
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace kleiotide {
AppPreferences AppPreferences::defaults() {
  AppPreferences p;
  p.font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  p.font.setPointSize(11);
  p.mafftLauncher = QStandardPaths::findExecutable("mafft");
  return p;
}
TranslationSettings AppPreferences::translation() const {
  TranslationSettings t;
  t.code = code;
  if (!code && customCode)
    t.custom = std::make_shared<GeneticCode>(*customCode);
  return t;
}
QString AppPreferences::error() const {
  if (font.family().isEmpty() || font.pointSize() < 6 || font.pointSize() > 48)
    return "Choose an alignment font size from 6 to 48 points.";
  if (int(palette) < 0 || int(palette) >= residue_palette_count ||
      int(display) < 0 || int(display) > 2)
    return "Unknown alignment color choice.";
  if (rightClick < 0 || rightClick > 4 || fastaImport < 0 || fastaImport > 2)
    return "Unknown editing or import choice.";
  if (bulkMiB < 1 || bulkMiB > 65536 || undoMiB < 1 || undoMiB > 8192 ||
      overviewMiB < 1 || overviewMiB > 1024)
    return "Memory limits are outside the supported ranges.";
  if (scanDelayMs < 50 || scanDelayMs > 5000)
    return "Background scan delay must be 50–5000 milliseconds.";
  if (mafftThreads < 0 || mafftThreads > 256 || mafftLauncher.contains('\n') ||
      mafftLauncher.contains('\r') || mafftLauncher.contains('"'))
    return "Invalid MAFFT path or thread count.";
  if (!blastEmail.isEmpty() &&
      !QRegularExpression("^[^\\s@]+@[^\\s@]+\\.[^\\s@]+$")
           .match(blastEmail)
           .hasMatch())
    return "Enter a valid BLAST contact email or leave it empty.";
  for (const auto &path :
       {blastnLauncher, blastpLauncher, blastxLauncher, tblastnLauncher,
        tblastxLauncher, makeblastdbLauncher})
    if (path.contains('\n') || path.contains('\r') || path.contains('"'))
      return "Invalid BLAST+ executable path.";
  if (!blast_databases(false).contains(blastNucleotideDatabase) ||
      !blast_databases(true).contains(blastProteinDatabase))
    return "Choose a supported BLAST database.";
  if (!QStringList{"megablast", "blastn", "dc-megablast", "blastn-short"}
           .contains(blastNucleotideTask) ||
      !QStringList{"blastp", "blastp-short", "blastp-fast"}.contains(
          blastProteinTask))
    return "Choose a supported BLAST task.";
  if (blastMaxHits < 1 || blastMaxHits > 5000 || blastWaitMinutes < 1 ||
      blastWaitMinutes > 1440 || !std::isfinite(blastExpect) ||
      blastExpect <= 0 || blastExpect > 1e6 || !std::isfinite(blastIdentity) ||
      blastIdentity < 0 || blastIdentity > 100 ||
      !std::isfinite(blastCoverage) || blastCoverage < 0 || blastCoverage > 100)
    return "Invalid BLAST defaults or monitoring time limit.";
  if (localBlastThreads < 1 || localBlastThreads > 256 ||
      localBlastMinutes < 1 || localBlastMinutes > 1440 || localBlastHits < 1 ||
      localBlastHits > 5000 || !std::isfinite(localBlastExpect) ||
      localBlastExpect <= 0 || localBlastExpect > 1e6 ||
      !std::isfinite(localBlastIdentity) || localBlastIdentity < 0 ||
      localBlastIdentity > 100 || !std::isfinite(localBlastCoverage) ||
      localBlastCoverage < 0 || localBlastCoverage > 100)
    return "Invalid local BLAST defaults.";
  if (recoverySeconds < 15 || recoverySeconds > 3600 || recoveryQuotaMiB < 16 ||
      recoveryQuotaMiB > 65536)
    return "Invalid recovery interval or disk quota.";
  try {
    if (customCode)
      validate_genetic_code(*customCode);
    validate_translation_settings(translation());
    (void)AppSettings::bytes(bulkMiB);
    (void)AppSettings::bytes(undoMiB);
    (void)AppSettings::bytes(overviewMiB);
    (void)AppSettings::bytes(recoveryQuotaMiB);
  } catch (const std::exception &e) {
    return QString::fromUtf8(e.what());
  }
  return {};
}
std::size_t AppSettings::bytes(int mib) {
  if (mib < 0 || std::size_t(mib) >
                     std::numeric_limits<std::size_t>::max() / (1024 * 1024))
    throw std::length_error("Memory limit is too large on this computer.");
  return std::size_t(mib) * 1024 * 1024;
}
namespace {
QJsonObject encode(const AppPreferences &p) {
  QJsonObject j{{"version", 1},
                {"font", p.font.toString()},
                {"palette", palette_id(p.palette)},
                {"display", display_id(p.display)},
                {"dots", p.identityDots},
                {"view", p.viewOnly},
                {"insert", p.insert},
                {"overview", p.overviewVisible},
                {"trace", p.traceVisible},
                {"rightClick", p.rightClick},
                {"import", p.fastaImport},
                {"code", int(p.code)},
                {"bulkMiB", p.bulkMiB},
                {"undoMiB", p.undoMiB},
                {"overviewMiB", p.overviewMiB},
                {"scanDelay", p.scanDelayMs},
                {"mafft", p.mafftLauncher},
                {"threads", p.mafftThreads},
                {"blastx", p.blastxLauncher},
                {"tblastn", p.tblastnLauncher},
                {"tblastx", p.tblastxLauncher},
                {"makeblastdb", p.makeblastdbLauncher},
                {"localThreads", p.localBlastThreads},
                {"localMinutes", p.localBlastMinutes},
                {"localHits", p.localBlastHits},
                {"localExpect", p.localBlastExpect},
                {"localIdentity", p.localBlastIdentity},
                {"localCoverage", p.localBlastCoverage},
                {"localNmask", p.localBlastNmask},
                {"localPmask", p.localBlastPmask},
                {"blastEmail", p.blastEmail},
                {"blastn", p.blastnLauncher},
                {"blastp", p.blastpLauncher},
                {"blastNdb", p.blastNucleotideDatabase},
                {"blastPdb", p.blastProteinDatabase},
                {"blastNtask", p.blastNucleotideTask},
                {"blastPtask", p.blastProteinTask},
                {"blastHits", p.blastMaxHits},
                {"blastWait", p.blastWaitMinutes},
                {"blastExpect", p.blastExpect},
                {"blastIdentity", p.blastIdentity},
                {"blastCoverage", p.blastCoverage},
                {"blastExclude", p.blastExclude},
                {"blastNmask", p.blastNucleotideMask},
                {"blastPmask", p.blastProteinMask},
                {"blastToolkit", p.blastUseToolkit},
                {"recovery", p.recoveryEnabled},
                {"interval", p.recoverySeconds},
                {"quotaMiB", p.recoveryQuotaMiB}};
  if (p.customCode)
    j["custom"] =
        QJsonObject{{"id", int(p.customCode->id)},
                    {"name", QString::fromStdString(p.customCode->name)},
                    {"amino", QString::fromStdString(p.customCode->aminoAcids)},
                    {"stops", QString::number(p.customCode->contextStops)}};
  return j;
}
AppPreferences decode(const QJsonObject &j) {
  auto p = AppPreferences::defaults();
  if (j["version"].toInt() != 1)
    return p;
  QFont f;
  if (!j["font"].toString().isEmpty() && f.fromString(j["font"].toString())) {
    if (!QFontDatabase::families().contains(f.family(), Qt::CaseInsensitive))
      f.setFamily(p.font.family());
    p.font = f;
  }
  p.palette = palette_from_id(j["palette"].toString());
  p.display = display_from_id(j["display"].toString());
  p.identityDots = j["dots"].toBool();
  p.viewOnly = j["view"].toBool();
  p.insert = j["insert"].toBool();
  p.overviewVisible = j["overview"].toBool(true);
  p.traceVisible = j["trace"].toBool(true);
  p.rightClick = j["rightClick"].toInt();
  p.fastaImport = j["import"].toInt();
  p.code = unsigned(j["code"].toInt(1));
  p.bulkMiB = j["bulkMiB"].toInt(2048);
  p.undoMiB = j["undoMiB"].toInt(64);
  p.overviewMiB = j["overviewMiB"].toInt(16);
  p.scanDelayMs = j["scanDelay"].toInt(150);
  p.mafftLauncher = j["mafft"].toString(p.mafftLauncher);
  p.mafftThreads = j["threads"].toInt();
  p.blastxLauncher = j["blastx"].toString();
  p.tblastnLauncher = j["tblastn"].toString();
  p.tblastxLauncher = j["tblastx"].toString();
  p.makeblastdbLauncher = j["makeblastdb"].toString();
  p.localBlastThreads = j["localThreads"].toInt(1);
  p.localBlastMinutes = j["localMinutes"].toInt(60);
  p.localBlastHits = j["localHits"].toInt(10);
  p.localBlastExpect = j["localExpect"].toDouble(.05);
  p.localBlastIdentity = j["localIdentity"].toDouble();
  p.localBlastCoverage = j["localCoverage"].toDouble();
  p.localBlastNmask = j["localNmask"].toBool(true);
  p.localBlastPmask = j["localPmask"].toBool(false);
  p.blastEmail = j["blastEmail"].toString();
  p.blastnLauncher = j["blastn"].toString();
  p.blastpLauncher = j["blastp"].toString();
  p.blastNucleotideDatabase = j["blastNdb"].toString("core_nt");
  p.blastProteinDatabase = j["blastPdb"].toString("nr");
  p.blastNucleotideTask = j["blastNtask"].toString("megablast");
  p.blastProteinTask = j["blastPtask"].toString("blastp");
  p.blastMaxHits = j["blastHits"].toInt(10);
  p.blastWaitMinutes = j["blastWait"].toInt(120);
  p.blastExpect = j["blastExpect"].toDouble(.05);
  p.blastIdentity = j["blastIdentity"].toDouble();
  p.blastCoverage = j["blastCoverage"].toDouble();
  p.blastExclude = j["blastExclude"].toBool();
  p.blastNucleotideMask = j["blastNmask"].toBool(true);
  p.blastProteinMask = j["blastPmask"].toBool(false);
  p.blastUseToolkit = j["blastToolkit"].toBool();
  if (p.blastNucleotideTask != "blastn" && p.blastNucleotideTask != "megablast")
    p.blastNucleotideTask = "megablast";
  p.blastProteinTask = "blastp";
  p.blastUseToolkit = false;
  p.blastExclude = false;
  p.recoveryEnabled = j["recovery"].toBool(true);
  p.recoverySeconds = j["interval"].toInt(120);
  p.recoveryQuotaMiB = j["quotaMiB"].toInt(1024);
  const auto custom = j["custom"].toObject();
  if (!custom.isEmpty()) {
    GeneticCode c;
    c.id = unsigned(custom["id"].toInt());
    c.name = custom["name"].toString().toStdString();
    c.aminoAcids = custom["amino"].toString().toStdString();
    c.contextStops = custom["stops"].toString().toULongLong();
    p.customCode = std::move(c);
  }
  return p.error().isEmpty() ? p : AppPreferences::defaults();
}
} // namespace
AppSettings::AppSettings(QObject *parent) : QObject(parent) {
  install_dialog_policy();
  reload();
}
AppSettings &AppSettings::instance() {
  static AppSettings settings;
  return settings;
}
void AppSettings::reload() {
  QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                     "Kleiotide", "Kleiotide");
  if (settings.contains("preferences/v1"))
    values_ = decode(
        QJsonDocument::fromJson(settings.value("preferences/v1").toByteArray())
            .object());
  else {
    values_ = AppPreferences::defaults();
    values_.mafftLauncher =
        settings.value("mafft/launcher", values_.mafftLauncher).toString();
    values_.mafftThreads =
        std::max(0, settings.value("mafft/threads", -1).toInt());
    if (!values_.error().isEmpty())
      values_ = AppPreferences::defaults();
  }
}
bool AppSettings::apply(const AppPreferences &p, QString *error) {
  const auto invalid = p.error();
  if (!invalid.isEmpty()) {
    if (error)
      *error = invalid;
    return false;
  }
  QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                     "Kleiotide", "Kleiotide");
  const auto old = settings.value("preferences/v1");
  settings.setValue("preferences/v1",
                    QJsonDocument(encode(p)).toJson(QJsonDocument::Compact));
  settings.sync();
  if (settings.status() != QSettings::NoError) {
    if (old.isValid())
      settings.setValue("preferences/v1", old);
    else
      settings.remove("preferences/v1");
    if (error)
      *error = "Could not save global settings.";
    return false;
  }
  values_ = p;
  emit changed();
  return true;
}
} // namespace kleiotide
