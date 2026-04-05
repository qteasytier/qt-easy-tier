#include "qtetnetwork.h"

#include <QRegularExpressionValidator>
#include <QFormLayout>
#include <QFontDatabase>
#include <QFileDialog>
#include <QStandardPaths>
#include <set>

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
    , m_hostnameEdit(nullptr)
    , m_networkNameEdit(nullptr)
    , m_networkSecretEdit(nullptr)
    , m_dhcpCheckBox(nullptr)
    , m_ipv4Edit(nullptr)
    , m_latencyFirstCheckBox(nullptr)
    , m_privateModeCheckBox(nullptr)
    , m_serverEdit(nullptr)
    , m_addServerBtn(nullptr)
    , m_serverListWidget(nullptr)
    , m_removeServerBtn(nullptr)
    , m_publicServerBtn(nullptr)
    // 高级设置控件 - 功能开关
    , m_functionWidget(nullptr)
    , m_functionGridLayout(nullptr)
    , m_enableKcpProxyCheckBox(nullptr)
    , m_disableKcpInputCheckBox(nullptr)
    , m_noTunCheckBox(nullptr)
    , m_enableQuicProxyCheckBox(nullptr)
    , m_disableQuicInputCheckBox(nullptr)
    , m_disableUdpHolePunchingCheckBox(nullptr)
    , m_multiThreadCheckBox(nullptr)
    , m_useSmoltcpCheckBox(nullptr)
    , m_bindDeviceCheckBox(nullptr)
    , m_disableP2pCheckBox(nullptr)
    , m_enableExitNodeCheckBox(nullptr)
    , m_systemForwardingCheckBox(nullptr)
    , m_disableSymHolePunchingCheckBox(nullptr)
    , m_disableIpv6CheckBox(nullptr)
    , m_relayAllPeerRpcCheckBox(nullptr)
    , m_enableEncryptionCheckBox(nullptr)
    , m_acceptDnsCheckBox(nullptr)
    // 高级设置控件 - RPC 端口
    , m_rpcPortEdit(nullptr)
    // 高级设置控件 - 网络白名单
    , m_foreignNetworkWhitelistCheckBox(nullptr)
    , m_foreignNetworkWhitelistEdit(nullptr)
    , m_addWhitelistBtn(nullptr)
    , m_whitelistListWidget(nullptr)
    , m_removeWhitelistBtn(nullptr)
    // 高级设置控件 - 监听地址
    , m_listenAddrEdit(nullptr)
    , m_addListenAddrBtn(nullptr)
    , m_listenAddrListWidget(nullptr)
    , m_removeListenAddrBtn(nullptr)
    // 高级设置控件 - 子网代理 CIDR
    , m_proxyNetworkEdit(nullptr)
    , m_addProxyNetworkBtn(nullptr)
    , m_proxyNetworkListWidget(nullptr)
    , m_removeProxyNetworkBtn(nullptr)
    , m_calculateCidrBtn(nullptr)
    // 运行状态控件
    , m_statusLabel(nullptr)
    , m_nodeInfoContainer(nullptr)
    , m_nodeInfoLayout(nullptr)
    , m_emptyLabel(nullptr)
    // 运行日志控件
    , m_logLabel(nullptr)
    // 运行网络相关
    , m_runThread(nullptr)
    , m_runWorker(nullptr)
    , m_progressDialog(nullptr)
    , m_monitorTimer(nullptr)
    , m_runningNetworkCount(0)
    , m_mainLayout(nullptr)
{
    initUI();
    
    // 初始化运行网络的线程和 Worker
    m_runThread = new QThread(this);
    m_runWorker = new ETRunWorker();
    m_runWorker->moveToThread(m_runThread);
    
    // 连接 Worker 信号到主线程槽
    connect(m_runWorker, &ETRunWorker::etRunStarted, this, &QtETNetwork::onNetworkStarted, Qt::QueuedConnection);
    connect(m_runWorker, &ETRunWorker::etRunStopped, this, &QtETNetwork::onNetworkStopped, Qt::QueuedConnection);
    connect(m_runWorker, &ETRunWorker::infosCollected, this, &QtETNetwork::onInfosCollected, Qt::QueuedConnection);
    
    // 线程结束时清理 Worker
    connect(m_runThread, &QThread::finished, m_runWorker, &QObject::deleteLater);
    
    // 启动线程
    m_runThread->start();
    
    // 初始化节点监测定时器
    m_monitorTimer = new QTimer(this);
    m_monitorTimer->setInterval(2000);  // 2秒间隔
    connect(m_monitorTimer, &QTimer::timeout, this, &QtETNetwork::onMonitorTimerTimeout);
    
    // 启用右键菜单
    m_networksList->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // 连接信号槽
    connect(m_newNetworkBtn, &QPushButton::clicked, this, &QtETNetwork::onNewNetwork);
    connect(m_networksList, &QListWidget::itemSelectionChanged, this, &QtETNetwork::onNetworkSelected);
    connect(m_networksList, &QListWidget::customContextMenuRequested, this, &QtETNetwork::onListContextMenu);
    connect(m_exportConfBtn, &QPushButton::clicked, this, &QtETNetwork::onExportConf);
    connect(m_importConfBtn, &QPushButton::clicked, this, &QtETNetwork::onImportConf);
    connect(m_runNetworkBtn, &QPushButton::clicked, this, &QtETNetwork::onRunNetworkBtnClicked);
    
    // 设置UI控件的信号连接
    setupUIConnections();
    
    // 初始化 TabWidget 状态（禁用）
    updateTabWidgetState();
}

