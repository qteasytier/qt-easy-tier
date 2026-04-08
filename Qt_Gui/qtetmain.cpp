#include "ui_qtetmain.h"
#include "qtetmain.h"
#include "qtetcheckbtn.h"
#include "qtetpushbtn.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QFontDatabase>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>
#include <QDesktopServices>
#include <QMenu>
#include <QApplication>
#include <QMessageBox>

QtETMain::QtETMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::QtETMain)
{
    ui->setupUi(this);
    setMinimumSize(680, 460);

// =============== 初始化调色板 ===============
    // 设置一次调色板
    const Qt::ColorScheme &currentScheme = QGuiApplication::styleHints()->colorScheme();
    onSchemeChanged(currentScheme);
    // 监测系统调色板变化
    const QStyleHints *hints = QGuiApplication::styleHints();
    connect(hints, &QStyleHints::colorSchemeChanged, this, &QtETMain::onSchemeChanged);

// =============== 初始化界面 ===============
    initHelloPage();
    initNetworkPage();
    initOneClickPage();
    initServersPage();
    initSettingsPage();
    
// =============== 初始化系统托盘 ===============
    initTrayIcon();

// =============== 连接信号槽 ===============
    connect(ui->homeBtn, &QPushButton::clicked, this, [=, this]()
    {
        m_mainStackedWidget->setCurrentWidget(m_helloPage);
    });
    connect(ui->gitBtn, &QPushButton::clicked, this, [=, this]()
    {
        QDesktopServices::openUrl(QUrl("https://gitee.com/viagrahuang/qt-easy-tier"));
    });
    connect(ui->helpBtn, &QPushButton::clicked, this, [=, this]()
    {
        QDesktopServices::openUrl(QUrl("https://qtet.myqfeng.top/docs-home"));
    });
    connect(m_aboutUsBtn, &QPushButton::clicked, this, [=, this]()
    {
        QDesktopServices::openUrl(QUrl("https://qtet.myqfeng.top"));
    });
    connect(m_aboutETBtn, &QPushButton::clicked, this, [=, this]()
    {
        QDesktopServices::openUrl(QUrl("https://easytier.cn"));
    });
    connect(m_donateBtn, &QPushButton::clicked, this, [=, this]()
    {
        QDesktopServices::openUrl(QUrl("https://qtet.myqfeng.top/other/donate/"));
    });
    connect(m_notClickBtn, &QPushButton::clicked, this, [=, this]()
    {
        QDesktopServices::openUrl(QUrl("https://www.bilibili.com/festival/2026BH?bvid=BV1gAcNzSEf4"));
    });
    
    // 连接网络状态信号到托盘消息
    if (m_networkPage) {
        connect(m_networkPage, &QtETNetwork::networkStarted, this, &QtETMain::onNetworkStartedNotify);
        connect(m_networkPage, &QtETNetwork::networkStopped, this, &QtETMain::onNetworkStoppedNotify);
    }
}

QtETMain::~QtETMain()
{
    // 保存网络配置
    if (m_networkPage) {
        m_networkPage->saveAllNetworkConfs();
    }
    // 注意：设置页面的配置由用户手动保存，不自动保存
    delete ui;
}

/// @brief 重写关闭事件，实现隐藏到托盘
void QtETMain::closeEvent(QCloseEvent *event)
{
    // 根据缓存设置决定是否隐藏到托盘
    if (m_hideOnTray) {
        // 隐藏到托盘而不是关闭
        hide();
        m_isHiddenToTray = true;

        // 首次隐藏到托盘时弹出提示
        static bool firstHide = true;
        if (firstHide && m_trayIcon) {
            m_trayIcon->showMessage(
                tr("QtEasyTier"),
                tr("程序已最小化到系统托盘"),
                QSystemTrayIcon::Information,
                2000
            );
            firstHide = false;
        }

        event->ignore();
    } else {
        // 直接退出程序
        onQuitApp();
        event->accept();
    }
}

/// @brief 初始化系统托盘
void QtETMain::initTrayIcon()
{
    // 创建托盘图标
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/icon.ico"));
    m_trayIcon->setToolTip(tr("QtEasyTier"));
    
    // 创建托盘菜单
    m_trayMenu = new QMenu(this);
    
    // 显示窗口动作
    m_showAction = new QAction(tr("显示窗口"), this);
    m_showAction->setIcon(QIcon(":/icons/home.svg"));
    connect(m_showAction, &QAction::triggered, this, &QtETMain::onShowWindow);
    m_trayMenu->addAction(m_showAction);
    
    // 隐藏窗口动作
    m_hideAction = new QAction(tr("隐藏窗口"), this);
    m_hideAction->setIcon(QIcon(":/icons/eye-slash.svg"));
    connect(m_hideAction, &QAction::triggered, this, &QtETMain::onHideWindow);
    m_trayMenu->addAction(m_hideAction);
    
    // 分隔线
    m_trayMenu->addSeparator();
    
    // 退出程序动作
    m_quitAction = new QAction(tr("退出程序"), this);
    m_quitAction->setIcon(QIcon(":/icons/about.svg"));
    connect(m_quitAction, &QAction::triggered, this, &QtETMain::onQuitApp);
    m_trayMenu->addAction(m_quitAction);
    
    // 设置托盘菜单
    m_trayIcon->setContextMenu(m_trayMenu);
    
    // 连接托盘图标激活信号
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &QtETMain::onTrayIconActivated);
    
    // 显示托盘图标
    m_trayIcon->show();
}

