#ifndef QTETNETWORK_H
#define QTETNETWORK_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTabWidget>
#include <QPushButton>

#include "qtetlistwidget.h"

/// @brief 网络配置页面
/// 提供网络列表和配置选项卡的界面
class QtETNetwork : public QWidget
{
    Q_OBJECT

public:
    explicit QtETNetwork(QWidget *parent = nullptr);
    ~QtETNetwork() override;

private:
    /// @brief 初始化界面
    void initUI();
    /// @brief 初始化左侧网络列表区域
    void initLeftPanel();
    /// @brief 初始化右侧选项卡区域
    void initRightPanel();

private:
    // 左侧面板
    QFrame *m_leftFrame;                // 左侧面板容器
    QVBoxLayout *m_leftLayout;          // 左侧布局
    QtETListWidget *m_networksList;     // 网络列表
    QPushButton *m_newNetworkBtn;       // 新建网络按钮
    QPushButton *m_runNetworkBtn;       // 运行网络按钮
    QPushButton *m_importConfBtn;       // 导入配置按钮
    QPushButton *m_exportConfBtn;       // 导出配置按钮

    // 右侧面板
    QTabWidget *m_tabWidget;            // 选项卡容器
    QWidget *m_basicSettingsTab;        // 基础设置选项卡
    QWidget *m_advancedSettingsTab;     // 高级设置选项卡
    QWidget *m_runningStatusTab;        // 运行状态选项卡
    QWidget *m_runningLogTab;           // 运行日志选项卡

    // 主布局
    QHBoxLayout *m_mainLayout;          ///< 主布局
};

#endif // QTETNETWORK_H