QtETNetwork::~QtETNetwork()
{
    // 停止并清理线程
    if (m_runThread) {
        m_runThread->quit();
        m_runThread->wait();
    }
}

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

    // 创建网络列表标题
    QLabel *networksLabel = new QLabel(tr("我创建的:"), m_leftFrame);
    QFont font = networksLabel->font();
    font.setPointSize(12);
    font.setWeight(QFont::Bold);
    networksLabel->setFont(font);
    networksLabel->setMinimumHeight(32);
    m_leftLayout->addWidget(networksLabel);

    // 创建网络列表
    m_networksList = new QtETListWidget(m_leftFrame);
    m_networksList->setMinimumSize(140, 0);
    m_networksList->setMaximumSize(140, QWIDGETSIZE_MAX);

    /*m_networksList->addItem("家里的NAS");
    m_networksList->addItem("我的服务器");
    m_networksList->addItem("远程运维");*/

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
    m_hostnameEdit = new QLineEdit(networkFormWidget);
    m_hostnameEdit->setPlaceholderText(tr("请输入用户名（默认为本机名称）"));
    networkFormLayout->addRow(tr("用户名:"), m_hostnameEdit);

    // 网络号输入框
    m_networkNameEdit = new QLineEdit(networkFormWidget);
    m_networkNameEdit->setPlaceholderText(tr("请输入网络名称"));
    networkFormLayout->addRow(tr("网络号:"), m_networkNameEdit);

    // 密码输入框（行内小眼睛图标）
    m_networkSecretEdit = new QLineEdit(networkFormWidget);
    m_networkSecretEdit->setPlaceholderText(tr("请输入密码"));
    m_networkSecretEdit->setEchoMode(QLineEdit::Password);

    // 添加行内显示/隐藏密码按钮
    QAction *togglePasswordAction = m_networkSecretEdit->addAction(QIcon(QStringLiteral(":/icons/eye-slash.svg")), QLineEdit::TrailingPosition);
    togglePasswordAction->setCheckable(true);
    connect(togglePasswordAction, &QAction::toggled, this, [this, togglePasswordAction](bool checked) {
        if (checked) {
            m_networkSecretEdit->setEchoMode(QLineEdit::Normal);
            togglePasswordAction->setIcon(QIcon(QStringLiteral(":/icons/eye.svg")));
        } else {
            m_networkSecretEdit->setEchoMode(QLineEdit::Password);
            togglePasswordAction->setIcon(QIcon(QStringLiteral(":/icons/eye-slash.svg")));
        }
    });

    networkFormLayout->addRow(tr("密码:"), m_networkSecretEdit);

    // DHCP 开关
    m_dhcpCheckBox = new QtETCheckBtn(networkFormWidget);
    m_dhcpCheckBox->setText(tr("启用 DHCP"));
    m_dhcpCheckBox->setChecked(true);
    m_dhcpCheckBox->setBorderless(true);
    m_dhcpCheckBox->setToolTip(tr("自动分配虚拟IP地址"));
    networkFormLayout->addRow(tr("IP 设置:"), m_dhcpCheckBox);

    // IPv4 地址输入框
    m_ipv4Edit = new QLineEdit(networkFormWidget);
    m_ipv4Edit->setPlaceholderText(tr("请输入 IPv4 地址"));
    m_ipv4Edit->setEnabled(false);
    // 设置 IP 地址验证器
    static const QRegularExpression ipRegex("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    m_ipv4Edit->setValidator(new QRegularExpressionValidator(ipRegex, m_ipv4Edit));
    networkFormLayout->addRow(tr("IPv4 地址:"), m_ipv4Edit);

    // 低延迟优先和私有模式放在同一行
    QWidget *optionsWidget = new QWidget(networkFormWidget);
    QHBoxLayout *optionsLayout = new QHBoxLayout(optionsWidget);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(20);

    m_latencyFirstCheckBox = new QtETCheckBtn(optionsWidget);
    m_latencyFirstCheckBox->setText(tr("低延迟优先"));
    m_latencyFirstCheckBox->setBorderless(true);
    m_latencyFirstCheckBox->setMaximumWidth(200);
    m_latencyFirstCheckBox->setToolTip(tr("开启后,会根据算法自动选择低延时的线路进行连接"));

    m_privateModeCheckBox = new QtETCheckBtn(optionsWidget);
    m_privateModeCheckBox->setText(tr("私有模式"));
    m_privateModeCheckBox->setChecked(true);
    m_privateModeCheckBox->setBorderless(true);
    m_privateModeCheckBox->setMaximumWidth(200);
    m_privateModeCheckBox->setToolTip(tr("开启后,不允许网络号和密码不相同的节点使用本节点进行数据中转"));

    optionsLayout->addWidget(m_latencyFirstCheckBox, 1);
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
    m_publicServerBtn = new QPushButton(tr("服务器收藏列表"), serverWidget);
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

    // 设置服务器列表的字体为 UbuntuMono
    int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/icons/UbuntuMono-B.ttf"));
    if (fontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            QFont monoFont(fontFamilies.first());
            monoFont.setPointSize(11);
            m_serverListWidget->setFont(monoFont);
        }
    }

    // 连接信号槽
    connect(m_dhcpCheckBox, &QtETCheckBtn::toggled, this, [this](bool checked) {
        m_ipv4Edit->setEnabled(!checked);
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

    // 安装平滑滚动事件过滤器到 viewport（滚轮事件由 viewport 接收）
    SmoothScrollFilter *smoothFilter = new SmoothScrollFilter(scrollArea, scrollArea);
    scrollArea->viewport()->installEventFilter(smoothFilter);

    // 创建滚动区内容部件
    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置全局字体大小为10px
    QFont contentFont = scrollContent->font();
    contentFont.setPointSize(10);
    scrollContent->setFont(contentFont);

    // 创建垂直布局
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(10, 10, 10, 10);
    scrollLayout->setSpacing(8);

    // ========== 功能开关区域（每行两个）==========
    QWidget *functionWidget = new QWidget(scrollContent);
    QGridLayout *functionGridLayout = new QGridLayout(functionWidget);
    functionGridLayout->setHorizontalSpacing(6);
    functionGridLayout->setVerticalSpacing(6);

    // 初始化功能开关控件
    m_enableKcpProxyCheckBox = new QtETCheckBtn(functionWidget);
    m_enableKcpProxyCheckBox->setText(tr("启用 KCP 代理"));
    m_enableKcpProxyCheckBox->setChecked(true);
    m_enableKcpProxyCheckBox->setToolTip(tr("使用UDP协议代理TCP流量,启用以获得更好的UDP P2P效果"));
    m_enableKcpProxyCheckBox->setBriefTip(tr("允许使用 KCP 协议发包"));

    m_disableKcpInputCheckBox = new QtETCheckBtn(functionWidget);
    m_disableKcpInputCheckBox->setText(tr("禁用 KCP 输入"));
    m_disableKcpInputCheckBox->setChecked(false);
    m_disableKcpInputCheckBox->setToolTip(tr("不接受通过KCP协议传输的流量"));
    m_disableKcpInputCheckBox->setBriefTip(tr("不接收 KCP 协议的流量"));

    m_noTunCheckBox = new QtETCheckBtn(functionWidget);
    m_noTunCheckBox->setText(tr("无 TUN 模式"));
    m_noTunCheckBox->setChecked(false);
    m_noTunCheckBox->setToolTip(tr("不创建TUN网卡,开启时本节点无法主动访问其他节点,只能被动访问"));
    m_noTunCheckBox->setBriefTip(tr("不创建 TUN 虚拟网卡"));

    m_enableQuicProxyCheckBox = new QtETCheckBtn(functionWidget);
    m_enableQuicProxyCheckBox->setText(tr("启用 QUIC 代理"));
    m_enableQuicProxyCheckBox->setChecked(false);
    m_enableQuicProxyCheckBox->setToolTip(tr("QUIC是一个由Google设计,基于UDP的新一代可靠传输层协议,启用以获得更好的UDP P2P效果"));
    m_enableQuicProxyCheckBox->setBriefTip(tr("允许使用 QUIC 协议发包"));

    m_disableQuicInputCheckBox = new QtETCheckBtn(functionWidget);
    m_disableQuicInputCheckBox->setText(tr("禁用 QUIC 输入"));
    m_disableQuicInputCheckBox->setChecked(false);
    m_disableQuicInputCheckBox->setToolTip(tr("不接受通过 QUIC 协议传输的流量"));
    m_disableQuicInputCheckBox->setBriefTip(tr("不接收 QUIC 协议的流量"));

    m_disableUdpHolePunchingCheckBox = new QtETCheckBtn(functionWidget);
    m_disableUdpHolePunchingCheckBox->setText(tr("禁用 UDP 打洞"));
    m_disableUdpHolePunchingCheckBox->setChecked(false);
    m_disableUdpHolePunchingCheckBox->setToolTip(tr("禁用UDP打洞,仅允许通过TCP进行P2P访问"));
    m_disableUdpHolePunchingCheckBox->setBriefTip(tr("不使用 UDP 协议打洞"));

    m_multiThreadCheckBox = new QtETCheckBtn(functionWidget);
    m_multiThreadCheckBox->setText(tr("启用多线程"));
    m_multiThreadCheckBox->setChecked(true);
    m_multiThreadCheckBox->setToolTip(tr("后端启用多线程,可提升组网性能,但可能会增加占用"));
    m_multiThreadCheckBox->setBriefTip(tr("使用多线程优化提升组网性能"));

    m_useSmoltcpCheckBox = new QtETCheckBtn(functionWidget);
    m_useSmoltcpCheckBox->setText(tr("使用用户态协议栈"));
    m_useSmoltcpCheckBox->setChecked(false);
    m_useSmoltcpCheckBox->setToolTip(tr("使用用户态协议栈,默认使用系统协议栈"));
    m_useSmoltcpCheckBox->setBriefTip(tr("默认使用系统协议栈"));

    m_bindDeviceCheckBox = new QtETCheckBtn(functionWidget);
    m_bindDeviceCheckBox->setText(tr("仅使用物理网卡"));
    m_bindDeviceCheckBox->setChecked(true);
    m_bindDeviceCheckBox->setToolTip(tr("仅使用物理网卡与其他节点建立P2P连接"));
    m_bindDeviceCheckBox->setBriefTip(tr("不通过其他虚拟网卡进行连接"));

    m_disableP2pCheckBox = new QtETCheckBtn(functionWidget);
    m_disableP2pCheckBox->setText(tr("禁用 P2P"));
    m_disableP2pCheckBox->setChecked(false);
    m_disableP2pCheckBox->setToolTip(tr("流量需要经过你添加的中转服务器,不直接与其他节点建立P2P连接"));
    m_disableP2pCheckBox->setBriefTip(tr("流量只从添加的节点中转"));

    m_enableExitNodeCheckBox = new QtETCheckBtn(functionWidget);
    m_enableExitNodeCheckBox->setText(tr("启用出口节点"));
    m_enableExitNodeCheckBox->setChecked(false);
    m_enableExitNodeCheckBox->setToolTip(tr("本节点可作为VPN的出口节点, 可被用于端口转发"));
    m_enableExitNodeCheckBox->setBriefTip(tr("请慎用该功能"));

    m_systemForwardingCheckBox = new QtETCheckBtn(functionWidget);
    m_systemForwardingCheckBox->setText(tr("系统转发"));
    m_systemForwardingCheckBox->setChecked(false);
    m_systemForwardingCheckBox->setToolTip(tr("通过系统内核转发子网代理数据包，禁用内置NAT"));
    m_systemForwardingCheckBox->setBriefTip(tr("通过系统内核转发子网数据包"));

    m_disableSymHolePunchingCheckBox = new QtETCheckBtn(functionWidget);
    m_disableSymHolePunchingCheckBox->setText(tr("禁用对称 NAT 打洞"));
    m_disableSymHolePunchingCheckBox->setChecked(false);
    m_disableSymHolePunchingCheckBox->setBriefTip(tr("在对称 NAT 环境下打洞可能失败"));

    m_disableIpv6CheckBox = new QtETCheckBtn(functionWidget);
    m_disableIpv6CheckBox->setText(tr("禁用 IPv6"));
    m_disableIpv6CheckBox->setChecked(false);
    m_disableIpv6CheckBox->setToolTip(tr("禁用IPv6连接, 仅使用IPv4"));
    m_disableIpv6CheckBox->setBriefTip(tr("虚拟网络内禁用 IPv6 通信。"));

    m_relayAllPeerRpcCheckBox = new QtETCheckBtn(functionWidget);
    m_relayAllPeerRpcCheckBox->setText(tr("转发 RPC 包"));
    m_relayAllPeerRpcCheckBox->setChecked(false);
    m_relayAllPeerRpcCheckBox->setToolTip(tr("转发其他节点的网络配置,不论该节点是否在网络白名单中, 帮助其他节点建立P2P连接"));
    m_relayAllPeerRpcCheckBox->setBriefTip(tr("帮助其他节点建立P2P连接"));

    m_enableEncryptionCheckBox = new QtETCheckBtn(functionWidget);
    m_enableEncryptionCheckBox->setText(tr("启用加密"));
    m_enableEncryptionCheckBox->setChecked(true);
    m_enableEncryptionCheckBox->setToolTip(tr("启用本节点通信的加密，必须与对等节点相同,正常情况下请保持加密"));
    m_enableEncryptionCheckBox->setBriefTip(tr("正常情况下请保持加密"));

    m_acceptDnsCheckBox = new QtETCheckBtn(functionWidget);
    m_acceptDnsCheckBox->setText(tr("启用魔法 DNS"));
    m_acceptDnsCheckBox->setChecked(false);
    m_acceptDnsCheckBox->setToolTip(tr("启用后可通过\"用户名.et.net\"访问本节点\n注意：Linux用户需要手动配置 DNS 服务器为100.100.100.101"));
    m_acceptDnsCheckBox->setBriefTip(tr("通过\"用户名.et.net\"访问本节点"));

    // 将所有功能开关添加到列表中
    m_functionCheckBoxes.clear();
    m_functionCheckBoxes.append(m_enableKcpProxyCheckBox);
    m_functionCheckBoxes.append(m_disableKcpInputCheckBox);
    m_functionCheckBoxes.append(m_enableQuicProxyCheckBox);
    m_functionCheckBoxes.append(m_disableQuicInputCheckBox);
    m_functionCheckBoxes.append(m_noTunCheckBox);
    m_functionCheckBoxes.append(m_disableUdpHolePunchingCheckBox);
    m_functionCheckBoxes.append(m_multiThreadCheckBox);
    m_functionCheckBoxes.append(m_useSmoltcpCheckBox);
    m_functionCheckBoxes.append(m_disableP2pCheckBox);
    m_functionCheckBoxes.append(m_bindDeviceCheckBox);
    m_functionCheckBoxes.append(m_enableExitNodeCheckBox);
    m_functionCheckBoxes.append(m_systemForwardingCheckBox);
    m_functionCheckBoxes.append(m_disableSymHolePunchingCheckBox);
    m_functionCheckBoxes.append(m_disableIpv6CheckBox);
    m_functionCheckBoxes.append(m_relayAllPeerRpcCheckBox);
    m_functionCheckBoxes.append(m_enableEncryptionCheckBox);
    m_functionCheckBoxes.append(m_acceptDnsCheckBox);

    // 存储容器和布局
    m_functionWidget = functionWidget;
    m_functionGridLayout = functionGridLayout;

    // 初始化网格布局
    updateFunctionGridLayout();

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

    m_foreignNetworkWhitelistCheckBox = new QtETCheckBtn(whitelistWidget);
    m_foreignNetworkWhitelistCheckBox->setText(tr("启用网络白名单"));
    m_foreignNetworkWhitelistCheckBox->setToolTip(tr("仅转发网络白名单中VPN的流量, 留空则为不转发任何网络的流量"));
    m_foreignNetworkWhitelistCheckBox->setChecked(false);
    m_foreignNetworkWhitelistCheckBox->setBorderless(true);
    m_foreignNetworkWhitelistCheckBox->setMaximumWidth(160);
    whitelistLayout->addWidget(m_foreignNetworkWhitelistCheckBox);

    // 白名单控件容器
    QWidget *whitelistControlsWidget = new QWidget(whitelistWidget);
    QVBoxLayout *whitelistControlsLayout = new QVBoxLayout(whitelistControlsWidget);
    whitelistControlsLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *addWhitelistLayout = new QHBoxLayout();
    m_foreignNetworkWhitelistEdit = new QLineEdit(whitelistControlsWidget);
    m_foreignNetworkWhitelistEdit->setPlaceholderText(tr("请输入网络名称"));
    m_addWhitelistBtn = new QPushButton(tr("添加"), whitelistControlsWidget);
    m_addWhitelistBtn->setMinimumWidth(80);
    addWhitelistLayout->addWidget(m_foreignNetworkWhitelistEdit, 1);
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
    m_proxyNetworkEdit = new QLineEdit(cidrWidget);
    m_proxyNetworkEdit->setPlaceholderText(tr("请输入子网代理 CIDR"));
    m_addProxyNetworkBtn = new QPushButton(tr("添加"), cidrWidget);
    m_addProxyNetworkBtn->setMinimumWidth(80);
    addCidrLayout->addWidget(m_proxyNetworkEdit, 1);
    addCidrLayout->addWidget(m_addProxyNetworkBtn);
    cidrLayout->addLayout(addCidrLayout);

    QHBoxLayout *cidrListLayout = new QHBoxLayout();
    m_proxyNetworkListWidget = new QListWidget(cidrWidget);
    m_proxyNetworkListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_removeProxyNetworkBtn = new QPushButton(tr("删除"), cidrWidget);
    m_removeProxyNetworkBtn->setMinimumWidth(80);
    m_removeProxyNetworkBtn->setEnabled(false);
    cidrListLayout->addWidget(m_proxyNetworkListWidget, 1);
    cidrListLayout->addWidget(m_removeProxyNetworkBtn);
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

    // 设置列表控件的字体为 UbuntuMono
    int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/icons/UbuntuMono-B.ttf"));
    if (fontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            QFont monoFont(fontFamilies.first());
            monoFont.setPointSize(11);
            m_listenAddrListWidget->setFont(monoFont);
            m_whitelistListWidget->setFont(monoFont);
            m_proxyNetworkListWidget->setFont(monoFont);
        }
    }

    // 连接信号槽
    connect(m_foreignNetworkWhitelistCheckBox, &QtETCheckBtn::toggled, this, [this, whitelistControlsWidget](bool checked) {
        whitelistControlsWidget->setVisible(checked);
        m_foreignNetworkWhitelistEdit->setEnabled(checked);
        m_addWhitelistBtn->setEnabled(checked);
        m_whitelistListWidget->setEnabled(checked);
    });
    connect(m_listenAddrListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeListenAddrBtn->setEnabled(m_listenAddrListWidget->selectedItems().count() > 0);
    });
    connect(m_whitelistListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        if (m_foreignNetworkWhitelistCheckBox->isChecked()) {
            m_removeWhitelistBtn->setEnabled(m_whitelistListWidget->selectedItems().count() > 0);
        }
    });
    connect(m_proxyNetworkListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeProxyNetworkBtn->setEnabled(m_proxyNetworkListWidget->selectedItems().count() > 0);
    });
}

