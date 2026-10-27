#pragma once
#include "core/translation_types.hpp"
#include "gui/residue_style.hpp"
#include <QFont>
#include <QObject>
#include <optional>
namespace kleiotide {
struct AppPreferences {
    QFont font;
    ResiduePalette palette=ResiduePalette::Classic;
    ResidueDisplay display=ResidueDisplay::Letters;
    bool identityDots=false,viewOnly=false,insert=false,overviewVisible=true,traceVisible=true;
    int rightClick=0,fastaImport=0;
    unsigned code=1;
    std::optional<GeneticCode> customCode;
    int bulkMiB=2048,undoMiB=64,overviewMiB=16,scanDelayMs=150;
    QString mafftLauncher;
    int mafftThreads=0;
    bool recoveryEnabled=true;
    int recoverySeconds=120,recoveryQuotaMiB=1024;
    static AppPreferences defaults();
    TranslationSettings translation()const;
    QString error()const;
    bool operator==(const AppPreferences&)const=default;
};
// All persisted defaults use one validated, versioned QSettings value. Session
// overrides are never written here implicitly by editing or per-job dialogs.
class AppSettings final:public QObject {
    Q_OBJECT
public:
    explicit AppSettings(QObject* parent=nullptr);
    static AppSettings& instance();
    const AppPreferences& values()const{return values_;}
    bool apply(const AppPreferences&,QString* error=nullptr);
    void reload();
    static std::size_t bytes(int mib);
signals:
    void changed();
private:
    AppPreferences values_;
};
}
