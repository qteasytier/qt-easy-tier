/** @file ConfigCommandService.h @brief 配置命令服务，负责配置的读写与增删改操作 */
#pragma once

#include "core/config/ConfigOperationResult.h"
#include "config/NetworkConf.h"

#include <QObject>
#include <optional>

class NetworkConfigRepository;

/** @brief 配置命令服务，提供配置的读写与增删改操作，统一返回 ConfigOperationResult */
class ConfigCommandService : public QObject
{
    Q_OBJECT

public:
    /** @brief 构造函数 @param repository 配置仓库 @param parent 父对象 */
    explicit ConfigCommandService(NetworkConfigRepository *repository, QObject *parent = nullptr);

    /** @brief 加载单个配置 @param instanceName 配置实例名 @return 配置对象，不存在时返回 std::nullopt */
    std::optional<NetworkConf> load(const QString &instanceName) const;
    /** @brief 加载全部配置 @return 全部配置列表 */
    QList<NetworkConf> loadAll() const;
    /** @brief 保存完整配置（编辑器的落盘操作） @param conf 待保存的配置 @return 是否成功 */
    bool save(const NetworkConf &conf) const;

    /** @brief 创建一份新配置 @return 包含实例名的操作结果 */
    ConfigOperationResult createNewConfig();
    /** @brief 删除指定配置 @param instanceName 配置实例名 @return 操作结果 */
    ConfigOperationResult deleteConfig(const QString &instanceName);
    /** @brief 重命名指定配置 @param instanceName 配置实例名 @param newDisplayName 新的显示名称 @return 操作结果 */
    ConfigOperationResult renameConfig(const QString &instanceName, const QString &newDisplayName);

private:
    /** @brief 生成不重复的显示名称 @return 格式为 "新配置 N" 的显示名 */
    QString generateDisplayName() const;

    NetworkConfigRepository *m_repository = nullptr;
};
