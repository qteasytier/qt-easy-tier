#ifndef NETPAGE_H
#define NETPAGE_H

#include "easytierworker.h"

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

QT_BEGIN_NAMESPACE
namespace Ui {
    class NetPage;
}
QT_END_NAMESPACE

// 前向声明
class EasyTierWorker;

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

    // ===============配置保存和加载===============
    QJsonObject getNetworkConfig() const;  // 获取当前网络配置
    void setNetworkConfig(const QJsonObject &config);  // 设置网络配置

    // ===============运行与检测相关===============
    int realRpcPort = 0;  // 实际的RPC端口号，运行Et前赋值，用于检测运行状态
    bool isRunning() const;
    void runNetworkOnAutoStart();  // 运行网络（开机自启）

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
    // 处理日志输出
    void onWorkerLogOutput(const QString& logText, bool isError);
    // 处理节点信息更新
    void onWorkerPeerInfoUpdated(const QJsonArray& peers);
    // 处理进程崩溃
    void onWorkerProcessCrashed(int exitCode);

    // ===============配置导入导出相关===============
    void onImportConfigClicked();
    void onExportConfigClicked();

    // ===============日志相关===============
    void onOpenLogFileClicked();  // 打开日志文件

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

    // 检查并准备EasyTier程序
    bool prepareEasyTierProgram(QString& appDir, QString& easytierPath);
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

signals:
    void networkStarted();    // 网络启动信号
    void networkFinished();   // 网络停止信号
};

#endif // NETPAGE_H
