/**
 * @file NetworkConfigRepository.cpp
 * @brief 网络配置仓库实现
 */
#include "NetworkConfigRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDateTime>
#include "core/util/LogHelper.h"

NetworkConfigRepository::NetworkConfigRepository(QSqlDatabase db, QObject *parent)
    : QObject(parent), m_db(db)
{
}

/**
 * @brief 保存完整配置（元数据 + 所有字段）
 *
 * 执行流程：
 * 1. 验证 instanceName 非空
 * 2. 确定显示名称：优先使用传入的 displayName，否则用 instanceName
 * 3. 检查元数据表中是否已有该配置：
 *    - 已有 → UPDATE display_name 和 updated_at
 *    - 没有 → INSERT 新行，同时设置 created_at 和 updated_at
 * 4. 遍历配置的所有字段，逐字段调用 saveField() 持久化
 *
 * 注意：步骤 3 和步骤 4 之间没有事务包裹。如果步骤 4 中某个字段保存失败，
 * 步骤 3 的元数据修改已经持久化。这在当前场景下可接受，因为：
 * - 字段级存储天然支持部分写入
 * - 下次 save() 会重新保存所有字段
 */
bool NetworkConfigRepository::save(const NetworkConf &config)
{
    // 验证：实例名不能为空
    if (config.instanceName().isEmpty()) {
        LogHelper::logWarning("save: instanceName 为空", "NetworkConfig");
        return false;
    }

    // 启动事务，确保元数据和字段写入原子性
    m_db.transaction();

    const qint64 now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    // 显示名称：优先使用用户设置的值，否则使用实例名作为默认
    const QString displayName = config.displayName.isEmpty()
        ? config.instanceName() : config.displayName;

    // --- 第一步：保存 / 更新配置元数据 ---
    {
        QSqlQuery check(m_db);
        check.prepare(QStringLiteral("SELECT 1 FROM network_configs WHERE instance_name = ?"));
        check.addBindValue(config.instanceName());
        check.exec();
        const bool rowExists = check.next();

        QSqlQuery meta(m_db);
        if (rowExists) {
            // 已有记录 → 更新显示名称和修改时间
            meta.prepare(QStringLiteral(
                "UPDATE network_configs SET display_name = ?, updated_at = ? WHERE instance_name = ?"
            ));
            meta.addBindValue(displayName);
            meta.addBindValue(now);
            meta.addBindValue(config.instanceName());
        } else {
            // 新记录 → 插入包含创建时间的完整行
            meta.prepare(QStringLiteral(
                "INSERT INTO network_configs (instance_name, display_name, created_at, updated_at) "
                "VALUES (?, ?, ?, ?)"
            ));
            meta.addBindValue(config.instanceName());
            meta.addBindValue(displayName);
            meta.addBindValue(now);  // created_at
            meta.addBindValue(now);  // updated_at
        }
        if (!meta.exec()) {
            LogHelper::logWarning(QStringLiteral("保存 network_configs 失败: %1").arg(meta.lastError().text()), "NetworkConfig");
            m_db.rollback();
            return false;
        }
    }

    // --- 第二步：逐字段保存到字段明细表 ---
    const QVariantMap map = config.toFieldMap();
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (!saveField(config.instanceName(), it.key(), it.value())) {
            m_db.rollback();
            return false;
        }
    }

    return m_db.commit();
}

/**
 * @brief 保存单个字段值
 *
 * JSON 序列化策略：
 * - 如果字段值本身是对象（QVariantMap/QVariantHash），直接序列化该对象
 * - 否则用 {"v": <value>} 包装，便于加载时区分"单值"和"对象值"
 *
 * 使用 INSERT OR REPLACE 实现 upsert：
 * - 主键 (instance_name, field_name) 不存在时插入新行
 * - 主键已存在时更新 field_json 和 updated_at
 */
bool NetworkConfigRepository::saveField(const QString &instanceName,
                                         const QString &fieldName,
                                         const QVariant &value)
{
    const qint64 now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    // 将 QVariant 转换为 QJsonValue 并以统一格式包装
    QJsonValue v = QJsonValue::fromVariant(value);
    QJsonObject wrapper;
    if (v.isObject()) {
        // 值本身是对象 → 直接使用，不额外包装
        wrapper = v.toObject();
    } else {
        // 值不是对象 → 用 "v" 键包装为单属性对象
        wrapper["v"] = v;
    }
    const QString fieldJson = QString::fromUtf8(
        QJsonDocument(wrapper).toJson(QJsonDocument::Compact));

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO network_config_fields "
        "(instance_name, field_name, field_json, updated_at) VALUES (?, ?, ?, ?)"
    ));
    query.addBindValue(instanceName);
    query.addBindValue(fieldName);
    query.addBindValue(fieldJson);
    query.addBindValue(now);

    if (!query.exec()) {
        LogHelper::logWarning(QStringLiteral("saveField 失败: %1").arg(query.lastError().text()), "NetworkConfig");
        return false;
    }
    return true;
}

