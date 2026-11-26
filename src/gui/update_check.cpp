#include "gui/update_check.hpp"
#include "core/version.hpp"
#include "gui/zip_entry.hpp"
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <memory>
#include <stdexcept>
#include <tuple>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace kleiotide {
namespace {
const QString repo = "https://github.com/Gurdhhu/kleiotide";
const QUrl releaseFeed("https://api.github.com/repos/Gurdhhu/kleiotide/releases?per_page=100");
constexpr qint64 maximumDownload = 192 * 1024 * 1024;

UpdatePlatform platform() {
    if (QSysInfo::buildCpuArchitecture() != "x86_64") return UpdatePlatform::Other;
#ifdef Q_OS_WIN
    return UpdatePlatform::WindowsX64;
#elif defined(Q_OS_LINUX)
    QFile release("/etc/os-release");
    if (release.open(QIODevice::ReadOnly)) {
        const auto lines = release.readAll().replace('"',"").split('\n');
        if (lines.contains("ID=debian") && lines.contains("VERSION_ID=13"))
            return UpdatePlatform::Debian13Amd64;
    }
#endif
    return UpdatePlatform::Other;
}

bool trusted_download(const QUrl& url, const QUrl& initial) {
    if (url.scheme() != "https" || !url.userInfo().isEmpty() || url.port(443) != 443) return false;
    return url == initial || url.host() == "release-assets.githubusercontent.com" ||
           url.host() == "objects.githubusercontent.com" ||
           url.host() == "github-releases.githubusercontent.com";
}
}

ReleaseUpdate find_release_update(const QByteArray& bytes, const QVersionNumber& current, UpdatePlatform target) {
    QJsonParseError error{};
    const auto releases = QJsonDocument::fromJson(bytes,&error);
    if (error.error != QJsonParseError::NoError || !releases.isArray())
        throw std::runtime_error("GitHub did not return a release list.");

    static const QRegularExpression tagPattern(R"(^v(\d{1,6}\.\d{1,6}\.\d{1,6})(?:-(rc|pre\.)(\d{1,6}))?$)");
    static const QRegularExpression hexPattern("^[a-fA-F0-9]{64}$");
    using Rank = std::tuple<QVersionNumber,int,int>;
    Rank latest, compatible;
    ReleaseUpdate linkOnly, selected;
    for (const auto& entry : releases.array()) {
        const auto release = entry.toObject();
        const auto tag = release["tag_name"].toString();
        const auto match = tagPattern.match(tag);
        if (release["draft"].toBool() || !match.hasMatch()) continue;
        const auto versionText = match.captured(1);
        const auto published = QVersionNumber::fromString(versionText);
        if (published <= current) continue;
        const auto style = match.captured(2);
        const bool preview = release["prerelease"].toBool() || !style.isEmpty();
        const Rank rank{published, preview ? (style == "rc" ? 1 : 0) : 2, match.captured(3).toInt()};

        ReleaseUpdate next;
        next.tag = tag;
        next.preview = preview;
        next.releaseUrl = QUrl(repo + "/releases/tag/" + tag);
        if (rank > latest) {
            latest = rank;
            linkOnly = next;
        }
        if (target == UpdatePlatform::Other || rank <= compatible) continue;

        const auto winStem = "Kleiotide_" + versionText +
            (style.isEmpty() ? QString{} : "_" + QString(tag).section('-',1)) + "_Windows_x64_Setup";
        const QRegularExpression debName("^kleiotide_" + QRegularExpression::escape(versionText) +
                                        "-[0-9]+[.~]deb13_amd64\\.deb$");
        for (const auto& item : release["assets"].toArray()) {
            const auto asset = item.toObject();
            const auto name = asset["name"].toString();
            const bool matches = target == UpdatePlatform::WindowsX64 ?
                (name == winStem + ".exe" || name == winStem + ".zip") : debName.match(name).hasMatch();
            const auto size = asset["size"].toInteger();
            const auto digest = asset["digest"].toString();
            const QUrl expected(repo + "/releases/download/" + tag + '/' + name);
            if (!matches || asset["state"].toString() != "uploaded" || size <= 0 ||
                size > maximumDownload || !digest.startsWith("sha256:") ||
                !hexPattern.match(digest.mid(7)).hasMatch() ||
                QUrl(asset["browser_download_url"].toString()) != expected)
                continue;
            next.asset = name;
            next.assetUrl = expected;
            next.digest = QByteArray::fromHex(digest.mid(7).toLatin1());
            next.size = size;
            if (target == UpdatePlatform::WindowsX64) next.windowsInstaller = winStem + ".exe";
            if (!name.endsWith(".zip")) break;
        }
        if (!next.asset.isEmpty()) {
            compatible = rank;
            selected = std::move(next);
        }
    }
    return selected.tag.isEmpty() ? linkOnly : selected;
}

