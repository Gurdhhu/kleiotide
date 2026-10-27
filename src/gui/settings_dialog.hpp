#pragma once
#include "gui/app_settings.hpp"
#include <QDialog>
namespace kleiotide {
// The callbacks capture current session values; defaults never implicitly alter it.
void show_settings(QWidget*,const AppPreferences& currentAppearance,const TranslationSettings& currentCode);
}
