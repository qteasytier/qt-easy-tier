/** @file ConfigCommandService.h @brief 配置命令服务，负责创建、删除、重命名配置的写操作 */
#pragma once

#include "core/application/config/ConfigOperationResult.h"

#include <QObject>

class NetworkConfigRepository;

/** @brief 配置命令服务，提供配置的增删改等写操作，统一返回 ConfigOperationResult */
class ConfigCommandService : public QObject
{
    Q_OBJECT

public:
    /** @brief 构造函数 @param repository 配置仓库 @param parent 父对象 */
    explicit ConfigCommandService(NetworkConfigRepository *repository, QObject *parent = nullptr);

    /** @brief 创建一份新配置 @return 包含实例名的操作结果 */
    ConfigOperationResult createNewConfig();
    /** @brief 删除指定配置 @param instanceName 配置实例名 @return 操作结果 */
    ConfigOperationResult deleteConfig(const QString &instanceName);
    /** @brief 重命名指定配置 @param instanceName 配置实例名 @param newDisplayName 新的显示名称 @return 操作结果 */
    ConfigOperationResult renameConfig(const QString &instanceName, const QString &newDisplayName);

private:
    /** @brief 生成唯一的配置实例名 @return 格式为 QtET-<UUID> 的实例名 */
    QString generateInstanceName() const;
    /** @brief 生成不重复的显示名称 @return 格式为 "新配置 N" 的显示名 */
    QString generateDisplayName() const;

    NetworkConfigRepository *m_repository = nullptr;
};
