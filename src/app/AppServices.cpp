/**
 * @file AppServices.cpp
 * @brief AppServices 实现
 *
 * 实现应用启动时的服务对象装配：
 * - 按依赖顺序创建基础设施 → 数据层 → ViewModel 层的服务对象
 * - wireLogging() 连线日志分发器、存储槽和设置 ViewModel
 * - wireRuntime() 连线 VPN 管理器、应用状态和配置列表之间的信号
 */
#include "AppServices.h"

#include "core/application/config/ConfigCommandService.h"
#include "core/application/config/ConfigImportExportService.h"
#include "core/application/logging/RepositoryLogSink.h"
#include "core/log/LogDispatcher.h"
#include "core/repository/LogRepository.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include "core/service/DaemonClient.h"
#include "core/system_tray/SystemTrayManager.h"
#include "core/system_tray/TrayMessageHelper.h"
#include "core/util/DaemonRegisterHelper.h"
#include "core/util/FontHelper.h"
#include "core/util/UpdateCheckService.h"
#include "core/viewmodel/AppState.h"
#include "core/viewmodel/ConfigEditorViewModel.h"
#include "core/viewmodel/ConfigListModel.h"
#include "core/viewmodel/FavoriteNodeViewModel.h"
#include "core/viewmodel/LogViewModel.h"
#include "core/viewmodel/SettingsViewModel.h"
#include "core/viewmodel/nodes/ImportNodesViewModel.h"
#include "core/viewmodel/runtime/BackendStatusViewModel.h"
#include "core/viewmodel/runtime/NetworkPageViewModel.h"
#include "core/vpn_manager/StatusMonitor.h"
#include "core/vpn_manager/VpnManager.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QUrl>

AppServices::AppServices(const QSqlDatabase &database,
                         QQmlApplicationEngine *engine,
                         DaemonConnectionMode daemonConnectionMode,
                         QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    QObject *parentObject = serviceParent();

    // ===== 基础设施层：daemon IPC 客户端与 API =====
    m_daemonClient = new DaemonClient(parentObject);
    if (daemonConnectionMode == ConnectToDaemon)
        m_daemonClient->connectToDaemon(QStringLiteral("qtet-daemon.sock"));
    m_daemonApi = new DaemonApi(m_daemonClient, parentObject);
    m_backendStatusViewModel = new BackendStatusViewModel(m_daemonClient, parentObject);

    // ===== 应用基础服务：状态、设置、字体 =====
    m_appState = new AppState(parentObject);
    m_updateCheckService = new UpdateCheckService(parentObject);
    m_settingsViewModel = new SettingsViewModel(m_daemonApi, m_updateCheckService, parentObject);
    m_fontHelper = new FontHelper(parentObject);
    m_systemTrayManager = new SystemTrayManager(parentObject);
    // 托盘需要立即显示 daemon 初始状态，后续变化在 wireRuntime() 中持续同步。
    m_systemTrayManager->setDaemonConnectionState(m_daemonClient->connectionState());
    wireNotifications();

    // daemon 连接建立后查询自动回连后端状态
    QObject::connect(m_daemonClient, &DaemonClient::connectionStateChanged,
                     m_settingsViewModel, [this](DaemonClient::ConnectionState state) {
                         if (state == DaemonClient::ConnectionState::Connected)
                             m_settingsViewModel->refreshAutoReconnect();
                     });

    // 托盘退出请求 → 由 AppServices 编排退出提示策略
    QObject::connect(m_systemTrayManager, &SystemTrayManager::quitRequestedByUser,
                     this, &AppServices::handleUserQuitRequest);

    // ===== 数据层与 ViewModel 层（依赖有效数据库连接） =====
    if (database.isValid()) {
        m_configRepository = new NetworkConfigRepository(database, parentObject);
        m_logRepository = new LogRepository(database, parentObject);
        m_favoriteNodeViewModel = new FavoriteNodeViewModel(database, parentObject);
        m_importNodesViewModel = new ImportNodesViewModel(m_favoriteNodeViewModel,
                                                          QUrl(QStringLiteral("qrc:/publicservers.json")),
                                                          parentObject);
        wireFavoriteNodeNotifications();
        m_logViewModel = new LogViewModel(m_logRepository, parentObject);
        m_repositoryLogSink = new RepositoryLogSink(m_logRepository, parentObject);
        m_statusMonitor = new StatusMonitor(parentObject);
        m_vpnManager = new VpnManager(m_daemonClient, m_daemonApi, m_configRepository, m_statusMonitor, parentObject);
        m_configCommandService = new ConfigCommandService(m_configRepository, parentObject);
        m_configImportExportService = new ConfigImportExportService(m_configRepository, m_daemonApi, parentObject);
        m_configListModel = new ConfigListModel(m_configRepository, m_daemonApi, m_configCommandService,
                                                m_configImportExportService, parentObject);
        m_configEditorViewModel = new ConfigEditorViewModel(m_configRepository, parentObject);
        m_networkPageViewModel = new NetworkPageViewModel(m_configListModel, m_configEditorViewModel,
                                                          m_vpnManager, m_backendStatusViewModel,
                                                          parentObject);

        // 连线日志和运行时信号
        wireLogging();
        wireRuntime();
    }

    if (daemonConnectionMode == ConnectToDaemon)
        m_settingsViewModel->checkForUpdatesOnStartup();
}

