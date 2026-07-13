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

    // 尝试打开文件，失败则立即返回错误
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return finishedResult(ConfigOperationResult::fail(QStringLiteral("无法打开文件: %1").arg(localPath)));

    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 生成不重复的实例名
    QString instanceName;
    do {
        instanceName = generateInstanceName();
    } while (m_repository && m_repository->exists(instanceName));

    // 校验 TOML 语法合法性
    try {
        const auto parsed = toml::parse(content.toStdString());
        Q_UNUSED(parsed)
    } catch (const toml::parse_error &e) {
        return finishedResult(ConfigOperationResult::fail(
            QStringLiteral("TOML 格式错误: %1").arg(QString::fromUtf8(e.what()))));
    }

    // 将 TOML 反序列化为 NetworkConf，并以文件名作为显示名
    NetworkConf cfg = NetworkConfToml::fromToml(content, instanceName);
    cfg.displayName = QFileInfo(localPath).baseName();

    // 业务规则校验
    const QStringList errors = ConfigValidator::validate(cfg);
    if (!errors.isEmpty())
        return finishedResult(ConfigOperationResult::fail(errors.join(QStringLiteral("\n"))));

    if (!m_daemonApi)
        return finishedResult(ConfigOperationResult::fail(QStringLiteral("daemon 配置校验不可用")));

    // 异步调用 daemon 校验，通过后才真正保存配置
    auto *futureInterface = new QFutureInterface<ConfigOperationResult>();
    futureInterface->reportStarted();
    auto resultFuture = futureInterface->future();

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this,
            [this, cfg, watcher, futureInterface]() mutable {
        watcher->deleteLater();
        try {
            // daemon 校验成功，获取结果（不使用时仅检查无异常）
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

ConfigOperationResult ConfigImportExportService::exportToFile(const QString &instanceName, const QUrl &fileUrl)
{
    if (!m_repository)
        return ConfigOperationResult::fail(QStringLiteral("配置仓库不可用"));

    // 从仓库加载配置并序列化为 TOML 写入文件
    auto loaded = m_repository->load(instanceName);
    if (!loaded.has_value())
        return ConfigOperationResult::fail(QStringLiteral("配置不存在: %1").arg(instanceName));

    const QString localPath = fileUrl.toLocalFile();
    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return ConfigOperationResult::fail(QStringLiteral("无法写入文件: %1").arg(localPath));

    file.write(NetworkConfToml::toToml(loaded.value()).toUtf8());
    file.close();
    return ConfigOperationResult::ok(instanceName);
}

QString ConfigImportExportService::generateInstanceName() const
{
    return QStringLiteral("QtET-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QFuture<ConfigOperationResult> ConfigImportExportService::finishedResult(const ConfigOperationResult &result) const
{
    // 将同步结果包装为已完成的 QFuture，用于同步校验失败时立即返回
    QFutureInterface<ConfigOperationResult> futureInterface;
    futureInterface.reportStarted();
    futureInterface.reportResult(result);
    futureInterface.reportFinished();
    return futureInterface.future();
}
