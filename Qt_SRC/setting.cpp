#include "setting.h"

#include <iostream>

#include "ui_setting.h"
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QTimer>
#include <QThread>
#include <QNetworkReply>

// VersionDetectionWorker实现
VersionDetectionWorker::VersionDetectionWorker(QObject *parent)
    : QObject(parent)
{
}

void VersionDetectionWorker::detectVersions()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString etcoreDir = appDir + "/etcore";

    // 如果目录不存在，尝试在当前目录下查找
    if (!QDir(etcoreDir).exists()) {
        etcoreDir = appDir + "/../EasyTier";
        if (!QDir(etcoreDir).exists()) {
            etcoreDir = appDir;
        }
    }

    QString corePath = etcoreDir + "/easytier-core.exe";
    QString cliPath = etcoreDir + "/easytier-cli.exe";

    // 检测core版本
    QString coreVersion = getExecutableVersion(corePath);
    emit coreVersionDetected(coreVersion);

    // 检测cli版本
    QString cliVersion = getExecutableVersion(cliPath);
    emit cliVersionDetected(cliVersion);

    emit detectionFinished();
}

QString VersionDetectionWorker::getExecutableVersion(const QString &executablePath)
{
    if (!QFile::exists(executablePath)) {
        return QString();
    }

    QProcess process;
    process.start(executablePath, QStringList() << "-V");
    process.waitForFinished(3000);

    if (process.exitCode() != 0) {
        return QString();
    }

    QString output = process.readAllStandardOutput();
    // 去掉"easytier"和空格，只保留版本号
    output = output.trimmed();
    if (output.startsWith("easytier-core", Qt::CaseInsensitive)) {
        output = output.mid(14).trimmed();
    } else if (output.startsWith("easytier-cli", Qt::CaseInsensitive)) {
        output = output.mid(13).trimmed();
    } else {
        output = "Error";
    }

    return output;
}


// setting类实现
setting::setting(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::setting)
    , m_versionThread(nullptr)
    , m_versionWorker(nullptr)
{
    ui->setupUi(this);

    // 加载设置
    loadSettings();

    // 设置界面初始状态
    ui->autoRuncheckBox->setChecked(g_autoRun);
    ui->autoStartCheckBox->setChecked(m_autoStart);
    ui->versionLabel->setText(m_softwareVer);

    // 创建版本检测线程
    m_versionThread = new QThread(this);
}

setting::~setting()
{
    // 第一步：立即断开所有信号槽连接，防止后续调用
    this->disconnect();

    // 断开与worker相关的所有连接
    if (m_versionWorker) {
        m_versionWorker->disconnect();
    }

    // 断开与线程相关的所有连接
    if (m_versionThread) {
        m_versionThread->disconnect();
    }

    // 第二步：停止并等待线程安全退出
    if (m_versionThread && m_versionThread->isRunning()) {
        // 请求线程退出
        m_versionThread->quit();

        // 等待线程退出，设置合理的超时时间
        if (!m_versionThread->wait(5000)) {  // 增加到5秒超时
            // 如果线程仍未退出，强制终止
            m_versionThread->terminate();
            m_versionThread->wait(2000);  // 增加等待时间
        }
    }

    // 第三步：按正确顺序销毁对象
    // 先销毁worker对象（同步销毁）
    if (m_versionWorker) {
        // 在销毁前确保worker不在任何线程中运行
        if (m_versionWorker->thread() != QThread::currentThread()) {
            m_versionWorker->moveToThread(QThread::currentThread());
        }
        delete m_versionWorker;
        m_versionWorker = nullptr;
    }

    // 再销毁线程对象（同步销毁）
    if (m_versionThread) {
        delete m_versionThread;
        m_versionThread = nullptr;
    }

    // 最后销毁UI
    delete ui;
}

