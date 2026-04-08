//
// Created by YMHuang on 2026/4/7.
//

#include "qtetsettings.h"
#include "qtetcheckbtn.h"
#include "qtetpushbtn.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QPixmap>
#include <QFontDatabase>
#include <QScrollArea>
#include <iostream>


QtETSettings::QtETSettings(QWidget *parent)
    : QWidget(parent)
{
    // 加载设置
    loadSettings();
    
    // 初始化界面
    initUI();
    
    // 连接信号槽
    setupConnections();
}

// ==================== 初始化相关 ====================

void QtETSettings::initUI()
{
    // 设置字体大小
    QFont font;
    font.setPointSize(10);
    setFont(font);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ========== 标题栏区域 ==========
    auto *titleWidget = new QWidget(this);
    auto *titleLayout = new QHBoxLayout(titleWidget);
    titleLayout->setSpacing(15);
    titleLayout->setContentsMargins(16, 15, 16, 6);

    // 左侧图标
    auto *iconLabel = new QLabel(titleWidget);
    iconLabel->setMaximumSize(48, 48);
    iconLabel->setMinimumSize(48, 48);
    iconLabel->setPixmap(QPixmap(":/icons/icon.ico"));
    iconLabel->setScaledContents(true);

    // 标题文字
    auto *titleLabel = new QLabel(titleWidget);
    QFont titleFont;
    titleFont.setPointSize(20);
    titleLabel->setFont(titleFont);
    titleLabel->setText(tr("设置"));
    titleLabel->setAlignment(Qt::AlignCenter);


    titleLayout->addWidget(iconLabel, 0, Qt::AlignTop);
    titleLayout->addWidget(titleLabel, 0, Qt::AlignHCenter | Qt::AlignVCenter);

    mainLayout->addWidget(titleWidget, 0, Qt::AlignTop | Qt::AlignLeft);

    // ========== 滚动区域：设置选项 ==========
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 设置选项内容容器
    auto *settingsWidget = new QWidget();
    auto *settingsLayout = new QVBoxLayout(settingsWidget);
    settingsLayout->setSpacing(10);
    settingsLayout->setContentsMargins(20, 10, 20, 10);

    // 创建开关按钮
    m_hideOnTrayCheckBox = new QtETCheckBtn(tr("关闭时隐藏到托盘"), this);
    m_autoReconnectCheckBox = new QtETCheckBtn(tr("自动回连"), this);
    m_autoCheckUpdateCheckBox = new QtETCheckBtn(tr("自动检查更新"), this);
    m_autoStartCheckBox = new QtETCheckBtn(tr("开机自启"), this);

    // 设置字体
    QFont checkBoxFont;
    checkBoxFont.setPointSize(11);
    m_hideOnTrayCheckBox->setFont(checkBoxFont);
    m_autoReconnectCheckBox->setFont(checkBoxFont);
    m_autoCheckUpdateCheckBox->setFont(checkBoxFont);
    m_autoStartCheckBox->setFont(checkBoxFont);

    // 设置简要提示
    m_hideOnTrayCheckBox->setBriefTip(tr("关闭窗口时最小化到系统托盘而不是退出程序"));
    m_autoReconnectCheckBox->setBriefTip(tr("程序启动时自动连接上次退出时正在运行的网络"));
    m_autoCheckUpdateCheckBox->setBriefTip(tr("程序启动时自动检查是否有新版本"));
    m_autoStartCheckBox->setBriefTip(tr("系统开机时自动启动程序"));

    // 设置初始状态
    m_hideOnTrayCheckBox->setChecked(m_hideOnTray);
    m_autoReconnectCheckBox->setChecked(m_autoReconnect);
    m_autoCheckUpdateCheckBox->setChecked(m_autoCheckUpdate);
    m_autoStartCheckBox->setChecked(m_autoStart);

    // 版本信息区域
    auto *versionWidget = new QWidget(settingsWidget);
    auto *versionLayout = new QHBoxLayout(versionWidget);
    versionLayout->setSpacing(10);
    versionLayout->setContentsMargins(0, 0, 0, 0);

    // 构建版本字符串
    QString versionStr = tr("QtEasyTier 版本：") + QString::fromUtf8(PROJECT_VERSION);
#if IS_BETA_VERSION
    versionStr += " beta";
#endif
    versionStr += QString(" (EasyTier %1)").arg(ET_VERSION);

    m_versionLabel = new QLabel(versionStr, this);
    QFont versionFont;
    versionFont.setPointSize(10);
    m_versionLabel->setFont(versionFont);
    m_versionLabel->setAlignment(Qt::AlignLeft);

    m_checkUpdateBtn = new QtETPushBtn(tr("检查更新"), this);
    m_checkUpdateBtn->setFixedWidth(120);

    versionLayout->addWidget(m_versionLabel);
    versionLayout->addStretch();
    versionLayout->addWidget(m_checkUpdateBtn);

    // 添加到布局（垂直布局，一行一个）
    settingsLayout->addWidget(m_hideOnTrayCheckBox);
    settingsLayout->addWidget(m_autoReconnectCheckBox);
    settingsLayout->addWidget(m_autoCheckUpdateCheckBox);
    settingsLayout->addWidget(m_autoStartCheckBox);
    settingsLayout->addWidget(versionWidget);
    settingsLayout->addStretch();  // 弹簧让内容向上对齐

    // 将 settingsWidget 放入 scrollArea
    scrollArea->setWidget(settingsWidget);

    mainLayout->addWidget(scrollArea, 1);  // stretch=1 让滚动区域占据剩余空间

    // ========== 底部区域：操作按钮 ==========
    auto *bottomWidget = new QWidget(this);
    auto *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setSpacing(10);
    bottomLayout->setContentsMargins(20, 10, 20, 10);

    // 操作按钮区域
    auto *buttonWidget = new QWidget(bottomWidget);
    auto *buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    buttonLayout->addStretch();

    m_discardBtn = new QtETPushBtn(tr("丢弃"), this);
    m_discardBtn->setFixedWidth(80);
    m_discardBtn->setEnabled(false);

    m_saveBtn = new QtETPushBtn(tr("保存"), this);
    m_saveBtn->setFixedWidth(80);
    m_saveBtn->setEnabled(false);

    buttonLayout->addWidget(m_discardBtn);
    buttonLayout->addWidget(m_saveBtn);

    bottomLayout->addWidget(buttonWidget);

    mainLayout->addWidget(bottomWidget);
}

