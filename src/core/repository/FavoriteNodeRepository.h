/**
 * @file FavoriteNodeRepository.h
 * @brief 收藏节点持久化仓库
 *
 * 封装 favorite_nodes 表的 CRUD 操作。
 * 收藏节点是用户标记为常用的 EasyTier 节点，存储其连接信息。
 */
#pragma once
#include <QObject>
#include <QList>
#include <QSqlDatabase>
#include <optional>
#include "core/favorite/FavoriteNode.h"

/**
 * @class FavoriteNodeRepository
 * @brief 收藏节点的持久化仓库
 *
 * 提供节点收藏的增删改查操作。uri 列设有 UNIQUE 约束，
 * 保证同一节点不会被重复收藏。
 *
 * 注意：所有公开方法均使用参数绑定，无 SQL 注入风险。
 */
class FavoriteNodeRepository : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造仓库实例
     * @param db 已打开的 SQLite 数据库连接
     * @param parent Qt 父对象，用于生命周期管理
     */
    explicit FavoriteNodeRepository(QSqlDatabase db, QObject *parent = nullptr);

    /**
     * @brief 加载所有收藏节点
     * @return 收藏节点列表，按创建时间降序排列（最新的在前）
     */
    QList<FavoriteNode> loadAll() const;

    /**
     * @brief 添加新的收藏节点
     * @param name 节点显示名称
     * @param uri 节点连接 URI（须唯一）
     * @param publicKey 公钥（可选，用于加密连接）
     * @return 成功时返回填充了 id 和 createdAt 的 FavoriteNode 对象；
     *         失败时（空参数、URI 重复、数据库错误）返回 std::nullopt
     */
    std::optional<FavoriteNode> add(const QString &name, const QString &uri, const QString &publicKey = {});

    /**
     * @brief 更新收藏节点信息
     * @param id 节点数据库 ID
     * @param name 新的显示名称
     * @param uri 新的连接 URI
     * @param publicKey 新的公钥
     * @return true 表示更新成功（影响行数 > 0）；false 表示参数无效、
     *         URI 冲突或数据库错误
     */
    bool update(qint64 id, const QString &name, const QString &uri, const QString &publicKey);

    /**
     * @brief 删除指定节点
     * @param id 要删除的节点数据库 ID
     * @return true 表示 SQL 执行成功（即使没有匹配的行也返回 true）
     */
    bool remove(qint64 id);

    /**
     * @brief 清空所有收藏节点
     * @return true 表示清空成功
     */
    bool clear();

    /**
     * @brief 检查指定 URI 是否已被收藏
     * @param uri 要检查的节点 URI
     * @param excludeId 排除的节点 ID（-1 表示不排除任何节点）
     * @return true 表示 URI 已存在
     *
     * excludeId 用于更新场景：检查新 URI 是否与其他节点冲突时，
     * 排除当前正在编辑的节点自身。
     */
    bool existsByUri(const QString &uri, qint64 excludeId = -1) const;

private:
    QSqlDatabase m_db; ///< 外部传入的数据库连接句柄（值拷贝，隐式共享）
};
