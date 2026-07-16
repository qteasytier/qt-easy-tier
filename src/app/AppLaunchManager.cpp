/**
 * @file AppLaunchManager.cpp
 * @brief 应用启动管理工具实现
 */
#include "AppLaunchManager.h"

#include <QLocalServer>
#include <QLocalSocket>

AppLaunchManager::AppLaunchManager(QObject *parent)
    : QObject(parent)
    , m_singleInstanceServer(new QLocalServer(this))
{
    connect(m_singleInstanceServer, &QLocalServer::newConnection,
            this, &AppLaunchManager::handlePendingActivationRequests);
}

AppLaunchManager::~AppLaunchManager()
{
    if (m_singleInstanceServer->isListening())
        m_singleInstanceServer->close();
    if (!m_singleInstanceServerName.isEmpty())
        QLocalServer::removeServer(m_singleInstanceServerName);
}

bool AppLaunchManager::isAutoStartLaunch(const QStringList &arguments)
{
    // 精确匹配自启动参数，避免 --autostarted 等相似参数误判。
    return arguments.contains(QStringLiteral("--autostart"));
}

QString AppLaunchManager::singleInstanceServerName()
{
    return QStringLiteral("cn.qtet.qteasytier.sock");
}

bool AppLaunchManager::tryConnectToExistingInstance(const QString &serverName, int timeoutMs)
{
    QLocalSocket socket;
    socket.connectToServer(serverName);
    return socket.waitForConnected(timeoutMs);
}

bool AppLaunchManager::listenForSingleInstance(const QString &serverName)
{
    if (m_singleInstanceServer->isListening())
        m_singleInstanceServer->close();

    m_singleInstanceServerName = serverName;
    if (m_singleInstanceServer->listen(serverName))
        return true;

    QLocalServer::removeServer(serverName);
    return m_singleInstanceServer->listen(serverName);
}

bool AppLaunchManager::isSingleInstanceListening() const
{
    return m_singleInstanceServer->isListening();
}

void AppLaunchManager::handlePendingActivationRequests()
{
    while (m_singleInstanceServer->hasPendingConnections()) {
        QLocalSocket *socket = m_singleInstanceServer->nextPendingConnection();
        socket->disconnectFromServer();
        socket->deleteLater();
    }

    emit activationRequested();
}
