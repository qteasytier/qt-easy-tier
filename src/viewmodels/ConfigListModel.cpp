/**
 * @file ConfigListModel.cpp
 * @brief ConfigListModel 实现
 *
 * 与 VpnRuntimeService 平权解耦，通过信号/槽通信：
 * - deleteConfig → emit requestStopConfig → 外部决定是否停止
 * - deleteConfig → emit configDeleted → 外部清理资源
 * - 外部 → onRunningStateChanged → 更新 m_runningInstances 缓存
 */
#include "ConfigListModel.h"
#include "app_service/config/ConfigCommandService.h"
#include "app_service/config/ConfigImportExportService.h"
#include <QFutureWatcher>
#include <QUrl>

ConfigListModel::ConfigListModel(ConfigCommandService *commandService,
                                 ConfigImportExportService *importExportService,
                                 QObject *parent)
    : QAbstractListModel(parent)
    , m_commandService(commandService)
    , m_importExportService(importExportService)
{
    // 首次加载全部配置数据
    refresh();
}

int ConfigListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    // 本地配置 + 外部实例条目
    return m_configs.size() + m_externalInstanceList.size();
}

QVariant ConfigListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    // 本地配置条目
    if (index.row() < m_configs.size()) {
        const auto &cfg = m_configs.at(index.row());
        switch (role) {
        case InstanceNameRole: return cfg.instanceName();
        case DisplayNameRole:  return cfg.displayName;
        case HostnameRole:     return cfg.hostname;
        // RunningRole: 从完整状态缓存中查询 Running
        case RunningRole:
            return configRunStateIsRunning(m_instanceStates.value(cfg.instanceName(), ConfigRunState::Stopped));
        // RunStateRole: 暴露完整运行状态（0=Stopped,1=Starting,2=Running,3=Stopping,4=Error）
        case RunStateRole:
            return static_cast<int>(m_instanceStates.value(cfg.instanceName(), ConfigRunState::Stopped));
        case UpdatedAtRole:    return QVariant{};
        case IsExternalRole:   return false;
        case Qt::DisplayRole:  return cfg.displayName;
        default: return {};
        }
    }

    // 外部实例条目（本地配置之后）：仅展示 instance_name，始终处于运行中
    const int externalRow = index.row() - m_configs.size();
    if (externalRow >= m_externalInstanceList.size())
        return {};
    const QString &name = m_externalInstanceList.at(externalRow);
    switch (role) {
    case InstanceNameRole: return name;
    case DisplayNameRole:  return name;
    case HostnameRole:     return QVariant{};
    case RunningRole:      return true;
    case RunStateRole:     return static_cast<int>(ConfigRunState::Running);
    case UpdatedAtRole:    return QVariant{};
    case IsExternalRole:   return true;
    case Qt::DisplayRole:  return name;
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
        { RunStateRole,     "runState" },
        { UpdatedAtRole,    "updatedAt" },
        { IsExternalRole,   "isExternal" }
    };
}

void ConfigListModel::refresh()
{
    beginResetModel();
    m_configs = m_commandService ? m_commandService->loadAll() : QList<NetworkConf>{};
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
    // 通知外部同步本地 controller（新建配置后运行时即应存在对应状态机）
    emit configCreated(result.instanceName);
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

    // 通知 QML 刷新对应行的 RunningRole 与 RunStateRole
    for (int i = 0; i < m_configs.size(); ++i) {
        if (m_configs.at(i).instanceName() == instanceName) {
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {RunningRole, RunStateRole});
            return;
        }
    }
}

void ConfigListModel::onExternalInstancesChanged(const QStringList &instanceNames)
{
    if (m_externalInstanceList == instanceNames)
        return;

    // 外部实例条目整体追加在本地配置之后，集合变化时整体重置模型
    beginResetModel();
    m_externalInstanceList = instanceNames;
    endResetModel();
}

bool ConfigListModel::isExternal(const QString &instanceName) const
{
    return m_externalInstanceList.contains(instanceName);
}

bool ConfigListModel::isLocalInstance(const QString &instanceName) const
{
    for (const auto &cfg : std::as_const(m_configs)) {
        if (cfg.instanceName() == instanceName)
            return true;
    }
    return false;
}

ConfigRunState ConfigListModel::instanceState(const QString &instanceName) const
{
    // 外部实例必然处于运行中（由 daemon 心跳发现）
    if (m_externalInstanceList.contains(instanceName))
        return ConfigRunState::Running;
    return m_instanceStates.value(instanceName, ConfigRunState::Stopped);
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
    // 通知协调方同步编辑器等共享快照的显示名称，避免后续完整保存覆盖重命名
    emit configRenamed(instanceName, newDisplayName.trimmed());
    return true;
}

void ConfigListModel::importConfigFile(const QString &filePath)
{
    emit importStarted();
    auto *watcher = new QFutureWatcher<ConfigOperationResult>(this);
    connect(watcher, &QFutureWatcher<ConfigOperationResult>::finished, this,
            [this, watcher]() {
        watcher->deleteLater();
        const auto result = watcher->result();
        if (result.success) {
            refresh();
            // 通知外部同步本地 controller（导入成功后运行时即应存在对应状态机）
            emit configCreated(result.instanceName);
            emit importSucceeded();
        } else {
            emit importFailed(result.message);
        }
    });

    watcher->setFuture(m_importExportService->importFromFile(QUrl(filePath)));
}

void ConfigListModel::importConfigUrl(const QString &url)
{
    emit importStarted();
    auto *watcher = new QFutureWatcher<ConfigOperationResult>(this);
    connect(watcher, &QFutureWatcher<ConfigOperationResult>::finished, this,
            [this, watcher]() {
        watcher->deleteLater();
        const auto result = watcher->result();
        if (result.success) {
            refresh();
            // 通知外部同步本地 controller（导入成功后运行时即应存在对应状态机）
            emit configCreated(result.instanceName);
            emit importSucceeded();
        } else {
            emit importFailed(result.message);
        }
    });

    watcher->setFuture(m_importExportService->importFromUrl(url));
}

bool ConfigListModel::exportConfigFile(const QString &instanceName, const QString &filePath)
{
    const auto result = m_importExportService->exportToFile(instanceName, QUrl(filePath));
    if (!result.success) {
        emit errorOccurred(result.message);
        return false;
    }
    return true;
}

QString ConfigListModel::exportConfigUrl(const QString &instanceName)
{
    ConfigTextResult result = m_importExportService->exportToUrl(instanceName);
    if (!result.success) {
        emit errorOccurred(result.error);
        return {};
    }
    return result.value;
}
