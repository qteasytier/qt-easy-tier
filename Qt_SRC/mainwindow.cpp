#include "mainwindow.h"
#include "netpage.h"
#include "./ui_mainwindow.h"

#include <QListWidget>
#include <QListWidgetItem>
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

    // 设置窗口图标
    setWindowIcon(QIcon(":/icons/icon.ico"));

    // 当点击千万别点按钮时候, 使用浏览器打开《诈骗小曲》
    connect(ui->dontClick, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://www.bilibili.com/video/BV1UT42167xb/"));
    });

    connect(ui->gitPushButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier"));
    });

    // 当点击首页按钮时，把主页换成首页
    connect(ui->homeButton, &QPushButton::clicked, this, [=, this]() {
        __changeWidget(ui->homePage);
    });
    // 当点击添加按钮时，添加一个network页面并添加到列表中
    connect(ui->addPushButton, &QPushButton::clicked, this, [=, this]() {
        // 创建一个新的NetPage实例
        NetPage *newNetPage = new NetPage();
        // 将新的NetPage实例添加到列表中
        m_netpages.append(newNetPage);
        QListWidgetItem *item = new QListWidgetItem(ui->netListWidget);
        item->setText("Network " + QString::number(m_netpages.size())); // 默认网络名称设置
        ui->netListWidget->addItem(item);
        item->setSelected(true);
        // 调用__changeWidget函数将新的NetPage实例显示在主窗口右侧
        __changeWidget(newNetPage);
    });

    // 当列表项被点击时，切换到对应的network页面
    connect(ui->netListWidget, &QListWidget::itemClicked, this, [=, this](QListWidgetItem *item) {
        int index = ui->netListWidget->row(item);
        if (index >= 0 && index < m_netpages.size()) {
            __changeWidget(m_netpages[index]);
        } else {
            std::cerr << "无效的索引，程序出错" << std::endl;
            std::exit(1);
        }
    });

    // 设置右键菜单和双击事件
    setupContextMenu();

    // 启动时自动加载配置
    loadNetworkConfig();

    // 创建系统托盘
    createTrayIcon();
}

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

    int index = ui->netListWidget->row(currentItem);
    if (index < 0 || index >= m_netpages.size()) {
        QMessageBox::critical(this, "错误", "无效的网络项索引");
        std::exit(1);
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                 "确定要删除网络 '" + currentItem->text() + "' 吗？",
                                 QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        __changeWidget(ui->homePage);
        // 删除对应的NetPage实例
        delete m_netpages[index];
        m_netpages.remove(index);

        // 删除列表项
        delete ui->netListWidget->takeItem(index);

    }
}

MainWindow::~MainWindow()
{
    // 关闭应用前保存配置
    saveNetworkConfig();

    ui->netListWidget->clear();
    delete ui;
}

/// @brief 页面替换函数
/// @param newWidget 新的Widget指针
///
/// @note 该函数用于替换主窗口右侧的页面为首页或者network页面
/// @warning 该函数的设计初衷是为了主窗口右侧的界面，并没有考虑其他使用情况，不要乱用
void MainWindow::__changeWidget(QWidget *newWidget) const {
    // 获取centralWidget的box布局
    QBoxLayout *boxLayout = qobject_cast<QBoxLayout*>(ui->centralwidget->layout());
    if (!boxLayout) {
        std::clog << "centralWidget的布局不是QBoxLayout" << std::endl;
        return;
    }

    // 移除旧的Widget
    if (boxLayout->count() > 0) {
        QWidget *oldWidget = boxLayout->itemAt(1)->widget();
        if (oldWidget) {
            boxLayout->removeWidget(oldWidget);
            oldWidget->hide();
        }
    }

    // 添加新的Widget
    boxLayout->insertWidget(1, newWidget);
    newWidget->show();
}

// 加载网络配置
void MainWindow::loadNetworkConfig() {
    // 创建配置目录
    QDir configDir;
    QString configPath = QCoreApplication::applicationDirPath() + "/config";
    if (!configDir.exists(configPath)) {
        configDir.mkpath(configPath);
    }

    // 构建配置文件路径
    QString configFile = configPath + "/network.json";

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

    QByteArray data = file.readAll();
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
    for (int i = 0; i < networks.size(); ++i) {
        QJsonValue networkValue = networks[i];
        if (!networkValue.isObject()) continue;

        QJsonObject networkObj = networkValue.toObject();

        NetPage *netPage;

        // 创建新页面
        netPage = new NetPage();
        m_netpages.append(netPage);
        QListWidgetItem *item = new QListWidgetItem(ui->netListWidget);
        QString name = networkObj.contains("name") ? networkObj["name"].toString() : ("Network " + QString::number(i + 1));
        item->setText(name);
        ui->netListWidget->addItem(item);

        // 应用配置
        netPage->setNetworkConfig(networkObj);

        // 如果网络是活跃的，尝试重新启动它（这需要额外的逻辑）
        if (networkObj.contains("isActive") && networkObj["isActive"].toBool()) {
            // 注意：这里不能立即启动网络，因为可能依赖的服务还没准备好
            // 可以设置标志位，在适当的时机再启动
        }
    }

    // 选中第一个网络
    if (ui->netListWidget->count() > 0) {
        ui->netListWidget->item(0)->setSelected(true);
        if (!m_netpages.isEmpty()) {
            __changeWidget(m_netpages[0]);
        }
    }
}

// 保存网络配置
void MainWindow::saveNetworkConfig() {
    // 创建配置目录
    QDir configDir;
    QString configPath = QCoreApplication::applicationDirPath() + "/config";
    if (!configDir.exists(configPath)) {
        configDir.mkpath(configPath);
    }

    // 构建配置文件路径
    QString configFile = configPath + "/network.json";

    QJsonObject rootObj;
    rootObj["version"] = "1.0";

    QJsonArray networks;
    for (int i = 0; i < m_netpages.size(); ++i) {
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

// 重写关闭事件，使窗口隐藏到系统托盘而不是退出
void MainWindow::closeEvent(QCloseEvent *event)
{
    // 隐藏主窗口而不是关闭
    hide();
    event->ignore();  // 忽略关闭事件，防止程序退出

    // 显示提示信息（可选）
    if (trayIcon) {
        trayIcon->showMessage("QtEasyTier", "程序已隐藏到系统托盘", QSystemTrayIcon::Information, 2000);
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