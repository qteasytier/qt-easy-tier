/** @file UpdateCheckService.h @brief 通过 Gitee API 检查 QtEasyTier 新版本并弹出更新提示 */
#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class UpdateCheckService : public QObject {
    Q_OBJECT

public:
    struct UpdateInfo {
        bool available = false;
        QString latestVersion;
        QString title;
        QString body;
        QString downloadUrl;
        QString releaseUrl;
        QString errorMessage;
    };

    explicit UpdateCheckService(QObject *parent = nullptr);

    void checkLatestRelease(const QString &currentVersion, bool notifyWhenUpToDate = false);

signals:
    void updateAvailable(const UpdateInfo &info);
    void noUpdateAvailable(const QString &message);
    void updateCheckFailed(const QString &message);
    void checkFinished();

private:
    void handleReply(QNetworkReply *reply, const QString &currentVersion, bool notifyWhenUpToDate);
    void showUpdateDialog(const UpdateInfo &info);

    QNetworkAccessManager *m_networkAccessManager = nullptr;
};
