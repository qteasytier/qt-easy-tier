#ifndef QTETNETWORK_H
#define QTETNETWORK_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QComboBox>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QAbstractButton>
#include <QFileDialog>
#include <QInputDialog>
#include <QThread>
#include <QProgressDialog>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextEdit>
#include <QProcess>
#include <QCoreApplication>
#include <QEventLoop>
#include <vector>

#include "qtetlabellist.h"
#include <QListWidget>
#include "qtetcheckbtn.h"
#include "qtetpushbtn.h"
#include "qtetcombobox.h"
#include "qtetnodeinfo.h"
#include "qtettabwidget.h"
#include "qtetlineedit.h"
#include "qtetresponsivegrid.h"
#include "networkconf.h"
#include "ETRunWorker.h"

/// @brief 网络配置页面
/// 提供网络列表和配置选项卡的界面
class QtETNetwork : public QWidget
{
    Q_OBJECT

    // 声明 NetworkConf 为友元，允许其访问私有成员
    friend class NetworkConf;

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

public:
    /// @brief 加载所有网络配置到UI
    /// @return 返回配置文件中 is_running 为 true 的网络索引列表（用于自动回连）
    QVector<int> loadAllNetworkConfs();
    /// @brief 保存所有网络配置到文件
    void saveAllNetworkConfs() const;
    /// @brief 根据索引启动网络（用于自动回连）
    /// @param index 网络配置索引
    void runNetworkByIndex(int index);

private:
    /// @brief 将指定索引的网络配置加载到UI
    /// @param index 网络配置索引
    void loadConfToUI(int index) const;
    /// @brief 从UI保存配置到指定索引的网络配置
    /// @param index 网络配置索引
    void saveConfFromUI(int index);
    /// @brief 更新 TabWidget 的启用状态
    void updateTabWidgetState() const;
    /// @brief 设置UI控件的信号连接
    void setupUIConnections();
    /// @brief 更新运行按钮的样式（根据当前选中的网络状态）
    void updateRunButtonStyle() const;
    /// @brief 更新网络列表项的样式
    /// @param index 网络配置索引
    void updateListItemStyle(int index) const;
    /// @brief 更新所有网络列表项的样式
    void updateAllListItemStyles() const;
    /// @brief 启动网络内部实现
    void onRunNetworkBtnClicked_Start(const NetworkConf &conf);
    /// @brief 停止网络内部实现
    void onRunNetworkBtnClicked_Stop(const NetworkConf &conf);
    /// @brief 解析网络信息JSON并返回节点列表
    static QVector<NodeInfo> parseNodeInfosFromJson(const QString &jsonStr);
    /// @brief 根据当前选中网络的 m_runningStatus 更新节点列表 UI
    void updateCurrentNetworkUI();
    /// @brief 解析运行日志JSON并增量更新到 m_runningLog
    static void parseAndUpdateRunningLogs(NetworkConf &conf, const QString &jsonStr);
    /// @brief 根据当前选中网络的 m_runningLog 更新日志 UI
    void updateCurrentNetworkLogUI();
    /// @brief 格式化单条日志条目
    static QString formatLogEntry(const QString &time, const QJsonObject &eventObj);
    /// @brief 将uint32_t IP地址转换为点分十进制字符串
    static QString ipAddrToString(quint32 addr);
    /// @brief 启动节点监测定时器
    void startNodeMonitor() const;
    /// @brief 停止节点监测定时器
    void stopNodeMonitor();
    /// @brief 获取网络显示名称（优先使用标签，否则使用"网络x"）
    /// @param index 网络配置索引
    /// @return 显示名称
    QString getNetworkDisplayName(int index) const;
    /// @brief 更新指定索引的列表项显示名称
    /// @param index 网络配置索引
    void updateListItemDisplayName(int index) const;
    /// @brief 同步停止网络并等待完成
    /// @param instName 实例名称
    /// @param timeoutMs 超时时间（毫秒）
    /// @return true 停止成功，false 超时或停止失败
    bool stopNetworkAndWait(const std::string &instName, int timeoutMs);

private slots:
    /// @brief 新建网络按钮点击
    void onNewNetwork();
    /// @brief 网络列表选中项变化
    void onNetworkSelected();
    /// @brief 网络列表右键菜单
    void onListContextMenu(const QPoint &pos);
    /// @brief 删除网络
    void onDeleteNetwork();
    /// @brief 重命名网络标签
    void onRenameNetwork();
    /// @brief 网络列表双击编辑
    void onListDoubleClicked();
    /// @brief UI控件值变化时保存到当前配置
    void onUIChanged();
    /// @brief 导出配置按钮点击
    void onExportConf();
    /// @brief 导入配置按钮点击
    void onImportConf();
    /// @brief 运行/停止网络按钮点击
    void onRunNetworkBtnClicked();
    /// @brief 网络启动完成处理
    void onNetworkStarted(const std::string &instName, bool success, const std::string &errorMsg);
    /// @brief 网络停止完成处理
    void onNetworkStopped(const std::string &instName, bool success, const std::string &errorMsg);
    /// @brief 网络信息收集完成处理
    void onInfosCollected(const std::vector<EasyTierFFI::KVPair> &infos);
    /// @brief 定时器超时，请求收集网络信息
    void onMonitorTimerTimeout() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    // 常量配置，避免魔法数字
    static constexpr int LEFT_PANEL_MIN_WIDTH = 160; // 左侧面板最小宽度
    static constexpr double MAX_WIDTH_RATIO = 0.5; // 最大宽度占窗口比例上限
    static constexpr double FULLSCREEN_RATIO = 0.15; // 全屏时左侧宽度占比
    static constexpr double EXPONENTIAL_K = 0.6; // 指数伸缩因子

private:
    // 左侧面板
    QFrame *m_leftFrame;                /// @brief 左侧面板容器
    QVBoxLayout *m_leftLayout;          /// @brief 左侧布局
    QtETLabelList *m_networksList;      /// @brief 网络列表
    QtETPushBtn *m_newNetworkBtn;       /// @brief 新建网络按钮
    QtETPushBtn *m_runNetworkBtn;       /// @brief 运行网络按钮
    QtETPushBtn *m_importConfBtn;       /// @brief 导入配置按钮
    QtETPushBtn *m_exportConfBtn;       /// @brief 导出配置按钮

