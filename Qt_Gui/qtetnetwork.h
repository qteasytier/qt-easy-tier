#ifndef QTETNETWORK_H
#define QTETNETWORK_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QFormLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QComboBox>

#include "qtetlistwidget.h"
#include "qtetcheckbtn.h"

/// @brief 网络配置页面
/// 提供网络列表和配置选项卡的界面
class QtETNetwork : public QWidget
{
    Q_OBJECT

public:
    explicit QtETNetwork(QWidget *parent = nullptr);
    ~QtETNetwork() override;

private:
    /// @brief 初始化界面
    void initUI();
    /// @brief 初始化左侧网络列表区域
    void initLeftPanel();
    /// @brief 初始化右侧选项卡区域
    void initRightPanel();
    /// @brief 初始化基础设置页面
    void initBasicSettingsPage();
    /// @brief 初始化高级设置页面
    void initAdvancedSettingsPage();
    /// @brief 初始化运行状态页面
    void initRunningStatusPage();
    /// @brief 初始化运行日志页面
    void initRunningLogPage();

private:
    // 左侧面板
    QFrame *m_leftFrame;                ///< 左侧面板容器
    QVBoxLayout *m_leftLayout;          ///< 左侧布局
    QtETListWidget *m_networksList;     ///< 网络列表
    QPushButton *m_newNetworkBtn;       ///< 新建网络按钮
    QPushButton *m_runNetworkBtn;       ///< 运行网络按钮
    QPushButton *m_importConfBtn;       ///< 导入配置按钮
    QPushButton *m_exportConfBtn;       ///< 导出配置按钮

    // 右侧面板
    QTabWidget *m_tabWidget;            ///< 选项卡容器
    QWidget *m_basicSettingsTab;        ///< 基础设置选项卡
    QWidget *m_advancedSettingsTab;     ///< 高级设置选项卡
    QWidget *m_runningStatusTab;        ///< 运行状态选项卡
    QWidget *m_runningLogTab;           ///< 运行日志选项卡

    // 基础设置控件
    QLineEdit *m_usernameEdit;          ///< 用户名输入框
    QLineEdit *m_networkNameEdit;       ///< 网络号输入框
    QLineEdit *m_passwordEdit;          ///< 密码输入框
    QPushButton *m_togglePasswordBtn;   ///< 密码显示/隐藏按钮
    QtETCheckBtn *m_dhcpCheckBox;       ///< DHCP 开关
    QLineEdit *m_ipEdit;                ///< IPv4 地址输入框
    QtETCheckBtn *m_lowLatencyCheckBox; ///< 低延迟优先开关
    QtETCheckBtn *m_privateModeCheckBox;///< 私有模式开关
    QLineEdit *m_serverEdit;            ///< 服务器地址输入框
    QPushButton *m_addServerBtn;        ///< 添加服务器按钮
    QListWidget *m_serverListWidget;    ///< 服务器列表
    QPushButton *m_removeServerBtn;     ///< 删除服务器按钮
    QPushButton *m_publicServerBtn;     ///< 公共服务器列表按钮

    // 高级设置控件 - 功能开关
    QtETCheckBtn *m_kcpProxyCheckBox;           ///< 启用 KCP 代理
    QtETCheckBtn *m_kcpInputDisableCheckBox;    ///< 禁用 KCP 输入
    QtETCheckBtn *m_noTunModeCheckBox;          ///< 无 TUN 模式
    QtETCheckBtn *m_quicProxyCheckBox;          ///< 启用 QUIC 代理
    QtETCheckBtn *m_quicInputDisableCheckBox;   ///< 禁用 QUIC 输入
    QtETCheckBtn *m_udpHolePunchingDisableCheckBox; ///< 禁用 UDP 打洞
    QtETCheckBtn *m_multithreadCheckBox;        ///< 启用多线程
    QtETCheckBtn *m_userModeStackCheckBox;      ///< 使用用户态协议栈
    QtETCheckBtn *m_onlyPhysicalNicCheckBox;    ///< 仅使用物理网卡
    QtETCheckBtn *m_p2pDisableCheckBox;         ///< 禁用 P2P
    QtETCheckBtn *m_exitNodeCheckBox;           ///< 启用出口节点
    QtETCheckBtn *m_systemForwardingCheckBox;   ///< 系统转发
    QtETCheckBtn *m_symmetricNatHolePunchingDisableCheckBox; ///< 禁用对称 NAT 打洞
    QtETCheckBtn *m_ipv6DisableCheckBox;        ///< 禁用 IPv6
    QtETCheckBtn *m_rpcPacketForwardingCheckBox;///< 转发 RPC 包
    QtETCheckBtn *m_encryptionDisableCheckBox;  ///< 禁用加密
    QtETCheckBtn *m_magicDnsCheckBox;           ///< 启用魔法 DNS

    // 高级设置控件 - RPC 端口
    QLineEdit *m_rpcPortEdit;           ///< RPC 端口输入框

    // 高级设置控件 - 网络白名单
    QtETCheckBtn *m_relayNetworkWhitelistCheckBox;  ///< 启用网络白名单
    QLineEdit *m_relayNetworkWhitelistEdit;         ///< 网络白名单输入框
    QPushButton *m_addWhitelistBtn;                  ///< 添加白名单按钮
    QListWidget *m_whitelistListWidget;              ///< 白名单列表
    QPushButton *m_removeWhitelistBtn;               ///< 删除白名单按钮

    // 高级设置控件 - 监听地址
    QLineEdit *m_listenAddrEdit;        ///< 监听地址输入框
    QPushButton *m_addListenAddrBtn;    ///< 添加监听地址按钮
    QListWidget *m_listenAddrListWidget;///< 监听地址列表
    QPushButton *m_removeListenAddrBtn; ///< 删除监听地址按钮

    // 高级设置控件 - 子网代理 CIDR
    QLineEdit *m_cidrEdit;              ///< 子网代理 CIDR 输入框
    QPushButton *m_addCidrBtn;          ///< 添加 CIDR 按钮
    QListWidget *m_cidrListWidget;      ///< CIDR 列表
    QPushButton *m_removeCidrBtn;       ///< 删除 CIDR 按钮
    QPushButton *m_calculateCidrBtn;    ///< 打开 CIDR 计算器按钮

    // 运行状态控件
    QLabel *m_statusLabel;              ///< 状态标签

    // 运行日志控件
    QLabel *m_logLabel;                 ///< 日志标签

    // 主布局
    QHBoxLayout *m_mainLayout;          ///< 主布局
};

#endif // QTETNETWORK_H