AppState *AppServices::appState() const { return m_appState; }
SettingsViewModel *AppServices::settingsViewModel() const { return m_settingsViewModel; }
FavoriteNodeViewModel *AppServices::favoriteNodeViewModel() const { return m_favoriteNodeViewModel; }
LogViewModel *AppServices::logViewModel() const { return m_logViewModel; }
FontHelper *AppServices::fontHelper() const { return m_fontHelper; }
ConfigListModel *AppServices::configListModel() const { return m_configListModel; }
ConfigEditorViewModel *AppServices::configEditorViewModel() const { return m_configEditorViewModel; }
NetworkPageViewModel *AppServices::networkPageViewModel() const { return m_networkPageViewModel; }
BackendStatusViewModel *AppServices::backendStatusViewModel() const { return m_backendStatusViewModel; }
ImportNodesViewModel *AppServices::importNodesViewModel() const { return m_importNodesViewModel; }
VpnManager *AppServices::vpnManager() const { return m_vpnManager; }
DaemonClient *AppServices::daemonClient() const { return m_daemonClient; }
DaemonApi *AppServices::daemonApi() const { return m_daemonApi; }
SystemTrayManager *AppServices::systemTrayManager() const { return m_systemTrayManager; }

QObject *AppServices::serviceParent() const
{
    // 优先使用 QML 引擎作为父对象，确保服务对象生命周期与 QML 引擎绑定
    return m_engine ? static_cast<QObject *>(m_engine) : const_cast<AppServices *>(this);
}

void AppServices::wireLogging()
{
    if (!m_repositoryLogSink || !m_settingsViewModel)
        return;

    // 从设置中读取初始值，配置日志槽
    m_repositoryLogSink->setMaxEntries(m_settingsViewModel->maxLogEntries());
    auto *logDispatcher = LogDispatcher::instance();
    logDispatcher->clearSinks();
    logDispatcher->setMinimumLevel(static_cast<LogLevel>(m_settingsViewModel->logLevel()));
    logDispatcher->addSink(m_repositoryLogSink);

    // 设置变更时动态调整日志级别和最大条目数
    QObject::connect(m_settingsViewModel, &SettingsViewModel::logLevelChanged,
                     logDispatcher, [logDispatcher, this]() {
                         logDispatcher->setMinimumLevel(static_cast<LogLevel>(m_settingsViewModel->logLevel()));
                     });
    QObject::connect(m_settingsViewModel, &SettingsViewModel::maxLogEntriesChanged,
                     m_repositoryLogSink, [this]() {
                         m_repositoryLogSink->setMaxEntries(m_settingsViewModel->maxLogEntries());
                     });
}

void AppServices::wireNotifications()
{
    if (!m_appState)
        return;

    QObject::connect(m_appState, &AppState::errorOccurred,
                     this, [](const QString &message) {
                         TrayMessageHelper::showError(QStringLiteral("错误"), message);
                     });
}