QByteArray verified_update_payload(const ReleaseUpdate& release, QByteArray bytes) {
    if (bytes.size() != release.size || QCryptographicHash::hash(bytes,QCryptographicHash::Sha256) != release.digest)
        throw std::runtime_error("The downloaded file does not match the release's SHA-256 checksum.");
    if (release.asset.endsWith(".zip"))
        bytes = zip_entry(bytes,release.windowsInstaller.toUtf8(),quint32(maximumDownload));
    if (release.windowsInstaller.isEmpty() ? !bytes.startsWith("!<arch>\n") : !bytes.startsWith("MZ"))
        throw std::runtime_error("The download does not contain the expected installer.");
    return bytes;
}

namespace {
class UpdateDialog final : public QDialog {
public:
    UpdateDialog(QWidget* parent, std::function<void(const QString&)> installWindows)
        : QDialog(parent), installWindows_(std::move(installWindows)) {
        setWindowTitle("Kleiotide updates");
        resize(460,180);
        auto* layout = new QVBoxLayout(this);
        status_ = new QLabel("Checking GitHub for a newer release…",this);
        status_->setWordWrap(true);
        status_->setTextFormat(Qt::PlainText);
        layout->addWidget(status_);
        progress_ = new QProgressBar(this);
        progress_->setRange(0,0);
        layout->addWidget(progress_);
        layout->addStretch();
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close,this);
        download_ = buttons->addButton("Download update",QDialogButtonBox::ActionRole);
        install_ = buttons->addButton("Install update…",QDialogButtonBox::ActionRole);
        page_ = buttons->addButton("Open GitHub release",QDialogButtonBox::ActionRole);
        for (auto* button : {download_,install_,page_}) button->hide();
        layout->addWidget(buttons);
        connect(buttons,&QDialogButtonBox::rejected,this,&UpdateDialog::reject);
        connect(page_,&QPushButton::clicked,this,[this]{QDesktopServices::openUrl(update_.releaseUrl);});
        connect(download_,&QPushButton::clicked,this,[this]{
            download_->hide();
            progress_->show();
            progress_->setRange(0,100);
            progress_->setValue(0);
            status_->setText("Downloading " + update_.tag + " from GitHub…");
            fetch(update_.assetUrl,update_.size,true);
        });
        connect(install_,&QPushButton::clicked,this,[this]{install();});
        fetch(releaseFeed,2*1024*1024,false);
    }

    void reject() override {
        if (!installing_) QDialog::reject();
    }

private:
    void fetch(const QUrl& url, qint64 limit, bool asset, int redirects = 0) {
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
        request.setHeader(QNetworkRequest::UserAgentHeader,"Kleiotide/" + QString::fromLatin1(version));
        request.setTransferTimeout(30000);
        if (!asset) request.setRawHeader("Accept","application/vnd.github+json");
        auto* reply = network_.get(request);
        auto bytes = std::make_shared<QByteArray>();
        const auto read = [reply,bytes,limit]{
            *bytes += reply->readAll();
            if (bytes->size() > limit) reply->abort();
        };
        connect(reply,&QNetworkReply::readyRead,this,read);
        if (asset) connect(reply,&QNetworkReply::downloadProgress,this,[this,limit](qint64 received,qint64){
            progress_->setValue(int(qMin(qint64(100),received*100/limit)));
        });
        connect(reply,&QNetworkReply::finished,this,[this,reply,bytes,limit,asset,redirects,read]{
            read();
            const auto redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            const auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const auto failed = reply->error() != QNetworkReply::NoError;
            const auto error = reply->errorString();
            const auto target = reply->url().resolved(redirect);
            reply->deleteLater();
            if (bytes->size() > limit) {
                report("GitHub returned a larger response than expected.");
            } else if (!redirect.isEmpty()) {
                if (!asset || redirects >= 5 || !trusted_download(target,update_.assetUrl))
                    report("GitHub redirected the request to an unexpected address.");
                else fetch(target,limit,true,redirects+1);
            } else if (failed || code != 200) {
                report("Could not reach the GitHub release: " + error);
            } else if (asset) {
                stage_download(std::move(*bytes));
            } else {
                show_release(*bytes);
            }
        });
    }

    void report(const QString& text) {
        progress_->hide();
        status_->setText(text.left(450));
    }

