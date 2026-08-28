/**
 * @file AppServices.cpp
 * @brief AppServices 实现
 *
 * 实现应用启动时的服务对象装配：
 * - 按依赖顺序创建基础设施 → 数据层 → ViewModel 层的服务对象
 * - wireLogging() 连线日志分发器、存储槽和设置 ViewModel
 * - wireRuntime() 连线 VPN 运行服务、应用状态和配置列表之间的信号
 */
#include "AppServices.h"

#include "app_service/config/ConfigCommandService.h"
#include "app_service/config/ConfigImportExportService.h"
#include "app_service/credential/CredentialService.h"
#include "app_service/dangerous/DangerousOperationService.h"
#include "app_service/favorite/FavoriteNodeImportExportService.h"
#include "app_service/logging/RepositoryLogSink.h"
#include "app_service/runtime/VpnRuntimeService.h"
#include "app_service/settings/UpdateCheckService.h"
#include "core/log/LogDispatcher.h"
#include "core/repository/FavoriteNodeRepository.h"
#include "core/repository/LogRepository.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include "core/service/DaemonClient.h"
#include "core/system_tray/SystemTrayManager.h"
#include "core/system_tray/TrayMessageHelper.h"
#include "platform/DaemonRegisterHelper.h"
#include "platform/FontHelper.h"
#include "app_service/settings/UpdateCheckService.h"
#include "viewmodels/AppState.h"
#include "viewmodels/ConfigEditorViewModel.h"
#include "viewmodels/ConfigListModel.h"
#include "viewmodels/credential/CredentialViewModel.h"
#include "viewmodels/FavoriteNodeViewModel.h"
#include "viewmodels/LogViewModel.h"
#include "viewmodels/SettingsViewModel.h"
#include "viewmodels/nodes/ImportNodesViewModel.h"
#include "viewmodels/runtime/BackendStatusViewModel.h"
#include "viewmodels/runtime/NetworkPageViewModel.h"
#include "app_service/runtime/StatusMonitor.h"

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
    // 设置 ViewModel：直接协调本地设置、daemon 自动回连与版本更新检查
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
        m_favoriteNodeRepository = new FavoriteNodeRepository(database, parentObject);
        m_logRepository = new LogRepository(database, parentObject);
        m_favoriteNodeImportExportService = new FavoriteNodeImportExportService(m_favoriteNodeRepository, parentObject);
        m_favoriteNodeViewModel = new FavoriteNodeViewModel(m_favoriteNodeRepository,
                                                            m_favoriteNodeImportExportService,
                                                            parentObject);
        m_importNodesViewModel = new ImportNodesViewModel(m_favoriteNodeViewModel,
                                                          QUrl(QStringLiteral("qrc:/publicservers.json")),
                                                          parentObject);
        wireFavoriteNodeNotifications();
        m_logViewModel = new LogViewModel(m_logRepository, parentObject);
        m_repositoryLogSink = new RepositoryLogSink(m_logRepository, parentObject);
        m_statusMonitor = new StatusMonitor(parentObject);
        // VPN 运行服务：应用级 runtime 协调器，暴露运行状态展示模型并管理实例生命周期
        m_vpnRuntimeService = new VpnRuntimeService(m_daemonClient, m_daemonApi,
                                                    m_configRepository, m_statusMonitor,
                                                    parentObject);
        // 临时凭证服务：签发安全模式临时节点密钥（经 DaemonApi::callJsonRpc 调 daemon）
        m_credentialService = new CredentialService(m_daemonApi, parentObject);
        m_credentialViewModel = new CredentialViewModel(m_credentialService, parentObject);
        // 危险操作服务：编排后端安装/卸载与全量数据清空的跨基础服务流程
        m_dangerousOperationService = new DangerousOperationService(m_vpnRuntimeService,
                                                                    m_configRepository,
                                                                    m_favoriteNodeRepository,
                                                                    m_logRepository,
                                                                     QString(),
                                                                     parentObject);
        // 清空全部数据成功后退出应用（信号方式便于测试）
        QObject::connect(m_dangerousOperationService, &DangerousOperationService::quitRequested,
                         this, []() {
                             QCoreApplication::quit();
                         });
        m_configCommandService = new ConfigCommandService(m_configRepository, parentObject);
        m_configImportExportService = new ConfigImportExportService(m_configRepository, m_daemonApi, parentObject);
        m_configListModel = new ConfigListModel(m_configCommandService, m_configImportExportService, parentObject);
        m_configEditorViewModel = new ConfigEditorViewModel(m_configCommandService, parentObject);
        m_networkPageViewModel = new NetworkPageViewModel(m_configListModel, m_configEditorViewModel,
                                                          m_vpnRuntimeService, m_backendStatusViewModel,
                                                          parentObject);

        // 连线日志和运行时信号
        wireLogging();
        wireRuntime();
        wireConfigCoordination();
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
VpnRuntimeService *AppServices::vpnRuntimeService() const { return m_vpnRuntimeService; }
CredentialService *AppServices::credentialService() const { return m_credentialService; }
CredentialViewModel *AppServices::credentialViewModel() const { return m_credentialViewModel; }
DangerousOperationService *AppServices::dangerousOperationService() const { return m_dangerousOperationService; }
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
    if (!m_vpnRuntimeService || !m_appState || !m_configListModel || !m_networkPageViewModel)
        return;

    // 设置页的服务节点隐藏开关只影响运行状态 UI 展示，作用于 VPN 运行服务的节点信息模型。
    if (m_settingsViewModel) {
        m_vpnRuntimeService->setHideServerNodes(m_settingsViewModel->hideServerNodes());
        QObject::connect(m_settingsViewModel, &SettingsViewModel::hideServerNodesChanged,
                         m_vpnRuntimeService, [this]() {
                             m_vpnRuntimeService->setHideServerNodes(m_settingsViewModel->hideServerNodes());
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
    QObject::connect(m_vpnRuntimeService, &VpnRuntimeService::configStateChanged,
                     m_systemTrayManager, &SystemTrayManager::setConfigRunState);
    // VPN 停止失败 → 显示全局错误消息
    QObject::connect(m_vpnRuntimeService, &VpnRuntimeService::stopFailed,
                     m_appState, &AppState::showError);
    // 配置列表请求停止配置 → 经 VPN 运行服务执行停止
    QObject::connect(m_configListModel, &ConfigListModel::requestStopConfig,
                     m_vpnRuntimeService, &VpnRuntimeService::stopConfig);
    // 配置被删除 → 经 VPN 运行服务通知清理对应的 controller
    QObject::connect(m_configListModel, &ConfigListModel::configDeleted,
                     m_vpnRuntimeService, &VpnRuntimeService::cleanupController);
    // 配置被创建/导入 → 经 VPN 运行服务同步本地 controller（与数据库配置集合保持一致）
    QObject::connect(m_configListModel, &ConfigListModel::configCreated,
                     m_vpnRuntimeService, &VpnRuntimeService::ensureLocalController);
    // VPN 状态变更 → 同步更新配置列表的显示状态
    QObject::connect(m_vpnRuntimeService, &VpnRuntimeService::configStateChanged,
                     m_configListModel, &ConfigListModel::onRunningStateChanged);
    // VPN 状态变更 → 页面 ViewModel 刷新当前实例状态。
    // 连接顺序保证在 onRunningStateChanged 之后，确保读取到已更新的状态缓存。
    QObject::connect(m_vpnRuntimeService, &VpnRuntimeService::configStateChanged,
                     m_networkPageViewModel, &NetworkPageViewModel::refreshRunning);
    // 外部实例集合变化 → 配置列表末尾追加/移除外部实例条目
    QObject::connect(m_vpnRuntimeService, &VpnRuntimeService::externalInstancesChanged,
                     m_configListModel, &ConfigListModel::onExternalInstancesChanged);
    // 外部实例从 daemon 消失时，若恰为当前选中实例则清空选中，避免右侧残留失效实例
    QObject::connect(m_vpnRuntimeService, &VpnRuntimeService::externalInstancesChanged,
                     this, [this](const QStringList &instanceNames) {
                         const QString current = m_networkPageViewModel->currentInstanceName();
                         if (current.isEmpty())
                             return;
                         // 本地配置不受外部实例集合变化影响
                         if (m_configListModel->isLocalInstance(current))
                             return;
                         // 既非本地配置又不在新的外部实例列表中 → 当前选中的外部实例已消失，清空选中
                         if (!instanceNames.contains(current))
                             m_networkPageViewModel->clearSelection();
                     });
}

void AppServices::wireConfigCoordination()
{
    if (!m_configListModel || !m_configEditorViewModel || !m_networkPageViewModel)
        return;

    // 配置重命名成功后：同步编辑器共享快照的显示名称，
    // 避免用户随后修改其他字段触发完整保存时，把旧显示名覆盖回去。
    QObject::connect(m_configListModel, &ConfigListModel::configRenamed,
                     m_configEditorViewModel, &ConfigEditorViewModel::syncDisplayName);

    // 配置真正从仓库删除成功后（含运行中先停止后删除）：再清空页面选择。
    // 编辑器使用"丢弃式清空"，不刷写待保存修改，避免把已删除配置重新保存回仓库。
    QObject::connect(m_configListModel, &ConfigListModel::configDeleted,
                     m_networkPageViewModel, &NetworkPageViewModel::handleConfigDeleted);
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
        const QString missingText = QStringLiteral("未找到可执行的后端程序 qtet-daemon：\n%1\n\n应用无法连接后端，即将退出。")
                                        .arg(DaemonRegisterHelper::daemonBinaryPath());
        QMessageBox::critical(nullptr,
                              QStringLiteral("后端程序缺失"),
                              missingText);
        QCoreApplication::quit();
        return;
    }

    QString text;
    if (action == DaemonRegisterHelper::RequiredAction::RegisterService) {
#if defined(Q_OS_WIN)
        text = QStringLiteral("后端服务尚未注册。\n\nQtEasyTier 需要将当前程序目录下的 qtet-daemon.exe 注册为 Windows 服务：\n%1\n\n点击“是”后将通过 UAC 请求管理员权限，依次执行 qtet-daemon.exe --install 和 qtet-daemon.exe --start。\n点击“否”将退出程序。")
                   .arg(DaemonRegisterHelper::daemonBinaryPath());
#else
        text = QStringLiteral("后端服务尚未注册。\n\nQtEasyTier 需要将当前程序目录下的 qtet-daemon 注册为系统服务：\n%1\n\n点击“是”后将通过 pkexec 请求管理员权限并注册、启动 qtet-daemon.service。\n点击“否”将退出程序。")
                   .arg(DaemonRegisterHelper::daemonBinaryPath());
#endif
    } else {
#if defined(Q_OS_WIN)
        text = QStringLiteral("后端服务尚未运行。\n\n检测到 qtet-daemon Windows 服务已注册，但 qtet-daemon 进程未运行。\n\n点击“是”后将通过 UAC 请求管理员权限并执行 qtet-daemon.exe --start。\n点击“否”将退出程序。");
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
