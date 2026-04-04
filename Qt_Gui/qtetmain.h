#ifndef QTETMAIN_H
#define QTETMAIN_H

#include <QWidget>
#include <ui_qtetmain.h>

#include <qtetnetwork.h>

namespace Ui {
class QtETMain;
}

class QtETMain : public QWidget
{
    Q_OBJECT

public:
    explicit QtETMain(QWidget *parent = nullptr);
    ~QtETMain();

private:
    Ui::QtETMain *ui;

    // ======== 初始化相关 ========
    void initHelloPage();
    void initNetworkPage();

    // ======== 子窗口 ========
    QStackedWidget* &m_mainStackedWidget = ui->mainStackedWidget;
    QWidget* &m_helloPage = ui->helloStackedPage;
    QtETNetwork* m_networkPage = nullptr;

    // ======== 欢迎界面控件 ========
    QPushButton *m_aboutUsBtn = nullptr;     // 关于项目
    QPushButton *m_aboutETBtn = nullptr;     // 关于EasyTier
    QPushButton *m_donateBtn = nullptr;      // 捐赠
    QPushButton *m_notClickBtn = nullptr;    // “千万别点”彩蛋

private slots:
    void onSchemeChanged(const Qt::ColorScheme &scheme); // 处理系统调色板变化
};

#endif // QTETMAIN_H
