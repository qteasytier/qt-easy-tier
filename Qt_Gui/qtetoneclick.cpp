#include "qtetoneclick.h"
#include "qtetpublicserverdialog.h"
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QScrollArea>
#include <random>
#include <iostream>

// ==================== Base32 编解码常量 ====================
static const QString BASE32_CHARS = QStringLiteral("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");

QtETOneClick::QtETOneClick(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    initWorkerThread();
}

QtETOneClick::~QtETOneClick()
{
    cleanupWorkerThread();
    closeProgressDialog();
}

bool QtETOneClick::isRunning() const
{
    return m_currentRole == UserRole::Host || m_currentRole == UserRole::Guest;
}

// ==================== UI 初始化 ====================

void QtETOneClick::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    // 创建滚动区域
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 创建滚动区域内容容器
    auto *contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(contentWidget);
    m_contentLayout->setSpacing(10);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);

    // 初始化所有区域（全部放入滚动区域）
    initTitleArea();
    initFormArea();
    initServerArea();
    initButtonArea();

    // 添加弹簧让内容向上对齐
    m_contentLayout->addStretch();

    // 将内容容器放入滚动区域
    scrollArea->setWidget(contentWidget);

    // 将滚动区域添加到主布局
    m_mainLayout->addWidget(scrollArea, 1);

    setupConnections();
}

void QtETOneClick::initTitleArea()
{
    m_titleWidget = new QWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleWidget);
    titleLayout->setSpacing(15);
    titleLayout->setContentsMargins(16, 15, 16, 6);

    // 左侧图标
    m_iconLabel = new QLabel(m_titleWidget);
    m_iconLabel->setMaximumSize(48, 48);
    m_iconLabel->setPixmap(QPixmap(":/icons/icon.ico"));
    m_iconLabel->setScaledContents(true);

    // 标题文字
    m_titleLabel = new QLabel(m_titleWidget);
    QFont titleFont;
    titleFont.setPointSize(20);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setText(QStringLiteral("一键联机"));
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // 右侧图标
    m_rightIconLabel = new QLabel(m_titleWidget);
    m_rightIconLabel->setMaximumSize(48, 48);
    m_rightIconLabel->setPixmap(QPixmap(":/icons/one-click.svg"));
    m_rightIconLabel->setScaledContents(true);

    titleLayout->addWidget(m_iconLabel, 0, Qt::AlignTop);
    titleLayout->addWidget(m_titleLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);
    titleLayout->addWidget(m_rightIconLabel);

    m_contentLayout->addWidget(m_titleWidget, 0, Qt::AlignHCenter | Qt::AlignTop);
}

void QtETOneClick::initFormArea()
{
    m_formWidget = new QWidget(this);
    QFormLayout *formLayout = new QFormLayout(m_formWidget);

    QFont labelFont;
    labelFont.setPointSize(12);

    // 联机房间号/联机码
    m_roomIdLabel = new QLabel(m_formWidget);
    m_roomIdLabel->setFont(labelFont);
    m_roomIdLabel->setText(QStringLiteral("联机码："));

    m_roomIdEdit = new QtETLineEdit(m_formWidget);
    m_roomIdEdit->setFont(labelFont);
    m_roomIdEdit->setAlignment(Qt::AlignCenter);  // 文字居中
    m_roomIdEdit->setReadOnly(false);  // 默认房客模式，可输入
    m_roomIdEdit->setPlaceholderText(QStringLiteral("请输入联机码"));

    // 房主虚拟 IP
    m_hostIpLabel = new QLabel(m_formWidget);
    m_hostIpLabel->setFont(labelFont);
    m_hostIpLabel->setText(QStringLiteral("房主虚拟IP："));

    m_hostIpEdit = new QtETLineEdit(m_formWidget);
    m_hostIpEdit->setFont(labelFont);
    m_hostIpEdit->setAlignment(Qt::AlignCenter);  // 文字居中
    m_hostIpEdit->setReadOnly(true);  // 只读，显示房主 IP
    m_hostIpEdit->setPlaceholderText(QStringLiteral("房客模式显示房主IP"));

    formLayout->addRow(m_roomIdLabel, m_roomIdEdit);
    formLayout->addRow(m_hostIpLabel, m_hostIpEdit);

    m_contentLayout->addWidget(m_formWidget, 0, Qt::AlignTop);

    // 一键联机按钮
    m_oneClickBtn = new QtETPushBtn(this);
    m_oneClickBtn->setMinimumWidth(200);
    m_oneClickBtn->setMaximumWidth(200);
    QFont btnFont;
    btnFont.setPointSize(12);
    m_oneClickBtn->setFont(btnFont);
    m_oneClickBtn->setText(QStringLiteral("开始联机"));

    m_contentLayout->addWidget(m_oneClickBtn, 0, Qt::AlignHCenter);
}

