#include "netpage.h"
#include "ui_netpage.h"
#include "generateconf.h"
#include "publicserver.h"

#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QRegularExpressionValidator>
#include <QMessageBox>
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QProcess>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHeaderView>
#include <QDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDateTime>
#include <iostream>

NetPage::NetPage(QWidget *parent)
    : QGroupBox(parent)
    , ui(new Ui::NetPage)
{
    ui->setupUi(this);
    createScrollArea();          // 初始化简单设置页面
    createAdvancedSetPage();     // 初始化高级设置页面
    initRunningLogWindow();      // 初始化运行日志窗口
    initRunningStatePage();      // 初始化运行状态页面

    ui->useWebBtn->setToolTip(tr("使用Web控制台管理该进程，请先前往首页打开Web控制台\n"
                    "警告：只应有一个进程被 Web 控制台管理，否则可能会导致奇怪的问题。"));
    connect(ui->useWebBtn, &QPushButton::clicked, this, [this]() {
        if (ui->useWebBtn->isChecked()) {
            // 不允许使用设置界面和运行状态页面
            ui->primerSet->setEnabled(false);
            ui->advancedSet->setEnabled(false);
            ui->runningState->setEnabled(false);
        } else {
            ui->primerSet->setEnabled(true);
            ui->advancedSet->setEnabled(true);
            ui->runningState->setEnabled(true);
        }
    });

    // 连接运行网络按钮的点击事件
    connect(ui->startPushButton, &QPushButton::clicked, this, &NetPage::onRunNetwork);
    // 连接导入和导出配置按钮的点击事件
    connect(ui->importPushButton, &QPushButton::clicked, this, &NetPage::onImportConfigClicked);
    connect(ui->exportPushButton, &QPushButton::clicked, this, &NetPage::onExportConfigClicked);
}

NetPage::~NetPage()
{
    // 清理所有进程
    stopCurrentNetwork();

    // 关闭日志文件
    closeLogFile();

    // 清理定时器
    if (m_peerUpdateTimer) {
        m_peerUpdateTimer->stop();
        m_peerUpdateTimer->deleteLater();
        m_peerUpdateTimer = nullptr;
    }

    // 清理启动过程对话框
    if (m_processDialog) {
        m_processDialog->deleteLater();
        m_processDialog = nullptr;
        m_processLogTextEdit = nullptr;
    }

    delete ui;
}

void NetPage::createScrollArea()
{
    // 创建滚动区
    QScrollArea *scrollArea = new QScrollArea(ui->primerSet);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 创建滚动区内容部件
    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 创建垂直布局
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(20, 20, 20, 20);
    scrollLayout->setSpacing(8);

    // 初始化网络设置界面
    initNetworkSettings();

    // 初始化服务器管理界面
    initServerManagement();

    // 创建一个容器部件来放置网络设置表单
    QWidget *networkFormWidget = new QWidget(scrollContent);
    QFormLayout *networkFormLayout = new QFormLayout(networkFormWidget);
    networkFormLayout->setHorizontalSpacing(20); // 设置水平间距为20
    networkFormLayout->setVerticalSpacing(15);   // 设置垂直间距为15

    // 将网络设置控件添加到表单布局
    networkFormLayout->addRow(tr("用户名:"), m_usernameEdit);
    networkFormLayout->addRow(tr("网络号:"), m_networkNameEdit);

    QHBoxLayout *passwordLayout = new QHBoxLayout(scrollContent);
    passwordLayout->addWidget(m_passwordEdit);
    passwordLayout->addWidget(m_togglePasswordBtn);
    networkFormLayout->addRow(tr("密码:"), passwordLayout);

    networkFormLayout->addRow(tr("IP设置:"), m_dhcpCheckBox);
    networkFormLayout->addRow(tr("IPv4地址:"), m_ipEdit);

    // 低延迟优先和私有模式
    networkFormLayout->addRow(tr(""), m_lowLatencyCheckBox);
    networkFormLayout->addRow(tr(""), m_privateModeCheckBox);

    // 将网络设置表单添加到滚动布局
    scrollLayout->addWidget(networkFormWidget);

    // 创建服务器管理分组
    QWidget *serverWidget = new QWidget(scrollContent);
    QVBoxLayout *serverLayout = new QVBoxLayout(serverWidget);
    serverLayout->setContentsMargins(15, 6, 15, 15);

    // 添加服务器管理标题
    QLabel *serverTitle = new QLabel(tr("服务器:"), serverWidget);
    serverTitle->setStyleSheet("font-size: 12px;");
    serverLayout->addWidget(serverTitle);
    // 添加服务器输入和按钮
    QHBoxLayout *addServerLayout = new QHBoxLayout(serverWidget);
    addServerLayout->addWidget(m_serverEdit, 1);
    addServerLayout->addWidget(m_addServerBtn);
    serverLayout->addLayout(addServerLayout);

    // 添加已添加服务器列表和删除按钮
    QHBoxLayout *serverListLayout = new QHBoxLayout(serverWidget);
    serverListLayout->addWidget(m_serverListWidget, 1);
    serverListLayout->addWidget(m_removeServerBtn);
    serverLayout->addLayout(serverListLayout);

    QHBoxLayout *serverHelpLayout = new QHBoxLayout(serverWidget);
    serverHelpLayout->addWidget(m_publicServerBtn);
    serverHelpLayout->addWidget(m_serverHelpBtn);
    serverLayout->addLayout(serverHelpLayout);

    scrollLayout->addWidget(serverWidget);

    // 添加垂直伸展空间
    scrollLayout->addStretch();

    // 设置滚动区内容
    scrollArea->setWidget(scrollContent);

    // 将滚动区添加到primerSet页面
    QVBoxLayout *primerLayout = new QVBoxLayout(ui->primerSet);
    primerLayout->setContentsMargins(0, 0, 0, 0);
    primerLayout->addWidget(scrollArea);
}

