#include "netpage.h"
#include "ui_netpage.h"
#include "generateconf.h"
#include "cidrcalculator.h"

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
#include <iostream>

NetPage::NetPage(QWidget *parent)
    : QGroupBox(parent)
    , ui(new Ui::NetPage)
    , m_logTextEdit(nullptr)
    , m_easytierProcess(nullptr)
    , m_isRunning(false)
    , realRpcPort(0)
    , m_asyncProcess(nullptr)
{
    ui->setupUi(this);
    createScrollArea();          // 初始化简单设置页面
    createAdvancedSetPage();     // 初始化高级设置页面
    initRunningLogWindow();      // 初始化运行日志窗口
    initRunningStatePage();      // 初始化运行状态页面

    // 连接运行网络按钮的点击事件
    connect(ui->pushButton, &QPushButton::clicked, this, &NetPage::onRunNetwork);
}

NetPage::~NetPage()
{
    // 终止并等待easytier进程结束
    if (m_easytierProcess) {
        // 断开所有信号连接
        disconnect(m_easytierProcess, nullptr, this, nullptr);
        m_logTextEdit->appendPlainText("正在终止EasyTier进程...");
        m_easytierProcess->kill();

        // 等待强制终止完成
        if (m_easytierProcess->waitForFinished(1000)) {
            m_logTextEdit->appendPlainText("EasyTier进程终止成功");
        } else {
            m_logTextEdit->appendPlainText("警告：EasyTier进程可能未完全终止");
        }

        // 清理进程对象
        m_easytierProcess->deleteLater();
        m_easytierProcess = nullptr;
    }

    // 终止并清理异步进程
    if (m_asyncProcess) {
        m_asyncProcess->kill();
        m_asyncProcess->deleteLater();
        m_asyncProcess = nullptr;
    }

    // 清理定时器
    if (m_peerUpdateTimer) {
        m_peerUpdateTimer->stop();
        m_peerUpdateTimer->deleteLater();
        m_peerUpdateTimer = nullptr;
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

    // 私有模式选项
    m_privateModeCheckBox = new QCheckBox(tr("私有模式"), this);
    m_privateModeCheckBox->setChecked(true);

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

    // 连接信号槽
    connect(m_addServerBtn, &QPushButton::clicked, this, &NetPage::onAddServer);
    connect(m_removeServerBtn, &QPushButton::clicked, this, &NetPage::onRemoveServer);
    connect(m_serverListWidget, &QListWidget::itemSelectionChanged, [this]() {
        m_removeServerBtn->setEnabled(m_serverListWidget->selectedItems().count() > 0);
    });
}

void NetPage::onDhcpStateChanged(Qt::CheckState state)
{
    m_ipEdit->setEnabled(state != Qt::Checked);
}

void NetPage::onTogglePasswordVisibility()
{
    if (m_togglePasswordBtn->isChecked()) {
        m_passwordEdit->setEchoMode(QLineEdit::Normal);
        m_togglePasswordBtn->setIcon(QIcon(":/icons/eye.svg"));
    } else {
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_togglePasswordBtn->setIcon(QIcon(":/icons/eye-slash.svg"));
    }
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

void NetPage::onRemoveServer()
{
    QListWidgetItem *selectedItem = m_serverListWidget->currentItem();
    if (selectedItem) {
        delete selectedItem;
        //m_removeServerBtn->setEnabled(false);
    }
}

// 服务器地址补全数据更新
void NetPage::onServerEditCompleterChanged()
{
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

    m_quicInputDisableCheckBox = new QCheckBox(tr("禁用 QUIC 输入"), this);
    m_quicInputDisableCheckBox->setChecked(false);

    m_noTunModeCheckBox = new QCheckBox(tr("无 TUN 模式"), this);
    m_noTunModeCheckBox->setChecked(false);

    m_multithreadCheckBox = new QCheckBox(tr("启用多线程"), this);
    m_multithreadCheckBox->setChecked(true);

    m_udpHolePunchingDisableCheckBox = new QCheckBox(tr("禁用 UDP 打洞"), this);
    m_udpHolePunchingDisableCheckBox->setChecked(false);

    m_userModeStackCheckBox = new QCheckBox(tr("使用用户态协议栈"), this);
    m_userModeStackCheckBox->setChecked(false);

    m_kcpInputDisableCheckBox = new QCheckBox(tr("禁用 KCP 输入"), this);
    m_kcpInputDisableCheckBox->setChecked(false);

    m_p2pDisableCheckBox = new QCheckBox(tr("禁用 P2P"), this);
    m_p2pDisableCheckBox->setChecked(false);

    m_exitNodeCheckBox = new QCheckBox(tr("启用出口节点"), this);
    m_exitNodeCheckBox->setChecked(false);

    m_systemForwardingCheckBox = new QCheckBox(tr("系统转发"), this);
    m_systemForwardingCheckBox->setChecked(false);

    m_symmetricNatHolePunchingDisableCheckBox = new QCheckBox(tr("禁用对称 NAT 打洞"), this);
    m_symmetricNatHolePunchingDisableCheckBox->setChecked(false);

    m_ipv6DisableCheckBox = new QCheckBox(tr("禁用 IPv6"), this);
    m_ipv6DisableCheckBox->setChecked(false);

    m_quicProxyCheckBox = new QCheckBox(tr("启用 QUIC 代理"), this);
    m_quicProxyCheckBox->setChecked(false);

    m_onlyPhysicalNicCheckBox = new QCheckBox(tr("仅使用物理网卡"), this);
    m_onlyPhysicalNicCheckBox->setChecked(true);

    m_rpcPacketForwardingCheckBox = new QCheckBox(tr("转发 RPC 包"), this);
    m_rpcPacketForwardingCheckBox->setChecked(false);

    m_encryptionDisableCheckBox = new QCheckBox(tr("禁用加密"), this);
    m_encryptionDisableCheckBox->setChecked(false);

    m_magicDnsCheckBox = new QCheckBox(tr("启用魔法 DNS"), this);
    m_magicDnsCheckBox->setChecked(false);

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
}

void NetPage::initListenAddrManagement()
{
    // 监听地址输入框
    m_listenAddrEdit = new QLineEdit(this);
    m_listenAddrEdit->setPlaceholderText(tr("请输入监听地址"));

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
    connect(m_cidrListWidget, &QListWidget::itemSelectionChanged, [this]() {
        m_removeCidrBtn->setEnabled(m_cidrListWidget->selectedItems().count() > 0);
    });
    connect(m_calculateCidrBtn, &QPushButton::clicked, this, [] {
        auto cidrCalc = new CIDRCalculator(nullptr);
        cidrCalc->show();
    });
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

    // 创建功能开关标题
    QLabel *functionTitle = new QLabel(tr("功能开关"), this);
    functionTitle->setStyleSheet("font-size: 16px; font-weight: bold;");
    scrollLayout->addWidget(functionTitle);

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

    // 创建监听地址管理分组
    QWidget *listenAddrWidget = new QWidget(scrollContent);
    QVBoxLayout *listenAddrLayout = new QVBoxLayout(listenAddrWidget);
    listenAddrLayout->setContentsMargins(15, 5, 15, 0);

    // 添加监听地址管理标题
    QLabel *listenAddrTitle = new QLabel(tr("监听地址:"), listenAddrWidget);
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
    QListWidgetItem *selectedItem = m_cidrListWidget->currentItem();
    if (selectedItem) {
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


// =====================运行EasyTier相关=======================

// 4. 添加初始化运行日志窗口的方法
void NetPage::initRunningLogWindow()
{
    // 创建日志文本框
    m_logTextEdit = new QPlainTextEdit(ui->runningLog);
    m_logTextEdit->setReadOnly(true);
    //m_logTextEdit->setFont(QFont("Consolas", 9));
    //m_logTextEdit->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4;");

    // 添加到runningLog页面
    QVBoxLayout *logLayout = new QVBoxLayout(ui->runningLog);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->addWidget(m_logTextEdit);
}

// EasyTier按键运行方法
void NetPage::onRunNetwork()
{
    // 如果网络正在运行，则停止它
    if (m_isRunning) {
        if (m_easytierProcess) {
            // 断开所有信号连接
            disconnect(m_easytierProcess, nullptr, this, nullptr);
            m_logTextEdit->appendPlainText("正在终止EasyTier进程...");
            m_easytierProcess->kill();
                
            // 等待强制终止完成
            if (m_easytierProcess->waitForFinished(1000)) {
                m_logTextEdit->appendPlainText("EasyTier进程终止成功");
            } else {
                m_logTextEdit->appendPlainText("警告：EasyTier进程可能未完全终止");
            }

            emit networkFinished(); // 发送网络停止信号
            resetStateDisplay();    // 更新UI状态
            // 清理进程对象
            m_easytierProcess->deleteLater();
            m_easytierProcess = nullptr;
        }
        return;
    }

    // 跳转到运行状态窗口
    ui->netTabWidget->setCurrentWidget(ui->runningState);

    // 清理之前的日志
    m_logTextEdit->clear();
    m_logTextEdit->appendPlainText("正在启动EasyTier网络...");

    // 获取当前程序目录
    QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
    QString easytierPath = appDir + "/easytier-core.exe";

    // 检查程序是否存在
    QFileInfo fileInfo(easytierPath);
    if (!fileInfo.exists()) {
        m_logTextEdit->appendPlainText(QString("错误: 找不到 %1").arg(easytierPath));
        resetStateDisplay();
        return;
    }

    // 创建并配置进程
    m_easytierProcess = new QProcess(this);
    m_easytierProcess->setWorkingDirectory(appDir);

    // 连接信号槽，实时获取进程输出
    connect(m_easytierProcess, &QProcess::readyReadStandardOutput, 
            this, &NetPage::onProcessOutputReady);
    connect(m_easytierProcess, &QProcess::readyReadStandardError, 
            this, &NetPage::onProcessErrorReady);
    // 连接进程完成信号（发生错误）
    connect(m_easytierProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NetPage::onProcessFinished);

    // 新建一个Dialog窗口展示启动过程
    QDialog *dialog = new QDialog(this);
    QVBoxLayout *dialogLayout = new QVBoxLayout(dialog);
    QLabel *dialogTitleLabel = new QLabel("启动日志", dialog);
    QPlainTextEdit *processLogTextEdit = new QPlainTextEdit(dialog);
    // 启动进程
    try {
        dialog->setWindowTitle("启动EasyTier中。。。");
        dialog->setModal(true);

        dialogLayout->setContentsMargins(20, 20, 20, 20);
        dialog->setMinimumSize(400, 300);

        dialogTitleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
        dialogLayout->addWidget(dialogTitleLabel);

        processLogTextEdit->setReadOnly(true);
        processLogTextEdit->setFont(QFont("Consolas", 12));

        dialogLayout->addWidget(processLogTextEdit);
        dialog->setWindowFlag(Qt::WindowCloseButtonHint, false);

        // 显示对话框
        dialog->show();
        processLogTextEdit->appendPlainText("启动EasyTier进程...");
        processLogTextEdit->appendPlainText("生成启动配置...");
        processLogTextEdit->appendPlainText("正在检测RPC端口...");
        QApplication::processEvents();

        QStringList arguments = generateConfCommand(this);

        processLogTextEdit->appendPlainText("检测到可用端口: " + QString::number(realRpcPort));
        processLogTextEdit->appendPlainText("启动配置已生成: " + arguments.join(" "));
        processLogTextEdit->appendPlainText("正在传入参数并启动EasyTier进程...");
        QApplication::processEvents();

        m_easytierProcess->start(easytierPath, arguments);

        // 等待进程启动（最多3秒）
        if (!m_easytierProcess->waitForStarted(3000)) {
            resetStateDisplay();
            processLogTextEdit->appendPlainText("进程启动超时");
            throw std::runtime_error("进程启动超时");
        }
        
        m_logTextEdit->appendPlainText(QString("正在启动 %1").arg(easytierPath));
        m_logTextEdit->appendPlainText(QString("启动参数: %1").arg(arguments.join(" ")));

        // 更新按钮状态
        ui->pushButton->setText("运行中");
        ui->pushButton->setStyleSheet("color: green; font-weight: bold;");
        m_isRunning = true;

        // 更新运行状态标签, 显示节点表格
        m_runningStatusLabel->setText(getNetworkName() + tr(" 网络已运行"));
        m_peerTable->show();

        emit networkStarted();  // 发送网络启动信号
        processLogTextEdit->appendPlainText("EasyTier进程启动成功");
    } catch (const std::exception& e) {
        m_logTextEdit->appendPlainText(QString("启动异常: %1").arg(e.what()));

        QMessageBox::warning(this, "警告", QString("启动异常: %1").arg(e.what()));
        resetStateDisplay();
        if (m_easytierProcess) {
            emit networkFinished(); // 发送网络停止信号
            m_easytierProcess->deleteLater();
            m_easytierProcess = nullptr;
        }
    }
    // 定时器延时一秒关闭对话框
    QTimer::singleShot(800, dialog, &QDialog::deleteLater);
}

// 进程输出处理
void NetPage::onProcessOutputReady()
{
    if (m_easytierProcess) {
        QString output = m_easytierProcess->readAllStandardOutput();
        m_logTextEdit->appendPlainText(output);
    }
}

// 进程错误处理
void NetPage::onProcessErrorReady()
{
    if (m_easytierProcess) {
        QString error = m_easytierProcess->readAllStandardError();
        m_logTextEdit->appendPlainText("错误: " + error);
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
    resetStateDisplay();
    if (m_easytierProcess) {
        m_easytierProcess->deleteLater();
        m_easytierProcess = nullptr;
    }
    emit networkFinished(); // 发送网络停止信号
    QMessageBox::warning(this, "警告", "EasyTier进程异常终止，请前往日志查看详细信息");
}

// 重置运行按钮状态的方法
void NetPage::resetStateDisplay()
{
    // 重置运行状态页面
    m_runningStatusLabel->setText("EasyTier实例未运行，请先点击运行网络");
    m_peerTable->hide();

    // 重置按钮样式
    ui->pushButton->setStyleSheet("");
    ui->pushButton->setText("运行网络");
    m_isRunning = false;
}

// ===================运行状态监测相关===================
// 初始化运行状态页面
void NetPage::initRunningStatePage()
{
    // 创建运行状态标签
    m_runningStatusLabel = new QLabel("EasyTier未运行，请先点击运行网络", ui->runningState);
    m_runningStatusLabel->setAlignment(Qt::AlignCenter);
    m_runningStatusLabel->setStyleSheet("font-size: 16px; font-weight: bold;");

    // 创建节点信息表格
    m_peerTable = new QTableWidget(ui->runningState);
    m_peerTable->setColumnCount(9); // 根据JSON数据字段数量设定列数
    m_peerTable->setHorizontalHeaderLabels({
        tr("主机名"), tr("虚拟IPv4"), tr("连接方式"),
        tr("协议"), tr("延迟(ms)"), tr("丢包率"),
        tr("接收字节"), tr("发送字节"), tr("CIDR")
    });

    // 设置列宽策略，每列最小宽度为100
    QHeaderView *header = m_peerTable->horizontalHeader();
    header->setMinimumSectionSize(100); // 设置最小列宽
    header->setDefaultSectionSize(100); // 设置默认列宽
    m_peerTable->verticalHeader()->setDefaultSectionSize(40); // 设置行高为40
    m_peerTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置表格自适应大小
    m_peerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_peerTable->setSelectionBehavior(QAbstractItemView::SelectRows); // 只能选择整行

    // 布局
    QVBoxLayout *layout = new QVBoxLayout(ui->runningState);
    layout->addWidget(m_runningStatusLabel);
    layout->addWidget(m_peerTable);

    // 移除这行代码：m_asyncProcess = nullptr;  // 初始化异步进程对象
    m_peerTable->hide();       // 初始隐藏表格（当网络未运行时）

    // 设置定时器，定期更新节点信息
    m_peerUpdateTimer = new QTimer(this);
    connect(m_peerUpdateTimer, &QTimer::timeout, this, &NetPage::updatePeerInfo);
    m_peerUpdateTimer->start(2000); // 每2秒更新一次
}

// 更新节点信息
void NetPage::updatePeerInfo() {
    // 如果网络未运行，不执行更新并删除异步进程
    if (!m_isRunning) {
        if (m_asyncProcess) {
            m_asyncProcess->deleteLater();
            m_asyncProcess = nullptr;
        }
        return;
    }

    // et-cli 命令路径信息
    QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
    QString cliPath = appDir + "/easytier-cli.exe";

    // 如果异步进程为nullptr，创建新的进程并结束本次函数
    if (!m_asyncProcess) {
        std::clog << "et已运行，创建异步cli进程"<< std::endl;
        m_asyncProcess = new QProcess(this);

        // 检查CLI程序是否存在
        QFileInfo fileInfo(cliPath);
        if (!fileInfo.exists()) {
            m_logTextEdit->appendPlainText(QString("错误: 找不到 %1").arg(cliPath));
            return;
        }
        m_asyncProcess->setWorkingDirectory(appDir);

        return;
    }

    // 如果检测进程还在运行则终止并报错
    if (m_asyncProcess->state() == QProcess::Running) {
        m_asyncProcess->kill();
        m_logTextEdit->appendPlainText(tr("警告: 获取节点信息超时, CLI进程可能出错"));
        return;
    }

    QByteArray output = m_asyncProcess->readAllStandardOutput();
    QString errorOutput = m_asyncProcess->readAllStandardError();

    if (!errorOutput.isEmpty()) {
        m_logTextEdit->appendPlainText("错误输出: " + errorOutput);
        return;
    }

    if (!output.isEmpty()) {
        parseAndDisplayPeerInfo(output);
    }

    // 再次运行CLI进程
    m_asyncProcess->start(cliPath, QStringList() <<"-p"<<"127.0.0.1:"+QString::number(realRpcPort)<< "-o" << "json" << "peer");
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

        int row = m_peerTable->rowCount();
        m_peerTable->insertRow(row);

        // 按顺序设置各列数据
        m_peerTable->setItem(row, 0, new QTableWidgetItem(peerObj.value("hostname").toString()));
        m_peerTable->setItem(row, 1, new QTableWidgetItem(peerObj.value("ipv4").toString()));
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
    config["isActive"] = m_isRunning; // 记录当前是否处于激活运行状态

    return config;
}

// 从文件中读取并设置网络配置
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
void NetPage::runNetworkOnAutoStart() {
    m_logTextEdit->clear();
    m_logTextEdit->appendPlainText("正在启动EasyTier网络（自启动）...");

    // 获取当前程序目录
    QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
    QString easytierPath = appDir + "/easytier-core.exe";

    // 检查程序是否存在
    QFileInfo fileInfo(easytierPath);
    if (!fileInfo.exists()) {
        m_logTextEdit->appendPlainText(QString("错误: 找不到 %1").arg(easytierPath));
        resetStateDisplay();
        return;
    }

    // 创建并配置进程
    m_easytierProcess = new QProcess(this);
    m_easytierProcess->setWorkingDirectory(appDir);

    // 连接信号槽，实时获取进程输出
    connect(m_easytierProcess, &QProcess::readyReadStandardOutput,
            this, &NetPage::onProcessOutputReady);
    connect(m_easytierProcess, &QProcess::readyReadStandardError,
            this, &NetPage::onProcessErrorReady);
    // 连接进程完成信号（发生错误）
    connect(m_easytierProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NetPage::onProcessFinished);

    try {

        QApplication::processEvents();
        QStringList arguments = generateConfCommand(this);
        QApplication::processEvents();
        m_easytierProcess->start(easytierPath, arguments);

        // 等待进程启动
        if (!m_easytierProcess->waitForStarted(3000)) {
            resetStateDisplay();
            throw std::runtime_error("进程启动超时");
        }

        m_logTextEdit->appendPlainText(QString("正在启动 %1").arg(easytierPath));
        m_logTextEdit->appendPlainText(QString("启动参数: %1").arg(arguments.join(" ")));

        // 更新按钮状态
        ui->pushButton->setText("运行中");
        ui->pushButton->setStyleSheet("color: green; font-weight: bold;");
        m_isRunning = true;

        // 更新运行状态标签, 显示节点表格
        m_runningStatusLabel->setText(getNetworkName() + tr(" 网络已运行"));
        m_peerTable->show();
        emit networkStarted();  // 发送网络启动信号
    } catch (const std::exception& e) {
        m_logTextEdit->appendPlainText(QString("启动异常: %1").arg(e.what()));
        resetStateDisplay();
        if (m_easytierProcess) {
            emit networkFinished(); // 发送网络停止信号
            m_easytierProcess->deleteLater();
            m_easytierProcess = nullptr;
        }
    }
}