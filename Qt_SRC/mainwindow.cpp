#include "mainwindow.h"
#include "netpage.h"
#include "setting.h"
#include "oneclick.h"
#include "generateconf.h"
#include "donate.h"
#include "./ui_mainwindow.h"

#include <QListWidget>
#include <QDesktopServices>
#include <QVBoxLayout>
#include <QLayout>
#include <QUrl>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <iostream>
#include <QDir>           // 添加目录操作支持
#include <QFile>          // 添加文件操作支持
#include <QJsonDocument>  // 添加JSON文档支持
#include <QJsonObject>    // 添加JSON对象支持
#include <QJsonArray>     // 添加JSON数组支持
#include <QStandardPaths> // 添加标准路径支持

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->webDashboardBtn->setToolTip(tr("Web控制台需要占用55667(API)、55668(配置下发)端口\n"
        "启动后，会自动打开网页，请使用名为admin的账户登录，默认密码是admin\n"
        "然后就可以新建一个网络配置，勾选使用Web控制台管理后启动即可"
    ));

    // 设置窗口图标
    setWindowIcon(QIcon(":/icons/icon.ico"));

    // 当点击千万别点按钮时候, 使用浏览器打开《恭喜发财》（新春特别版）
    connect(ui->dontClick, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://www.bilibili.com/video/BV1ad4y1V7wb/"));
    });

    connect(ui->gitPushButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier"));
    });

    // 当点击首页按钮时，把主页换成首页
    connect(ui->homeButton, &QPushButton::clicked, this, [=, this]() {
        _changeWidget(ui->homePage);
    });

    // 点击关于按钮时，打开关于对话框
    connect(ui->aboutPushButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://qtet.070219.xyz"));
    });

    // 点击帮助按钮时，打开URL
    connect(ui->helpPushButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://qtet.070219.xyz/docs-home"));
    });

    // 点击设置按钮时打开设置窗口
    connect(ui->setPushButton, &QPushButton::clicked, this, &MainWindow::onClickSettingBtn);

    // 点击赞助按钮时打开赞助窗口
    connect(ui->donatePushButton, &QPushButton::clicked, this, [=, this]() {
        Donate *donateDialog = new Donate(this);
        donateDialog->exec();
        donateDialog->deleteLater();
    });

    // 点击etPushButton时，打开et官网
    connect(ui->etPushButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://easytier.cn"));
    });

    // 当点击添加按钮时，添加一个network页面并添加到列表中
    connect(ui->addPushButton, &QPushButton::clicked, this, [=, this]() {
        // 创建一个新的NetPage实例
        NetPage *newNetPage = new NetPage(this);
        // 将新的NetPage实例添加到列表中
        m_netpages.append(newNetPage);
        QListWidgetItem *item = new QListWidgetItem(ui->netListWidget);
        item->setText("Network " + QString::number(static_cast<int>(m_netpages.size()))); // 默认名称设置
        ui->netListWidget->addItem(item);
        item->setSelected(true);
        item->setIcon(QIcon(":/icons/network.svg"));

        // 连接网络启动与关闭信号设置列表字体颜色
        connect(newNetPage, &NetPage::networkStarted, this, [=]() {
            item->setIcon(QIcon(":/icons/network-running.svg"));
        });

        connect(newNetPage, &NetPage::networkFinished, this, [=]() {
            item->setIcon(QIcon(":/icons/network.svg")); // 设置图标颜色为默认值
        });


        // 调用__changeWidget函数将新的NetPage实例显示在主窗口右侧
        _changeWidget(newNetPage);
    });

    // 当列表项被点击时，切换到对应的network页面
    connect(ui->netListWidget, &QListWidget::itemClicked, this, [=, this](const QListWidgetItem *item) {
        int index = ui->netListWidget->row(item);
        if (index >= 0 && index < static_cast<int>(m_netpages.size())) {
            _changeWidget(m_netpages[index]);
        } else {
            std::cerr << "无效的索引，程序出错" << std::endl;
            std::exit(1);
        }
    });

    // 点击一键联机按钮时
    connect(ui->oneClickBtn, &QPushButton::clicked, this, &MainWindow::onClickOneClickBtn);

    // 当点击Web控制台按钮时
    connect(ui->webDashboardBtn, &QPushButton::clicked, this, &MainWindow::onClickWebDashboardBtn);

    // 设置右键菜单和双击事件
    setupContextMenu();

    // 启动时自动加载配置
    loadConfig();

    // 创建系统托盘
    createTrayIcon();

    // 清理过期日志文件
    {
        // 先加载设置获取日志保存天数
        QDir().mkpath(m_configPath);
        QString settingsFile = m_configPath + "/settings.json";
        int logRetentionDays = 7;  // 默认7天
        
        QFile file(settingsFile);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isObject()) {
                logRetentionDays = doc.object().value("logRetentionDays").toInt(7);
            }
        }
        
        int deletedCount = Settings::cleanupOldLogs(logRetentionDays);
        if (deletedCount > 0) {
            std::clog << "已清理 " << deletedCount << " 个过期日志文件" << std::endl;
        }
    }

    std::clog << "配置文件路径: " << m_configPath.toStdString() << std::endl;
}