/// @brief 托盘图标激活处理
void QtETMain::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::DoubleClick:
            // 双击显示/隐藏窗口
            if (m_isHiddenToTray) {
                onShowWindow();
            } else {
                onHideWindow();
            }
            break;
        case QSystemTrayIcon::Trigger:
            // 单击显示窗口
            onShowWindow();
            break;
        default:
            break;
    }
}

/// @brief 显示窗口
void QtETMain::onShowWindow()
{
    show();
    setWindowState(Qt::WindowActive);
    activateWindow();
    raise();
    m_isHiddenToTray = false;
}

/// @brief 隐藏窗口到托盘
void QtETMain::onHideWindow()
{
    hide();
    m_isHiddenToTray = true;
    
    // 弹出提示消息
    if (m_trayIcon) {
        m_trayIcon->showMessage(
            tr("QtEasyTier"),
            tr("程序已最小化到系统托盘"),
            QSystemTrayIcon::Information,
            2000
        );
    }
}

/// @brief 退出程序
void QtETMain::onQuitApp()
{
    // 保存网络配置
    if (m_networkPage) {
        m_networkPage->saveAllNetworkConfs();
    }
    
    // 隐藏托盘图标
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    
    // 退出应用
    qApp->quit();
}

/// @brief 网络启动通知
void QtETMain::onNetworkStartedNotify(const QString &networkName, bool success, const QString &errorMsg)
{
    if (!m_trayIcon) {
        return;
    }
    
    if (success) {
        m_trayIcon->showMessage(
            tr("网络启动成功"),
            tr("网络 \"%1\" 已成功启动").arg(networkName),
            QSystemTrayIcon::Information,
            3000
        );
    } else {
        m_trayIcon->showMessage(
            tr("网络启动失败"),
            tr("网络 \"%1\" 启动失败:\n%2").arg(networkName).arg(errorMsg),
            QSystemTrayIcon::Warning,
            5000
        );
    }
}

/// @brief 网络停止通知
void QtETMain::onNetworkStoppedNotify(const QString &networkName, bool success, const QString &errorMsg)
{
    if (!m_trayIcon) {
        return;
    }
    
    if (success) {
        m_trayIcon->showMessage(
            tr("网络停止成功"),
            tr("网络 \"%1\" 已成功停止").arg(networkName),
            QSystemTrayIcon::Information,
            3000
        );
    } else {
        m_trayIcon->showMessage(
            tr("网络停止失败"),
            tr("网络 \"%1\" 停止失败:\n%2").arg(networkName).arg(errorMsg),
            QSystemTrayIcon::Warning,
            5000
        );
    }
}

/// @brief 处理系统调色板变化
void QtETMain::onSchemeChanged(const Qt::ColorScheme &scheme)
{
    if (scheme == Qt::ColorScheme::Dark) {

        // 处理暗黑模式，设置sideWidget背景调色板为深色
        ui->sideBox->setPalette(QPalette(QColor("#131516")));
        ui->sideBox->setAutoFillBackground(true);
        // 强制更新一次应用的调色板
        qApp->style()->unpolish(qApp);
        qApp->style()->polish(qApp);
        qApp->processEvents();


    } else {
        // 处理亮色模式，设置sideWidget背景调色板为浅色
        ui->sideBox->setPalette(QPalette(QColor("#f6f7f7")));
        ui->sideBox->setAutoFillBackground(true);
        // 强制更新一次应用的调色板
        qApp->style()->unpolish(qApp);
        qApp->style()->polish(qApp);
        qApp->processEvents();
    }
}