void QtETNetwork::initRunningStatusPage()
{
    QVBoxLayout *layout = new QVBoxLayout(m_runningStatusTab);
    layout->setContentsMargins(10, 20, 10, 20);

    m_statusLabel = new QLabel(tr("请先点击运行网络"), m_runningStatusTab);
    QFont titleLabelFont = m_statusLabel->font();
    titleLabelFont.setPointSize(12);
    titleLabelFont.setWeight(QFont::Bold);
    m_statusLabel->setFont(titleLabelFont);
    layout->addWidget(m_statusLabel);

    // 创建节点信息容器（带滚动区域）
    QScrollArea *scrollArea = new QScrollArea(m_runningStatusTab);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_nodeInfoContainer = new QWidget(scrollArea);
    m_nodeInfoLayout = new QVBoxLayout(m_nodeInfoContainer);
    m_nodeInfoLayout->setContentsMargins(0, 10, 0, 0);
    m_nodeInfoLayout->setSpacing(8);

    // 空状态提示标签
    m_emptyLabel = new QLabel(tr("空空如也"), m_nodeInfoContainer);
    QFont emptyFont = m_emptyLabel->font();
    emptyFont.setPointSize(24);
    m_emptyLabel->setFont(emptyFont);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #888888;"));
    m_emptyLabel->hide();  // 默认隐藏
    m_nodeInfoLayout->addWidget(m_emptyLabel, 1);

    m_nodeInfoLayout->addStretch();

    scrollArea->setWidget(m_nodeInfoContainer);
    layout->addWidget(scrollArea);
}

