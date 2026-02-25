#include "netpage.h"
#include "ui_netpage.h"
#include "generateconf.h"
#include "easytierworker.h"
#include "publicserver.h"

#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QRegularExpressionValidator>
#include <QIntValidator>
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
#include <QProgressBar>
#include <QStandardPaths>

NetPage::NetPage(QWidget *parent)
    : QGroupBox(parent)
    , ui(new Ui::NetPage)
{
    ui->setupUi(this);
    createScrollArea();          // 初始化简单设置页面
    createAdvancedSetPage();     // 初始化高级设置页面
    initRunningLogWindow();      // 初始化运行日志窗口
    initRunningStatePage();      // 初始化运行状态页面
    initWorkerThread();          // 初始化Worker线程
    initOtherSettingsPage();     // 初始化其他设置页面

    // 连接运行网络按钮的点击事件
    connect(ui->startPushButton, &QPushButton::clicked, this, &NetPage::onRunNetwork);
    // 连接导入和导出配置按钮的点击事件
    connect(ui->importPushButton, &QPushButton::clicked, this, &NetPage::onImportConfigClicked);
    connect(ui->exportPushButton, &QPushButton::clicked, this, &NetPage::onExportConfigClicked);
}

NetPage::~NetPage()
{
    // 清理Worker线程
    cleanupWorkerThread();

    // 清理临时配置文件
    cleanupTempConfigFile();

    // 清理启动过程对话框
    if (m_processDialog) {
        m_processDialog->deleteLater();
        m_processDialog = nullptr;
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
    m_serverListWidget->addItem("txt://qtet-public.070219.xyz");
    m_serverListWidget->addItem("txt://qtet-public2.070219.xyz");
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
        QDesktopServices::openUrl(QUrl("https://qtet.070219.xyz/servers/server-instruction/"));
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
    // 同步到配置文件模式的 RPC 端口输入框
    connect(m_rpcPortEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_confRpcPortEdit) {
            m_confRpcPortEdit->blockSignals(true);
            m_confRpcPortEdit->setText(text);
            m_confRpcPortEdit->blockSignals(false);
        }
    });

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


// =====================EasyTierWorker线程管理===================

void NetPage::initWorkerThread()
{
    // 创建工作线程
    m_workerThread = new QThread(this);
    m_workerThread->setObjectName("EasyTierWorkerThread");

    // 创建工作对象（不指定父对象，因为要移动到线程）
    m_worker = new EasyTierWorker();

    // 将工作对象移动到线程
    m_worker->moveToThread(m_workerThread);

    // 连接信号槽（使用Qt::QueuedConnection确保线程安全）
    connect(m_worker, &EasyTierWorker::processStarted,
            this, &NetPage::onWorkerProcessStarted, Qt::QueuedConnection);
    connect(m_worker, &EasyTierWorker::processStopped,
            this, &NetPage::onWorkerProcessStopped, Qt::QueuedConnection);
    connect(m_worker, &EasyTierWorker::logOutput,
            this, &NetPage::onWorkerLogOutput, Qt::QueuedConnection);
    connect(m_worker, &EasyTierWorker::peerInfoUpdated,
            this, &NetPage::onWorkerPeerInfoUpdated, Qt::QueuedConnection);
    connect(m_worker, &EasyTierWorker::processCrashed,
            this, &NetPage::onWorkerProcessCrashed, Qt::QueuedConnection);

    // 线程结束时清理工作对象
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // 启动线程
    m_workerThread->start();
}

void NetPage::cleanupWorkerThread()
{
    if (m_workerThread) {
        // 先停止工作
        if (m_worker && m_worker->isRunning()) {
            // 同步调用stopEasyTier
            QMetaObject::invokeMethod(m_worker, "stopEasyTier", Qt::BlockingQueuedConnection);
        }

        // 停止线程
        m_workerThread->quit();
        m_workerThread->wait();

        m_workerThread->deleteLater();
        m_workerThread = nullptr;
        m_worker = nullptr;  // 已由finished信号deleteLater
    }
}