MainWindow::~MainWindow()
{
    // 关闭应用前保存配置
    saveNetworkConfig();

    // 清理过期日志文件
    {
        // 从设置文件读取日志保存天数
        QDir().mkpath(m_configPath);
        QString settingsFile = m_configPath + "/settings.json";
        int logRetentionDays = 7;  // 默认7天
        
        QFile file(settingsFile);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isObject()) {
                logRetentionDays = doc.object().value("logRetentionDays").toInt(7);
            }
        }
        
        int deletedCount = Settings::cleanupOldLogs(logRetentionDays);
        if (deletedCount > 0) {
            std::clog << "已清理 " << deletedCount << " 个过期日志文件" << std::endl;
        }
    }

    if (m_webDashboardProcess) {
        m_webDashboardProcess->disconnect();
        m_webDashboardProcess->kill();
        if (m_webDashboardProcess->waitForFinished(1000)) {
            std::clog << "成功终止Web进程" << std::endl;
        } else {
            std::cerr << "可能无法终止Web进程" << std::endl;
        }
        m_webDashboardProcess->deleteLater();
        m_webDashboardProcess = nullptr;
    }

    // 杀死所有easytier-core进程
    QProcess process;
#ifdef Q_OS_WIN
    process.startDetached("taskkill /F /IM easytier-core.exe");
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    process.startDetached("pkill easytier-core");
#endif
    process.waitForFinished(5000);

    delete ui;
}


// ==========================主界面控件相关===========================

void MainWindow::setupContextMenu()
{
    // 启用右键菜单
    ui->netListWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // 连接右键菜单信号
    connect(ui->netListWidget, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QListWidgetItem *item = ui->netListWidget->itemAt(pos);
        if (item) {
            QMenu menu(this);
            QAction *renameAction = menu.addAction("更改名称");
            QAction *deleteAction = menu.addAction("删除网络");

            connect(renameAction, &QAction::triggered, this, &MainWindow::onRenameNetwork);
            connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteNetwork);

            menu.exec(ui->netListWidget->mapToGlobal(pos));
        }
    });

    // 连接双击事件
    connect(ui->netListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onNetListItemDoubleClicked);
}

void MainWindow::onNetListItemDoubleClicked(QListWidgetItem *item)
{
    onRenameNetwork();
}

void MainWindow::onRenameNetwork()
{
    QListWidgetItem *currentItem = ui->netListWidget->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "警告", "请先选择一个网络项");
        return;
    }

    bool ok;
    QString newName = QInputDialog::getText(this, "更改网络名称",
                                           "请输入新的网络名称:",
                                           QLineEdit::Normal,
                                           currentItem->text(),
                                           &ok);
    if (ok && !newName.isEmpty()) {
        currentItem->setText(newName);
    }
}