void QtETNetwork::initRunningLogPage()
{
    QVBoxLayout *layout = new QVBoxLayout(m_runningLogTab);
    layout->setContentsMargins(20, 20, 20, 20);

    m_logLabel = new QLabel(tr("运行日志页面（待实现）"), m_runningLogTab);
    m_logLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_logLabel);
}

void QtETNetwork::updateFunctionGridLayout()
{
    if (!m_functionGridLayout || m_functionCheckBoxes.isEmpty()) {
        return;
    }

    // 根据宽度计算列数
    const int width = m_functionWidget ? m_functionWidget->width() : 0;
    int columns = 2;  // 默认每行 2 个

    if (width > 1020) {
        columns = 5;
    } else if (width > 820) {
        columns = 4;
    } else if (width > 620) {
        columns = 3;
    }

    // 移除所有控件从布局中
    for (QtETCheckBtn *checkBox : m_functionCheckBoxes) {
        m_functionGridLayout->removeWidget(checkBox);
    }

    // 重新添加控件
    int row = 0;
    int col = 0;
    for (QtETCheckBtn *checkBox : m_functionCheckBoxes) {
        m_functionGridLayout->addWidget(checkBox, row, col);
        ++col;
        if (col >= columns) {
            col = 0;
            ++row;
        }
    }
}

void QtETNetwork::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 更新功能开关网格布局
    updateFunctionGridLayout();
}

// ==================== 网络配置管理方法 ====================

void QtETNetwork::onNewNetwork()
{
    // 创建新的网络配置（构造函数会自动初始化 instanceName 和默认值）
    NetworkConf newConf;
    
    // 添加到配置列表
    m_networkConfs.push_back(newConf);
    
    // 在列表中添加新项
    int index = static_cast<int>(m_networkConfs.size()) - 1;
    QString displayName = tr("网络 %1").arg(index + 1);
    m_networksList->addItem(displayName);
    
    // 选中新添加的项
    m_networksList->setCurrentRow(index);
    
    // 更新 TabWidget 状态
    updateTabWidgetState();
}

void QtETNetwork::onNetworkSelected()
{
    int currentRow = m_networksList->currentRow();
    
    // 更新 TabWidget 状态
    updateTabWidgetState();
    
    // 如果有选中的项，加载对应的配置到UI
    if (currentRow >= 0 && currentRow < static_cast<int>(m_networkConfs.size())) {
        loadConfToUI(currentRow);
        
        // 清空当前节点信息显示
        for (QtETNodeInfo *widget : m_nodeInfoWidgets) {
            m_nodeInfoLayout->removeWidget(widget);
            widget->deleteLater();
        }
        m_nodeInfoWidgets.clear();
        
        // 根据网络运行状态更新标签
        if (m_networkConfs[currentRow].isRunning()) {
            m_statusLabel->setText(tr("节点列表: 加载中..."));
            m_emptyLabel->hide();
            // 立即请求更新节点信息
            if (m_runWorker) {
                QMetaObject::invokeMethod(m_runWorker, "collectInfos", Qt::QueuedConnection);
            }
        } else {
            m_statusLabel->setText(tr("请先点击运行网络"));
            m_emptyLabel->hide();
        }
    }
    
    // 更新运行按钮样式
    updateRunButtonStyle();
}

void QtETNetwork::onListContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_networksList->itemAt(pos);
    if (!item) {
        return;
    }
    
    QMenu contextMenu(tr("网络操作"), this);
    QAction *deleteAction = contextMenu.addAction(tr("删除网络"));
    deleteAction->setIcon(QIcon(QStringLiteral(":/icons/net-page.svg")));
    
    QAction *selectedAction = contextMenu.exec(m_networksList->mapToGlobal(pos));
    
    if (selectedAction == deleteAction) {
        onDeleteNetwork();
    }
}

void QtETNetwork::onDeleteNetwork()
{
    int currentRow = m_networksList->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_networkConfs.size())) {
        return;
    }
    
    // 弹出确认框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("确认删除"));
    msgBox.setText(tr("确定要删除网络 \"%1\" 吗？").arg(m_networksList->item(currentRow)->text()));
    msgBox.setInformativeText(tr("删除后无法恢复。"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    
    if (msgBox.exec() != QMessageBox::Ok) {
        return;
    }
    
    // 从配置列表中删除
    m_networkConfs.erase(m_networkConfs.begin() + currentRow);
    
    // 从列表中删除
    delete m_networksList->takeItem(currentRow);
    
    // 更新列表项显示名称（索引值可能变化）
    for (int i = 0; i < m_networksList->count(); ++i) {
        m_networksList->item(i)->setText(tr("网络 %1").arg(i + 1));
    }
    
    // 更新 TabWidget 状态
    updateTabWidgetState();
    
    // 如果还有网络，选中第一个
    if (m_networksList->count() > 0) {
        m_networksList->setCurrentRow(0);
    }
}

void QtETNetwork::onUIChanged()
{
    int currentRow = m_networksList->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(m_networkConfs.size())) {
        saveConfFromUI(currentRow);
    }
}

