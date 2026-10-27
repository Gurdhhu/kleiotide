#include "gui/app_settings.hpp"
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <limits>
#include <stdexcept>
namespace kleiotide {
AppPreferences AppPreferences::defaults(){AppPreferences p;p.font=QFontDatabase::systemFont(QFontDatabase::FixedFont);p.font.setPointSize(11);p.mafftLauncher=QStandardPaths::findExecutable("mafft");return p;}
TranslationSettings AppPreferences::translation()const{TranslationSettings t;t.code=code;if(!code&&customCode)t.custom=std::make_shared<GeneticCode>(*customCode);return t;}
QString AppPreferences::error()const{
    if(font.family().isEmpty()||font.pointSize()<6||font.pointSize()>48)return "Choose an alignment font size from 6 to 48 points.";
    if(int(palette)<0||int(palette)>=residue_palette_count||int(display)<0||int(display)>2)return "Unknown alignment color choice.";
    if(rightClick<0||rightClick>4||fastaImport<0||fastaImport>2)return "Unknown editing or import choice.";
    if(bulkMiB<1||bulkMiB>65536||undoMiB<1||undoMiB>8192||overviewMiB<1||overviewMiB>1024)return "Memory limits are outside the supported ranges.";
    if(scanDelayMs<50||scanDelayMs>5000)return "Background scan delay must be 50–5000 milliseconds.";
    if(mafftThreads<0||mafftThreads>256||mafftLauncher.contains('\n')||mafftLauncher.contains('\r')||mafftLauncher.contains('"'))return "Invalid MAFFT path or thread count.";
    if(recoverySeconds<15||recoverySeconds>3600||recoveryQuotaMiB<16||recoveryQuotaMiB>65536)return "Invalid recovery interval or disk quota.";
    try{if(customCode)validate_genetic_code(*customCode);validate_translation_settings(translation());(void)AppSettings::bytes(bulkMiB);(void)AppSettings::bytes(undoMiB);(void)AppSettings::bytes(overviewMiB);(void)AppSettings::bytes(recoveryQuotaMiB);}catch(const std::exception& e){return QString::fromUtf8(e.what());}
    return {};
}
std::size_t AppSettings::bytes(int mib){if(mib<0||std::size_t(mib)>std::numeric_limits<std::size_t>::max()/(1024*1024))throw std::length_error("Memory limit is too large on this computer.");return std::size_t(mib)*1024*1024;}
namespace {
QJsonObject encode(const AppPreferences& p){
    QJsonObject j{{"version",1},{"font",p.font.toString()},{"palette",palette_id(p.palette)},{"display",display_id(p.display)},
        {"dots",p.identityDots},{"view",p.viewOnly},{"insert",p.insert},{"overview",p.overviewVisible},{"trace",p.traceVisible},
        {"rightClick",p.rightClick},{"import",p.fastaImport},{"code",int(p.code)},{"bulkMiB",p.bulkMiB},{"undoMiB",p.undoMiB},
        {"overviewMiB",p.overviewMiB},{"scanDelay",p.scanDelayMs},{"mafft",p.mafftLauncher},{"threads",p.mafftThreads},
        {"recovery",p.recoveryEnabled},{"interval",p.recoverySeconds},{"quotaMiB",p.recoveryQuotaMiB}};
    if(p.customCode)j["custom"]=QJsonObject{{"id",int(p.customCode->id)},{"name",QString::fromStdString(p.customCode->name)},
        {"amino",QString::fromStdString(p.customCode->aminoAcids)},{"stops",QString::number(p.customCode->contextStops)}};
    return j;
}
AppPreferences decode(const QJsonObject& j){
    auto p=AppPreferences::defaults();
    if(j["version"].toInt()!=1)return p;
    QFont f;if(!j["font"].toString().isEmpty()&&f.fromString(j["font"].toString())){if(!QFontDatabase::families().contains(f.family(),Qt::CaseInsensitive))f.setFamily(p.font.family());p.font=f;}
    p.palette=palette_from_id(j["palette"].toString());p.display=display_from_id(j["display"].toString());
    p.identityDots=j["dots"].toBool();p.viewOnly=j["view"].toBool();p.insert=j["insert"].toBool();
    p.overviewVisible=j["overview"].toBool(true);p.traceVisible=j["trace"].toBool(true);
    p.rightClick=j["rightClick"].toInt();p.fastaImport=j["import"].toInt();p.code=unsigned(j["code"].toInt(1));
    p.bulkMiB=j["bulkMiB"].toInt(2048);p.undoMiB=j["undoMiB"].toInt(64);p.overviewMiB=j["overviewMiB"].toInt(16);p.scanDelayMs=j["scanDelay"].toInt(150);
    p.mafftLauncher=j["mafft"].toString(p.mafftLauncher);p.mafftThreads=j["threads"].toInt();
    p.recoveryEnabled=j["recovery"].toBool(true);p.recoverySeconds=j["interval"].toInt(120);p.recoveryQuotaMiB=j["quotaMiB"].toInt(1024);
    const auto custom=j["custom"].toObject();
    if(!custom.isEmpty()){GeneticCode c;c.id=unsigned(custom["id"].toInt());c.name=custom["name"].toString().toStdString();c.aminoAcids=custom["amino"].toString().toStdString();c.contextStops=custom["stops"].toString().toULongLong();p.customCode=std::move(c);}
    return p.error().isEmpty()?p:AppPreferences::defaults();
}
}
AppSettings::AppSettings(QObject* parent):QObject(parent){reload();}
AppSettings& AppSettings::instance(){static AppSettings settings;return settings;}
void AppSettings::reload(){
    QSettings settings(QSettings::defaultFormat(),QSettings::UserScope,"Kleiotide","Kleiotide");
    if(settings.contains("preferences/v1"))values_=decode(QJsonDocument::fromJson(settings.value("preferences/v1").toByteArray()).object());
    else{
        values_=AppPreferences::defaults();values_.mafftLauncher=settings.value("mafft/launcher",values_.mafftLauncher).toString();
        values_.mafftThreads=std::max(0,settings.value("mafft/threads",-1).toInt());
        if(!values_.error().isEmpty())values_=AppPreferences::defaults();
    }
}
bool AppSettings::apply(const AppPreferences& p,QString* error){
    const auto invalid=p.error();if(!invalid.isEmpty()){if(error)*error=invalid;return false;}
    QSettings settings(QSettings::defaultFormat(),QSettings::UserScope,"Kleiotide","Kleiotide");const auto old=settings.value("preferences/v1");
    settings.setValue("preferences/v1",QJsonDocument(encode(p)).toJson(QJsonDocument::Compact));settings.sync();
    if(settings.status()!=QSettings::NoError){if(old.isValid())settings.setValue("preferences/v1",old);else settings.remove("preferences/v1");if(error)*error="Could not save global settings.";return false;}
    values_=p;emit changed();return true;
}
}
