#include "qtetnetwork.h"

#include <QRegularExpressionValidator>

QtETNetwork::QtETNetwork(QWidget *parent)
    : QWidget(parent)
    , m_leftFrame(nullptr)
    , m_leftLayout(nullptr)
    , m_networksList(nullptr)
    , m_newNetworkBtn(nullptr)
    , m_runNetworkBtn(nullptr)
    , m_importConfBtn(nullptr)
    , m_exportConfBtn(nullptr)
    , m_tabWidget(nullptr)
    , m_basicSettingsTab(nullptr)
    , m_advancedSettingsTab(nullptr)
    , m_runningStatusTab(nullptr)
    , m_runningLogTab(nullptr)
    // 基础设置控件
    , m_usernameEdit(nullptr)
    , m_networkNameEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_togglePasswordBtn(nullptr)
    , m_dhcpCheckBox(nullptr)
    , m_ipEdit(nullptr)
    , m_lowLatencyCheckBox(nullptr)
    , m_privateModeCheckBox(nullptr)
    , m_serverEdit(nullptr)
    , m_addServerBtn(nullptr)
    , m_serverListWidget(nullptr)
    , m_removeServerBtn(nullptr)
    , m_publicServerBtn(nullptr)
    // 高级设置控件 - 功能开关
    , m_kcpProxyCheckBox(nullptr)
    , m_kcpInputDisableCheckBox(nullptr)
    , m_noTunModeCheckBox(nullptr)
    , m_quicProxyCheckBox(nullptr)
    , m_quicInputDisableCheckBox(nullptr)
    , m_udpHolePunchingDisableCheckBox(nullptr)
    , m_multithreadCheckBox(nullptr)
    , m_userModeStackCheckBox(nullptr)
    , m_onlyPhysicalNicCheckBox(nullptr)
    , m_p2pDisableCheckBox(nullptr)
    , m_exitNodeCheckBox(nullptr)
    , m_systemForwardingCheckBox(nullptr)
    , m_symmetricNatHolePunchingDisableCheckBox(nullptr)
    , m_ipv6DisableCheckBox(nullptr)
    , m_rpcPacketForwardingCheckBox(nullptr)
    , m_encryptionDisableCheckBox(nullptr)
    , m_magicDnsCheckBox(nullptr)
    // 高级设置控件 - RPC 端口
    , m_rpcPortEdit(nullptr)
    // 高级设置控件 - 网络白名单
    , m_relayNetworkWhitelistCheckBox(nullptr)
    , m_relayNetworkWhitelistEdit(nullptr)
    , m_addWhitelistBtn(nullptr)
    , m_whitelistListWidget(nullptr)
    , m_removeWhitelistBtn(nullptr)
    // 高级设置控件 - 监听地址
    , m_listenAddrEdit(nullptr)
    , m_addListenAddrBtn(nullptr)
    , m_listenAddrListWidget(nullptr)
    , m_removeListenAddrBtn(nullptr)
    // 高级设置控件 - 子网代理 CIDR
    , m_cidrEdit(nullptr)
    , m_addCidrBtn(nullptr)
    , m_cidrListWidget(nullptr)
    , m_removeCidrBtn(nullptr)
    , m_calculateCidrBtn(nullptr)
    // 运行状态控件
    , m_statusLabel(nullptr)
    // 运行日志控件
    , m_logLabel(nullptr)
    , m_mainLayout(nullptr)
{
    initUI();
}

QtETNetwork::~QtETNetwork() = default;

void QtETNetwork::initUI()
{
    // 创建主布局
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);

    // 初始化左右面板
    initLeftPanel();
    initRightPanel();
}

