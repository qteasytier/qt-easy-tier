#include "oneclick.h"
#include "generateconf.h"
#include "ui_oneclick.h"
#include "publicserver.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QIntValidator>
#include <iostream>

// OneClick 专用的 RPC 端口
constexpr int ONECLICK_RPC_PORT = 55666;

OneClick::OneClick(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OneClick)
{
    ui->setupUi(this);
    initHostComponents();
    initGuestComponents();
    setupServerList();
    initWorkerThread();

    // 连接Tab切换信号
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &OneClick::onTabChanged);
}

OneClick::~OneClick()
{
    cleanupWorkerThread();
    closeProcessDialog();
    delete ui;
}

bool OneClick::isRunning() const
{
    return m_worker && m_worker->isRunning();
}

// 初始化Worker线程
void OneClick::initWorkerThread()
{
    // 创建工作线程
    m_workerThread = new QThread(this);
    m_workerThread->setObjectName("OneClickWorkerThread");

    // 创建工作对象（不指定父对象，因为要移动到线程）
    m_worker = new EasyTierWorker();

    // 将工作对象移动到线程
    m_worker->moveToThread(m_workerThread);

    // 连接信号槽（使用Qt::QueuedConnection确保线程安全）
    connect(m_worker, &EasyTierWorker::processStarted,
            this, &OneClick::onWorkerProcessStarted, Qt::QueuedConnection);
    connect(m_worker, &EasyTierWorker::processStopped,
            this, &OneClick::onWorkerProcessStopped, Qt::QueuedConnection);
    connect(m_worker, &EasyTierWorker::logOutput,
            this, &OneClick::onWorkerLogOutput, Qt::QueuedConnection);
    connect(m_worker, &EasyTierWorker::peerInfoUpdated,
            this, &OneClick::onWorkerPeerInfoUpdated, Qt::QueuedConnection);
    connect(m_worker, &EasyTierWorker::processCrashed,
            this, &OneClick::onWorkerProcessCrashed, Qt::QueuedConnection);

    // 线程结束时清理工作对象
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // 启动线程
    m_workerThread->start();
}

// 清理Worker线程
void OneClick::cleanupWorkerThread()
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

// 显示启动过程对话框（无限进度条）
void OneClick::showProcessDialog(const QString& title)
{
    if (m_processDialog) {
        m_processDialog->deleteLater();
    }

    m_processDialog = new QDialog(this);
    m_processDialog->setWindowTitle(title);
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
    QTimer::singleShot(EASYTIER_START_TIMEOUT_MS, this, [this]() {
        if (m_processDialog && m_processDialog->isVisible()) {
            closeProcessDialog();
            QMessageBox::warning(this, tr("警告"), tr("EasyTier进程启动超时"));
            stopCurrentProcess();
        }
    });
}

// 关闭启动过程对话框
void OneClick::closeProcessDialog()
{
    if (m_processDialog) {
        m_processDialog->close();
        m_processDialog->deleteLater();
        m_processDialog = nullptr;
        m_progressBar = nullptr;
        m_progressLabel = nullptr;
    }
}

void OneClick::initHostComponents() {
    // 获取房主界面的组件
    m_hostCodeLineEdit = ui->roomNumEdit;  // 房间号显示框
    m_serverAddrEdit = ui->serverAddrEdit;
    m_addServerBtn = ui->addBtn;
    m_removeServerBtn = ui->removeBtn;
    m_publicServerBtn = ui->publicServerBtn;
    m_hostStartBtn = ui->hostStartBtn;  // 开始联机按钮
    m_serverListWidget = ui->serverListWidget;
    m_playerCountEdit = ui->playerCountEdit;

    // 连接信号槽
    connect(m_addServerBtn, &QPushButton::clicked, this, &OneClick::onAddServerClicked);
    connect(m_removeServerBtn, &QPushButton::clicked, this, &OneClick::onRemoveServerClicked);
    connect(m_publicServerBtn, &QPushButton::clicked, this, &OneClick::onPublicServerClicked);
    connect(m_hostStartBtn, &QPushButton::clicked, this, &OneClick::onHostStartClicked);

    // 默认添加EasyTier公共服务器
    m_serverListWidget->addItem("txt://qtet-public.070219.xyz");
    m_serverListWidget->addItem("tcp://public.easytier.top:11010");
    m_serverListWidget->addItem("tcp://public2.easytier.cn:54321");
    m_serverListWidget->addItem("https://et-public.lctn.site");
    m_serverListWidget->addItem("tcp://et.sbgov.cn:11010");
    m_serverListWidget->addItem("tcp://turn.js.629957.xyz:11012");
}