void AppServices::wireFavoriteNodeNotifications()
{
    if (!m_favoriteNodeViewModel)
        return;

    QObject::connect(m_favoriteNodeViewModel, &FavoriteNodeViewModel::importCompleted,
                     this, [](int importedCount, int skippedCount) {
                         TrayMessageHelper::showInfo(
                             QStringLiteral("节点导入完成"),
                             QStringLiteral("已导入 %1 个节点，跳过 %2 个节点").arg(importedCount).arg(skippedCount));
                     });
    QObject::connect(m_favoriteNodeViewModel, &FavoriteNodeViewModel::exportCompleted,
                     this, []() {
                         TrayMessageHelper::showInfo(QStringLiteral("节点导出完成"),
                                                     QStringLiteral("收藏节点已导出"));
                     });
}

void AppServices::wireRuntime()
{
    if (!m_vpnManager || !m_appState || !m_configListModel)
        return;

    // 设置页的服务节点隐藏开关只影响运行状态 UI 展示，不影响 controller 内部缓存。
    if (m_settingsViewModel) {
        m_vpnManager->setHideServerNodes(m_settingsViewModel->hideServerNodes());
        QObject::connect(m_settingsViewModel, &SettingsViewModel::hideServerNodesChanged,
                         m_vpnManager, [this]() {
                             m_vpnManager->setHideServerNodes(m_settingsViewModel->hideServerNodes());
                         });
    }

    // daemon 状态变化 → 更新托盘菜单状态与红色/普通/绿色图标优先级。
    QObject::connect(m_daemonClient, &DaemonClient::connectionStateChanged,
                     m_systemTrayManager, &SystemTrayManager::setDaemonConnectionState);
    QObject::connect(m_daemonClient, &DaemonClient::connectionStateChanged,
                     this, [this](DaemonClient::ConnectionState state) {
                         if (state == DaemonClient::ConnectionState::Disconnected)
                             ensureDaemonServiceOnce();
                     });
    // VPN 配置运行状态变化 → 托盘只统计 Running 状态的网络连接数量。
    QObject::connect(m_vpnManager, &VpnManager::configStateChanged,
                     m_systemTrayManager, &SystemTrayManager::setConfigRunState);
    // VPN 停止失败 → 显示全局错误消息
    QObject::connect(m_vpnManager, &VpnManager::stopFailed, m_appState, &AppState::showError);
    // 配置列表请求停止配置 → 调用 VpnManager 执行停止
    QObject::connect(m_configListModel, &ConfigListModel::requestStopConfig,
                     m_vpnManager, &VpnManager::stopConfig);
    // 配置被删除 → 通知 VpnManager 清理对应的 controller
    QObject::connect(m_configListModel, &ConfigListModel::configDeleted,
                     m_vpnManager, &VpnManager::cleanupController);
    // VPN 状态变更 → 同步更新配置列表的显示状态
    QObject::connect(m_vpnManager, &VpnManager::configStateChanged,
                     m_configListModel, &ConfigListModel::onRunningStateChanged);
}

