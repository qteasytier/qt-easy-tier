/**
 * @file SystemTrayManager.cpp
 * @brief 系统托盘管理器实现
 */
#include "SystemTrayManager.h"

#include "core/system_tray/TrayMessageDispatcher.h"
#include "core/system_tray/TrayMessageHelper.h"
#include "core/util/LogHelper.h"

#include <QAction>
#include <QCoreApplication>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWindow>

SystemTrayManager::SystemTrayManager(QObject *parent)
    : QObject(parent)
{
    createTrayIcon();
    TrayMessageDispatcher::instance()->addSink(this);
}

SystemTrayManager::~SystemTrayManager()
{
    TrayMessageDispatcher::instance()->removeSink(this);
    if (m_mainWindow)
        m_mainWindow->removeEventFilter(this);
    if (m_trayIcon)
        m_trayIcon->setContextMenu(nullptr);
    delete m_menu;
}

void SystemTrayManager::setMainWindow(QWindow *window)
{
    if (m_mainWindow == window)
        return;

    if (m_mainWindow) {
        m_mainWindow->removeEventFilter(this);
        QObject::disconnect(m_mainWindow.data(), nullptr, this, nullptr);
    }

    m_mainWindow = window;

    if (m_mainWindow && isAvailable()) {
        m_mainWindow->installEventFilter(this);
        connect(m_mainWindow, &QWindow::visibleChanged,
                this, &SystemTrayManager::updateWindowActionText,
                Qt::UniqueConnection);
        connect(m_mainWindow, &QObject::destroyed, this, [this, window]() {
            // QML 根窗口可能早于托盘管理器析构；窗口销毁后必须清空弱引用，避免退出时访问悬空窗口。
            if (m_mainWindow == window)
                m_mainWindow = nullptr;
            updateWindowActionText();
        });
    }

    updateWindowActionText();
}

void SystemTrayManager::showMainWindowFromTray()
{
    showMainWindow();
}

bool SystemTrayManager::quitRequested() const
{
    return m_quitRequested;
}

bool SystemTrayManager::isAvailable() const
{
    return m_trayIcon && m_trayIcon->isVisible();
}

void SystemTrayManager::setDaemonConnectionState(DaemonClient::ConnectionState state)
{
    if (m_daemonState == state)
        return;

    m_daemonState = state;
    // daemon 不可用时，本地运行实例状态已不可信，立即清空托盘侧计数。
    if (m_daemonState != DaemonClient::ConnectionState::Connected)
        m_runningInstances.clear();

    refreshTrayStatus();
}

void SystemTrayManager::setConfigRunState(const QString &instanceName, ConfigRunState state)
{
    if (instanceName.isEmpty())
        return;

    if (state == ConfigRunState::Running)
        m_runningInstances.insert(instanceName);
    else
        m_runningInstances.remove(instanceName);

    refreshTrayStatus();
}

int SystemTrayManager::runningConfigCount() const
{
    return m_runningInstances.size();
}

QString SystemTrayManager::daemonStatusText() const
{
    switch (m_daemonState) {
    case DaemonClient::ConnectionState::Connected:
        return QStringLiteral("已连接");
    case DaemonClient::ConnectionState::Connecting:
        return QStringLiteral("连接中");
    case DaemonClient::ConnectionState::Disconnected:
        return QStringLiteral("未连接");
    }
    return QStringLiteral("未连接");
}

QString SystemTrayManager::daemonStatusMenuText() const
{
    switch (m_daemonState) {
    case DaemonClient::ConnectionState::Connected:
        return QStringLiteral("🟢 后端状态：%1").arg(daemonStatusText());
    case DaemonClient::ConnectionState::Connecting:
        return QStringLiteral("🟠 后端状态：%1").arg(daemonStatusText());
    case DaemonClient::ConnectionState::Disconnected:
        return QStringLiteral("🔴 后端状态：%1").arg(daemonStatusText());
    }
    return QStringLiteral("🔴 后端状态：%1").arg(daemonStatusText());
}

QString SystemTrayManager::networkCountMenuText() const
{
    const QString indicator = runningConfigCount() > 0
                                  ? QStringLiteral("🟢")
                                  : QStringLiteral("🟠");
    return QStringLiteral("%1 网络连接：%2 个").arg(indicator).arg(runningConfigCount());
}

QString SystemTrayManager::currentIconPath() const
{
    if (m_daemonState != DaemonClient::ConnectionState::Connected)
        return QStringLiteral(":/icons/qtet-red.png");
    if (!m_runningInstances.isEmpty())
        return QStringLiteral(":/icons/qtet-green.png");
    return QStringLiteral(":/icons/qtet.png");
}

void SystemTrayManager::showTrayMessage(const TrayMessage &message)
{
    if (!m_trayIcon || !m_trayIcon->isVisible())
        return;

    m_trayIcon->showMessage(message.title,
                            message.message,
                            toQtMessageIcon(message.level),
                            message.durationMs);
}