void OneClick::initGuestComponents() {
    // 获取房客界面的组件
    m_guestCodeLineEdit = ui->guestRoomNumEdit;  // 房间号输入框
    m_guestIpLineEdit = ui->ipEdit;    // IP地址显示框
    m_guestStartBtn = ui->guestStartBtn;    // 开始联机按钮

    // 连接信号槽
    connect(m_guestStartBtn, &QPushButton::clicked, this, &OneClick::onGuestStartClicked);

    // 设置房客界面的提示文本
    m_guestCodeLineEdit->setPlaceholderText("请输入联机码");

    // 设置房主虚拟ip框为只读
    m_guestIpLineEdit->setReadOnly(true);

    // 设置房客编号输入验证器
    QIntValidator *guestNumValidator = new QIntValidator(1, 253, ui->guestNumEdit);
    ui->guestNumEdit->setValidator(guestNumValidator);
}

void OneClick::setupServerList() {
    // 设置服务器列表的选择模式
    m_serverListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // 连接选择变化信号
    connect(m_serverListWidget, &QListWidget::itemSelectionChanged, [this]() {
        m_removeServerBtn->setEnabled(m_serverListWidget->selectedItems().count() > 0);
    });
}

// 停止当前运行的进程
void OneClick::stopCurrentProcess() {
    if (m_worker && m_worker->isRunning()) {
        // 通过信号槽调用Worker的stopEasyTier方法
        QMetaObject::invokeMethod(m_worker, "stopEasyTier", Qt::QueuedConnection);
    }
    // 重置界面状态
    updateInterfaceState(UserRole::None);
}

// 更新界面状态
void OneClick::updateInterfaceState(UserRole role) {
    m_currentRole = role;

    switch (role) {
    case UserRole::None:
        m_hostStartBtn->setText(tr("开始联机"));
        m_hostStartBtn->setStyleSheet("");
        m_guestStartBtn->setText(tr("开始联机"));
        m_guestStartBtn->setStyleSheet("");
        m_hostCodeLineEdit->setToolTip("");
        m_hostCodeLineEdit->clear();
        m_guestIpLineEdit->clear();
        m_lastHostIp.clear();
        m_playerCountEdit->clear();
        break;

    case UserRole::Host:
        m_hostStartBtn->setText(tr("运行中"));
        m_hostStartBtn->setStyleSheet("color: green; font-weight: bold;");
        m_guestStartBtn->setText(tr("开始联机"));
        m_guestStartBtn->setStyleSheet("");
        break;

    case UserRole::Guest:
        m_guestStartBtn->setText(tr("运行中"));
        m_guestStartBtn->setStyleSheet("color: green; font-weight: bold;");
        m_hostStartBtn->setText(tr("开始联机"));
        m_hostStartBtn->setStyleSheet("");
        break;
    }
}

// 验证Tab切换是否允许
bool OneClick::canSwitchToTab(int tabIndex) {
    // 如果当前没有运行任何角色，允许切换
    if (m_currentRole == UserRole::None) {
        return true;
    }

    // 如果要切换到房主Tab且当前是房客，或反之，则不允许
    if ((tabIndex == 0 && m_currentRole == UserRole::Guest) ||
        (tabIndex == 1 && m_currentRole == UserRole::Host)) {
        QString currentRoleStr = (m_currentRole == UserRole::Host) ? tr("房主") : tr("房客");
        QMessageBox::warning(this, tr("警告"),
            tr("当前正在以%1身份运行，无法切换到另一个身份。\n请先停止当前运行再切换。").arg(currentRoleStr));
        return false;
    }

    return true;
}

