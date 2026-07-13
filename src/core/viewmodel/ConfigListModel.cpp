/**
 * @file ConfigListModel.cpp
 * @brief ConfigListModel 实现
 *
 * 与 VpnManager 平权解耦，通过信号/槽通信：
 * - deleteConfig → emit requestStopConfig → 外部决定是否停止
 * - deleteConfig → emit configDeleted → 外部清理资源
 * - 外部 → onRunningStateChanged → 更新 m_runningInstances 缓存
 */
#include "ConfigListModel.h"
#include "core/application/config/ConfigCommandService.h"
#include "core/application/config/ConfigImportExportService.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include <QFutureWatcher>
#include <QUrl>

ConfigListModel::ConfigListModel(NetworkConfigRepository *repo,
                                 DaemonApi *daemonApi,
                                 ConfigCommandService *commandService,
                                 ConfigImportExportService *importExportService,
                                 QObject *parent)
    : QAbstractListModel(parent)
    , m_repo(repo)
    , m_daemonApi(daemonApi)
    , m_commandService(commandService)
    , m_importExportService(importExportService)
{
    if (!m_commandService)
        m_commandService = new ConfigCommandService(repo, this);
    if (!m_importExportService)
        m_importExportService = new ConfigImportExportService(repo, daemonApi, this);
    // 首次加载全部配置数据
    refresh();
}

ConfigListModel::ConfigListModel(NetworkConfigRepository *repo, DaemonApi *daemonApi, QObject *parent)
    : ConfigListModel(repo, daemonApi, nullptr, nullptr, parent)
{
}

int ConfigListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_configs.size();
}

QVariant ConfigListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_configs.size())
        return {};

    const auto &cfg = m_configs.at(index.row());

    switch (role) {
    case InstanceNameRole: return cfg.instanceName();
    case DisplayNameRole:  return cfg.displayName;
    case HostnameRole:     return cfg.hostname;
    // RunningRole: 从完整状态缓存中查询 Running
    case RunningRole:
        return configRunStateIsRunning(m_instanceStates.value(cfg.instanceName(), ConfigRunState::Stopped));
    case UpdatedAtRole:    return QVariant{};
    case Qt::DisplayRole:  return cfg.displayName;
    default: return {};
    }
}

QHash<int, QByteArray> ConfigListModel::roleNames() const
{
    return {
        { InstanceNameRole, "instanceName" },
        { DisplayNameRole,  "displayName" },
        { HostnameRole,     "hostname" },
        { RunningRole,      "running"  },
        { UpdatedAtRole,    "updatedAt" }
    };
}

void ConfigListModel::refresh()
{
    beginResetModel();
    m_configs = m_repo->loadAll();
    endResetModel();
}

QString ConfigListModel::createNewConfig()
{
    const auto result = m_commandService->createNewConfig();
    if (!result.success) {
        emit errorOccurred(result.message);
        return {};
    }
    refresh();
    return result.instanceName;
}

void ConfigListModel::onRunningStateChanged(const QString &instanceName, ConfigRunState state)
{
    // 更新完整状态缓存
    m_instanceStates[instanceName] = state;

    // 如果该实例正在等待删除，且已停止（Unstarted），则执行删除
    if (m_pendingDeletion.contains(instanceName) &&
        state == ConfigRunState::Stopped) {
        m_pendingDeletion.remove(instanceName);
        m_instanceStates.remove(instanceName);
        performDelete(instanceName);
        return;
    }

    // 通知 QML 刷新对应行的 RunningRole
    for (int i = 0; i < m_configs.size(); ++i) {
        if (m_configs.at(i).instanceName() == instanceName) {
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {RunningRole});
            return;
        }
    }
}

bool ConfigListModel::deleteConfig(const QString &instanceName)
{
    const ConfigRunState state = m_instanceStates.value(instanceName,
        ConfigRunState::Stopped);

    // 若正在运行，先请求停止，待状态变为 Unstarted 后再删除
    if (state == ConfigRunState::Running) {
        m_pendingDeletion.insert(instanceName);
        emit requestStopConfig(instanceName);
        return true;
    }

    // 正在启动或停止中，禁止删除，避免 daemon 侧残留幽灵实例
    if (!configRunStateCanDelete(state)) {
        emit errorOccurred(QStringLiteral("配置正在启动或停止，请稍后再删除"));
        return false;
    }

    // 未运行则立即删除
    return performDelete(instanceName);
}

bool ConfigListModel::performDelete(const QString &instanceName)
{
    // 从数据库删除
    const auto result = m_commandService->deleteConfig(instanceName);
    if (!result.success) {
        emit errorOccurred(result.message);
        return false;
    }

    // 通知外部清理相关资源
    emit configDeleted(instanceName);

    // 刷新模型以反映变更
    refresh();
    return true;
}

bool ConfigListModel::renameConfig(const QString &instanceName, const QString &newDisplayName)
{
    const auto result = m_commandService->renameConfig(instanceName, newDisplayName);
    if (!result.success) {
        emit errorOccurred(result.message);
        return false;
    }

    refresh();
    return true;
}

void ConfigListModel::importConfig(const QString &filePath)
{
    emit importStarted();
    auto *watcher = new QFutureWatcher<ConfigOperationResult>(this);
    connect(watcher, &QFutureWatcher<ConfigOperationResult>::finished, this,
            [this, watcher]() {
        watcher->deleteLater();
        const auto result = watcher->result();
        if (result.success) {
            refresh();
            emit importSucceeded();
        } else {
            emit importFailed(result.message);
        }
    });

    watcher->setFuture(m_importExportService->importFromFile(QUrl(filePath)));
}

bool ConfigListModel::exportConfig(const QString &instanceName, const QString &filePath)
{
    const auto result = m_importExportService->exportToFile(instanceName, QUrl(filePath));
    if (!result.success) {
        emit errorOccurred(result.message);
        return false;
    }
    return true;
}