void NetPage::initNetworkSettings()
{
    // 用户名设置
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(tr("请输入用户名（默认为本机名称）"));

    // 网络名称设置
    m_networkNameEdit = new QLineEdit(this);
    m_networkNameEdit->setPlaceholderText(tr("请输入网络名称"));

    // 密码设置
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(tr("请输入密码"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    // 密码小眼睛按钮
    m_togglePasswordBtn = new QPushButton(this);
    m_togglePasswordBtn->setCheckable(true);
    m_togglePasswordBtn->setIcon(QIcon(":/icons/eye-slash.svg"));
    m_togglePasswordBtn->setIconSize(QSize(20, 20));
    m_togglePasswordBtn->setFixedSize(32, 26);
    m_togglePasswordBtn->setStyleSheet("background-color: #66ccff; border: none; border-radius: 5px; margin-left: 5px;");

    // DHCP设置，默认勾选
    m_dhcpCheckBox = new QCheckBox(tr("启用DHCP"), this);
    m_dhcpCheckBox->setChecked(true);
    m_dhcpCheckBox->setToolTip(tr("自动分配虚拟IP地址"));

    // IP地址设置
    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setPlaceholderText(tr("请输入IPv4地址"));

    // 设置IP地址验证器
    QRegularExpression ipRegex("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
    m_ipEdit->setValidator(ipValidator);

    // 低延迟优先选项
    m_lowLatencyCheckBox = new QCheckBox(tr("低延迟优先"), this);
    m_lowLatencyCheckBox->setChecked(false);
    m_lowLatencyCheckBox->setToolTip(tr("开启后,会根据算法自动选择低延时的线路进行连接"));

    // 私有模式选项
    m_privateModeCheckBox = new QCheckBox(tr("私有模式"), this);
    m_privateModeCheckBox->setChecked(true);
    m_privateModeCheckBox->setToolTip(tr("开启后,不允许网络号和密码不相同的节点使用本节点进行数据中转"));

    // 连接信号槽 - 使用新的checkStateChanged信号
    connect(m_dhcpCheckBox, &QCheckBox::checkStateChanged, this, &NetPage::onDhcpStateChanged);
    connect(m_togglePasswordBtn, &QPushButton::toggled, this, &NetPage::onTogglePasswordVisibility);

    // 默认禁用IP输入框
    onDhcpStateChanged(Qt::Checked);
}

void NetPage::initServerManagement()
{
    // 服务器地址输入框
    m_serverEdit = new QLineEdit(this);
    m_serverEdit->setPlaceholderText(tr("请输入服务器地址"));

    // 服务器地址补全
    m_serverEditCompleter = new QCompleter(this);
    m_serverListEditModel = new QStringListModel(this);
    m_serverEditCompleter->setModel(m_serverListEditModel);
    m_serverEditCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    m_serverEdit->setCompleter(m_serverEditCompleter);
    //当输入内容发生变化时，更新补全列表
    connect(m_serverEdit, &QLineEdit::textChanged, this, &NetPage::onServerEditCompleterChanged);

    // 添加服务器按钮
    m_addServerBtn = new QPushButton(tr("添加"), this);
    m_addServerBtn->setMinimumWidth(80);

    // 已添加服务器列表
    m_serverListWidget = new QListWidget(this);
    m_serverListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // 删除服务器按钮
    m_removeServerBtn = new QPushButton(tr("删除"), this);
    m_removeServerBtn->setMinimumWidth(80);
    m_removeServerBtn->setEnabled(false);

    // 默认添加EasyTier公共服务器地址
    m_serverListWidget->addItem("tcp://public.easytier.top:11010");

    // 公共服务器和服务器帮助按钮
    m_publicServerBtn = new QPushButton(tr("公共服务器列表"), this);
    m_serverHelpBtn = new QPushButton(tr("服务器使用帮助"), this);
    // 连接信号槽
    connect(m_addServerBtn, &QPushButton::clicked, this, &NetPage::onAddServer);
    connect(m_removeServerBtn, &QPushButton::clicked, this, &NetPage::onRemoveServer);
    connect(m_publicServerBtn, &QPushButton::clicked, this, &NetPage::onOpenPublicServerList);
    connect(m_serverListWidget, &QListWidget::itemSelectionChanged, [this]() {
        m_removeServerBtn->setEnabled(m_serverListWidget->selectedItems().count() > 0);
    });
    // 连接信号槽: 点击服务器帮助按钮
    connect(m_serverHelpBtn, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier/blob/master/docs/server.md"));
    });
}

void NetPage::onDhcpStateChanged(Qt::CheckState state) const {
    m_ipEdit->setEnabled(state != Qt::Checked);
}

void NetPage::onTogglePasswordVisibility() const {
    if (m_togglePasswordBtn->isChecked()) {
        m_passwordEdit->setEchoMode(QLineEdit::Normal);
        m_togglePasswordBtn->setIcon(QIcon(":/icons/eye.svg"));
    } else {
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_togglePasswordBtn->setIcon(QIcon(":/icons/eye-slash.svg"));
    }
}

void NetPage::onOpenPublicServerList() {
    auto publicServer = new PublicServer(this);
    publicServer->exec();
    const QStringList serverList = publicServer->getSelectedServers();
    // 添加到服务器列表
    for (auto &addr : serverList) {
        //检查是否重复
        bool isDuplicate = false;
        for (auto &item : getServerList()) {
            if (item == addr) {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate) m_serverListWidget->addItem(addr);
    }
    publicServer->deleteLater();
    publicServer = nullptr;
}
void NetPage::onAddServer()
{
    QString serverAddress = m_serverEdit->text().trimmed();
    // 删除所有空格
    serverAddress.remove(' ');
    if (serverAddress.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("服务器地址不能为空！"));
        return;
    }

    // 检查是否已存在相同的服务器
    for (int i = 0; i < m_serverListWidget->count(); ++i) {
        if (m_serverListWidget->item(i)->text() == serverAddress) {
            QMessageBox::warning(this, tr("警告"), tr("该服务器已存在！"));
            return;
        }
    }

    // 把中文冒号替换为英文冒号
    serverAddress.replace("：", ":");

    // 添加服务器到列表
    m_serverListWidget->addItem(serverAddress);
    m_serverEdit->clear();
}

void NetPage::onRemoveServer() const {
    if (QListWidgetItem *selectedItem = m_serverListWidget->currentItem()) {
        delete selectedItem;
        //m_removeServerBtn->setEnabled(false);
    }
}

// 服务器地址补全数据更新
void NetPage::onServerEditCompleterChanged() const {
    QString input = m_serverEdit->text();
    // 如果输入为空或者满足完整的地址格式(xx://xxx:xxx),用正则表达式匹配
    static const QRegularExpression serverRegex("^[a-zA-Z0-9+.-]+://.+:\\d+$");
    if (input.isEmpty() || serverRegex.match(input).hasMatch()) {
        return;
    }
    // 如果输入缺少协议和端口（不含冒号）
    if (!input.contains(":")) {
        QStringList temp;
        temp << "tcp://" + input + ":11010"
             << "udp://" + input + ":11010"
             << "ws://"  + input + ":11011"
             << "wg://"  + input + ":11011";
        m_serverListEditModel->setStringList(temp);
        return;
    }
    //如果缺少协议（不含://）
    if (!input.contains("://")) {
        QStringList temp;
        temp << "tcp://" + input << "udp://" + input
             << "ws://"  + input << "wg://"  + input;
        m_serverListEditModel->setStringList(temp);
        return;
    }
    // 剩下的是缺少端口号
    QStringList temp;
    temp << input+":11010" << input+":11011";
    m_serverListEditModel->setStringList(temp);
}

QString NetPage::getUsername() const
{
    return m_usernameEdit->text();
}

QString NetPage::getNetworkName() const
{
    return m_networkNameEdit->text();
}

QString NetPage::getPassword() const
{
    return m_passwordEdit->text();
}

QString NetPage::getIpAddress() const
{
    return m_ipEdit->text();
}

bool NetPage::isDhcpEnabled() const
{
    return m_dhcpCheckBox->isChecked();
}

// 获取低延迟优先和私有模式设置
bool NetPage::isLowLatencyPriority() const
{
    return m_lowLatencyCheckBox->isChecked();
}

bool NetPage::isPrivateMode() const
{
    return m_privateModeCheckBox->isChecked();
}

QStringList NetPage::getServerList() const
{
    QStringList serverList;
    for (int i = 0; i < m_serverListWidget->count(); ++i) {
        serverList << m_serverListWidget->item(i)->text();
    }
    return serverList;
}

// ===============高级设置实现===============

void NetPage::initAdvancedSettings()
{
    // 初始化高级设置组件
    m_kcpProxyCheckBox = new QCheckBox(tr("启用 KCP 代理"), this);
    m_kcpProxyCheckBox->setChecked(true);
    m_kcpProxyCheckBox->setToolTip(tr("使用UDP协议代理TCP流量,启用以获得更好的UDP P2P效果"));

    m_quicInputDisableCheckBox = new QCheckBox(tr("禁用 QUIC 输入"), this);
    m_quicInputDisableCheckBox->setChecked(false);
    m_quicInputDisableCheckBox->setToolTip(tr("不接受通过QUIC协议传输的流量"));

    m_noTunModeCheckBox = new QCheckBox(tr("无 TUN 模式"), this);
    m_noTunModeCheckBox->setChecked(false);
    m_noTunModeCheckBox->setToolTip(tr("不创建TUN网卡,开启时本节点无法主动访问其他节点,只能被动访问"));

    m_multithreadCheckBox = new QCheckBox(tr("启用多线程"), this);
    m_multithreadCheckBox->setChecked(true);
    m_multithreadCheckBox->setToolTip(tr("后端启用多线程,可提升组网性能,但可能会增加占用"));

    m_udpHolePunchingDisableCheckBox = new QCheckBox(tr("禁用 UDP 打洞"), this);
    m_udpHolePunchingDisableCheckBox->setChecked(false);
    m_udpHolePunchingDisableCheckBox->setToolTip(tr("禁用UDP打洞,仅允许通过TCP进行P2P访问"));

    m_userModeStackCheckBox = new QCheckBox(tr("使用用户态协议栈"), this);
    m_userModeStackCheckBox->setChecked(false);
    m_userModeStackCheckBox->setToolTip(tr("使用用户态协议栈,默认使用系统协议栈"));

    m_kcpInputDisableCheckBox = new QCheckBox(tr("禁用 KCP 输入"), this);
    m_kcpInputDisableCheckBox->setChecked(false);
    m_kcpInputDisableCheckBox->setToolTip(tr("不接受通过KCP协议传输的流量"));

    m_p2pDisableCheckBox = new QCheckBox(tr("禁用 P2P"), this);
    m_p2pDisableCheckBox->setChecked(false);
    m_p2pDisableCheckBox->setToolTip(tr("流量需要经过你添加的中转服务器,不直接与其他节点建立P2P连接\n注意: 如果添加的服务器都是仅建立P2P连接,会导致无法访问其他节点"));

    m_exitNodeCheckBox = new QCheckBox(tr("启用出口节点"), this);
    m_exitNodeCheckBox->setChecked(false);
    m_exitNodeCheckBox->setToolTip(tr("本节点可作为VPN的出口节点, 可被用于端口转发"));

    m_systemForwardingCheckBox = new QCheckBox(tr("系统转发"), this);
    m_systemForwardingCheckBox->setChecked(false);
    m_systemForwardingCheckBox->setToolTip(tr("通过系统内核转发子网代理数据包，禁用内置NAT"));

    m_symmetricNatHolePunchingDisableCheckBox = new QCheckBox(tr("禁用对称 NAT 打洞"), this);
    m_symmetricNatHolePunchingDisableCheckBox->setChecked(false);

    m_ipv6DisableCheckBox = new QCheckBox(tr("禁用 IPv6"), this);
    m_ipv6DisableCheckBox->setChecked(false);
    m_ipv6DisableCheckBox->setToolTip(tr("禁用IPv6连接, 仅使用IPv4"));

    m_quicProxyCheckBox = new QCheckBox(tr("启用 QUIC 代理"), this);
    m_quicProxyCheckBox->setChecked(false);
    m_quicProxyCheckBox->setToolTip(tr("QUIC是一个由Google设计,基于UDP的新一代可靠传输层协议,启用以获得更好的UDP P2P效果"));

    m_onlyPhysicalNicCheckBox = new QCheckBox(tr("仅使用物理网卡"), this);
    m_onlyPhysicalNicCheckBox->setChecked(true);
    m_onlyPhysicalNicCheckBox->setToolTip(tr("仅使用物理网卡与其他节点建立P2P连接"));

    m_rpcPacketForwardingCheckBox = new QCheckBox(tr("转发 RPC 包"), this);
    m_rpcPacketForwardingCheckBox->setChecked(false);
    m_rpcPacketForwardingCheckBox->setToolTip(tr("转发其他节点的网络配置,不论该节点是否在网络白名单中, 帮助其他节点建立P2P连接"));

    m_encryptionDisableCheckBox = new QCheckBox(tr("禁用加密"), this);
    m_encryptionDisableCheckBox->setChecked(false);
    m_encryptionDisableCheckBox->setToolTip(tr("禁用本节点通信的加密，必须与对等节点相同,正常情况下请保持加密"));

    m_magicDnsCheckBox = new QCheckBox(tr("启用魔法 DNS"), this);
    m_magicDnsCheckBox->setChecked(false);
    m_magicDnsCheckBox->setToolTip(tr("启用后可通过\"用户名.et.net\"访问本节点\n注意: Linux下要手动配置DNS服务器为 100.100.100.101 才可正常使用"));

    // 初始化RPC端口输入框
    m_rpcPortEdit = new QLineEdit(this);
    m_rpcPortEdit->setPlaceholderText(tr("请输入RPC端口号"));
    m_rpcPortEdit->setText("0"); // 默认值设为0
    m_rpcPortEdit->setValidator(new QIntValidator(0, 65535, m_rpcPortEdit)); // 限制输入0-65535的端口号
    connect(m_rpcPortEdit, &QLineEdit::textChanged, this, &NetPage::onRpcPortTextChanged);

    // 初始化监听地址管理组件
    initListenAddrManagement();
    // 初始化子网代理CIDR管理组件
    initCidrManagement();
    // 初始化网络白名单管理组件
    initRelayNetworkWhitelistManagement();
}

void NetPage::initRelayNetworkWhitelistManagement()
{
    // 网络白名单启用复选框
    m_relayNetworkWhitelistCheckBox = new QCheckBox(tr("启用网络白名单"), this);
    m_relayNetworkWhitelistCheckBox->setToolTip(tr("仅转发网络白名单中VPN的流量, 留空则为不转发任何网络的流量"));
    m_relayNetworkWhitelistCheckBox->setChecked(false);
    connect(m_relayNetworkWhitelistCheckBox, &QCheckBox::checkStateChanged,
        this, &NetPage::onRelayNetworkWhitelistStateChanged);

    // 网络白名单输入框
    m_relayNetworkWhitelistEdit = new QLineEdit(this);
    m_relayNetworkWhitelistEdit->setPlaceholderText(tr("请输入网络名称"));
    m_relayNetworkWhitelistEdit->setEnabled(false); // 初始状态为禁用

    // 添加网络白名单按钮
    m_addRelayNetworkWhitelistBtn = new QPushButton(tr("添加"), this);
    m_addRelayNetworkWhitelistBtn->setMinimumWidth(80);
    m_addRelayNetworkWhitelistBtn->setEnabled(false);

    // 已添加网络白名单列表
    m_relayNetworkWhitelistListWidget = new QListWidget(this);
    m_relayNetworkWhitelistListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_relayNetworkWhitelistListWidget->setEnabled(false);

    // 删除网络白名单按钮
    m_removeRelayNetworkWhitelistBtn = new QPushButton(tr("删除"), this);
    m_removeRelayNetworkWhitelistBtn->setMinimumWidth(80);
    m_removeRelayNetworkWhitelistBtn->setEnabled(false);

    connect(m_addRelayNetworkWhitelistBtn, &QPushButton::clicked,
            this, &NetPage::onAddRelayNetworkWhitelist);
    connect(m_removeRelayNetworkWhitelistBtn, &QPushButton::clicked,
            this, &NetPage::onRemoveRelayNetworkWhitelist);
    connect(m_relayNetworkWhitelistListWidget, &QListWidget::itemSelectionChanged, [this]() {
        if (m_relayNetworkWhitelistCheckBox->isChecked()) {
            m_removeRelayNetworkWhitelistBtn->setEnabled(
                m_relayNetworkWhitelistListWidget->selectedItems().count() > 0);
        }
    });
}

void NetPage::initListenAddrManagement()
{
    // 监听地址输入框
    m_listenAddrEdit = new QLineEdit(this);
    m_listenAddrEdit->setPlaceholderText(tr("请输入监听地址与端口"));
    // 监听地址补全组件
    m_listenAddrEditCompleter = new QCompleter(this);
    m_listenAddrListEditModel = new QStringListModel(this);
    m_listenAddrEditCompleter->setModel(m_listenAddrListEditModel);
    m_listenAddrEditCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    m_listenAddrEdit->setCompleter(m_listenAddrEditCompleter);
    //当输入内容发生变化时，更新补全列表
    connect(m_listenAddrEdit, &QLineEdit::textChanged, this, &NetPage::onListenAddrEditCompleterChanged);

    // 添加监听地址按钮
    m_addListenAddrBtn = new QPushButton(tr("添加"), this);
    m_addListenAddrBtn->setMinimumWidth(80);

    // 已添加监听地址列表
    m_listenAddrListWidget = new QListWidget(this);
    m_listenAddrListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // 删除监听地址按钮
    m_removeListenAddrBtn = new QPushButton(tr("删除"), this);
    m_removeListenAddrBtn->setMinimumWidth(80);
    m_removeListenAddrBtn->setEnabled(false);

    // 添加EasyTier默认监听地址
    m_listenAddrListWidget->addItem("tcp://0.0.0.0:11010");
    m_listenAddrListWidget->addItem("udp://0.0.0.0:11010");

    // 连接信号槽
    connect(m_addListenAddrBtn, &QPushButton::clicked, this, &NetPage::onAddListenAddr);
    connect(m_removeListenAddrBtn, &QPushButton::clicked, this, &NetPage::onRemoveListenAddr);
    connect(m_listenAddrListWidget, &QListWidget::itemSelectionChanged, [this]() {
        m_removeListenAddrBtn->setEnabled(m_listenAddrListWidget->selectedItems().count() > 0);
    });
}

void NetPage::initCidrManagement()
{
    // 子网代理CIDR输入框
    m_cidrEdit = new QLineEdit(this);
    m_cidrEdit->setPlaceholderText(tr("请输入子网代理CIDR"));

    // 添加子网代理CIDR按钮
    m_addCidrBtn = new QPushButton(tr("添加"), this);
    m_addCidrBtn->setMinimumWidth(80);

    // 已添加子网代理CIDR列表
    m_cidrListWidget = new QListWidget(this);
    m_cidrListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // 删除子网代理CIDR按钮
    m_removeCidrBtn = new QPushButton(tr("删除"), this);
    m_removeCidrBtn->setMinimumWidth(80);
    m_removeCidrBtn->setEnabled(false);

    // 打开CIDR计算器按钮
    m_calculateCidrBtn = new QPushButton(tr("打开CIDR计算器"), this);
    m_calculateCidrBtn->setMinimumWidth(80);

    // 连接信号槽
    connect(m_addCidrBtn, &QPushButton::clicked, this, &NetPage::onAddCidr);
    connect(m_removeCidrBtn, &QPushButton::clicked, this, &NetPage::onRemoveCidr);
    connect(m_cidrListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        m_removeCidrBtn->setEnabled(m_cidrListWidget->selectedItems().count() > 0);
    });
    connect(m_calculateCidrBtn, &QPushButton::clicked, this, &NetPage::onClickCidrCalculator);
}

void NetPage::createAdvancedSetPage()
{
    // 初始化高级设置组件
    initAdvancedSettings();

    // 创建滚动区
    QScrollArea *scrollArea = new QScrollArea(ui->advancedSet);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 创建滚动区内容部件
    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 创建垂直布局
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(20, 20, 20, 20);
    scrollLayout->setSpacing(8);

    //

    // 创建网格布局来放置功能开关，3列布局
    QWidget *functionWidget = new QWidget(scrollContent);
    QGridLayout *functionGridLayout = new QGridLayout(functionWidget);
    functionGridLayout->setHorizontalSpacing(30);
    functionGridLayout->setVerticalSpacing(15);

    // 添加功能开关到网格布局
    functionGridLayout->addWidget(m_kcpProxyCheckBox, 0, 0);
    functionGridLayout->addWidget(m_kcpInputDisableCheckBox, 0, 1);
    functionGridLayout->addWidget(m_noTunModeCheckBox, 0, 2);

    functionGridLayout->addWidget(m_quicProxyCheckBox, 1, 0);
    functionGridLayout->addWidget(m_quicInputDisableCheckBox, 1, 1);
    functionGridLayout->addWidget(m_udpHolePunchingDisableCheckBox, 1, 2);

    functionGridLayout->addWidget(m_multithreadCheckBox, 2, 0);
    functionGridLayout->addWidget(m_userModeStackCheckBox, 2, 1);
    functionGridLayout->addWidget(m_onlyPhysicalNicCheckBox, 2, 2);


    functionGridLayout->addWidget(m_p2pDisableCheckBox, 3, 0);
    functionGridLayout->addWidget(m_exitNodeCheckBox, 3, 1);
    functionGridLayout->addWidget(m_systemForwardingCheckBox, 3, 2);

    functionGridLayout->addWidget(m_symmetricNatHolePunchingDisableCheckBox, 4, 0);
    functionGridLayout->addWidget(m_ipv6DisableCheckBox, 4, 1);
    functionGridLayout->addWidget(m_rpcPacketForwardingCheckBox, 4, 2);

    functionGridLayout->addWidget(m_encryptionDisableCheckBox, 5, 0);
    functionGridLayout->addWidget(m_magicDnsCheckBox, 5, 1);

    // 将功能开关部件添加到滚动布局
    scrollLayout->addWidget(functionWidget);

    // 创建RPC端口设置分组（放在监听地址之前）
    QWidget *rpcPortWidget = new QWidget(scrollContent);
    QVBoxLayout *rpcPortLayout = new QVBoxLayout(rpcPortWidget);
    rpcPortLayout->setContentsMargins(15, 5, 15, 0);
    // 添加RPC端口输入框
    QHBoxLayout *rpcPortInputLayout = new QHBoxLayout(rpcPortWidget);
    QLabel *rpcPortLabel = new QLabel(tr("RPC端口号:"), rpcPortWidget);
    rpcPortLabel->setToolTip(tr("RPC是EasyTier去中心化组网中用于下发组网配置的通道"));
    rpcPortInputLayout->addWidget(rpcPortLabel);
    rpcPortInputLayout->addWidget(m_rpcPortEdit);
    rpcPortInputLayout->addStretch();
    rpcPortLayout->addLayout(rpcPortInputLayout);

    // 添加端口号说明
    QLabel *rpcPortHint = new QLabel(tr("端口范围: 0-65535 (0表示使用随机端口)"), rpcPortWidget);
    rpcPortHint->setStyleSheet("color: gray; font-size: 11px;");
    rpcPortLayout->addWidget(rpcPortHint);

    // 将RPC端口设置分组添加到滚动布局
    scrollLayout->addWidget(rpcPortWidget);

    // 创建网络白名单管理分组（放在监听地址之前）
    QWidget *whitelistWidget = new QWidget(scrollContent);
    QVBoxLayout *whitelistLayout = new QVBoxLayout(whitelistWidget);
    whitelistLayout->setContentsMargins(15, 5, 15, 10);

    // 添加网络白名单启用复选框
    whitelistLayout->addWidget(m_relayNetworkWhitelistCheckBox);

    // 创建容器用于放置添加白名单的控件，初始设置为不可见
    QWidget *whitelistControlsWidget = new QWidget(whitelistWidget);
    QVBoxLayout *whitelistControlsLayout = new QVBoxLayout(whitelistControlsWidget);
    whitelistControlsLayout->setContentsMargins(0, 0, 0, 0);

    // 添加网络白名单输入和按钮
    QHBoxLayout *addWhitelistLayout = new QHBoxLayout();
    addWhitelistLayout->addWidget(m_relayNetworkWhitelistEdit, 1);
    addWhitelistLayout->addWidget(m_addRelayNetworkWhitelistBtn);
    whitelistControlsLayout->addLayout(addWhitelistLayout);

    // 添加已添加网络白名单列表和删除按钮
    QHBoxLayout *whitelistListLayout = new QHBoxLayout();
    whitelistListLayout->addWidget(m_relayNetworkWhitelistListWidget, 1);
    whitelistListLayout->addWidget(m_removeRelayNetworkWhitelistBtn);
    whitelistControlsLayout->addLayout(whitelistListLayout);

    // 将白名单控件容器添加到主布局
    whitelistLayout->addWidget(whitelistControlsWidget);

    // 初始隐藏白名单控件（当复选框未选中时）
    whitelistControlsWidget->setVisible(false);

    // 连接复选框状态变化信号，显示/隐藏白名单控件
    // 连接复选框状态变化信号，显示/隐藏白名单控件
    connect(m_relayNetworkWhitelistCheckBox, &QCheckBox::checkStateChanged, [whitelistControlsWidget](Qt::CheckState state) {
        whitelistControlsWidget->setVisible(state == Qt::Checked);
    });

    // 将网络白名单管理分组添加到滚动布局
    scrollLayout->addWidget(whitelistWidget);

    // 创建监听地址管理分组
    QWidget *listenAddrWidget = new QWidget(scrollContent);
    QVBoxLayout *listenAddrLayout = new QVBoxLayout(listenAddrWidget);
    listenAddrLayout->setContentsMargins(15, 5, 15, 0);

    // 添加监听地址管理标题
    QLabel *listenAddrTitle = new QLabel(tr("监听地址:"), listenAddrWidget);
    listenAddrTitle->setToolTip(tr("监听节点连接,他人若想通过本节点加入组网需要连接监听地址"));
    listenAddrTitle->setStyleSheet("font-size: 12px;");
    listenAddrLayout->addWidget(listenAddrTitle);
    // 添加监听地址输入和按钮
    QHBoxLayout *addListenAddrLayout = new QHBoxLayout(listenAddrWidget);
    addListenAddrLayout->addWidget(m_listenAddrEdit, 1);
    addListenAddrLayout->addWidget(m_addListenAddrBtn);
    listenAddrLayout->addLayout(addListenAddrLayout);

    // 添加已添加监听地址列表和删除按钮
    QHBoxLayout *listenAddrListLayout = new QHBoxLayout(listenAddrWidget);
    listenAddrListLayout->addWidget(m_listenAddrListWidget, 1);
    listenAddrListLayout->addWidget(m_removeListenAddrBtn);
    listenAddrLayout->addLayout(listenAddrListLayout);

    // 将监听地址管理分组添加到滚动布局
    scrollLayout->addWidget(listenAddrWidget);

    // 创建子网代理CIDR管理分组
    QWidget *cidrWidget = new QWidget(scrollContent);
    QVBoxLayout *cidrLayout = new QVBoxLayout(cidrWidget);
    cidrLayout->setContentsMargins(15, 15, 15, 15);

    // 添加子网代理CIDR管理标题
    QLabel *cidrTitle = new QLabel(tr("子网代理CIDR:"), cidrWidget);
    cidrTitle->setToolTip(tr("输入想要进行子网代理的IP范围, CIDR格式"));
    cidrTitle->setStyleSheet("font-size: 12px;");
    cidrLayout->addWidget(cidrTitle);
    // 添加子网代理CIDR输入和按钮
    QHBoxLayout *addCidrLayout = new QHBoxLayout(cidrWidget);
    addCidrLayout->addWidget(m_cidrEdit, 1);
    addCidrLayout->addWidget(m_addCidrBtn);
    cidrLayout->addLayout(addCidrLayout);

    // 添加已添加子网代理CIDR列表和删除按钮
    QHBoxLayout *cidrListLayout = new QHBoxLayout(cidrWidget);
    cidrListLayout->addWidget(m_cidrListWidget, 1);
    cidrListLayout->addWidget(m_removeCidrBtn);
    cidrLayout->addLayout(cidrListLayout);

    cidrLayout->addWidget(m_calculateCidrBtn);

    // 将子网代理CIDR管理分组添加到滚动布局
    scrollLayout->addWidget(cidrWidget);

    // 添加垂直伸展空间
    scrollLayout->addStretch();

    // 设置滚动区内容
    scrollArea->setWidget(scrollContent);

    // 将滚动区添加到advancedSet页面
    QVBoxLayout *advancedLayout = new QVBoxLayout(ui->advancedSet);
    advancedLayout->setContentsMargins(0, 0, 0, 0);
    advancedLayout->addWidget(scrollArea);
}

// 高级设置getter方法实现
bool NetPage::isKcpProxyEnabled() const
{
    return m_kcpProxyCheckBox->isChecked();
}

bool NetPage::isQuicInputDisabled() const
{
    return m_quicInputDisableCheckBox->isChecked();
}

bool NetPage::isTunModeDisabled() const
{
    return m_noTunModeCheckBox->isChecked();
}

bool NetPage::isMultithreadEnabled() const
{
    return m_multithreadCheckBox->isChecked();
}

bool NetPage::isUdpHolePunchingDisabled() const
{
    return m_udpHolePunchingDisableCheckBox->isChecked();
}

bool NetPage::isUserModeStackEnabled() const
{
    return m_userModeStackCheckBox->isChecked();
}

bool NetPage::isKcpInputDisabled() const
{
    return m_kcpInputDisableCheckBox->isChecked();
}

bool NetPage::isP2pDisabled() const
{
    return m_p2pDisableCheckBox->isChecked();
}

bool NetPage::isExitNodeEnabled() const
{
    return m_exitNodeCheckBox->isChecked();
}

bool NetPage::isSystemForwardingEnabled() const
{
    return m_systemForwardingCheckBox->isChecked();
}

bool NetPage::isSymmetricNatHolePunchingDisabled() const
{
    return m_symmetricNatHolePunchingDisableCheckBox->isChecked();
}

bool NetPage::isIpv6Disabled() const
{
    return m_ipv6DisableCheckBox->isChecked();
}

bool NetPage::isQuicProxyEnabled() const
{
    return m_quicProxyCheckBox->isChecked();
}

bool NetPage::isBindDevice() const
{
    return m_onlyPhysicalNicCheckBox->isChecked();
}

bool NetPage::isRpcPacketForwardingEnabled() const
{
    return m_rpcPacketForwardingCheckBox->isChecked();
}

bool NetPage::isEncryptionDisabled() const
{
    return m_encryptionDisableCheckBox->isChecked();
}

bool NetPage::isMagicDnsEnabled() const
{
    return m_magicDnsCheckBox->isChecked();
}

// 网络白名单启用状态变化处理
void NetPage::onRelayNetworkWhitelistStateChanged(Qt::CheckState state) const {
    bool enabled = (state == Qt::Checked);
    m_relayNetworkWhitelistEdit->setEnabled(enabled);
    m_addRelayNetworkWhitelistBtn->setEnabled(enabled);
    m_relayNetworkWhitelistListWidget->setEnabled(enabled);

    // 更新删除按钮状态
    if (enabled) {
        m_removeRelayNetworkWhitelistBtn->setEnabled(
            m_relayNetworkWhitelistListWidget->selectedItems().count() > 0);
    } else {
        m_removeRelayNetworkWhitelistBtn->setEnabled(false);
    }
}

// 添加网络白名单
void NetPage::onAddRelayNetworkWhitelist()
{
    QString networkName = m_relayNetworkWhitelistEdit->text().trimmed();
    networkName.remove(' ');  // 删除所有空格
    if (networkName.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("网络名称不能为空！"));
        return;
    }

    // 检查是否已存在相同的网络名称
    for (int i = 0; i < m_relayNetworkWhitelistListWidget->count(); ++i) {
        if (m_relayNetworkWhitelistListWidget->item(i)->text() == networkName) {
            QMessageBox::warning(this, tr("警告"), tr("该网络名称已存在！"));
            return;
        }
    }

    // 添加网络名称到列表
    m_relayNetworkWhitelistListWidget->addItem(networkName);
    m_relayNetworkWhitelistEdit->clear();
}

