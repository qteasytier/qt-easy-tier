/**
 * @file SettingsViewModel.cpp
 * @brief SettingsViewModel 实现
 *
 * 本地设置（settings3.json）与开机自启由本类直接管理；
 * 自动回连与版本更新检查委托 SettingsBackendService，
 * 本类仅负责将后端服务的信号转发为 QML 可绑定的属性通知。
 */
#include "SettingsViewModel.h"
#include "AppVersion.h"
#include "core/util/LogHelper.h"

SettingsViewModel::SettingsViewModel(SettingsBackendService *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
{
    load();

    // 转发设置后端服务（自动回连 / 更新检查）的状态信号给 QML
    if (m_backend) {
        connect(m_backend, &SettingsBackendService::autoReconnectChanged,
                this, &SettingsViewModel::autoReconnectChanged);
        connect(m_backend, &SettingsBackendService::autoReconnectBusyChanged,
                this, &SettingsViewModel::autoReconnectBusyChanged);
        connect(m_backend, &SettingsBackendService::autoReconnectOperationFailed,
                this, &SettingsViewModel::autoReconnectOperationFailed);
        connect(m_backend, &SettingsBackendService::updateCheckBusyChanged,
                this, &SettingsViewModel::updateCheckBusyChanged);
    }
}

bool SettingsViewModel::autoStart() const
{
    return m_autoStart;
}

bool SettingsViewModel::autoReconnect() const
{
    // 状态由设置后端服务持有，本类只读转发
    return m_backend ? m_backend->autoReconnect() : false;
}

bool SettingsViewModel::autoReconnectBusy() const
{
    return m_backend ? m_backend->autoReconnectBusy() : false;
}

bool SettingsViewModel::autoCheckUpdates() const
{
    return m_autoCheckUpdates;
}

void SettingsViewModel::setAutoCheckUpdates(bool value)
{
    if (m_autoCheckUpdates == value)
        return;

    m_autoCheckUpdates = value;
    emit autoCheckUpdatesChanged();
    save();
}

bool SettingsViewModel::updateCheckBusy() const
{
    return m_backend ? m_backend->updateCheckBusy() : false;
}

bool SettingsViewModel::hideServerNodes() const
{
    return m_hideServerNodes;
}

void SettingsViewModel::setHideServerNodes(bool value)
{
    if (m_hideServerNodes == value)
        return;

    m_hideServerNodes = value;
    emit hideServerNodesChanged();
    save();
}

bool SettingsViewModel::showExitPrompt() const
{
    return m_showExitPrompt;
}

void SettingsViewModel::setShowExitPrompt(bool value)
{
    if (m_showExitPrompt == value)
        return;

    m_showExitPrompt = value;
    emit showExitPromptChanged();
    save();
}

void SettingsViewModel::load()
{
    applySettings(m_store.load(settings()));

    emit autoStartChanged();
    emit autoCheckUpdatesChanged();
    emit hideServerNodesChanged();
    emit showExitPromptChanged();
    emit logLevelChanged();
    emit maxLogEntriesChanged();
}

void SettingsViewModel::save()
{
    m_store.save(settings());
}

void SettingsViewModel::refreshAutoReconnect()
{
    // 委托设置后端服务发起查询，结果经信号转发回 QML
    if (m_backend)
        m_backend->refreshAutoReconnect();
    else
        LogHelper::logWarning(QStringLiteral("设置后端服务不可用，无法查询自动回连状态"), "Settings");
}

void SettingsViewModel::setAutoReconnectEnabled(bool enabled)
{
    if (!m_backend) {
        LogHelper::logWarning(QStringLiteral("设置后端服务不可用，无法设置自动回连"), "Settings");
        emit autoReconnectOperationFailed(QStringLiteral("后端未连接，无法设置自动回连"));
        return;
    }

    m_backend->setAutoReconnect(enabled);
}

void SettingsViewModel::checkForUpdates()
{
    if (m_backend)
        m_backend->checkForUpdates(frontendVersion(), true);
    else
        LogHelper::logWarning(QStringLiteral("设置后端服务不可用，无法检查更新"), "Settings");
}

void SettingsViewModel::checkForUpdatesOnStartup()
{
    if (!m_autoCheckUpdates)
        return;
    if (m_backend)
        m_backend->checkForUpdates(frontendVersion(), false);
}

bool SettingsViewModel::setAutoStart(bool enabled)
{
    if (m_autoStart == enabled)
        return true;

    if (!m_autoStartService.setEnabled(enabled)) {
        LogHelper::logWarning(QStringLiteral("开机自启动状态%1失败，保持原状态")
                                  .arg(enabled ? QStringLiteral("开启") : QStringLiteral("关闭")),
                              "Settings");
        return false;
    }

    m_autoStart = enabled;
    emit autoStartChanged();
    save();

    LogHelper::logInfo(QStringLiteral("开机自启动已%1")
        .arg(enabled ? QStringLiteral("开启") : QStringLiteral("关闭")), "Settings");
    return true;
}

bool SettingsViewModel::isAutoStartEnabled() const
{
    return m_autoStartService.isEnabled();
}

int SettingsViewModel::logLevel() const
{
    return m_logLevel;
}

void SettingsViewModel::setLogLevel(int value)
{
    if (value < 0 || value > 3)
        value = 1;
    if (m_logLevel == value)
        return;
    m_logLevel = value;
    emit logLevelChanged();
    save();
}

int SettingsViewModel::maxLogEntries() const
{
    return m_maxLogEntries;
}

void SettingsViewModel::setMaxLogEntries(int value)
{
    if (value < 1)
        value = 1;
    if (value > 1000)
        value = 1000;
    if (m_maxLogEntries == value)
        return;
    m_maxLogEntries = value;
    emit maxLogEntriesChanged();
    save();
}

QString SettingsViewModel::frontendVersion() const
{
    return QStringLiteral(QTET_FRONTEND_VERSION);
}

QString SettingsViewModel::easyTierVersion() const
{
    return QStringLiteral(QTET_EASYTIER_VERSION);
}

SettingsStore::Settings SettingsViewModel::settings() const
{
    SettingsStore::Settings settings;
    settings.autoStart = m_autoStart;
    settings.autoCheckUpdates = m_autoCheckUpdates;
    settings.showExitPrompt = m_showExitPrompt;
    settings.hideServerNodes = m_hideServerNodes;
    settings.logLevel = m_logLevel;
    settings.maxLogEntries = m_maxLogEntries;
    return settings;
}

void SettingsViewModel::applySettings(const SettingsStore::Settings &settings)
{
    const SettingsStore::Settings normalizedSettings = SettingsStore::normalized(settings);
    m_autoStart = normalizedSettings.autoStart;
    m_autoCheckUpdates = normalizedSettings.autoCheckUpdates;
    m_showExitPrompt = normalizedSettings.showExitPrompt;
    m_hideServerNodes = normalizedSettings.hideServerNodes;
    m_logLevel = normalizedSettings.logLevel;
    m_maxLogEntries = normalizedSettings.maxLogEntries;
}
