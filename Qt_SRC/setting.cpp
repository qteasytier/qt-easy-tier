#include "setting.h"
#include "ui_setting.h"

#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QTimer>
#include <QNetworkReply>
#include <QIntValidator>
#include <iostream>

// =============================================================================
// VersionDetectionWorker 实现
// =============================================================================

VersionDetectionWorker::VersionDetectionWorker(QObject *parent)
    : QObject(parent)
{
}

VersionDetectionWorker::~VersionDetectionWorker()
{
    // 清理进程对象（确保线程安全的资源释放）
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
        m_process->deleteLater();
    }
}

void VersionDetectionWorker::startDetection()
{
    // 重置检测状态
    m_coreDetected = false;
    m_cliDetected = false;
    m_webDetected = false;

    // 获取可执行文件目录
    QString execDir = m_executableDir.isEmpty() ? findExecutableDir() : m_executableDir;
    QString corePath = execDir + "/easytier-core.exe";

    // 先检测核心版本
    detectSingleExecutable(corePath, "core");
}

void VersionDetectionWorker::detectSingleExecutable(const QString &executablePath, const QString &type)
{
    m_currentDetectType = type;

    // 检查文件是否存在
    if (!QFile::exists(executablePath)) {
        if (type == "core") {
            m_coreDetected = true;
            emit coreVersionReady(QString());
            // 继续检测 CLI
            QString execDir = m_executableDir.isEmpty() ? findExecutableDir() : m_executableDir;
            detectSingleExecutable(execDir + "/easytier-cli.exe", "cli");
        } else if (type == "cli") {
            m_cliDetected = true;
            emit cliVersionReady(QString());
            // 继续检测 Web
            QString execDir = m_executableDir.isEmpty() ? findExecutableDir() : m_executableDir;
            detectSingleExecutable(execDir + "/easytier-web-embed.exe", "web");
        } else {
            m_webDetected = true;
            emit webVersionReady(QString());
            emit detectionFinished();
        }
        return;
    }

    // 清理旧进程（确保不会有残留进程占用资源）
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
        m_process->deleteLater();
    }

    // 创建新进程（进程对象作为 worker 的子对象，随 worker 一起销毁）
    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VersionDetectionWorker::onProcessFinished);

    // 启动进程获取版本（异步执行）
    m_process->start(executablePath, QStringList{"-V"});
}

void VersionDetectionWorker::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    QString version;

    if (exitCode == 0 && m_process) {
        QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput()).trimmed();
        version = parseVersionOutput(output, m_currentDetectType);
    }

    // 根据当前检测类型发送信号并继续下一个检测
    QString execDir = m_executableDir.isEmpty() ? findExecutableDir() : m_executableDir;

    if (m_currentDetectType == "core") {
        m_coreDetected = true;
        emit coreVersionReady(version);
        // 继续检测 CLI
        detectSingleExecutable(execDir + "/easytier-cli.exe", "cli");
    } else if (m_currentDetectType == "cli") {
        m_cliDetected = true;
        emit cliVersionReady(version);
        // 继续检测 Web
        detectSingleExecutable(execDir + "/easytier-web-embed.exe", "web");
    } else {
        m_webDetected = true;
        emit webVersionReady(version);
        emit detectionFinished();
    }
}

QString VersionDetectionWorker::parseVersionOutput(const QString &output, const QString &type)
{
    QString result = output;

    // 移除程序名前缀，只保留版本号
    if (type == "core" && result.startsWith("easytier-core", Qt::CaseInsensitive)) {
        result = result.mid(13).trimmed();
    } else if (type == "cli" && result.startsWith("easytier-cli", Qt::CaseInsensitive)) {
        result = result.mid(12).trimmed();
    } else if (type == "web" && result.startsWith("easytier-web", Qt::CaseInsensitive)) {
        result = result.mid(12).trimmed();
    }

    return result.isEmpty() ? QString("未知") : result;
}

QString VersionDetectionWorker::findExecutableDir() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList searchPaths = {
        appDir + "/etcore",
        appDir + "/../EasyTier",
        appDir
    };

    for (const QString &path : searchPaths) {
        if (QDir(path).exists()) {
            return QDir::cleanPath(path);
        }
    }

    return appDir;
}

