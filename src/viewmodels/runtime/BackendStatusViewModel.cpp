/**
 * @file BackendStatusViewModel.cpp
 * @brief BackendStatusViewModel 实现
 *
 * 连接 DaemonClient::connectionStateChanged 信号，
 * 当连接状态变化时一次性发射三个 Changed 信号通知 QML 刷新。
 */
#include "BackendStatusViewModel.h"

#include "core/service/DaemonClient.h"

BackendStatusViewModel::BackendStatusViewModel(DaemonClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    if (!m_client)
        return;

    // 监听守护进程客户端连接状态变化
    // 状态变化时一次性发射三个属性变更信号，让 QML 同步刷新
    connect(m_client, &DaemonClient::connectionStateChanged, this, [this]() {
        emit connectedChanged();
        emit connectingChanged();
        emit statusTextChanged();
    });
}

bool BackendStatusViewModel::connected() const
{
    // 检查连接状态是否为 Connected
    return m_client && m_client->connectionState() == DaemonClient::ConnectionState::Connected;
}

bool BackendStatusViewModel::connecting() const
{
    // 检查连接状态是否为 Connecting
    return m_client && m_client->connectionState() == DaemonClient::ConnectionState::Connecting;
}

QString BackendStatusViewModel::statusText() const
{
    // 根据当前连接状态返回对应的中文状态文本
    if (connected())
        return QStringLiteral("后端已连接");
    if (connecting())
        return QStringLiteral("连接中...");
    return QStringLiteral("后端未连接");
}
