#ifndef ONECLICK_H
#define ONECLICK_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QTimer>
#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace Ui {
class OneClick;
}

class OneClick : public QWidget
{
    Q_OBJECT

public:
    explicit OneClick(QWidget *parent = nullptr);
    ~OneClick();

    bool isRunning() const { return m_coreProcess && m_coreProcess->state() == QProcess::Running; }

private slots:
    // 房主相关槽函数
    void onHostStartClicked();
    void onAddServerClicked();
    void onRemoveServerClicked();
    void onPublicServerClicked();
    void onFinishedCore(int exitCode, QProcess::ExitStatus exitStatus);
    // 房客相关槽函数
    void onGuestStartClicked();
    // Tab切换处理
    void onTabChanged(int index);
    // 更新运行cli获取信息
    void updateHostIpAddress();
    // 更新联机人数
    void updatePlayerNum();

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

    // ET进程相关
    QProcess *m_coreProcess = nullptr;
    QProcess *m_cliProcess = nullptr;

    // 启动过程对话框
    QDialog *m_processDialog = nullptr;
    QPlainTextEdit *m_processLogTextEdit = nullptr;

    // 房主IP地址更新相关
    QTimer *m_cliUpdateTimer =  nullptr;
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
    // 核心启动逻辑（提取的公共函数）
    bool startEasyTierProcess(const QStringList& arguments, const QString& windowTitle);
    // 停止当前运行的进程
    void stopCurrentProcess();
    // 更新界面状态
    void updateInterfaceState(UserRole role);
    // 验证Tab切换是否允许
    bool canSwitchToTab(int tabIndex);
    // 创建或重建启动过程对话框
    void createProcessDialog(const QString& title);
    // 关闭启动过程对话框
    void closeProcessDialog();
    // 解析CLI输出获取房主IP
    void parseHostIpAddress(const QByteArray& jsonData);
    void parsePlayerNum(const QByteArray& jsonData);
};

#endif // ONECLICK_H