void QtETOneClick::initServerArea()
{
    m_serverWidget = new QWidget(this);
    QGridLayout *serverLayout = new QGridLayout(m_serverWidget);

    // 服务器地址标签
    QLabel *serverLabel = new QLabel(m_serverWidget);
    QFont labelFont;
    labelFont.setPointSize(11);
    serverLabel->setFont(labelFont);
    serverLabel->setText(QStringLiteral("服务器地址："));

    // 服务器地址输入框
    m_serverEdit = new QtETLineEdit(m_serverWidget);

    // 添加按钮
    m_addServerBtn = new QtETPushBtn(m_serverWidget);
    m_addServerBtn->setMinimumWidth(100);
    m_addServerBtn->setMaximumWidth(100);
    m_addServerBtn->setText(QStringLiteral("添加"));

    // 服务器列表
    m_serverListWidget = new QListWidget(m_serverWidget);
    m_serverListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_serverListWidget->setMaximumHeight(120);

    // 右侧按钮布局
    QVBoxLayout *btnLayout = new QVBoxLayout();
    
    // 删除按钮
    m_removeServerBtn = new QtETPushBtn(m_serverWidget);
    m_removeServerBtn->setText(QStringLiteral("删除"));
    m_removeServerBtn->setEnabled(false);  // 默认禁用

    // 公共服务器列表按钮
    m_publicServerBtn = new QtETPushBtn(m_serverWidget);
    m_publicServerBtn->setText(QStringLiteral("服务器列表"));

    btnLayout->addWidget(m_removeServerBtn);
    btnLayout->addWidget(m_publicServerBtn);

    // 添加到网格布局
    serverLayout->addWidget(serverLabel, 0, 0);
    serverLayout->addWidget(m_serverEdit, 1, 0);
    serverLayout->addWidget(m_addServerBtn, 1, 1);
    serverLayout->addWidget(m_serverListWidget, 2, 0);
    serverLayout->addLayout(btnLayout, 2, 1);

    // 添加默认公共服务器
    m_serverListWidget->addItem(QStringLiteral("wss://qtet-public.070219.xyz"));
    m_serverListWidget->addItem(QStringLiteral("tcp://qtet-public2.070219.xyz:27773"));

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

    m_contentLayout->addWidget(m_serverWidget);
}

void QtETOneClick::initButtonArea()
{
    m_bottomWidget = new QWidget(this);
    QGridLayout *bottomLayout = new QGridLayout(m_bottomWidget);

    // 我做房主（默认不勾选，即房客模式）
    m_hostModeCheckBox = new QtETCheckBtn(m_bottomWidget);
    m_hostModeCheckBox->setText(QStringLiteral("我做房主"));
    m_hostModeCheckBox->setChecked(false);  // 默认房客模式
    m_hostModeCheckBox->setBriefTip(tr("打开时作为房主，关闭时作为房客"));

    // 低延迟优先
    m_latencyFirstCheckBox = new QtETCheckBtn(m_bottomWidget);
    m_latencyFirstCheckBox->setText(QStringLiteral("低延迟优先"));
    m_latencyFirstCheckBox->setChecked(true);  // 默认开启
    m_latencyFirstCheckBox->setBriefTip(tr("使用延迟优先的路由策略，提升联机体验"));
    m_latencyFirstCheckBox->setToolTip(tr("注意：该选项可能会影响稳定性，如果遇到问题请关闭该选项"));

    bottomLayout->addWidget(m_hostModeCheckBox, 0, 0);
    bottomLayout->addWidget(m_latencyFirstCheckBox, 0, 1);

    m_contentLayout->addWidget(m_bottomWidget);
}

