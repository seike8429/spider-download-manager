#pragma once

#include <QNetworkReply>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QWidget;

// Checks GitHub Releases for a newer Spider Download Manager and, when the
// user accepts, downloads the Setup.exe and runs it. The app exits first so
// the installer can replace the running files.
class Updater : public QObject {
    Q_OBJECT
public:
    explicit Updater(QWidget* parentWindow, QObject* parent = nullptr);

    // silent=true: only speak up when an update is actually available
    // (used for the automatic check at startup). silent=false always
    // reports the result (used by Help > Check for updates).
    void check(bool silent);

private:
    void onCheckFinished(QNetworkReply* reply);
    void onDownloadFinished(QNetworkReply* reply);
    void offerUpdate(const QString& version, const QString& downloadUrl);
    void downloadAndInstall(const QString& downloadUrl);

    QWidget* window_;
    QNetworkAccessManager* manager_;
    bool silent_ = true;
    QString pendingDownloadUrl_;
};