// 删除网络白名单
void NetPage::onRemoveRelayNetworkWhitelist()
{
    if (QListWidgetItem *selectedItem = m_relayNetworkWhitelistListWidget->currentItem()) {
        delete selectedItem;
    }
}

// 获取网络白名单启用状态
bool NetPage::isRelayNetworkWhitelistEnabled() const
{
    return m_relayNetworkWhitelistCheckBox->isChecked();
}

// 获取网络白名单列表
QStringList NetPage::getRelayNetworkWhitelist() const
{
    QStringList whitelist;
    for (int i = 0; i < m_relayNetworkWhitelistListWidget->count(); ++i) {
        whitelist << m_relayNetworkWhitelistListWidget->item(i)->text();
    }
    return whitelist;
}

// 监听地址列表相关方法
void NetPage::onAddListenAddr()
{
    QString listenAddr = m_listenAddrEdit->text().trimmed();
    // 删除所有空格
    listenAddr.remove(' ');
    if (listenAddr.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("监听地址不能为空！"));
        return;
    }

    // 检查是否已存在相同的监听地址
    for (int i = 0; i < m_listenAddrListWidget->count(); ++i) {
        if (m_listenAddrListWidget->item(i)->text() == listenAddr) {
            QMessageBox::warning(this, tr("警告"), tr("该监听地址已存在！"));
            return;
        }
    }

    // 将中文冒号替换为英文冒号
    listenAddr.replace("：", ":");

    // 添加监听地址到列表
    m_listenAddrListWidget->addItem(listenAddr);
    m_listenAddrEdit->clear();
}

