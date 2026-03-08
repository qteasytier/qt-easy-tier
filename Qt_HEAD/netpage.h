#ifndef NETPAGE_H
#define NETPAGE_H

#include "easytierworker.h"
#include "generateconf.h"  // 包含 StartMode 枚举定义

#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QDir>
#include <QTableWidget>
#include <QLabel>
#include <QJsonObject>
#include <QCompleter>
#include <QStringListModel>
#include <QDialog>
#include <QProgressBar>
#include <QThread>
#include <QFile>
#include <QTextStream>

QT_BEGIN_NAMESPACE
namespace Ui {
    class NetPage;
}
QT_END_NAMESPACE

// 前向声明
class EasyTierWorker;

/**
 * @brief 配置文件来源枚举类
 */
enum class ConfigSource {
    Input,          // 下方输入
    SelectFile      // 选择文件
};

class NetPage : public QGroupBox
{
    Q_OBJECT

public:
    explicit NetPage(QWidget *parent = nullptr);
    ~NetPage();

// ===============基础设置===============

    // 获取网络设置参数
    QString getUsername() const;
    QString getNetworkName() const;
    QString getPassword() const;
    QString getIpAddress() const;
    bool isDhcpEnabled() const;

    // 获取低延迟优先和私有模式设置
    bool isLowLatencyPriority() const;
    bool isPrivateMode() const;

    // 获取已添加的服务器列表
    QStringList getServerList() const;

// ===============高级设置===============

    // 获取高级设置参数
    bool isKcpProxyEnabled() const;  // 是否启用 KCP 代理
    bool isQuicInputDisabled() const;  // 是否禁用 QUIC 输入
    bool isTunModeDisabled() const;  // 是否禁用 TUN 模式
    bool isMultithreadEnabled() const;  // 是否启用多线程
    bool isUdpHolePunchingDisabled() const;  // 是否禁用 UDP 打洞
    bool isUserModeStackEnabled() const;  // 是否启用用户模式栈
    bool isKcpInputDisabled() const;  // 是否禁用 KCP 输入
    bool isP2pDisabled() const;  // 是否禁用 P2P 连接
    bool isExitNodeEnabled() const;  // 是否启用出口节点
    bool isSystemForwardingEnabled() const;  // 是否启用系统转发
    bool isSymmetricNatHolePunchingDisabled() const;  // 是否禁用对称 NAT 端口映射
    bool isIpv6Disabled() const;  // 是否禁用 IPv6
    bool isQuicProxyEnabled() const;  // 是否启用 QUIC 代理
    bool isBindDevice() const;  // 是否仅使用物理网卡
    bool isRpcPacketForwardingEnabled() const;  // 是否启用 RPC 包转发
    bool isEncryptionDisabled() const;  // 是否禁用加密
    bool isMagicDnsEnabled() const;  // 是否启用 Magic DNS

    // 获取监听地址列表
    QStringList getListenAddrList() const;
    // 获取子网代理CIDR列表
    QStringList getCidrList() const;
    // 获取网络白名单列表
    QStringList getRelayNetworkWhitelist() const;
    // 获取网络白名单启用状态
    bool isRelayNetworkWhitelistEnabled() const;
    // 获取RPC端口号
    int getRpcPort() const;

    // =====================其他设置相关====================
    // 是否自动配置RPC端口（配置文件模式）
    bool isAutoRpcEnabled() const;

    // ===============配置保存和加载===============
    QJsonObject getNetworkConfig() const;  // 获取当前网络配置
    void setNetworkConfig(const QJsonObject &config);  // 设置网络配置