// =====================运行EasyTier相关===================

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

    QWidget *buttonWidget = new QWidget(ui->runningLog);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
    // 创建打开日志文件(夹)按钮
    m_openLogFileBtn = new QPushButton(tr("打开当前日志文件"), ui->runningLog);
    m_openLogDirBtn = new QPushButton(tr("打开日志保存目录"), ui->runningLog);
    buttonLayout->addWidget(m_openLogFileBtn);
    buttonLayout->addWidget(m_openLogDirBtn);

    // 添加控件到主布局
    mainLayout->addWidget(m_logTextEdit, 1);  // 日志文本框占主要空间
    mainLayout->addWidget(buttonWidget);

    // 设置runningLog页面的布局
    ui->runningLog->setLayout(mainLayout);

    // 连接打开日志文件按钮的点击事件
    connect(m_openLogFileBtn, &QPushButton::clicked, this, &NetPage::onOpenLogFileClicked);
    connect(m_openLogDirBtn, &QPushButton::clicked, this, []() {
        const QString &logDir = QCoreApplication::applicationDirPath() + "/log";
        QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
    });
}

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

    updateUIState(false);
}

// EasyTier按键运行方法
void NetPage::onRunNetwork()
{
    // 检查进程状态，防止在启动/停止过程中重复操作
    if (m_worker) {
        auto state = m_worker->getProcessState();
        if (state == EasyTierWorker::ProcessState::Starting ||
            state == EasyTierWorker::ProcessState::Stopping) {
            return;  // 直接返回，不处理
        }
    }

    // 如果网络正在运行，则停止它
    if (isRunning()) {
        onStopNetwork();
        return;
    }

    // 显示启动过程对话框（无限进度条）
    showProcessDialog();

    // 跳转到运行状态窗口
    ui->netTabWidget->setCurrentWidget(ui->runningState);

    // 清理之前的日志和临时配置文件
    m_logTextEdit->clear();
    cleanupTempConfigFile();
    m_logTextEdit->appendPlainText(tr("正在启动EasyTier网络..."));

    // 准备EasyTier程序
    QString appDir, easytierPath;
    if (!prepareEasyTierProgram(appDir, easytierPath)) {
        closeProcessDialog();
        updateUIState(false);
        return;
    }

    try {
        QStringList arguments;
        QString networkName = getNetworkName();

        switch (m_startMode) {
        case StartMode::WebConsole: {
            // Web 控制台管理模式
            m_logTextEdit->appendPlainText(tr("启动模式: Web 控制台管理"));

            // 获取连接地址
            QString webArg;
            if (ui->connectToLocalBox->isChecked()) {
                // 连接到本地控制台
                const Settings::WebConsoleConfig webConfig = Settings::getWebConsoleConfig();
                QString protocol = webConfig.getConfigProtocolString();
                webArg = QString("%1://127.0.0.1:%2/admin").arg(protocol).arg(webConfig.configPort);
            } else if (m_webConnectAddrEdit && !m_webConnectAddrEdit->text().isEmpty()) {
                // 使用自定义连接地址
                webArg = m_webConnectAddrEdit->text().trimmed();
            } else {
                closeProcessDialog();
                m_logTextEdit->appendPlainText(tr("错误: 未指定Web控制台连接地址"));
                QMessageBox::warning(this, tr("警告"), tr("请输入Web控制台连接地址或勾选连接到本地控制台"));
                updateUIState(false);
                return;
            }
            arguments << "-w" << webArg;
            // Web 控制台管理模式不需要 CLI 监测
            realRpcPort = NO_USE_CLI;
            break;
        }
        case StartMode::ConfigFile: {
            // 配置文件启动模式
            m_logTextEdit->appendPlainText(tr("启动模式: 配置文件启动"));

            // 准备配置文件路径
            QString configFilePath;
            if (m_configSource == ConfigSource::SelectFile) {
                // 选择文件模式：直接使用所选文件
                configFilePath = m_configFilePathEdit->text().trimmed();
                if (configFilePath.isEmpty()) {
                    closeProcessDialog();
                    m_logTextEdit->appendPlainText(tr("错误: 未选择配置文件"));
                    QMessageBox::warning(this, tr("警告"), tr("请选择配置文件"));
                    updateUIState(false);
                    return;
                }
                if (!QFile::exists(configFilePath)) {
                    closeProcessDialog();
                    m_logTextEdit->appendPlainText(tr("错误: 配置文件不存在"));
                    QMessageBox::warning(this, tr("警告"), tr("配置文件不存在"));
                    updateUIState(false);
                    return;
                }
            } else {
                // 下方输入模式：创建临时配置文件
                if (!createTempConfigFile(networkName.isEmpty() ? "default" : networkName)) {
                    closeProcessDialog();
                    m_logTextEdit->appendPlainText(tr("错误: 创建临时配置文件失败"));
                    QMessageBox::warning(this, tr("警告"), tr("创建临时配置文件失败"));
                    updateUIState(false);
                    return;
                }
                configFilePath = m_tempConfigFilePath;
            }

            arguments << "-c" << configFilePath;

            // 处理 RPC 端口配置（与常规启动逻辑一致）
            const int &rpcPort = getRpcPort();  // 从 m_rpcPortEdit 获取值
            if (rpcPort != 0 && !m_autoRpcCheckBox->isChecked()) {
                // 用户指定端口，检查是否被占用
                if (isPortOccupied(rpcPort)) {
                    closeProcessDialog();
                    m_logTextEdit->appendPlainText(tr("错误: RPC 端口 %1 已被占用").arg(rpcPort));
                    QMessageBox::warning(this, tr("警告"), tr("RPC 端口 %1 已被占用，请更换端口").arg(rpcPort));
                    updateUIState(false);
                    return;
                }
                realRpcPort = rpcPort;
                arguments << "--rpc-portal" << QString::number(rpcPort);
                m_logTextEdit->appendPlainText(tr("RPC 端口: %1 (手动配置)").arg(rpcPort));
            } else {
                // 自动分配端口，循环找到一个未被占用的端口
                for (int i = 10000; i < 50000; i++) {
                    int port = getRandomPort();
                    if (!isPortOccupied(port)) {
                        realRpcPort = port;
                        arguments << "--rpc-portal" << QString::number(port);
                        m_logTextEdit->appendPlainText(tr("RPC 端口: %1 (自动分配)").arg(port));
                        break;
                    }
                }
                if (realRpcPort == 0) {
                    closeProcessDialog();
                    m_logTextEdit->appendPlainText(tr("错误: 无法找到可用的 RPC 端口"));
                    QMessageBox::warning(this, tr("警告"), tr("无法找到可用的 RPC 端口"));
                    updateUIState(false);
                    return;
                }
            }
            break;
        }
        case StartMode::Normal:
        default:
            // 常规管理模式
            m_logTextEdit->appendPlainText(tr("启动模式: 常规管理"));
            arguments = generateConfCommand(this);
            break;
        }

        m_logTextEdit->appendPlainText(tr("启动参数: %1").arg(arguments.join(" ")));

        // 通过信号槽调用Worker的startEasyTier方法
        QMetaObject::invokeMethod(m_worker, "startEasyTier",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, networkName),
                                  Q_ARG(QStringList, arguments),
                                  Q_ARG(QString, appDir),
                                  Q_ARG(QString, easytierPath),
                                  Q_ARG(int, realRpcPort));

    } catch (const std::exception& e) {
        closeProcessDialog();
        m_logTextEdit->appendPlainText(tr("启动异常: %1").arg(e.what()));
        QMessageBox::warning(this, tr("警告"), tr("启动异常: %1").arg(e.what()));
        updateUIState(false);
    }
}

