/** @file ConfigImportExportService.h @brief 配置导入导出服务，负责文件/URL 的 TOML 配置导入与导出操作 */
#pragma once

#include "core/application/config/ConfigOperationResult.h"
#include "core/config/ConfigUrlCodec.h"

#include <QFuture>
#include <QObject>
#include <QUrl>

class DaemonApi;
class NetworkConfigRepository;

/** @brief 配置导入导出服务，提供从文件/URL 导入配置和将配置导出为文件/URL 的功能 */
class ConfigImportExportService : public QObject
{
    Q_OBJECT

public:
    explicit ConfigImportExportService(NetworkConfigRepository *repository,
                                       DaemonApi *daemonApi,
                                       QObject *parent = nullptr);

    QFuture<ConfigOperationResult> importFromFile(const QUrl &fileUrl);
    ConfigOperationResult exportToFile(const QString &instanceName, const QUrl &fileUrl);

    QFuture<ConfigOperationResult> importFromUrl(const QString &url);
    ConfigTextResult exportToUrl(const QString &instanceName);

private:
    QFuture<ConfigOperationResult> importFromToml(const QString &content, const QString &displayName);
    ConfigTextResult tomlForExport(const QString &instanceName) const;

    QString generateInstanceName() const;
    QFuture<ConfigOperationResult> finishedResult(const ConfigOperationResult &result) const;

    NetworkConfigRepository *m_repository = nullptr;
    DaemonApi *m_daemonApi = nullptr;
};
