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

    // 更新房主IP地址
    void updateHostIpAddress();

private:
    Ui::OneClick *ui;

    // 房主界面组件
    QLineEdit *m_hostCodeLineEdit;
    QLineEdit *m_serverAddrEdit;
    QPushButton *m_addServerBtn;
    QPushButton *m_removeServerBtn;
    QPushButton *m_publicServerBtn;
    QPushButton *m_hostStartBtn;
    QListWidget *m_serverListWidget;

    // 房客界面组件
    QLineEdit *m_guestCodeLineEdit;
    QLineEdit *m_guestIpLineEdit;
    QPushButton *m_guestStartBtn;

    // 当前生成的房间信息
    QString m_currentNetworkId;
    QString m_currentPassword;

    // ET进程相关
    QProcess *m_coreProcess;
    QProcess *m_cliProcess;

    // 启动过程对话框
    QDialog *m_processDialog;
    QPlainTextEdit *m_processLogTextEdit;

    // 房主IP地址更新相关
    QTimer *m_hostIpUpdateTimer;
    QString m_lastHostIp;

    // 身份状态管理
    enum class UserRole {
        None,    // 未运行任何角色
        Host,    // 当前是房主
        Guest    // 当前是房客
    };
    UserRole m_currentRole;

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

signals:
    void isNotRunning();  // 未运行任何角色

protected:
    // 重写窗口隐藏事件
    void hideEvent(QHideEvent *event) override {
        QWidget::hideEvent(event);
        if (!(m_coreProcess && m_coreProcess->state() == QProcess::Running)) {
            emit isNotRunning();
        }
    };
};

#endif // ONECLICK_H
