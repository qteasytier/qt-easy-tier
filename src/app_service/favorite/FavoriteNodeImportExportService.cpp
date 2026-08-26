/**
 * @file FavoriteNodeImportExportService.cpp
 * @brief FavoriteNodeImportExportService 实现
 */
#include "FavoriteNodeImportExportService.h"

#include "core/repository/FavoriteNodeRepository.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QVariant>

namespace {
constexpr char kImportTimedOutProperty[] = "qtetImportTimedOut";
}

FavoriteNodeImportExportService::FavoriteNodeImportExportService(FavoriteNodeRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_networkAccessManager(new QNetworkAccessManager(this))
{
}

void FavoriteNodeImportExportService::setImportUrlTimeoutMs(int timeoutMs)
{
    if (timeoutMs > 0)
        m_importUrlTimeoutMs = timeoutMs;
}

bool FavoriteNodeImportExportService::importFromFile(const QUrl &fileUrl)
{
    return importParsedNodes(FavoriteNodeJsonCodec::loadNodes(fileUrl));
}

void FavoriteNodeImportExportService::importFromUrl(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    if (!url.isValid() || (scheme != QStringLiteral("http") && scheme != QStringLiteral("https"))) {
        emit operationFailed(QStringLiteral("仅支持 http/https 节点导入地址"));
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("QtEasyTier"));
    QNetworkReply *reply = m_networkAccessManager->get(request);
    QTimer *timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply]() {
        reply->setProperty(kImportTimedOutProperty, true);
        if (reply->isRunning())
            reply->abort();
    });
    timer->start(m_importUrlTimeoutMs);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleImportUrlReply(reply);
    });
}

bool FavoriteNodeImportExportService::exportToFile(const QUrl &fileUrl, const QList<FavoriteNode> &nodes)
{
    QString error;
    if (!FavoriteNodeJsonCodec::saveNodes(fileUrl, nodes, &error)) {
        emit operationFailed(error);
        return false;
    }

    emit exportCompleted();
    return true;
}

bool FavoriteNodeImportExportService::importParsedNodes(const FavoriteNodeJsonParseResult &parseResult)
{
    if (!m_repository) {
        emit operationFailed(QStringLiteral("收藏节点仓库不可用"));
        return false;
    }
    if (!parseResult.error.isEmpty()) {
        emit operationFailed(parseResult.error);
        return false;
    }

    int importedCount = 0;
    int skippedCount = parseResult.skippedCount;
    for (const FavoriteNode &node : parseResult.nodes) {
        if (m_repository->existsByUri(node.uri)) {
            ++skippedCount;
            continue;
        }

        if (m_repository->add(node.name, node.uri, node.publicKey).has_value()) {
            ++importedCount;
        } else {
            ++skippedCount;
        }
    }

    if (importedCount == 0) {
        emit operationFailed(QStringLiteral("没有可导入的节点"));
        return false;
    }

    emit importCompleted(importedCount, skippedCount);
    return true;
}

void FavoriteNodeImportExportService::handleImportUrlReply(QNetworkReply *reply)
{
    const auto cleanupReply = [reply]() { reply->deleteLater(); };

    if (reply->property(kImportTimedOutProperty).toBool()) {
        emit operationFailed(QStringLiteral("节点导入失败：请求超时"));
        cleanupReply();
        return;
    }

    const QVariant statusValue = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const int statusCode = statusValue.isValid() ? statusValue.toInt() : 0;
    if (statusValue.isValid() && (statusCode < 200 || statusCode >= 300)) {
        emit operationFailed(QStringLiteral("节点导入失败：HTTP 状态码 %1").arg(statusCode));
        cleanupReply();
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        emit operationFailed(QStringLiteral("节点导入失败：%1").arg(reply->errorString()));
        cleanupReply();
        return;
    }

    importParsedNodes(FavoriteNodeJsonCodec::parseNodes(reply->readAll()));
    cleanupReply();
}
