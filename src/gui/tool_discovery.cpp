#include "gui/tool_discovery.hpp"
#include <QBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <utility>

namespace kleiotide {
QString find_local_tool(const QString& name, const QStringList& siblingLaunchers) {
    QStringList folders;
    for (const auto& launcher : siblingLaunchers)
        if (QFileInfo(launcher).isAbsolute()) folders.append(QFileInfo(launcher).absolutePath());
    QStringList binaries{name};
#ifdef Q_OS_WIN
    binaries = name == "mafft" ? QStringList{"mafft.bat", "mafft.cmd", "mafft.exe"} : QStringList{name + ".exe"};
    QSettings machine("HKEY_LOCAL_MACHINE\\SOFTWARE\\Kleiotide\\Tools", QSettings::Registry64Format);
    const auto installed = machine.value(name == "mafft" ? "MafftDirectory" : "BlastDirectory").toString();
    if (!installed.isEmpty()) folders.append(installed);
#endif
    folders.append(qEnvironmentVariable("PATH").split(QDir::listSeparator(), Qt::SkipEmptyParts));
#ifdef Q_OS_WIN
    const QDir ncbi(QDir(qEnvironmentVariable("ProgramFiles")).filePath("NCBI"));
    if (name != "mafft")
        for (const auto& version : ncbi.entryList({"blast-*"}, QDir::Dirs, QDir::Time))
            folders.append(ncbi.filePath(version + "/bin"));
#else
    folders.append({"/opt/homebrew/bin", "/usr/local/bin", "/usr/bin", "/opt/local/bin"});
#endif
    for (const auto& folder : folders) {
        if (!QDir::isAbsolutePath(folder)) continue;
        for (const auto& binary : binaries) {
            const QFileInfo candidate(QDir(folder).filePath(binary));
            if (candidate.isFile() && candidate.isExecutable()) return candidate.absoluteFilePath();
        }
    }
    return {};
}

void add_locate_button(QBoxLayout* layout, QLineEdit* field, std::function<QString()> locate) {
    auto* button = new QPushButton("Locate");
    button->setToolTip("Look for an installed copy on this computer.");
    layout->addWidget(button);
    QObject::connect(button, &QPushButton::clicked, field, [field, locate=std::move(locate)] {
        const auto path = locate();
        if (!path.isEmpty()) field->setText(path);
        else QMessageBox::information(field, "Locate executable", "No installed copy was found. Use Browse to choose the executable.");
    });
}
}