bool SystemTrayManager::eventFilter(QObject *watched, QEvent *event)
{
    if (m_mainWindow && watched == m_mainWindow.data() && event->type() == QEvent::Close && isAvailable() && !m_quitRequested) {
        event->ignore();
        hideMainWindow();
        TrayMessageHelper::showInfo(QStringLiteral("QtEasyTier"),
                                    QStringLiteral("程序已隐藏到系统托盘，右键托盘图标可退出程序。"));
        return true;
    }

    return QObject::eventFilter(watched, event);
}

void SystemTrayManager::createTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        LogHelper::logWarning(QStringLiteral("当前系统托盘不可用，托盘功能将被禁用。"),
                              QStringLiteral("SystemTray"));
        return;
    }

    m_menu = new QMenu();
    m_daemonStatusAction = m_menu->addAction(QString());
    m_networkCountAction = m_menu->addAction(QString());
    m_menu->addSeparator();
    m_toggleWindowAction = m_menu->addAction(QStringLiteral("关闭程序页面"));
    m_quitAction = m_menu->addAction(QStringLiteral("退出程序"));

    m_trayIcon = new QSystemTrayIcon(QIcon(currentIconPath()), this);
    m_trayIcon->setContextMenu(m_menu);

    connect(m_toggleWindowAction, &QAction::triggered,
            this, &SystemTrayManager::toggleMainWindow);
    connect(m_quitAction, &QAction::triggered,
            this, &SystemTrayManager::requestQuit);
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
                // 常见桌面环境下 Trigger 是左键单击，DoubleClick 是双击；二者都用于恢复窗口。
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                    showMainWindowFromTray();
            });

    m_trayIcon->show();
    refreshTrayStatus();
}

void SystemTrayManager::updateWindowActionText()
{
    if (!m_toggleWindowAction)
        return;

    const bool windowVisible = m_mainWindow && m_mainWindow->isVisible();
    m_toggleWindowAction->setText(windowVisible
                                      ? QStringLiteral("关闭程序页面")
                                      : QStringLiteral("开启程序页面"));
}

void SystemTrayManager::updateStatusActions()
{
    if (m_daemonStatusAction)
        m_daemonStatusAction->setText(daemonStatusMenuText());
    if (m_networkCountAction)
        m_networkCountAction->setText(networkCountMenuText());
}

void SystemTrayManager::updateTrayIcon()
{
    if (!m_trayIcon)
        return;

    m_trayIcon->setIcon(QIcon(currentIconPath()));
}

void SystemTrayManager::updateTrayToolTip()
{
    if (!m_trayIcon)
        return;

    m_trayIcon->setToolTip(QStringLiteral("QtEasyTier\n后端状态：%1\n网络连接：%2 个")
                               .arg(daemonStatusText())
                               .arg(runningConfigCount()));
}

void SystemTrayManager::refreshTrayStatus()
{
    updateStatusActions();
    updateTrayIcon();
    updateTrayToolTip();
}

void SystemTrayManager::showMainWindow()
{
    if (!m_mainWindow)
        return;

    m_mainWindow->show();
    m_mainWindow->raise();
    m_mainWindow->requestActivate();
    updateWindowActionText();
}

void SystemTrayManager::hideMainWindow()
{
    if (!m_mainWindow)
        return;

    m_mainWindow->hide();
    updateWindowActionText();
}

void SystemTrayManager::toggleMainWindow()
{
    if (m_mainWindow && m_mainWindow->isVisible())
        hideMainWindow();
    else
        showMainWindowFromTray();
}

void SystemTrayManager::requestQuit()
{
    emit quitRequestedByUser();
}

void SystemTrayManager::quitApplication()
{
    m_quitRequested = true;
    // 退出前主动解除窗口事件过滤，避免 QML 根窗口销毁时回调已进入退出流程的托盘管理器。
    if (m_mainWindow) {
        m_mainWindow->removeEventFilter(this);
        m_mainWindow = nullptr;
    }
    // 托盘菜单由原生平台事件触发，先解绑并隐藏托盘图标，再延迟到当前菜单事件返回后退出。
    if (m_trayIcon) {
        m_trayIcon->setContextMenu(nullptr);
        m_trayIcon->hide();
    }
    QTimer::singleShot(0, QCoreApplication::instance(), []() {
        QCoreApplication::quit();
    });
}

QSystemTrayIcon::MessageIcon SystemTrayManager::toQtMessageIcon(TrayMessageLevel level) const
{
    switch (level) {
    case TrayMessageLevel::Info:
        return QSystemTrayIcon::Information;
    case TrayMessageLevel::Warning:
        return QSystemTrayIcon::Warning;
    case TrayMessageLevel::Error:
        return QSystemTrayIcon::Critical;
    }
    return QSystemTrayIcon::NoIcon;
}
