/**
 * @file FavoriteNodeRepository.cpp
 * @brief 收藏节点仓库实现
 */
#include "FavoriteNodeRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include "core/util/LogHelper.h"

FavoriteNodeRepository::FavoriteNodeRepository(QSqlDatabase db, QObject *parent)
    : QObject(parent), m_db(db)
{
}

/**
 * @brief 加载所有收藏节点
 * @return 按创建时间降序排列的节点列表
 *
 * 查询所有列（id, name, uri, public_key, created_at），
 * 结果按 created_at 降序排列，最新收藏的节点排在前面。
 * 如果查询失败（数据库错误等），返回空列表并输出警告。
 */
QList<FavoriteNode> FavoriteNodeRepository::loadAll() const
{
    QList<FavoriteNode> result;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id, name, uri, public_key, created_at FROM favorite_nodes ORDER BY created_at DESC"
    ));
    if (!query.exec()) {
        LogHelper::logWarning(QStringLiteral("loadAll 失败: %1").arg(query.lastError().text()), "FavoriteNode");
        return result;
    }
    while (query.next()) {
        FavoriteNode node;
        node.id = query.value(0).toLongLong();
        node.name = query.value(1).toString();
        node.uri = query.value(2).toString();
        node.publicKey = query.value(3).toString();
        node.createdAt = query.value(4).toLongLong();
        result.append(node);
    }
    return result;
}

/**
 * @brief 添加新的收藏节点
 * @return 成功时返回构造好的 FavoriteNode 对象（含数据库 ID 和时间戳）
 *
 * 执行流程：
 * 1. 验证 name 和 uri 非空（trim 后判断，避免纯空白字符串）
 * 2. 检查 URI 是否已被其他节点使用（UNIQUE 约束的预检查）
 * 3. 执行 INSERT 并获取自增 ID
 * 4. 构造 FavoriteNode 对象返回
 *
 * 竞态条件说明：
 * 步骤 2 和步骤 3 之间未使用事务，理论上存在 TOCTOU（检查时间/使用时间）竞态。
 * 但由于 SQLite 在同一连接内是串行的，且 uri 列有 UNIQUE 约束作为最后防线，
 * 即使在多线程环境下也不会产生数据不一致——只是最坏情况下可能返回"插入失败"
 * 而非专门的"URI 已存在"错误信息。
 */
std::optional<FavoriteNode> FavoriteNodeRepository::add(const QString &name, const QString &uri, const QString &publicKey)
{
    // 参数验证：name 和 uri 不能为空或纯空白
    if (name.trimmed().isEmpty() || uri.trimmed().isEmpty()) {
        LogHelper::logWarning("add: name 或 uri 为空", "FavoriteNode");
        return std::nullopt;
    }

    // URI 唯一性预检查（非原子，但有 UNIQUE 约束兜底）
    if (existsByUri(uri.trimmed())) {
        LogHelper::logWarning(QStringLiteral("add: uri 已存在: %1").arg(uri), "FavoriteNode");
        return std::nullopt;
    }

    const qint64 now = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO favorite_nodes (name, uri, public_key, created_at) VALUES (?, ?, ?, ?)"
    ));
    // 使用 trimmed 值去除首尾空格，保证数据整洁
    query.addBindValue(name.trimmed());
    query.addBindValue(uri.trimmed());
    query.addBindValue(publicKey.trimmed());
    query.addBindValue(now);

    if (!query.exec()) {
        LogHelper::logWarning(QStringLiteral("add 失败: %1").arg(query.lastError().text()), "FavoriteNode");
        return std::nullopt;
    }

    FavoriteNode node;
    node.id = query.lastInsertId().toLongLong();  // 获取刚插入行的自增 ID
    node.name = name.trimmed();
    node.uri = uri.trimmed();
    node.publicKey = publicKey.trimmed();
    node.createdAt = now;
    return node;
}

