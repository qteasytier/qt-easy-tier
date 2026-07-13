/**
 * @file NetworkConfigRepository.h
 * @brief 网络配置持久化仓库
 *
 * 封装 network_configs 和 network_config_fields 表的 CRUD 操作。
 * 配置以字段级方式存储（每条记录一个字段的 JSON 值），
 * 通过 NetworkConf::toFieldMap()/fromFieldMap() 进行转换。
 *
 * 设计理念：
 * - 字段级存储：每个配置字段单独一行，支持增量读写，无需全量序列化
 * - 元数据表与字段表分离：name/时间戳等元信息与字段值分开存储
 * - 不持有 DatabaseConnection 所有权，依赖外部传入已打开的 QSqlDatabase 句柄
 */
#pragma once
#include <QObject>
#include <QList>
#include <QSqlDatabase>
#include <optional>
#include "core/config/NetworkConf.h"

/**
 * @class NetworkConfigRepository
 * @brief 网络配置的持久化仓库
 *
 * 提供配置的增删改查操作，内部使用参数绑定防止 SQL 注入。
 * 所有操作均为同步执行（SQLite 文件锁自动处理并发安全）。
 */
class NetworkConfigRepository : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造仓库实例
     * @param db 已打开的 SQLite 数据库连接
     * @param parent Qt 父对象，用于生命周期管理
     */
    explicit NetworkConfigRepository(QSqlDatabase db, QObject *parent = nullptr);

    /**
     * @brief 保存完整配置（元数据 + 所有字段）
     * @param config 要保存的网络配置对象
     * @return true 表示元数据和所有字段均保存成功
     *
     * 内部流程：
     * 1. 根据 instanceName 是否存在决定 INSERT 或 UPDATE 元数据行
     * 2. 遍历 toFieldMap() 的每个字段，逐字段保存到 fields 表
     */
    bool save(const NetworkConf &config);

    /**
     * @brief 保存单个字段值（增量更新）
     * @param instanceName 配置实例名
     * @param fieldName 字段名称
     * @param value 字段值（任意 QVariant 类型）
     * @return true 表示保存成功
     *
     * 内部使用 INSERT OR REPLACE 实现 upsert 语义。
     * 字段值会被 JSON 序列化后存入 field_json 列。
     */
    bool saveField(const QString &instanceName, const QString &fieldName, const QVariant &value);

    /**
     * @brief 加载单个配置的完整数据
     * @param instanceName 配置实例名
     * @return 成功时返回 NetworkConf 对象；配置不存在时返回 std::nullopt
     *
     * 内部流程：
     * 1. 先检查配置是否存在（exists 查询）
     * 2. 从 network_config_fields 加载所有字段的 JSON 值
     * 3. 调用 fromFieldMap() 反序列化为 NetworkConf
     * 4. 从 network_configs 读取 displayName 等元信息
     */
    std::optional<NetworkConf> load(const QString &instanceName) const;

    /**
     * @brief 加载全部配置列表
     * @return 所有已存储的配置对象，按更新时间降序排列
     */
    QList<NetworkConf> loadAll() const;

    /**
     * @brief 删除指定配置
     * @param instanceName 要删除的配置实例名
     * @return true 表示删除成功
     *
     * 由于 network_config_fields 表有 ON DELETE CASCADE 外键约束，
     * 删除 network_configs 行时字段行会自动级联删除。
     * 这里也显式删除字段行作为双重保障。
     */
    bool remove(const QString &instanceName);

    /**
     * @brief 检查配置是否存在
     * @param instanceName 配置实例名
     * @return true 表示配置已存在
     */
    bool exists(const QString &instanceName) const;

private:
    QSqlDatabase m_db; ///< 外部传入的数据库连接句柄（值拷贝，隐式共享）
};
