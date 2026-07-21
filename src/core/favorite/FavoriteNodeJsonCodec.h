/**
 * @file FavoriteNodeJsonCodec.h
 * @brief 收藏节点 JSON 编解码工具
 *
 * 统一解析和生成收藏节点 JSON 格式。内置公共服务器列表与用户收藏节点批量
 * 导入导出共用同一格式，字段为 uri / display_name / publicKey。
 */
#pragma once

#include "core/favorite/FavoriteNode.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QUrl>

/** @brief 收藏节点 JSON 解析结果 */
struct FavoriteNodeJsonParseResult {
    QList<FavoriteNode> nodes; ///< 成功解析出的节点列表
    QString error;             ///< 整体解析错误，非空表示输入不可用
    int skippedCount = 0;      ///< 因条目无效被跳过的数量
};

/** @brief 收藏节点 JSON 编解码工具 */
class FavoriteNodeJsonCodec {
public:
    /** @brief 缺少 display_name 时使用的默认显示名称 */
    static QString defaultDisplayName();

    /** @brief 从 JSON 字节解析收藏节点数组 */
    static FavoriteNodeJsonParseResult parseNodes(const QByteArray &json);
    /** @brief 将收藏节点列表序列化为 JSON 字节 */
    static QByteArray toJson(const QList<FavoriteNode> &nodes);
    /** @brief 从本地文件或 qrc 资源 URL 加载收藏节点数组 */
    static FavoriteNodeJsonParseResult loadNodes(const QUrl &fileUrl);
    /** @brief 将收藏节点数组保存到本地文件 URL */
    static bool saveNodes(const QUrl &fileUrl, const QList<FavoriteNode> &nodes, QString *error = nullptr);

private:
    static QString pathFromUrl(const QUrl &fileUrl);
};