void AppServices::ensureDaemonServiceOnce()
{
    if (m_daemonServiceEnsureAttempted)
        return;
    m_daemonServiceEnsureAttempted = true;

    const auto action = DaemonRegisterHelper::requiredAction();
    if (action == DaemonRegisterHelper::RequiredAction::None ||
        action == DaemonRegisterHelper::RequiredAction::UnsupportedPlatform) {
        return;
    }

    if (action == DaemonRegisterHelper::RequiredAction::DaemonBinaryMissing) {
#if defined(Q_OS_WIN)
        const QString missingText = QStringLiteral("未找到可执行的后端程序或服务安装器：\n%1\n%2\n\n应用无法连接后端，即将退出。")
                                        .arg(DaemonRegisterHelper::daemonBinaryPath(),
                                             DaemonRegisterHelper::serviceInstallerPath());
#else
        const QString missingText = QStringLiteral("未找到可执行的后端程序 qtet-daemon：\n%1\n\n应用无法连接后端，即将退出。")
                                        .arg(DaemonRegisterHelper::daemonBinaryPath());
#endif
        QMessageBox::critical(nullptr,
                              QStringLiteral("后端程序缺失"),
                              missingText);
        QCoreApplication::quit();
        return;
    }

    QString text;
    if (action == DaemonRegisterHelper::RequiredAction::RegisterService) {
#if defined(Q_OS_WIN)
        text = QStringLiteral("后端服务尚未注册。\n\nQtEasyTier 需要将当前程序目录下的 qtet-daemon.exe 注册为 Windows 服务：\n%1\n\n点击“是”后将通过 UAC 请求管理员权限，依次执行 DaemonInstaller.exe install 和 DaemonInstaller.exe start。\n点击“否”将退出程序。")
                   .arg(DaemonRegisterHelper::daemonBinaryPath());
#else
        text = QStringLiteral("后端服务尚未注册。\n\nQtEasyTier 需要将当前程序目录下的 qtet-daemon 注册为系统服务：\n%1\n\n点击“是”后将通过 pkexec 请求管理员权限并注册、启动 qtet-daemon.service。\n点击“否”将退出程序。")
                   .arg(DaemonRegisterHelper::daemonBinaryPath());
#endif
    } else {
#if defined(Q_OS_WIN)
        text = QStringLiteral("后端服务尚未运行。\n\n检测到 qtet-daemon Windows 服务已注册，但 qtet-daemon 进程未运行。\n\n点击“是”后将通过 UAC 请求管理员权限并执行 DaemonInstaller.exe start。\n点击“否”将退出程序。");
#else
        text = QStringLiteral("后端服务尚未运行。\n\n检测到 qtet-daemon.service 已注册，但 qtet-daemon 进程未运行。\n\n点击“是”后将通过 pkexec 请求管理员权限并启动 qtet-daemon.service。\n点击“否”将退出程序。");
#endif
    }

    const auto answer = QMessageBox::question(nullptr,
                                              QStringLiteral("需要后端服务"),
                                              text,
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::Yes);
    if (answer != QMessageBox::Yes) {
        QCoreApplication::quit();
        return;
    }

    const auto result = DaemonRegisterHelper::ensureDaemonService();
    if (result == DaemonRegisterHelper::EnsureResult::RegisterFailed ||
        result == DaemonRegisterHelper::EnsureResult::StartFailed) {
#if defined(Q_OS_WIN)
        const QString failureText = QStringLiteral("无法注册或启动 qtet-daemon Windows 服务，应用即将退出。");
#else
        const QString failureText = QStringLiteral("无法注册或启动 qtet-daemon.service，应用即将退出。");
#endif
        QMessageBox::critical(nullptr,
                              QStringLiteral("后端服务启动失败"),
                              failureText);
        QCoreApplication::quit();
    }
}

void AppServices::setExitPromptHandler(ExitPromptHandler handler)
{
    m_exitPromptHandler = std::move(handler);
}

void AppServices::handleUserQuitRequest()
{
    if (!m_systemTrayManager)
        return;

    if (!m_settingsViewModel || !m_settingsViewModel->showExitPrompt()) {
        m_systemTrayManager->quitApplication();
        return;
    }

    const ExitPromptResult result = m_exitPromptHandler
        ? m_exitPromptHandler()
        : showExitPromptDialog();

    if (!result.confirmed)
        return;

    if (result.dontShowAgain)
        m_settingsViewModel->setShowExitPrompt(false);

    m_systemTrayManager->quitApplication();
}

AppServices::ExitPromptResult AppServices::showExitPromptDialog()
{
    QMessageBox box;
    box.setWindowTitle(QStringLiteral("退出前端程序"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("退出 QtEasyTier 前端程序不会结束正在运行的后端 VPN 实例。"));
    box.setInformativeText(QStringLiteral("如需断开 VPN，请先在网络页面手动点击「停止」按钮。"));

    auto *checkBox = new QCheckBox(QStringLiteral("不再显示此提示"), &box);
    box.setCheckBox(checkBox);

    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);
    box.button(QMessageBox::Ok)->setText(QStringLiteral("退出程序"));
    box.button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));

    const auto result = box.exec();

    return ExitPromptResult{
        result == QMessageBox::Ok,
        checkBox->isChecked()
    };
}
