/**
 * @file DatabaseConnection.h
 * @brief SQLite 数据库连接与迁移管理
 *
 * 每个 DatabaseConnection 实例维护一条独立的具名数据库连接，
 * 通过 UUID 避免连接名冲突。析构时自动关闭连接并释放资源。
 *
 * 迁移脚本在 open() 时自动执行，保证首次运行即拥有完整表结构。
 */
#pragma once
#include <QSqlDatabase>
#include <QString>

/**
 * @class DatabaseConnection
 * @brief SQLite 数据库连接管理器
 *
 * 封装连接的生命周期（打开 / 迁移 / 关闭），
 * 并提供默认数据库路径的静态工具方法。
 *
 * 设计要点：
 * - 每个实例使用 UUID 生成唯一的连接名，避免多实例冲突
 * - Qt 的 QSqlDatabase 为值语义（隐式共享），直接按值传递即可
 * - open() 失败时连接不会残留，因为连接尚未成功建立
 */
class DatabaseConnection {
public:
    /**
     * @brief 构造连接管理器
     * @param dbPath SQLite 数据库文件的完整路径，需确保父目录存在
     */
    explicit DatabaseConnection(const QString &dbPath);

    /**
     * @brief 关闭数据库连接并从 QSqlDatabase 连接池中移除
     *
     * 先调用 close() 关闭连接（参数 false 表示不发出 aboutToClose 信号），
     * 再调用 removeDatabase() 清理连接名，防止连接名泄露。
     */
    ~DatabaseConnection();

    /**
     * @brief 打开数据库并执行迁移
     * @return true 表示打开成功且所有迁移语句执行成功
     *
     * 内部流程：
     * 1. 添加 SQLite 数据库连接（QSQLITE 驱动）
     * 2. 启用 WAL 日志模式（提升并发读性能）
     * 3. 启用外键约束（PRAGMA foreign_keys=ON）
     * 4. 执行迁移脚本建表
     */
    bool open();

    /**
     * @brief 获取 QSqlDatabase 句柄
     * @return 可用于执行 SQL 查询的数据库句柄（值拷贝，隐式共享零开销）
     */
    QSqlDatabase database() const;

    /**
     * @brief 执行建表与索引迁移脚本
     * @return true 表示所有迁移语句执行成功
     *
     * 当前包含以下表的迁移：
     * - network_configs：配置元数据表
     * - network_config_fields：配置字段明细表（字段级读写）
     * - favorite_nodes：节点收藏表
     * - runtime_instances：运行时实例缓存表
     *
     * 所有建表语句使用 IF NOT EXISTS，确保幂等执行。
     */
    bool runMigrations();

    /**
     * @brief 计算默认数据库存储路径
     * @return 平台相关的 AppConfigLocation/qteasytier.db 路径
     *
     * 例如 Linux 平台：
     * ~/.config/qteasytier/QtEasyTier/qteasytier.db
     */
    static QString defaultDatabasePath();

private:
    QString m_dbPath;         ///< 数据库文件的完整路径
    QString m_connectionName; ///< 唯一连接名，由 UUID 生成，保证多实例不冲突
};
