#include "oneclick.h"
#include "ui_oneclick.h"
#include "generateconf.h"
#include "publicserver.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <iostream>

OneClick::OneClick(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OneClick)
    , m_hostCodeLineEdit(nullptr)
    , m_serverAddrEdit(nullptr)
    , m_addServerBtn(nullptr)
    , m_removeServerBtn(nullptr)
    , m_publicServerBtn(nullptr)
    , m_hostStartBtn(nullptr)
    , m_serverListWidget(nullptr)
    , m_guestCodeLineEdit(nullptr)
    , m_guestIpLineEdit(nullptr)
    , m_guestStartBtn(nullptr)
    , m_coreProcess(nullptr)
    , m_cliProcess(nullptr)
    , m_processDialog(nullptr)
    , m_processLogTextEdit(nullptr)
    , m_hostIpUpdateTimer(nullptr)
    , m_lastHostIp("")
    , m_currentRole(UserRole::None)
{
    ui->setupUi(this);
    initHostComponents();
    initGuestComponents();
    setupServerList();

    // 连接Tab切换信号
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &OneClick::onTabChanged);

    // 初始化房主IP更新定时器
    m_hostIpUpdateTimer = new QTimer(this);
    connect(m_hostIpUpdateTimer, &QTimer::timeout, this, &OneClick::updateHostIpAddress);
}

OneClick::~OneClick()
{
    stopCurrentProcess();
    closeProcessDialog();

    // 停止并清理IP更新定时器和进程
    if (m_hostIpUpdateTimer) {
        m_hostIpUpdateTimer->stop();
        m_hostIpUpdateTimer->deleteLater();
    }
    if (m_cliProcess) {
        if (m_cliProcess->state() == QProcess::Running) {
            m_cliProcess->kill();
        }
        m_cliProcess->deleteLater();
    }

    delete ui;
}

// 创建或重建启动过程对话框
void OneClick::createProcessDialog(const QString& title) {
    // 如果已有对话框，先删除
    if (m_processDialog) {
        m_processDialog->deleteLater();
        m_processDialog = nullptr;
        m_processLogTextEdit = nullptr;
    }

    // 创建新的对话框
    m_processDialog = new QDialog(this);
    QVBoxLayout *dialogLayout = new QVBoxLayout(m_processDialog);
    QLabel *dialogTitleLabel = new QLabel("启动日志", m_processDialog);
    m_processLogTextEdit = new QPlainTextEdit(m_processDialog);

    m_processDialog->setWindowTitle(title);
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

    m_processLogTextEdit->appendPlainText("启动EasyTier进程...");
    QApplication::processEvents();
}

// 关闭启动过程对话框
void OneClick::closeProcessDialog() {
    if (m_processDialog) {
        m_processDialog->deleteLater();
        m_processDialog = nullptr;
        m_processLogTextEdit = nullptr;
    }
}

void OneClick::initHostComponents() {
    // 获取房主界面的组件
    m_hostCodeLineEdit = ui->lineEdit;  // 房间号显示框
    m_serverAddrEdit = ui->serverAddrEdit;
    m_addServerBtn = ui->addBtn;
    m_removeServerBtn = ui->removeBtn;
    m_publicServerBtn = ui->publicServerBtn;
    m_hostStartBtn = ui->pushButton;  // 开始联机按钮
    m_serverListWidget = ui->serverListWidget;

    // 连接信号槽
    connect(m_addServerBtn, &QPushButton::clicked, this, &OneClick::onAddServerClicked);
    connect(m_removeServerBtn, &QPushButton::clicked, this, &OneClick::onRemoveServerClicked);
    connect(m_publicServerBtn, &QPushButton::clicked, this, &OneClick::onPublicServerClicked);
    connect(m_hostStartBtn, &QPushButton::clicked, this, &OneClick::onHostStartClicked);

    // 默认添加EasyTier公共服务器
    m_serverListWidget->addItem("tcp://public.easytier.top:11010");
    m_serverListWidget->addItem("txt://qtet-public.070219.xyz");
    m_serverListWidget->addItem("https://et-public.lctn.site");
}