    // 右侧面板
    QtETTabWidget *m_tabWidget;         /// @brief 选项卡容器
    QWidget *m_basicSettingsTab;        /// @brief 基础设置选项卡
    QWidget *m_advancedSettingsTab;     /// @brief 高级设置选项卡
    QWidget *m_runningStatusTab;        /// @brief 运行状态选项卡
    QWidget *m_runningLogTab;           /// @brief 运行日志选项卡

    // 基础设置控件
    QtETLineEdit *m_hostnameEdit;              /// @brief 用户名输入框 (hostname)
    QtETLineEdit *m_networkNameEdit;           /// @brief 网络号输入框 (network_name)
    QtETLineEdit *m_networkSecretEdit;         /// @brief 网络密钥输入框 (network_secret)
    QtETCheckBtn *m_dhcpCheckBox;           /// @brief DHCP 开关 (dhcp)
    QtETLineEdit *m_ipv4Edit;                  /// @brief IPv4 地址输入框 (ipv4)
    QtETCheckBtn *m_latencyFirstCheckBox;   /// @brief 低延迟优先开关 (latency_first)
    QtETCheckBtn *m_privateModeCheckBox;    /// @brief 私有模式开关 (private_mode)
    QtETLineEdit *m_serverEdit;                /// @brief 服务器地址输入框
    QtETPushBtn *m_addServerBtn;            /// @brief 添加服务器按钮
    QListWidget *m_serverListWidget;     /// @brief 服务器列表
    QtETPushBtn *m_removeServerBtn;         /// @brief 删除服务器按钮
    QtETPushBtn *m_publicServerBtn;     /// @brief 公共服务器列表按钮