void QtETOneClick::setupConnections()
{
    // 添加服务器按钮
    connect(m_addServerBtn, &QPushButton::clicked, this, &QtETOneClick::onAddServer);
    
    // 输入框回车添加
    connect(m_serverEdit, &QLineEdit::returnPressed, this, &QtETOneClick::onServerEditReturnPressed);
    
    // 删除服务器按钮
    connect(m_removeServerBtn, &QPushButton::clicked, this, &QtETOneClick::onRemoveServer);
    
    // 服务器列表选中变化
    connect(m_serverListWidget, &QListWidget::itemSelectionChanged, 
            this, &QtETOneClick::onServerSelectionChanged);
    
    // 一键联机按钮
    connect(m_oneClickBtn, &QPushButton::clicked, this, &QtETOneClick::onOneClickBtnClicked);
    
    // 房主模式开关
    connect(m_hostModeCheckBox, &QtETCheckBtn::toggled, this, &QtETOneClick::onHostModeChanged);
    
    // 公共服务器列表按钮
    connect(m_publicServerBtn, &QPushButton::clicked, this, [this]() {
        QtETPublicServerDialog dialog(this);
        
        // 获取当前服务器列表
        QStringList currentServers;
        for (int i = 0; i < m_serverListWidget->count(); ++i) {
            currentServers.append(m_serverListWidget->item(i)->text());
        }
        dialog.setSelectedServers(currentServers);
        
        if (dialog.exec() == QDialog::Accepted) {
            // 获取用户选择的服务器
            QStringList selectedUrls = dialog.selectedServers();
            
            // 添加到服务器列表（避免重复）
            for (const QString &url : selectedUrls) {
                bool exists = false;
                for (int i = 0; i < m_serverListWidget->count(); ++i) {
                    if (m_serverListWidget->item(i)->text() == url) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    m_serverListWidget->addItem(url);
                }
            }
        }
    });
}

// ==================== Worker 线程管理 ====================

void QtETOneClick::initWorkerThread()
{
    m_workerThread = new QThread(this);
    m_workerThread->setObjectName("OneClickWorkerThread");
    
    m_worker = new ETRunWorker();
    m_worker->moveToThread(m_workerThread);
    
    // 连接信号槽
    connect(m_worker, &ETRunWorker::etRunStarted, 
            this, &QtETOneClick::onNetworkStarted, Qt::QueuedConnection);
    connect(m_worker, &ETRunWorker::etRunStopped, 
            this, &QtETOneClick::onNetworkStopped, Qt::QueuedConnection);
    connect(m_worker, &ETRunWorker::infosCollected, 
            this, &QtETOneClick::onInfosCollected, Qt::QueuedConnection);
    
    // 线程结束时清理工作对象
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    
    m_workerThread->start();
    
    // 初始化节点监测定时器
    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &QtETOneClick::onMonitorTimerTimeout);
    
    // 初始化连接超时定时器
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(CONNECTION_TIMEOUT_MS);
    connect(m_connectionTimer, &QTimer::timeout, this, &QtETOneClick::onConnectionTimeout);
}

void QtETOneClick::cleanupWorkerThread()
{
    // 停止监测定时器
    if (m_monitorTimer) {
        m_monitorTimer->stop();
    }
    
    // 停止连接超时定时器
    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }
    
    if (m_workerThread) {
        // 如果正在运行，先停止
        if (isRunning() && m_worker) {
            QMetaObject::invokeMethod(m_worker, "stopNetwork", 
                                      Qt::BlockingQueuedConnection,
                                      Q_ARG(std::string, "QtET-OneClick"));
        }
        
        m_workerThread->quit();
        m_workerThread->wait();
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
        m_worker = nullptr;
    }
}

// ==================== Base32 编解码 ====================