// 停止网络
void NetPage::onStopNetwork()
{
    if (!m_worker || !m_worker->isRunning()) {
        return;
    }

    // 设置停止中状态，禁用按钮并显示"停止中..."
    ui->startPushButton->setText(tr("停止中..."));
    ui->startPushButton->setStyleSheet("color: orange; font-weight: bold;");
    ui->startPushButton->setEnabled(false);

    m_logTextEdit->appendPlainText(tr("正在停止EasyTier进程..."));

    // 通过信号槽调用Worker的stopEasyTier方法
    QMetaObject::invokeMethod(m_worker, "stopEasyTier", Qt::QueuedConnection);
}

// 检查并准备EasyTier程序
bool NetPage::prepareEasyTierProgram(QString& appDir, QString& easytierPath)
{
    appDir = QCoreApplication::applicationDirPath() + "/etcore";
    easytierPath = appDir + "/easytier-core.exe";

    QFileInfo fileInfo(easytierPath);
    if (!fileInfo.exists()) {
        m_logTextEdit->appendPlainText(QString("错误: 找不到 %1").arg(easytierPath));
        QMessageBox::critical(this, tr("错误"), tr("找不到EasyTier程序"));
        return false;
    }
    return true;
}

// 检查是否正在运行
bool NetPage::isRunning() const
{
    return m_worker && m_worker->isRunning();
}

