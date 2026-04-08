#ifndef QTETMAIN_H
#define QTETMAIN_H

#include <QWidget>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <ui_qtetmain.h>

#include "qtetnetwork.h"
#include "qtetoneclick.h"
#include "qtetpushbtn.h"
#include "qtetsettings.h"
#include "qtetservers.h"

namespace Ui {
class QtETMain;
}

class QtETMain : public QWidget
{
    Q_OBJECT

public:
    explicit QtETMain(QWidget *parent = nullptr);
    ~QtETMain();

protected:
    /// @brief 重写关闭事件，实现隐藏到托盘
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::QtETMain *ui;

    // ======== 初始化相关 ========
    void initHelloPage();
    void initNetworkPage();
    void initOneClickPage();
    void initServersPage();     // 初始化服务器收藏页面
    void initSettingsPage();    // 初始化设置页面
    void initTrayIcon();        // 初始化系统托盘

    // ======== 子窗口 ========
    QStackedWidget* &m_mainStackedWidget = ui->mainStackedWidget;
    QWidget* &m_helloPage = ui->helloStackedPage;
    QtETNetwork* m_networkPage = nullptr;
    QtETOneClick* m_oneClickPage = nullptr;
    QtETServers* m_serversPage = nullptr;
    QtETSettings* m_settingsPage = nullptr;

    // ======== 欢迎界面控件 ========
    QtETPushBtn *m_aboutUsBtn = nullptr;     // 关于项目
    QtETPushBtn *m_aboutETBtn = nullptr;     // 关于EasyTier
    QtETPushBtn *m_donateBtn = nullptr;      // 捐赠
    QtETPushBtn *m_notClickBtn = nullptr;    // "千万别点"彩蛋

    // ======== 系统托盘 ========
    QSystemTrayIcon *m_trayIcon = nullptr;   // 系统托盘图标
    QMenu *m_trayMenu = nullptr;             // 托盘右键菜单
    QAction *m_showAction = nullptr;         // 显示窗口动作
    QAction *m_hideAction = nullptr;         // 隐藏窗口动作
    QAction *m_quitAction = nullptr;         // 退出程序动作
    bool m_isHiddenToTray = false;           // 是否已隐藏到托盘
    bool m_hideOnTray = true;                // 缓存：是否隐藏到托盘

private slots:
    void onSchemeChanged(const Qt::ColorScheme &scheme);    // 处理系统调色板变化
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason); // 托盘图标激活
    void onShowWindow();                     // 显示窗口
    void onHideWindow();                     // 隐藏窗口到托盘
    void onQuitApp();                        // 退出程序
    void onNetworkStartedNotify(const QString &networkName, bool success, const QString &errorMsg); // 网络启动通知
    void onNetworkStoppedNotify(const QString &networkName, bool success, const QString &errorMsg); // 网络停止通知
    void onHideOnTrayChanged(bool hideOnTray); // 隐藏到托盘设置变更
};

#endif // QTETMAIN_H