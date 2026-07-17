/**
 * @file SettingsViewModel.cpp
 * @brief SettingsViewModel 实现
 */
#include "SettingsViewModel.h"
#include "AppVersion.h"
#include "core/service/DaemonApi.h"
#include "core/util/LogHelper.h"

SettingsViewModel::SettingsViewModel(DaemonApi *daemonApi, QObject *parent)
    : QObject(parent)
    , m_daemonApi(daemonApi)
{
    load();
}

bool SettingsViewModel::autoStart() const
{
    return m_autoStart;
}

bool SettingsViewModel::autoReconnect() const
{
    return m_autoReconnect;
}

bool SettingsViewModel::autoReconnectBusy() const
{
    return m_autoReconnectBusy;
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
    emit hideServerNodesChanged();
    emit showExitPromptChanged();
    emit logLevelChanged();
    emit maxLogEntriesChanged();
}

void SettingsViewModel::save()
{
    m_store.save(settings());
}

void SettingsViewModel::setBusy(bool busy)
{
    if (m_autoReconnectBusy == busy)
        return;
    m_autoReconnectBusy = busy;
    emit autoReconnectBusyChanged();
}

void SettingsViewModel::refreshAutoReconnect()
{
    if (!m_daemonApi) {
        LogHelper::logWarning(QStringLiteral("DaemonApi 不可用，无法查询自动回连状态"), "Settings");
        return;
    }

    setBusy(true);

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        setBusy(false);
        try {
            const QJsonObject result = watcher->result();
            const bool enabled = result.value(QStringLiteral("autoReconnect")).toBool(false);
            if (m_autoReconnect != enabled) {
                m_autoReconnect = enabled;
                emit autoReconnectChanged();
            }
        } catch (const QException &e) {
            LogHelper::logWarning(QStringLiteral("查询自动回连状态失败: %1").arg(e.what()), "Settings");
        }
        watcher->deleteLater();
    });

    watcher->setFuture(m_daemonApi->getAutoReconnect());
}

void SettingsViewModel::setAutoReconnectEnabled(bool enabled)
{
    if (!m_daemonApi) {
        LogHelper::logWarning(QStringLiteral("DaemonApi 不可用，无法设置自动回连"), "Settings");
        emit autoReconnectOperationFailed(QStringLiteral("后端未连接，无法设置自动回连"));
        return;
    }

    setBusy(true);

    const bool previous = m_autoReconnect;

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher, previous]() {
        setBusy(false);
        try {
            const QJsonObject result = watcher->result();
            const bool actual = result.value(QStringLiteral("autoReconnect")).toBool(false);
            if (m_autoReconnect != actual) {
                m_autoReconnect = actual;
                emit autoReconnectChanged();
            }
        } catch (const QException &e) {
            LogHelper::logWarning(QStringLiteral("设置自动回连失败: %1").arg(e.what()), "Settings");
            if (m_autoReconnect != previous) {
                m_autoReconnect = previous;
                emit autoReconnectChanged();
            }
            emit autoReconnectOperationFailed(QStringLiteral("设置自动回连失败: %1").arg(e.what()));
        }
        watcher->deleteLater();
    });

    watcher->setFuture(m_daemonApi->setAutoReconnect(enabled));
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
    m_showExitPrompt = normalizedSettings.showExitPrompt;
    m_hideServerNodes = normalizedSettings.hideServerNodes;
    m_logLevel = normalizedSettings.logLevel;
    m_maxLogEntries = normalizedSettings.maxLogEntries;
}