// =============================================================================
// Settings 实现
// =============================================================================

Settings::Settings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::setting)
{
    ui->setupUi(this);

    // 初始化网络管理器（作为 Settings 的子对象，随 Settings 一起销毁）
    m_networkManager = new QNetworkAccessManager(this);

    // 加载设置
    loadSettings();

    // 设置界面初始状态
    setupUi();
    setupWebConfigUi();

    // 连接信号槽
    connectSignals();
    
    // 设置端口输入验证
    setupPortValidation();

    // 增加启动计数
    incrementLaunchCount();
}

Settings::~Settings()
{
    // 取消正在进行的网络请求
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    // 清理版本检测线程（确保线程安全退出）
    cleanupVersionThread();

    delete ui;
}

void Settings::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // 窗口显示时启动版本检测
    startVersionDetection();
}

void Settings::setupUi()
{
    ui->autoRuncheckBox->setChecked(m_autoRun);
    ui->versionLabel->setText(m_softwareVer);
    ui->hideOnTrayBox->setChecked(m_isHideOnTray);
    ui->autoStartCheckBox->setChecked(m_autoStart);
    ui->autoUpdateCheckBox->setChecked(m_autoUpdate);

    // 设置日志保存天数下拉框
    int daysIndex = 0;
    if (m_logRetentionDays == 0) daysIndex = 0;
    else if (m_logRetentionDays == 3) daysIndex = 1;
    else if (m_logRetentionDays == 7) daysIndex = 2;
    else if (m_logRetentionDays == 14) daysIndex = 3;
    else if (m_logRetentionDays == 30) daysIndex = 4;
    ui->logRetentionDaysBox->setCurrentIndex(daysIndex);
}

void Settings::setupWebConfigUi()
{
    // 配置下发端口
    ui->confSendEdit->setText(QString::number(m_webConfig.configPort));
    
    // 控制台前端端口
    ui->webPageEdit->setText(QString::number(m_webConfig.webPagePort));
    
    // 使用本地 API
    ui->isUseLocalApiBox->setChecked(m_webConfig.useLocalApi);
    
    // API 地址
    ui->apiAddrEdit->setText(m_webConfig.apiAddress);
    ui->apiAddrEdit->setEnabled(!m_webConfig.useLocalApi);
    
    // Core 连接地址
    ui->coreConnectAddrEdit->setText(m_webConfig.coreConnectAddress);
    
    // 配置下发协议
    int protocolIndex = 0;
    if (m_webConfig.configProtocol == "TCP") {
        protocolIndex = 1;
    } else if (m_webConfig.configProtocol == "WebSocket") {
        protocolIndex = 2;
    }
    ui->configProtocolBox->setCurrentIndex(protocolIndex);
}

void Settings::connectSignals()
{
    // UI 按钮信号
    connect(ui->detAgainPushButton, &QPushButton::clicked, this, &Settings::onRedetectButtonClicked);
    connect(ui->detectWebBtn, &QPushButton::clicked, this, &Settings::onRedetectWebButtonClicked);
    connect(ui->openFileBtn, &QPushButton::clicked, this, &Settings::onOpenFolderButtonClicked);
    connect(ui->newVerPushButton, &QPushButton::clicked, this, &Settings::onCheckUpdateButtonClicked);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &Settings::onDialogAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &Settings::onDialogRejected);
    connect(ui->clearLogBtn, &QPushButton::clicked, this, &Settings::onClearLogClicked);
    connect(ui->openLogDirBtn, &QPushButton::clicked, this, &Settings::onOpenLogDirClicked);
    
    // 使用本地 API 复选框状态变化
    connect(ui->isUseLocalApiBox, &QCheckBox::checkStateChanged, this, &Settings::onUseLocalApiChanged);
}

// =============================================================================
// 版本检测线程管理
// =============================================================================