void QtETNetwork::loadConfToUI(const int& index) const
{
    if (index < 0 || index >= static_cast<int>(m_networkConfs.size())) {
        return;
    }
    
    const NetworkConf &conf = m_networkConfs[index];
    
    // 阻塞信号，避免循环触发
    m_hostnameEdit->blockSignals(true);
    m_networkNameEdit->blockSignals(true);
    m_networkSecretEdit->blockSignals(true);
    m_dhcpCheckBox->blockSignals(true);
    m_ipv4Edit->blockSignals(true);
    m_latencyFirstCheckBox->blockSignals(true);
    m_privateModeCheckBox->blockSignals(true);
    m_serverListWidget->blockSignals(true);
    m_rpcPortEdit->blockSignals(true);
    m_foreignNetworkWhitelistCheckBox->blockSignals(true);
    m_whitelistListWidget->blockSignals(true);
    m_listenAddrListWidget->blockSignals(true);
    m_proxyNetworkListWidget->blockSignals(true);
    m_enableKcpProxyCheckBox->blockSignals(true);
    m_disableKcpInputCheckBox->blockSignals(true);
    m_noTunCheckBox->blockSignals(true);
    m_enableQuicProxyCheckBox->blockSignals(true);
    m_disableQuicInputCheckBox->blockSignals(true);
    m_disableUdpHolePunchingCheckBox->blockSignals(true);
    m_multiThreadCheckBox->blockSignals(true);
    m_useSmoltcpCheckBox->blockSignals(true);
    m_bindDeviceCheckBox->blockSignals(true);
    m_disableP2pCheckBox->blockSignals(true);
    m_enableExitNodeCheckBox->blockSignals(true);
    m_systemForwardingCheckBox->blockSignals(true);
    m_disableSymHolePunchingCheckBox->blockSignals(true);
    m_disableIpv6CheckBox->blockSignals(true);
    m_relayAllPeerRpcCheckBox->blockSignals(true);
    m_enableEncryptionCheckBox->blockSignals(true);
    m_acceptDnsCheckBox->blockSignals(true);
    
    // 基础设置
    m_hostnameEdit->setText(QString::fromStdString(conf.m_hostname));
    m_networkNameEdit->setText(QString::fromStdString(conf.m_networkName));
    m_networkSecretEdit->setText(QString::fromStdString(conf.m_networkSecret));
    m_dhcpCheckBox->setChecked(conf.m_dhcp);
    m_ipv4Edit->setText(QString::fromStdString(conf.m_ipv4));
    m_ipv4Edit->setEnabled(!conf.m_dhcp);
    m_latencyFirstCheckBox->setChecked(conf.m_latencyFirst);
    m_privateModeCheckBox->setChecked(conf.m_privateMode);
    
    // 服务器列表
    m_serverListWidget->clear();
    for (const auto &server : conf.m_servers) {
        m_serverListWidget->addItem(QString::fromStdString(server));
    }
    
    // 高级设置 - 功能开关
    m_enableKcpProxyCheckBox->setChecked(conf.m_enableKcpProxy);
    m_disableKcpInputCheckBox->setChecked(conf.m_disableKcpInput);
    m_noTunCheckBox->setChecked(conf.m_noTun);
    m_enableQuicProxyCheckBox->setChecked(conf.m_enableQuicProxy);
    m_disableQuicInputCheckBox->setChecked(conf.m_disableQuicInput);
    m_disableUdpHolePunchingCheckBox->setChecked(conf.m_disableUdpHolePunching);
    m_multiThreadCheckBox->setChecked(conf.m_multiThread);
    m_useSmoltcpCheckBox->setChecked(conf.m_useSmoltcp);
    m_bindDeviceCheckBox->setChecked(conf.m_bindDevice);
    m_disableP2pCheckBox->setChecked(conf.m_disableP2p);
    m_enableExitNodeCheckBox->setChecked(conf.m_enableExitNode);
    m_systemForwardingCheckBox->setChecked(conf.m_systemForwarding);
    m_disableSymHolePunchingCheckBox->setChecked(conf.m_disableSymHolePunching);
    m_disableIpv6CheckBox->setChecked(conf.m_disableIpv6);
    m_relayAllPeerRpcCheckBox->setChecked(conf.m_relayAllPeerRpc);
    m_enableEncryptionCheckBox->setChecked(conf.m_enableEncryption);
    m_acceptDnsCheckBox->setChecked(conf.m_acceptDns);
    
    // RPC 端口
    m_rpcPortEdit->setText(QString::number(conf.m_rpcPort));
    
    // 网络白名单
    m_foreignNetworkWhitelistCheckBox->setChecked(conf.m_foreignNetworkWhitelistEnabled);
    m_whitelistListWidget->clear();
    for (const auto &item : conf.m_foreignNetworkWhitelist) {
        m_whitelistListWidget->addItem(QString::fromStdString(item));
    }
    
    // 监听地址
    m_listenAddrListWidget->clear();
    for (const auto &addr : conf.m_listenAddresses) {
        m_listenAddrListWidget->addItem(QString::fromStdString(addr));
    }
    
    // 子网代理 CIDR
    m_proxyNetworkListWidget->clear();
    for (const auto &cidr : conf.m_proxyNetworks) {
        m_proxyNetworkListWidget->addItem(QString::fromStdString(cidr));
    }
    
    // 恢复信号
    m_hostnameEdit->blockSignals(false);
    m_networkNameEdit->blockSignals(false);
    m_networkSecretEdit->blockSignals(false);
    m_dhcpCheckBox->blockSignals(false);
    m_ipv4Edit->blockSignals(false);
    m_latencyFirstCheckBox->blockSignals(false);
    m_privateModeCheckBox->blockSignals(false);
    m_serverListWidget->blockSignals(false);
    m_rpcPortEdit->blockSignals(false);
    m_foreignNetworkWhitelistCheckBox->blockSignals(false);
    m_whitelistListWidget->blockSignals(false);
    m_listenAddrListWidget->blockSignals(false);
    m_proxyNetworkListWidget->blockSignals(false);
    m_enableKcpProxyCheckBox->blockSignals(false);
    m_disableKcpInputCheckBox->blockSignals(false);
    m_noTunCheckBox->blockSignals(false);
    m_enableQuicProxyCheckBox->blockSignals(false);
    m_disableQuicInputCheckBox->blockSignals(false);
    m_disableUdpHolePunchingCheckBox->blockSignals(false);
    m_multiThreadCheckBox->blockSignals(false);
    m_useSmoltcpCheckBox->blockSignals(false);
    m_bindDeviceCheckBox->blockSignals(false);
    m_disableP2pCheckBox->blockSignals(false);
    m_enableExitNodeCheckBox->blockSignals(false);
    m_systemForwardingCheckBox->blockSignals(false);
    m_disableSymHolePunchingCheckBox->blockSignals(false);
    m_disableIpv6CheckBox->blockSignals(false);
    m_relayAllPeerRpcCheckBox->blockSignals(false);
    m_enableEncryptionCheckBox->blockSignals(false);
    m_acceptDnsCheckBox->blockSignals(false);
}

void QtETNetwork::saveConfFromUI(const int &index)
{
    if (index < 0 || index >= static_cast<int>(m_networkConfs.size())) {
        return;
    }
    // 直接调用 readFromUI 读取当前 UI 数据
    m_networkConfs[index].readFromUI(this);
}

void QtETNetwork::updateTabWidgetState() const
{
    const bool hasSelection = m_networksList->currentRow() >= 0 && m_networksList->count() > 0;
    m_tabWidget->setEnabled(hasSelection);
}

