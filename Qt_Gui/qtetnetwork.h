#ifndef QTETNETWORK_H
#define QTETNETWORK_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QComboBox>
#include <QAction>
#include <QScrollBar>
#include <QWheelEvent>
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
#include <vector>

#include "qtetlistwidget.h"
#include "qtetcheckbtn.h"
#include "qtetnodeinfo.h"
#include "networkconf.h"
#include "ETRunWorker.h"

/// @brief 平滑滚动事件过滤器
/// 实现滚轮平滑滚动效果
/// 支持所有继承自 QAbstractScrollArea 的控件（如 QScrollArea、QTextEdit 等）
class SmoothScrollFilter : public QObject
{
    Q_OBJECT

public:
    explicit SmoothScrollFilter(QAbstractScrollArea *scrollArea, QObject *parent = nullptr)
        : QObject(parent)
        , m_scrollArea(scrollArea)
        , m_animation(nullptr)
        , m_targetValue(0)
    {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::Wheel && m_scrollArea) {
            // 滚轮事件由 viewport 接收，而非 QAbstractScrollArea 本身
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
            if (!scrollBar || !scrollBar->isVisible()) {
                return QObject::eventFilter(watched, event);
            }

            // 计算滚动距离
            int delta = -wheelEvent->angleDelta().y();
            int scrollStep = 80; // 平滑滚动步长

            // 目标位置
            int currentValue = scrollBar->value();
            m_targetValue = currentValue + (delta > 0 ? scrollStep : -scrollStep);
            m_targetValue = qBound(scrollBar->minimum(), m_targetValue, scrollBar->maximum());

            // 创建或更新动画
            if (!m_animation) {
                m_animation = new QPropertyAnimation(scrollBar, "value", this);
                m_animation->setDuration(150); // 动画持续时间(毫秒)
                m_animation->setEasingCurve(QEasingCurve::OutQuad); // 缓出效果
            }

            m_animation->stop();
            m_animation->setStartValue(currentValue);
            m_animation->setEndValue(m_targetValue);
            m_animation->start();

            return true; // 事件已处理
        }

        return QObject::eventFilter(watched, event);
    }

private:
    QAbstractScrollArea *m_scrollArea;  ///< 关联的滚动区域
    QPropertyAnimation *m_animation;     ///< 滚动动画
    int m_targetValue;                   ///< 目标滚动位置
};

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
    /// @brief 更新高级设置功能开关的网格布局
    void updateFunctionGridLayout();
    /// @brief 重写 resizeEvent 监听宽度变化
    void resizeEvent(QResizeEvent *event) override;

public:
    /// @brief 加载所有网络配置到UI
    void loadAllNetworkConfs();
    /// @brief 保存所有网络配置到文件
    void saveAllNetworkConfs() const;

private:
    /// @brief 将指定索引的网络配置加载到UI
    /// @param index 网络配置索引
    void loadConfToUI(const int& index) const;
    /// @brief 从UI保存配置到指定索引的网络配置
    /// @param index 网络配置索引
    void saveConfFromUI(const int& index);
    /// @brief 更新 TabWidget 的启用状态
    void updateTabWidgetState() const;
    /// @brief 设置UI控件的信号连接
    void setupUIConnections();
    /// @brief 更新运行按钮的样式（根据当前选中的网络状态）
    void updateRunButtonStyle() const;
    /// @brief 更新网络列表项的样式
    /// @param index 网络配置索引
    void updateListItemStyle(const int& index) const;
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

signals:
    /// @brief 网络启动完成信号
    /// @param networkName 网络名称
    /// @param success 是否成功
    /// @param errorMsg 错误信息（成功时为空）
    void networkStarted(const QString &networkName, bool success, const QString &errorMsg);
    
    /// @brief 网络停止完成信号
    /// @param networkName 网络名称
    /// @param success 是否成功
    /// @param errorMsg 错误信息（成功时为空）
    void networkStopped(const QString &networkName, bool success, const QString &errorMsg);

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
    void onListDoubleClicked(QListWidgetItem *item);
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

