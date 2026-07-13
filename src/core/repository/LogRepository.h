/**
 * @file LogRepository.h
 * @brief 应用日志持久化仓库
 *
 * 封装 app_logs 表的 CRUD 操作，支持日志插入、查询、清理和数量裁剪。
 */
#pragma once
#include "core/log/LogTypes.h"

#include <QObject>
#include <QList>
#include <QSqlDatabase>

/**
 * @class LogRepository
 * @brief 应用日志的持久化仓库
 *
 * 提供日志的增删查操作。支持限制最大条目数（通过 trimToMax 裁剪旧日志），
 * 并在插入和清空时通过信号通知外部监听者。
 */
class LogRepository : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造日志仓库实例
     * @param db 已打开的 SQLite 数据库连接
     * @param parent Qt 父对象，用于生命周期管理
     */
    explicit LogRepository(QSqlDatabase db, QObject *parent = nullptr);

    /**
     * @brief 插入一条日志记录
     * @param level 日志级别
     * @param message 日志消息内容
     * @param context 日志上下文（模块名等）
     * @param maxEntries 最大保留条目数，插入后自动裁剪超出部分
     * @return true 表示插入成功
     */
    bool insertLog(LogLevel level, const QString &message, const QString &context, int maxEntries);

    /**
     * @brief 清空所有日志记录
     * @return true 表示清空成功
     */
    bool clearAll();

    /**
     * @brief 获取当前日志总条数
     * @return 日志记录数量，查询失败时返回 0
     */
    int count() const;

    /**
     * @brief 按时间戳裁剪日志，仅保留最新的 maxEntries 条
     * @param maxEntries 最大保留条目数
     * @return true 表示裁剪执行成功
     *
     * 内部使用子查询保留最新的 N 条记录，删除其余。
     */
    bool trimToMax(int maxEntries);

    /**
     * @brief 加载最近的日志记录
     * @param limit 最大返回条数
     * @return 按时间戳降序排列的日志条目列表
     */
    QList<LogEntry> loadRecent(int limit) const;

signals:
    /**
     * @brief 新日志插入信号
     */
    void logInserted();

    /**
     * @brief 日志清空信号
     */
    void logsCleared();

private:
    QSqlDatabase m_db; ///< 外部传入的数据库连接句柄（值拷贝，隐式共享）
};
