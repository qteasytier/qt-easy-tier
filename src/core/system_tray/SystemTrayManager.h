/**
 * @file SystemTrayManager.h
 * @brief 系统托盘管理器
 *
 * 负责创建托盘图标、右键菜单、窗口显隐行为和真实系统通知输出。
 * 托盘消息通过 TrayMessageSink 接口接入，调用方不直接依赖此类。
 */
#pragma once

#include "core/application/runtime/ConfigRunState.h"
#include "core/service/DaemonClient.h"
#include "core/system_tray/TrayMessageSink.h"

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QSystemTrayIcon>

class QAction;
class QMenu;
class QWindow;

/** @brief 应用系统托盘控制器 */
class SystemTrayManager : public QObject, public TrayMessageSink {
    Q_OBJECT

public:
    explicit SystemTrayManager(QObject *parent = nullptr);
    ~SystemTrayManager() override;

    /** @brief 绑定主窗口，托盘菜单和关闭拦截都围绕该窗口工作 */
    void setMainWindow(QWindow *window);
    /** @brief 从托盘或启动流程显示主窗口，并同步菜单文字 */
    void showMainWindowFromTray();

    /** @brief 托盘菜单“退出程序”会设置该标记后允许应用真正退出 */
    bool quitRequested() const;
    /** @brief 当前平台是否成功创建了系统托盘图标 */
    bool isAvailable() const;

    /** @brief 执行真实退出流程：解除窗口过滤、隐藏托盘并退出应用 */
    void quitApplication();

    /** @brief 更新 daemon 连接状态，并刷新托盘菜单和图标 */
    void setDaemonConnectionState(DaemonClient::ConnectionState state);
    /** @brief 更新单个网络配置运行状态，仅 Running 会计入网络连接数量 */
    void setConfigRunState(const QString &instanceName, ConfigRunState state);
    /** @brief 当前 Running 网络配置数量，供菜单展示和测试验证 */
    int runningConfigCount() const;
    /** @brief 当前 daemon 状态中文文案 */
    QString daemonStatusText() const;
    /** @brief 带颜色圆点的 daemon 状态菜单文本 */
    QString daemonStatusMenuText() const;
    /** @brief 带颜色圆点的网络连接数量菜单文本 */
    QString networkCountMenuText() const;
    /** @brief 当前应使用的托盘图标资源路径 */
    QString currentIconPath() const;

    /** @brief TrayMessageSink 实现：把统一消息转换为 QSystemTrayIcon 通知 */
    void showTrayMessage(const TrayMessage &message) override;

signals:
    /** @brief 用户通过托盘菜单请求退出，由 AppServices 决定是否确认退出 */
    void quitRequestedByUser();

protected:
    /** @brief 拦截主窗口关闭事件，将右上角关闭转换为隐藏到托盘 */
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void createTrayIcon();
    void updateWindowActionText();
    void updateStatusActions();
    void updateTrayIcon();
    void updateTrayToolTip();
    void refreshTrayStatus();
    void showMainWindow();
    void hideMainWindow();
    void toggleMainWindow();
    void requestQuit();
    QSystemTrayIcon::MessageIcon toQtMessageIcon(TrayMessageLevel level) const;

    QPointer<QWindow> m_mainWindow;           ///< QML 根窗口弱引用，窗口销毁后自动置空
    QSystemTrayIcon *m_trayIcon = nullptr;    ///< 系统托盘图标
    QMenu *m_menu = nullptr;                  ///< 托盘右键菜单
    QAction *m_daemonStatusAction = nullptr;  ///< 只读状态项：daemon 连接状态
    QAction *m_networkCountAction = nullptr;  ///< 只读状态项：Running 网络数量
    QAction *m_toggleWindowAction = nullptr;  ///< 开启/关闭程序页面
    QAction *m_quitAction = nullptr;          ///< 退出程序
    QSet<QString> m_runningInstances;         ///< 当前 Running 状态的配置实例名集合
    DaemonClient::ConnectionState m_daemonState = DaemonClient::ConnectionState::Disconnected;
    bool m_quitRequested = false;             ///< 是否由托盘菜单请求真正退出
};