// 更新UI状态
void NetPage::updateUIState(bool isRunning)
{
    // 提示信息
    const QString runningTooltip = tr("网络正在运行中，设置不可更改");

    if (isRunning) {
        ui->startPushButton->setText(tr("运行中"));
        ui->startPushButton->setStyleSheet("color: green; font-weight: bold;");
        ui->startPushButton->setEnabled(true);
        m_runningStatusLabel->setText(tr("正在运行 ") + getNetworkName());
        if (m_startMode == StartMode::WebConsole){
            ui->runningState->setEnabled(false);
        } else
        {
            ui->runningState->setEnabled(true);
        }

        // 禁用三个设置页面并添加 tooltip
        ui->primerSet->setEnabled(false);
        ui->primerSet->setToolTip(runningTooltip);
        ui->advancedSet->setEnabled(false);
        ui->advancedSet->setToolTip(runningTooltip);
        ui->otherSet->setEnabled(false);
        ui->otherSet->setToolTip(runningTooltip);

    } else {
        ui->startPushButton->setStyleSheet("");
        ui->startPushButton->setText(tr("运行网络"));
        ui->startPushButton->setEnabled(true);
        m_runningStatusLabel->setText(tr("EasyTier实例未运行，请先点击运行网络"));
        // 清空节点列表
        if (m_peerTable) {
            m_peerTable->setRowCount(0);
        }
        ui->runningState->setEnabled(false);

        // 启用三个设置页面并清除 tooltip
        ui->primerSet->setEnabled(true);
        ui->primerSet->setToolTip("");
        ui->advancedSet->setEnabled(true);
        ui->advancedSet->setToolTip("");
        ui->otherSet->setEnabled(true);
        ui->otherSet->setToolTip("");

        // 恢复其他设置页面的启动方式状态
        updateStartModeState();
    }
}

// 显示启动过程对话框（无限进度条）
void NetPage::showProcessDialog()
{
    if (m_processDialog) {
        m_processDialog->deleteLater();
    }

    m_processDialog = new QDialog(this);
    m_processDialog->setWindowTitle(tr("启动EasyTier中..."));
    m_processDialog->setModal(true);
    m_processDialog->setWindowFlag(Qt::WindowCloseButtonHint, false);
    m_processDialog->setMinimumWidth(300);

    QVBoxLayout* layout = new QVBoxLayout(m_processDialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // 标签
    m_progressLabel = new QLabel(tr("正在启动EasyTier进程，请稍候..."), m_processDialog);
    m_progressLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_progressLabel);

    // 无限进度条
    m_progressBar = new QProgressBar(m_processDialog);
    m_progressBar->setRange(0, 0);  // 设置为无限滚动模式
    m_progressBar->setTextVisible(false);
    m_progressBar->setMinimumHeight(20);
    layout->addWidget(m_progressBar);

    m_processDialog->setLayout(layout);
    m_processDialog->show();

    // 启动超时定时器
    QTimer::singleShot(EASYTIER_START_TIMEOUT_MS, m_processDialog, [this]() {
        if (m_processDialog && m_processDialog->isVisible()) {
            closeProcessDialog();
            m_logTextEdit->appendPlainText(tr("启动超时"));
            QMessageBox::warning(this, tr("警告"), tr("EasyTier进程启动超时"));
            onStopNetwork();

        }
    });
}