void QtETNetwork::initLeftPanel()
{
    // 创建左侧面板容器
    m_leftFrame = new QFrame(this);
    m_leftFrame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    // 创建左侧布局
    m_leftLayout = new QVBoxLayout(m_leftFrame);
    m_leftLayout->setSpacing(5);
    m_leftLayout->setContentsMargins(1, 0, 0, 0);

    // 创建网络列表
    m_networksList = new QtETListWidget(m_leftFrame);
    m_networksList->setMinimumSize(140, 0);
    m_networksList->setMaximumSize(140, QWIDGETSIZE_MAX);

    m_leftLayout->addWidget(m_networksList);

    // 创建按钮
    m_newNetworkBtn = new QPushButton(tr("新建网络"), m_leftFrame);
    m_runNetworkBtn = new QPushButton(tr("运行网络"), m_leftFrame);
    m_importConfBtn = new QPushButton(tr("导入配置"), m_leftFrame);
    m_exportConfBtn = new QPushButton(tr("导出配置"), m_leftFrame);

    m_leftLayout->addWidget(m_newNetworkBtn);
    m_leftLayout->addWidget(m_runNetworkBtn);
    m_leftLayout->addWidget(m_importConfBtn);
    m_leftLayout->addWidget(m_exportConfBtn);

    // 将左侧面板添加到主布局
    m_mainLayout->addWidget(m_leftFrame);
}

void QtETNetwork::initRightPanel()
{
    // 创建选项卡容器
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setCurrentIndex(0);

    // 创建基础设置选项卡
    m_basicSettingsTab = new QWidget();
    m_tabWidget->addTab(m_basicSettingsTab, tr("基础设置"));
    initBasicSettingsPage();

    // 创建高级设置选项卡
    m_advancedSettingsTab = new QWidget();
    m_tabWidget->addTab(m_advancedSettingsTab, tr("高级设置"));
    initAdvancedSettingsPage();

    // 创建运行状态选项卡
    m_runningStatusTab = new QWidget();
    m_tabWidget->addTab(m_runningStatusTab, tr("运行状态"));
    initRunningStatusPage();

    // 创建运行日志选项卡
    m_runningLogTab = new QWidget();
    m_tabWidget->addTab(m_runningLogTab, tr("运行日志"));
    initRunningLogPage();

    // 将选项卡添加到主布局
    m_mainLayout->addWidget(m_tabWidget);
}