// Tab切换处理
void OneClick::onTabChanged(int index) {
    if (!canSwitchToTab(index)) {
        // 恢复到原来的Tab
        int currentIndex = (m_currentRole == UserRole::Host) ? 0 : 1;
        ui->tabWidget->setCurrentIndex(currentIndex);
    }
}

// Worker信号处理
void OneClick::onWorkerProcessStarted(bool success, const QString& errorMessage)
{
    closeProcessDialog();

    if (!success) {
        updateInterfaceState(UserRole::None);
        if (!errorMessage.isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("启动失败: %1").arg(errorMessage));
        }
    }
}

void OneClick::onWorkerProcessStopped(bool success)
{
    updateInterfaceState(UserRole::None);
    Q_UNUSED(success);
}

void OneClick::onWorkerLogOutput(const QString& logText, bool isError)
{
    Q_UNUSED(logText)
    Q_UNUSED(isError)
    // OneClick 模式下不显示日志到界面，只输出到控制台
    std::clog << logText.toStdString() << std::endl;
}

void OneClick::onWorkerPeerInfoUpdated(const QJsonArray& peers)
{
    parsePeerInfo(peers);
}

void OneClick::onWorkerProcessCrashed(int exitCode)
{
    closeProcessDialog();

    if (m_currentRole == UserRole::Host) {
        QMessageBox::warning(this, tr("警告"), tr("房主EasyTier进程异常终止，退出码: %1").arg(exitCode));
    } else if (m_currentRole == UserRole::Guest) {
        QMessageBox::warning(this, tr("警告"), tr("房客EasyTier进程异常终止，退出码: %1").arg(exitCode));
    }
    updateInterfaceState(UserRole::None);
}

// 解析节点信息
void OneClick::parsePeerInfo(const QJsonArray& peers)
{
    if (m_currentRole == UserRole::Host) {
        // 房主模式：计算联机人数
        int count = 0;
        for (const auto &peerValue : peers) {
            if (!peerValue.isObject()) continue;

            QJsonObject peerObj = peerValue.toObject();
            QString hostname = peerObj.value("hostname").toString();
            QString ipv4 = peerObj.value("ipv4").toString();

            if (hostname.contains("PublicServer") || ipv4.isEmpty()) {
                continue;
            }
            count++;
        }
        m_playerCountEdit->setText(QString::number(count));
    }
    else if (m_currentRole == UserRole::Guest) {
        // 房客模式：查找房主IP
        QString hostIp;
        for (const auto &peerValue : peers) {
            if (!peerValue.isObject()) continue;

            QJsonObject peerObj = peerValue.toObject();
            QString hostname = peerObj.value("hostname").toString();

            if (hostname == "host") {
                hostIp = peerObj.value("ipv4").toString();
                break;
            }
        }

        // 只有IP有变动时才更新显示
        if (hostIp != m_lastHostIp) {
            if (hostIp.isEmpty()) {
                m_guestIpLineEdit->setText("......" + tr("房主可能已经断开连接"));
            } else {
                m_guestIpLineEdit->setText(hostIp);
            }
            m_lastHostIp = hostIp;
            std::clog << "The IP of host has been changed: " << hostIp.toStdString() << std::endl;
        }
    }
}

void OneClick::onAddServerClicked() {
    QString serverAddress = m_serverAddrEdit->text().trimmed();
    serverAddress.remove(' ');  // 删除所有空格

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
    m_serverAddrEdit->clear();
}

void OneClick::onRemoveServerClicked() {
    QListWidgetItem *selectedItem = m_serverListWidget->currentItem();
    if (selectedItem) {
        delete selectedItem;
    }
}

