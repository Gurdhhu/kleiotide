#include "gui/dialog_policy.hpp"
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTimer>
namespace kleiotide {
namespace {
QString field_label(QWidget *widget) {
  auto *parent = widget->parentWidget();
  if (!parent || !parent->layout())
    return {};
  auto *form = qobject_cast<QFormLayout *>(parent->layout());
  if (form)
    if (auto *label = qobject_cast<QLabel *>(form->labelForField(widget)))
      return label->text();
  return {};
}
class Policy final : public QObject {
  bool eventFilter(QObject *object, QEvent *event) override {
    if (event->type() == QEvent::Show)
      if (auto *dialog = qobject_cast<QDialog *>(object)) {
        for (auto *widget : dialog->findChildren<QWidget *>()) {
          bool inTable = false;
          for (auto *parent = widget->parentWidget();
               parent && parent != dialog; parent = parent->parentWidget())
            if (qobject_cast<QAbstractItemView *>(parent)) {
              inTable = true;
              break;
            }
          if (inTable)
            continue;
          if (auto *spin = qobject_cast<QAbstractSpinBox *>(widget)) {
            const int width = std::min(
                spin->sizeHint().width(),
                spin->fontMetrics().horizontalAdvance("000000000000.00") + 40);
            spin->setMaximumWidth(width);
            spin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
          } else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            if (!combo->isEditable() &&
                combo->maximumWidth() == QWIDGETSIZE_MAX) {
              combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
              combo->setMaximumWidth(420);
              combo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            }
          }
          if (!widget->toolTip().isEmpty())
            continue;
          if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
            auto title = button->text();
            title.remove('&');
            title.remove(QChar(0x2026));
            QString tip;
            if (title == "Apply")
              tip = "Apply the previewed changes.";
            else if (title == "Cancel")
              tip = "Close without applying pending changes.";
            else if (title == "Close")
              tip = "Close this window.";
            else if (title == "Preview")
              tip = "Calculate and review the result.";
            else if (title == "Browse..." || title == "Browse")
              tip = "Choose a file on this computer.";
            else if (title == "Restore Defaults" || title == "Restore defaults")
              tip = "Reset the displayed settings to their defaults.";
            else if (title == "OK")
              tip = "Apply these choices and close.";
            else if (qobject_cast<QCheckBox *>(button))
              tip = title + ".";
            else if (!title.isEmpty())
              tip = title + ".";
            button->setToolTip(tip);
          } else {
            auto label = field_label(widget);
            if (!label.isEmpty()) {
              label.remove('&');
              widget->setToolTip(
                  (qobject_cast<QComboBox *>(widget) ? "Choose " : "Set ") +
                  label.toLower() + ".");
            }
          }
        }
      }
    return QObject::eventFilter(object, event);
  }
};
} // namespace
void install_dialog_policy() {
  static Policy *policy = nullptr;
  if (!policy && qApp) {
    policy = new Policy;
    policy->setParent(qApp);
    qApp->installEventFilter(policy);
  }
}
} // namespace kleiotide