private:
    // 左侧面板
    QFrame *m_leftFrame;                /// @brief 左侧面板容器
    QVBoxLayout *m_leftLayout;          /// @brief 左侧布局
    QtETListWidget *m_networksList;     /// @brief 网络列表
    QPushButton *m_newNetworkBtn;       /// @brief 新建网络按钮
    QPushButton *m_runNetworkBtn;       /// @brief 运行网络按钮
    QPushButton *m_importConfBtn;       /// @brief 导入配置按钮
    QPushButton *m_exportConfBtn;       /// @brief 导出配置按钮

    // 右侧面板
    QTabWidget *m_tabWidget;            /// @brief 选项卡容器
    QWidget *m_basicSettingsTab;        /// @brief 基础设置选项卡
    QWidget *m_advancedSettingsTab;     /// @brief 高级设置选项卡
    QWidget *m_runningStatusTab;        /// @brief 运行状态选项卡
    QWidget *m_runningLogTab;           /// @brief 运行日志选项卡

    // 基础设置控件
    QLineEdit *m_hostnameEdit;              /// @brief 用户名输入框 (hostname)
    QLineEdit *m_networkNameEdit;           /// @brief 网络号输入框 (network_name)
    QLineEdit *m_networkSecretEdit;         /// @brief 网络密钥输入框 (network_secret)
    QtETCheckBtn *m_dhcpCheckBox;           /// @brief DHCP 开关 (dhcp)
    QLineEdit *m_ipv4Edit;                  /// @brief IPv4 地址输入框 (ipv4)
    QtETCheckBtn *m_latencyFirstCheckBox;   /// @brief 低延迟优先开关 (latency_first)
    QtETCheckBtn *m_privateModeCheckBox;    /// @brief 私有模式开关 (private_mode)
    QLineEdit *m_serverEdit;            /// @brief 服务器地址输入框
    QPushButton *m_addServerBtn;        /// @brief 添加服务器按钮
    QListWidget *m_serverListWidget;    /// @brief 服务器列表
    QPushButton *m_removeServerBtn;     /// @brief 删除服务器按钮
    QPushButton *m_publicServerBtn;     /// @brief 公共服务器列表按钮

    // 高级设置控件 - 功能开关
    QWidget *m_functionWidget;                          /// @brief 功能开关容器
    QGridLayout *m_functionGridLayout;                  /// @brief 功能开关网格布局
    QList<QtETCheckBtn*> m_functionCheckBoxes;          /// @brief 功能开关列表
    QtETCheckBtn *m_enableKcpProxyCheckBox;             /// @brief 启用 KCP 代理 (enable_kcp_proxy)
    QtETCheckBtn *m_disableKcpInputCheckBox;            /// @brief 禁用 KCP 输入 (disable_kcp_input)
    QtETCheckBtn *m_noTunCheckBox;                      /// @brief 无 TUN 模式 (no_tun)
    QtETCheckBtn *m_enableQuicProxyCheckBox;            /// @brief 启用 QUIC 代理 (enable_quic_proxy)
    QtETCheckBtn *m_disableQuicInputCheckBox;           /// @brief 禁用 QUIC 输入 (disable_quic_input)
    QtETCheckBtn *m_disableUdpHolePunchingCheckBox;     /// @brief 禁用 UDP 打洞 (disable_udp_hole_punching)
    QtETCheckBtn *m_multiThreadCheckBox;                /// @brief 启用多线程 (multi_thread)
    QtETCheckBtn *m_useSmoltcpCheckBox;                 /// @brief 使用用户态协议栈 (use_smoltcp)
    QtETCheckBtn *m_bindDeviceCheckBox;                 /// @brief 仅使用物理网卡 (bind_device)
    QtETCheckBtn *m_disableP2pCheckBox;                 /// @brief 禁用 P2P (disable_p2p)
    QtETCheckBtn *m_enableExitNodeCheckBox;             /// @brief 启用出口节点 (enable_exit_node)
    QtETCheckBtn *m_systemForwardingCheckBox;           /// @brief 系统转发 (system_forwarding)
    QtETCheckBtn *m_disableSymHolePunchingCheckBox;     /// @brief 禁用对称 NAT 打洞 (disable_sym_hole_punching)
    QtETCheckBtn *m_disableIpv6CheckBox;                /// @brief 禁用 IPv6
    QtETCheckBtn *m_relayAllPeerRpcCheckBox;            /// @brief 转发 RPC 包 (relay_all_peer_rpc)
    QtETCheckBtn *m_enableEncryptionCheckBox;           /// @brief 启用加密 (enable_encryption)
    QtETCheckBtn *m_acceptDnsCheckBox;                  /// @brief 启用魔法 DNS (accept_dns)

    // 高级设置控件 - 网络白名单
    QtETCheckBtn *m_foreignNetworkWhitelistCheckBox;    /// @brief 启用网络白名单 (foreign_network_whitelist)
    QLineEdit *m_foreignNetworkWhitelistEdit;           /// @brief 网络白名单输入框
    QPushButton *m_addWhitelistBtn;                     /// @brief 添加白名单按钮
    QListWidget *m_whitelistListWidget;                 /// @brief 白名单列表
    QPushButton *m_removeWhitelistBtn;                  /// @brief 删除白名单按钮

    // 高级设置控件 - 监听地址
    QLineEdit *m_listenAddrEdit;        /// @brief 监听地址输入框
    QPushButton *m_addListenAddrBtn;    /// @brief 添加监听地址按钮
    QListWidget *m_listenAddrListWidget;/// @brief 监听地址列表
    QPushButton *m_removeListenAddrBtn; /// @brief 删除监听地址按钮

    // 高级设置控件 - 子网代理 CIDR
    QLineEdit *m_proxyNetworkEdit;           /// @brief 子网代理 CIDR 输入框 (proxy_network)
    QPushButton *m_addProxyNetworkBtn;       /// @brief 添加 CIDR 按钮
    QListWidget *m_proxyNetworkListWidget;   /// @brief CIDR 列表
    QPushButton *m_removeProxyNetworkBtn;    /// @brief 删除 CIDR 按钮
    QPushButton *m_calculateCidrBtn;         /// @brief 打开 CIDR 计算器按钮

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

    // 主布局
    QHBoxLayout *m_mainLayout;          /// @brief 主布局
};

#endif // QTETNETWORK_H
