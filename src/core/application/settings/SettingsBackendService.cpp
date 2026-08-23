/**
 * @file SettingsBackendService.cpp
 * @brief SettingsBackendService 实现
 *
 * 自动回连与更新检查均为异步 RPC / HTTP 请求，
 * 通过 QFutureWatcher 等待结果并在主线程回调中更新状态、发射信号。
 */
#include "SettingsBackendService.h"

#include "core/service/DaemonApi.h"
#include "core/util/LogHelper.h"
#include "core/util/UpdateCheckService.h"

#include <QFutureWatcher>
#include <QJsonObject>

SettingsBackendService::SettingsBackendService(DaemonApi *daemonApi,
                                               UpdateCheckService *updateCheckService,
                                               QObject *parent)
    : QObject(parent)
    , m_daemonApi(daemonApi)
    , m_updateCheckService(updateCheckService)
{
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

bool SettingsBackendService::autoReconnect() const
{
    return m_autoReconnect;
}

bool SettingsBackendService::autoReconnectBusy() const
{
    return m_autoReconnectBusy;
}

bool SettingsBackendService::updateCheckBusy() const
{
    return m_updateCheckBusy;
}

void SettingsBackendService::refreshAutoReconnect()
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

void SettingsBackendService::setAutoReconnect(bool enabled)
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

void SettingsBackendService::checkForUpdates(const QString &frontendVersion, bool manual)
{
    if (!m_updateCheckService) {
        LogHelper::logWarning(QStringLiteral("UpdateCheckService 不可用，无法检查更新"), "Settings");
        return;
    }

    if (m_updateCheckBusy)
        return;

    setUpdateCheckBusy(true);
    m_updateCheckService->checkLatestRelease(frontendVersion, manual);
}

void SettingsBackendService::setAutoReconnectBusy(bool busy)
{
    if (m_autoReconnectBusy == busy)
        return;
    m_autoReconnectBusy = busy;
    emit autoReconnectBusyChanged();
}

void SettingsBackendService::setUpdateCheckBusy(bool busy)
{
    if (m_updateCheckBusy == busy)
        return;
    m_updateCheckBusy = busy;
    emit updateCheckBusyChanged();
}