void OneClick::onPublicServerClicked() {
    auto publicServer = new PublicServer(this);
    publicServer->exec();
    const QStringList serverList = publicServer->getSelectedServers();

    // 添加到服务器列表
    for (const auto &addr : serverList) {
        // 检查是否重复
        bool isDuplicate = false;
        for (int i = 0; i < m_serverListWidget->count(); ++i) {
            if (m_serverListWidget->item(i)->text() == addr) {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate) {
            m_serverListWidget->addItem(addr);
        }
    }
    publicServer->deleteLater();
}

// 房主点击开始联机按钮
void OneClick::onHostStartClicked() {
    // 如果当前是房主且正在运行，则停止
    if (m_currentRole == UserRole::Host && isRunning()) {
        stopCurrentProcess();
        return;
    }

    // 如果当前是房客正在运行，不允许切换
    if (m_currentRole == UserRole::Guest) {
        QMessageBox::warning(this, tr("警告"), tr("当前正在以房客身份运行，无法切换到房主模式。\n请先停止房客运行。"));
        return;
    }

    // 检测端口占用
    if (isPortOccupied(ONECLICK_RPC_PORT)) {
        QMessageBox::warning(this, tr("错误"), tr("端口55666被占用，请检查是否其他程序正在使用。"));
        return;
    }

    // 显示启动过程对话框
    showProcessDialog(tr("启动EasyTier中..."));

    try {
        // 生成房间凭证
        QPair<QString, QString> credentials = generateRoomCredentials();
        m_currentNetworkId = credentials.first;   // 原始网络号
        m_currentPassword = credentials.second;   // 原始密码

        // 生成联机码（Base32编码）
        QString connectionCode = encodeConnectionCode(m_currentNetworkId, m_currentPassword);

        if (connectionCode.isEmpty()) {
            throw std::runtime_error("无法生成房间凭证");
        }

        QStringList arguments;
        arguments << "--private-mode" << "true" << "--enable-kcp-proxy"
                  << "--dhcp" << "false" << "--hostname" << "host"
                  << "--ipv4" << "11.45.14.1/24"
                  << "--network-name" << m_currentNetworkId
                  << "--network-secret" << m_currentPassword
                  << "--rpc-portal" << QString::number(ONECLICK_RPC_PORT);

        // 服务器列表
        for (int i = 0; i < m_serverListWidget->count(); ++i) {
            arguments << "--peers" << m_serverListWidget->item(i)->text();
        }

        // 输出调试信息
        std::clog << "=== 房主信息 ===" << std::endl;
        std::clog << "联机码: " << connectionCode.toStdString() << std::endl;
        std::clog << "网络号(原始): " << m_currentNetworkId.toStdString() << std::endl;
        std::clog << "密码(原始): " << m_currentPassword.toStdString() << std::endl;

        // 准备程序路径
        QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
        QString easytierPath = appDir + "/easytier-core.exe";

        // 检查程序是否存在
        QFileInfo fileInfo(easytierPath);
        if (!fileInfo.exists()) {
            throw std::runtime_error("无法找到EasyTier-core");
        }

        // 显示联机码
        m_hostCodeLineEdit->setText(connectionCode);
        m_hostCodeLineEdit->setReadOnly(true);

        // 鼠标放上去显示原始账密
        m_hostCodeLineEdit->setToolTip(QString(tr("如需使用其他EasyTier工具联机请使用以下信息"
            "\n网络号: %1\n密码: %2").arg(m_currentNetworkId, m_currentPassword)));

        // 更新状态
        updateInterfaceState(UserRole::Host);

        // 通过信号槽调用Worker的startEasyTier方法
        QMetaObject::invokeMethod(m_worker, "startEasyTier",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, m_currentNetworkId),
                                  Q_ARG(QStringList, arguments),
                                  Q_ARG(QString, appDir),
                                  Q_ARG(QString, easytierPath),
                                  Q_ARG(int, ONECLICK_RPC_PORT));

    } catch (const std::exception& e) {
        closeProcessDialog();
        QMessageBox::warning(this, tr("警告"), tr("启动异常: %1").arg(e.what()));
        updateInterfaceState(UserRole::None);
    }
}