QString QtETOneClick::base32Encode(const QByteArray& data)
{
    QString result;
    int buffer = 0;
    int bitsLeft = 0;

    for (int i = 0; i < static_cast<int>(data.size()); ++i) {
        buffer = (buffer << 8) | static_cast<unsigned char>(data[i]);
        bitsLeft += 8;

        while (bitsLeft >= 5) {
            result += BASE32_CHARS[(buffer >> (bitsLeft - 5)) & 0x1F];
            bitsLeft -= 5;
        }
    }

    if (bitsLeft > 0) {
        result += BASE32_CHARS[(buffer << (5 - bitsLeft)) & 0x1F];
    }

    return result;
}

QByteArray QtETOneClick::base32Decode(const QString& encoded)
{
    QByteArray result;
    int buffer = 0;
    int bitsLeft = 0;

    for (int i = 0; i < static_cast<int>(encoded.length()); ++i) {
        QChar ch = encoded[i];
        if (ch == QLatin1Char('=')) continue;

        int val = static_cast<int>(BASE32_CHARS.indexOf(ch.toUpper()));
        if (val == -1) continue;

        buffer = (buffer << 5) | val;
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            result.append(static_cast<char>((buffer >> (bitsLeft - 8)) & 0xFF));
            bitsLeft -= 8;
        }
    }

    return result;
}

// ==================== 联机码生成/解码 ====================

QPair<QString, QString> QtETOneClick::generateRoomCredentials()
{
    static const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*&#$!";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(charset.size() - 1));

    // 生成7字节的原始网络号数据（前2位固定为"QE"，后5位随机）
    QByteArray networkIdRaw(7, 0);
    networkIdRaw[0] = 'Q';
    networkIdRaw[1] = 'E';
    for (int i = 2; i < 7; ++i) {
        networkIdRaw[i] = charset[dis(gen)];
    }

    // 生成8字节的原始密码数据
    QByteArray passwordRaw(8, 0);
    for (int i = 0; i < 8; ++i) {
        passwordRaw[i] = charset[dis(gen)];
    }

    QString networkId = QStringLiteral("QtET-OneClick-") + QString::fromLatin1(networkIdRaw);
    QString password = QString::fromLatin1(passwordRaw);

    return qMakePair(networkId, password);
}

QString QtETOneClick::encodeConnectionCode(const QString& networkId, const QString& password)
{
    QString networkPureId = networkId;
    networkPureId.remove(QStringLiteral("QtET-OneClick-"));
    
    QByteArray networkIdData = networkPureId.toLatin1();
    QByteArray passwordData = password.toLatin1();

    // 验证网络号前两位是否为"QE"
    if (networkIdData.size() < 2 || networkIdData[0] != 'Q' || networkIdData[1] != 'E') {
        return {};
    }

    QString encodedNetworkId = base32Encode(networkIdData);
    QString encodedPassword = base32Encode(passwordData);

    // 7字节数据Base32编码后为12字符，8字节数据编码后为13字符
    if (encodedNetworkId.length() != 12 || encodedPassword.length() != 13) {
        return {};
    }

    // 组合格式：XXXX-XXXX-XXXX-XXXXX-XXXXX-XXX
    QString code = encodedNetworkId.left(4) + "-" +
                   encodedNetworkId.mid(4, 4) + "-" +
                   encodedNetworkId.mid(8, 4) + "-" +
                   encodedPassword.left(5) + "-" +
                   encodedPassword.mid(5, 5) + "-" +
                   encodedPassword.mid(10, 3);

    return code.toUpper();
}

QPair<QString, QString> QtETOneClick::decodeConnectionCode(const QString& code)
{
    // 移除所有非Base32字符并转为大写
    QString cleanCode = code;
    static const QRegularExpression re("[^A-Za-z2-7]");
    cleanCode.remove(re);
    cleanCode = cleanCode.toUpper();

    // 检查长度（应该是25个Base32字符）
    if (cleanCode.length() != 25) {
        return qMakePair(QString(), QString());
    }

    QString encodedNetworkId = cleanCode.left(12);
    QString encodedPassword = cleanCode.mid(12, 13);

    QByteArray networkIdData = base32Decode(encodedNetworkId);
    QByteArray passwordData = base32Decode(encodedPassword);

    // 验证解码结果长度
    if (networkIdData.size() != 7 || passwordData.size() != 8) {
        return qMakePair(QString(), QString());
    }

    // 验证网络号前两位是否为"QE"
    if (networkIdData[0] != 'Q' || networkIdData[1] != 'E') {
        return qMakePair(QString(), QString());
    }

    QString networkPureId = QString::fromLatin1(networkIdData);
    QString password = QString::fromLatin1(passwordData);

    return qMakePair(QStringLiteral("QtET-OneClick-") + networkPureId, password);
}