void MainWindow::onDeleteNetwork()
{
    QListWidgetItem *currentItem = ui->netListWidget->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "警告", "请先选择一个网络项");
        return;
    }

    const int &index = ui->netListWidget->row(currentItem);
    if (index < 0 || index >= static_cast<int>(m_netpages.size())) {
        QMessageBox::critical(this, "错误", "无效的网络项索引");
        std::exit(1);
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                 "确定要删除网络 '" + currentItem->text() + "' 吗？",
                                 QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        _changeWidget(ui->homePage);
        // 删除对应的NetPage实例
        m_netpages[index]->deleteLater();
        m_netpages.remove(index);

        // 删除列表项
        delete ui->netListWidget->takeItem(index);

    }
}

void MainWindow::onClickOneClickBtn() {
    if (!m_oneClick) {
        m_oneClick = new OneClick(this);
    }
    _changeWidget(m_oneClick);
}

void MainWindow::onClickWebDashboardBtn() {
    if (m_webDashboardProcess) {
        m_webDashboardProcess->disconnect();
        m_webDashboardProcess->kill();

        // 等待强制终止完成
        if (m_webDashboardProcess->waitForFinished(1000)) {
            std::clog << "成功终止Web进程" << std::endl;
        } else {
            QMessageBox::warning(this, "警告", "终止Web进程可能失败");
        }

        m_webDashboardProcess->deleteLater();
        m_webDashboardProcess = nullptr;
        ui->webDashboardBtn->setText("启动 Web 控制台");
        ui->webDashboardBtn->setStyleSheet("");

        return;
    }
    // 启动web控制台
    const QString &appDir = QCoreApplication::applicationDirPath()+"/etcore";
    const QString &appFile = appDir + "/easytier-web-embed.exe";

    // 检查appFile是否存在
    if (!QFile::exists(appFile)) {
        QMessageBox::critical(this, "错误", "找不到easytier-web-embed.exe");
        return;
    }

    // 检测端口占用情况
    if (isPortOccupied(m_webConfig.webPagePort) || isPortOccupied(m_webConfig.configPort))
    {
        QMessageBox::critical(this, "错误", 
            QString("%1或%2端口被占用").arg(m_webConfig.webPagePort).arg(m_webConfig.configPort));
        return;
    }

    // 构建启动命令
    QStringList args;
    args << "--api-server-port" << QString::number(m_webConfig.webPagePort);
    args << "--api-host" << m_webConfig.getActualApiAddress();
    args << "--config-server-port" << QString::number(m_webConfig.configPort);
    args << "--config-server-protocol" << m_webConfig.getConfigProtocolString();

    std::clog << "启动Web控制台命令：" << appFile.toStdString() << " " << args.join(" ").toStdString() << std::endl;

    m_webDashboardProcess = new QProcess(this);
    m_webDashboardProcess->start(appFile, args);
    ui->webDashboardBtn->setText("Web 控制台已启动");
    ui->webDashboardBtn->setStyleSheet("color: #66ccff; gridline-color: #66ccff");

    // 等待一秒打开浏览器
    QTimer::singleShot(1000, [=, this]() {
        if (m_webDashboardProcess && m_webDashboardProcess->state() == QProcess::Running) {
            QDesktopServices::openUrl(QUrl(QString("http://127.0.0.1:%1").arg(m_webConfig.webPagePort)));
        } else {
            QMessageBox::warning(this, "警告", "Web控制台可能启动失败");
        }

    });
}

/// @brief 页面替换函数
/// @param newWidget 新的Widget指针
///
/// @note 该函数用于替换主窗口右侧的页面为首页或者network页面
/// @warning 该函数的设计初衷是为了主窗口右侧的界面，并没有考虑其他使用情况，不要乱用
void MainWindow::_changeWidget(QWidget *newWidget) {
    // 获取centralWidget的box布局
    QBoxLayout *boxLayout = qobject_cast<QBoxLayout*>(ui->centralwidget->layout());
    if (!boxLayout) {
        std::clog << "centralWidget的布局不是QBoxLayout" << std::endl;
        return;
    }

    // 移除旧的Widget
    if (boxLayout->count() > 0) {
        QWidget *oldWidget = boxLayout->itemAt(1)->widget();
        if (oldWidget == newWidget) {
            std::clog << "界面没有任何更改" << std::endl;
            return;
        }
        if (oldWidget) {
            boxLayout->removeWidget(oldWidget);
            oldWidget->hide();

            // 如果未运行则删除OneClick实例
            if (oldWidget == m_oneClick) {
                if (!m_oneClick->isRunning()) {
                    m_oneClick->deleteLater();
                    m_oneClick = nullptr;
                }
            }
        }
    }

    // 添加新的Widget
    boxLayout->insertWidget(1, newWidget);
    newWidget->show();
}

