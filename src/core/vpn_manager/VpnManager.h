/**
 * @file VpnManager.h
 * @brief VPN 全局管理器单例
 *
 * 管理所有网络配置的生命周期，为每个配置维护独立的 VpnController 状态机。
 * 内置心跳定时器，通过 daemon list_instances 同步真实运行状态。
 * 暴露 activeInstanceName 与运行状态模型供 QML 绑定。
 *
 * ## 架构职责
 * - VpnManager 是 QML 层的唯一入口，所有启动/停止/查询操作均通过此类
 * - 每个配置对应一个 VpnController 实例（懒创建，存储在 m_controllers 哈希表中）
 * - 心跳定时器每 3 秒向 daemon 查询当前运行实例列表，与内部状态机对比纠偏
 * - collect_network_infos 的结果交给 StatusMonitor 异步解析，解析完成后回填到对应 VpnController
 *
 * ## 线程模型
 * - 所有操作均在主线程（QML 引擎线程）
 * - DaemonClient 的 IPC 调用通过 QFutureWatcher 异步等待，回调在主线程
 * - StatusMonitor 的异步解析在 QtConcurrent 线程池执行，通过 QueuedConnection 回主线程
 *
 * @see VpnController
 * @see DaemonClient
 * @see StatusMonitor
 */
#pragma once
#include <QObject>
#include <QHash>
#include <QTimer>
#include <QVariantList>
#include "core/application/runtime/ConfigRunState.h"
#include "core/service/DaemonClient.h"
#include "core/viewmodel/runtime/NodeInfoModel.h"
#include "core/viewmodel/runtime/RuntimeLogModel.h"
#include "VpnController.h"

class DaemonApi;
class NetworkConfigRepository;
class StatusMonitor;
class QJsonArray;

/** @brief VPN 全局管理器单例，负责所有网络配置的生命周期管理与 QML 交互入口 */
class VpnManager : public QObject {
    Q_OBJECT

    /// QML 属性：当前正在查看的实例名称（用于日志和节点信息面板）
    Q_PROPERTY(QString activeInstanceName READ activeInstanceName
               WRITE setActiveInstanceName NOTIFY activeInstanceNameChanged FINAL)

    /// QML 属性：节点信息模型（当前查看实例的节点列表）
    Q_PROPERTY(NodeInfoModel *nodeInfoModel READ nodeInfoModel CONSTANT)
    /// QML 属性：运行时日志模型（当前查看实例的事件日志）
    Q_PROPERTY(RuntimeLogModel *runtimeLogModel READ runtimeLogModel CONSTANT)

public:
    /**
     * @brief 构造函数
     * @param client        daemon IPC 客户端（用于发送 run/delete/list 等 RPC 调用）
     * @param repo          配置仓库（启动时读取 TOML 配置）
     * @param statusMonitor 状态监视器（接收 collect_network_infos 结果并异步解析）
     * @param parent        父对象（应为 QQmlApplicationEngine，防止 double free）
     */
    explicit VpnManager(DaemonClient *client, DaemonApi *daemonApi, NetworkConfigRepository *repo,
                        StatusMonitor *statusMonitor, QObject *parent = nullptr);

    /// 请求启动指定配置（如果未运行则创建 controller 并进入 Starting 状态）
    Q_INVOKABLE void startConfig(const QString &instanceName);

    /// 请求停止指定配置（仅 Running 状态下有效，进入 Stopping 状态）
    Q_INVOKABLE void stopConfig(const QString &instanceName);

    /// 查询指定配置的当前状态，返回 VpnController::State 枚举的整数值
    Q_INVOKABLE int configState(const QString &instanceName) const;

    /// 查询指定配置是否正在运行中
    Q_INVOKABLE bool isRunning(const QString &instanceName) const;

    /// 导出当前选中实例的运行日志到本地文件
    Q_INVOKABLE bool exportLog(const QString &filePath);

    /// 移除指定配置的 controller 并释放资源（仅在配置被删除时调用）
    void removeController(const QString &instanceName);

    // QML 属性读取器

    /// 获取当前 QML 选中的实例名
    QString activeInstanceName() const;
    /// 设置当前 QML 选中的实例名，并刷新节点信息和日志模型
    void setActiveInstanceName(const QString &name);
    /// 获取节点信息模型（当前选中实例的节点数据）
    NodeInfoModel *nodeInfoModel() const;
    /// 获取运行时日志模型（当前选中实例的事件日志）
    RuntimeLogModel *runtimeLogModel() const;

    /// 设置运行状态页是否隐藏公共服务器节点
    void setHideServerNodes(bool value);

signals:
    /// 通知 QML：某配置的状态已变更（参数字 state 为 VpnController::State 枚举整数值）
    void configStateChanged(const QString &instanceName, ConfigRunState state);

    /// 通知 QML：某配置的停止操作失败（daemon 返回错误或超时）
    void stopFailed(const QString &instanceName, const QString &error);

    /// 通知 QML：当前选中实例名已变更
    void activeInstanceNameChanged();

public slots:
    /// StatusMonitor 异步解析完成后回调：将解析结果缓存到对应 VpnController 并刷新 QML 绑定
    /// @param instName   实例名称
    /// @param nodeInfos  解析后的节点信息列表
    /// @param logEntries 解析后的事件日志列表
    void onInstanceInfoParsed(const QString &instName,
                              const QVariantList &nodeInfos,
                              const QVariantList &logEntries);

    /// 清理指定配置的 controller 资源（由外部在删除配置后调用）
    void cleanupController(const QString &instanceName);

private slots:
    /// 心跳定时器到期回调：向 daemon 发送 list_instances 请求
    void onHeartbeat();

    /// daemon 连接状态变更回调：连接时启动心跳，断开时停止心跳并重置所有 controller
    void onDaemonConnectionChanged(DaemonClient::ConnectionState state);

    /// 收到 list_instances 响应：同步 daemon 中的运行状态，并发起 collect_network_infos 请求
    void onGotInstList(const QJsonObject &result);

    /// 收到 collect_network_infos 响应：将原始 JSON 交给 StatusMonitor 异步解析
    void onGotNetworkInfos(const QJsonObject &result);

private:
    /// 获取或懒创建一个 VpnController 实例
    VpnController *getOrCreate(const QString &instanceName);

    /// 根据 daemon 返回的实例列表，与内部状态机对比纠偏
    /// - daemon 中有但状态不是 Running → 纠正为 Running
    /// - daemon 中没有但状态不是 Unstarted → 重置为 Unstarted
    void syncStatesFromDaemon(const QJsonArray &instances);

    /// controller 哈希表：key=实例名, value=VpnController 实例
    QHash<QString, VpnController *> m_controllers;

    DaemonClient *m_client;
    DaemonApi *m_daemonApi;
    NetworkConfigRepository *m_repo;
    StatusMonitor *m_statusMonitor;
    NodeInfoModel *m_nodeInfoModel = nullptr;
    RuntimeLogModel *m_runtimeLogModel = nullptr;

    /// 心跳定时器：每 kHeartbeatIntervalMs 毫秒触发一次，向 daemon 轮询运行状态
    QTimer *m_heartbeatTimer;

    /// 当前 QML 选中的实例名
    QString m_activeInstanceName;

    /// 心跳进行中标志：上一轮心跳未完成时跳过本次，防止并发堆积
    bool m_heartbeatInFlight = false;

    /// 心跳间隔（毫秒）
    static constexpr int kHeartbeatIntervalMs = 3000;
};