void NetPage::onRemoveListenAddr()
{
    // 1. 获取当前选中项，先判空避免无效操作
    QListWidgetItem *selectedItem = m_listenAddrListWidget->currentItem();
    if (selectedItem == nullptr) {
        return;
    }
    delete selectedItem;
}

QStringList NetPage::getListenAddrList() const
{
    QStringList listenAddrList;
    for (int i = 0; i < m_listenAddrListWidget->count(); ++i) {
        listenAddrList << m_listenAddrListWidget->item(i)->text();
    }
    return listenAddrList;
}

// 监听地址补全数据更新
void NetPage::onListenAddrEditCompleterChanged()
{
    QString input = m_listenAddrEdit->text();
    // 如果输入为空或者满足完整的地址格式(xx://xxx:xxx),用正则表达式匹配
    static const QRegularExpression listenerRegex("^[a-zA-Z0-9+.-]+://.+:\\d+$");
    if (input.isEmpty() || listenerRegex.match(input).hasMatch()) {
        return;
    }
    // 如果输入缺少协议和端口（不含冒号）
    if (!input.contains(":")) {
        QStringList temp;
        temp << "tcp://" + input + ":11010"
             << "udp://" + input + ":11010"
             << "ws://"  + input + ":11011"
             << "wg://"  + input + ":11011";
        m_listenAddrListEditModel->setStringList(temp);
        return;
    }
    //如果缺少协议（不含://）
    if (!input.contains("://")) {
        QStringList temp;
        temp << "tcp://" + input << "udp://" + input
             << "ws://"  + input << "wg://"  + input;
        m_listenAddrListEditModel->setStringList(temp);
        return;
    }
    // 剩下的是缺少端口号
    QStringList temp;
    temp << input+":11010" << input+":11011";
    m_listenAddrListEditModel->setStringList(temp);
}

