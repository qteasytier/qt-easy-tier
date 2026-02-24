#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "netpage.h"
#include "setting.h"
#include "oneclick.h"

#include <QMainWindow>
#include <QVector>
#include <QSystemTrayIcon>  // 添加系统托盘支持
#include <QMenu>           // 添加菜单支持
#include <QAction>         // 添加动作支持
#include <QStandardPaths>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

// 前向声明
class Donate;
class WebDashboardWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 网络配置管理
    void loadConfig();  // 加载配置
    void saveNetworkConfig();  // 保存网络配置

private slots:
    void onNetListItemDoubleClicked(QListWidgetItem *item);
    void onRenameNetwork();
    void onDeleteNetwork();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);  // 系统托盘激活事件
    void onShowWindow();  // 显示主窗口
    void onExitApp();     // 退出应用程序
    void onClickOneClickBtn(); // 当点击一键联机按钮时
    void onClickSettingBtn(); // 当点击设置按钮时

    void onClickWebDashboardBtn();  // 当点击Web控制台按钮时

    // Web控制台 Worker 信号处理槽函数
    void onWebProcessStarted(bool success, const QString &message);
    void onWebProcessStopped(bool success);
    void onWebProcessCrashed(int exitCode, QProcess::ExitStatus exitStatus);

protected:
    void closeEvent(QCloseEvent *event) override;  // 重写关闭事件

private:
    Ui::MainWindow *ui;

    // 界面组件
    QVector<NetPage*> m_netpages;
    OneClick *m_oneClick = nullptr;

    // 系统托盘相关
    QSystemTrayIcon *trayIcon = nullptr;
    QMenu *trayMenu = nullptr;
    QAction *showAction = nullptr;
    QAction *exitAction = nullptr;
    bool m_isHideOnTray =  true;

    // Web控制台
    WebDashboardWorker *m_webWorker = nullptr;

    // 配置保存路径
#if SAVE_CONF_IN_APP_DIR == true
    QString m_configPath = QApplication::applicationDirPath() + "/config";
#else
    QString m_configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);;
#endif

    void _changeWidget(QWidget *newWidget);
    void setupContextMenu();
    void createTrayIcon();  // 创建系统托盘
};
#endif // MAINWINDOW_H