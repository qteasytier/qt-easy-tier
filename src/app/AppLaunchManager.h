/**
 * @file AppLaunchManager.h
 * @brief 应用启动管理工具
 *
 * 只放与应用入口相关的轻量启动逻辑，避免在 main.cpp 中堆叠不可测试的判断。
 */
#pragma once

#include <QObject>
#include <QStringList>

class QLocalServer;

/** @brief 应用启动管理器：启动参数判断和前端单实例监听 */
class AppLaunchManager : public QObject {
    Q_OBJECT

public:
    explicit AppLaunchManager(QObject *parent = nullptr);
    ~AppLaunchManager() override;

    /** @brief 判断当前启动是否来自开机自启动入口 */
    static bool isAutoStartLaunch(const QStringList &arguments);
    /** @brief 前端单实例使用的本地 socket 名称 */
    static QString singleInstanceServerName();

    /** @brief 尝试连接已有前端实例；成功表示当前进程应退出 */
    bool tryConnectToExistingInstance(const QString &serverName = singleInstanceServerName(), int timeoutMs = 100);
    /** @brief 作为首实例监听本地 socket，收到连接时发出 activationRequested */
    bool listenForSingleInstance(const QString &serverName = singleInstanceServerName());
    /** @brief 当前是否已成功监听单实例 socket */
    bool isSingleInstanceListening() const;

signals:
    /** @brief 有新实例尝试启动，首实例应显示主窗口 */
    void activationRequested();

private:
    void handlePendingActivationRequests();

    QLocalServer *m_singleInstanceServer = nullptr;
    QString m_singleInstanceServerName;
};