    // 高级设置控件 - 功能开关板块
    QtETCheckBtn *m_enableKcpProxyCheckBox;             /// @brief 启用 KCP 代理 (enable_kcp_proxy)
    QtETCheckBtn *m_disableKcpInputCheckBox;            /// @brief 禁用 KCP 输入 (disable_kcp_input)
    QtETCheckBtn *m_noTunCheckBox;                      /// @brief 无 TUN 模式 (no_tun)
    QtETCheckBtn *m_enableQuicProxyCheckBox;            /// @brief 启用 QUIC 代理 (enable_quic_proxy)
    QtETCheckBtn *m_disableQuicInputCheckBox;           /// @brief 禁用 QUIC 输入 (disable_quic_input)
    QtETCheckBtn *m_disableRelayKcpCheckBox;            /// @brief 禁止转发 KCP (disable_relay_kcp)
    QtETCheckBtn *m_disableRelayQuicCheckBox;           /// @brief 禁止转发 QUIC (disable_relay_quic)
    QtETCheckBtn *m_enableRelayForeignNetworkKcpCheckBox;/// @brief 允许转发其他网络 KCP (enable_relay_foreign_network_kcp)
    QtETCheckBtn *m_enableRelayForeignNetworkQuicCheckBox;/// @brief 允许转发其他网络 QUIC (enable_relay_foreign_network_quic)
    QtETCheckBtn *m_disableUdpHolePunchingCheckBox;     /// @brief 禁用 UDP 打洞 (disable_udp_hole_punching)
    QtETCheckBtn *m_disableTcpHolePunchingCheckBox;     /// @brief 禁用 TCP 打洞 (disable_tcp_hole_punching)
    QtETCheckBtn *m_disableUpnpCheckBox;                /// @brief 禁用 UPnP (disable_upnp)
    QtETCheckBtn *m_needP2pCheckBox;                    /// @brief 需要 P2P (need_p2p)
    QtETCheckBtn *m_lazyP2pCheckBox;                    /// @brief 按需 P2P (lazy_p2p)
    QtETCheckBtn *m_p2pOnlyCheckBox;                    /// @brief 仅 P2P 通信 (p2p_only)
    QtETCheckBtn *m_multiThreadCheckBox;                /// @brief 启用多线程 (multi_thread)
    QtETCheckBtn *m_useSmoltcpCheckBox;                 /// @brief 使用 smoltcp 协议栈 (use_smoltcp)
    QtETCheckBtn *m_bindDeviceCheckBox;                 /// @brief 仅使用物理网卡 (bind_device)
    QtETCheckBtn *m_disableP2pCheckBox;                 /// @brief 禁用 P2P (disable_p2p)
    QtETCheckBtn *m_enableExitNodeCheckBox;             /// @brief 启用出口节点 (enable_exit_node)
    QtETCheckBtn *m_systemForwardingCheckBox;           /// @brief 系统转发 (system_forwarding)
    QtETCheckBtn *m_disableSymHolePunchingCheckBox;     /// @brief 禁用对称 NAT 打洞 (disable_sym_hole_punching)
    QtETCheckBtn *m_disableIpv6CheckBox;                /// @brief 禁用 IPv6
    QtETLineEdit *m_devNameEdit;                        /// @brief TUN 设备名 (dev_name)
    QtETLineEdit *m_mtuEdit;                            /// @brief MTU 值 (mtu)
    QtETCheckBtn *m_relayAllPeerRpcCheckBox;            /// @brief 转发 RPC 包 (relay_all_peer_rpc)
    QtETCheckBtn *m_enableEncryptionCheckBox;           /// @brief 启用加密 (enable_encryption)
    QtETCheckBtn *m_acceptDnsCheckBox;                  /// @brief 启用魔法 DNS (accept_dns)
    QtETComboBox *m_defaultProtocolCombo;               /// @brief 默认连接协议 (default_protocol)
    QtETComboBox *m_encryptionAlgorithmCombo;           /// @brief 默认加密协议 (encryption_algorithm)

