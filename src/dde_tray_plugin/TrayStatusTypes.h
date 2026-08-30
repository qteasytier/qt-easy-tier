/**
 * @file TrayStatusTypes.h
 * @brief DDE 托盘插件使用的轻量网络状态类型
 */
#pragma once

#include "config/ConfigRunState.h"
#include "daemon_service/DaemonClient.h"

#include <QList>
#include <QString>

#include <optional>

/** @brief 单个网络实例的托盘状态，不包含节点明细或日志 */
struct TrayInstanceStatus {
    QString instanceName;
    QString displayName;
    bool local = false;
    ConfigRunState state = ConfigRunState::Stopped;
    std::optional<int> nodeCount;
};

/** @brief 托盘插件展示的整体状态快照 */
struct TrayStatusSnapshot {
    DaemonClient::ConnectionState daemonState =
        DaemonClient::ConnectionState::Disconnected;
    QList<TrayInstanceStatus> instances;
};