// 关闭启动过程对话框
void NetPage::closeProcessDialog()
{
    if (m_processDialog) {
        m_processDialog->close();
        m_processDialog->deleteLater();
        m_processDialog = nullptr;
        m_progressBar = nullptr;
        m_progressLabel = nullptr;
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


// =====================Worker信号处理===================

void NetPage::onWorkerProcessStarted(bool success, const QString& errorMessage)
{
    closeProcessDialog();

    if (success) {
        updateUIState(true);
        emit networkStarted();
    } else {
        updateUIState(false);
        if (!errorMessage.isEmpty()) {
            m_logTextEdit->appendPlainText(tr("启动失败: %1").arg(errorMessage));
            QMessageBox::warning(this, tr("警告"), tr("启动失败: %1").arg(errorMessage));
        }
    }
}

void NetPage::onWorkerProcessStopped(bool success)
{
    // 清理临时配置文件
    cleanupTempConfigFile();

    updateUIState(false);
    emit networkFinished();

    if (!success) {
        m_logTextEdit->appendPlainText(tr("警告: 进程停止可能不完全"));
    }
}

void NetPage::onWorkerLogOutput(const QString& logText, bool isError)
{
    if (isError) {
        m_logTextEdit->appendPlainText(logText);
    } else {
        m_logTextEdit->appendPlainText(logText);
    }
    limitLogLines(MAX_LOG_LINES);
}

void NetPage::onWorkerPeerInfoUpdated(const QJsonArray& peers)
{
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

void NetPage::onWorkerProcessCrashed(int exitCode)
{
    m_logTextEdit->appendPlainText(tr("EasyTier进程异常终止，退出码: %1").arg(exitCode));
    QMessageBox::warning(this, tr("警告"), tr("EasyTier进程异常终止，退出码: %1").arg(exitCode));
    emit networkFinished();
    updateUIState(false);
}

// 打开日志文件
void NetPage::onOpenLogFileClicked()
{
    // 检查Worker是否存在(不存在则发生严重错误)
    if (!m_worker) {
        QMessageBox::critical(this, tr("错误"), tr("未创建EasyTier Worker实例"));
        return;
    }

    // 从Worker获取日志文件路径
    QString logFilePath = m_worker->getLogFilePath();
    if (logFilePath.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("暂无日志文件，请先启动网络"));
        return;
    }

    QFileInfo fileInfo(logFilePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, tr("警告"), tr("日志文件不存在: %1").arg(logFilePath));
        return;
    }

    // 使用系统默认程序打开日志文件
    QDesktopServices::openUrl(QUrl::fromLocalFile(logFilePath));
}


// ===================== 开机自动运行相关 =====================
// 开机自启动方法 - 重构版本
void NetPage::runNetworkOnAutoStart()
{
    m_logTextEdit->clear();
    m_logTextEdit->appendPlainText(tr("正在启动EasyTier网络（自启动）..."));

    // 准备EasyTier程序
    QString appDir, easytierPath;
    if (!prepareEasyTierProgram(appDir, easytierPath)) {
        updateUIState(false);
        return;
    }

    try {
        QStringList arguments = generateConfCommand(this);

        m_logTextEdit->appendPlainText(tr("启动参数: %1").arg(arguments.join(" ")));

        // 通过信号槽调用Worker的startEasyTier方法
        QMetaObject::invokeMethod(m_worker, "startEasyTier",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, getNetworkName()),
                                  Q_ARG(QStringList, arguments),
                                  Q_ARG(QString, appDir),
                                  Q_ARG(QString, easytierPath),
                                  Q_ARG(int, realRpcPort));

    } catch (const std::exception& e) {
        m_logTextEdit->appendPlainText(tr("启动异常: %1").arg(e.what()));
        updateUIState(false);
    }
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
        onStopNetwork();
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

    // 其他设置页面配置
    QJsonObject otherSettings;
    otherSettings["startMode"] = static_cast<int>(m_startMode);
    otherSettings["configSource"] = static_cast<int>(m_configSource);

    // Web 控制台连接地址
    if (m_webConnectAddrEdit) {
        otherSettings["webConnectAddress"] = m_webConnectAddrEdit->text();
    }
    otherSettings["connectToLocal"] = ui->connectToLocalBox->isChecked();

    // 配置文件内容
    if (m_configTextEdit) {
        otherSettings["configContent"] = m_configTextEdit->toPlainText();
    }

    // 配置文件路径
    if (m_configFilePathEdit) {
        otherSettings["configFilePath"] = m_configFilePathEdit->text();
    }

    config["otherSettings"] = otherSettings;
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

    if (config.contains("otherSettings") && config["otherSettings"].isObject()) {
        QJsonObject otherSettings = config["otherSettings"].toObject();

        // 启动方式
        if (otherSettings.contains("startMode")) {
            int mode = otherSettings["startMode"].toInt(0);
            m_startMode = static_cast<StartMode>(mode);
            updateStartModeState();
        }

        // Web 控制台连接地址
        if (otherSettings.contains("webConnectAddress")) {
            if (m_webConnectAddrEdit) {
                m_webConnectAddrEdit->setText(otherSettings["webConnectAddress"].toString());
            }
        }

        // 连接到本地控制台
        if (otherSettings.contains("connectToLocal")) {
            ui->connectToLocalBox->setChecked(otherSettings["connectToLocal"].toBool());
        }

        // 配置文件来源
        if (otherSettings.contains("configSource")) {
            int source = otherSettings["configSource"].toInt(0);
            m_configSource = static_cast<ConfigSource>(source);
            ui->confSourceBox->setCurrentIndex(source);
            updateConfigSourceUI();
        }

        // 配置文件内容
        if (otherSettings.contains("configContent")) {
            if (m_configTextEdit) {
                m_configTextEdit->setPlainText(otherSettings["configContent"].toString());
            }
        }

        // 配置文件路径
        if (otherSettings.contains("configFilePath")) {
            if (m_configFilePathEdit) {
                m_configFilePathEdit->setText(otherSettings["configFilePath"].toString());
            }
        }
    }
}