// ==================== TOML 配置生成 ====================

std::string QtETOneClick::generateHostTomlConfig()
{
    // 生成房间凭证
    auto credentials = generateRoomCredentials();
    m_currentNetworkId = credentials.first;
    m_currentPassword = credentials.second;

    // 生成联机码
    QString connectionCode = encodeConnectionCode(m_currentNetworkId, m_currentPassword);
    if (connectionCode.isEmpty()) {
        throw std::runtime_error("Failed to generate connection code");
    }

    // 更新 UI 显示联机码
    m_roomIdEdit->setText(connectionCode);
    m_roomIdEdit->setToolTip(tr("如需使用其他EasyTier工具联机请使用以下信息\n"
                                "网络号: %1\n密码: %2").arg(m_currentNetworkId, m_currentPassword));

    // 构建 TOML 配置
    std::ostringstream toml;
    toml << "instance_name = \"QtET-OneClick\"\n";
    toml << "hostname = \"host\"\n";
    toml << "dhcp = false\n";
    toml << "ipv4 = \"11.45.14.1/24\"\n\n";

    toml << "[network_identity]\n";
    toml << "network_name = \"" << m_currentNetworkId.toStdString() << "\"\n";
    toml << "network_secret = \"" << m_currentPassword.toStdString() << "\"\n";

    // 添加服务器列表（peers）
    for (int i = 0; i < m_serverListWidget->count(); ++i) {
        toml << "[[peer]]\n";
        toml << "uri = \"" << m_serverListWidget->item(i)->text().toStdString() << "\"\n";
    }
    toml << "\n";
    
    toml << "[flags]\n";
    toml << "latency_first = " << (m_latencyFirstCheckBox->isChecked() ? "true" : "false") << "\n";
    toml << "private_mode = true\n";

    return toml.str();
}

std::string QtETOneClick::generateGuestTomlConfig()
{
    QString inputCode = m_roomIdEdit->text().trimmed();
    if (inputCode.isEmpty()) {
        throw std::runtime_error(tr("请输入联机码").toStdString());
    }

    // 解码联机码
    auto decoded = decodeConnectionCode(inputCode);
    if (decoded.first.isEmpty() || decoded.second.isEmpty()) {
        throw std::runtime_error(tr("联机码错误").toStdString());
    }

    m_currentNetworkId = decoded.first;
    m_currentPassword = decoded.second;

    std::clog << "=== 房客解码信息 ===" << std::endl;
    std::clog << "网络号: " << m_currentNetworkId.toStdString() << std::endl;
    std::clog << "密码: " << m_currentPassword.toStdString() << std::endl;

    // 构建 TOML 配置
    std::ostringstream toml;
    toml << "instance_name = \"QtET-OneClick\"\n\n";
    toml << "hostname = \"guest\"\n";
    toml << "dhcp = true\n\n";
    toml << "[network_identity]\n";
    toml << "network_name = \"" << m_currentNetworkId.toStdString() << "\"\n";
    toml << "network_secret = \"" << m_currentPassword.toStdString() << "\"\n";
    
    // 添加服务器列表（peers）
    for (int i = 0; i < m_serverListWidget->count(); ++i) {
        toml << "[[peer]]\n";
        toml << "uri = \"" << m_serverListWidget->item(i)->text().toStdString() << "\"\n";
    }
    toml << "\n";
    
    toml << "[flags]\n";
    toml << "latency_first = " << (m_latencyFirstCheckBox->isChecked() ? "true" : "false") << "\n";
    toml << "private_mode = true\n";

    return toml.str();
}

// ==================== 界面状态更新 ====================