void QtETNetwork::initBasicSettingsPage()
{
    // 创建滚动区
    QScrollArea *scrollArea = new QScrollArea(m_basicSettingsTab);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 创建滚动区内容部件
    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置全局字体大小为10px
    QFont contentFont = scrollContent->font();
    contentFont.setPointSize(10);
    scrollContent->setFont(contentFont);

    // 创建垂直布局
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(20, 20, 20, 20);
    scrollLayout->setSpacing(8);

    // 创建网络设置表单
    QWidget *networkFormWidget = new QWidget(scrollContent);
    QFormLayout *networkFormLayout = new QFormLayout(networkFormWidget);
    networkFormLayout->setHorizontalSpacing(20);
    networkFormLayout->setVerticalSpacing(15);

    // 用户名输入框
    m_usernameEdit = new QLineEdit(networkFormWidget);
    m_usernameEdit->setPlaceholderText(tr("请输入用户名（默认为本机名称）"));
    networkFormLayout->addRow(tr("用户名:"), m_usernameEdit);

    // 网络号输入框
    m_networkNameEdit = new QLineEdit(networkFormWidget);
    m_networkNameEdit->setPlaceholderText(tr("请输入网络名称"));
    networkFormLayout->addRow(tr("网络号:"), m_networkNameEdit);

    // 密码输入框 + 显示/隐藏按钮
    QWidget *passwordWidget = new QWidget(networkFormWidget);
    QHBoxLayout *passwordLayout = new QHBoxLayout();
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(5);
    passwordWidget->setLayout(passwordLayout);

    m_passwordEdit = new QLineEdit(passwordWidget);
    m_passwordEdit->setPlaceholderText(tr("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_togglePasswordBtn = new QPushButton(passwordWidget);
    m_togglePasswordBtn->setCheckable(true);
    m_togglePasswordBtn->setIcon(QIcon(QStringLiteral(":/icons/eye-slash.svg")));
    m_togglePasswordBtn->setIconSize(QSize(20, 20));
    m_togglePasswordBtn->setFixedSize(32, 26);
    m_togglePasswordBtn->setStyleSheet(QStringLiteral("background-color: #66ccff; border: none; border-radius: 5px;"));

    passwordLayout->addWidget(m_passwordEdit);
    passwordLayout->addWidget(m_togglePasswordBtn);
    networkFormLayout->addRow(tr("密码:"), passwordWidget);

    // DHCP 开关
    m_dhcpCheckBox = new QtETCheckBtn(networkFormWidget);
    m_dhcpCheckBox->setText(tr("启用 DHCP"));
    m_dhcpCheckBox->setChecked(true);
    m_dhcpCheckBox->setToolTip(tr("自动分配虚拟IP地址"));
    networkFormLayout->addRow(tr("IP 设置:"), m_dhcpCheckBox);

    // IPv4 地址输入框
    m_ipEdit = new QLineEdit(networkFormWidget);
    m_ipEdit->setPlaceholderText(tr("请输入 IPv4 地址"));
    m_ipEdit->setEnabled(false);
    // 设置 IP 地址验证器
    static const QRegularExpression ipRegex("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    m_ipEdit->setValidator(new QRegularExpressionValidator(ipRegex, m_ipEdit));
    networkFormLayout->addRow(tr("IPv4 地址:"), m_ipEdit);

    // 低延迟优先和私有模式放在同一行
    QWidget *optionsWidget = new QWidget(networkFormWidget);
    QHBoxLayout *optionsLayout = new QHBoxLayout(optionsWidget);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(20);

    m_lowLatencyCheckBox = new QtETCheckBtn(optionsWidget);
    m_lowLatencyCheckBox->setText(tr("低延迟优先"));
    m_lowLatencyCheckBox->setChecked(false);
    m_lowLatencyCheckBox->setToolTip(tr("开启后,会根据算法自动选择低延时的线路进行连接"));

    m_privateModeCheckBox = new QtETCheckBtn(optionsWidget);
    m_privateModeCheckBox->setText(tr("私有模式"));
    m_privateModeCheckBox->setChecked(true);
    m_privateModeCheckBox->setToolTip(tr("开启后,不允许网络号和密码不相同的节点使用本节点进行数据中转"));

    optionsLayout->addWidget(m_lowLatencyCheckBox, 1);
    optionsLayout->addWidget(m_privateModeCheckBox, 1);
    networkFormLayout->addRow(QString(), optionsWidget);

    scrollLayout->addWidget(networkFormWidget);

    // 创建服务器管理分组
    QWidget *serverWidget = new QWidget(scrollContent);
    QVBoxLayout *serverLayout = new QVBoxLayout(serverWidget);
    serverLayout->setContentsMargins(15, 6, 15, 15);

    // 服务器标题
    QLabel *serverTitle = new QLabel(tr("服务器:"), serverWidget);
    serverLayout->addWidget(serverTitle);

    // 服务器输入框和添加按钮
    QHBoxLayout *addServerLayout = new QHBoxLayout();
    m_serverEdit = new QLineEdit(serverWidget);
    m_serverEdit->setPlaceholderText(tr("请输入服务器地址"));
    m_addServerBtn = new QPushButton(tr("添加"), serverWidget);
    m_addServerBtn->setMinimumWidth(80);
    addServerLayout->addWidget(m_serverEdit, 1);
    addServerLayout->addWidget(m_addServerBtn);
    serverLayout->addLayout(addServerLayout);

    // 服务器列表和删除按钮
    QHBoxLayout *serverListLayout = new QHBoxLayout();
    m_serverListWidget = new QListWidget(serverWidget);
    m_serverListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    // 添加默认服务器
    m_serverListWidget->addItem(QStringLiteral("tcp://public.easytier.top:11010"));
    m_removeServerBtn = new QPushButton(tr("删除"), serverWidget);
    m_removeServerBtn->setMinimumWidth(80);
    m_removeServerBtn->setEnabled(false);
    serverListLayout->addWidget(m_serverListWidget, 1);
    serverListLayout->addWidget(m_removeServerBtn);
    serverLayout->addLayout(serverListLayout);

    // 公共服务器按钮
    m_publicServerBtn = new QPushButton(tr("公共服务器列表"), serverWidget);
    serverLayout->addWidget(m_publicServerBtn);

    scrollLayout->addWidget(serverWidget);

    // 添加垂直伸展空间
    scrollLayout->addStretch();

    // 设置滚动区内容
    scrollArea->setWidget(scrollContent);

    // 将滚动区添加到基础设置页面
    QVBoxLayout *pageLayout = new QVBoxLayout(m_basicSettingsTab);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // 连接信号槽
    connect(m_dhcpCheckBox, &QtETCheckBtn::toggled, this, [this](bool checked) {
        m_ipEdit->setEnabled(!checked);
    });
    connect(m_togglePasswordBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) {
            m_passwordEdit->setEchoMode(QLineEdit::Normal);
            m_togglePasswordBtn->setIcon(QIcon(QStringLiteral(":/icons/eye.svg")));
        } else {
            m_passwordEdit->setEchoMode(QLineEdit::Password);
            m_togglePasswordBtn->setIcon(QIcon(QStringLiteral(":/icons/eye-slash.svg")));
        }
    });
    connect(m_serverListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeServerBtn->setEnabled(m_serverListWidget->selectedItems().count() > 0);
    });
}