// =====================其他设置页面相关===================

void NetPage::initOtherSettingsPage()
{
    // 设置工具提示
    ui->useWebBox->setToolTip(tr("使用 Web 控制台管理该进程，请先前往首页打开 Web 控制台\n"
                    "警告：只应有一个进程被本地 Web 控制台管理，否则可能会导致奇怪的问题。"));
    ui->connectToLocalBox->setToolTip(tr("勾选后连接到本地 Web 控制台的 admin 用户\n"
                    "如需连接其他用户，请手动输入完整“地址/用户名”格式信息"));
    ui->useConfFileBox->setToolTip(tr("通过 TOML 配置文件启动 EasyTier 核心"));

    // 创建 Web 控制台连接地址输入框（放在 formWidget 中）
    m_webConnectAddrEdit = ui->webDashboardAddrEdit;
    m_webConnectAddrEdit->setPlaceholderText(tr("例：udp://qtet.example.cn/firefly"));
    m_webConnectAddrEdit->setObjectName("webConnectAddrEdit");

    // 初始化 RPC 端口相关控件（使用 UI 文件中的控件）
    // 配置文件模式的 RPC 端口输入框与高级设置中的 m_rpcPortEdit 共用同一个值
    // 这里 m_confRpcPortEdit 只是显示和编辑，实际值存储在 m_rpcPortEdit 中
    m_autoRpcCheckBox = ui->autoRpcBox;
    m_confRpcPortEdit = ui->confRpcPortEdit;
    // 设置 RPC 端口输入框的验证器（0-65535，0表示自动分配）
    m_confRpcPortEdit->setValidator(new QIntValidator(0, 65535, this));
    m_confRpcPortEdit->setPlaceholderText(tr("0 表示自动分配"));
    // 默认勾选自动配置
    m_autoRpcCheckBox->setChecked(true);
    m_confRpcPortEdit->setEnabled(false);  // 自动模式下禁用输入

    // 创建配置文件文本编辑框
    m_configTextEdit = new QPlainTextEdit(this);
    m_configTextEdit->setPlaceholderText(tr("在此输入TOML配置内容..."));
    m_configTextEdit->setPlainText(getDefaultConfigContent());
    m_configTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_configTextEdit->setStyleSheet("font-family: Consolas, 'Courier New', monospace; font-size: 12px;");
    // 禁用滚动条，让外层 QScrollArea 处理滚动
    m_configTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 设置高度可以无限延展
    m_configTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_configTextEdit->setMinimumHeight(150);

    // 创建配置文件路径输入框
    m_configFilePathEdit = new QLineEdit(this);
    m_configFilePathEdit->setPlaceholderText(tr("请选择配置文件..."));
    m_configFilePathEdit->setReadOnly(true);

    // 创建选择配置文件按钮
    m_selectConfigFileBtn = new QPushButton(tr("选择文件"), this);
    m_selectConfigFileBtn->setMinimumWidth(80);

    // 将控件添加到 confGroupBox 的布局中
    QVBoxLayout *confLayout = qobject_cast<QVBoxLayout*>(ui->confGroupBox->layout());
    if (confLayout) {
        // 添加配置文件文本编辑框（用于下方输入）
        confLayout->addWidget(m_configTextEdit, 1, Qt::AlignTop);

        // 创建文件选择行的容器
        QWidget *fileSelectWidget = new QWidget(this);
        fileSelectWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QHBoxLayout *fileSelectLayout = new QHBoxLayout(fileSelectWidget);
        fileSelectLayout->setContentsMargins(0, 0, 0, 0);
        fileSelectLayout->addWidget(m_configFilePathEdit, 1);
        fileSelectLayout->addWidget(m_selectConfigFileBtn);
        confLayout->addWidget(fileSelectWidget, 1, Qt::AlignTop);
    }

    // 连接信号槽
    connect(ui->useWebBox, &QCheckBox::checkStateChanged, this, &NetPage::onUseWebBoxChanged);
    connect(ui->useConfFileBox, &QCheckBox::checkStateChanged, this, &NetPage::onUseConfFileBoxChanged);
    connect(ui->connectToLocalBox, &QCheckBox::checkStateChanged, this, &NetPage::onConnectToLocalBoxChanged);
    connect(ui->confSourceBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NetPage::onConfSourceBoxChanged);
    connect(m_selectConfigFileBtn, &QPushButton::clicked, this, &NetPage::onSelectConfigFileClicked);
    // 文本内容变化时动态调整高度
    connect(m_configTextEdit, &QPlainTextEdit::textChanged, this, &NetPage::onConfigTextChanged);
    // 自动配置 RPC 端口复选框状态变化
    connect(m_autoRpcCheckBox, &QCheckBox::checkStateChanged, this, &NetPage::onAutoRpcBoxChanged);
    // 配置文件模式的 RPC 端口输入框文本变化时，同步到 m_rpcPortEdit
    connect(m_confRpcPortEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_rpcPortEdit) {
            m_rpcPortEdit->blockSignals(true);
            m_rpcPortEdit->setText(text);
            m_rpcPortEdit->blockSignals(false);
        }
    });

    // 初始化UI状态
    updateStartModeState();
    updateConfigSourceUI();
    // 初始化文本编辑框高度
    onConfigTextChanged();
}