//

// 子网代理CIDR列表相关方法
void NetPage::onAddCidr()
{
    QString cidr = m_cidrEdit->text().trimmed();
    cidr.remove(' ');  // 删除所有空格
    if (cidr.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("子网代理CIDR不能为空！"));
        return;
    }

    // 检查是否已存在相同的子网代理CIDR
    for (int i = 0; i < m_cidrListWidget->count(); ++i) {
        if (m_cidrListWidget->item(i)->text() == cidr) {
            QMessageBox::warning(this, tr("警告"), tr("该子网代理CIDR已存在！"));
            return;
        }
    }

    // 将中文冒号替换为英文冒号
    cidr.replace("：", ":");

    // 添加子网代理CIDR到列表
    m_cidrListWidget->addItem(cidr);
    m_cidrEdit->clear();
}

void NetPage::onRemoveCidr()
{
    if (QListWidgetItem *selectedItem = m_cidrListWidget->currentItem()) {
        delete selectedItem;
    }
}

QStringList NetPage::getCidrList() const
{
    QStringList cidrList;
    for (int i = 0; i < m_cidrListWidget->count(); ++i) {
        cidrList << m_cidrListWidget->item(i)->text();
    }
    return cidrList;
}

int NetPage::getRpcPort() const
{
    QString portText = m_rpcPortEdit->text().trimmed();
    if (portText.isEmpty()) {
        return 0; // 默认值
    }
    bool ok;
    int port = portText.toInt(&ok);
    return ok ? port : 0; // 如果转换失败，返回默认值0
}

void NetPage::onRpcPortTextChanged(const QString &text)
{
    // 实时验证端口号输入
    QString trimmedText = text.trimmed();
    if (trimmedText.isEmpty()) {
        m_rpcPortEdit->setStyleSheet(""); // 空文本是允许的
        return;
    }

    bool ok;
    int port = trimmedText.toInt(&ok);
    if (!ok || port < 0 || port > 65535) {
        // 输入无效，设置红色边框提示
        m_rpcPortEdit->setStyleSheet("border: 1px solid red;");
    } else {
        // 输入有效，恢复正常样式
        m_rpcPortEdit->setStyleSheet("");
    }
}

void NetPage::onClickCidrCalculator() {
    // 检测CidrCalculator.exe是否存在
    // 获取当前程序目录
    const QString calculatorDir = QCoreApplication::applicationDirPath() + "/CIDRCalculator.exe";

    // 检查程序是否存在
    const QFileInfo fileInfo(calculatorDir);
    if (!fileInfo.exists()) {
        m_logTextEdit->appendPlainText(QString("错误: 找不到 %1").arg(calculatorDir));
        QMessageBox::critical(this, tr("错误"), tr("找不到 CIDRCalculator.exe"));
        return;
    }
    // 打开目录下的CidrCalculator.exe
    QProcess::startDetached(calculatorDir);
}


// =====================运行EasyTier相关=======================

// 添加初始化运行日志窗口的方法
void NetPage::initRunningLogWindow()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(ui->runningLog);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(5);

    // 创建日志文本框
    m_logTextEdit = new QPlainTextEdit(ui->runningLog);
    m_logTextEdit->setReadOnly(true);
    //m_logTextEdit->setFont(QFont("Consolas", 9));
    //m_logTextEdit->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");

    // 创建打开日志文件按钮
    m_openLogFileBtn = new QPushButton(tr("打开日志文件查看完整日志"), ui->runningLog);

    // 添加控件到主布局
    mainLayout->addWidget(m_logTextEdit, 1);  // 日志文本框占主要空间
    mainLayout->addWidget(m_openLogFileBtn);

    // 设置runningLog页面的布局
    ui->runningLog->setLayout(mainLayout);

    // 连接打开日志文件按钮的点击事件
    connect(m_openLogFileBtn, &QPushButton::clicked, this, &NetPage::onOpenLogFileClicked);
}


// EasyTier按键运行方法 - 重构版本
void NetPage::onRunNetwork()
{
    // 如果网络正在运行，则停止它
    if (m_easytierProcess && m_easytierProcess->state() == QProcess::Running) {
        stopCurrentNetwork();
        return;
    }

    // 跳转到运行状态窗口
    ui->netTabWidget->setCurrentWidget(ui->runningState);

    // 清理之前的日志并初始化新日志文件
    m_logTextEdit->clear();
    m_logTextEdit->appendPlainText("正在启动EasyTier网络...");

    // 初始化日志文件
    if (!initLogFile()) {
        m_logTextEdit->appendPlainText("警告: 日志文件初始化失败，将继续运行但不会保存日志");
    }

    // 准备EasyTier程序
    QString appDir, easytierPath;
    if (!prepareEasyTierProgram(appDir, easytierPath)) {
        closeLogFile();
        updateUIState(false);;
        return;
    }

    // 创建启动过程对话框
    closeProcessDialog();  // 如果已有对话框，先删除
    // 创建新的对话框
    m_processDialog = new QDialog(this);
    auto *dialogLayout = new QVBoxLayout(m_processDialog);
    auto *dialogTitleLabel = new QLabel("启动日志", m_processDialog);
    m_processLogTextEdit = new QPlainTextEdit(m_processDialog);

    m_processDialog->setWindowTitle(tr("启动EasyTier中。。。"));
    m_processDialog->setModal(true);
    dialogLayout->setContentsMargins(20, 20, 20, 20);
    m_processDialog->setMinimumSize(400, 300);
    dialogTitleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    dialogLayout->addWidget(dialogTitleLabel);
    m_processLogTextEdit->setReadOnly(true);
    m_processLogTextEdit->setFont(QFont("Consolas", 12));
    dialogLayout->addWidget(m_processLogTextEdit);
    m_processDialog->setWindowFlag(Qt::WindowCloseButtonHint, false);
    m_processDialog->show();

    m_processLogTextEdit->appendPlainText(tr("启动EasyTier进程..."));
    QApplication::processEvents();

    try {
        if (m_processLogTextEdit) {
            m_processLogTextEdit->appendPlainText(tr("生成启动配置..."));
            QApplication::processEvents();
        }

        QStringList arguments;
        if (ui->useWebBtn->isChecked()) {
            arguments << "-w" << "udp://127.0.0.1:55668/admin";
        } else {
            arguments = generateConfCommand(this);
        }

        if (m_processLogTextEdit) {
            m_processLogTextEdit->appendPlainText(tr("检测到可用RPC端口: ") + QString::number(realRpcPort));
            m_processLogTextEdit->appendPlainText(tr("启动配置已生成: ") + arguments.join(" "));
            m_processLogTextEdit->appendPlainText(tr("正在传入参数并启动EasyTier进程..."));
            QApplication::processEvents();
        }

        // 启动进程

        if (bool success = startEasyTierProcess(arguments, appDir, easytierPath)) {
            m_logTextEdit->appendPlainText(QString("正在启动 %1").arg(easytierPath));
            m_logTextEdit->appendPlainText(QString("启动参数: %1").arg(arguments.join(" ")));

            // 更新状态
            updateUIState(true);
            handleProcessStartResult(true);

            QTimer::singleShot(800, this, &NetPage::closeProcessDialog);
        } else {
            QTimer::singleShot(1600, this, &NetPage::closeProcessDialog);
            handleProcessStartResult(false);
        }
    } catch (const std::exception& e) {
        if (m_processLogTextEdit) {
            m_processLogTextEdit->appendPlainText(QString("启动异常: %1").arg(e.what()));
        }
        QMessageBox::warning(this, "警告", QString("启动异常: %1").arg(e.what()));
        QTimer::singleShot(1600, this, &NetPage::closeProcessDialog);
        handleProcessStartResult(false);
    }
    //closeLogFile();
}

