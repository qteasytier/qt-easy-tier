#ifndef NETPAGE_H
#define NETPAGE_H

#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QDir>
#include <QTableWidget>
#include <QLabel>
#include <QTimer>
#include <QJsonObject>  // 添加JSON支持

QT_BEGIN_NAMESPACE
namespace Ui {
    class NetPage;
}
QT_END_NAMESPACE

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
    // 获取RPC端口号
    int getRpcPort() const;

    // ===============配置保存和加载===============
    QJsonObject getNetworkConfig() const;  // 获取当前网络配置
    void setNetworkConfig(const QJsonObject &config);  // 设置网络配置

    // ===============运行检测相关===============
    int realRpcPort;  // 实际的RPC端口号，运行Et前赋值，用于检测运行状态

private slots:
    // ===============网络设置相关===============
    // DHCP复选框状态变化处理
    void onDhcpStateChanged(Qt::CheckState state);
    // 密码可见性切换
    void onTogglePasswordVisibility();
    // 添加服务器
    void onAddServer();
    // 删除服务器
    void onRemoveServer();
    // 添加监听地址
    void onAddListenAddr();
    // 删除监听地址
    void onRemoveListenAddr();
    // 添加子网代理CIDR
    void onAddCidr();
    // 删除子网代理CIDR
    void onRemoveCidr();
    // RPC端口输入验证
    void onRpcPortTextChanged(const QString &text);

    // ===============运行状态页面相关===============
    // 初始化运行日志窗口
    void initRunningLogWindow();
    // 运行网络
    void onRunNetwork();
    // 进程输出处理
    void onProcessOutputReady();
    // 进程错误处理
    void onProcessErrorReady();
    // 进程完成处理
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::NetPage *ui;

    // 界面组件
    QLineEdit *m_usernameEdit;
    QLineEdit *m_networkNameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_togglePasswordBtn;
    QCheckBox *m_dhcpCheckBox;
    QLineEdit *m_ipEdit;

    // 低延迟优先和私有模式选项
    QCheckBox *m_lowLatencyCheckBox;
    QCheckBox *m_privateModeCheckBox;

    // 服务器管理组件
    QLineEdit *m_serverEdit;
    QPushButton *m_addServerBtn;
    QListWidget *m_serverListWidget;
    QPushButton *m_removeServerBtn;

    // 高级设置组件
    QCheckBox *m_kcpProxyCheckBox;        // 启用 KCP 代理
    QCheckBox *m_quicInputDisableCheckBox; // 禁用 QUIC 输入
    QCheckBox *m_noTunModeCheckBox;       // 无 TUN 模式
    QCheckBox *m_multithreadCheckBox;     // 启用多线程
    QCheckBox *m_udpHolePunchingDisableCheckBox; // 禁用 UDP 打洞
    QCheckBox *m_userModeStackCheckBox;   // 使用用户态协议栈
    QCheckBox *m_kcpInputDisableCheckBox; // 禁用 KCP 输入
    QCheckBox *m_p2pDisableCheckBox;      // 禁用 P2P
    QCheckBox *m_exitNodeCheckBox;        // 启用出口节点
    QCheckBox *m_systemForwardingCheckBox; // 系统转发
    QCheckBox *m_symmetricNatHolePunchingDisableCheckBox; // 禁用对称 NAT 打洞
    QCheckBox *m_ipv6DisableCheckBox;     // 禁用 IPv6
    QCheckBox *m_quicProxyCheckBox;       // 启用 QUIC 代理
    QCheckBox *m_onlyPhysicalNicCheckBox; // 仅使用物理网卡
    QCheckBox *m_rpcPacketForwardingCheckBox; // 转发 RPC 包
    QCheckBox *m_encryptionDisableCheckBox; // 禁用加密
    QCheckBox *m_magicDnsCheckBox;        // 启用魔法 DNS

    // RPC端口输入框
    QLineEdit *m_rpcPortEdit;

    // 监听地址管理组件
    QLineEdit *m_listenAddrEdit;
    QPushButton *m_addListenAddrBtn;
    QListWidget *m_listenAddrListWidget;
    QPushButton *m_removeListenAddrBtn;

    // 子网代理CIDR管理组件
    QLineEdit *m_cidrEdit;
    QPushButton *m_addCidrBtn;
    QListWidget *m_cidrListWidget;
    QPushButton *m_removeCidrBtn;

    // 运行et相关
    QPlainTextEdit *m_logTextEdit;
    QProcess *m_easytierProcess;
    bool m_isRunning;      // 运行状态跟踪

    // 运行状态页面相关
    QProcess *m_asyncProcess;     // 执行cli异步获取节点信息
    QLabel *m_runningStatusLabel;
    QTableWidget *m_peerTable;
    QTimer *m_peerUpdateTimer;

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
    // 初始化监听地址管理界面
    void initListenAddrManagement();
    // 初始化子网代理CIDR管理界面
    void initCidrManagement();
    // 重置运行按钮状态
    void resetStateDisplay();

    // 初始化运行状态页面
    void initRunningStatePage();
    // 更新节点信息
    void updatePeerInfo();
    // 解析并显示节点信息
    void parseAndDisplayPeerInfo(const QByteArray &jsonData);

};

#endif // NETPAGE_H