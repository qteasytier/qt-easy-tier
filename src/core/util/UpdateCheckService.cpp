/** @file UpdateCheckService.cpp @brief Gitee 最新版本检查实现 */
#include "UpdateCheckService.h"

#include "core/util/LogHelper.h"

#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr char kReleaseApiUrl[] = "https://gitee.com/api/v5/repos/qteasytier/qt-easy-tier/releases/latest";
constexpr char kReleasePageBaseUrl[] = "https://gitee.com/qteasytier/qt-easy-tier/releases/tag/";
}

UpdateCheckService::UpdateCheckService(QObject *parent)
    : QObject(parent)
    , m_networkAccessManager(new QNetworkAccessManager(this))
{
}

void UpdateCheckService::checkLatestRelease(const QString &currentVersion, bool notifyWhenUpToDate)
{
    QNetworkRequest request(QUrl(QString::fromLatin1(kReleaseApiUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("QtEasyTier/%1").arg(currentVersion));

    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, currentVersion, notifyWhenUpToDate]() {
        handleReply(reply, currentVersion, notifyWhenUpToDate);
    });
}

void UpdateCheckService::handleReply(QNetworkReply *reply, const QString &currentVersion, bool notifyWhenUpToDate)
{
    const auto cleanupReply = [reply]() { reply->deleteLater(); };

    if (reply->error() != QNetworkReply::NoError) {
        const QString message = QStringLiteral("检查更新失败：%1").arg(reply->errorString());
        LogHelper::logWarning(message, "UpdateCheck");
        emit updateCheckFailed(message);
        if (notifyWhenUpToDate)
            QMessageBox::warning(nullptr, QStringLiteral("检查更新"), message);
        emit checkFinished();
        cleanupReply();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString message = QStringLiteral("检查更新失败：无法解析服务器返回的数据");
        LogHelper::logWarning(message, "UpdateCheck");
        emit updateCheckFailed(message);
        if (notifyWhenUpToDate)
            QMessageBox::warning(nullptr, QStringLiteral("检查更新"), message);
        emit checkFinished();
        cleanupReply();
        return;
    }

    const QJsonObject obj = doc.object();
    const QString latestVersion = obj.value(QStringLiteral("tag_name")).toString().trimmed();
    if (latestVersion.isEmpty()) {
        const QString message = QStringLiteral("检查更新失败：服务器返回了空版本号");
        LogHelper::logWarning(message, "UpdateCheck");
        emit updateCheckFailed(message);
        if (notifyWhenUpToDate)
            QMessageBox::warning(nullptr, QStringLiteral("检查更新"), message);
        emit checkFinished();
        cleanupReply();
        return;
    }

    UpdateInfo info;
    info.latestVersion = latestVersion;
    info.title = obj.value(QStringLiteral("name")).toString();
    info.body = obj.value(QStringLiteral("body")).toString();
    info.releaseUrl = QStringLiteral("%1%2").arg(QString::fromLatin1(kReleasePageBaseUrl), latestVersion);
    info.downloadUrl = info.releaseUrl;
    info.available = latestVersion > currentVersion;

    if (info.available) {
        LogHelper::logInfo(QStringLiteral("发现新版本：%1 -> %2").arg(currentVersion, latestVersion), "UpdateCheck");
        emit updateAvailable(info);
        showUpdateDialog(info);
    } else if (notifyWhenUpToDate) {
        const QString message = QStringLiteral("当前已是最新版本：%1").arg(currentVersion);
        emit noUpdateAvailable(message);
        QMessageBox::information(nullptr, QStringLiteral("检查更新"), message);
    }

    emit checkFinished();
    cleanupReply();
}

void UpdateCheckService::showUpdateDialog(const UpdateInfo &info)
{
    QDialog dialog;
    dialog.setWindowTitle(QStringLiteral("发现新版本"));
    dialog.resize(560, 420);

    auto *layout = new QVBoxLayout(&dialog);
    auto *titleLabel = new QLabel(QStringLiteral("发现 QtEasyTier 新版本：%1").arg(info.latestVersion), &dialog);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    if (!info.title.isEmpty()) {
        auto *releaseNameLabel = new QLabel(info.title, &dialog);
        releaseNameLabel->setWordWrap(true);
        layout->addWidget(releaseNameLabel);
    }

    auto *bodyBrowser = new QTextBrowser(&dialog);
    bodyBrowser->setOpenExternalLinks(true);
    bodyBrowser->setMarkdown(info.body.isEmpty() ? QStringLiteral("暂无更新日志。") : info.body);
    layout->addWidget(bodyBrowser, 1);

    auto *buttonBox = new QDialogButtonBox(&dialog);
    QPushButton *downloadButton = buttonBox->addButton(QStringLiteral("前往下载"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(QStringLiteral("稍后"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(downloadButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted)
        QDesktopServices::openUrl(QUrl(info.downloadUrl));
}