void QtETNetwork::setupUIConnections()
{
    // 基础设置控件
    connect(m_hostnameEdit, &QLineEdit::textChanged, this, &QtETNetwork::onUIChanged);
    connect(m_networkNameEdit, &QLineEdit::textChanged, this, &QtETNetwork::onUIChanged);
    connect(m_networkSecretEdit, &QLineEdit::textChanged, this, &QtETNetwork::onUIChanged);
    connect(m_dhcpCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_ipv4Edit, &QLineEdit::textChanged, this, &QtETNetwork::onUIChanged);
    connect(m_latencyFirstCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_privateModeCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    
    // 服务器列表变化
    connect(m_addServerBtn, &QPushButton::clicked, this, [this]() {
        QString server = m_serverEdit->text().trimmed();
        if (!server.isEmpty()) {
            m_serverListWidget->addItem(server);
            m_serverEdit->clear();
            onUIChanged();
        }
    });
    connect(m_removeServerBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = m_serverListWidget->currentItem();
        if (item) {
            delete m_serverListWidget->takeItem(m_serverListWidget->row(item));
            onUIChanged();
        }
    });
    
    // 高级设置 - 功能开关
    connect(m_enableKcpProxyCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_disableKcpInputCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_noTunCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_enableQuicProxyCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_disableQuicInputCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_disableUdpHolePunchingCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_multiThreadCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_useSmoltcpCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_bindDeviceCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_disableP2pCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_enableExitNodeCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_systemForwardingCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_disableSymHolePunchingCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_disableIpv6CheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_relayAllPeerRpcCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_enableEncryptionCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_acceptDnsCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    
    // RPC 端口
    connect(m_rpcPortEdit, &QLineEdit::textChanged, this, &QtETNetwork::onUIChanged);
    
    // 网络白名单
    connect(m_foreignNetworkWhitelistCheckBox, &QtETCheckBtn::toggled, this, &QtETNetwork::onUIChanged);
    connect(m_addWhitelistBtn, &QPushButton::clicked, this, [this]() {
        QString item = m_foreignNetworkWhitelistEdit->text().trimmed();
        if (!item.isEmpty()) {
            m_whitelistListWidget->addItem(item);
            m_foreignNetworkWhitelistEdit->clear();
            onUIChanged();
        }
    });
    connect(m_removeWhitelistBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = m_whitelistListWidget->currentItem();
        if (item) {
            delete m_whitelistListWidget->takeItem(m_whitelistListWidget->row(item));
            onUIChanged();
        }
    });
    
    // 监听地址
    connect(m_addListenAddrBtn, &QPushButton::clicked, this, [this]() {
        QString addr = m_listenAddrEdit->text().trimmed();
        if (!addr.isEmpty()) {
            m_listenAddrListWidget->addItem(addr);
            m_listenAddrEdit->clear();
            onUIChanged();
        }
    });
    connect(m_removeListenAddrBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = m_listenAddrListWidget->currentItem();
        if (item) {
            delete m_listenAddrListWidget->takeItem(m_listenAddrListWidget->row(item));
            onUIChanged();
        }
    });
    
    // 子网代理 CIDR
    connect(m_addProxyNetworkBtn, &QPushButton::clicked, this, [this]() {
        QString cidr = m_proxyNetworkEdit->text().trimmed();
        if (!cidr.isEmpty()) {
            m_proxyNetworkListWidget->addItem(cidr);
            m_proxyNetworkEdit->clear();
            onUIChanged();
        }
    });
    connect(m_removeProxyNetworkBtn, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = m_proxyNetworkListWidget->currentItem();
        if (item) {
            delete m_proxyNetworkListWidget->takeItem(m_proxyNetworkListWidget->row(item));
            onUIChanged();
        }
    });
}

void QtETNetwork::loadAllNetworkConfs()
{
    // 清空现有数据
    m_networksList->clear();
    m_networkConfs.clear();
    
    // 从文件读取配置
    m_networkConfs = ::readAllNetworkConf();
    
    // 从 FFI 获取实际运行的网络实例列表
    std::vector<EasyTierFFI::KVPair> runningInstances;
    size_t maxLen = MAX_NETWORK_INSTANCES;
    int runningCount = EasyTierFFI::collectNetworkInfos(runningInstances, maxLen);
    
    // 同步运行状态：根据 FFI 实际运行状态更新配置
    if (runningCount > 0) {
        // 收集所有运行中的 instanceName
        std::set<std::string> runningInstanceNames;
        for (const auto &info : runningInstances) {
            runningInstanceNames.insert(info.key);
        }
        
        // 更新每个配置的运行状态
        for (auto &conf : m_networkConfs) {
            conf.setRunning(runningInstanceNames.contains(conf.getInstanceName()));
        }
    } else {
        // 没有运行中的实例，所有配置都标记为未运行
        for (auto &conf : m_networkConfs) {
            conf.setRunning(false);
        }
    }
    
    // 添加到列表
    for (size_t i = 0; i < m_networkConfs.size(); ++i) {
        QString displayName = tr("网络 %1").arg(i + 1);
        m_networksList->addItem(displayName);
    }
    
    // 更新所有列表项的样式
    updateAllListItemStyles();
    
    // 更新 TabWidget 状态
    updateTabWidgetState();
    
    // 如果有网络配置，默认选中第一项
    if (m_networksList->count() > 0) {
        m_networksList->setCurrentRow(0);
    }
    
    // 更新运行按钮样式
    updateRunButtonStyle();
}

void QtETNetwork::saveAllNetworkConfs() const
{
    // 保存到文件
    QString errorMsg;
    if (!::saveAllNetworkConf(m_networkConfs, &errorMsg)) {
        qWarning() << "保存网络配置失败:" << errorMsg;
    }
}

void QtETNetwork::onExportConf()
{
    // 检查是否有选中的配置
    int currentRow = m_networksList->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_networkConfs.size())) {
        QMessageBox::information(this, tr("提示"), tr("请先选择一个配置"));
        return;
    }
    
    // 获取用户主目录
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString defaultPath = homePath + "/qtet-config.json";
    
    // 获取保存文件路径
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("导出配置"),
        defaultPath,
        tr("JSON 文件 (*.json);;所有文件 (*)")
    );
    
    if (filePath.isEmpty()) {
        return;  // 用户取消了选择
    }
    
    // 获取当前配置的 JSON 对象，并删除 hostname 和 instance_name
    QJsonObject json = m_networkConfs[currentRow].toJson();
    json.remove("hostname");
    json.remove("instance_name");
    QJsonDocument doc(json);
    
    // 写入文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入文件: %1").arg(filePath));
        return;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    QMessageBox::information(this, tr("导出成功"), tr("配置已导出到: %1").arg(filePath));
}

void QtETNetwork::onImportConf()
{
    // 获取用户主目录
    const QString &homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    
    // 选择要导入的文件
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("导入配置"),
        homePath,
        tr("JSON 文件 (*.json);;所有文件 (*)")
    );
    
    if (filePath.isEmpty()) {
        return;  // 用户取消了选择
    }
    
    // 读取文件内容
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("导入失败"), tr("无法读取文件: %1").arg(filePath));
        return;
    }

    const QByteArray &jsonData = file.readAll();
    file.close();
    
    // 解析 JSON
    QJsonParseError parseError;
    const QJsonDocument &doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("导入失败"), tr("JSON 解析错误: %1").arg(parseError.errorString()));
        return;
    }
    
    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("导入失败"), tr("配置文件格式错误"));
        return;
    }
    
    // 创建新的网络配置并导入（构造函数会自动生成 instanceName）
    NetworkConf newConf;
    newConf.readFromJson(doc.object());
    
    // 添加到配置列表
    m_networkConfs.push_back(newConf);
    
    // 在列表中添加新项
    const int &index = static_cast<int>(m_networkConfs.size()) - 1;
    const QString &displayName = tr("网络 %1").arg(index + 1);
    m_networksList->addItem(displayName);
    
    // 选中新添加的项
    m_networksList->setCurrentRow(index);
    
    // 更新 TabWidget 状态
    updateTabWidgetState();
    
    QMessageBox::information(this, tr("导入成功"), tr("配置已成功导入"));
}

void QtETNetwork::onRunNetworkBtnClicked()
{
    const int &currentRow = m_networksList->currentRow();
    
    // 检查是否有选中的网络
    if (currentRow < 0 || currentRow >= static_cast<int>(m_networkConfs.size())) {
        QMessageBox::information(this, tr("提示"), tr("请先选择一个网络"));
        return;
    }
    
    NetworkConf &conf = m_networkConfs[currentRow];
    
    // 根据运行状态决定是启动还是停止
    if (conf.isRunning()) {
        // 停止网络
        onRunNetworkBtnClicked_Stop(conf);
    } else {
        // 启动网络
        onRunNetworkBtnClicked_Start(conf);
    }
}

