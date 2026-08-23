/** @file UpdateCheckService.cpp @brief 腾讯 CNB 最新版本检查实现 */
#include "UpdateCheckService.h"

#include "core/util/LogHelper.h"

#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QStringList>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

namespace {

// cnb.cool 官方 OpenAPI（api.cnb.cool）的所有发布接口均需 Bearer token 鉴权，
// 这里携带只读令牌请求最新正式发布。
constexpr char kApiLatestUrl[] = "https://api.cnb.cool/myqfeng/qteasytier/qt-easy-tier/-/releases/latest";
constexpr char kApiToken[] = "3VPr3215CKeEeELNJyLSXjjPKRN";
constexpr char kReleasePageBaseUrl[] = "https://cnb.cool/myqfeng/qteasytier/qt-easy-tier/-/releases/tag/";

// 将版本号拆分为数字段列表，便于逐段比较；忽略前缀字母与预发布后缀。
QList<int> versionParts(const QString &raw)
{
    QString version = raw.trimmed();
    int i = 0;
    while (i < version.size() && !version.at(i).isDigit())
        ++i;
    version = version.mid(i);
    const int dash = version.indexOf(QLatin1Char('-'));
    if (dash >= 0)
        version = version.left(dash);

    QList<int> parts;
    const QStringList segments = version.split(QLatin1Char('.'));
    for (const QString &segment : segments) {
        bool ok = false;
        const int number = segment.toInt(&ok);
        parts.append(ok ? number : 0);
    }
    return parts;
}

// 语义化版本比较：返回 1 表示 a > b，-1 表示 a < b，0 表示相等。
int compareVersions(const QString &a, const QString &b)
{
    const QList<int> partsA = versionParts(a);
    const QList<int> partsB = versionParts(b);
    const int count = qMax(partsA.size(), partsB.size());
    for (int i = 0; i < count; ++i) {
        const int x = i < partsA.size() ? partsA.at(i) : 0;
        const int y = i < partsB.size() ? partsB.at(i) : 0;
        if (x != y)
            return x < y ? -1 : 1;
    }
    return 0;
}

} // namespace

UpdateCheckService::UpdateCheckService(QObject *parent)
    : QObject(parent)
    , m_networkAccessManager(new QNetworkAccessManager(this))
{
}

void UpdateCheckService::checkLatestRelease(const QString &currentVersion, bool notifyWhenUpToDate)
{
    // 请求官方 OpenAPI 的最新发布接口（需 Bearer token）
    QNetworkRequest request(QUrl(QString::fromLatin1(kApiLatestUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("QtEasyTier/%1").arg(currentVersion));
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("application/vnd.cnb.api+json"));
    request.setRawHeader(QByteArrayLiteral("Authorization"),
                         QByteArrayLiteral("Bearer ") + QByteArray(kApiToken));

    QNetworkReply *reply = m_networkAccessManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, currentVersion, notifyWhenUpToDate]() {
        handleApiReply(reply, currentVersion, notifyWhenUpToDate);
    });
}

void UpdateCheckService::handleApiReply(QNetworkReply *reply, const QString &currentVersion, bool notifyWhenUpToDate)
{
    const QString errorString = reply->errorString();
    const QByteArray payload = (reply->error() == QNetworkReply::NoError) ? reply->readAll() : QByteArray();
    reply->deleteLater();

    if (payload.isEmpty()) {
        failCheck(QStringLiteral("检查更新失败：%1").arg(errorString), notifyWhenUpToDate);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        failCheck(QStringLiteral("检查更新失败：无法解析服务器返回的数据"), notifyWhenUpToDate);
        return;
    }

    const QJsonObject obj = doc.object();
    const QString latestVersion = obj.value(QStringLiteral("tag_name")).toString().trimmed();
    if (latestVersion.isEmpty()) {
        failCheck(QStringLiteral("检查更新失败：服务器返回了空版本号"), notifyWhenUpToDate);
        return;
    }

    UpdateInfo info;
    info.latestVersion = latestVersion;
    info.title = obj.value(QStringLiteral("name")).toString();
    info.body = obj.value(QStringLiteral("body")).toString();
    finishCheck(info, currentVersion, notifyWhenUpToDate);
}

void UpdateCheckService::finishCheck(const UpdateInfo &info, const QString &currentVersion, bool notifyWhenUpToDate)
{
    UpdateInfo result = info;
    result.releaseUrl = QStringLiteral("%1%2").arg(QString::fromLatin1(kReleasePageBaseUrl), result.latestVersion);
    result.downloadUrl = result.releaseUrl;
    result.available = compareVersions(result.latestVersion, currentVersion) > 0;

    if (result.available) {
        LogHelper::logInfo(QStringLiteral("发现新版本：%1 -> %2").arg(currentVersion, result.latestVersion), "UpdateCheck");
        emit updateAvailable(result);
        showUpdateDialog(result);
    } else if (notifyWhenUpToDate) {
        const QString message = QStringLiteral("当前已是最新版本：%1").arg(currentVersion);
        emit noUpdateAvailable(message);
        QMessageBox::information(nullptr, QStringLiteral("检查更新"), message);
    }

    emit checkFinished();
}

void UpdateCheckService::failCheck(const QString &message, bool notifyWhenUpToDate)
{
    LogHelper::logWarning(message, "UpdateCheck");
    emit updateCheckFailed(message);
    if (notifyWhenUpToDate)
        QMessageBox::warning(nullptr, QStringLiteral("检查更新"), message);
    emit checkFinished();
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
