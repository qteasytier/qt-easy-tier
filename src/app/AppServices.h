/**
 * @file AppServices.h
 * @brief 应用服务装配层，创建并持有所有核心服务对象
 *
 * AppServices 是应用启动时的服务对象工厂和依赖注入容器：
 * - 按依赖顺序创建所有服务、ViewModel、Repository 对象
 * - 以 QQmlApplicationEngine 作为父对象，统一管理生命周期
 * - 通过 wireLogging() / wireRuntime() 完成信号-槽连线
 * - 通过访问器方法供 QmlSingletonRegistrar 注册到 QML 上下文
 *
 * ## 创建的服务对象分类
 * - 基础设施层：DaemonClient、DaemonApi、StatusMonitor、FontHelper
 * - 数据层：NetworkConfigRepository、LogRepository、PublicServerProvider
 * - ViewModel 层：各种 ViewModel 和 Model（供 QML 绑定）
 * - VPN 管理层：VpnManager（启停控制器）
 * - 配置管理层：ConfigCommandService、ConfigImportExportService
 *
 * @see QmlSingletonRegistrar
 */
#pragma once

#include <QObject>
#include <QSqlDatabase>

#include <functional>

class AppState;
class BackendStatusViewModel;
class ConfigCommandService;
class ConfigEditorViewModel;
class ConfigImportExportService;
class ConfigListModel;
class DaemonApi;
class DaemonClient;
class FavoriteNodeViewModel;
class FontHelper;
class ImportNodesViewModel;
class LogRepository;
class LogViewModel;
class NetworkConfigRepository;
class NetworkPageViewModel;
class PublicServerProvider;
class QQmlApplicationEngine;
class RepositoryLogSink;
class SettingsViewModel;
class StatusMonitor;
class SystemTrayManager;
class VpnManager;

/** @brief 应用服务装配容器，负责所有核心服务对象的创建、连线与生命周期管理 */
class AppServices : public QObject {
    Q_OBJECT

public:
    /** @brief 退出提示弹窗返回结果 */
    struct ExitPromptResult {
        bool confirmed = false;     ///< 用户是否点击了退出
        bool dontShowAgain = false; ///< 是否勾选了"不再显示此提示"
    };

    /** @brief 退出提示处理函数类型，可替换用于测试 */
    using ExitPromptHandler = std::function<ExitPromptResult()>;

public:
    /** @brief daemon 连接模式枚举 */
    enum DaemonConnectionMode {
        ConnectToDaemon,     ///< 启动时连接 daemon
        SkipDaemonConnection ///< 跳过 daemon 连接（测试场景用）
    };

    /**
     * @brief 构造函数，按依赖顺序创建所有服务对象
     * @param database             SQLite 数据库连接（isValid() 为 false 时不创建数据相关服务）
     * @param engine               QQmlApplicationEngine，作为服务对象的父级
     * @param daemonConnectionMode daemon 连接策略
     * @param parent               父对象
     */
    explicit AppServices(const QSqlDatabase &database,
                         QQmlApplicationEngine *engine,
                         DaemonConnectionMode daemonConnectionMode = ConnectToDaemon,
                         QObject *parent = nullptr);

    // ===== 服务对象访问器 =====

    /// 获取应用全局状态（消息通知、当前页面等）
    AppState *appState() const;
    /// 获取设置 ViewModel（日志级别、自启动等）
    SettingsViewModel *settingsViewModel() const;
    /// 获取收藏节点 ViewModel
    FavoriteNodeViewModel *favoriteNodeViewModel() const;
    /// 获取日志 ViewModel
    LogViewModel *logViewModel() const;
    /// 获取字体辅助工具（QML 中文字体支持）
    FontHelper *fontHelper() const;
    /// 获取配置列表 Model（网络配置的管理界面数据源）
    ConfigListModel *configListModel() const;
    /// 获取配置编辑器 ViewModel
    ConfigEditorViewModel *configEditorViewModel() const;
    /// 获取网络页面 ViewModel（配置启停、节点信息、日志综合视图）
    NetworkPageViewModel *networkPageViewModel() const;
    /// 获取后端状态 ViewModel（daemon 连接状态）
    BackendStatusViewModel *backendStatusViewModel() const;
    /// 获取导入节点 ViewModel（从公共服务器导入配置）
    ImportNodesViewModel *importNodesViewModel() const;
    /// 获取 VPN 管理器（启停控制、运行状态、心跳同步）
    VpnManager *vpnManager() const;
    /// 获取 daemon IPC 客户端
    DaemonClient *daemonClient() const;
    /// 获取 daemon API（JSON-RPC 调用封装）
    DaemonApi *daemonApi() const;
    /// 获取系统托盘管理器（窗口显隐、托盘菜单、托盘消息输出）
    SystemTrayManager *systemTrayManager() const;

    /** @brief 设置退出提示处理函数（仅用于测试） */
    void setExitPromptHandler(ExitPromptHandler handler);

private:
    /// 返回服务对象的父级（优先使用 QML 引擎，确保生命周期由引擎管理）
    QObject *serviceParent() const;
    /// 连线日志相关信号与槽（LogDispatcher ↔ RepositoryLogSink ↔ SettingsViewModel）
    void wireLogging();
    /// 连线运行时信号与槽（VpnManager ↔ AppState ↔ ConfigListModel）
    void wireRuntime();
    /// daemon 断开时尝试确保系统服务已注册并启动（每次应用生命周期最多一次）
    void ensureDaemonServiceOnce();

    /// 处理托盘菜单用户退出请求：按设置决定是否显示退出提示。
    void handleUserQuitRequest();

    /// 显示退出提示弹窗，返回用户选择结果。
    ExitPromptResult showExitPromptDialog();

    QQmlApplicationEngine *m_engine = nullptr;
    NetworkConfigRepository *m_configRepository = nullptr;
    LogRepository *m_logRepository = nullptr;
    DaemonClient *m_daemonClient = nullptr;
    DaemonApi *m_daemonApi = nullptr;
    BackendStatusViewModel *m_backendStatusViewModel = nullptr;
    AppState *m_appState = nullptr;
    SettingsViewModel *m_settingsViewModel = nullptr;
    FavoriteNodeViewModel *m_favoriteNodeViewModel = nullptr;
    PublicServerProvider *m_publicServerProvider = nullptr;
    ImportNodesViewModel *m_importNodesViewModel = nullptr;
    LogViewModel *m_logViewModel = nullptr;
    RepositoryLogSink *m_repositoryLogSink = nullptr;
    StatusMonitor *m_statusMonitor = nullptr;
    VpnManager *m_vpnManager = nullptr;
    ConfigCommandService *m_configCommandService = nullptr;
    ConfigImportExportService *m_configImportExportService = nullptr;
    ConfigListModel *m_configListModel = nullptr;
    ConfigEditorViewModel *m_configEditorViewModel = nullptr;
    NetworkPageViewModel *m_networkPageViewModel = nullptr;
    FontHelper *m_fontHelper = nullptr;
    SystemTrayManager *m_systemTrayManager = nullptr;
    bool m_daemonServiceEnsureAttempted = false;
    ExitPromptHandler m_exitPromptHandler;
};