void Settings::startVersionDetection()
{
    // 防止重复检测
    if (m_versionDetecting) {
        return;
    }

    // 清理之前的线程
    cleanupVersionThread();

    m_versionDetecting = true;

    // 创建线程和工作对象
    m_versionThread = new QThread(this);
    m_versionWorker = new VersionDetectionWorker();

    // 将 worker 移动到工作线程（符合 Qt 线程绑定规范）
    m_versionWorker->moveToThread(m_versionThread);

    // 连接信号
    // 线程启动时开始检测
    connect(m_versionThread, &QThread::started, m_versionWorker, &VersionDetectionWorker::startDetection);
    // 线程结束时清理 worker
    connect(m_versionThread, &QThread::finished, m_versionWorker, &QObject::deleteLater);

    // 版本检测结果信号（使用 QueuedConnection 确保跨线程安全）
    connect(m_versionWorker, &VersionDetectionWorker::coreVersionReady,
            this, &Settings::onCoreVersionReady, Qt::QueuedConnection);
    connect(m_versionWorker, &VersionDetectionWorker::cliVersionReady,
            this, &Settings::onCliVersionReady, Qt::QueuedConnection);
    connect(m_versionWorker, &VersionDetectionWorker::webVersionReady,
            this, &Settings::onWebVersionReady, Qt::QueuedConnection);
    connect(m_versionWorker, &VersionDetectionWorker::detectionFinished,
            this, &Settings::onVersionDetectionFinished, Qt::QueuedConnection);

    // 启动线程
    m_versionThread->start();
}

void Settings::cleanupVersionThread()
{
    if (!m_versionThread) {
        return;
    }

    if (m_versionThread->isRunning()) {
        // 请求线程退出
        m_versionThread->quit();
        // 等待线程结束（最多 2 秒）
        if (!m_versionThread->wait(2000)) {
            // 超时则强制终止（最后手段）
            m_versionThread->terminate();
            m_versionThread->wait(500);
        }
    }

    // 线程对象会通过 finished 信号自动清理 worker
    // 但需要手动删除线程对象（它有父对象，析构时会自动删除）
    m_versionThread = nullptr;
    m_versionWorker = nullptr;
}

// =============================================================================
// 版本检测结果处理
// =============================================================================

void Settings::onCoreVersionReady(const QString &version)
{
    ui->coreVerLabel->setText(version.isEmpty() ? "未找到" : version);
}

void Settings::onCliVersionReady(const QString &version)
{
    ui->label_3->setText(version.isEmpty() ? "未找到" : version);
}

void Settings::onWebVersionReady(const QString &version)
{
    ui->webVerBtn->setText(version.isEmpty() ? "未找到" : version);
}

void Settings::onVersionDetectionFinished()
{
    m_versionDetecting = false;
}

// =============================================================================
// UI 事件处理
// =============================================================================

void Settings::onRedetectButtonClicked()
{
    ui->coreVerLabel->setText("检测中...");
    ui->label_3->setText("检测中...");

    // 重置检测状态并启动新检测
    m_versionDetecting = false;
    startVersionDetection();
}

void Settings::onRedetectWebButtonClicked()
{
    ui->webVerBtn->setText("检测中...");
    // 重置检测状态并启动新检测
    m_versionDetecting = false;
    startVersionDetection();
}

void Settings::onUseLocalApiChanged(int state)
{
    // 根据是否使用本地 API 来启用/禁用 API 地址输入框
    ui->apiAddrEdit->setEnabled(state != Qt::Checked);
}

void Settings::onOpenFolderButtonClicked()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString etcoreDir = appDir + "/etcore";

    // 查找可执行文件目录
    if (!QDir(etcoreDir).exists()) {
        etcoreDir = appDir + "/../EasyTier";
        if (!QDir(etcoreDir).exists()) {
            etcoreDir = appDir;
        }
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(etcoreDir));
}

void Settings::onCheckUpdateButtonClicked()
{
    detectSoftwareVersion(true);
}

