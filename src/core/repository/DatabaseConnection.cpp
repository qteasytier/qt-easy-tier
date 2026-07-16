/**
 * @file DatabaseConnection.cpp
 * @brief SQLite 数据库连接实现
 */
#include "DatabaseConnection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include "core/util/LogHelper.h"

/**
 * @brief 构造函数：记录数据库路径并生成唯一连接名
 * @param dbPath SQLite 数据库文件完整路径
 *
 * 连接名通过 UUID 生成（去除花括号），确保即使创建多个 DatabaseConnection
 * 实例也不会在 QSqlDatabase 连接池中发生冲突。
 */
DatabaseConnection::DatabaseConnection(const QString &dbPath)
    : m_dbPath(dbPath)
    , m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

/**
 * @brief 析构函数：安全关闭并移除数据库连接
 *
 * 两步操作：
 * 1. close() 关闭底层连接（参数 false 表示不发出 aboutToClose 信号，
 *    避免在析构阶段触发信号导致悬垂引用）
 * 2. removeDatabase() 从 Qt 全局连接池中移除连接名，释放相关资源
 *
 * 注意：如果连接从未成功打开（open() 返回 false），此时 database() 可能
 * 已处于关闭状态，close() 为无害操作。
 */
DatabaseConnection::~DatabaseConnection()
{
    QSqlDatabase::database(m_connectionName, false).close();
    QSqlDatabase::removeDatabase(m_connectionName);
}

/**
 * @brief 计算默认数据库存储路径（静态工具方法）
 *
 * 使用 QStandardPaths::AppConfigLocation 获取平台相关的应用配置目录，
 * 并自动创建该目录（如果不存在）。
 *
 * 路径示例（Linux）：~/.config/qteasytier/QtEasyTier/qteasytier.db
 */
QString DatabaseConnection::defaultDatabasePath()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return configDir + QStringLiteral("/qteasytier.db");
}

/**
 * @brief 打开数据库连接并执行初始化
 * @return true 表示打开成功且所有 PRAGMA 和迁移语句执行成功
 *
 * 执行流程：
 * 1. 通过 QSQLITE 驱动注册新连接
 * 2. 打开数据库文件
 * 3. 设置 PRAGMA（WAL 模式 + 外键约束）
 * 4. 执行建表迁移脚本
 *
 * WAL（Write-Ahead Logging）模式的优势：
 * - 读写可并发：读操作不阻塞写操作，写操作不阻塞读操作
 * - 更好的并发性能：适合多线程 / 多进程场景
 *
 * 外键约束：SQLite 默认不强制执行外键，需显式启用。
 * 本项目中外键关系如 network_config_fields 引用 network_configs。
 */
bool DatabaseConnection::open()
{
    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        LogHelper::logError(QStringLiteral("数据库打开失败: %1").arg(db.lastError().text()), "Database");
        return false;
    }

    // 启用 WAL 模式与外键约束以获得更好的并发性能
    {
        QSqlQuery pragma(db);
        pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
        pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    }

    return runMigrations();
}

/**
 * @brief 获取当前连接的 QSqlDatabase 句柄
 * @return 可执行 SQL 的数据库句柄
 *
 * QSqlDatabase 为值语义类型，内部使用隐式共享（QExplicitlySharedDataPointer），
 * 按值返回不会产生实质开销。
 */
QSqlDatabase DatabaseConnection::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