void QtETOneClick::updateInterfaceState()
{
    switch (m_currentRole) {
    case UserRole::None:
        m_oneClickBtn->setText(tr("开始联机"));
        m_oneClickBtn->setStyleSheet({});
        m_oneClickBtn->setEnabled(true);
        m_hostModeCheckBox->setEnabled(true);
        m_latencyFirstCheckBox->setEnabled(true);
        m_roomIdEdit->setReadOnly(m_hostModeCheckBox->isChecked());
        m_roomIdEdit->setPlaceholderText(m_hostModeCheckBox->isChecked() ? 
            QStringLiteral("房主模式自动生成") : QStringLiteral("请输入联机码"));
        m_hostIpEdit->clear();
        m_lastHostIp.clear();
        break;

    case UserRole::Host:
        m_oneClickBtn->setText(tr("运行中 - 点击停止"));
        m_oneClickBtn->setStyleSheet("color: green; font-weight: bold;");
        m_oneClickBtn->setEnabled(true);
        m_hostModeCheckBox->setEnabled(false);
        m_latencyFirstCheckBox->setEnabled(false);
        break;

    case UserRole::Guest:
        m_oneClickBtn->setText(tr("运行中 - 点击停止"));
        m_oneClickBtn->setStyleSheet("color: green; font-weight: bold;");
        m_oneClickBtn->setEnabled(true);
        m_hostModeCheckBox->setEnabled(false);
        m_latencyFirstCheckBox->setEnabled(false);
        m_hostIpEdit->setText(QStringLiteral("......"));
        break;

    case UserRole::Stopping:
        m_oneClickBtn->setText(tr("停止中..."));
        m_oneClickBtn->setStyleSheet("color: orange; font-weight: bold;");
        m_oneClickBtn->setEnabled(false);
        break;
    }

    emit networkStateChanged(isRunning());
}

void QtETOneClick::showProgressDialog(const QString& title)
{
    if (m_progressDialog) {
        m_progressDialog->deleteLater();
    }

    m_progressDialog = new QProgressDialog(this);
    m_progressDialog->setWindowTitle(title);
    m_progressDialog->setLabelText(tr("正在启动网络，请稍候..."));
    m_progressDialog->setRange(0, 0);  // 无限进度条
    m_progressDialog->setCancelButton(nullptr);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setMinimumDuration(0);
    m_progressDialog->show();

    // 启动连接超时定时器
    m_connectionTimer->start();
}