// 停止当前网络的统一方法
void NetPage::stopCurrentNetwork()
{
    if (m_easytierProcess) {
        m_easytierProcess->disconnect();
        m_logTextEdit->appendPlainText("正在终止EasyTier进程...");
        m_easytierProcess->kill();

        // 等待强制终止完成
        if (m_easytierProcess->waitForFinished(1000)) {
            m_logTextEdit->appendPlainText("EasyTier进程终止成功");
        } else {
            m_logTextEdit->appendPlainText("警告：EasyTier进程可能未完全终止");
            QMessageBox::warning(this, "警告", "强制终止EasyTier进程失败");
        }
    }

    emit networkFinished(); // 发送网络停止信号
    updateUIState(false);;    // 更新UI状态

    // 清理进程对象
    if (m_easytierProcess) {
        m_easytierProcess->deleteLater();
        m_easytierProcess = nullptr;
    }

    // 停止cli进程
    if (m_cliProcess) {
        m_cliProcess->disconnect();
        m_cliProcess->kill();
        m_cliProcess->deleteLater();
        m_cliProcess = nullptr;
    }
    closeLogFile();
}

// 检查并准备EasyTier程序
bool NetPage::prepareEasyTierProgram(QString& appDir, QString& easytierPath)
{
    appDir = QCoreApplication::applicationDirPath() + "/etcore";
    easytierPath = appDir + "/easytier-core.exe";

    QFileInfo fileInfo(easytierPath);
    if (!fileInfo.exists()) {
        m_logTextEdit->appendPlainText(QString("错误: 找不到 %1").arg(easytierPath));
        QMessageBox::critical(this, "错误", "找不到EasyTier程序");
        return false;
    }
    return true;
}

// 启动EasyTier进程 - 重构版本
bool NetPage::startEasyTierProcess(const QStringList& arguments, const QString& appDir, const QString& easytierPath)
{
    try {
        // 创建并配置进程
        m_easytierProcess = new QProcess(this);
        m_easytierProcess->setWorkingDirectory(appDir);

        setupProcessConnections();

        // 启动进程
        m_easytierProcess->start(easytierPath, arguments);

        // 等待进程启动（最多3秒）
        if (!m_easytierProcess->waitForStarted(3000)) {
            if (m_processLogTextEdit) {
                m_processLogTextEdit->appendPlainText("进程启动超时");
            }
            m_easytierProcess->kill();
            throw std::runtime_error("进程启动超时");
        }

        if (m_processLogTextEdit) {
            m_processLogTextEdit->appendPlainText("EasyTier进程启动成功");
        }

        return true;
    } catch (const std::exception& e) {
        if (m_processLogTextEdit) {
            m_processLogTextEdit->appendPlainText(QString("启动异常: %1").arg(e.what()));
        }
        QMessageBox::warning(this, "警告", QString("启动异常: %1").arg(e.what()));
        return false;
    }
}

// 创建并配置进程连接
void NetPage::setupProcessConnections()
{
    if (!m_easytierProcess) return;

    // 连接信号槽，实时获取进程输出
    connect(m_easytierProcess, &QProcess::readyReadStandardOutput,
            this, &NetPage::onProcessOutputReady);
    connect(m_easytierProcess, &QProcess::readyReadStandardError,
            this, &NetPage::onProcessErrorReady);
    // 连接进程完成信号（发生错误）
    connect(m_easytierProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NetPage::onProcessFinished);
}

// 处理进程启动结果
void NetPage::handleProcessStartResult(bool success, const QString& errorMessage)
{
    if (success) {
        updateUIState(true);
        emit networkStarted();  // 发送网络启动信号
    } else {
        updateUIState(false);
        if (m_easytierProcess) {
            emit networkFinished(); // 发送网络停止信号
            m_easytierProcess->deleteLater();
            m_easytierProcess = nullptr;
        }
    }
}

// 更新UI状态
void NetPage::updateUIState(bool isRunning)
{
    if (isRunning) {
        ui->startPushButton->setText("运行中");
        ui->startPushButton->setStyleSheet("color: green; font-weight: bold;");
        m_runningStatusLabel->setText(tr("正在运行 ") + getNetworkName());
    } else {
        ui->startPushButton->setStyleSheet("");
        ui->startPushButton->setText("运行网络");
        m_runningStatusLabel->setText("EasyTier实例未运行，请先点击运行网络");
    }
}

// 关闭启动过程对话框
void NetPage::closeProcessDialog()
{
    if (m_processDialog) {
        m_processDialog->disconnect();
        m_processDialog->deleteLater();
        m_processDialog = nullptr;
        m_processLogTextEdit = nullptr;
    }
}

// 限制日志行数
void NetPage::limitLogLines(int maxLines)
{
    if (!m_logTextEdit) return;

    QTextDocument *doc = m_logTextEdit->document();
    int lineCount = doc->blockCount();

    if (lineCount > maxLines) {
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lineCount - maxLines);
        cursor.removeSelectedText();
    }
}

// 初始化日志文件
bool NetPage::initLogFile()
{
    // 创建log文件夹
    QString logDirPath = QCoreApplication::applicationDirPath() + "/log";
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        if (!logDir.mkpath(".")) {
            m_logTextEdit->appendPlainText("警告: 无法创建日志目录");
            return false;
        }
    }

    // 生成日志文件名
    QString networkName = getNetworkName();
    if (networkName.isEmpty()) {
        networkName = "default";
    }
    //QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    m_currentLogFileName = QString("%1/%2.log").arg(logDirPath, networkName);

    // 打开日志文件
    m_logFile = new QFile(m_currentLogFileName);
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logTextEdit->appendPlainText(QString("警告: 无法打开日志文件 %1").arg(m_currentLogFileName));
        delete m_logFile;
        m_logFile = nullptr;
        return false;
    }

    m_logStream = new QTextStream(m_logFile);
    return true;
}

// 关闭日志文件
void NetPage::closeLogFile()
{
    if (m_logStream) {
        delete m_logStream;
        m_logStream = nullptr;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
}

// 保存日志到文件
void NetPage::saveLogToFile(const QString& text)
{
    if (m_logStream) {
        *m_logStream << text << Qt::endl;
        m_logStream->flush();
    }
}

// 打开日志文件
void NetPage::onOpenLogFileClicked()
{
    if (m_currentLogFileName.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("暂无日志文件"));
        return;
    }

    QFileInfo fileInfo(m_currentLogFileName);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, tr("警告"), tr("日志文件不存在"));
        return;
    }

    // 使用系统默认程序打开日志文件
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentLogFileName));
}


// 进程输出处理
void NetPage::onProcessOutputReady()
{
    if (m_easytierProcess) {
        QString output = m_easytierProcess->readAllStandardOutput();
        m_logTextEdit->appendPlainText(output);
        saveLogToFile(output);
        limitLogLines(1000);  // 限制显示1000行
    }
}

// 进程错误处理
void NetPage::onProcessErrorReady()
{
    if (m_easytierProcess) {
        QString error = m_easytierProcess->readAllStandardError();
        QString errorText = "错误: " + error;
        m_logTextEdit->appendPlainText(errorText);
        saveLogToFile(errorText);
        limitLogLines(1000);  // 限制显示1000行
    }
}

// 进程完成处理(Et进程自动退出代表发生错误)
void NetPage::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit) {
        m_logTextEdit->appendPlainText(QString("EasyTier已停止，退出码: %1").arg(exitCode));
    } else {
        m_logTextEdit->appendPlainText("EasyTier异常终止");
    }
    updateUIState(false);;
    if (m_easytierProcess) {
        m_easytierProcess->deleteLater();
        m_easytierProcess = nullptr;
    }
    emit networkFinished(); // 发送网络停止信号
    QMessageBox::warning(this, "警告", "EasyTier进程异常终止，请前往日志查看详细信息");
}

// ===================运行状态监测相关===================
// 初始化运行状态页面
void NetPage::initRunningStatePage()
{
    // 创建运行状态标签
    m_runningStatusLabel = new QLabel(tr("EasyTier未运行，请先点击运行网络"), ui->runningState);
    m_runningStatusLabel->setAlignment(Qt::AlignLeft);
    m_runningStatusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_runningStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold;");

    // 创建隐藏服务器信息勾选框
    m_isHideServersBox = new QCheckBox(tr("隐藏服务器信息"), ui->runningState);
    m_isHideServersBox->setChecked(false);
    m_isHideServersBox->setToolTip(tr("勾选后，将不会在列表中显示没有分配虚拟IP的节点的信息"));
    m_isHideServersBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    QWidget *infoWidget = new QWidget(ui->runningState);
    QHBoxLayout *infoLayout = new QHBoxLayout(infoWidget);
    infoLayout->addWidget(m_runningStatusLabel);
    infoLayout->addWidget(m_isHideServersBox);
    infoWidget->setLayout(infoLayout);

    // 创建节点信息表格
    m_peerTable = new QTableWidget(ui->runningState);
    m_peerTable->setColumnCount(9); // 根据JSON数据字段数量设定列数
    m_peerTable->setHorizontalHeaderLabels({
        tr("用户名"), tr("虚拟IPv4"), tr("连接方式"),
        tr("协议"), tr("延迟(ms)"), tr("丢包率"),
        tr("接收字节"), tr("发送字节"), tr("CIDR")
    });

    // 设置列宽策略，每列最小宽度为100
    QHeaderView *header = m_peerTable->horizontalHeader();
    header->setMinimumSectionSize(110); // 设置最小列宽
    header->setDefaultSectionSize(110); // 设置默认列宽
    m_peerTable->verticalHeader()->setDefaultSectionSize(40); // 设置行高为40
    m_peerTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置表格自适应大小
    m_peerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_peerTable->setSelectionBehavior(QAbstractItemView::SelectRows); // 只能选择整行

    // 布局
    QVBoxLayout *layout = new QVBoxLayout(ui->runningState);
    layout->addWidget(infoWidget);
    layout->addWidget(m_peerTable);
    layout->setAlignment(Qt::AlignTop);

    // 设置定时器，定期更新节点信息
    m_peerUpdateTimer = new QTimer(this);
    connect(m_peerUpdateTimer, &QTimer::timeout, this, &NetPage::updatePeerInfo);
    connect(this, &NetPage::networkStarted, this, [=, this] {
        if (!ui->useWebBtn->isChecked()) m_peerUpdateTimer->start(2000);
    });
    connect(this, &NetPage::networkFinished, this, [=, this] {
        m_peerUpdateTimer->stop();
    });
}