// 房客点击开始联机按钮
void OneClick::onGuestStartClicked() {
    // 如果当前是房客且正在运行，则停止
    if (m_currentRole == UserRole::Guest && isRunning()) {
        stopCurrentProcess();
        return;
    }

    // 如果当前是房主正在运行，不允许切换
    if (m_currentRole == UserRole::Host) {
        QMessageBox::warning(this, tr("警告"), tr("当前正在以房主身份运行，无法切换到房客模式。\n请先停止房主运行。"));
        return;
    }

    QString inputCode = m_guestCodeLineEdit->text().trimmed();

    if (inputCode.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请输入联机码！"));
        return;
    }

    // 解码联机码
    QPair<QString, QString> decoded = decodeConnectionCode(inputCode);

    if (decoded.first.isEmpty() || decoded.second.isEmpty()) {
        QMessageBox::critical(this, tr("错误"), tr("联机码错误！"));
        return;
    }

    QString networkId = decoded.first;   // 解码后的原始网络号
    QString password = decoded.second;   // 解码后的原始密码

    // 输出解码结果到控制台
    std::clog << "=== 房客解码信息 ===" << std::endl;
    std::clog << "输入的联机码: " << inputCode.toStdString() << std::endl;
    std::clog << "解码后的网络号(原始): " << networkId.toStdString() << std::endl;
    std::clog << "解码后的密码(原始): " << password.toStdString() << std::endl;

    // 检测端口占用
    if (isPortOccupied(ONECLICK_RPC_PORT)) {
        QMessageBox::warning(this, tr("错误"), tr("端口55666被占用，请检查是否其他程序正在使用。"));
        return;
    }

    // 显示启动过程对话框
    showProcessDialog(tr("启动EasyTier中..."));

    try {
        // 准备程序路径
        QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
        QString easytierPath = appDir + "/easytier-core.exe";

        // 检查程序是否存在
        QFileInfo fileInfo(easytierPath);
        if (!fileInfo.exists()) {
            throw std::runtime_error("无法找到EasyTier-core");
        }

        // 构造房客启动参数
        QStringList arguments {"--dhcp"};
        if (!ui->guestNumEdit->text().isEmpty() && ui->guestNumEdit->text().toInt() > 0) {
            arguments << "false" << "--ipv4" << "11.45.14."+QString::number(ui->guestNumEdit->text().toInt()+1);
        } else {
            arguments << "true";
        }
        arguments << "--private-mode" << "true" << "--enable-kcp-proxy"
                  << "--latency-first" << "--hostname" << "guest"
                  << "--network-name" << networkId
                  << "--network-secret" << password
                  << "--rpc-portal" << QString::number(ONECLICK_RPC_PORT);

        // 服务器列表（使用相同的服务器）
        for (int i = 0; i < m_serverListWidget->count(); ++i) {
            arguments << "--peers" << m_serverListWidget->item(i)->text();
        }

        // 显示房主的虚拟IP地址（初始化为默认值）
        m_guestIpLineEdit->setText("......");

        // 更新状态
        updateInterfaceState(UserRole::Guest);

        // 通过信号槽调用Worker的startEasyTier方法
        QMetaObject::invokeMethod(m_worker, "startEasyTier",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, networkId),
                                  Q_ARG(QStringList, arguments),
                                  Q_ARG(QString, appDir),
                                  Q_ARG(QString, easytierPath),
                                  Q_ARG(int, ONECLICK_RPC_PORT));

    } catch (const std::exception& e) {
        closeProcessDialog();
        QMessageBox::warning(this, tr("警告"), tr("启动异常: %1").arg(e.what()));
        updateInterfaceState(UserRole::None);
    }
}