void QtETNetwork::onRunNetworkBtnClicked_Start(const NetworkConf &conf)
{
    // 保存当前配置到 UI
    const int &currentRow = m_networksList->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(m_networkConfs.size())) {
        saveConfFromUI(currentRow);
    }
    
    // 生成 TOML 配置
    const std::string &tomlConfig = conf.toToml();
    
    // 创建进度对话框
    if (!m_progressDialog) {
        m_progressDialog = new QProgressDialog(this);
        m_progressDialog->setWindowModality(Qt::WindowModal);
        m_progressDialog->setCancelButton(nullptr);
        m_progressDialog->setRange(0, 0);  // 无限进度条
    }
    m_progressDialog->setLabelText(tr("正在启动网络 \"%1\"...").arg(QString::fromStdString(conf.m_networkName)));
    m_progressDialog->show();
    
    // 记录当前正在操作的实例名称（FFI 中 instance_name 是 m_instanceName）
    m_currentRunningInstName = conf.getInstanceName();
    
    // 调用 Worker 启动网络
    QMetaObject::invokeMethod(m_runWorker, "runNetwork", Qt::QueuedConnection,
                              Q_ARG(std::string, conf.getInstanceName()),
                              Q_ARG(std::string, tomlConfig));
}

void QtETNetwork::onRunNetworkBtnClicked_Stop(const NetworkConf &conf)
{
    // 创建进度对话框
    if (!m_progressDialog) {
        m_progressDialog = new QProgressDialog(this);
        m_progressDialog->setWindowModality(Qt::WindowModal);
        m_progressDialog->setCancelButton(nullptr);
        m_progressDialog->setRange(0, 0);  // 无限进度条
    }
    m_progressDialog->setLabelText(tr("正在停止网络 \"%1\"...").arg(QString::fromStdString(conf.m_networkName)));
    m_progressDialog->show();
    
    // 记录当前正在操作的实例名称
    m_currentRunningInstName = conf.getInstanceName();
    
    // 调用 Worker 停止网络（使用 instanceName 标识）
    QMetaObject::invokeMethod(m_runWorker, "stopNetwork", Qt::QueuedConnection,
                              Q_ARG(std::string, conf.getInstanceName()));
}

void QtETNetwork::onNetworkStarted(const std::string &instName, bool success, const std::string &errorMsg)
{
    // 关闭进度对话框
    if (m_progressDialog) {
        m_progressDialog->hide();
    }
    
    // 查找对应的网络配置（使用 instanceName 匹配）
    for (size_t i = 0; i < m_networkConfs.size(); ++i) {
        if (m_networkConfs[i].getInstanceName() == instName) {
            if (success) {
                // 更新运行状态
                m_networkConfs[i].setRunning(true);
                
                // 增加运行网络计数并启动监测
                m_runningNetworkCount++;
                startNodeMonitor();
                
                // 更新列表项样式
                updateListItemStyle(static_cast<int>(i));
                
                // 更新按钮样式
                updateRunButtonStyle();
                
                // 保存配置
                saveAllNetworkConfs();
                
                QMessageBox::information(this, tr("启动成功"), 
                    tr("网络 \"%1\" 已成功启动").arg(QString::fromStdString(m_networkConfs[i].m_networkName)));
            } else {
                QMessageBox::warning(this, tr("启动失败"), 
                    tr("网络 \"%1\" 启动失败:\n%2").arg(QString::fromStdString(m_networkConfs[i].m_networkName)).arg(QString::fromStdString(errorMsg)));
            }
            return;
        }
    }
}

void QtETNetwork::onNetworkStopped(const std::string &instName, bool success, const std::string &errorMsg)
{
    // 关闭进度对话框
    if (m_progressDialog) {
        m_progressDialog->hide();
    }
    
    // 查找对应的网络配置（使用 instanceName 匹配）
    for (size_t i = 0; i < m_networkConfs.size(); ++i) {
        if (m_networkConfs[i].getInstanceName() == instName) {
            if (success) {
                // 更新运行状态
                m_networkConfs[i].setRunning(false);
                
                // 减少运行网络计数，如果没有运行的网络则停止监测
                m_runningNetworkCount--;
                if (m_runningNetworkCount <= 0) {
                    m_runningNetworkCount = 0;
                    stopNodeMonitor();
                }
                
                // 更新列表项样式
                updateListItemStyle(static_cast<int>(i));
                
                // 更新按钮样式
                updateRunButtonStyle();
                
                // 保存配置
                saveAllNetworkConfs();
                
                QMessageBox::information(this, tr("停止成功"), 
                    tr("网络 \"%1\" 已成功停止").arg(QString::fromStdString(m_networkConfs[i].m_networkName)));
            } else {
                QMessageBox::warning(this, tr("停止失败"), 
                    tr("网络 \"%1\" 停止失败:\n%2").arg(QString::fromStdString(m_networkConfs[i].m_networkName)).arg(QString::fromStdString(errorMsg)));
            }
            return;
        }
    }
}

void QtETNetwork::updateRunButtonStyle() const
{
    int currentRow = m_networksList->currentRow();
    
    if (currentRow < 0 || currentRow >= static_cast<int>(m_networkConfs.size())) {
        // 没有选中网络，恢复默认样式
        m_runNetworkBtn->setText(tr("运行网络"));
        m_runNetworkBtn->setStyleSheet(QString());
        return;
    }
    
    const NetworkConf &conf = m_networkConfs[currentRow];
    
    if (conf.isRunning()) {
        // 网络运行中，显示停止按钮样式
        m_runNetworkBtn->setText(tr("运行中"));
        m_runNetworkBtn->setStyleSheet(QStringLiteral(
            R"(QPushButton {
              color: #2ecc71;
              font-weight: bold;
            })"
        ));
    } else {
        // 网络未运行，恢复默认样式
        m_runNetworkBtn->setText(tr("运行网络"));
        m_runNetworkBtn->setStyleSheet(QString());
    }
}

void QtETNetwork::updateListItemStyle(const int &index) const
{
    if (index < 0 || index >= m_networksList->count()) {
        return;
    }
    
    QListWidgetItem *item = m_networksList->item(index);
    if (!item) {
        return;
    }
    
    if (index >= 0 && index < static_cast<int>(m_networkConfs.size())) {
        const NetworkConf &conf = m_networkConfs[index];
        
        if (conf.isRunning()) {
            // 运行中：绿色粗体
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
            item->setForeground(QColor(46, 204, 113));  // 绿色
        } else {
            // 未运行：恢复默认
            QFont font = item->font();
            font.setBold(false);
            item->setFont(font);
            item->setForeground(QColor());  // 默认颜色
        }
    }
}

void QtETNetwork::updateAllListItemStyles() const
{
    for (int i = 0; i < m_networksList->count(); ++i) {
        updateListItemStyle(i);
    }
}

QString QtETNetwork::ipAddrToString(quint32 addr)
{
    // 将 uint32 IP 地址转换为点分十进制字符串
    // addr 是网络字节序（大端），最高字节在前
    return QString("%1.%2.%3.%4")
        .arg((addr >> 24) & 0xFF)
        .arg((addr >> 16) & 0xFF)
        .arg((addr >> 8) & 0xFF)
        .arg(addr & 0xFF);
}

