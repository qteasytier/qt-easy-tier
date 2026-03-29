#include "mainwindow.h"
#include "netpage.h"
#include "setting.h"
#include "oneclick.h"
#include "generateconf.h"
#include "donate.h"
#include "webdashboardworker.h"
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
#include <QStandardPaths> // 添加标准路径支持1

MainWindow::MainWindow(QWidget *parent, const bool isAutoStart)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->webDashboardBtn->setToolTip(tr("Web控制台默认占用55667(网页/API)、55668(配置下发)端口，可在设置界面更改\n"
        "启动后，会自动打开网页，登录即可开始使用，admin用户默认密码为admin\n"
    ));

    // 设置窗口图标标题
    setWindowTitle("QtEasyTier Ver" + QString(PROJECT_VERSION));
    setWindowIcon(QIcon(":/icons/icon.ico"));

    // 当点击千万别点按钮时候, 使用浏览器打开《恭喜发财》（新春特别版）
    connect(ui->dontClick, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://www.bilibili.com/video/BV1UT42167xb"));
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
        connect(newNetPage, &NetPage::networkStarted, this, [=, this]() {
            item->setIcon(QIcon(":/icons/network-running.svg"));
            saveNetworkConfig();
        });

        connect(newNetPage, &NetPage::networkFinished, this, [=, this]() {
            item->setIcon(QIcon(":/icons/network.svg")); // 设置图标颜色为默认值
            saveNetworkConfig();
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
            QMessageBox::critical(this, tr("错误"), tr("无效的索引，程序出错"));
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

    int deletedCount = Settings::cleanupOldLogs();
    if (deletedCount > 0) {
        std::clog << "已清理 " << deletedCount << " 个过期日志文件" << std::endl;
    }

    if (isAutoStart)
    {
        close();
    }
}

MainWindow::~MainWindow()
{
    // 关闭应用前保存配置
    saveNetworkConfig();

    int deletedCount = Settings::cleanupOldLogs();
    if (deletedCount > 0) {
        std::clog << "已清理 " << deletedCount << " 个过期日志文件" << std::endl;
    }

    // 清理 Web 控制台 Worker
    if (m_webWorker) {
        if (!m_webWorker->disconnect())
        {
            std::cerr << "Can not disconnect WebDashboardWorker" << std::endl;
        }
        m_webWorker->stopProcess();
        QApplication::processEvents();
        delete m_webWorker;
        m_webWorker = nullptr;
    }
 
#ifdef Q_OS_WIN
    QProcess::startDetached("taskkill /F /IM easytier-core.exe");
    QProcess::startDetached("taskkill /F /IM easytier-cli.exe");
    QProcess::startDetached("taskkill /F /IM easytier-web-embed.exe");
#elif defined(Q_OS_LINUX)
    QProcess::startDetached("pkill easytier-core");
    QProcess::startDetached("pkill easytier-cli");
    QProcess::startDetached("pkill easytier-web-embed");
#endif
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
    const QString newName = QInputDialog::getText(this, "更改网络名称",
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

    // 如果 Worker 已存在且正在运行，则停止

    if (m_webWorker && m_webWorker->isRunning()) {
        ui->webDashboardBtn->setStyleSheet("color: yellow; gridline-color: yellow; font-weight: bold;");
        ui->webDashboardBtn->setText(tr("结束Web控制台进程中..."));
        ui->webDashboardBtn->setEnabled(false);
        QApplication::processEvents();  // 先刷新 UI
        m_webWorker->stopProcess();
        return;
    }

    // 启动web控制台
    const QString &appDir = QCoreApplication::applicationDirPath()+"/etcore";
    
    // 根据平台设置可执行文件路径
#ifdef Q_OS_WIN
    const QString &appFile = appDir + "/easytier-web-embed.exe";
#else
    const QString &appFile = appDir + "/easytier-web-embed";
#endif

    // 检查appFile是否存在
    if (!QFile::exists(appFile)) {
        QMessageBox::critical(this, tr("错误"), tr("找不到easytier-web-embed"));
        return;
    }
    const Settings::WebConsoleConfig webConfig = Settings::getWebConsoleConfig();

    if (isPortOccupied(webConfig.webPagePort) || isPortOccupied(webConfig.configPort)) {
        QMessageBox::critical(this, tr("错误"),
            tr("%1或%2端口被占用").arg(webConfig.webPagePort).arg(webConfig.configPort));
        return;
    }

    // 构建启动命令
    QStringList args;
    args << "--api-server-port" << QString::number(webConfig.webPagePort);
    args << "--api-host" << webConfig.getActualApiAddress();
    args << "--config-server-port" << QString::number(webConfig.configPort);
    args << "--config-server-protocol" << webConfig.getConfigProtocolString();

    // 创建 Worker 对象（直接在主线程，QProcess 本身是异步的）
    if (!m_webWorker) {
        m_webWorker = new WebDashboardWorker(this);
        // 连接 Worker 信号
        connect(m_webWorker, &WebDashboardWorker::processStarted,
                this, &MainWindow::onWebProcessStarted);
        connect(m_webWorker, &WebDashboardWorker::processStopped,
                this, &MainWindow::onWebProcessStopped);
        connect(m_webWorker, &WebDashboardWorker::processCrashed,
                this, &MainWindow::onWebProcessCrashed);
    }

    // 更新 UI 状态
    ui->webDashboardBtn->setText(tr("启动Web控制台中..."));
    ui->webDashboardBtn->setEnabled(false);
    QApplication::processEvents();  // 先刷新 UI

    // 保存配置用于后续打开浏览器
    m_webWorker->setProperty("webPagePort", webConfig.webPagePort);

    // 直接调用启动进程（QProcess 是异步的，不会阻塞）
    m_webWorker->startProcess(appFile, args);
}

void MainWindow::onWebProcessStarted(bool success, const QString &message) {
    if (success) {
        ui->webDashboardBtn->setText(tr("Web 控制台已启动"));
        ui->webDashboardBtn->setStyleSheet("color: #66ccff; gridline-color: #66ccff; font-weight: bold;");
        ui->webDashboardBtn->setEnabled(true);

        // 获取保存的端口号并打开浏览器
        const int webPagePort = m_webWorker ? m_webWorker->property("webPagePort").toInt() : 55667;
        QDesktopServices::openUrl(QUrl(QString("http://127.0.0.1:%1").arg(webPagePort)));
    } else {
        QMessageBox::warning(this, tr("警告"), message);
        ui->webDashboardBtn->setStyleSheet("");
        ui->webDashboardBtn->setText(tr("启动 Web 控制台"));
        ui->webDashboardBtn->setEnabled(true);
    }
}

void MainWindow::onWebProcessStopped(bool success) {
    if (!success) {
        QMessageBox::warning(this, tr("警告"), tr("Web控制台进程结束超时"));
    }
    m_webWorker->deleteLater();
    m_webWorker = nullptr;
    
    ui->webDashboardBtn->setStyleSheet("");
    ui->webDashboardBtn->setText(tr("启动 Web 控制台"));
    ui->webDashboardBtn->setEnabled(true);
}

void MainWindow::onWebProcessCrashed(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    
    if (exitStatus == QProcess::CrashExit) {
        QMessageBox::critical(this, tr("错误"), tr("Web控制台进程发生崩溃"));
    } else {
        QMessageBox::warning(this, tr("警告"), tr("Web控制台进程正常结束"));
    }

    m_webWorker->deleteLater();
    m_webWorker = nullptr;

    ui->webDashboardBtn->setStyleSheet("");
    ui->webDashboardBtn->setText(tr("启动 Web 控制台"));
    ui->webDashboardBtn->setEnabled(true);
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
    
    settings->exec();
    m_isHideOnTray = Settings::isHideOnTray();
    settings->deleteLater();
}


// ======================== 配置文件加载相关 ===========================


// 加载QtEasyTier配置
void MainWindow::loadConfig() {
    std::clog << "Config file save path: " << Settings::getConfigPath().toStdString() << std::endl;

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
    const QDir configDir;
    if (!configDir.exists(Settings::getConfigPath())) {
        configDir.mkpath(Settings::getConfigPath());
    }

    // 配置文件路径
    const QString &configFile = Settings::getConfigPath() + "/network.json";

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
        return;
    }

    // 读取配置文件
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开配置文件进行读取:" << configFile;
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        QMessageBox::critical(this, "错误", "JSON解析错误:" + error.errorString());
        return;
    }

    if (!doc.isObject()) {
        qDebug() << "配置文件格式错误，应为JSON对象";
        QMessageBox::critical(this, "错误", "配置文件格式错误，应为JSON对象");
        return;
    }

    QJsonObject rootObj = doc.object();

    if (!rootObj.contains("networks") || !rootObj["networks"].isArray()) {
        qDebug() << "配置文件缺少networks数组";
        QMessageBox::critical(this, "错误", "配置文件缺少networks数组");
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
        connect(netPage, &NetPage::networkStarted, this, [=, this]() {
            item->setIcon(QIcon(":/icons/network-running.svg"));
            saveNetworkConfig();
        });
        connect(netPage, &NetPage::networkFinished, this, [=, this]() {
            item->setIcon(QIcon(":/icons/network.svg")); // 设置图标颜色为默认值
            saveNetworkConfig();
        });
        ui->netListWidget->addItem(item);

        // 应用配置
        netPage->setNetworkConfig(networkObj);

        // 如果网络是活跃的，尝试重新启动它
        if (networkObj.contains("isActive") && networkObj["isActive"].toBool()) {
            // 从settings文件加载autoRun设置
            if (Settings::isAutoRun()) {
                netPage->runNetworkOnAutoStart();
            }
        }
    }

    // 是否隐藏到系统托盘
    m_isHideOnTray = Settings::isHideOnTray();

    // 是否打开赞助窗口
    if (Settings::shouldShowDonate()) {
        Donate donate(this);
        donate.exec();
    }

    // 是否自动检查更新
    if (Settings::isAutoCheckUpdate())
    {
        Settings::detectSoftwareVersion(this, true);
    }

    // 是否为自启动
    if (Settings::isAutoStart())
    {
        // 设置为自启动时，尝试写入一次计划任务（冲掉旧的防止路径变化）
        Settings::setAutoStart(true, this);
    }
}

// 保存网络配置
void MainWindow::saveNetworkConfig() {
    // 创建配置目录
    const QDir configDir;
    if (!configDir.exists(Settings::getConfigPath())) {
        configDir.mkpath(Settings::getConfigPath());
    }

    // 构建配置文件路径
    const QString &configFile = Settings::getConfigPath() + "/network.json";

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

// ============================系统托盘与主界面显示/关闭相关=============================

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