    // ===============运行与检测相关===============
    int realRpcPort = 0;  // 实际的RPC端口号，运行Et前赋值，用于检测运行状态
    bool isRunning() const;
    void runNetworkOnAutoStart();  // 运行网络（开机自启）
    QString getLogFilePath() const { return m_currentLogFileName; }  // 获取当前日志文件路径

private slots:
    // ===============网络设置相关===============
    // DHCP复选框状态变化处理
    void onDhcpStateChanged(Qt::CheckState state) const;
    // 密码可见性切换
    void onTogglePasswordVisibility() const;
    // 添加服务器
    void onAddServer();
    // 删除服务器
    void onRemoveServer() const;
    // 服务器地址补全数据更新
    void onServerEditCompleterChanged() const;
    // 打开公共服务器列表
    void onOpenPublicServerList();
    // 白名单开启按钮状态变化处理
    void onRelayNetworkWhitelistStateChanged(Qt::CheckState state) const;
    // 添加网络白名单
    void onAddRelayNetworkWhitelist();
    // 删除网络白名单
    void onRemoveRelayNetworkWhitelist();
    // 添加监听地址
    void onAddListenAddr();
    // 删除监听地址
    void onRemoveListenAddr();
    // 监听地址补全数据更新
    void onListenAddrEditCompleterChanged();
    // 添加子网代理CIDR
    void onAddCidr();
    // 删除子网代理CIDR
    void onRemoveCidr();
    // RPC端口输入验证
    void onRpcPortTextChanged(const QString &text);
    // CIDR 计算器按钮被点击时的处理
    void onClickCidrCalculator();

    // ===============运行状态页面相关===============
    // 运行网络
    void onRunNetwork();
    // 停止网络
    void onStopNetwork();

    // ===============EasyTierWorker信号处理===============
    // 处理进程启动结果
    void onWorkerProcessStarted(bool success, const QString& errorMessage);
    // 处理进程停止结果
    void onWorkerProcessStopped(bool success);
    // 处理节点信息更新
    void onWorkerPeerInfoUpdated(const QJsonArray& peers);
    // 处理进程崩溃
    void onWorkerProcessCrashed(int exitCode);

    // ===============配置导入导出相关===============
    void onImportConfigClicked();
    void onExportConfigClicked();

    // ===============日志相关===============
    void onOpenLogFileClicked();  // 打开日志文件
    void onWorkerLogOutput(const QString& logText, bool isError);  // 处理来自 Worker 的日志输出
    void onHandleLogs(const QString& logText, bool isError = false, bool addTimestamp = true);  // 统一日志处理函数
    // ===============其他设置页面相关===============
    void onUseWebBoxChanged(int state);
    void onUseConfFileBoxChanged(int state);
    void onConnectToLocalBoxChanged(int state);
    void onConfSourceBoxChanged(int index);
    void onSelectConfigFileClicked();
    // 配置文件文本编辑框内容变化，动态调整高度
    void onConfigTextChanged();
    // 自动配置RPC端口复选框状态变化
    void onAutoRpcBoxChanged(int state);

    // ===============列表项编辑相关===============
    // 服务器列表：双击编辑
    void onServerListItemDoubleClicked(QListWidgetItem *item);
    // 子网代理列表：双击编辑
    void onCidrListItemDoubleClicked(QListWidgetItem *item);
    // 网络白名单列表：双击编辑
    void onWhitelistItemDoubleClicked(QListWidgetItem *item);
    // 监听地址列表：双击编辑
    void onListenAddrItemDoubleClicked(QListWidgetItem *item);

private:
    Ui::NetPage *ui;

    // 界面组件
    QLineEdit *m_usernameEdit =  nullptr;
    QLineEdit *m_networkNameEdit =  nullptr;
    QLineEdit *m_passwordEdit =  nullptr;
    QPushButton *m_togglePasswordBtn =  nullptr;
    QCheckBox *m_dhcpCheckBox =  nullptr;
    QLineEdit *m_ipEdit =  nullptr;

    // 低延迟优先和私有模式选项
    QCheckBox *m_lowLatencyCheckBox =  nullptr;
    QCheckBox *m_privateModeCheckBox =  nullptr;