/**
 * @brief 执行数据库迁移脚本
 * @return true 表示所有建表语句执行成功
 *
 * 迁移采用"全量建表"策略（不使用版本号增量迁移）：
 * - 所有建表语句使用 CREATE TABLE IF NOT EXISTS
 * - 所有索引语句使用 CREATE INDEX IF NOT EXISTS
 * - 因此迁移脚本天然幂等，可安全重复执行
 *
 * 表结构说明：
 *
 * network_configs（配置元数据表）：
 * - instance_name TEXT PRIMARY KEY   : 配置实例唯一标识
 * - display_name  TEXT NOT NULL      : 显示名称
 * - created_at    INTEGER NOT NULL   : 创建时间（Unix 秒）
 * - updated_at    INTEGER NOT NULL   : 最后修改时间（Unix 秒）
 * - favorite      INTEGER DEFAULT 0  : 是否收藏
 * - temporary     INTEGER DEFAULT 0  : 是否为临时配置
 *
 * network_config_fields（配置字段明细表，字段级读写）：
 * - instance_name TEXT NOT NULL      : 所属配置实例名
 * - field_name    TEXT NOT NULL      : 字段名称
 * - field_json    TEXT NOT NULL      : 字段值（JSON 序列化）
 * - updated_at    INTEGER NOT NULL   : 最后更新时间
 * - PRIMARY KEY (instance_name, field_name) 联合主键
 * - FOREIGN KEY 级联删除：删除配置时自动清理字段
 *
 * favorite_nodes（节点收藏表）：
 * - id         INTEGER PRIMARY KEY AUTOINCREMENT : 自增主键
 * - name       TEXT NOT NULL                     : 节点名称
 * - uri        TEXT NOT NULL UNIQUE              : 节点 URI（唯一约束）
 * - public_key TEXT                              : 公钥（可选）
 * - created_at INTEGER NOT NULL                  : 收藏时间
 *
 * runtime_instances（运行时实例缓存表）：
 * - instance_name TEXT PRIMARY KEY : 配置实例名
 * - is_running    INTEGER          : 是否正在运行
 * - info_json     TEXT             : 运行时信息（JSON）
 * - updated_at    INTEGER          : 最后更新时间
 *
 * 已知局限：
 * - 迁移未使用事务包裹，若中途失败会留下已创建的表。
 *   但由于所有语句均为 IF NOT EXISTS，重复执行时幂等，影响有限。
 * - 当前 WAL 模式的 PRAGMA 执行结果未检查，若失败不会阻塞后续流程。
 */
bool DatabaseConnection::runMigrations()
{
    QSqlQuery query(database());

    const QStringList migrations = {
        // --- 配置元数据表 ---
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS network_configs ("
            "  instance_name TEXT PRIMARY KEY,"
            "  display_name  TEXT NOT NULL,"
            "  created_at    INTEGER NOT NULL,"
            "  updated_at    INTEGER NOT NULL,"
            "  favorite      INTEGER NOT NULL DEFAULT 0,"
            "  temporary     INTEGER NOT NULL DEFAULT 0"
            ")"
        ),
        // --- 配置字段明细表（字段级读写）---
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS network_config_fields ("
            "  instance_name TEXT NOT NULL,"
            "  field_name    TEXT NOT NULL,"
            "  field_json    TEXT NOT NULL,"
            "  updated_at    INTEGER NOT NULL,"
            "  PRIMARY KEY (instance_name, field_name),"
            "  FOREIGN KEY (instance_name) REFERENCES network_configs(instance_name) ON DELETE CASCADE"
            ")"
        ),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_ncf_instance_name "
            "ON network_config_fields(instance_name)"
        ),
        // --- 节点收藏 ---
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS favorite_nodes ("
            "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  name       TEXT NOT NULL,"
            "  uri        TEXT NOT NULL UNIQUE,"
            "  public_key TEXT,"
            "  created_at INTEGER NOT NULL"
            ")"
        ),
        // --- 运行时实例缓存 ---
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS runtime_instances ("
            "  instance_name TEXT PRIMARY KEY,"
            "  is_running    INTEGER,"
            "  info_json     TEXT,"
            "  updated_at    INTEGER"
            ")"
        ),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS app_logs ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  timestamp INTEGER NOT NULL,"
            "  level INTEGER NOT NULL,"
            "  context TEXT,"
            "  message TEXT NOT NULL"
            ")"
        ),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_app_logs_timestamp "
            "ON app_logs(timestamp DESC, id DESC)"
        ),
    };

    for (const auto &sql : migrations) {
        if (!query.exec(sql)) {
            LogHelper::logError(QStringLiteral("迁移失败: %1，SQL: %2").arg(query.lastError().text()).arg(sql), "Database");
            return false;
        }
    }

    return true;
}
