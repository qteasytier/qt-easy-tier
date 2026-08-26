/**
 * @file DangerousOperationService.cpp
 * @brief DangerousOperationService 实现
 *
 * 实现后端安装/卸载与全量数据清空的编排流程。
 * 设计要点：
 * - 仅依赖数据层、VPN 管理器与平台工具，不重置任何 ViewModel/Model 的内存状态，
 *   因为清空成功后应用会直接退出，进程结束即是最彻底的清理。
 * - 清空成功后通过 quitRequested() 信号请求退出，由 AppServices 连接执行，
 *   便于测试（测试不连接该信号即可观察清空结果）。
 */
#include "DangerousOperationService.h"

#include "core/repository/FavoriteNodeRepository.h"
#include "core/repository/LogRepository.h"
#include "core/repository/NetworkConfigRepository.h"
#include "platform/DaemonRegisterHelper.h"
#include "core/log/LogHelper.h"
#include "core/vpn_manager/VpnManager.h"

DangerousOperationService::DangerousOperationService(VpnManager *vpnManager,
                                                     NetworkConfigRepository *configRepository,
                                                     FavoriteNodeRepository *favoriteRepository,
                                                     LogRepository *logRepository,
                                                     const QString &settingsFilePath,
                                                     QObject *parent)
    : QObject(parent)
    , m_vpnManager(vpnManager)
    , m_configRepository(configRepository)
    , m_favoriteRepository(favoriteRepository)
    , m_logRepository(logRepository)
    , m_store(settingsFilePath.isEmpty() ? SettingsStore::defaultFilePath() : settingsFilePath)
{
}

bool DangerousOperationService::busy() const
{
    return m_busy;
}

bool DangerousOperationService::daemonInstalled() const
{
    return m_daemonInstalled;
}

bool DangerousOperationService::daemonOperationEnabled() const
{
    return m_daemonOperationEnabled;
}

void DangerousOperationService::refreshDaemonStatus()
{
    // 检测后端状态并缓存，避免 QML 绑定重复执行平台检测
    const auto action = DaemonRegisterHelper::requiredAction();
    m_daemonInstalled = (action == DaemonRegisterHelper::RequiredAction::None);
    m_daemonOperationEnabled = (action != DaemonRegisterHelper::RequiredAction::DaemonBinaryMissing
                                && action != DaemonRegisterHelper::RequiredAction::UnsupportedPlatform);
    emit daemonStatusChanged();
}

void DangerousOperationService::performDaemonOperation()
{
    if (m_busy)
        return;

    setBusy(true);

    bool success = false;
    if (daemonInstalled()) {
        // 已注册且运行中 → 停止并卸载
        success = DaemonRegisterHelper::uninstallDaemonService();
    } else {
        // 未注册 → 安装并启动；已注册未运行 → 仅启动
        const auto result = DaemonRegisterHelper::ensureDaemonService();
        success = (result == DaemonRegisterHelper::EnsureResult::AlreadyRunning
                   || result == DaemonRegisterHelper::EnsureResult::RegisteredAndStartRequested
                   || result == DaemonRegisterHelper::EnsureResult::StartRequested);
    }

    // 先按操作前状态组织失败文案，再刷新状态（刷新后 daemonInstalled 可能已变化）
    const QString failMessage = success ? QString()
        : (daemonInstalled()
               ? QStringLiteral("停止并卸载后端服务失败，请检查管理员权限后重试")
               : QStringLiteral("安装或启动后端服务失败，请检查管理员权限后重试"));

    refreshDaemonStatus();
    setBusy(false);
    emit operationFinished(success, failMessage);
}

void DangerousOperationService::clearAllData()
{
    if (m_busy)
        return;

    LogHelper::logInfo("危险操作: 请求清空全部数据", "DangerousOperation");
    setBusy(true);

    // 先停止所有正在运行的网络服务，全部成功收敛后才允许清空
    connect(m_vpnManager, &VpnManager::allStopped,
            this, &DangerousOperationService::onAllStopped, Qt::UniqueConnection);
    m_vpnManager->stopAll();
}

void DangerousOperationService::onAllStopped(bool success)
{
    if (!success) {
        LogHelper::logWarning("危险操作: 停止网络服务失败，已取消清空", "DangerousOperation");
        setBusy(false);
        emit operationFinished(false, QStringLiteral("停止网络服务失败，已取消清空，请稍后重试"));
        return;
    }

    performClear();
}

void DangerousOperationService::performClear()
{
    const bool configsOk = m_configRepository->clearAll();
    const bool favoritesOk = m_favoriteRepository->clear();
    const bool logsOk = m_logRepository->clearAll();
    // 设置文件写回默认值（不更新内存状态，应用即将退出）
    const bool settingsOk = m_store.save(SettingsStore::Settings{});

    if (!configsOk || !favoritesOk || !logsOk || !settingsOk) {
        LogHelper::logError("危险操作: 清空数据失败", "DangerousOperation");
        setBusy(false);
        emit operationFinished(false, QStringLiteral("清空数据失败，请检查存储权限后重试"));
        return;
    }

    LogHelper::logInfo("危险操作: 全部数据已清空，请求退出应用", "DangerousOperation");
    setBusy(false);
    emit operationFinished(true, QString());
    emit quitRequested();
}

void DangerousOperationService::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}
