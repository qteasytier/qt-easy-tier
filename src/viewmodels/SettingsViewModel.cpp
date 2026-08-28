/**
 * @file SettingsViewModel.cpp
 * @brief SettingsViewModel 实现
 *
 * 本地设置（settings3.json）由本类直接管理；开机自启以系统实际状态为唯一权威源，
 * 直接通过 AutoStartHelper 读写，不持久化到 JSON。
 * 自动回连通过注入的 DaemonApi 发起 RPC 并持有状态/忙状态；
 * 版本更新检查通过注入的 UpdateCheckService 发起，并监听其终态信号收敛忙状态。
 */
#include "SettingsViewModel.h"
#include "AppVersion.h"
#include "platform/AutoStartHelper.h"
#include "app_service/settings/UpdateCheckService.h"
#include "core/log/LogHelper.h"
#include "core/service/DaemonApi.h"

#include <QFutureWatcher>
#include <QJsonObject>

SettingsViewModel::SettingsViewModel(DaemonApi *daemonApi,
                                     UpdateCheckService *updateCheckService,
                                     QObject *parent)
    : QObject(parent)
    , m_daemonApi(daemonApi)
    , m_updateCheckService(updateCheckService)
{
    load();

    // 更新检查的任意结束信号都表示请求已收敛，解除忙状态
    if (m_updateCheckService) {
        connect(m_updateCheckService, &UpdateCheckService::updateCheckFailed,
                this, [this](const QString &message) {
                    setUpdateCheckBusy(false);
                    LogHelper::logWarning(message, "Settings");
                });
        connect(m_updateCheckService, &UpdateCheckService::noUpdateAvailable,
                this, [this](const QString &message) {
                    setUpdateCheckBusy(false);
                    LogHelper::logInfo(message, "Settings");
                });
        connect(m_updateCheckService, &UpdateCheckService::updateAvailable,
                this, [this](const UpdateCheckService::UpdateInfo &) {
                    setUpdateCheckBusy(false);
                });
        connect(m_updateCheckService, &UpdateCheckService::checkFinished,
                this, [this]() { setUpdateCheckBusy(false); });
    }
}

bool SettingsViewModel::autoStart() const
{
    // 以系统实际自启动状态为唯一权威源，每次读取实时查询
    return AutoStartHelper::isAutoStartEnabled();
}

bool SettingsViewModel::autoReconnect() const
{
    return m_autoReconnect;
}

bool SettingsViewModel::autoReconnectBusy() const
{
    return m_autoReconnectBusy;
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
    return m_updateCheckBusy;
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
    if (!m_daemonApi) {
        LogHelper::logWarning(QStringLiteral("DaemonApi 不可用，无法查询自动回连状态"), "Settings");
        return;
    }

    setAutoReconnectBusy(true);

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        setAutoReconnectBusy(false);
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

    setAutoReconnectBusy(true);

    const bool previous = m_autoReconnect;

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher, previous]() {
        setAutoReconnectBusy(false);
        try {
            // 以 daemon 返回的实际状态为准，而非用户传入值
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
    // 以系统实际状态为准：已处于目标状态则直接成功，不产生多余系统调用
    const bool before = AutoStartHelper::isAutoStartEnabled();
    if (before == enabled)
        return true;

    const bool operationOk = AutoStartHelper::setAutoStart(enabled);
    const bool after = AutoStartHelper::isAutoStartEnabled();

    // 系统实际状态发生变化时通知 QML 刷新
    if (after != before)
        emit autoStartChanged();

    // 操作失败或系统最终未达到目标状态，保持警告并返回失败
    if (!operationOk || after != enabled) {
        LogHelper::logWarning(QStringLiteral("开机自启动状态%1失败，保持原状态")
                                  .arg(enabled ? QStringLiteral("开启") : QStringLiteral("关闭")),
                              "Settings");
        return false;
    }

    LogHelper::logInfo(QStringLiteral("开机自启动已%1")
        .arg(enabled ? QStringLiteral("开启") : QStringLiteral("关闭")), "Settings");
    return true;
}

void SettingsViewModel::refreshAutoStart()
{
    // 重新发射属性通知，使 QML 读取系统真实自启动状态
    emit autoStartChanged();
}

void SettingsViewModel::checkForUpdates()
{
    checkForUpdatesInternal(true);
}

void SettingsViewModel::checkForUpdatesOnStartup()
{
    if (!m_autoCheckUpdates)
        return;
    checkForUpdatesInternal(false);
}

void SettingsViewModel::checkForUpdatesInternal(bool manual)
{
    if (!m_updateCheckService) {
        LogHelper::logWarning(QStringLiteral("UpdateCheckService 不可用，无法检查更新"), "Settings");
        return;
    }

    if (m_updateCheckBusy)
        return;

    setUpdateCheckBusy(true);
    m_updateCheckService->checkLatestRelease(frontendVersion(), manual);
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

void SettingsViewModel::setAutoReconnectBusy(bool busy)
{
    if (m_autoReconnectBusy == busy)
        return;
    m_autoReconnectBusy = busy;
    emit autoReconnectBusyChanged();
}

void SettingsViewModel::setUpdateCheckBusy(bool busy)
{
    if (m_updateCheckBusy == busy)
        return;
    m_updateCheckBusy = busy;
    emit updateCheckBusyChanged();
}

SettingsStore::Settings SettingsViewModel::settings() const
{
    SettingsStore::Settings settings;
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
    m_autoCheckUpdates = normalizedSettings.autoCheckUpdates;
    m_showExitPrompt = normalizedSettings.showExitPrompt;
    m_hideServerNodes = normalizedSettings.hideServerNodes;
    m_logLevel = normalizedSettings.logLevel;
    m_maxLogEntries = normalizedSettings.maxLogEntries;
}
