#ifndef ONECLICK_H
#define ONECLICK_H

#include "easytierworker.h"

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QString>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QTimer>
#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QThread>
#include <QProgressBar>

namespace Ui {
class OneClick;
}

class OneClick : public QWidget
{
    Q_OBJECT

public:
    explicit OneClick(QWidget *parent = nullptr);
    ~OneClick();

    bool isRunning() const;

private slots:
    // 房主相关槽函数
    void onHostStartClicked();
    void onAddServerClicked();
    void onRemoveServerClicked();
    void onPublicServerClicked();

    // 房客相关槽函数
    void onGuestStartClicked();

    // Tab切换处理
    void onTabChanged(int index);

    // EasyTierWorker信号处理
    void onWorkerProcessStarted(bool success, const QString& errorMessage);
    void onWorkerProcessStopped(bool success);
    void onWorkerLogOutput(const QString& logText, bool isError);
    void onWorkerPeerInfoUpdated(const QJsonArray& peers);
    void onWorkerProcessCrashed(int exitCode);

private:
    Ui::OneClick *ui;

    // 房主界面组件
    QLineEdit *m_hostCodeLineEdit = nullptr;
    QLineEdit *m_serverAddrEdit = nullptr;
    QLineEdit *m_playerCountEdit = nullptr;
    QPushButton *m_addServerBtn = nullptr;
    QPushButton *m_removeServerBtn = nullptr;
    QPushButton *m_publicServerBtn = nullptr;
    QPushButton *m_hostStartBtn = nullptr;
    QListWidget *m_serverListWidget = nullptr;

    // 房客界面组件
    QLineEdit *m_guestCodeLineEdit = nullptr;
    QLineEdit *m_guestIpLineEdit = nullptr;
    QPushButton *m_guestStartBtn = nullptr;

    // 当前生成的房间信息
    QString m_currentNetworkId = "";
    QString m_currentPassword = "";

    // EasyTierWorker相关（线程和工作对象）
    QThread* m_workerThread = nullptr;
    EasyTierWorker* m_worker = nullptr;

    // 启动过程对话框
    QDialog* m_processDialog = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_progressLabel = nullptr;

    // 房主IP地址更新相关
    QString m_lastHostIp = "";

    // 身份状态管理
    enum class UserRole {
        None,    // 未运行任何角色
        Host,    // 当前是房主
        Guest    // 当前是房客
    };
    UserRole m_currentRole = UserRole::None;

    // 初始化界面组件
    void initHostComponents();
    void initGuestComponents();

    // 服务器管理相关
    void setupServerList();

    // 初始化Worker线程
    void initWorkerThread();
    // 清理Worker线程
    void cleanupWorkerThread();

    // 停止当前运行的进程
    void stopCurrentProcess();

    // 更新界面状态
    void updateInterfaceState(UserRole role);

    // 验证Tab切换是否允许
    bool canSwitchToTab(int tabIndex);

    // 显示启动过程对话框（无限进度条）
    void showProcessDialog(const QString& title);
    // 关闭启动过程对话框
    void closeProcessDialog();

    // 解析节点信息更新房主IP和联机人数
    void parsePeerInfo(const QJsonArray& peers);
};

#endif // ONECLICK_H