void Settings::onDialogAccepted()
{
    // 验证端口输入
    bool ok1, ok2;
    int configPort = ui->confSendEdit->text().toInt(&ok1);
    int webPagePort = ui->webPageEdit->text().toInt(&ok2);
    
    if (!ok1 || configPort < 1 || configPort >= 65535) {
        QMessageBox::warning(this, "输入错误", "配置下发端口必须在 1-65535 之间");
        return;
    }
    if (!ok2 || webPagePort < 1 || webPagePort >= 65535) {
        QMessageBox::warning(this, "输入错误", "控制台前端端口必须在 1-65535 之间");
        return;
    }

    // 保存界面设置
    m_autoRun = ui->autoRuncheckBox->isChecked();
    m_isHideOnTray = ui->hideOnTrayBox->isChecked();
    m_autoUpdate = ui->autoUpdateCheckBox->isChecked();

    // 处理开机自启设置变更
    if (m_autoStart != ui->autoStartCheckBox->isChecked()) {
        m_autoStart = ui->autoStartCheckBox->isChecked();
        setAutoStart(m_autoStart);
    }

    // 保存 Web 控制台配置
    m_webConfig.configPort = configPort;
    m_webConfig.webPagePort = webPagePort;
    m_webConfig.useLocalApi = ui->isUseLocalApiBox->isChecked();
    m_webConfig.apiAddress = ui->apiAddrEdit->text();
    m_webConfig.coreConnectAddress = ui->coreConnectAddrEdit->text();
    
    // 保存协议设置
    switch (ui->configProtocolBox->currentIndex()) {
        case 1: m_webConfig.configProtocol = "TCP"; break;
        case 2: m_webConfig.configProtocol = "WebSocket"; break;
        default: m_webConfig.configProtocol = "udp"; break;
    }

    // 保存日志保存天数
    int logDaysIndex = ui->logRetentionDaysBox->currentIndex();
    switch (logDaysIndex) {
        case 0: m_logRetentionDays = 0; break;
        case 1: m_logRetentionDays = 3; break;
        case 2: m_logRetentionDays = 7; break;
        case 3: m_logRetentionDays = 14; break;
        case 4: m_logRetentionDays = 30; break;
        default: m_logRetentionDays = 7; break;
    }

    saveSettings();
    
    accept();
}

void Settings::onDialogRejected()
{
    reject();
}

// =============================================================================
// 软件版本检测（网络请求）
// =============================================================================

void Settings::detectSoftwareVersion(bool isFromInternal)
{
    const QUrl url("https://gitee.com/api/v5/repos/viagrahuang/qt-easy-tier/releases/latest");

    // 取消之前的请求
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    // 发送 GET 请求
    m_currentReply = m_networkManager->get(QNetworkRequest(url));

    // 设置超时（5 秒）
    QTimer::singleShot(5000, this, [this]() {
        if (m_currentReply && m_currentReply->isRunning()) {
            m_currentReply->abort();
        }
    });

    // 连接请求完成信号，传递 isFromInternal 参数
    connect(m_currentReply, &QNetworkReply::finished, this, [this, isFromInternal]() {
        onNetworkReplyFinished(isFromInternal);
    });
}

void Settings::onNetworkReplyFinished(bool isFromInternal)
{
    if (!m_currentReply) {
        return;
    }

    QNetworkReply *reply = m_currentReply;
    m_currentReply = nullptr; // 清空指针，防止重复处理

    // 使用智能指针确保资源清理
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyGuard(reply);

    if (reply->error() == QNetworkReply::NoError) {
        // 解析 JSON 响应
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();

        QString latestVersion = jsonObj["tag_name"].toString();

        // 对比版本号
        if (!latestVersion.isEmpty() && latestVersion != m_softwareVer) {
            QString msg = QString("发现新版本：%1\n当前版本：%2\n是否前往下载？")
                          .arg(latestVersion).arg(m_softwareVer);

            QMessageBox::StandardButton ret = QMessageBox::question(
                this, "检查更新", msg, QMessageBox::Yes | QMessageBox::No);

            if (ret == QMessageBox::Yes) {
                QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier/releases"));
            }
        } else if (isFromInternal) {
            // 只有内部调用时才显示"已是最新"提示
            QMessageBox::information(this, "检查更新", "当前已经是最新版本！");
        }
    } else if (reply->error() != QNetworkReply::OperationCanceledError) {
        // 非取消操作才显示错误（且只在内部调用时显示）
        if (isFromInternal) {
            QMessageBox::warning(this, "检查更新", "检查更新失败：" + reply->errorString());
        }
    }

    emit finishDetectUpdate();
}

