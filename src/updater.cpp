#include "updater.h"

#include "common.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace {

// GitHub Releases for this repo. The installer asset is named
// SpiderDownloadManager-Setup-<version>.exe.
const char* kReleasesUrl =
    "https://api.github.com/repos/seike8429/spider-download-manager/"
    "releases/latest";

// "v1.2" or "1.2" -> {1, 2}. Returns false when the text is not a version.
bool ParseVersion(const QString& text, int* major, int* minor, int* patch) {
    QString clean = text.trimmed();
    if (clean.startsWith('v') || clean.startsWith('V')) clean.remove(0, 1);
    const QStringList parts = clean.split('.');
    if (parts.isEmpty() || parts.size() > 3) return false;
    bool ok = false;
    *major = parts.value(0).toInt(&ok);
    if (!ok) return false;
    *minor = parts.size() > 1 ? parts.value(1).toInt(&ok) : 0;
    if (!ok) return false;
    *patch = parts.size() > 2 ? parts.value(2).toInt(&ok) : 0;
    if (!ok) return false;
    return true;
}

// True when "other" is a newer version than "current".
bool IsNewer(const QString& current, const QString& other) {
    int curMajor = 0, curMinor = 0, curPatch = 0;
    int newMajor = 0, newMinor = 0, newPatch = 0;
    if (!ParseVersion(current, &curMajor, &curMinor, &curPatch)) return false;
    if (!ParseVersion(other, &newMajor, &newMinor, &newPatch)) return false;
    if (newMajor != curMajor) return newMajor > curMajor;
    if (newMinor != curMinor) return newMinor > curMinor;
    return newPatch > curPatch;
}

}  // namespace

Updater::Updater(QWidget* parentWindow, QObject* parent)
    : QObject(parent),
      window_(parentWindow),
      manager_(new QNetworkAccessManager(this)) {}

void Updater::check(bool silent) {
    silent_ = silent;
    QNetworkRequest request(QUrl(QString::fromLatin1(kReleasesUrl)));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "SpiderDownloadManager-updater");
    QNetworkReply* reply = manager_->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply] { onCheckFinished(reply); });
}

void Updater::onCheckFinished(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        if (!silent_) {
            QMessageBox::information(
                window_, QString::fromWCharArray(APP_NAME),
                tr("Could not check for updates. Please check your "
                   "internet connection and try again."));
        }
        return;
    }

    const QJsonDocument document =
        QJsonDocument::fromJson(reply->readAll());
    if (!document.isObject()) {
        if (!silent_) {
            QMessageBox::information(window_, QString::fromWCharArray(APP_NAME),
                                     tr("Could not check for updates."));
        }
        return;
    }

    const QJsonObject release = document.object();
    const QString tag = release.value(QStringLiteral("tag_name")).toString();
    if (tag.isEmpty() ||
        !IsNewer(QString::fromWCharArray(APP_VERSION), tag)) {
        if (!silent_) {
            QMessageBox::information(
                window_, QString::fromWCharArray(APP_NAME),
                tr("You are using the latest version."));
        }
        return;
    }

    // Find the installer asset for this release.
    QString downloadUrl;
    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue& value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (name.startsWith(QStringLiteral("SpiderDownloadManager-Setup-")) &&
            name.endsWith(QStringLiteral(".exe"))) {
            downloadUrl =
                asset.value(QStringLiteral("browser_download_url")).toString();
            break;
        }
    }
    if (downloadUrl.isEmpty()) {
        if (!silent_) {
            QMessageBox::information(
                window_, QString::fromWCharArray(APP_NAME),
                tr("A new version (%1) is available, but no installer "
                   "was found for it.")
                    .arg(tag));
        }
        return;
    }

    offerUpdate(tag, downloadUrl);
}

void Updater::offerUpdate(const QString& version,
                          const QString& downloadUrl) {
    const QMessageBox::StandardButton answer = QMessageBox::question(
        window_, QString::fromWCharArray(APP_NAME),
        tr("Spider Download Manager %1 is available. "
           "Download and install it now?")
            .arg(version),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer != QMessageBox::Yes) return;
    downloadAndInstall(downloadUrl);
}

void Updater::downloadAndInstall(const QString& downloadUrl) {
    pendingDownloadUrl_ = downloadUrl;
    QUrl url(downloadUrl);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "SpiderDownloadManager-updater");
    QNetworkReply* reply = manager_->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply] { onDownloadFinished(reply); });
}

void Updater::onDownloadFinished(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::information(
            window_, QString::fromWCharArray(APP_NAME),
            tr("The update could not be downloaded. Please try again "
               "later."));
        return;
    }

    const QString fileName =
        QUrl(pendingDownloadUrl_).fileName();
    const QString path = QDir::tempPath() + QLatin1Char('/') +
                         (fileName.isEmpty()
                              ? QStringLiteral("SpiderDownloadManager-Setup.exe")
                              : fileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::information(
            window_, QString::fromWCharArray(APP_NAME),
            tr("The update could not be saved. Please try again later."));
        return;
    }
    file.write(reply->readAll());
    file.close();

    // Run the installer, then quit so it can replace the running app.
    // QProcess::startDetached keeps the installer alive after we exit.
    if (!QProcess::startDetached(path, QStringList())) {
        QMessageBox::information(
            window_, QString::fromWCharArray(APP_NAME),
            tr("The update was downloaded to %1. Please run it to "
               "install.")
                .arg(path));
        return;
    }
    QCoreApplication::quit();
}
