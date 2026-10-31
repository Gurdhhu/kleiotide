#include "gui/dialog_policy.hpp"
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFontDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPlainTextEdit>
#include <QProxyStyle>
#include <QPushButton>
#include <QSet>
#include <QSpinBox>
#include <QTableView>
#include <QTimer>
#include <memory>
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
class ButtonStyle final : public QProxyStyle {
public:
  ButtonStyle() : QProxyStyle("fusion") {}
  int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                const QWidget *widget = nullptr,
                QStyleHintReturn *result = nullptr) const override {
    if (hint == QStyle::SH_DialogButtonLayout)
      return QDialogButtonBox::WinLayout;
    return QProxyStyle::styleHint(hint, option, widget, result);
  }
};
class Policy final : public QObject {
  bool eventFilter(QObject *object, QEvent *event) override {
    if (event->type() == QEvent::Show)
      if (auto *dialog = qobject_cast<QDialog *>(object)) {
        // One-time dialog wiring: previews remain read-only until the named
        // action is clicked.
        if (!dialog->property("followupPolicy").toBool()) {
          dialog->setProperty("followupPolicy", true);
          for (auto *box : dialog->findChildren<QDialogButtonBox *>()) {
            auto *style = new ButtonStyle;
            style->setParent(box);
            box->setStyle(style);
            auto *cancel = box->button(QDialogButtonBox::Cancel);
            if (cancel) {
              QTimer::singleShot(0, box, [box, cancel] {
                if (auto *line = qobject_cast<QBoxLayout *>(box->layout())) {
                  line->removeWidget(cancel);
                  line->addWidget(cancel);
                }
              });
            }
          }
          auto buttonBoxes = dialog->findChildren<QDialogButtonBox *>();
          if (!buttonBoxes.isEmpty())
            for (auto *box : buttonBoxes) {
              for (auto *button : box->buttons())
                if (box->buttonRole(button) == QDialogButtonBox::RejectRole &&
                    !box->button(QDialogButtonBox::Cancel)) {
                  if (auto *close = qobject_cast<QPushButton *>(button)) {
                    auto *target = buttonBoxes.back();
                    if (target != box) {
                      box->removeButton(close);
                      target->addButton(close, QDialogButtonBox::RejectRole);
                    }
                    QTimer::singleShot(0, target, [target, close] {
                      if (auto *row =
                              qobject_cast<QBoxLayout *>(target->layout())) {
                        row->removeWidget(close);
                        row->addWidget(close);
                      }
                    });
                  }
                }
            }
          const QSet<QString> compact = {
              "sequenceTypeDialog",  "selectionDialog", "assignGroupDialog",
              "analysisScopeDialog", "mafftSettings",   "groupIdenticalDialog"};
          if (compact.contains(dialog->objectName()) ||
              qobject_cast<QFontDialog *>(dialog) ||
              dialog->windowTitle().startsWith("Select ") ||
              dialog->windowTitle().contains("Analysis scope",
                                             Qt::CaseInsensitive) ||
              dialog->windowTitle() == "Alignment font") {
            if (dialog->layout())
              dialog->layout()->setAlignment(Qt::AlignTop);
            QTimer::singleShot(0, dialog, [dialog] {
              dialog->resize(dialog->width(), dialog->sizeHint().height());
            });
          }
          for (auto *label : dialog->findChildren<QLabel *>())
            if (label->text() == "Identical sequence clusters" ||
                label->text() == "Members of the selected cluster") {
              auto font = label->font();
              font.setBold(true);
              font.setPointSize(font.pointSize() + 1);
              label->setFont(font);
            }
          if (dialog->objectName() == "localBlastDialog")
            if (auto *combo =
                    dialog->findChild<QComboBox *>("localBlastDatabase")) {
              combo->view()->setTextElideMode(Qt::ElideNone);
            }
          const QMap<QString, QStringList> workflows = {
              {"batchRenameDialog",
               {"renamePreviewButton", "renameApplyButton", "Rename"}},
              {"mergeDialog", {"mergePreview", "mergeApply", "Merge"}},
              {"traceQualityDialog",
               {"qualityPreviewButton", "qualityApply", "Perform edit"}},
              {"collapseDialog",
               {"collapsePreview", "collapseApply", "Collapse"}},
              {"traceLinkDialog",
               {"traceLinkPreview", "traceLinkApply", "Link traces"}}};
          if (workflows.contains(dialog->objectName())) {
            auto config = workflows[dialog->objectName()];
            if (dialog->objectName() == "traceQualityDialog")
              config[2] = dialog->windowTitle().startsWith("Trim")
                              ? "Trim ends"
                              : "Convert calls to N";
            auto *preview = dialog->findChild<QPushButton *>(config[0]);
            auto *commit = dialog->findChild<QPushButton *>(config[1]);
            if (preview && commit) {
              preview->hide();
              preview->setDefault(false);
              preview->setAutoDefault(false);
              commit->setDefault(true);
              commit->setText(config[2]);
              for (auto *w : dialog->findChildren<QWidget *>()) {
                auto tip = w->toolTip();
                tip.replace("Apply", config[2]);
                w->setToolTip(tip);
                if (auto *label = qobject_cast<QLabel *>(w)) {
                  auto text = label->text();
                  text.replace("Apply", config[2]);
                  label->setText(text);
                }
              }
              auto *timer = new QTimer(dialog);
              timer->setInterval(100);
              auto pending = std::make_shared<QString>();
              auto last = std::make_shared<QString>();
              auto changedAt = std::make_shared<qint64>(0);
              connect(
                  timer, &QTimer::timeout, dialog,
                  [dialog, preview, commit, pending, last, changedAt] {
                    QString signature;
                    for (auto *w : dialog->findChildren<QWidget *>()) {
                      auto name = w->objectName();
                      if (!(name.startsWith("rename") ||
                            name.startsWith("merge") ||
                            name.startsWith("quality") ||
                            name.startsWith("collapse")))
                        continue;
                      if (name == "mergeContigChoice" || name=="mergeName" || name=="collapseNaming" || name=="collapseSeparator")
                        continue;
                      bool inTable = false;
                      for (auto *p = w->parentWidget(); p && p != dialog;
                           p = p->parentWidget())
                        if (qobject_cast<QAbstractItemView *>(p))
                          inTable = true;
                      if (inTable)
                        continue;
                      if (auto *combo = qobject_cast<QComboBox *>(w)) {
                        signature += name + ":" +
                                     QString::number(combo->currentIndex()) + ";";
                      } else if (auto *check = qobject_cast<QCheckBox *>(w)) {
                        signature += name + ":" +
                                     QString::number(check->isChecked()) + ";";
                      } else if (auto *spin = qobject_cast<QSpinBox *>(w)) {
                        signature += name + ":" +
                                     QString::number(spin->value()) + ";";
                      } else if (auto *doubleSpin =
                                     qobject_cast<QDoubleSpinBox *>(w)) {
                        signature += name + ":" +
                                     QString::number(doubleSpin->value()) + ";";
                      } else if (auto *lineEdit = qobject_cast<QLineEdit *>(w)) {
                        signature += name + ":" + lineEdit->text() + ";";
                      } else if (auto *plainText =
                                     qobject_cast<QPlainTextEdit *>(w);
                                 plainText && !plainText->isReadOnly()) {
                        signature += name + ":" + plainText->toPlainText() + ";";
                      }
                    }
                    if (auto *table =
                            dialog->findChild<QTableView *>("traceAssignments"))
                      for (int row = 0; row < table->model()->rowCount(); ++row)
                        signature += QString::number(table->model()
                                                         ->index(row, 1)
                                                         .data(Qt::EditRole)
                                                         .toInt()) +
                                     ",";
                    signature += QString::number(
                        dialog->property("traceAssignmentsRevision")
                            .toLongLong());
                    const auto now = QDateTime::currentMSecsSinceEpoch();
                    if(*last=="__initial__"&&commit->isEnabled())*last=signature;
                    if (signature != *pending) {
                      *pending = signature;
                      *changedAt = now;
                      return;
                    }
                    if (now - *changedAt >= 250 && signature != *last &&
                        preview->isEnabled()) {
                      *last = signature;
                      preview->click();
                    }
                  });
              *last = "__initial__";
              timer->start();
            }
          }
        }

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