void QtETSettings::setupConnections()
{
    // 复选框状态变化
    connect(m_hideOnTrayCheckBox, &QCheckBox::checkStateChanged, this, &QtETSettings::onCheckBoxStateChanged);
    connect(m_autoReconnectCheckBox, &QCheckBox::checkStateChanged, this, &QtETSettings::onCheckBoxStateChanged);
    connect(m_autoCheckUpdateCheckBox, &QCheckBox::checkStateChanged, this, &QtETSettings::onCheckBoxStateChanged);
    connect(m_autoStartCheckBox, &QCheckBox::checkStateChanged, this, &QtETSettings::onCheckBoxStateChanged);

    // 按钮点击
    connect(m_saveBtn, &QPushButton::clicked, this, &QtETSettings::onSaveButtonClicked);
    connect(m_discardBtn, &QPushButton::clicked, this, &QtETSettings::onDiscardButtonClicked);
    connect(m_checkUpdateBtn, &QPushButton::clicked, this, &QtETSettings::onCheckUpdateButtonClicked);
}

// ==================== UI 事件处理 ====================

void QtETSettings::onCheckBoxStateChanged()
{
    updateButtonState();
}

void QtETSettings::onSaveButtonClicked()
{
    // 处理开机自启设置变更
    bool newAutoStart = m_autoStartCheckBox->isChecked();
    if (m_lastSavedAutoStart != newAutoStart) {
        if (!setAutoStart(newAutoStart)) {
            // 设置失败，恢复复选框状态
            m_autoStartCheckBox->setChecked(m_lastSavedAutoStart);
            return;
        }
    }

    // 保存设置
    saveSettings();

    // 更新上次保存的值
    m_lastSavedHideOnTray = m_hideOnTray;
    m_lastSavedAutoReconnect = m_autoReconnect;
    m_lastSavedAutoCheckUpdate = m_autoCheckUpdate;
    m_lastSavedAutoStart = m_autoStart;

    // 更新按钮状态
    updateButtonState();

    // 发送设置更改信号
    emit settingsChanged();

    // 如果隐藏到托盘设置改变了，发送专门信号
    if (m_hideOnTray != m_lastSavedHideOnTray) {
        emit hideOnTrayChanged(m_hideOnTray);
    }

    // 显示保存成功提示
    QMessageBox::information(this, tr("保存成功"), tr("设置已保存"));
}