// =============================================================================
// 设置读写
// =============================================================================

void Settings::loadSettings()
{
    QDir().mkpath(m_configPath);
    QString settingsFile = m_configPath + "/settings.json";

    QFile file(settingsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        // 使用默认设置
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        // 使用默认设置
        return;
    }

    QJsonObject settings = doc.object();
    m_autoRun = settings.value("autoRun").toBool(false);
    m_autoStart = settings.value("autoStart").toBool(false);
    m_isHideOnTray = settings.value("isHideOnTray").toBool(true);
    m_autoUpdate = settings.value("autoUpdate").toBool(true);
    m_launchCount = settings.value("launchCount").toInt(0);

    // 加载 Web 控制台配置
    QJsonObject webConfig = settings.value("webConsole").toObject();
    m_webConfig.configPort = webConfig.value("configPort").toInt(55668);
    m_webConfig.webPagePort = webConfig.value("webPagePort").toInt(55667);
    m_webConfig.useLocalApi = webConfig.value("useLocalApi").toBool(true);
    m_webConfig.apiAddress = webConfig.value("apiAddress").toString();
    m_webConfig.coreConnectAddress = webConfig.value("coreConnectAddress").toString();
    m_webConfig.configProtocol = webConfig.value("configProtocol").toString("udp");

    // 加载日志保存天数
    m_logRetentionDays = settings.value("logRetentionDays").toInt(7);
}

void Settings::saveSettings()
{
    QString settingsFile = m_configPath + "/settings.json";

    QJsonObject settings;
    settings["autoRun"] = m_autoRun;
    settings["autoStart"] = m_autoStart;
    settings["isHideOnTray"] = m_isHideOnTray;
    settings["autoUpdate"] = m_autoUpdate;
    settings["launchCount"] = m_launchCount;

    // 保存 Web 控制台配置
    QJsonObject webConfig;
    webConfig["configPort"] = m_webConfig.configPort;
    webConfig["webPagePort"] = m_webConfig.webPagePort;
    webConfig["useLocalApi"] = m_webConfig.useLocalApi;
    webConfig["apiAddress"] = m_webConfig.apiAddress;
    webConfig["coreConnectAddress"] = m_webConfig.coreConnectAddress;
    webConfig["configProtocol"] = m_webConfig.configProtocol;
    settings["webConsole"] = webConfig;

    // 保存日志保存天数
    settings["logRetentionDays"] = m_logRetentionDays;

    QJsonDocument doc(settings);

    QFile file(settingsFile);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "错误", "无法保存设置");
        return;
    }

    file.write(doc.toJson());
    file.close();
}

// =============================================================================
// 辅助功能
// =============================================================================

void Settings::incrementLaunchCount()
{
    // 启动次数计数（达到阈值后停止计数）
    if (m_launchCount < DONATE_THRESHOLD) {
        m_launchCount++;
        saveSettings();
    }

    // 达到阈值时设置赞助弹窗标志
    if (m_launchCount == DONATE_THRESHOLD) {
        m_shouldShowDonate = true;
    }
}