void MainWindow::onClickSettingBtn() {
    Settings *settings = new Settings(this);
    
    // 连接 Web 配置更新信号，使配置立即生效
    connect(settings, &Settings::webConfigUpdated, this, [this](const WebConsoleConfig &config) {
        m_webConfig = config;
    });
    
    settings->exec();
    m_isHideOnTray = settings->isHideOnTray();
    settings->deleteLater();
    settings = nullptr;
}


// ======================== 配置文件加载相关 ===========================


// 加载网络配置
void MainWindow::loadConfig() {
    // 创建一个setting对象用于加载配置
    auto *tempSettings = new Settings();

    QWidget loadingMessage ;
    loadingMessage.setWindowTitle("QtEasyTier");
    loadingMessage.resize(300, 100);
    loadingMessage.setWindowIcon(QIcon(":/icons/icon.ico"));
    QHBoxLayout layout(&loadingMessage);
    QLabel loadingLabel("正在加载网络配置, 请稍后...", &loadingMessage);
    loadingLabel.setStyleSheet("font-size: 14px;");
    loadingLabel.setAlignment(Qt::AlignCenter);
    layout.addWidget(&loadingLabel);
    loadingMessage.show();

    // 配置目录
    QDir configDir;
    if (!configDir.exists(m_configPath)) {
        configDir.mkpath(m_configPath);
    }

    // 配置文件路径
    QString configFile = m_configPath + "/network.json";

    // 检查配置文件是否存在
    QFile file(configFile);
    if (!file.exists()) {
        // 如果配置文件不存在，创建一个默认的配置文件
        QJsonObject defaultConfig;
        defaultConfig["version"] = "1.0";
        defaultConfig["networks"] = QJsonArray();

        QJsonDocument doc(defaultConfig);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
        }
        tempSettings->deleteLater();
        return;
    }

    // 读取配置文件
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开配置文件进行读取:" << configFile;
        tempSettings->deleteLater();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        QMessageBox::critical(this, "错误", "JSON解析错误:" + error.errorString());
        tempSettings->deleteLater();
        return;
    }

    if (!doc.isObject()) {
        qDebug() << "配置文件格式错误，应为JSON对象";
        QMessageBox::critical(this, "错误", "配置文件格式错误，应为JSON对象");
        tempSettings->deleteLater();
        return;
    }

    QJsonObject rootObj = doc.object();

    if (!rootObj.contains("networks") || !rootObj["networks"].isArray()) {
        qDebug() << "配置文件缺少networks数组";
        QMessageBox::critical(this, "错误", "配置文件缺少networks数组");
        tempSettings->deleteLater();
        return;
    }

    QJsonArray networks = rootObj["networks"].toArray();

    // 加载网络配置
    for (int i = 0; i < static_cast<int>(networks.size()); ++i) {
        QJsonValue networkValue = networks[i];
        if (!networkValue.isObject()) continue;

        QJsonObject networkObj = networkValue.toObject();

        // 创建新页面
        NetPage *netPage = new NetPage(this);
        netPage->hide();
        m_netpages.append(netPage);
        QListWidgetItem *item = new QListWidgetItem(ui->netListWidget);
        QString name = networkObj.contains("name") ? networkObj["name"].toString() : ("Network " + QString::number(i + 1));
        item->setText(name);
        item->setIcon(QIcon(":/icons/network.svg"));

        // 连接网络启动与关闭信号设置列表字体颜色
        connect(netPage, &NetPage::networkStarted, this, [=]() {
            item->setIcon(QIcon(":/icons/network-running.svg"));
        });
        connect(netPage, &NetPage::networkFinished, this, [=]() {
            item->setIcon(QIcon(":/icons/network.svg")); // 设置图标颜色为默认值
        });
        ui->netListWidget->addItem(item);

        // 应用配置
        netPage->setNetworkConfig(networkObj);

        // 如果网络是活跃的，尝试重新启动它
        if (networkObj.contains("isActive") && networkObj["isActive"].toBool()) {
            // 从settings文件加载autoRun设置
            if (tempSettings->isAutoRun()) {
                netPage->runNetworkOnAutoStart();
            }
        }
    }

    // 是否隐藏到系统托盘
    m_isHideOnTray = tempSettings->isHideOnTray();
    
    // 加载 Web 控制台配置
    m_webConfig = tempSettings->getWebConsoleConfig();

    if (tempSettings->shouldShowDonate()) {
        Donate *donateWindow = new Donate(this);
        donateWindow->exec();
        donateWindow->deleteLater();
    }

    // 自启检查更新
    if (tempSettings->isAutoUpdate())
    {
        tempSettings->detectSoftwareVersion();
    }
    
    // 检测完成后删除临时 setting 对象
    connect(tempSettings, &Settings::finishDetectUpdate, this, [tempSettings]() {
        if (tempSettings) tempSettings->deleteLater();
    });
}