/// @brief 初始化欢迎界面
void QtETMain::initHelloPage()
{
    //获取布局
    auto *mainLayout = qobject_cast<QVBoxLayout*>(m_helloPage->layout());
    mainLayout->setSpacing(10);

    // 主标题（图标 + 标题水平布局）
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(10);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QPixmap(":/icons/icon.ico").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    titleLayout->addWidget(iconLabel);

    QLabel *titleLabel = new QLabel("QtEasyTier", this);
    int fontId = QFontDatabase::addApplicationFont(":/icons/Ubuntu-B.ttf");
    QFont titleFont;
    if (fontId != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            titleFont = QFont(families.first(), 48);
        }
    } else {
        titleFont.setPointSize(48);
    }
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLayout->addWidget(titleLabel);

    titleLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addLayout(titleLayout);

    // 副标题
    QLabel *subTitleLabel = new QLabel("简洁实用的异地组网工具 | 基于 EasyTier", this);
    QFont subTitleFont;
    subTitleFont.setPointSize(12);
    subTitleLabel->setFont(subTitleFont);
    subTitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subTitleLabel);

    mainLayout->addStretch();

    // 设置口号Label
    QLabel *sloganLabel = new QLabel(SLOGAN, this);
    fontId = QFontDatabase::addApplicationFont(":/icons/YeGenyou.ttf");
    QFont sloganFont;
    if (fontId != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            sloganFont = QFont(families.first(), 14);
        }
    } else {
        sloganFont.setPointSize(14);
    }
    sloganLabel->setFont(sloganFont);
    sloganLabel->setAlignment(Qt::AlignCenter);
    sloganLabel->setStyleSheet("color: #66ccff;");
    mainLayout->addWidget(sloganLabel);


    mainLayout->addStretch();

    QLabel *tipLabel = new QLabel(tr("点击右侧问号获取使用帮助"), this);
    tipLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(tipLabel);

    // 功能按钮区域 (2x2 网格布局)
    QGridLayout *buttonGrid = new QGridLayout();
    buttonGrid->setSpacing(6);

    m_aboutUsBtn = new QtETPushBtn("关于项目", this);
    m_aboutUsBtn->setFixedWidth(125);
    m_aboutETBtn = new QtETPushBtn("关于 EasyTier", this);
    m_aboutETBtn->setFixedWidth(125);
    m_donateBtn = new QtETPushBtn("捐赠", this);
    m_donateBtn->setFixedWidth(125);
    m_notClickBtn = new QtETPushBtn("千万别点", this);
    m_notClickBtn->setFixedWidth(125);

    buttonGrid->addWidget(m_aboutUsBtn, 0, 0);
    buttonGrid->addWidget(m_aboutETBtn, 0, 1);
    buttonGrid->addWidget(m_donateBtn, 1, 0);
    buttonGrid->addWidget(m_notClickBtn, 1, 1);
    buttonGrid->setAlignment(Qt::AlignCenter);

    mainLayout->addLayout(buttonGrid);

    // 版权信息
    QLabel *copyrightLabel = new QLabel("Copyright © 2026 明月清风. All rights reserved.", this);
    QFont copyrightFont;
    copyrightFont.setPointSize(10);
    copyrightLabel->setFont(copyrightFont);
    copyrightLabel->setStyleSheet("color: #aaaaaa;");
    copyrightLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(copyrightLabel);

    m_mainStackedWidget->setCurrentWidget(m_helloPage);
}

void QtETMain::initNetworkPage()
{
    // 初始化网络界面
    m_networkPage = new QtETNetwork(this);
    m_mainStackedWidget->addWidget(m_networkPage);
    
    // 加载保存的网络配置
    m_networkPage->loadAllNetworkConfs();

    // 连接切换信号槽
    connect(ui->networkBtn, &QPushButton::clicked, m_networkPage, [=, this]()
    {
        m_mainStackedWidget->setCurrentWidget(m_networkPage);
    });
}

void QtETMain::initOneClickPage()
{
    // 初始化一键组网界面
    m_oneClickPage = new QtETOneClick(this);
    m_mainStackedWidget->addWidget(m_oneClickPage);

    // 连接切换信号槽
    connect(ui->oneClickBtn, &QPushButton::clicked, m_oneClickPage, [=, this]()
    {
        m_mainStackedWidget->setCurrentWidget(m_oneClickPage);
    });
}

void QtETMain::initServersPage()
{
    // 初始化服务器收藏界面
    m_serversPage = new QtETServers(this);
    m_mainStackedWidget->addWidget(m_serversPage);

    // 连接切换信号槽
    connect(ui->serversBtn, &QPushButton::clicked, this, [=, this]()
    {
        m_mainStackedWidget->setCurrentWidget(m_serversPage);
    });
}

void QtETMain::initSettingsPage()
{
    // 初始化设置界面
    m_settingsPage = new QtETSettings(this);
    m_mainStackedWidget->addWidget(m_settingsPage);

    // 连接切换信号槽
    connect(ui->settingsBtn, &QPushButton::clicked, this, [=, this]()
    {
        m_mainStackedWidget->setCurrentWidget(m_settingsPage);
    });

    // 连接隐藏到托盘设置变更信号
    connect(m_settingsPage, &QtETSettings::hideOnTrayChanged, this, &QtETMain::onHideOnTrayChanged);

    // 初始化缓存的设置值
    m_hideOnTray = QtETSettings::isHideOnTray();
}

void QtETMain::onHideOnTrayChanged(bool hideOnTray)
{
    // 更新缓存的设置值
    m_hideOnTray = hideOnTray;
}