void QtETOneClick::closeProgressDialog()
{
    m_connectionTimer->stop();
    
    if (m_progressDialog) {
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
}

// ==================== 槽函数 ====================

void QtETOneClick::onAddServer()
{
    QString serverAddr = m_serverEdit->text().trimmed();
    serverAddr.remove(' ');  // 删除所有空格

    if (serverAddr.isEmpty()) {
        return;
    }

    // 检查是否已存在
    for (int i = 0; i < m_serverListWidget->count(); ++i) {
        if (m_serverListWidget->item(i)->text() == serverAddr) {
            m_serverEdit->clear();
            return;
        }
    }

    // 替换中文冒号
    serverAddr.replace(QStringLiteral("："), QStringLiteral(":"));

    m_serverListWidget->addItem(serverAddr);
    m_serverEdit->clear();
}

void QtETOneClick::onServerEditReturnPressed()
{
    onAddServer();
}

void QtETOneClick::onRemoveServer()
{
    int currentRow = m_serverListWidget->currentRow();
    if (currentRow >= 0) {
        delete m_serverListWidget->takeItem(currentRow);
        if (m_serverListWidget->count() == 0) {
            m_removeServerBtn->setEnabled(false);
        }
    }
}

void QtETOneClick::onServerSelectionChanged()
{
    bool hasSelection = m_serverListWidget->currentRow() >= 0;
    m_removeServerBtn->setEnabled(hasSelection);
}

void QtETOneClick::onHostModeChanged(bool checked)
{
    if (checked) {
        // 房主模式：显示联机码（只读）
        m_roomIdLabel->setText(QStringLiteral("联机码："));
        m_roomIdEdit->setReadOnly(true);
        m_roomIdEdit->setPlaceholderText(QStringLiteral("房主模式自动生成"));
        m_roomIdEdit->clear();
    } else {
        // 房客模式：输入联机码
        m_roomIdLabel->setText(QStringLiteral("联机码："));
        m_roomIdEdit->setReadOnly(false);
        m_roomIdEdit->setPlaceholderText(QStringLiteral("请输入联机码"));
        m_roomIdEdit->clear();
    }
}

void QtETOneClick::onOneClickBtnClicked()
{
    // 如果正在停止中，不处理
    if (m_currentRole == UserRole::Stopping) {
        return;
    }

    // 如果正在运行，停止网络
    if (isRunning()) {
        stopCurrentNetwork();
        return;
    }

    // 启动网络
    try {
        showProgressDialog(tr("启动网络中..."));

        std::string tomlConfig;
        if (m_hostModeCheckBox->isChecked()) {
            // 房主模式
            m_currentRole = UserRole::Host;
            tomlConfig = generateHostTomlConfig();
            
            std::clog << "=== 房主模式 ===" << std::endl;
            std::clog << "联机码: " << m_roomIdEdit->text().toStdString() << std::endl;
        } else {
            // 房客模式
            m_currentRole = UserRole::Guest;
            tomlConfig = generateGuestTomlConfig();
        }

        // 输出 TOML 配置用于调试
        std::clog << "=== TOML 配置 ===" << std::endl;
        std::clog << tomlConfig << std::endl;

        updateInterfaceState();

        // 调用 Worker 启动网络
        QMetaObject::invokeMethod(m_worker, "runNetwork",
                                  Qt::QueuedConnection,
                                  Q_ARG(std::string, "QtET-OneClick"),
                                  Q_ARG(std::string, tomlConfig));

    } catch (const std::exception& e) {
        closeProgressDialog();
        QMessageBox::warning(this, tr("警告"), tr("启动失败: %1").arg(e.what()));
        m_currentRole = UserRole::None;
        updateInterfaceState();
    }
}

void QtETOneClick::stopCurrentNetwork()
{
    if (m_worker && isRunning()) {
        m_currentRole = UserRole::Stopping;
        updateInterfaceState();
        
        QMetaObject::invokeMethod(m_worker, "stopNetwork",
                                  Qt::QueuedConnection,
                                  Q_ARG(std::string, "QtET-OneClick"));
    }
}

void QtETOneClick::onNetworkStarted(const std::string &instName, bool success, const std::string &errorMsg)
{
    Q_UNUSED(instName);
    
    if (!success) {
        closeProgressDialog();
        QMessageBox::warning(this, tr("警告"), tr("启动失败: %1")
            .arg(QString::fromStdString(errorMsg)));
        m_currentRole = UserRole::None;
        updateInterfaceState();
        return;
    }

    // 更新进度对话框
    if (m_progressDialog) {
        m_progressDialog->setLabelText(tr("网络已启动，正在连接服务器..."));
    }

    // 启动节点监测
    m_monitorTimer->start(MONITOR_INTERVAL_MS);
}

void QtETOneClick::onNetworkStopped(const std::string &instName, bool success, const std::string &errorMsg)
{
    Q_UNUSED(instName);
    Q_UNUSED(success);
    Q_UNUSED(errorMsg);
    
    // 停止节点监测
    m_monitorTimer->stop();
    
    m_currentRole = UserRole::None;
    updateInterfaceState();
}

void QtETOneClick::onInfosCollected(const std::vector<EasyTierFFI::KVPair> &infos)
{
    for (const auto &info : infos) {
        if (info.key == "QtET-OneClick") {
            // 解析 JSON
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(info.value));
            // 打印 JSON 内容用于调试
            std::clog << "收到 JSON 内容: " << doc.toJson().toStdString() << std::endl;

            if (!doc.isObject()) continue;
            
            const QJsonObject root = doc.object();
            
            // peers 数组：直连节点（包括公共服务器）
            const QJsonArray peers = root.value("peers").toArray();
            
            // routes 数组：所有节点路由信息
            const QJsonArray routes = root.value("routes").toArray();
            
            // 输出调试信息
            std::clog << "收到节点信息，直连节点数: " << peers.size() 
                      << "，路由节点数: " << routes.size() << std::endl;
            
            // 更新进度对话框（连接到至少一个节点即算成功）
            if (m_progressDialog && !peers.isEmpty()) {
                closeProgressDialog();
            }
            
            parsePeerInfo(peers, routes, root);
            break;
        }
    }
}