// 启动版本检测线程
void setting::startVersionDetection()
{
    // 如果已有线程在运行，直接返回，避免重复启动
    if (m_versionThread && m_versionThread->isRunning()) {
        return;
    }

    // 清理之前的worker对象（如果存在）
    if (m_versionWorker) {
        // 确保worker在正确的线程中
        if (m_versionWorker->thread() != QThread::currentThread()) {
            m_versionWorker->moveToThread(QThread::currentThread());
        }
        delete m_versionWorker;
        m_versionWorker = nullptr;
    }

    // 重新创建线程（如果需要）
    if (!m_versionThread) {
        m_versionThread = new QThread(this);
    }

    // 创建新的工作对象
    m_versionWorker = new VersionDetectionWorker(nullptr);  // 不设置父对象

    // 将工作对象移动到线程
    m_versionWorker->moveToThread(m_versionThread);

    // 连接信号和槽
    connect(m_versionThread, &QThread::started, m_versionWorker, &VersionDetectionWorker::detectVersions, Qt::DirectConnection);
    connect(m_versionWorker, &VersionDetectionWorker::coreVersionDetected, this, &setting::onCoreVersionDetected, Qt::QueuedConnection);
    connect(m_versionWorker, &VersionDetectionWorker::cliVersionDetected, this, &setting::onCliVersionDetected, Qt::QueuedConnection);
    connect(m_versionWorker, &VersionDetectionWorker::detectionFinished, m_versionThread, &QThread::quit, Qt::DirectConnection);

    // 线程结束后清理worker对象
    connect(m_versionThread, &QThread::finished, this, [this]() {
        if (m_versionWorker) {
            m_versionWorker->deleteLater();
            m_versionWorker = nullptr;
        }
    }, Qt::DirectConnection);

    // 启动线程
    m_versionThread->start();
}

// 重新检测版本按钮点击事件
void setting::on_detAgainPushButton_clicked()
{
    ui->coreVerLabel->setText("检测中......");
    ui->label_3->setText("检测中......");

    startVersionDetection();
}

// 版本检测结果处理
void setting::onCoreVersionDetected(const QString &version)
{
    if (!version.isEmpty()) {
        ui->coreVerLabel->setText(version);
    } else {
        ui->coreVerLabel->setText("未找到");
    }
}

void setting::onCliVersionDetected(const QString &version)
{
    if (!version.isEmpty()) {
        ui->label_3->setText(version);
    } else {
        ui->label_3->setText("未找到");
    }
}

// 打开核心目录按钮点击事件
void setting::on_pushButton_2_clicked()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString etcoreDir = appDir + "/etcore";

    // 如果目录不存在，尝试在当前目录下查找
    if (!QDir(etcoreDir).exists()) {
        etcoreDir = appDir + "/../EasyTier";
        if (!QDir(etcoreDir).exists()) {
            etcoreDir = appDir;
        }
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(etcoreDir));
}

// 检查更新按钮点击事件
void setting::on_newVerPushButton_clicked()
{
    detectSoftwareVersion(true);
}

void setting::detectSoftwareVersion(const bool &isFromInternal)
{
    const QString &currentVersion = m_softwareVer;

    auto *manager = new QNetworkAccessManager(this);
    const QUrl url("https://gitee.com/api/v5/repos/viagrahuang/qt-easy-tier/releases/latest");

    // 发送 GET 请求
    QNetworkReply *reply = manager->get(QNetworkRequest(url));

    // 连接请求完成信号
    connect(reply, &QNetworkReply::finished, this, [=, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            // 读取返回的 JSON 数据
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();

            // 解析 tag_name 字段
            QString latestVersion = jsonObj["tag_name"].toString();

            // 对比版本号
            if (latestVersion != currentVersion) {
                QString msg = QString("发现新版本：%1\n当前版本：%2\n是否前往下载？")
                              .arg(latestVersion).arg(currentVersion);
                QMessageBox::StandardButton ret = QMessageBox::question(
                    this, "检查更新", msg,
                    QMessageBox::Yes | QMessageBox::No
                );
                if (ret == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier/releases"));
                }
            } else if (isFromInternal) {
                QMessageBox::information(this, "检查更新", "当前已经是最新版本！");
            }
        } else {
            // 网络请求失败
            QMessageBox::warning(this, "检查更新",
                "检查更新失败：" + reply->errorString());
        }

        reply->deleteLater();

        // 外部调用完毕后自动删除setting
        if (!isFromInternal) {
            this->deleteLater();
        } else {
            manager->deleteLater();
        }
    });
}

