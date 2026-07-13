/** @file ConfigImportExportService.h @brief 配置导入导出服务，负责文件级 TOML 配置的导入与导出操作 */
#pragma once

#include "core/application/config/ConfigOperationResult.h"

#include <QFuture>
#include <QObject>
#include <QUrl>

class DaemonApi;
class NetworkConfigRepository;

/** @brief 配置导入导出服务，提供从文件导入配置和将配置导出为 TOML 文件的功能 */
class ConfigImportExportService : public QObject
{
    Q_OBJECT

public:
    /** @brief 构造函数 @param repository 配置仓库 @param daemonApi daemon 接口，用于导入时的校验 @param parent 父对象 */
    explicit ConfigImportExportService(NetworkConfigRepository *repository,
                                       DaemonApi *daemonApi,
                                       QObject *parent = nullptr);

    /** @brief 从文件导入配置（异步），包含 TOML 解析、校验、daemon 校验 @param fileUrl 文件 URL @return 异步操作结果 */
    QFuture<ConfigOperationResult> importFromFile(const QUrl &fileUrl);
    /** @brief 将指定配置导出到文件 @param instanceName 配置实例名 @param fileUrl 目标文件 URL @return 同步操作结果 */
    ConfigOperationResult exportToFile(const QString &instanceName, const QUrl &fileUrl);

private:
    /** @brief 生成唯一实例名 @return QtET-<UUID> 格式字符串 */
    QString generateInstanceName() const;
    /** @brief 包装同步结果为已完成的 QFuture @param result 操作结果 @return 已完成的 future */
    QFuture<ConfigOperationResult> finishedResult(const ConfigOperationResult &result) const;

    NetworkConfigRepository *m_repository = nullptr;
    DaemonApi *m_daemonApi = nullptr;
};