    // 服务器管理组件
    QLineEdit *m_serverEdit =  nullptr;
    QPushButton *m_addServerBtn =  nullptr;
    QListWidget *m_serverListWidget =  nullptr;
    QPushButton *m_removeServerBtn =  nullptr;
    QCompleter *m_serverEditCompleter =  nullptr;        // 用于服务器地址补全
    QStringListModel *m_serverListEditModel =  nullptr;  // 补全器的模型
    QPushButton *m_publicServerBtn =  nullptr;
    QPushButton *m_serverHelpBtn =  nullptr;

    // 高级设置组件
    QCheckBox *m_kcpProxyCheckBox =  nullptr;        // 启用 KCP 代理
    QCheckBox *m_quicInputDisableCheckBox =  nullptr; // 禁用 QUIC 输入
    QCheckBox *m_noTunModeCheckBox =  nullptr;       // 无 TUN 模式
    QCheckBox *m_multithreadCheckBox =  nullptr;     // 启用多线程
    QCheckBox *m_udpHolePunchingDisableCheckBox =  nullptr; // 禁用 UDP 打洞
    QCheckBox *m_userModeStackCheckBox =  nullptr;   // 使用用户态协议栈
    QCheckBox *m_kcpInputDisableCheckBox =  nullptr; // 禁用 KCP 输入
    QCheckBox *m_p2pDisableCheckBox =  nullptr;      // 禁用 P2P
    QCheckBox *m_exitNodeCheckBox =  nullptr;        // 启用出口节点
    QCheckBox *m_systemForwardingCheckBox =  nullptr; // 系统转发
    QCheckBox *m_symmetricNatHolePunchingDisableCheckBox =  nullptr; // 禁用对称 NAT 打洞
    QCheckBox *m_ipv6DisableCheckBox =  nullptr;     // 禁用 IPv6
    QCheckBox *m_quicProxyCheckBox =  nullptr;       // 启用 QUIC 代理
    QCheckBox *m_onlyPhysicalNicCheckBox =  nullptr; // 仅使用物理网卡
    QCheckBox *m_rpcPacketForwardingCheckBox =  nullptr; // 转发 RPC 包
    QCheckBox *m_encryptionDisableCheckBox =  nullptr; // 禁用加密
    QCheckBox *m_magicDnsCheckBox =  nullptr;        // 启用魔法 DNS

    // RPC端口输入框
    QLineEdit *m_rpcPortEdit =  nullptr;

    // ===============其他设置页面相关===============
    // Web 控制台管理组件
    QLineEdit *m_webConnectAddrEdit = nullptr;   // Web 控制台连接地址输入框

    // 配置文件启动相关组件
    QPlainTextEdit *m_configTextEdit = nullptr;      // 配置文件文本编辑框
    QLineEdit *m_configFilePathEdit = nullptr;       // 配置文件路径输入框
    QPushButton *m_selectConfigFileBtn = nullptr;    // 选择配置文件按钮
    QCheckBox *m_autoRpcCheckBox = nullptr;          // 自动配置 RPC 端口复选框
    QLineEdit *m_confRpcPortEdit = nullptr;          // 配置文件模式的 RPC 端口输入框

    // 启动方式管理
    StartMode m_startMode = StartMode::Normal;       // 当前启动方式
    ConfigSource m_configSource = ConfigSource::Input; // 配置文件来源
    QString m_tempConfigFilePath;                     // 临时配置文件路径

    // 网络白名单管理组件
    QCheckBox *m_relayNetworkWhitelistCheckBox =  nullptr; // 是否启用网络白名单
    QLineEdit *m_relayNetworkWhitelistEdit =  nullptr; // 网络白名单输入框
    QPushButton *m_addRelayNetworkWhitelistBtn =  nullptr; // 添加按钮
    QListWidget *m_relayNetworkWhitelistListWidget =  nullptr; // 白名单列表
    QPushButton *m_removeRelayNetworkWhitelistBtn =  nullptr; // 删除按钮

