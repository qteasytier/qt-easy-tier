/**
 * @file LogRepository.cpp
 * @brief 应用日志仓库实现
 *
 * 实现日志的插入、查询、清空和裁剪功能。
 * 日志按时间戳降序排列，支持限制最大保留条数以控制存储空间。
 */
#include "LogRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

/**
 * @brief 构造函数：保存数据库连接引用
 * @param db 已打开的 SQLite 数据库连接
 * @param parent Qt 父对象
 */
LogRepository::LogRepository(QSqlDatabase db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

/**
 * @brief 插入日志并裁剪超出的旧记录
 * @param level 日志级别
 * @param message 日志消息
 * @param context 日志上下文
 * @param maxEntries 保留的最大条目数
 * @return true 表示插入成功
 *
 * 流程：
 * 1. 将日志写入 app_logs 表（时间戳为当前 UTC 毫秒数）
 * 2. 调用 trimToMax() 删除超出 maxEntries 的旧记录
 * 3. 发出 logInserted() 信号通知监听者
 */
bool LogRepository::insertLog(LogLevel level, const QString &message, const QString &context, int maxEntries)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO app_logs (timestamp, level, context, message) VALUES (?, ?, ?, ?)"
    ));
    query.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    query.addBindValue(static_cast<int>(level));
    query.addBindValue(context);
    query.addBindValue(message);

    if (!query.exec()) {
        qWarning().noquote() << QStringLiteral("insertLog 失败: %1").arg(query.lastError().text());
        return false;
    }

    // 插入后裁剪旧记录，保持日志条目不超出上限
    trimToMax(maxEntries);
    emit logInserted();
    return true;
}

/**
 * @brief 清空所有日志
 * @return true 表示清空成功
 *
 * 使用 DELETE FROM 无 WHERE 子句删除全部行，
 * 操作成功后发出 logsCleared() 信号。
 */
bool LogRepository::clearAll()
{
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("DELETE FROM app_logs"))) {
        qWarning().noquote() << QStringLiteral("clearAll 失败: %1").arg(query.lastError().text());
        return false;
    }
    emit logsCleared();
    return true;
}

/**
 * @brief 获取日志总条数
 * @return 日志记录数量
 *
 * 使用 SELECT COUNT(*) 统计。查询失败时返回 0。
 */
int LogRepository::count() const
{
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM app_logs")))
        return 0;
    if (query.next())
        return query.value(0).toInt();
    return 0;
}

/**
 * @brief 裁剪日志，仅保留最新的 maxEntries 条
 * @param maxEntries 最大保留条目数
 * @return true 表示裁剪执行成功
 *
 * 裁剪策略：
 * - 使用子查询按 (timestamp DESC, id DESC) 排序选出最新的 N 条
 * - 删除 id 不在该子查询结果中的旧记录
 * - id 降序作为第二排序键，确保相同时间戳时优先保留插入较晚的记录
 */
bool LogRepository::trimToMax(int maxEntries)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "DELETE FROM app_logs WHERE id NOT IN ("
        "  SELECT id FROM app_logs ORDER BY timestamp DESC, id DESC LIMIT ?"
        ")"
    ));
    query.addBindValue(maxEntries);
    if (!query.exec()) {
        qWarning().noquote() << QStringLiteral("trimToMax 失败: %1").arg(query.lastError().text());
        return false;
    }
    return true;
}

/**
 * @brief 加载最近的日志记录
 * @param limit 最大返回条数
 * @return 按时间戳降序排列的日志列表
 *
 * 按 (timestamp DESC, id DESC) 排序，最新日志排在最前。
 * 查询失败时返回空列表。
 */
QList<LogEntry> LogRepository::loadRecent(int limit) const
{
    QList<LogEntry> result;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id, timestamp, level, context, message FROM app_logs "
        "ORDER BY timestamp DESC, id DESC LIMIT ?"
    ));
    query.addBindValue(limit);

    if (!query.exec()) {
        qWarning().noquote() << QStringLiteral("loadRecent 失败: %1").arg(query.lastError().text());
        return result;
    }

    // 逐行读取并构造 LogEntry 对象
    while (query.next()) {
        LogEntry entry;
        entry.id = query.value(0).toLongLong();
        entry.timestamp = query.value(1).toLongLong();
        entry.level = static_cast<LogLevel>(query.value(2).toInt());
        entry.context = query.value(3).toString();
        entry.message = query.value(4).toString();
        result.append(entry);
    }
    return result;
}