void Settings::setAutoStart(bool enable)
{
#ifdef WIN32
    const QString appName = "QtEasyTier";
    const QString appPath = QCoreApplication::applicationFilePath().replace("/", "\\");
    QProcess process;

    QStringList args;
    if (enable) {
#if SAVE_CONF_IN_APP_DIR == true
        int ret = QMessageBox::information(this, tr("提示"), tr("开机自启会写入计划任务，若是便携使用可能污染系统环境，是否继续？"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            m_autoStart = false;
            return;
        }
#endif
        // 创建开机自启任务
        args << "/create"
             << "/tn" << appName
             << "/tr" << QString("\"%1\" --auto-start").arg(appPath)
             << "/sc" << "onlogon"
             << "/delay" << "0000:02"
             << "/rl" << "highest"
             << "/f";
    } else {
        // 删除开机自启任务
        args << "/delete"
             << "/tn" << appName
             << "/f";
    }

    process.start("schtasks.exe", args);

    if (!process.waitForFinished(5000)) {
        QMessageBox::warning(this, "错误",
            QString("%1自启任务失败：命令执行超时").arg(enable ? "创建" : "删除"));
        return;
    }

    if (process.exitCode() != 0) {
        QString error = QString::fromLocal8Bit(process.readAllStandardError());
        QMessageBox::warning(this, "错误",
            QString("%1自启任务失败：%2").arg(enable ? "创建" : "删除", error));
    }
#else
    Q_UNUSED(enable)
#endif
}

// =============================================================================
// 端口输入验证
// =============================================================================

void Settings::setupPortValidation()
{
    // 设置端口输入框的验证器（只允许输入数字）
    QIntValidator *portValidator = new QIntValidator(1, 65535, this);
    ui->confSendEdit->setValidator(portValidator);
    ui->webPageEdit->setValidator(portValidator);
    
    // 设置输入掩码提示
    ui->confSendEdit->setPlaceholderText("1-65535");
    ui->webPageEdit->setPlaceholderText("1-65535");
}

bool Settings::validatePortInput(const QString &text, int minPort, int maxPort)
{
    bool ok;
    int port = text.toInt(&ok);
    return ok && port >= minPort && port <= maxPort;
}

// =============================================================================
// 日志管理
// =============================================================================

QString Settings::getLogDirPath()
{
    return QCoreApplication::applicationDirPath() + "/log";
}

int Settings::cleanupOldLogs(int retentionDays)
{
    QString logDirPath = getLogDirPath();
    QDir logDir(logDirPath);
    
    if (!logDir.exists()) {
        return 0;
    }
    
    // 如果 retentionDays 为 0，清理所有日志
    if (retentionDays <= 0) {
        return clearAllLogs();
    }
    
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-retentionDays);
    int deletedCount = 0;
    
    // 日志文件名格式：yyyyMMdd_HHmmss_Base32编码的网络名.log
    QStringList logFiles = logDir.entryList(QStringList() << "*.log", QDir::Files);
    
    for (const QString &fileName : logFiles) {
        // 提取文件名中的时间戳部分（前15个字符：yyyyMMdd_HHmmss）
        if (fileName.length() < 15) {
            continue;
        }
        
        QString timestampStr = fileName.left(15);
        QDateTime fileDateTime = QDateTime::fromString(timestampStr, "yyyyMMdd_HHmmss");
        
        if (!fileDateTime.isValid()) {
            continue;
        }
        
        // 如果文件时间早于截止日期，删除
        if (fileDateTime < cutoffDate) {
            QString filePath = logDir.filePath(fileName);
            if (QFile::remove(filePath)) {
                deletedCount++;
            }
        }
    }
    
    return deletedCount;
}

int Settings::clearAllLogs()
{
    QString logDirPath = getLogDirPath();
    QDir logDir(logDirPath);
    
    if (!logDir.exists()) {
        return 0;
    }
    
    int deletedCount = 0;
    QStringList logFiles = logDir.entryList(QStringList() << "*.log", QDir::Files);
    
    for (const QString &fileName : logFiles) {
        QString filePath = logDir.filePath(fileName);
        if (QFile::remove(filePath)) {
            deletedCount++;
        }
    }
    
    return deletedCount;
}

void Settings::onOpenLogDirClicked()
{
    QString logDirPath = getLogDirPath();
    QDir logDir(logDirPath);
    
    // 如果目录不存在，创建它
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDirPath));
}

void Settings::onClearLogClicked()
{
    QMessageBox::StandardButton ret = QMessageBox::question(
        this, tr("确认清空"),
        tr("确定要清空所有日志文件吗？\n此操作不可恢复。"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        int deletedCount = clearAllLogs();
        QMessageBox::information(this, tr("完成"), 
            tr("已清空 %1 个日志文件").arg(deletedCount));
    }
}