/// @brief 将 uint32_t IP 地址转换为点分十进制字符串
/// addr 是网络字节序（大端），最高字节在前
static QString ipAddrToString(quint32 addr)
{
    return QString("%1.%2.%3.%4")
        .arg((addr >> 24) & 0xFF)
        .arg((addr >> 16) & 0xFF)
        .arg((addr >> 8) & 0xFF)
        .arg(addr & 0xFF);
}

void QtETOneClick::parsePeerInfo(const QJsonArray& peers, const QJsonArray& routes, const QJsonObject& root)
{
    Q_UNUSED(peers);  // peers 用于连接检测，在此函数中不使用
    
    if (m_currentRole == UserRole::Host) {
        // 房主模式：显示本机虚拟 IP 并计算联机人数
        
        // 从 my_node_info 获取本机虚拟 IP
        QJsonObject myNodeInfo = root.value("my_node_info").toObject();
        QString localIp;
        
        if (!myNodeInfo.isEmpty()) {
            QJsonObject myIpv4 = myNodeInfo.value("virtual_ipv4").toObject();
            QJsonObject myAddrObj = myIpv4.value("address").toObject();
            quint32 myAddr = myAddrObj.value("addr").toVariant().toUInt();
            if (myAddr != 0) {
                localIp = ipAddrToString(myAddr);
            }
        }
        
        // 更新本机 IP 显示
        if (localIp != m_lastHostIp) {
            m_hostIpEdit->setText(localIp);
            m_lastHostIp = localIp;
            std::clog << "房主本机IP: " << localIp.toStdString() << std::endl;
        }
        
        // 计算联机人数（只计算有虚拟 IP 且不是公共服务器的节点）
        int count = 0;
        
        for (const auto &routeVal : routes) {
            if (!routeVal.isObject()) continue;
            
            QJsonObject routeObj = routeVal.toObject();
            
            // 检查是否为公共服务器
            QJsonObject featureFlag = routeObj.value("feature_flag").toObject();
            bool isPublicServer = featureFlag.value("is_public_server").toBool();
            if (isPublicServer) {
                continue;
            }
            
            // 检查是否有虚拟 IP
            QJsonObject ipv4Addr = routeObj.value("ipv4_addr").toObject();
            QJsonObject addressObj = ipv4Addr.value("address").toObject();
            quint32 addr = addressObj.value("addr").toVariant().toUInt();
            if (addr == 0) {
                continue;
            }
            
            count++;
        }
        
        std::clog << "当前联机人数: " << count << std::endl;
    }
    else if (m_currentRole == UserRole::Guest) {
        // 房客模式：从 routes 中查找房主 IP
        QString hostIp;
        
        for (const auto &routeVal : routes) {
            if (!routeVal.isObject()) continue;
            
            QJsonObject routeObj = routeVal.toObject();
            QString hostname = routeObj.value("hostname").toString();
            
            if (hostname == QLatin1String("host")) {
                // 解析虚拟 IP
                QJsonObject ipv4Addr = routeObj.value("ipv4_addr").toObject();
                QJsonObject addressObj = ipv4Addr.value("address").toObject();
                quint32 addr = addressObj.value("addr").toVariant().toUInt();
                if (addr != 0) {
                    hostIp = ipAddrToString(addr);
                }
                break;
            }
        }

        if (hostIp != m_lastHostIp) {
            if (hostIp.isEmpty()) {
                m_hostIpEdit->setText(QStringLiteral("...... 房主可能已断开"));
            } else {
                m_hostIpEdit->setText(hostIp);
            }
            m_lastHostIp = hostIp;
            std::clog << "房主IP: " << hostIp.toStdString() << std::endl;
        }
    }
}

void QtETOneClick::onConnectionTimeout()
{
    if (m_progressDialog && m_progressDialog->isVisible()) {
        closeProgressDialog();
        QMessageBox::warning(this, tr("警告"), 
            tr("连接服务器超时（%1秒），请检查网络连接后重试。")
            .arg(CONNECTION_TIMEOUT_MS / 1000));
        stopCurrentNetwork();
    }
}

void QtETOneClick::onMonitorTimerTimeout()
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "collectInfos", Qt::QueuedConnection);
    }
}