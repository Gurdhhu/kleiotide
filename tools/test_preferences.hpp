#pragma once
#include "gui/app_settings.hpp"
#include <QSettings>
#include <QTemporaryDir>
#include <stdexcept>
namespace kleiotide {
// GUI workflows must not read or modify a tester's real registry/configuration.
// AppSettings explicitly honors defaultFormat; named QSettings constructors
// otherwise select NativeFormat, even after setDefaultFormat(IniFormat).
class TestPreferences {
public:
    TestPreferences(){
        if(!directory_.isValid())throw std::runtime_error("Cannot create isolated test preferences.");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory_.path());
        QSettings::setPath(QSettings::IniFormat,QSettings::SystemScope,directory_.filePath("system"));
        auto& service=AppSettings::instance();service.reload();auto values=AppPreferences::defaults();values.recoveryEnabled=false;
        if(!service.apply(values))throw std::runtime_error("Cannot initialize isolated test preferences.");
    }
private:QTemporaryDir directory_;
};
}
