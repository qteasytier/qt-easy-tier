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
    
    // 根据平台设置可执行文件路径
#ifdef Q_OS_WIN
    QString corePath = execDir + "/easytier-core.exe";
    QString cliPath = execDir + "/easytier-cli.exe";
    QString webPath = execDir + "/easytier-web-embed.exe";
#else
    QString corePath = execDir + "/easytier-core";
    QString cliPath = execDir + "/easytier-cli";
    QString webPath = execDir + "/easytier-web-embed";
#endif

    // 先检测核心版本
    detectSingleExecutable(corePath, "core");
}

void VersionDetectionWorker::detectSingleExecutable(const QString &executablePath, const QString &type)
{
    m_currentDetectType = type;

    // 获取可执行文件目录
    QString execDir = m_executableDir.isEmpty() ? findExecutableDir() : m_executableDir;
    
    // 根据平台设置可执行文件后缀
#ifdef Q_OS_WIN
    QString exeSuffix = ".exe";
#else
    QString exeSuffix = "";
#endif

    // 检查文件是否存在
    if (!QFile::exists(executablePath)) {
        if (type == "core") {
            m_coreDetected = true;
            emit coreVersionReady(QString());
            // 继续检测 CLI
            detectSingleExecutable(execDir + "/easytier-cli" + exeSuffix, "cli");
        } else if (type == "cli") {
            m_cliDetected = true;
            emit cliVersionReady(QString());
            // 继续检测 Web
            detectSingleExecutable(execDir + "/easytier-web-embed" + exeSuffix, "web");
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
    
    // 根据平台设置可执行文件后缀
#ifdef Q_OS_WIN
    QString exeSuffix = ".exe";
#else
    QString exeSuffix = "";
#endif

    if (m_currentDetectType == "core") {
        m_coreDetected = true;
        emit coreVersionReady(version);
        // 继续检测 CLI
        detectSingleExecutable(execDir + "/easytier-cli" + exeSuffix, "cli");
    } else if (m_currentDetectType == "cli") {
        m_cliDetected = true;
        emit cliVersionReady(version);
        // 继续检测 Web
        detectSingleExecutable(execDir + "/easytier-web-embed" + exeSuffix, "web");
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

    // 加载设置
    loadSettings();

    // 设置界面初始状态
    setupUi();
    setupWebConfigUi();

    // 连接信号槽
    connectSignals();
    
    // 设置端口输入验证
    setupPortValidation();
    
    // 注意：启动次数计数已移到 Settings::shouldShowDonate() 静态方法中统一处理
}

Settings::~Settings()
{
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
    detectSoftwareVersion(this);
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

void Settings::detectSoftwareVersion(QWidget *parent, bool isNotMessageBox)
{
    const QUrl url("https://gitee.com/api/v5/repos/viagrahuang/qt-easy-tier/releases/latest");

    // 创建网络管理器，设置父对象以确保生命周期管理
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(parent);
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    reply->setParent(networkManager);

    // 设置超时定时器（5 秒）
    QTimer *timeoutTimer = new QTimer(networkManager);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->start(5000);

    // 超时处理：中止请求
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort);

    // 请求完成处理
    QObject::connect(reply, &QNetworkReply::finished, parent, [=]() {
        // 停止并删除超时定时器
        if (timeoutTimer) {
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
        }

        // 使用智能指针确保 reply 被正确释放
        QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyGuard(reply);

        // 检查请求结果
        if (reply->error() == QNetworkReply::NoError) {
            // 解析 JSON 响应
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();

            QString latestVersion = jsonObj["tag_name"].toString();

            // 对比版本号（使用 PROJECT_VERSION 宏）
            if (!latestVersion.isEmpty() && latestVersion != PROJECT_VERSION && !isNotMessageBox) {
                QString msg = QString("发现新版本：%1\n当前版本：%2\n是否前往下载？")
                              .arg(latestVersion).arg(PROJECT_VERSION);

                QMessageBox::StandardButton ret = QMessageBox::question(
                    parent, "检查更新", msg, QMessageBox::Yes | QMessageBox::No);

                if (ret == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier/releases"));
                }
            } else if (!isNotMessageBox) {
                QMessageBox::information(parent, "检查更新", "当前已是最新版本！");
            }
        } else if (reply->error() != QNetworkReply::OperationCanceledError) {
            // 非超时错误才显示提示
            QMessageBox::warning(parent, "检查更新", 
                QString("网络请求失败：%1").arg(reply->errorString()));
        } else {
            // 超时提示
            QMessageBox::warning(parent, "检查更新", "网络请求超时，请稍后重试！");
        }

        // 删除网络管理器（会同时删除 reply，因为 reply 的父对象是 networkManager）
        networkManager->deleteLater();
    });
}

// =============================================================================
// 设置读写
// =============================================================================

QJsonObject Settings::loadSettingsFromFile()
{
    QDir().mkpath(getConfigPath());
    QString settingsFile = getConfigPath() + "/settings.json";

    QFile file(settingsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject(); // 返回空对象
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return QJsonObject(); // 返回空对象
    }

    return doc.object();
}

bool Settings::saveSettingsToFile(const QJsonObject &settings)
{
    QDir().mkpath(getConfigPath());
    QString settingsFile = getConfigPath() + "/settings.json";

    QJsonDocument doc(settings);

    QFile file(settingsFile);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

void Settings::loadSettings()
{
    QJsonObject settings = loadSettingsFromFile();

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
    m_webConfig.configProtocol = webConfig.value("configProtocol").toString("udp");

    // 加载日志保存天数
    m_logRetentionDays = settings.value("logRetentionDays").toInt(7);
}

void Settings::saveSettings()
{
    QJsonObject settings = loadSettingsFromFile();

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
    webConfig["configProtocol"] = m_webConfig.configProtocol;
    settings["webConsole"] = webConfig;

    // 保存日志保存天数
    settings["logRetentionDays"] = m_logRetentionDays;

    if (!saveSettingsToFile(settings)) {
        QMessageBox::warning(this, "错误", "无法保存设置");
    }
}

// =============================================================================
// 辅助功能
// =============================================================================

bool Settings::isAutoRun()
{
    QJsonObject settings = loadSettingsFromFile();
    return settings.value("autoRun").toBool(false);
}

bool Settings::isHideOnTray()
{
    QJsonObject settings = loadSettingsFromFile();
    return settings.value("isHideOnTray").toBool(true);
}

bool Settings::isAutoCheckUpdate()
{
    QJsonObject settings = loadSettingsFromFile();
    return settings.value("autoUpdate").toBool(true);
}

bool Settings::shouldShowDonate()
{
    QJsonObject settings = loadSettingsFromFile();
    int launchCount = settings.value("launchCount").toInt(0);

    // 达到阈值时返回 true
    if (launchCount >= DONATE_THRESHOLD) {
        return true;
    }

    // 增加启动次数
    launchCount++;
    settings["launchCount"] = launchCount;
    saveSettingsToFile(settings);

    return false;
}

void Settings::setAutoStart(bool enable)
{
#ifdef Q_OS_WIN
    // Windows 平台：使用任务计划程序实现开机自启
    const QString appName = "QtEasyTier";
    const QString appPath = QCoreApplication::applicationFilePath();
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
             << "/tn" << appName+"-onlogon"
             << "/tr" << QString("\"%1\" --auto-start").arg(appPath)
             << "/sc" << "onlogon"
             << "/delay" << "0000:02"
             << "/rl" << "highest"
             << "/f";
    } else {
        // 删除开机自启任务
        args << "/delete"
             << "/tn" << appName+"-onlogon"
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

#elif defined(Q_OS_LINUX)
    // Linux 平台：使用 freedesktop.org autostart 规范
    const QString appName = "QtEasyTier";
    const QString appPath = QCoreApplication::applicationFilePath();
    
    // 获取 autostart 目录路径
    QString configHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (configHome.isEmpty()) {
        configHome = QDir::homePath() + "/.config";
    }
    QString autostartDir = configHome + "/autostart";
    QString desktopFilePath = autostartDir + "/" + appName + ".desktop";
    
    if (enable) {
        // 创建 autostart 目录（如果不存在）
        QDir().mkpath(autostartDir);
        
        // 创建 .desktop 文件
        QString desktopContent = QString(
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=%1\n"
            "Exec=%2 --auto-start\n"
            "Icon=%1\n"
            "Comment=QtEasyTier - EasyTier GUI Client\n"
            "Terminal=false\n"
            "Categories=Network;\n"
        ).arg(appName, appPath);
        
        QFile desktopFile(desktopFilePath);
        if (desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            desktopFile.write(desktopContent.toUtf8());
            desktopFile.close();
            std::clog << "[AutoStart] Created autostart entry: " << desktopFilePath.toStdString() << std::endl;
        } else {
            QMessageBox::warning(this, "错误", tr("无法创建自启动文件：%1").arg(desktopFilePath));
        }
    } else {
        // 删除 .desktop 文件
        if (QFile::exists(desktopFilePath)) {
            if (QFile::remove(desktopFilePath)) {
                std::clog << "[AutoStart] Removed autostart entry: " << desktopFilePath.toStdString() << std::endl;
            } else {
                QMessageBox::warning(this, "错误", tr("无法删除自启动文件：%1").arg(desktopFilePath));
            }
        }
    }

#else
    // 其他平台：暂不支持
    Q_UNUSED(enable)
    QMessageBox::information(this, tr("提示"), tr("当前平台暂不支持开机自启功能"));
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

int Settings::cleanupOldLogs()
{
    // 清理过期日志文件
    // 先加载设置获取日志保存天数
    QJsonObject settings = loadSettingsFromFile();
    int logRetentionDays = settings.value("logRetentionDays").toInt(7);

    const QString &logDirPath = getLogDirPath();
    const QDir logDir(logDirPath);
    
    if (!logDir.exists()) {
        return 0;
    }
    
    // 如果 retentionDays 为 0，清理所有日志
    if (logRetentionDays <= 0) {
        return clearAllLogs();
    }
    
    const QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-logRetentionDays);
    int deletedCount = 0;
    
    // 日志文件名格式：yyyyMMdd_HHmmss_Base32编码的网络名.log
    QStringList logFiles = logDir.entryList(QStringList() << "*.log", QDir::Files);
    
    for (const QString &fileName : logFiles)
    {
        // 提取文件名中的时间戳部分（前15个字符：yyyyMMdd_HHmmss）
        if (fileName.length() < 15) {
            continue;
        }
        
        QString timestampStr = fileName.left(15);
        QDateTime fileDateTime = QDateTime::fromString(timestampStr, "yyyyMMdd_HHmmss");
        
        // 如果文件名格式错误或时间早于截止日期，删除
        if (!fileDateTime.isValid() || fileDateTime < cutoffDate) {
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


/// @brief 获取 Web 控制台配置
Settings::WebConsoleConfig Settings::getWebConsoleConfig()
{
    WebConsoleConfig config;
    QJsonObject settings = loadSettingsFromFile();
    QJsonObject webConfig = settings.value("webConsole").toObject();

    config.configPort = webConfig.value("configPort").toInt(55668);
    config.webPagePort = webConfig.value("webPagePort").toInt(55667);
    config.useLocalApi = webConfig.value("useLocalApi").toBool(true);
    config.apiAddress = webConfig.value("apiAddress").toString();
    config.configProtocol = webConfig.value("configProtocol").toString("udp");

    return config;
}