void QtETSettings::onDiscardButtonClicked()
{
    // 恢复到上次保存的设置
    discardChanges();
}

void QtETSettings::onCheckUpdateButtonClicked()
{
    checkForUpdate(this, false);
}

void QtETSettings::updateButtonState()
{
    bool hasChanges = hasUnsavedChanges();
    m_saveBtn->setEnabled(hasChanges);
    m_discardBtn->setEnabled(hasChanges);
}

// ==================== 设置读写 ====================

QString QtETSettings::getConfigPath()
{
#if SAVE_CONF_IN_APP_DIR == TRUE
    return QCoreApplication::applicationDirPath() + "/config";
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#endif
}

QJsonObject QtETSettings::loadSettingsFromFile()
{
    QString configPath = getConfigPath();
    QDir dir(configPath);

    // 确保目录存在
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString settingsFile = configPath + "/settings2.json";

    QFile file(settingsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return QJsonObject();
    }

    return doc.object();
}

bool QtETSettings::saveSettingsToFile(const QJsonObject &settings)
{
    QString configPath = getConfigPath();
    QDir dir(configPath);

    // 确保目录存在
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return false;
        }
    }

    QString settingsFile = configPath + "/settings2.json";
    QJsonDocument doc(settings);

    QFile file(settingsFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

void QtETSettings::loadSettings()
{
    QJsonObject settings = loadSettingsFromFile();

    m_hideOnTray = settings.value("hide_on_tray").toBool(true);
    m_autoReconnect = settings.value("auto_reconnect").toBool(false);
    m_autoCheckUpdate = settings.value("auto_check_update").toBool(true);
    m_autoStart = settings.value("auto_start").toBool(false);

    // 更新上次保存的值
    m_lastSavedHideOnTray = m_hideOnTray;
    m_lastSavedAutoReconnect = m_autoReconnect;
    m_lastSavedAutoCheckUpdate = m_autoCheckUpdate;
    m_lastSavedAutoStart = m_autoStart;
}

void QtETSettings::saveSettings()
{
    // 从 UI 读取当前值
    m_hideOnTray = m_hideOnTrayCheckBox->isChecked();
    m_autoReconnect = m_autoReconnectCheckBox->isChecked();
    m_autoCheckUpdate = m_autoCheckUpdateCheckBox->isChecked();
    m_autoStart = m_autoStartCheckBox->isChecked();

    // 保存到文件
    QJsonObject settings = loadSettingsFromFile();
    settings["hide_on_tray"] = m_hideOnTray;
    settings["auto_reconnect"] = m_autoReconnect;
    settings["auto_check_update"] = m_autoCheckUpdate;
    settings["auto_start"] = m_autoStart;

    if (!saveSettingsToFile(settings)) {
        QMessageBox::warning(this, tr("错误"), tr("无法保存设置"));
    }
}

void QtETSettings::discardChanges()
{
    // 恢复复选框状态
    m_hideOnTrayCheckBox->setChecked(m_lastSavedHideOnTray);
    m_autoReconnectCheckBox->setChecked(m_lastSavedAutoReconnect);
    m_autoCheckUpdateCheckBox->setChecked(m_lastSavedAutoCheckUpdate);
    m_autoStartCheckBox->setChecked(m_lastSavedAutoStart);

    // 恢复内部值
    m_hideOnTray = m_lastSavedHideOnTray;
    m_autoReconnect = m_lastSavedAutoReconnect;
    m_autoCheckUpdate = m_lastSavedAutoCheckUpdate;
    m_autoStart = m_lastSavedAutoStart;

    // 更新按钮状态
    updateButtonState();
}

bool QtETSettings::hasUnsavedChanges() const
{
    return m_hideOnTrayCheckBox->isChecked() != m_lastSavedHideOnTray ||
           m_autoReconnectCheckBox->isChecked() != m_lastSavedAutoReconnect ||
           m_autoCheckUpdateCheckBox->isChecked() != m_lastSavedAutoCheckUpdate ||
           m_autoStartCheckBox->isChecked() != m_lastSavedAutoStart;
}

// ==================== 静态方法 ====================

bool QtETSettings::isHideOnTray()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("hide_on_tray").toBool(true);
}