// 更新节点信息
void NetPage::updatePeerInfo() {
    // 如果网络未运行，不执行更新并删除异步进程
    if (m_easytierProcess == nullptr || m_easytierProcess->state() != QProcess::Running) {
        if (m_cliProcess) {
            m_cliProcess->deleteLater();
            m_cliProcess = nullptr;
        }
        return;
    }
    // 如果检测进程还在运行则终止并报错
    if (m_cliProcess && m_cliProcess->state() == QProcess::Running) {
        m_cliProcess->kill();
        m_logTextEdit->appendPlainText(tr("警告: 获取节点信息超时, CLI进程可能出错"));
    }

    // et-cli 命令路径信息
    QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
    QString cliPath = appDir + "/easytier-cli.exe";

    // 如果异步进程为nullptr，创建新的进程
    if (!m_cliProcess) {
        std::clog << "et已运行，创建异步cli进程"<< std::endl;
        m_cliProcess = new QProcess(this);

        // 检查CLI程序是否存在
        QFileInfo fileInfo(cliPath);
        if (!fileInfo.exists()) {
            m_logTextEdit->appendPlainText(QString("错误: 找不到 %1").arg(cliPath));
            return;
        }
        m_cliProcess->setWorkingDirectory(appDir);

        //连接进程完成信号
        connect(m_cliProcess, &QProcess::finished, m_cliProcess, [=, this] {
            QByteArray output = m_cliProcess->readAllStandardOutput();
            QString errorOutput = m_cliProcess->readAllStandardError();

            if (!errorOutput.isEmpty()) {
                m_logTextEdit->appendPlainText("错误输出: " + errorOutput);
                return;
            }

            if (!output.isEmpty()) {
                parseAndDisplayPeerInfo(output);
            }
        });
    }

    // 再次运行CLI进程
    m_cliProcess->start(cliPath, QStringList() <<"-p"<<"127.0.0.1:"+QString::number(realRpcPort)<< "-o" << "json" << "peer");
}

// 解析并显示节点信息
void NetPage::parseAndDisplayPeerInfo(const QByteArray &jsonData)
{
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        m_logTextEdit->appendPlainText("JSON解析错误: " + jsonError.errorString());
        return;
    }

    if (!doc.isArray()) {
        m_logTextEdit->appendPlainText("JSON数据格式错误: 应为数组格式");
        return;
    }

    QJsonArray peers = doc.array();

    // 清空表格
    m_peerTable->setRowCount(0);

    // 填充表格数据
    for (const auto &peerValue : peers) {
        if (!peerValue.isObject()) continue;

        QJsonObject peerObj = peerValue.toObject();
        if(m_isHideServersBox && m_isHideServersBox->isChecked() &&
            peerObj.value("ipv4").toString().isEmpty() &&
            !peerObj.value("hostname").toString().contains("Local") ) {
            continue;
        }

        int row = m_peerTable->rowCount();
        m_peerTable->insertRow(row);

        // 按顺序设置各列数据
        m_peerTable->setItem(row, 0, new QTableWidgetItem(peerObj.value("hostname").toString()));

        // 虚拟IPV4处添加服务器显示逻辑
        QString ipv4 = peerObj.value("ipv4").toString();
        if ( ipv4.isEmpty() ) {
            if (peerObj.value("hostname").toString().contains("PublicServer")) {
                ipv4 = tr("公共服务器");
            } else if (!peerObj.value("hostname").toString().contains("Local")) {
                ipv4 = tr("服务器");
            }
        }
        m_peerTable->setItem(row, 1, new QTableWidgetItem(ipv4));
        m_peerTable->setItem(row, 2, new QTableWidgetItem(peerObj.value("cost").toString()));
        m_peerTable->setItem(row, 3, new QTableWidgetItem(peerObj.value("tunnel_proto").toString()));
        m_peerTable->setItem(row, 4, new QTableWidgetItem(peerObj.value("lat_ms").toString()));
        m_peerTable->setItem(row, 5, new QTableWidgetItem(peerObj.value("loss_rate").toString()));
        m_peerTable->setItem(row, 6, new QTableWidgetItem(peerObj.value("rx_bytes").toString()));
        m_peerTable->setItem(row, 7, new QTableWidgetItem(peerObj.value("tx_bytes").toString()));
        m_peerTable->setItem(row, 8, new QTableWidgetItem(peerObj.value("cidr").toString()));
    }
}


// ==================数据持久化相关==================

// 获取当前网络配置
QJsonObject NetPage::getNetworkConfig() const
{
    QJsonObject config;

    // 基础设置
    config["username"] = m_usernameEdit->text();
    config["networkName"] = m_networkNameEdit->text();
    config["password"] = m_passwordEdit->text();
    config["useDhcp"] = m_dhcpCheckBox->isChecked();
    config["ipAddress"] = m_ipEdit->text();
    config["lowLatencyPriority"] = m_lowLatencyCheckBox->isChecked();
    config["privateMode"] = m_privateModeCheckBox->isChecked();

    // 服务器列表
    QJsonArray servers;
    for (int i = 0; i < m_serverListWidget->count(); ++i) {
        servers.append(m_serverListWidget->item(i)->text());
    }
    config["servers"] = servers;

    // 高级设置
    QJsonObject advancedSettings;
    advancedSettings["kcpProxy"] = m_kcpProxyCheckBox->isChecked();
    advancedSettings["quicInputDisable"] = m_quicInputDisableCheckBox->isChecked();
    advancedSettings["noTunMode"] = m_noTunModeCheckBox->isChecked();
    advancedSettings["multithread"] = m_multithreadCheckBox->isChecked();
    advancedSettings["udpHolePunchingDisable"] = m_udpHolePunchingDisableCheckBox->isChecked();
    advancedSettings["userModeStack"] = m_userModeStackCheckBox->isChecked();
    advancedSettings["kcpInputDisable"] = m_kcpInputDisableCheckBox->isChecked();
    advancedSettings["p2pDisable"] = m_p2pDisableCheckBox->isChecked();
    advancedSettings["exitNode"] = m_exitNodeCheckBox->isChecked();
    advancedSettings["systemForwarding"] = m_systemForwardingCheckBox->isChecked();
    advancedSettings["symmetricNatHolePunchingDisable"] = m_symmetricNatHolePunchingDisableCheckBox->isChecked();
    advancedSettings["ipv6Disable"] = m_ipv6DisableCheckBox->isChecked();
    advancedSettings["quicProxy"] = m_quicProxyCheckBox->isChecked();
    advancedSettings["bindDeviceOnly"] = m_onlyPhysicalNicCheckBox->isChecked();
    advancedSettings["rpcPacketForwarding"] = m_rpcPacketForwardingCheckBox->isChecked();
    advancedSettings["encryptionDisable"] = m_encryptionDisableCheckBox->isChecked();
    advancedSettings["magicDns"] = m_magicDnsCheckBox->isChecked();
    advancedSettings["rpcPort"] = m_rpcPortEdit->text();

    // 网络白名单设置
    advancedSettings["relayNetworkWhitelistEnabled"] = m_relayNetworkWhitelistCheckBox->isChecked();

    // 网络白名单列表
    QJsonArray whitelistArray;
    for (int i = 0; i < m_relayNetworkWhitelistListWidget->count(); ++i) {
        whitelistArray.append(m_relayNetworkWhitelistListWidget->item(i)->text());
    }
    advancedSettings["relayNetworkWhitelist"] = whitelistArray;

    // 监听地址列表
    QJsonArray listenAddresses;
    for (int i = 0; i < m_listenAddrListWidget->count(); ++i) {
        listenAddresses.append(m_listenAddrListWidget->item(i)->text());
    }
    advancedSettings["listenAddresses"] = listenAddresses;

    // CIDR列表
    QJsonArray cidrList;
    for (int i = 0; i < m_cidrListWidget->count(); ++i) {
        cidrList.append(m_cidrListWidget->item(i)->text());
    }
    advancedSettings["cidrList"] = cidrList;

    config["advancedSettings"] = advancedSettings;
    config["isActive"] = isRunning(); // 记录当前是否处于激活运行状态

    return config;
}