    // 监听地址管理组件
    QLineEdit *m_listenAddrEdit =  nullptr;
    QPushButton *m_addListenAddrBtn =  nullptr;
    QListWidget *m_listenAddrListWidget =  nullptr;
    QPushButton *m_removeListenAddrBtn =  nullptr;
    QCompleter *m_listenAddrEditCompleter =  nullptr;        // 用于监听地址补全
    QStringListModel *m_listenAddrListEditModel =  nullptr;  // 补全器的模型

    // 子网代理CIDR管理组件
    QLineEdit *m_cidrEdit =  nullptr;
    QPushButton *m_addCidrBtn =  nullptr;
    QListWidget *m_cidrListWidget =  nullptr;
    QPushButton *m_removeCidrBtn =  nullptr;
    QPushButton *m_calculateCidrBtn =  nullptr;

    // 运行状态页面相关
    QPlainTextEdit *m_logTextEdit =  nullptr;
    QLabel *m_runningStatusLabel =  nullptr;
    QTableWidget *m_peerTable =  nullptr;
    QCheckBox *m_isHideServersBox =  nullptr;  //是否隐藏服务器的信息
    QPushButton *m_openLogFileBtn =  nullptr;
    QPushButton *m_openLogDirBtn =  nullptr;

    // 日志文件相关
    QFile* m_logFile = nullptr;
    QTextStream* m_logStream = nullptr;
    QString m_currentLogFileName;
    int m_logLineCount = 0;

    // EasyTierWorker相关（线程和工作对象）
    QThread* m_workerThread = nullptr;
    EasyTierWorker* m_worker = nullptr;

    // 启动过程对话框相关
    QDialog* m_processDialog = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_progressLabel = nullptr;

// ============== 网络设置相关===============

    // 初始化网络设置界面（基本设置）
    void initNetworkSettings();
    // 创建滚动区
    void createScrollArea();
    // 初始化服务器管理界面
    void initServerManagement();
    // 初始化高级设置界面
    void initAdvancedSettings();
    // 创建高级设置页面
    void createAdvancedSetPage();
    // 初始化网络白名单管理界面
    void initRelayNetworkWhitelistManagement();
    // 初始化监听地址管理界面
    void initListenAddrManagement();
    // 初始化子网代理CIDR管理界面
    void initCidrManagement();
    // 初始化运行状态页面
    void initRunningStatePage();

// ============== 运行状态相关===============

    // 更新UI状态
    void updateUIState(bool isRunning);
    // 显示启动过程对话框（无限进度条）
    void showProcessDialog();
    // 关闭启动过程对话框
    void closeProcessDialog();
    // 限制日志行数
    void limitLogLines(int maxLines = MAX_LOG_LINES);
    // 初始化运行日志窗口
    void initRunningLogWindow();
    // 初始化Worker线程
    void initWorkerThread();
    // 清理Worker线程
    void cleanupWorkerThread();

// ============== 日志文件管理相关===============
    // 初始化日志文件
    bool initLogFile(const QString& networkName);
    // 关闭日志文件
    void closeLogFile();
    // 保存日志到文件
    void saveLogToFile(const QString& text, bool addTimestamp = false);

// ============== 其他设置页面相关===============
    // 初始化其他设置页面
    void initOtherSettingsPage();
    // 更新启动方式互斥状态
    void updateStartModeState();
    // 更新配置文件来源显示
    void updateConfigSourceUI();
    // 创建临时配置文件
    bool createTempConfigFile(const QString& networkName);
    // 清理临时配置文件
    void cleanupTempConfigFile();
    // 获取默认配置文件内容
    QString getDefaultConfigContent() const;

signals:
    void networkStarted();    // 网络启动信号
    void networkFinished();   // 网络停止信号
};

#endif // NETPAGE_H