bool QtETSettings::isAutoReconnect()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("auto_reconnect").toBool(false);
}

bool QtETSettings::isAutoStart()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("auto_start").toBool(false);
}

bool QtETSettings::isAutoCheckUpdate()
{
    const QJsonObject settings = loadSettingsFromFile();
    return settings.value("auto_check_update").toBool(true);
}

bool QtETSettings::setAutoStart(bool enable)
{
#ifdef Q_OS_WIN
    // Windows 平台：使用任务计划程序实现开机自启
    const QString appName = "QtEasyTier";
    const QString appPath = QCoreApplication::applicationFilePath().replace("/", "\\");
    QProcess process;

    QStringList args;
    if (enable) {
        // 创建开机自启任务
        args << "/create"
             << "/tn" << appName
             << "/tr" << QString("\"%1\" --auto-start").arg(appPath)
             << "/sc" << "onlogon"
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
        std::clog << QString("开机自启任务失败：命令执行超时").toStdString() << std::endl;
        return false;
    }

    if (process.exitCode() != 0) {
        QString error = QString::fromLocal8Bit(process.readAllStandardError());
        std::clog << QString("开机自启任务失败：%1").arg(error).toStdString() << std::endl;
        return false;
    }

    std::clog << QString("%1自启任务成功").arg(enable ? "创建" : "删除").toStdString() << std::endl;

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
        if (!desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        desktopFile.write(desktopContent.toUtf8());
        desktopFile.close();
        std::clog << "[AutoStart] Created autostart entry: " << desktopFilePath.toStdString() << std::endl;
    } else {
        // 删除 .desktop 文件
        if (QFile::exists(desktopFilePath)) {
            if (!QFile::remove(desktopFilePath)) {
                return false;
            }
        }
        std::clog << "[AutoStart] Removed autostart entry: " << desktopFilePath.toStdString() << std::endl;
    }

#else
    // 其他平台：暂不支持
    Q_UNUSED(enable)
    return false;
#endif

    return true;
}

void QtETSettings::checkForUpdate(QWidget *parent, bool silent)
{
    const QUrl url("https://gitee.com/api/v5/repos/viagrahuang/qt-easy-tier/releases/latest");

    // 创建网络管理器
    auto *networkManager = new QNetworkAccessManager(parent);
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    reply->setParent(networkManager);

    // 设置超时定时器（10 秒）
    auto *timeoutTimer = new QTimer(networkManager);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->start(10000);

    // 超时处理：中止请求
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort);

    // 请求完成处理
    QObject::connect(reply, &QNetworkReply::finished, parent, [=]() {
        // 停止并删除超时定时器
        if (timeoutTimer) {
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
        }

        // 检查请求结果
        if (reply->error() == QNetworkReply::NoError) {
            // 解析 JSON 响应
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();

            QString latestVersion = jsonObj["tag_name"].toString();

            // 对比版本号
            if (!latestVersion.isEmpty() && latestVersion > QString::fromUtf8(PROJECT_VERSION)) {
                QString msg = QString(tr("发现新版本：%1\n当前版本：%2\n是否前往下载？"))
                              .arg(latestVersion).arg(QString::fromUtf8(PROJECT_VERSION));

                QMessageBox::StandardButton ret = QMessageBox::question(
                    parent, tr("检查更新"), msg, QMessageBox::Yes | QMessageBox::No);

                if (ret == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier/releases"));
                }
            } else if (!silent) {
                QMessageBox::information(parent, tr("检查更新"), tr("当前已是最新版本！"));
            }
        } else if (reply->error() != QNetworkReply::OperationCanceledError) {
            // 非超时错误才显示提示
            QMessageBox::warning(parent, tr("检查更新"),
                QString(tr("网络请求失败：%1")).arg(reply->errorString()));
        } else {
            // 超时提示
            QMessageBox::warning(parent, tr("检查更新"), tr("网络请求超时，请稍后重试！"));
        }

        // 删除网络管理器
        networkManager->deleteLater();
    });
}