void NetPage::onUseWebBoxChanged(int state)
{
    if (state == Qt::Checked) {
        m_startMode = StartMode::WebConsole;
    } else if (!ui->useConfFileBox->isChecked()) {
        m_startMode = StartMode::Normal;
    }
    updateStartModeState();
}

void NetPage::onUseConfFileBoxChanged(int state)
{
    if (state == Qt::Checked) {
        m_startMode = StartMode::ConfigFile;
    } else if (!ui->useWebBox->isChecked()) {
        m_startMode = StartMode::Normal;
    }
    updateStartModeState();
}

void NetPage::onConnectToLocalBoxChanged(int state)
{
    // 连接到本地控制台时，禁用连接地址输入框
    if (m_webConnectAddrEdit) {
        m_webConnectAddrEdit->setEnabled(state != Qt::Checked);
    }
}

void NetPage::onAutoRpcBoxChanged(int state)
{
    if (state == Qt::Checked) {
        // 自动配置：禁用输入框
        //m_confRpcPortEdit->setText("0");
        m_confRpcPortEdit->setEnabled(false);
    } else {
        // 手动配置：启用输入框，允许用户输入端口号
        m_confRpcPortEdit->setEnabled(true);
    }
}

void NetPage::onConfSourceBoxChanged(int index)
{
    m_configSource = static_cast<ConfigSource>(index);
    updateConfigSourceUI();
}

void NetPage::onSelectConfigFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("选择配置文件"),
        QDir::homePath(),
        tr("TOML 配置文件 (*.toml);;所有文件 (*)")
    );

    if (!fileName.isEmpty()) {
        m_configFilePathEdit->setText(fileName);
        
        // 读取文件内容并显示在文本编辑框中
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_configTextEdit->setPlainText(QString::fromUtf8(file.readAll()));
            file.close();
        } else {
            QMessageBox::warning(this, tr("警告"), tr("无法读取配置文件: %1").arg(file.errorString()));
        }
    }
}

void NetPage::onConfigTextChanged()
{
    // 动态调整文本编辑框高度，让外层 QScrollArea 处理滚动
    if (!m_configTextEdit) return;
    
    // 获取文档高度
    QFontMetrics fm(m_configTextEdit->font());
    int lineSpacing = fm.lineSpacing();
    int lineCount = m_configTextEdit->document()->lineCount();
    // 文档内容高度 + 边距
    int docHeight = lineCount * lineSpacing + 50;
    // 设置最小高度，确保足够显示内容
    m_configTextEdit->setMinimumHeight(qMax(150, docHeight));
}

void NetPage::updateStartModeState()
{
    // 根据当前启动方式更新 UI 状态
    const bool useWeb = (m_startMode == StartMode::WebConsole);
    const bool useConfig = (m_startMode == StartMode::ConfigFile);

    // 提示信息
    const QString &webTooltip = tr("正在使用Web控制台管理");
    const QString &configTooltip = tr("正在使用配置文件管理");

    // 更新复选框状态（避免信号循环）
    ui->useWebBox->blockSignals(true);
    ui->useConfFileBox->blockSignals(true);
    ui->useWebBox->setChecked(useWeb);
    ui->useConfFileBox->setChecked(useConfig);
    ui->useWebBox->blockSignals(false);
    ui->useConfFileBox->blockSignals(false);

    // 禁用互斥的选项并添加 tooltip
    ui->webGroupBox->setEnabled(!useConfig);
    ui->webGroupBox->setToolTip(useConfig ? configTooltip : "");
    ui->confGroupBox->setEnabled(!useWeb);
    ui->confGroupBox->setToolTip(useWeb ? webTooltip : "");

    // 基础设置和高级设置的启用状态及 tooltip
    ui->primerSet->setEnabled(!useWeb && !useConfig);
    ui->primerSet->setToolTip(useWeb ? webTooltip : (useConfig ? configTooltip : ""));
    ui->advancedSet->setEnabled(!useWeb && !useConfig);
    ui->advancedSet->setToolTip(useWeb ? webTooltip : (useConfig ? configTooltip : ""));

    if (m_confRpcPortEdit) {
        // 同步 m_rpcPortEdit 的值到 m_confRpcPortEdit
        if (m_rpcPortEdit) {
            m_confRpcPortEdit->blockSignals(true);
            m_confRpcPortEdit->setText(m_rpcPortEdit->text());
            m_confRpcPortEdit->blockSignals(false);
        }
        // 根据自动配置复选框状态决定是否启用输入框
        m_confRpcPortEdit->setEnabled(useConfig && m_autoRpcCheckBox && !m_autoRpcCheckBox->isChecked());
    }
}