void QtETNetwork::parseAndUpdateNodeInfos(const QString &jsonStr)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }
    
    QJsonObject rootObj = doc.object();
    
    // 获取 routes 数组
    QJsonArray routes = rootObj["routes"].toArray();
    if (routes.isEmpty()) {
        return;
    }
    
    // 获取 peers 数组，用于判断直连/中转
    QJsonArray peers = rootObj["peers"].toArray();
    
    // 构建 peer_id 到连接信息的映射
    QHash<qint64, QStringList> peerConnMap;  // peer_id -> 协议列表
    QSet<qint64> directlyConnectedPeers;
    
    for (const QJsonValue &peerVal : peers) {
        QJsonObject peerObj = peerVal.toObject();
        qint64 peerId = peerObj["peer_id"].toVariant().toLongLong();
        directlyConnectedPeers.insert(peerId);
        
        // 获取该节点的所有连接协议（去重）
        QJsonArray conns = peerObj["conns"].toArray();
        QStringList protocols;
        QSet<QString> addedProtocols;  // 用于去重
        for (const QJsonValue &connVal : conns) {
            QJsonObject connObj = connVal.toObject();
            QString tunnelType = connObj["tunnel"].toObject()["tunnel_type"].toString();
            if (!tunnelType.isEmpty()) {
                QString proto = tunnelType.toUpper();
                if (!addedProtocols.contains(proto)) {
                    protocols.append(proto);
                    addedProtocols.insert(proto);
                }
            }
        }
        peerConnMap[peerId] = protocols;
    }
    
    // 构建新的节点信息列表
    QList<NodeInfo> newNodeInfos;
    
    // 解析本机节点信息 my_node_info
    QJsonObject myNodeInfo = rootObj["my_node_info"].toObject();
    qint64 myPeerId = myNodeInfo["peer_id"].toVariant().toLongLong();
    
    // 添加本机节点信息（放在第一个位置）
    if (!myNodeInfo.isEmpty()) {
        NodeInfo localInfo;
        localInfo.isLocalNode = true;
        
        // 解析本机虚拟 IP
        QJsonObject myIpv4 = myNodeInfo["virtual_ipv4"].toObject();
        QJsonObject myAddrObj = myIpv4["address"].toObject();
        quint32 myAddr = myAddrObj["addr"].toVariant().toUInt();
        localInfo.virtualIp = ipAddrToString(myAddr);
        
        // 解析本机 hostname
        localInfo.hostname = myNodeInfo["hostname"].toString();
        
        // 本机节点延迟为 0
        localInfo.latencyMs = 0;
        
        // 本机节点的协议从 listeners 获取
        QJsonArray listeners = myNodeInfo["listeners"].toArray();
        QStringList localProtocols;
        for (const QJsonValue &listenerVal : listeners) {
            QString url = listenerVal.toObject()["url"].toString();
            if (url.startsWith("tcp://") || url.startsWith("udp://")) {
                QString proto = url.section("://", 0, 0).toUpper();
                if (!localProtocols.contains(proto)) {
                    localProtocols.append(proto);
                }
            }
        }
        localInfo.protocol = localProtocols.join(",");
        
        // 本机节点显示为直连
        localInfo.connType = NodeConnType::Direct;
        localInfo.connMethod = QStringLiteral("Local");
        
        newNodeInfos.append(localInfo);
    }
    
    // 遍历 routes 构建其他节点信息
    for (const QJsonValue &routeVal : routes) {
        QJsonObject routeObj = routeVal.toObject();
        
        // 跳过本机节点
        qint64 peerId = routeObj["peer_id"].toVariant().toLongLong();
        if (peerId == myPeerId) {
            continue;
        }
        
        NodeInfo info;
        
        // 解析 IP 地址
        QJsonObject ipv4Addr = routeObj["ipv4_addr"].toObject();
        QJsonObject addressObj = ipv4Addr["address"].toObject();
        quint32 addr = addressObj["addr"].toVariant().toUInt();
        info.virtualIp = ipAddrToString(addr);
        
        // 解析 hostname
        info.hostname = routeObj["hostname"].toString();
        
        // 解析延迟（path_latency 单位是 ms）
        info.latencyMs = routeObj["path_latency"].toInt();
        
        // 判断是否为公共服务器
        QJsonObject featureFlag = routeObj["feature_flag"].toObject();
        bool isPublicServer = featureFlag["is_public_server"].toBool();
        
        // 判断是否直连
        bool isDirectlyConnected = directlyConnectedPeers.contains(peerId);
        
        if (isPublicServer) {
            info.connType = NodeConnType::Server;
            // 服务器如果是直连的，也显示协议
            if (isDirectlyConnected) {
                info.protocol = peerConnMap[peerId].join(",");
            }
        } else if (isDirectlyConnected) {
            info.connType = NodeConnType::Direct;
            // 获取直连节点的协议
            info.protocol = peerConnMap[peerId].join(",");
        } else {
            info.connType = NodeConnType::Relay;
        }
        
        // 设置连接方式
        if (info.connType == NodeConnType::Direct) {
            info.connMethod = QStringLiteral("P2P");
        } else if (info.connType == NodeConnType::Relay) {
            info.connMethod = QStringLiteral("Relay");
        } else {
            info.connMethod = QStringLiteral("Server");
        }
        
        newNodeInfos.append(info);
    }
    
    // 增量更新节点信息控件
    int oldCount = m_nodeInfoWidgets.size();
    int newCount = newNodeInfos.size();
    
    // 更新已存在的卡片信息
    for (int i = 0; i < qMin(oldCount, newCount); ++i) {
        m_nodeInfoWidgets[i]->setNodeInfo(newNodeInfos[i]);
    }
    
    // 如果节点增加，创建新卡片
    if (newCount > oldCount) {
        for (int i = oldCount; i < newCount; ++i) {
            QtETNodeInfo *nodeWidget = new QtETNodeInfo(newNodeInfos[i], m_nodeInfoContainer);
            m_nodeInfoWidgets.append(nodeWidget);
            // 插入到 stretch 之前（考虑空状态标签）
            int insertIndex = m_nodeInfoLayout->count() - 1;
            m_nodeInfoLayout->insertWidget(insertIndex, nodeWidget);
        }
    }
    // 如果节点减少，删除多余的卡片
    else if (newCount < oldCount) {
        for (int i = oldCount - 1; i >= newCount; --i) {
            QtETNodeInfo *widget = m_nodeInfoWidgets.takeAt(i);
            m_nodeInfoLayout->removeWidget(widget);
            widget->deleteLater();
        }
    }
    
    // 更新空状态标签显示
    m_emptyLabel->setVisible(m_nodeInfoWidgets.isEmpty());
    
    // 更新顶部标签
    m_statusLabel->setText(m_nodeInfoWidgets.isEmpty() ? tr("节点列表: 暂无节点") : tr("节点列表:"));
}

void QtETNetwork::startNodeMonitor() const
{
    if (m_monitorTimer && !m_monitorTimer->isActive()) {
        m_monitorTimer->start();
    }
    
    // 更新顶部标签
    m_statusLabel->setText(tr("节点列表: 加载中..."));
}

void QtETNetwork::stopNodeMonitor()
{
    if (m_monitorTimer && m_monitorTimer->isActive()) {
        m_monitorTimer->stop();
    }
    
    // 清空节点信息控件
    for (QtETNodeInfo *widget : m_nodeInfoWidgets) {
        m_nodeInfoLayout->removeWidget(widget);
        widget->deleteLater();
    }
    m_nodeInfoWidgets.clear();
    
    // 隐藏空状态标签，恢复默认提示
    m_emptyLabel->hide();
    m_statusLabel->setText(tr("请先点击运行网络"));
}

void QtETNetwork::onInfosCollected(const std::vector<EasyTierFFI::KVPair> &infos)
{
    // 如果没有运行中的网络，直接返回
    if (infos.empty()) {
        return;
    }
    
    // 获取当前选中网络的实例名称
    int currentIndex = m_networksList->currentRow();
    if (currentIndex < 0 || currentIndex >= static_cast<int>(m_networkConfs.size())) {
        return;
    }
    
    const NetworkConf &currentConf = m_networkConfs[currentIndex];
    const QString currentInstName = QString::fromStdString(currentConf.getInstanceName());
    
    // 查找当前网络的 JSON 信息
    for (const auto &info : infos) {
        if (QString::fromStdString(info.key) == currentInstName) {
            parseAndUpdateNodeInfos(QString::fromStdString(info.value));
            break;
        }
    }
}

void QtETNetwork::onMonitorTimerTimeout() const
{
    // 在工作线程中收集网络信息
    if (m_runWorker) {
        QMetaObject::invokeMethod(m_runWorker, "collectInfos", Qt::QueuedConnection);
    }
}