/**
 * @brief 加载单个配置
 *
 * 加载流程：
 * 1. 通过 exists() 快速检查配置是否存在（不存在直接返回 nullopt）
 * 2. 从 network_config_fields 表读取所有字段的 JSON 值
 * 3. 反向解析 JSON 包装格式：
 *    - 如果是 {"v": ...} 单值包装 → 提取 v 的值
 *    - 否则 → 将整个对象作为字段值
 * 4. 调用 NetworkConf::fromFieldMap() 从字段映射构造配置对象
 * 5. 从 network_configs 读取 displayName 覆盖到配置对象上
 * 6. 标记配置为"干净"状态（无未保存修改）
 */
std::optional<NetworkConf> NetworkConfigRepository::load(const QString &instanceName) const
{
    // 快速检查：配置不存在则直接返回
    if (!exists(instanceName))
        return std::nullopt;

    QVariantMap values;
    {
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral(
            "SELECT field_name, field_json FROM network_config_fields WHERE instance_name = ?"
        ));
        query.addBindValue(instanceName);
        if (!query.exec())
            return std::nullopt;

        while (query.next()) {
            const QString name = query.value(0).toString();
            const QJsonDocument doc = QJsonDocument::fromJson(query.value(1).toByteArray());
            QJsonValue v = doc.object();

            // 反向解包 JSON 包装格式：
            // saveField() 中对非对象值包装了 {"v": value}
            // 这里检查是否只有 "v" 一个键，是则提取 v 的值
            const QJsonObject obj = v.toObject();
            if (obj.contains("v") && obj.size() == 1) {
                v = obj["v"];
            }

            values[name] = v.toVariant();
        }
    }

    // 从字段映射重建 NetworkConf 对象
    NetworkConf conf = NetworkConf::fromFieldMap(values, instanceName);

    // 读取显示名称等元信息并覆盖
    QSqlQuery meta(m_db);
    meta.prepare(QStringLiteral(
        "SELECT display_name FROM network_configs WHERE instance_name = ?"
    ));
    meta.addBindValue(instanceName);
    if (meta.exec() && meta.next())
        conf.displayName = meta.value(0).toString();

    // 标记为干净状态，表示刚从数据库加载，无未保存修改
    conf.markClean();
    return conf;
}

/**
 * @brief 加载全部配置
 *
 * 先从 network_configs 获取所有 instanceName（按更新时间降序），
 * 然后逐个调用 load() 加载完整数据。
 *
 * 性能注意：对 N 个配置会产生 N+1 次查询（1 次查列表 + N 次逐个加载）。
 * 对于配置数量通常不大的场景可接受；若配置数量增长可优化为 JOIN 查询。
 */
QList<NetworkConf> NetworkConfigRepository::loadAll() const
{
    QSqlQuery query(m_db);
    QList<NetworkConf> result;

    if (!query.exec(QStringLiteral(
            "SELECT instance_name FROM network_configs ORDER BY updated_at DESC")))
        return result;

    while (query.next()) {
        if (auto cfg = load(query.value(0).toString()))
            result.append(std::move(*cfg));
    }
    return result;
}

/**
 * @brief 删除指定配置及其所有字段
 *
 * 删除顺序：
 * 1. 先删除 network_configs 元数据行（主表）
 * 2. 再显式删除 network_config_fields 字段行
 *
 * 由于 network_config_fields 有 ON DELETE CASCADE 外键约束，
 * 步骤 1 执行后字段行会被自动级联删除。
 * 步骤 2 的显式删除是额外保障，即使 CASCADE 未生效也能清理干净。
 * 步骤 1 失败时整个操作返回 false，不继续步骤 2。
 */
bool NetworkConfigRepository::remove(const QString &instanceName)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM network_configs WHERE instance_name = ?"));
    query.addBindValue(instanceName);
    if (!query.exec())
        return false;

    QSqlQuery delFields(m_db);
    delFields.prepare(QStringLiteral("DELETE FROM network_config_fields WHERE instance_name = ?"));
    delFields.addBindValue(instanceName);
    delFields.exec();  // 忽略返回值，因为 CASCADE 可能已经删除了
    return true;
}

/**
 * @brief 检查指定配置是否存在
 * @return true 表示 network_configs 表中存在该 instanceName
 *
 * 使用 SELECT 1 而非 SELECT * 以提升查询效率。
 * 如果 SQL 执行失败（数据库连接问题等），返回 false。
 */
bool NetworkConfigRepository::exists(const QString &instanceName) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT 1 FROM network_configs WHERE instance_name = ?"));
    query.addBindValue(instanceName);
    if (!query.exec())
        return false;
    return query.next();
}