void QtETNetwork::initAdvancedSettingsPage()
{
    // 创建滚动区
    QScrollArea *scrollArea = new QScrollArea(m_advancedSettingsTab);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 创建滚动区内容部件
    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置全局字体大小为10px
    QFont contentFont = scrollContent->font();
    contentFont.setPointSize(10);
    scrollContent->setFont(contentFont);

    // 创建垂直布局
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(20, 20, 20, 20);
    scrollLayout->setSpacing(8);

    // ========== 功能开关区域（每行两个）==========
    QWidget *functionWidget = new QWidget(scrollContent);
    QGridLayout *functionGridLayout = new QGridLayout(functionWidget);
    functionGridLayout->setHorizontalSpacing(30);
    functionGridLayout->setVerticalSpacing(15);

    // 初始化功能开关控件
    m_kcpProxyCheckBox = new QtETCheckBtn(functionWidget);
    m_kcpProxyCheckBox->setText(tr("启用 KCP 代理"));
    m_kcpProxyCheckBox->setChecked(true);
    m_kcpProxyCheckBox->setToolTip(tr("使用UDP协议代理TCP流量,启用以获得更好的UDP P2P效果"));
    m_kcpProxyCheckBox->setBriefTip(tr("允许使用 KCP 协议发包"));

    m_kcpInputDisableCheckBox = new QtETCheckBtn(functionWidget);
    m_kcpInputDisableCheckBox->setText(tr("禁用 KCP 输入"));
    m_kcpInputDisableCheckBox->setChecked(false);
    m_kcpInputDisableCheckBox->setToolTip(tr("不接受通过KCP协议传输的流量"));
    m_kcpInputDisableCheckBox->setBriefTip(tr("不接收 KCP 协议的流量"));

    m_noTunModeCheckBox = new QtETCheckBtn(functionWidget);
    m_noTunModeCheckBox->setText(tr("无 TUN 模式"));
    m_noTunModeCheckBox->setChecked(false);
    m_noTunModeCheckBox->setToolTip(tr("不创建TUN网卡,开启时本节点无法主动访问其他节点,只能被动访问"));
    m_noTunModeCheckBox->setBriefTip(tr("不创建 TUN 虚拟网卡"));

    m_quicProxyCheckBox = new QtETCheckBtn(functionWidget);
    m_quicProxyCheckBox->setText(tr("启用 QUIC 代理"));
    m_quicProxyCheckBox->setChecked(false);
    m_quicProxyCheckBox->setToolTip(tr("QUIC是一个由Google设计,基于UDP的新一代可靠传输层协议,启用以获得更好的UDP P2P效果"));
    m_quicProxyCheckBox->setBriefTip(tr("允许使用 QUIC 协议发包"));

    m_quicInputDisableCheckBox = new QtETCheckBtn(functionWidget);
    m_quicInputDisableCheckBox->setText(tr("禁用 QUIC 输入"));
    m_quicInputDisableCheckBox->setChecked(false);
    m_quicInputDisableCheckBox->setToolTip(tr("不接受通过 QUIC 协议传输的流量"));
    m_quicInputDisableCheckBox->setBriefTip(tr("不接收 QUIC 协议的流量"));

    m_udpHolePunchingDisableCheckBox = new QtETCheckBtn(functionWidget);
    m_udpHolePunchingDisableCheckBox->setText(tr("禁用 UDP 打洞"));
    m_udpHolePunchingDisableCheckBox->setChecked(false);
    m_udpHolePunchingDisableCheckBox->setToolTip(tr("禁用UDP打洞,仅允许通过TCP进行P2P访问"));
    m_udpHolePunchingDisableCheckBox->setBriefTip(tr("不使用 UDP 协议打洞"));

    m_multithreadCheckBox = new QtETCheckBtn(functionWidget);
    m_multithreadCheckBox->setText(tr("启用多线程"));
    m_multithreadCheckBox->setChecked(true);
    m_multithreadCheckBox->setToolTip(tr("后端启用多线程,可提升组网性能,但可能会增加占用"));
    m_multithreadCheckBox->setBriefTip(tr("使用多线程优化提升组网性能"));

    m_userModeStackCheckBox = new QtETCheckBtn(functionWidget);
    m_userModeStackCheckBox->setText(tr("使用用户态协议栈"));
    m_userModeStackCheckBox->setChecked(false);
    m_userModeStackCheckBox->setToolTip(tr("使用用户态协议栈,默认使用系统协议栈"));
    m_userModeStackCheckBox->setBriefTip(tr("默认使用系统协议栈"));

    m_onlyPhysicalNicCheckBox = new QtETCheckBtn(functionWidget);
    m_onlyPhysicalNicCheckBox->setText(tr("仅使用物理网卡"));
    m_onlyPhysicalNicCheckBox->setChecked(true);
    m_onlyPhysicalNicCheckBox->setToolTip(tr("仅使用物理网卡与其他节点建立P2P连接"));
    m_onlyPhysicalNicCheckBox->setBriefTip(tr("不通过其他虚拟网卡进行连接"));

    m_p2pDisableCheckBox = new QtETCheckBtn(functionWidget);
    m_p2pDisableCheckBox->setText(tr("禁用 P2P"));
    m_p2pDisableCheckBox->setChecked(false);
    m_p2pDisableCheckBox->setToolTip(tr("流量需要经过你添加的中转服务器,不直接与其他节点建立P2P连接"));
    m_p2pDisableCheckBox->setBriefTip(tr("流量只从添加的节点中转"));

    m_exitNodeCheckBox = new QtETCheckBtn(functionWidget);
    m_exitNodeCheckBox->setText(tr("启用出口节点"));
    m_exitNodeCheckBox->setChecked(false);
    m_exitNodeCheckBox->setToolTip(tr("本节点可作为VPN的出口节点, 可被用于端口转发"));
    m_exitNodeCheckBox->setBriefTip(tr("请慎用该功能"));

    m_systemForwardingCheckBox = new QtETCheckBtn(functionWidget);
    m_systemForwardingCheckBox->setText(tr("系统转发"));
    m_systemForwardingCheckBox->setChecked(false);
    m_systemForwardingCheckBox->setToolTip(tr("通过系统内核转发子网代理数据包，禁用内置NAT"));
    m_systemForwardingCheckBox->setBriefTip(tr("通过系统内核转发子网数据包"));

    m_symmetricNatHolePunchingDisableCheckBox = new QtETCheckBtn(functionWidget);
    m_symmetricNatHolePunchingDisableCheckBox->setText(tr("禁用对称 NAT 打洞"));
    m_symmetricNatHolePunchingDisableCheckBox->setChecked(false);
    m_symmetricNatHolePunchingDisableCheckBox->setBriefTip(tr("在对称 NAT 环境下打洞可能失败"));

    m_ipv6DisableCheckBox = new QtETCheckBtn(functionWidget);
    m_ipv6DisableCheckBox->setText(tr("禁用 IPv6"));
    m_ipv6DisableCheckBox->setChecked(false);
    m_ipv6DisableCheckBox->setToolTip(tr("禁用IPv6连接, 仅使用IPv4"));
    m_ipv6DisableCheckBox->setBriefTip(tr("虚拟网络内禁用 IPv6 通信。"));

    m_rpcPacketForwardingCheckBox = new QtETCheckBtn(functionWidget);
    m_rpcPacketForwardingCheckBox->setText(tr("转发 RPC 包"));
    m_rpcPacketForwardingCheckBox->setChecked(false);
    m_rpcPacketForwardingCheckBox->setToolTip(tr("转发其他节点的网络配置,不论该节点是否在网络白名单中, 帮助其他节点建立P2P连接"));
    m_rpcPacketForwardingCheckBox->setBriefTip(tr("帮助其他节点建立P2P连接"));

    m_encryptionDisableCheckBox = new QtETCheckBtn(functionWidget);
    m_encryptionDisableCheckBox->setText(tr("禁用加密"));
    m_encryptionDisableCheckBox->setChecked(false);
    m_encryptionDisableCheckBox->setToolTip(tr("禁用本节点通信的加密，必须与对等节点相同,正常情况下请保持加密"));
    m_encryptionDisableCheckBox->setBriefTip(tr("正常情况下请勿开启"));

    m_magicDnsCheckBox = new QtETCheckBtn(functionWidget);
    m_magicDnsCheckBox->setText(tr("启用魔法 DNS"));
    m_magicDnsCheckBox->setChecked(false);
    m_magicDnsCheckBox->setToolTip(tr("启用后可通过\"用户名.et.net\"访问本节点\n注意：Linux用户需要手动配置 DNS 服务器为100.100.100.101"));
    m_magicDnsCheckBox->setBriefTip(tr("通过\"用户名.et.net\"访问本节点"));

    // 添加功能开关到网格布局（每行两个）
    int row = 0;
    functionGridLayout->addWidget(m_kcpProxyCheckBox, row, 0);
    functionGridLayout->addWidget(m_kcpInputDisableCheckBox, row, 1);
    ++row;

    functionGridLayout->addWidget(m_quicProxyCheckBox, row, 0);
    functionGridLayout->addWidget(m_quicInputDisableCheckBox, row, 1);
    ++row;

    functionGridLayout->addWidget(m_noTunModeCheckBox, row, 0);
    functionGridLayout->addWidget(m_udpHolePunchingDisableCheckBox, row, 1);
    ++row;

    functionGridLayout->addWidget(m_multithreadCheckBox, row, 0);
    functionGridLayout->addWidget(m_userModeStackCheckBox, row, 1);
    ++row;

    functionGridLayout->addWidget(m_p2pDisableCheckBox, row, 0);
    functionGridLayout->addWidget(m_onlyPhysicalNicCheckBox, row, 1);
    ++row;

    functionGridLayout->addWidget(m_exitNodeCheckBox, row, 0);
    functionGridLayout->addWidget(m_systemForwardingCheckBox, row, 1);
    ++row;

    functionGridLayout->addWidget(m_symmetricNatHolePunchingDisableCheckBox, row, 0);
    functionGridLayout->addWidget(m_ipv6DisableCheckBox, row, 1);
    ++row;

    functionGridLayout->addWidget(m_rpcPacketForwardingCheckBox, row, 0);
    functionGridLayout->addWidget(m_encryptionDisableCheckBox, row, 1);
    ++row;

    functionGridLayout->addWidget(m_magicDnsCheckBox, row, 0);

    scrollLayout->addWidget(functionWidget);

    // ========== RPC 端口设置 ==========
    QWidget *rpcPortWidget = new QWidget(scrollContent);
    QVBoxLayout *rpcPortLayout = new QVBoxLayout(rpcPortWidget);
    rpcPortLayout->setContentsMargins(15, 5, 15, 0);

    QHBoxLayout *rpcPortInputLayout = new QHBoxLayout();
    QLabel *rpcPortLabel = new QLabel(tr("RPC 端口号:"), rpcPortWidget);
    rpcPortLabel->setToolTip(tr("RPC是EasyTier去中心化组网中用于下发组网配置的通道"));
    m_rpcPortEdit = new QLineEdit(rpcPortWidget);
    m_rpcPortEdit->setPlaceholderText(tr("请输入 RPC 端口号"));
    m_rpcPortEdit->setText(QStringLiteral("0"));
    m_rpcPortEdit->setMaximumWidth(150);
    rpcPortInputLayout->addWidget(rpcPortLabel);
    rpcPortInputLayout->addWidget(m_rpcPortEdit);
    rpcPortInputLayout->addStretch();
    rpcPortLayout->addLayout(rpcPortInputLayout);

    QLabel *rpcPortHint = new QLabel(tr("端口范围: 0-65535 (0 表示使用随机端口)"), rpcPortWidget);
    rpcPortHint->setStyleSheet(QStringLiteral("color: gray; font-size: 10px;"));
    rpcPortLayout->addWidget(rpcPortHint);

    scrollLayout->addWidget(rpcPortWidget);

    // ========== 网络白名单 ==========
    QWidget *whitelistWidget = new QWidget(scrollContent);
    QVBoxLayout *whitelistLayout = new QVBoxLayout(whitelistWidget);
    whitelistLayout->setContentsMargins(15, 5, 15, 10);

    m_relayNetworkWhitelistCheckBox = new QtETCheckBtn(whitelistWidget);
    m_relayNetworkWhitelistCheckBox->setText(tr("启用网络白名单"));
    m_relayNetworkWhitelistCheckBox->setToolTip(tr("仅转发网络白名单中VPN的流量, 留空则为不转发任何网络的流量"));
    m_relayNetworkWhitelistCheckBox->setChecked(false);
    whitelistLayout->addWidget(m_relayNetworkWhitelistCheckBox);

    // 白名单控件容器
    QWidget *whitelistControlsWidget = new QWidget(whitelistWidget);
    QVBoxLayout *whitelistControlsLayout = new QVBoxLayout(whitelistControlsWidget);
    whitelistControlsLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *addWhitelistLayout = new QHBoxLayout();
    m_relayNetworkWhitelistEdit = new QLineEdit(whitelistControlsWidget);
    m_relayNetworkWhitelistEdit->setPlaceholderText(tr("请输入网络名称"));
    m_addWhitelistBtn = new QPushButton(tr("添加"), whitelistControlsWidget);
    m_addWhitelistBtn->setMinimumWidth(80);
    addWhitelistLayout->addWidget(m_relayNetworkWhitelistEdit, 1);
    addWhitelistLayout->addWidget(m_addWhitelistBtn);
    whitelistControlsLayout->addLayout(addWhitelistLayout);

    QHBoxLayout *whitelistListLayout = new QHBoxLayout();
    m_whitelistListWidget = new QListWidget(whitelistControlsWidget);
    m_whitelistListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_removeWhitelistBtn = new QPushButton(tr("删除"), whitelistControlsWidget);
    m_removeWhitelistBtn->setMinimumWidth(80);
    m_removeWhitelistBtn->setEnabled(false);
    whitelistListLayout->addWidget(m_whitelistListWidget, 1);
    whitelistListLayout->addWidget(m_removeWhitelistBtn);
    whitelistControlsLayout->addLayout(whitelistListLayout);

    whitelistControlsWidget->setVisible(false);
    whitelistLayout->addWidget(whitelistControlsWidget);

    scrollLayout->addWidget(whitelistWidget);

    // ========== 监听地址 ==========
    QWidget *listenAddrWidget = new QWidget(scrollContent);
    QVBoxLayout *listenAddrLayout = new QVBoxLayout(listenAddrWidget);
    listenAddrLayout->setContentsMargins(15, 5, 15, 0);

    QLabel *listenAddrTitle = new QLabel(tr("监听地址:"), listenAddrWidget);
    listenAddrTitle->setToolTip(tr("监听节点连接,他人若想通过本节点加入组网需要连接监听地址"));
    listenAddrLayout->addWidget(listenAddrTitle);

    QHBoxLayout *addListenAddrLayout = new QHBoxLayout();
    m_listenAddrEdit = new QLineEdit(listenAddrWidget);
    m_listenAddrEdit->setPlaceholderText(tr("请输入监听地址与端口"));
    m_addListenAddrBtn = new QPushButton(tr("添加"), listenAddrWidget);
    m_addListenAddrBtn->setMinimumWidth(80);
    addListenAddrLayout->addWidget(m_listenAddrEdit, 1);
    addListenAddrLayout->addWidget(m_addListenAddrBtn);
    listenAddrLayout->addLayout(addListenAddrLayout);

    QHBoxLayout *listenAddrListLayout = new QHBoxLayout();
    m_listenAddrListWidget = new QListWidget(listenAddrWidget);
    m_listenAddrListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listenAddrListWidget->addItem(QStringLiteral("tcp://0.0.0.0:11010"));
    m_listenAddrListWidget->addItem(QStringLiteral("udp://0.0.0.0:11010"));
    m_removeListenAddrBtn = new QPushButton(tr("删除"), listenAddrWidget);
    m_removeListenAddrBtn->setMinimumWidth(80);
    m_removeListenAddrBtn->setEnabled(false);
    listenAddrListLayout->addWidget(m_listenAddrListWidget, 1);
    listenAddrListLayout->addWidget(m_removeListenAddrBtn);
    listenAddrLayout->addLayout(listenAddrListLayout);

    scrollLayout->addWidget(listenAddrWidget);

    // ========== 子网代理 CIDR ==========
    QWidget *cidrWidget = new QWidget(scrollContent);
    QVBoxLayout *cidrLayout = new QVBoxLayout(cidrWidget);
    cidrLayout->setContentsMargins(15, 15, 15, 15);

    QLabel *cidrTitle = new QLabel(tr("子网代理 CIDR:"), cidrWidget);
    cidrTitle->setToolTip(tr("输入想要进行子网代理的IP范围, CIDR格式"));
    cidrLayout->addWidget(cidrTitle);

    QHBoxLayout *addCidrLayout = new QHBoxLayout();
    m_cidrEdit = new QLineEdit(cidrWidget);
    m_cidrEdit->setPlaceholderText(tr("请输入子网代理 CIDR"));
    m_addCidrBtn = new QPushButton(tr("添加"), cidrWidget);
    m_addCidrBtn->setMinimumWidth(80);
    addCidrLayout->addWidget(m_cidrEdit, 1);
    addCidrLayout->addWidget(m_addCidrBtn);
    cidrLayout->addLayout(addCidrLayout);

    QHBoxLayout *cidrListLayout = new QHBoxLayout();
    m_cidrListWidget = new QListWidget(cidrWidget);
    m_cidrListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_removeCidrBtn = new QPushButton(tr("删除"), cidrWidget);
    m_removeCidrBtn->setMinimumWidth(80);
    m_removeCidrBtn->setEnabled(false);
    cidrListLayout->addWidget(m_cidrListWidget, 1);
    cidrListLayout->addWidget(m_removeCidrBtn);
    cidrLayout->addLayout(cidrListLayout);

    m_calculateCidrBtn = new QPushButton(tr("打开 CIDR 计算器"), cidrWidget);
    cidrLayout->addWidget(m_calculateCidrBtn);

    scrollLayout->addWidget(cidrWidget);

    // 添加垂直伸展空间
    scrollLayout->addStretch();

    // 设置滚动区内容
    scrollArea->setWidget(scrollContent);

    // 将滚动区添加到高级设置页面
    QVBoxLayout *pageLayout = new QVBoxLayout(m_advancedSettingsTab);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    // 连接信号槽
    connect(m_relayNetworkWhitelistCheckBox, &QtETCheckBtn::toggled, this, [this, whitelistControlsWidget](bool checked) {
        whitelistControlsWidget->setVisible(checked);
        m_relayNetworkWhitelistEdit->setEnabled(checked);
        m_addWhitelistBtn->setEnabled(checked);
        m_whitelistListWidget->setEnabled(checked);
    });
    connect(m_listenAddrListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeListenAddrBtn->setEnabled(m_listenAddrListWidget->selectedItems().count() > 0);
    });
    connect(m_whitelistListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        if (m_relayNetworkWhitelistCheckBox->isChecked()) {
            m_removeWhitelistBtn->setEnabled(m_whitelistListWidget->selectedItems().count() > 0);
        }
    });
    connect(m_cidrListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeCidrBtn->setEnabled(m_cidrListWidget->selectedItems().count() > 0);
    });
}

void QtETNetwork::initRunningStatusPage()
{
    QVBoxLayout *layout = new QVBoxLayout(m_runningStatusTab);
    layout->setContentsMargins(20, 20, 20, 20);

    m_statusLabel = new QLabel(tr("运行状态页面（待实现）"), m_runningStatusTab);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusLabel);
}

void QtETNetwork::initRunningLogPage()
{
    QVBoxLayout *layout = new QVBoxLayout(m_runningLogTab);
    layout->setContentsMargins(20, 20, 20, 20);

    m_logLabel = new QLabel(tr("运行日志页面（待实现）"), m_runningLogTab);
    m_logLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_logLabel);
}
