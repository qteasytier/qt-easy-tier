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

setting::setting(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::setting)
{
    ui->setupUi(this);

    // 加载设置
    loadSettings();

    // 设置界面初始状态
    ui->autoRuncheckBox->setChecked(m_autoRun);
    ui->autoStartCheckBox->setChecked(m_autoStart);

    // 定时器延时检测EasyTier版本
    QTimer::singleShot(500, this, &setting::detectEasyTierVersions);
}

setting::~setting()
{
    delete ui;
}

// 重新检测版本按钮点击事件
void setting::on_detAgainPushButton_clicked()
{
    ui->coreVerLabel->setText("检测中......");
    ui->label_3->setText("检测中......");

    QTimer::singleShot(100, this, &setting::detectEasyTierVersions);
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
    // 暂时不实现
    QMessageBox::information(this, "提示", "检查更新功能暂未实现");
}

// 确定按钮点击事件
void setting::on_buttonBox_accepted()
{
    // 保存设置
    m_autoRun = ui->autoRuncheckBox->isChecked();
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

// 检测EasyTier版本
void setting::detectEasyTierVersions()
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
    if (!coreVersion.isEmpty()) {
        ui->coreVerLabel->setText(coreVersion);
    } else {
        ui->coreVerLabel->setText("未找到");
    }

    // 检测cli版本
    QString cliVersion = getExecutableVersion(cliPath);
    if (!cliVersion.isEmpty()) {
        ui->label_3->setText(cliVersion);
    } else {
        ui->label_3->setText("未找到");
    }
}

// 获取可执行文件版本
QString setting::getExecutableVersion(const QString &executablePath)
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

// 加载设置
void setting::loadSettings()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    QString settingsFile = configPath + "/settings.json";

    QFile file(settingsFile);
    if (!file.exists()) {
        // 默认设置
        m_autoRun = false;
        m_autoStart = false;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        // 默认设置
        m_autoRun = false;
        m_autoStart = false;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        // 默认设置
        m_autoRun = false;
        m_autoStart = false;
        return;
    }

    QJsonObject settings = doc.object();
    m_autoRun = settings.value("autoRun").toBool(false);
    m_autoStart = settings.value("autoStart").toBool(false);
}

// 保存设置
void setting::saveSettings()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString settingsFile = configPath + "/settings.json";

    QJsonObject settings;
    settings["autoRun"] = m_autoRun;
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
             << "/tn" << appName                      // 任务名称
             << "/tr" << QString("\"%1\"").arg(appPath) + " --auto-start" // 要执行的程序路径
             << "/sc" << "onstart"                 // 触发条件：开机启动
             << "/delay" << "0000:05"                 // 延迟5s启动
             << "/rl" << "highest"                 // 运行级别：最高权限（管理员）
             << "/f";                                 // 覆盖已存在的任务

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
    if (!output.isEmpty() && process.exitCode() != 0) {
        std::clog << "schtasks命令执行结果：" << output.toStdString() << std::endl;
    }

#endif
}
