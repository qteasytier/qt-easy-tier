/** @file ConfigImportExportService.cpp @brief ConfigImportExportService 实现 */
#include "ConfigImportExportService.h"

#include "core/application/config/ConfigPayloadBuilder.h"
#include "core/config/ConfigValidator.h"
#include "core/config/NetworkConfToml.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"

#include <QFile>
#include <QFileInfo>
#include <QFutureInterface>
#include <QFutureWatcher>
#include <QUrl>
#include <QUuid>
#include <toml.hpp>

ConfigImportExportService::ConfigImportExportService(NetworkConfigRepository *repository,
                                                       DaemonApi *daemonApi,
                                                       QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_daemonApi(daemonApi)
{
}

QFuture<ConfigOperationResult> ConfigImportExportService::importFromFile(const QUrl &fileUrl)
{
    const QString localPath = fileUrl.toLocalFile();

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return finishedResult(ConfigOperationResult::fail(QStringLiteral("无法打开文件: %1").arg(localPath)));

    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    return importFromToml(content, QFileInfo(localPath).baseName());
}

QFuture<ConfigOperationResult> ConfigImportExportService::importFromUrl(const QString &url)
{
    ConfigTextResult decoded = ConfigUrlCodec::decodeUrl(url);
    if (!decoded.success)
        return finishedResult(ConfigOperationResult::fail(decoded.error));

    return importFromToml(decoded.value, QStringLiteral("URL导入配置"));
}

ConfigOperationResult ConfigImportExportService::exportToFile(const QString &instanceName, const QUrl &fileUrl)
{
    ConfigTextResult tomlResult = tomlForExport(instanceName);
    if (!tomlResult.success)
        return ConfigOperationResult::fail(tomlResult.error);

    const QString localPath = fileUrl.toLocalFile();
    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return ConfigOperationResult::fail(QStringLiteral("无法写入文件: %1").arg(localPath));

    file.write(tomlResult.value.toUtf8());
    file.close();
    return ConfigOperationResult::ok(instanceName);
}

ConfigTextResult ConfigImportExportService::exportToUrl(const QString &instanceName)
{
    ConfigTextResult tomlResult = tomlForExport(instanceName);
    if (!tomlResult.success)
        return tomlResult;

    return ConfigUrlCodec::encodeToml(tomlResult.value);
}

QFuture<ConfigOperationResult> ConfigImportExportService::importFromToml(const QString &content,
                                                                           const QString &displayName)
{
    QString instanceName;
    do {
        instanceName = generateInstanceName();
    } while (m_repository && m_repository->exists(instanceName));

    try {
        const auto parsed = toml::parse(content.toStdString());
        Q_UNUSED(parsed)
    } catch (const toml::parse_error &e) {
        return finishedResult(ConfigOperationResult::fail(
            QStringLiteral("TOML 格式错误: %1").arg(QString::fromUtf8(e.what()))));
    }

    NetworkConf cfg = NetworkConfToml::fromToml(content, instanceName);
    cfg.displayName = displayName;

    const QStringList errors = ConfigValidator::validate(cfg);
    if (!errors.isEmpty())
        return finishedResult(ConfigOperationResult::fail(errors.join(QStringLiteral("\n"))));

    if (!m_daemonApi)
        return finishedResult(ConfigOperationResult::fail(QStringLiteral("daemon 配置校验不可用")));

    auto *futureInterface = new QFutureInterface<ConfigOperationResult>();
    futureInterface->reportStarted();
    auto resultFuture = futureInterface->future();

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this,
            [this, cfg, watcher, futureInterface]() mutable {
        watcher->deleteLater();
        try {
            watcher->result();

            if (!m_repository || !m_repository->save(cfg)) {
                futureInterface->reportResult(ConfigOperationResult::fail(QStringLiteral("导入配置保存失败")));
            } else {
                futureInterface->reportResult(ConfigOperationResult::ok(cfg.instanceName()));
            }
        } catch (...) {
            futureInterface->reportResult(ConfigOperationResult::fail(QStringLiteral("daemon 配置校验不通过")));
        }
        futureInterface->reportFinished();
        delete futureInterface;
    });

    watcher->setFuture(m_daemonApi->parseConfig(ConfigPayloadBuilder::daemonConfigPayload(cfg)));
    return resultFuture;
}

ConfigTextResult ConfigImportExportService::tomlForExport(const QString &instanceName) const
{
    if (!m_repository)
        return ConfigTextResult::fail(QStringLiteral("配置仓库不可用"));

    auto loaded = m_repository->load(instanceName);
    if (!loaded.has_value())
        return ConfigTextResult::fail(QStringLiteral("配置不存在: %1").arg(instanceName));

    return ConfigTextResult::ok(NetworkConfToml::toToml(loaded.value()));
}

QString ConfigImportExportService::generateInstanceName() const
{
    return QStringLiteral("QtET-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QFuture<ConfigOperationResult> ConfigImportExportService::finishedResult(const ConfigOperationResult &result) const
{
    QFutureInterface<ConfigOperationResult> futureInterface;
    futureInterface.reportStarted();
    futureInterface.reportResult(result);
    futureInterface.reportFinished();
    return futureInterface.future();
}