void NetPage::updateConfigSourceUI()
{
    const bool isInput = (m_configSource == ConfigSource::Input);

    // 显示/隐藏相应的控件
    if (m_configTextEdit) {
        m_configTextEdit->setVisible(isInput);
    }
    if (m_configFilePathEdit) {
        m_configFilePathEdit->setVisible(!isInput);
    }
    if (m_selectConfigFileBtn) {
        m_selectConfigFileBtn->setVisible(!isInput);
    }
}

QString NetPage::getDefaultConfigContent() const
{
    return {R"(hostname = "YourNodeName"   # 节点名称，可自定义
dhcp = true
listeners = [
    "tcp://0.0.0.0:11010",
    "udp://0.0.0.0:11010",
]      # 监听端口

[network_identity]
network_name = "你的网络名称"
network_secret = "你的网络密钥"

[[peer]]
uri = "txt://qtet-public.070219.xyz"

[flags]
latency_first = true   # 低延迟模式
private_mode = true    # 私有模式
)"};
}

bool NetPage::createTempConfigFile(const QString& networkName)
{
    // 选择文件模式：直接使用所选文件路径，不创建临时文件
    if (m_configSource == ConfigSource::SelectFile) {
        if (!m_configFilePathEdit || m_configFilePathEdit->text().isEmpty()) {
            m_logTextEdit->appendPlainText(tr("错误: 未选择配置文件"));
            return false;
        }
        
        QString filePath = m_configFilePathEdit->text();
        if (!QFile::exists(filePath)) {
            m_logTextEdit->appendPlainText(tr("错误: 配置文件不存在: %1").arg(filePath));
            return false;
        }
        
        // 直接使用所选文件路径
        m_tempConfigFilePath.clear();  // 清空，表示没有临时文件需要清理
        m_logTextEdit->appendPlainText(tr("使用配置文件: %1").arg(filePath));
        return true;
    }
    
    // 下方输入模式：创建临时配置文件
    QString base32Name = base32Encode(networkName.toUtf8());
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    
    // 确保临时目录存在
    if (!QDir().mkpath(tempDir)) {
        m_logTextEdit->appendPlainText(tr("无法创建临时目录: %1").arg(tempDir));
        return false;
    }
    
    m_tempConfigFilePath = QString("%1/QtEasyTier_%2.toml").arg(tempDir, base32Name);
    
    // 获取配置内容
    QString configContent;
    if (m_configTextEdit) {
        configContent = m_configTextEdit->toPlainText();
    }
    
    if (configContent.trimmed().isEmpty()) {
        m_logTextEdit->appendPlainText(tr("错误: 配置内容为空"));
        return false;
    }
    
    // 写入临时配置文件
    QFile tempFile(m_tempConfigFilePath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_logTextEdit->appendPlainText(tr("无法创建临时配置文件: %1").arg(tempFile.errorString()));
        return false;
    }
    
    tempFile.write(configContent.toUtf8());
    tempFile.close();
    
    m_logTextEdit->appendPlainText(tr("已创建临时配置文件: %1").arg(m_tempConfigFilePath));
    return true;
}

void NetPage::cleanupTempConfigFile()
{
    if (!m_tempConfigFilePath.isEmpty()) {
        QFile file(m_tempConfigFilePath);
        if (file.exists()) {
            if (file.remove()) {
                m_logTextEdit->appendPlainText(tr("已清理临时配置文件: %1").arg(m_tempConfigFilePath));
            }
        }
        m_tempConfigFilePath.clear();
    }
}