    // 高级设置控件 - 网络白名单
    QtETCheckBtn *m_foreignNetworkWhitelistCheckBox;    /// @brief 启用网络白名单 (foreign_network_whitelist)
    QtETLineEdit *m_foreignNetworkWhitelistEdit;           /// @brief 网络白名单输入框
    QtETPushBtn *m_addWhitelistBtn;                     /// @brief 添加白名单按钮
    QListWidget *m_whitelistListWidget;              /// @brief 白名单列表
    QtETPushBtn *m_removeWhitelistBtn;                  /// @brief 删除白名单按钮

    // 高级设置控件 - 监听地址
    QtETLineEdit *m_listenAddrEdit;            /// @brief 监听地址输入框
    QtETPushBtn *m_addListenAddrBtn;        /// @brief 添加监听地址按钮
    QListWidget *m_listenAddrListWidget; /// @brief 监听地址列表
    QtETPushBtn *m_removeListenAddrBtn;     /// @brief 删除监听地址按钮

    // 高级设置控件 - 子网代理 CIDR
    QtETLineEdit *m_proxyNetworkEdit;               /// @brief 子网代理 CIDR 输入框 (proxy_network)
    QtETPushBtn *m_addProxyNetworkBtn;           /// @brief 添加 CIDR 按钮
    QListWidget *m_proxyNetworkListWidget;    /// @brief CIDR 列表
    QtETPushBtn *m_removeProxyNetworkBtn;        /// @brief 删除 CIDR 按钮
    QtETPushBtn *m_calculateCidrBtn;             /// @brief 打开 CIDR 计算器按钮

    // 高级设置控件 - 自定义路由规则
    QtETLineEdit *m_customRouteEdit;            /// @brief 自定义路由规则输入框 (custom_routes)
    QtETPushBtn *m_addCustomRouteBtn;        /// @brief 添加路由规则按钮
    QListWidget *m_customRouteListWidget;   /// @brief 路由规则列表
    QtETPushBtn *m_removeCustomRouteBtn;     /// @brief 删除路由规则按钮

    // 高级设置控件 - 出口节点列表
    QtETLineEdit *m_exitNodeEdit;               /// @brief 出口节点输入框 (exit_nodes)
    QtETPushBtn *m_addExitNodeBtn;           /// @brief 添加出口节点按钮
    QListWidget *m_exitNodeListWidget;      /// @brief 出口节点列表
    QtETPushBtn *m_removeExitNodeBtn;        /// @brief 删除出口节点按钮

    // 运行状态控件
    QLabel *m_statusLabel;              /// @brief 状态标签
    QWidget *m_nodeInfoContainer;       /// @brief 节点信息容器
    QVBoxLayout *m_nodeInfoLayout;      /// @brief 节点信息布局
    QList<QtETNodeInfo*> m_nodeInfoWidgets; /// @brief 节点信息控件列表
    QLabel *m_emptyLabel;               /// @brief 空状态提示标签

    // 运行日志控件
    QTextEdit *m_logTextEdit;           /// @brief 日志文本框

    // 常量
    static constexpr int MAX_LOG_COUNT = 300;  /// @brief 最大日志缓存数量

    // 网络配置数据
    std::vector<NetworkConf> m_networkConfs;  /// @brief 网络配置列表

    // 运行网络相关
    QThread *m_runThread;               /// @brief 运行网络的工作线程
    ETRunWorker *m_runWorker;           /// @brief 运行网络的工作对象
    QProgressDialog *m_progressDialog;  /// @brief 启动进度对话框
    std::string m_currentRunningInstName; /// @brief 当前正在操作的网络实例名称
    QTimer *m_monitorTimer;             /// @brief 节点监测定时器
    int m_runningNetworkCount;          /// @brief 正在运行的网络数量

    // 日志增量渲染状态
    QString m_lastRenderedInstName;     ///< 上次渲染到UI的网络实例名（用于检测网络切换）

    // 主布局
    QHBoxLayout *m_mainLayout;          /// @brief 主布局
    int m_initialWidth = 0;               /// @brief 初始窗口宽度（用于比例计算）
};

#endif // QTETNETWORK_H
