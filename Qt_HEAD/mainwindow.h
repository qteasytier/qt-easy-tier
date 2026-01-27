#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "netpage.h"

#include <QMainWindow>
#include <QVector>
#include <QJsonObject>  // 添加JSON支持

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

private:
    Ui::MainWindow *ui;

    // 界面组件
    QVector<NetPage*> m_netpages;

    void __changeWidget(QWidget *newWidget) const;
    void setupContextMenu();
};
#endif // MAINWINDOW_H