// 设置网络配置
void NetPage::setNetworkConfig(const QJsonObject &config)
{
    // 基础设置
    if (config.contains("username")) {
        m_usernameEdit->setText(config["username"].toString());
    }
    if (config.contains("networkName")) {
        m_networkNameEdit->setText(config["networkName"].toString());
    }
    if (config.contains("password")) {
        m_passwordEdit->setText(config["password"].toString());
    }
    if (config.contains("useDhcp")) {
        m_dhcpCheckBox->setChecked(config["useDhcp"].toBool());
        onDhcpStateChanged(m_dhcpCheckBox->isChecked() ? Qt::Checked : Qt::Unchecked);
    }
    if (config.contains("ipAddress")) {
        m_ipEdit->setText(config["ipAddress"].toString());
    }
    if (config.contains("lowLatencyPriority")) {
        m_lowLatencyCheckBox->setChecked(config["lowLatencyPriority"].toBool());
    }
    if (config.contains("privateMode")) {
        m_privateModeCheckBox->setChecked(config["privateMode"].toBool());
    }

    // 服务器列表
    if (config.contains("servers") && config["servers"].isArray()) {
        QJsonArray servers = config["servers"].toArray();
        m_serverListWidget->clear();
        for (const auto &serverValue : servers) {
            m_serverListWidget->addItem(serverValue.toString());
        }
    }

    // 高级设置
    if (config.contains("advancedSettings") && config["advancedSettings"].isObject()) {
        QJsonObject advancedSettings = config["advancedSettings"].toObject();

        if (advancedSettings.contains("kcpProxy")) {
            m_kcpProxyCheckBox->setChecked(advancedSettings["kcpProxy"].toBool());
        }
        if (advancedSettings.contains("quicInputDisable")) {
            m_quicInputDisableCheckBox->setChecked(advancedSettings["quicInputDisable"].toBool());
        }
        if (advancedSettings.contains("noTunMode")) {
            m_noTunModeCheckBox->setChecked(advancedSettings["noTunMode"].toBool());
        }
        if (advancedSettings.contains("multithread")) {
            m_multithreadCheckBox->setChecked(advancedSettings["multithread"].toBool());
        }
        if (advancedSettings.contains("udpHolePunchingDisable")) {
            m_udpHolePunchingDisableCheckBox->setChecked(advancedSettings["udpHolePunchingDisable"].toBool());
        }
        if (advancedSettings.contains("userModeStack")) {
            m_userModeStackCheckBox->setChecked(advancedSettings["userModeStack"].toBool());
        }
        if (advancedSettings.contains("kcpInputDisable")) {
            m_kcpInputDisableCheckBox->setChecked(advancedSettings["kcpInputDisable"].toBool());
        }
        if (advancedSettings.contains("p2pDisable")) {
            m_p2pDisableCheckBox->setChecked(advancedSettings["p2pDisable"].toBool());
        }
        if (advancedSettings.contains("exitNode")) {
            m_exitNodeCheckBox->setChecked(advancedSettings["exitNode"].toBool());
        }
        if (advancedSettings.contains("systemForwarding")) {
            m_systemForwardingCheckBox->setChecked(advancedSettings["systemForwarding"].toBool());
        }
        if (advancedSettings.contains("symmetricNatHolePunchingDisable")) {
            m_symmetricNatHolePunchingDisableCheckBox->setChecked(advancedSettings["symmetricNatHolePunchingDisable"].toBool());
        }
        if (advancedSettings.contains("ipv6Disable")) {
            m_ipv6DisableCheckBox->setChecked(advancedSettings["ipv6Disable"].toBool());
        }
        if (advancedSettings.contains("quicProxy")) {
            m_quicProxyCheckBox->setChecked(advancedSettings["quicProxy"].toBool());
        }
        if (advancedSettings.contains("bindDeviceOnly")) {
            m_onlyPhysicalNicCheckBox->setChecked(advancedSettings["bindDeviceOnly"].toBool());
        }
        if (advancedSettings.contains("rpcPacketForwarding")) {
            m_rpcPacketForwardingCheckBox->setChecked(advancedSettings["rpcPacketForwarding"].toBool());
        }
        if (advancedSettings.contains("encryptionDisable")) {
            m_encryptionDisableCheckBox->setChecked(advancedSettings["encryptionDisable"].toBool());
        }
        if (advancedSettings.contains("magicDns")) {
            m_magicDnsCheckBox->setChecked(advancedSettings["magicDns"].toBool());
        }
        if (advancedSettings.contains("rpcPort")) {
            m_rpcPortEdit->setText(advancedSettings["rpcPort"].toString());
        }

        // 网络白名单设置
        if (advancedSettings.contains("relayNetworkWhitelistEnabled")) {
            m_relayNetworkWhitelistCheckBox->setChecked(
                advancedSettings["relayNetworkWhitelistEnabled"].toBool());
            // 触发状态变化以更新UI
            onRelayNetworkWhitelistStateChanged(
                m_relayNetworkWhitelistCheckBox->isChecked() ? Qt::Checked : Qt::Unchecked);
        }

        // 网络白名单列表
        if (advancedSettings.contains("relayNetworkWhitelist") &&
            advancedSettings["relayNetworkWhitelist"].isArray()) {
            QJsonArray whitelistArray = advancedSettings["relayNetworkWhitelist"].toArray();
            m_relayNetworkWhitelistListWidget->clear();
            for (const auto &networkValue : whitelistArray) {
                m_relayNetworkWhitelistListWidget->addItem(networkValue.toString());
            }
            }

        // 监听地址列表
        if (advancedSettings.contains("listenAddresses") && advancedSettings["listenAddresses"].isArray()) {
            QJsonArray listenAddresses = advancedSettings["listenAddresses"].toArray();
            m_listenAddrListWidget->clear();
            for (const auto &addrValue : listenAddresses) {
                m_listenAddrListWidget->addItem(addrValue.toString());
            }
        }

        // CIDR列表
        if (advancedSettings.contains("cidrList") && advancedSettings["cidrList"].isArray()) {
            QJsonArray cidrList = advancedSettings["cidrList"].toArray();
            m_cidrListWidget->clear();
            for (const auto &cidrValue : cidrList) {
                m_cidrListWidget->addItem(cidrValue.toString());
            }
        }
    }
}

// ===================== 开机自动运行相关 =====================
// 开机自启动方法 - 重构版本
void NetPage::runNetworkOnAutoStart()
{
    m_logTextEdit->clear();
    m_logTextEdit->appendPlainText("正在启动EasyTier网络（自启动）...");

    // 初始化日志文件
    if (!initLogFile()) {
        m_logTextEdit->appendPlainText("警告: 日志文件初始化失败");
    }

    // 准备EasyTier程序
    QString appDir, easytierPath;
    if (!prepareEasyTierProgram(appDir, easytierPath)) {
        closeLogFile();
        updateUIState(false);
        return;
    }

    try {
        QApplication::processEvents();
        QStringList arguments = generateConfCommand(this);
        QApplication::processEvents();

        // 直接启动进程（无对话框）
        m_easytierProcess = new QProcess(this);
        m_easytierProcess->setWorkingDirectory(appDir);
        setupProcessConnections();
        m_easytierProcess->start(easytierPath, arguments);

        // 等待进程启动
        if (!m_easytierProcess->waitForStarted(1600)) {
            m_easytierProcess->kill();
            throw std::runtime_error("进程启动超时");
        }

        m_logTextEdit->appendPlainText(QString("正在启动 %1").arg(easytierPath));
        m_logTextEdit->appendPlainText(QString("启动参数: %1").arg(arguments.join(" ")));

        updateUIState(true);
        emit networkStarted();

    } catch (const std::exception& e) {
        m_logTextEdit->appendPlainText(QString("启动异常: %1").arg(e.what()));
        updateUIState(false);
        if (m_easytierProcess) {
            emit networkFinished();
            m_easytierProcess->deleteLater();
            m_easytierProcess = nullptr;
        }
    }

    //closeLogFile();
}


// ===============配置导入导出功能实现===============

void NetPage::onImportConfigClicked()
{
    // 弹出文件选择对话框，让用户选择要导入的配置文件
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("导入配置"),
        QDir::homePath(),
        tr("配置文件 (*.json);;所有文件 (*)")
    );

    if (fileName.isEmpty()) {
        return; // 用户取消了操作
    }

    QFile configFile(fileName);
    if (!configFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("错误"), tr("无法打开配置文件: %1").arg(configFile.errorString()));
        return;
    }

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll(), &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, tr("错误"), tr("配置文件格式错误: %1").arg(jsonError.errorString()));
        return;
    }

    if (!doc.isObject()) {
        QMessageBox::critical(this, tr("错误"), tr("配置文件格式错误: 不是有效的JSON对象"));
        return;
    }

    // 停止当前运行的网络（如果正在运行）
    if (isRunning()) {
        QMessageBox::StandardButton ret = QMessageBox::question(
            this,
            tr("确认"),
            tr("当前网络正在运行，导入配置将会停止当前网络，是否继续？"),
            QMessageBox::Yes | QMessageBox::No
        );
        if (ret == QMessageBox::No) {
            return;
        }

        // 停止正在运行的网络
        if (m_easytierProcess) {
            stopCurrentNetwork();
        }
    }

    // 使用导入的配置更新界面
    setNetworkConfig(doc.object());

    m_logTextEdit->appendPlainText(tr("配置导入成功: %1").arg(fileName));
    QMessageBox::information(this, tr("成功"), tr("配置导入成功！"));
}

// 导出配置
void NetPage::onExportConfigClicked()
{
    // 获取当前配置
    QJsonObject config = getNetworkConfig();
    // 删除用户名
    config.remove("username");

    // 弹出文件保存对话框
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("导出配置"),
        QDir::homePath() + "/QtEasyTier_config.json",
        tr("配置文件 (*.json);;所有文件 (*)")
    );

    if (fileName.isEmpty()) {
        return; // 用户取消了操作
    }

    // 如果文件名没有.json扩展名，添加它
    if (!fileName.endsWith(".json", Qt::CaseInsensitive)) {
        fileName += ".json";
    }

    QFile configFile(fileName);
    if (!configFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("错误"), tr("无法创建配置文件: %1").arg(configFile.errorString()));
        return;
    }

    QJsonDocument doc(config);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

    qint64 bytesWritten = configFile.write(jsonData);
    if (bytesWritten == -1) {
        QMessageBox::critical(this, tr("错误"), tr("无法写入配置文件: %1").arg(configFile.errorString()));
        return;
    }

    m_logTextEdit->appendPlainText(tr("配置导出成功: %1").arg(fileName));
    QMessageBox::information(this, tr("成功"), tr("配置导出成功！"));
}
