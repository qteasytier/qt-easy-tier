#ifndef SETTING_H
#define SETTING_H

#include <QDialog>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QApplication>
#include <QNetworkAccessManager>
#include "generateconf.h"  // 包含 WebConsoleConfig 定义

namespace Ui {
class setting;
}

/**
 * @brief 版本检测工作类
 * 
 * 在独立线程中执行版本检测，封装所有 QProcess 操作。
 * 符合 Qt 标准：工作对象在创建后 moveToThread 到目标线程运行。
 * 
 * 使用流程：
 * 1. 创建 worker 实例（在主线程）
 * 2. moveToThread 到工作线程
 * 3. 连接 started 信号到 startDetection 槽
 * 4. 启动线程，自动开始检测
 */
class VersionDetectionWorker : public QObject
{
    Q_OBJECT

public:
    explicit VersionDetectionWorker(QObject *parent = nullptr);
    ~VersionDetectionWorker() override;

    /// @brief 设置可执行文件目录（可选，默认自动检测）
    void setExecutableDir(const QString &dir) { m_executableDir = dir; }

public slots:
    /// @brief 开始检测版本（线程入口点）
    void startDetection();

signals:
    /// @brief 核心版本检测完成
    void coreVersionReady(const QString &version);
    /// @brief CLI版本检测完成
    void cliVersionReady(const QString &version);
    /// @brief Web控制台版本检测完成
    void webVersionReady(const QString &version);
    /// @brief 所有检测完成
    void detectionFinished();

private slots:
    /// @brief 处理进程输出
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    /// @brief 检测单个可执行文件的版本
    void detectSingleExecutable(const QString &executablePath, const QString &type);

    /// @brief 解析版本输出
    QString parseVersionOutput(const QString &output, const QString &type);

    /// @brief 自动查找可执行文件目录
    QString findExecutableDir() const;

    // 可执行文件目录（若为空则自动检测）
    QString m_executableDir;

    // 当前正在检测的类型（"core", "cli", "web"）
    QString m_currentDetectType;

    // 进程对象（作为成员变量，确保正确的线程亲和性）
    QProcess *m_process = nullptr;

    // 检测状态
    bool m_coreDetected = false;
    bool m_cliDetected = false;
    bool m_webDetected = false;
};

// =============================================================================

/**
 * @brief 设置对话框类
 * 
 * 管理应用程序设置，包括：
 * - 开机自启、自动回连、隐藏到托盘、自动检查更新
 * - 软件版本检测与显示
 * - 检查更新功能
 * - 启动次数统计与赞助弹窗控制
 * - Web 控制台配置
 */
class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings() override;

    /**
 * @brief Web 控制台配置结构体
 */
    struct WebConsoleConfig {
        int configPort = 55668;           // 配置下发端口
        int webPagePort = 55667;          // 控制台前端端口
        bool useLocalApi = true;          // 是否使用本地 API 地址
        QString apiAddress;               // 控制台 API 地址
        QString configProtocol = "udp";   // 配置下发协议（udp/tcp/ws）

        /// @brief 获取协议字符串（用于命令行参数）
        [[nodiscard]] QString getConfigProtocolString() const {
            if (configProtocol == "TCP") return "tcp";
            if (configProtocol == "WebSocket") return "ws";
            return "udp";
        }

        /// @brief 获取实际的 API 地址
        [[nodiscard]] QString getActualApiAddress() const {
            if (useLocalApi) {
                return QString("http://127.0.0.1:%1").arg(webPagePort);
            }
            return apiAddress;
        }
    };

    /// @brief 获取 Web 控制台配置（直接从配置文件读取）
    static WebConsoleConfig getWebConsoleConfig();

    /// @brief 检测软件版本
    /// @param parent 父窗口指针
    /// @param isNotMessageBox 是否不显示消息框
    static void detectSoftwareVersion(QWidget *parent = nullptr, bool isNotMessageBox = false);

    /// @brief 获取自动回连状态（直接从配置文件读取）
    static bool isAutoRun();

    /// @brief 是否隐藏到系统托盘（直接从配置文件读取）
    static bool isHideOnTray();

    /// @brief 是否自动检查更新（直接从配置文件读取）
    static bool isAutoCheckUpdate();

    /// @brief 获取配置保存路径
    static QString getConfigPath()
    {
        // 配置保存路径
#if SAVE_CONF_IN_APP_DIR == true
        return QApplication::applicationDirPath()+"/config";
        #else
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#endif
    }

    /// @brief 是否应该弹出赞助窗口（直接从配置文件读取并更新启动次数）
    /// @return 当启动次数达到阈值时返回 true
    static bool shouldShowDonate();

    /// @brief 清理过期日志文件
    /// @return 清理的文件数量
    static int cleanupOldLogs();

    /// @brief 清空所有日志文件
    /// @return 清理的文件数量
    static int clearAllLogs();

    /// @brief 获取日志文件夹路径
    static QString getLogDirPath();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    // === UI 事件处理 ===
    void onRedetectButtonClicked();
    void onRedetectWebButtonClicked();
    void onOpenFolderButtonClicked();
    void onCheckUpdateButtonClicked();
    void onDialogAccepted();
    void onDialogRejected();
    void onUseLocalApiChanged(int state);
    void onOpenLogDirClicked();
    void onClearLogClicked();

    // === 版本检测结果处理 ===
    void onCoreVersionReady(const QString &version);
    void onCliVersionReady(const QString &version);
    void onWebVersionReady(const QString &version);
    void onVersionDetectionFinished();


private:
    // === 初始化相关 ===
    void setupUi();
    void setupWebConfigUi();
    void connectSignals();

    // === 版本检测线程管理 ===
    void startVersionDetection();
    void cleanupVersionThread();

    // === 设置读写 ===
    void loadSettings();
    void saveSettings();

    // === 静态配置文件读写 ===
    /// @brief 从配置文件读取设置（供静态方法使用）
    static QJsonObject loadSettingsFromFile();
    /// @brief 保存设置到配置文件（供静态方法使用）
    static bool saveSettingsToFile(const QJsonObject &settings);

    // === 设置开机自启 ===
    void setAutoStart(bool enable);
    
    // === 输入验证 ===
    void setupPortValidation();
    bool validatePortInput(const QString &text, int minPort = 1, int maxPort = 65535);

private:
    Ui::setting *ui;

    // === 设置项 ===
    bool m_autoRun = false;       // 自动回连
    bool m_autoStart = false;     // 开机自启
    bool m_isHideOnTray = true;   // 隐藏到系统托盘
    bool m_autoUpdate = true;     // 自动检查更新
    int m_logRetentionDays = 7;   // 日志保存天数，默认7天
    QString m_softwareVer = PROJECT_VERSION;

    // === 启动计数相关 ===
    int m_launchCount = 0;        // 启动次数
    static constexpr int DONATE_THRESHOLD = 50; // 弹出赞助窗口的启动次数阈值

    // === Web 控制台配置 ===
    WebConsoleConfig m_webConfig;

    // === 版本检测线程相关 ===
    QThread *m_versionThread = nullptr;
    VersionDetectionWorker *m_versionWorker = nullptr;
    bool m_versionDetecting = false; // 防止重复检测
};

#endif // SETTING_H