// 保存网络配置
void MainWindow::saveNetworkConfig() {
    // 创建配置目录
    QDir configDir;
    if (!configDir.exists(m_configPath)) {
        configDir.mkpath(m_configPath);
    }

    // 构建配置文件路径
    QString configFile = m_configPath + "/network.json";

    QJsonObject rootObj;
    rootObj["version"] = "1.0";

    QJsonArray networks;
    for (int i = 0; i < static_cast<int>(m_netpages.size()); ++i) {
        QJsonObject networkConfig = m_netpages[i]->getNetworkConfig();

        // 添加网络名称
        QString networkName = "Network " + QString::number(i + 1);
        if (i < ui->netListWidget->count()) {
            QListWidgetItem *item = ui->netListWidget->item(i);
            if (item) {
                networkName = item->text();
            }
        }
        networkConfig["name"] = networkName;

        networks.append(networkConfig);
    }

    rootObj["networks"] = networks;

    QJsonDocument doc(rootObj);

    QFile file(configFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法打开配置文件进行写入:" << configFile;
        return;
    }

    file.write(doc.toJson());
    file.close();
}

// ============================系统托盘相关=============================

// 重写关闭事件，使窗口隐藏到系统托盘而不是退出
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isHideOnTray) {
        // 隐藏主窗口而不是关闭
        hide();
        event->ignore();  // 忽略关闭事件，防止程序退出

        // 显示提示信息（可选）
        if (trayIcon) {
            trayIcon->showMessage("QtEasyTier", "程序已隐藏到系统托盘", QSystemTrayIcon::Information, 2000);
        }
    } else {
        QMainWindow::closeEvent(event);
    }

}

// 创建系统托盘
void MainWindow::createTrayIcon()
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        // 创建系统托盘图标
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icons/icon.ico"));  // 使用与主窗口相同的图标
        trayIcon->setToolTip("QtEasyTier");

        // 创建托盘菜单
        trayMenu = new QMenu(this);
        showAction = new QAction("显示主界面", this);
        exitAction = new QAction("退出程序", this);

        connect(showAction, &QAction::triggered, this, &MainWindow::onShowWindow);
        connect(exitAction, &QAction::triggered, this, &MainWindow::onExitApp);

        trayMenu->addAction(showAction);
        trayMenu->addSeparator();  // 添加分割线
        trayMenu->addAction(exitAction);

        trayIcon->setContextMenu(trayMenu);

        // 连接托盘图标激活事件
        connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

        // 显示系统托盘图标
        trayIcon->show();
    }
}

// 托盘图标激活事件
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        // 单击或双击托盘图标显示主窗口
        onShowWindow();
    }
}

// 显示主窗口
void MainWindow::onShowWindow()
{
    show();
    raise();
    activateWindow();
}

// 退出应用程序
void MainWindow::onExitApp()
{
    // 先销毁系统托盘，避免再次弹出隐藏消息
    delete trayIcon;
    trayIcon = nullptr;
    // 然后退出程序
    QApplication::quit();
}