    void show_release(const QByteArray& bytes) {
        try {
            update_ = find_release_update(bytes,QVersionNumber::fromString(QString::fromLatin1(version)),platform());
            if (update_.tag.isEmpty()) {
                report("You have the newest Kleiotide version listed on GitHub.");
                return;
            }
            page_->show();
            const auto name = update_.tag + (update_.preview ? " (public preview)" : "");
            if (update_.asset.isEmpty()) {
                report("Kleiotide " + name + " is available. Open its release for downloads and build instructions for your system.");
                return;
            }
            report("Kleiotide " + name + " is available (" + QString::number(double(update_.size)/(1024*1024),'f',1) + " MiB).");
            download_->show();
        } catch (const std::exception& e) {
            report(QString::fromUtf8(e.what()));
        }
    }

    void stage_download(QByteArray bytes) {
        progress_->setRange(0,0);
        status_->setText("Verifying the downloaded update…");
        auto* watcher = new QFutureWatcher<QString>(this);
        // The task owns its temporary directory even if the dialog closes during verification.
        auto staging = std::make_shared<QTemporaryDir>();
        const auto name = update_.windowsInstaller.isEmpty() ? update_.asset : update_.windowsInstaller;
        const auto path = staging->filePath(name);
        connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,staging,path]{
            const auto error = watcher->result();
            watcher->deleteLater();
            if (!error.isEmpty()) {
                report(error);
                download_->show();
                return;
            }
            staging_ = staging;
            installer_ = path;
            report("Update verified and ready to install.");
            install_->show();
        });
        watcher->setFuture(QtConcurrent::run([update=update_,bytes=std::move(bytes),staging,path]() mutable -> QString {
            try {
                if (!staging->isValid()) return "Could not create a folder for the update.";
                const auto payload = verified_update_payload(update,std::move(bytes));
                QSaveFile file(path);
                if (!file.open(QIODevice::WriteOnly) || file.write(payload) != payload.size() || !file.commit())
                    return "Could not save the verified installer.";
            } catch (const std::exception& e) {
                return QString::fromUtf8(e.what());
            }
            return {};
        }));
    }

    void install() {
        if (!update_.windowsInstaller.isEmpty()) {
            staging_->setAutoRemove(false); // Setup runs after this dialog and the app close.
            accept();
            installWindows_(installer_);
            return;
        }
        if (!QFileInfo("/usr/bin/pkexec").isExecutable() || !QFileInfo("/usr/bin/apt-get").isExecutable()) {
            staging_->setAutoRemove(false);
            report("Automatic installation requires PolicyKit and APT. The verified Debian package is saved at " + installer_);
            return;
        }
        if (QMessageBox::question(this,"Install Kleiotide update",
            "Install the verified package with APT? Your system may ask for administrator approval.",
            QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel) != QMessageBox::Yes) return;
        installing_ = true;
        install_->setEnabled(false);
        auto* process = new QProcess(this);
        process->setProcessChannelMode(QProcess::MergedChannels);
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert("LC_ALL","C");
        process->setProcessEnvironment(environment);
        auto output = std::make_shared<QByteArray>();
        const auto consume = [process,output]{*output = (*output + process->readAll()).right(8192);};
        connect(process,&QProcess::readyRead,this,consume);
        connect(process,&QProcess::errorOccurred,this,[this,process](auto error){
            if (error != QProcess::FailedToStart) return;
            report("Could not start the installer. " + process->errorString());
            installing_ = false;
            install_->setEnabled(true);
            process->deleteLater();
        });
        connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,
            [this,process,consume,output](int code,QProcess::ExitStatus result){
                consume();
                installing_ = false;
                const bool ok = code == 0 && result == QProcess::NormalExit;
                report(ok ? QString("Update installed. Close and reopen Kleiotide to use the new version.") :
                    QString("APT did not install the update. ") + QString::fromUtf8(output->right(350)).simplified());
                install_->setEnabled(!ok);
                process->deleteLater();
            });
        status_->setText("Installing the update with APT…");
        progress_->setRange(0,0);
        progress_->show();
        process->start("/usr/bin/pkexec",{"/usr/bin/apt-get","--yes","--no-remove","install",installer_});
    }

    std::function<void(const QString&)> installWindows_;
    ReleaseUpdate update_;
    QNetworkAccessManager network_;
    QLabel* status_;
    QProgressBar* progress_;
    QPushButton *download_, *install_, *page_;
    std::shared_ptr<QTemporaryDir> staging_;
    QString installer_;
    bool installing_ = false;
};
}
void check_for_updates(QWidget* parent, const std::function<void(const QString&)>& installWindows) {
    UpdateDialog dialog(parent,installWindows);
    dialog.exec();
}

bool launch_windows_update(const QString& installer) {
#ifdef Q_OS_WIN
    const auto path = installer.toStdWString();
    const auto directory = QFileInfo(installer).absolutePath().toStdWString();
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr,L"runas",path.c_str(),nullptr,directory.c_str(),SW_SHOWNORMAL)) > 32;
#else
    Q_UNUSED(installer);
    return false;
#endif
}
}
