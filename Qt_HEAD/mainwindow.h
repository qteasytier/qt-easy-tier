#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "netpage.h"
#include "setting.h"
#include "oneclick.h"

#include <QMainWindow>
#include <QVector>
#include <QJsonObject>  // 添加JSON支持
#include <QSystemTrayIcon>  // 添加系统托盘支持
#include <QMenu>           // 添加菜单支持
#include <QAction>         // 添加动作支持

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 网络配置管理
    void loadNetworkConfig();  // 加载网络配置
    void saveNetworkConfig();  // 保存网络配置

private slots:
    void onNetListItemDoubleClicked(QListWidgetItem *item);
    void onRenameNetwork();
    void onDeleteNetwork();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);  // 系统托盘激活事件
    void onShowWindow();  // 显示主窗口
    void onExitApp();     // 退出应用程序
    void onClickOneClickBtn(); // 当点击一键联机按钮时

protected:
    void closeEvent(QCloseEvent *event) override;  // 重写关闭事件

private:
    Ui::MainWindow *ui;

    // 界面组件
    QVector<NetPage*> m_netpages;
    OneClick *m_oneClick;
    setting *m_settingsWindow;

    // 系统托盘相关
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QAction *showAction;
    QAction *exitAction;

    void __changeWidget(QWidget *newWidget) const;
    void setupContextMenu();
    void createTrayIcon();  // 创建系统托盘
};
#endif // MAINWINDOW_H