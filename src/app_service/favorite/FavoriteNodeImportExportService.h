/**
 * @file FavoriteNodeImportExportService.h
 * @brief 收藏节点批量导入导出服务
 *
 * 负责收藏节点 JSON 文件导入导出、URL 导入、网络超时处理和批量落库。
 * ViewModel 只调用本服务并转发结果，不直接处理 IO 或网络细节。
 */
#pragma once

#include "core/favorite/FavoriteNode.h"
#include "core/favorite/FavoriteNodeJsonCodec.h"

#include <QList>
#include <QObject>
#include <QUrl>

class FavoriteNodeRepository;
class QNetworkAccessManager;
class QNetworkReply;

/** @brief 收藏节点批量导入导出服务 */
class FavoriteNodeImportExportService : public QObject {
    Q_OBJECT

public:
    explicit FavoriteNodeImportExportService(FavoriteNodeRepository *repository, QObject *parent = nullptr);

    /** @brief 设置 URL 导入超时时间，主要用于测试和特殊运行环境调整 */
    void setImportUrlTimeoutMs(int timeoutMs);

    /** @brief 从 JSON 文件批量导入收藏节点 */
    bool importFromFile(const QUrl &fileUrl);
    /** @brief 从 http/https URL 批量导入收藏节点 */
    void importFromUrl(const QUrl &url);
    /** @brief 将收藏节点批量导出为 JSON 文件 */
    bool exportToFile(const QUrl &fileUrl, const QList<FavoriteNode> &nodes);

signals:
    /** @brief 批量导入完成 */
    void importCompleted(int importedCount, int skippedCount);
    /** @brief 批量导出完成 */
    void exportCompleted();
    /** @brief 导入导出失败 */
    void operationFailed(const QString &message);

private:
    bool importParsedNodes(const FavoriteNodeJsonParseResult &parseResult);
    void handleImportUrlReply(QNetworkReply *reply);

    FavoriteNodeRepository *m_repository = nullptr; ///< 收藏节点仓库，非所有权
    QNetworkAccessManager *m_networkAccessManager = nullptr; ///< URL 导入网络访问管理器
    int m_importUrlTimeoutMs = 15000; ///< URL 导入请求超时时间，单位毫秒
};