void OneClick::initGuestComponents() {
    // 获取房客界面的组件
    m_guestCodeLineEdit = ui->lineEdit_2;  // 房间号输入框
    m_guestIpLineEdit = ui->lineEdit_3;    // IP地址显示框
    m_guestStartBtn = ui->pushButton_2;    // 开始联机按钮

    // 连接信号槽
    connect(m_guestStartBtn, &QPushButton::clicked, this, &OneClick::onGuestStartClicked);

    // 设置房客界面的提示文本
    m_guestCodeLineEdit->setPlaceholderText("请输入联机码");

    // 设置房主虚拟ip框为只读
    m_guestIpLineEdit->setReadOnly(true);
}

void OneClick::setupServerList() {
    // 设置服务器列表的选择模式
    m_serverListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // 连接选择变化信号
    connect(m_serverListWidget, &QListWidget::itemSelectionChanged, [this]() {
        m_removeServerBtn->setEnabled(m_serverListWidget->selectedItems().count() > 0);
    });
}

// 提取的核心启动逻辑
bool OneClick::startEasyTierProcess(const QStringList& arguments, const QString& windowTitle) {
    try {
        m_coreProcess = new QProcess(this);

        // 获取当前程序目录
        QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
        QString easytierPath = appDir + "/easytier-core.exe";

        // 检查程序是否存在
        QFileInfo fileInfo(easytierPath);
        if (!fileInfo.exists()) {
            throw std::runtime_error("无法找到EasyTier-core");
        }

        // 创建并配置进程
        m_coreProcess->setWorkingDirectory(appDir);

        if (m_processLogTextEdit) {
            m_processLogTextEdit->appendPlainText("启动配置已生成");
            m_processLogTextEdit->appendPlainText("正在传入参数并启动EasyTier进程...");
            QApplication::processEvents();
        }

        m_coreProcess->start(easytierPath, arguments);

        // 连接进程结束信号
        connect(m_coreProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &OneClick::onFinishedCore);

        // 等待进程启动（最多3秒）
        if (!m_coreProcess->waitForStarted(3000)) {
            if (m_processLogTextEdit) {
                m_processLogTextEdit->appendPlainText("进程启动超时");
            }
            m_coreProcess->kill();
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

        if (m_coreProcess) {
            m_coreProcess->deleteLater();
            m_coreProcess = nullptr;
        }
        return false;
    }
}

// 停止当前运行的进程
void OneClick::stopCurrentProcess() {
    if (m_coreProcess) {
        disconnect(m_coreProcess, nullptr, this, nullptr);
        if (m_coreProcess->state() == QProcess::Running) {
            m_coreProcess->kill();
            if (m_coreProcess->waitForFinished(1000)) {
                std::clog << "EasyTier进程已完全终止" << std::endl;
            } else {
                std::clog << "警告：EasyTier进程可能未完全终止" << std::endl;
            }
        }
        m_coreProcess->deleteLater();
        m_coreProcess = nullptr;
    }

    // 停止IP更新定时器
    if (m_hostIpUpdateTimer) {
        m_hostIpUpdateTimer->stop();
    }

    // 停止并清理IP查询进程
    if (m_cliProcess) {
        if (m_cliProcess->state() == QProcess::Running) {
            m_cliProcess->kill();
        }
        m_cliProcess->deleteLater();
        m_cliProcess = nullptr;
    }

    // 重置界面状态
    updateInterfaceState(UserRole::None);
}

// 更新界面状态
void OneClick::updateInterfaceState(UserRole role) {
    m_currentRole = role;

    switch (role) {
    case UserRole::None:
        m_hostStartBtn->setText("开始联机");
        m_hostStartBtn->setStyleSheet("");
        m_guestStartBtn->setText("开始联机");
        m_guestStartBtn->setStyleSheet("");
        m_hostCodeLineEdit->setReadOnly(false);
        m_hostCodeLineEdit->clear();
        m_guestIpLineEdit->clear();
        m_lastHostIp.clear();
        break;

    case UserRole::Host:
        m_hostStartBtn->setText("运行中");
        m_hostStartBtn->setStyleSheet("color: green; font-weight: bold;");
        m_guestStartBtn->setText("开始联机");
        m_guestStartBtn->setStyleSheet("");
        break;

    case UserRole::Guest:
        m_guestStartBtn->setText("运行中");
        m_guestStartBtn->setStyleSheet("color: green; font-weight: bold;");
        m_hostStartBtn->setText("开始联机");
        m_hostStartBtn->setStyleSheet("");
        // 启动IP更新定时器
        m_hostIpUpdateTimer->start(2000);
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
        QString currentRoleStr = (m_currentRole == UserRole::Host) ? "房主" : "房客";
        QMessageBox::warning(this, "警告",
            QString("当前正在以%1身份运行，无法切换到另一个身份。\n请先停止当前运行再切换。").arg(currentRoleStr));
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

// 房主点击开始联机按钮
void OneClick::onHostStartClicked() {
    // 如果当前是房主且正在运行，则停止
    if (m_currentRole == UserRole::Host && m_coreProcess) {
        stopCurrentProcess();
        return;
    }

    // 如果当前是房客正在运行，不允许切换
    if (m_currentRole == UserRole::Guest) {
        QMessageBox::warning(this, "警告", "当前正在以房客身份运行，无法切换到房主模式。\n请先停止房客运行。");
        return;
    }

    // 创建启动过程对话框
    createProcessDialog("启动EasyTier中。。。");

    try {
        // 生成房间凭证
        QPair<QString, QString> credentials = generateRoomCredentials();
        m_currentNetworkId = credentials.first;   // 原始网络号（十六进制）
        m_currentPassword = credentials.second;   // 原始密码（十六进制）

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
                  << "--rpc-portal" << "55666";

        // 服务器列表
        for (int i = 0; i < m_serverListWidget->count(); ++i) {
            arguments << "--peers" << m_serverListWidget->item(i)->text();
        }

        if (m_processLogTextEdit) {
            m_processLogTextEdit->appendPlainText("使用RPC端口：55666");
            QApplication::processEvents();
        }

        // 启动进程
        if (startEasyTierProcess(arguments, "启动EasyTier中。。。")) {
            // 显示联机码
            m_hostCodeLineEdit->setText(connectionCode);
            m_hostCodeLineEdit->setReadOnly(true);

            // 输出调试信息
            std::clog << "=== 房主信息 ===" << std::endl;
            std::clog << "联机码: " << connectionCode.toStdString() << std::endl;
            std::clog << "网络号(原始): " << m_currentNetworkId.toStdString() << std::endl;
            std::clog << "密码(原始): " << m_currentPassword.toStdString() << std::endl;

            // 更新状态
            updateInterfaceState(UserRole::Host);

            QTimer::singleShot(800, this, &OneClick::closeProcessDialog);
        } else {
            QTimer::singleShot(1600, this, &OneClick::closeProcessDialog);
        }
    } catch (const std::exception& e) {
        if (m_processLogTextEdit) {
            m_processLogTextEdit->appendPlainText(QString("启动异常: %1").arg(e.what()));
        }
        QMessageBox::warning(this, "警告", QString("启动异常: %1").arg(e.what()));
        QTimer::singleShot(1600, this, &OneClick::closeProcessDialog);
    }
}

void OneClick::onFinishedCore(int exitCode, QProcess::ExitStatus exitStatus) {
    if (m_currentRole == UserRole::Host) {
        QMessageBox::warning(this, "警告", "房主EasyTier进程异常终止");
    } else if (m_currentRole == UserRole::Guest) {
        QMessageBox::warning(this, "警告", "房客EasyTier进程异常终止");
    }
    stopCurrentProcess();
}

void OneClick::onAddServerClicked() {
    QString serverAddress = m_serverAddrEdit->text().trimmed();
    serverAddress.remove(' ');  // 删除所有空格

    if (serverAddress.isEmpty()) {
        QMessageBox::warning(this, "警告", "服务器地址不能为空！");
        return;
    }

    // 检查是否已存在相同的服务器
    for (int i = 0; i < m_serverListWidget->count(); ++i) {
        if (m_serverListWidget->item(i)->text() == serverAddress) {
            QMessageBox::warning(this, "警告", "该服务器已存在！");
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

// 房客点击开始联机按钮
void OneClick::onGuestStartClicked() {
    // 如果当前是房客且正在运行，则停止
    if (m_currentRole == UserRole::Guest && m_coreProcess) {
        stopCurrentProcess();
        return;
    }

    // 如果当前是房主正在运行，不允许切换
    if (m_currentRole == UserRole::Host) {
        QMessageBox::warning(this, "警告", "当前正在以房主身份运行，无法切换到房客模式。\n请先停止房主运行。");
        return;
    }

    QString inputCode = m_guestCodeLineEdit->text().trimmed();

    if (inputCode.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入联机码！");
        return;
    }

    // 解码联机码
    QPair<QString, QString> decoded = decodeConnectionCode(inputCode);

    if (decoded.first.isEmpty() || decoded.second.isEmpty()) {
        QMessageBox::critical(this, "错误", "联机码错误！");
        return;
    }

    QString networkId = decoded.first;   // 解码后的原始网络号（十六进制）
    QString password = decoded.second;   // 解码后的原始密码（十六进制）

    // 输出解码结果到控制台
    std::clog << "=== 房客解码信息 ===" << std::endl;
    std::clog << "输入的联机码: " << inputCode.toStdString() << std::endl;
    std::clog << "解码后的网络号(原始): " << networkId.toStdString() << std::endl;
    std::clog << "解码后的密码(原始): " << password.toStdString() << std::endl;

    // 创建启动过程对话框
    createProcessDialog("启动EasyTier中。。。");

    // 构造房客启动参数
    QStringList arguments;
    arguments << "--private-mode" << "true" << "--enable-kcp-proxy"
              << "--dhcp" << "true" << "--hostname" << "guest"
              << "--network-name" << networkId
              << "--network-secret" << password
              << "--rpc-portal" << "55667";  // 使用不同的RPC端口

    // 服务器列表（使用相同的服务器）
    for (int i = 0; i < m_serverListWidget->count(); ++i) {
        arguments << "--peers" << m_serverListWidget->item(i)->text();
    }

    if (m_processLogTextEdit) {
        m_processLogTextEdit->appendPlainText("使用RPC端口：55667");
        QApplication::processEvents();
    }

    // 启动进程
    if (startEasyTierProcess(arguments, "启动EasyTier中。。。")) {
        // 显示房主的虚拟IP地址（初始化为默认值）
        m_guestIpLineEdit->setText("......");

        // 更新状态
        updateInterfaceState(UserRole::Guest);

        QTimer::singleShot(800, this, &OneClick::closeProcessDialog);
    } else {
        QTimer::singleShot(1600, this, &OneClick::closeProcessDialog);
    }
}

// 更新房主IP地址
void OneClick::updateHostIpAddress() {
    // 如果房客未运行，停止更新
    if (m_currentRole != UserRole::Guest || !m_coreProcess || m_coreProcess->state() != QProcess::Running) {
        return;
    }

    // 如果查询进程还在运行则终止
    if (m_cliProcess && m_cliProcess->state() == QProcess::Running) {
        m_cliProcess->kill();
        return;
    }

    // et-cli 命令路径信息
    QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
    QString cliPath = appDir + "/easytier-cli.exe";

    // 如果异步进程为nullptr，创建新的进程
    if (!m_cliProcess) {
        m_cliProcess = new QProcess(this);

        // 检查CLI程序是否存在
        QFileInfo fileInfo(cliPath);
        if (!fileInfo.exists()) {
            return;
        }
        m_cliProcess->setWorkingDirectory(appDir);

        // 连接进程完成信号
        connect(m_cliProcess, &QProcess::finished, m_cliProcess, [=, this]() {
            QByteArray output = m_cliProcess->readAllStandardOutput();
            QString errorOutput = m_cliProcess->readAllStandardError();

            if (!errorOutput.isEmpty()) {
                return;
            }

            if (!output.isEmpty()) {
                parseHostIpAddress(output);
            }
        });
    }

    // 运行CLI进程查询节点信息
    m_cliProcess->start(cliPath, QStringList() << "-p" << "127.0.0.1:55667" << "-o" << "json" << "peer");
}

// 解析CLI输出获取房主IP
void OneClick::parseHostIpAddress(const QByteArray& jsonData) {
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        return;
    }

    if (!doc.isArray()) {
        return;
    }

    QJsonArray peers = doc.array();
    QString hostIp;

    // 查找hostname为"host"的节点
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
    if (!hostIp.isEmpty() && hostIp != m_lastHostIp) {
        m_guestIpLineEdit->setText(hostIp);
        m_lastHostIp = hostIp;
        std::clog << "房主IP地址更新为: " << hostIp.toStdString() << std::endl;
    }
}