// 确定按钮点击事件
void setting::on_buttonBox_accepted()
{
    // 保存设置
    g_autoRun = ui->autoRuncheckBox->isChecked();
    m_autoStart = ui->autoStartCheckBox->isChecked();

    saveSettings();

    // 设置开机自启
    setAutoStart(m_autoStart);

    accept();
}

// 取消按钮点击事件
void setting::on_buttonBox_rejected()
{
    reject();
}

// 加载设置
void setting::loadSettings()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    QString settingsFile = configPath + "/settings.json";

    QFile file(settingsFile);
    if (!file.exists()) {
        // 默认设置
        g_autoRun = false;
        m_autoStart = false;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        // 默认设置
        g_autoRun = false;
        m_autoStart = false;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        // 默认设置
        g_autoRun = false;
        m_autoStart = false;
        return;
    }

    QJsonObject settings = doc.object();
    g_autoRun = settings.value("autoRun").toBool(false);
    m_autoStart = settings.value("autoStart").toBool(false);
}

// 保存设置
void setting::saveSettings()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString settingsFile = configPath + "/settings.json";

    QJsonObject settings;
    settings["autoRun"] = g_autoRun;
    settings["autoStart"] = m_autoStart;

    QJsonDocument doc(settings);

    QFile file(settingsFile);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "错误", "无法保存设置");
        return;
    }

    file.write(doc.toJson());
    file.close();
}

// 设置开机自启
void setting::setAutoStart(bool enable)
{
    // 仅在Windows系统下执行
#ifdef WIN32
    QString appName = "QtEasyTier";
    QString appPath = QCoreApplication::applicationFilePath().replace("/", "\\");
    QProcess process;

    if (enable) {
        QStringList args;
        args << "/create"
             << "/tn" << appName                    // 任务名称
             << "/tr" << QString("\"%1\"").arg(appPath) + " --auto-start" // 要执行的程序路径
             << "/sc" << "onlogon"               // 触发条件：开机启动
             << "/delay" << "0000:02"
             << "/rl" << "highest"               // 运行级别：最高权限（管理员）
             << "/f";                               // 覆盖已存在的任务

        std::clog << "schtasks.exe " << args.join(" ").toStdString() << std::endl;
        process.start("schtasks.exe", args);

        if (!process.waitForFinished(5000)) {
            std::cerr << "创建自启任务失败：命令执行超时" << std::endl;
            QMessageBox::warning(this, "错误", "创建自启任务失败：命令执行超时");
            return;
        }

        // 检查schtasks命令是否执行成功
        if (process.exitCode() != 0) {
            std::cerr << "创建自启任务失败：" << process.readAllStandardError().toStdString() << std::endl;
            QMessageBox::warning(this, "错误", "创建自启任务失败：" + QString(process.readAllStandardError()));
            return;
        }

    } else {
        // 删除任务计划：关闭自启
        QStringList args;
        args << "/delete"
             << "/tn" << appName                      // 任务名称
             << "/f";                                 // 强制删除

        process.start("schtasks.exe", args);

        if (!process.waitForFinished(1000)) {
            std::cerr << "删除自启任务失败：命令执行超时" << std::endl;
            QMessageBox::warning(this, "错误", "删除自启任务失败：命令执行超时");
            return;
        }

        // 检查schtasks命令是否执行成功
        if (process.exitCode() != 0) {
            std::cerr << "删除自启任务失败：" << process.readAllStandardError().toStdString() << std::endl;
            QMessageBox::warning(this, "错误", "删除自启任务失败：" + QString(process.readAllStandardError()));
            return;
        }
    }

    // 输出命令执行结果，方便排查问题
    const QString error = process.readAllStandardError();
    if (!error.isEmpty()) {
        std::cerr << "schtasks命令执行错误：" << error.toStdString() << std::endl;
        QMessageBox::warning(this, "Error", tr("设置计划任务失败，命令执行错误"));
    }
    const QString output = process.readAllStandardOutput();
    if (!output.isEmpty() && process.exitCode() == 0) {
        std::clog << "schtasks命令执行结果：" << output.toStdString() << std::endl;
    }

#endif
}