/**
 * @brief 更新收藏节点信息
 * @return true 表示更新成功且有行受影响
 *
 * 执行流程：
 * 1. 验证参数有效性（name/uri 非空，id 非负）
 * 2. 检查新 URI 是否与其他节点冲突（排除当前节点自身）
 * 3. 执行 UPDATE 并检查受影响行数
 *
 * 返回值 numRowsAffected() > 0 确保只有确实更新了行才返回 true，
 * 避免对不存在的 id 静默返回成功。
 */
bool FavoriteNodeRepository::update(qint64 id, const QString &name, const QString &uri, const QString &publicKey)
{
    // 参数验证
    if (name.trimmed().isEmpty() || uri.trimmed().isEmpty() || id < 0) {
        LogHelper::logWarning("update: 参数无效", "FavoriteNode");
        return false;
    }

    // URI 唯一性检查（排除自身 id，允许将 URI 改为相同值）
    if (existsByUri(uri.trimmed(), id)) {
        LogHelper::logWarning(QStringLiteral("update: uri 已存在: %1").arg(uri), "FavoriteNode");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE favorite_nodes SET name = ?, uri = ?, public_key = ? WHERE id = ?"
    ));
    query.addBindValue(name.trimmed());
    query.addBindValue(uri.trimmed());
    query.addBindValue(publicKey.trimmed());
    query.addBindValue(id);

    if (!query.exec()) {
        LogHelper::logWarning(QStringLiteral("update 失败: %1").arg(query.lastError().text()), "FavoriteNode");
        return false;
    }
    // 确保确实有行被更新（id 不存在时 numRowsAffected() 为 0）
    return query.numRowsAffected() > 0;
}

/**
 * @brief 删除指定节点
 * @return true 表示 SQL 执行成功（即使没有匹配的 id 也返回 true）
 *
 * 设计决策：删除不存在的 id 不算错误（幂等删除），
 * 因此不检查 numRowsAffected()。
 */
bool FavoriteNodeRepository::remove(qint64 id)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM favorite_nodes WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        LogHelper::logWarning(QStringLiteral("remove 失败: %1").arg(query.lastError().text()), "FavoriteNode");
        return false;
    }
    return true;
}

/**
 * @brief 清空所有收藏节点
 * @return true 表示清空成功
 *
 * 使用 DELETE FROM favorite_nodes 无 WHERE 子句，删除全部行。
 * 这是不可逆操作，应由调用方在 UI 层做二次确认。
 */
bool FavoriteNodeRepository::clear()
{
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("DELETE FROM favorite_nodes"))) {
        LogHelper::logWarning(QStringLiteral("clear 失败: %1").arg(query.lastError().text()), "FavoriteNode");
        return false;
    }
    return true;
}

/**
 * @brief 检查指定 URI 是否已被收藏
 * @param uri 要检查的节点 URI
 * @param excludeId 排除的节点 ID，-1 表示不排除
 * @return true 表示 URI 已存在于其他节点中
 *
 * 两种查询模式：
 * - excludeId < 0（添加场景）：只检查 URI 是否存在
 * - excludeId >= 0（更新场景）：检查 URI 是否被除当前节点外的其他节点使用
 *
 * 注意：exec() 失败时返回 false，与"URI 不存在"无法区分。
 * 但 SQL 查询失败属于极端异常（数据库损坏等），实际影响有限。
 */
bool FavoriteNodeRepository::existsByUri(const QString &uri, qint64 excludeId) const
{
    QSqlQuery query(m_db);
    if (excludeId >= 0) {
        // 更新场景：排除指定 id 的节点
        query.prepare(QStringLiteral(
            "SELECT 1 FROM favorite_nodes WHERE uri = ? AND id != ?"
        ));
        query.addBindValue(uri);
        query.addBindValue(excludeId);
    } else {
        // 添加场景：检查所有节点
        query.prepare(QStringLiteral(
            "SELECT 1 FROM favorite_nodes WHERE uri = ?"
        ));
        query.addBindValue(uri);
    }
    if (!query.exec())
        return false;
    return query.next();
}
