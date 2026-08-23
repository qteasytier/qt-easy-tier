/**
 * @file VpnManager.h
 * @brief VPN 全局管理器
 *
 * 管理所有网络配置的生命周期，为每个配置维护独立的 VpnController 状态机。
 * 内置心跳定时器，通过 daemon list_instances 同步真实运行状态。
 *
 * ## 架构职责
 * - 基础服务层：只负责实例生命周期与状态同步，不接触任何 UI 类型
 * - 运行状态展示数据（节点信息、运行时日志）通过 instanceInfoUpdated 信号
 *   向上层暴露，由应用服务层 VpnRuntimeService 填充展示模型
 * - activeInstanceName 表示"当前查看的实例"，运行状态页的数据源指向该实例
 *
 * ## 线程模型
 * - 所有操作均在主线程（QML 引擎线程）
 * - DaemonClient 的 IPC 调用通过 QFutureWatcher 异步等待，回调在主线程
 * - StatusMonitor 的异步解析在 QtConcurrent 线程池执行，通过 QueuedConnection 回主线程
 *
 * @see VpnController
 * @see DaemonClient
 * @see StatusMonitor
 * @see VpnRuntimeService
 */
#pragma once
#include <QObject>
#include <QHash>
#include <QTimer>
#include <QVariantList>
#include "core/config/ConfigRunState.h"
#include "core/service/DaemonClient.h"
#include "VpnController.h"

class DaemonApi;
class NetworkConfigRepository;
class StatusMonitor;
class QJsonArray;

/** @brief VPN 全局管理器，负责所有网络配置的生命周期管理与状态同步 */
class VpnManager : public QObject {
    Q_OBJECT

    /// 属性：当前正在查看的实例名称（运行状态页的数据源指向）
    Q_PROPERTY(QString activeInstanceName READ activeInstanceName
               WRITE setActiveInstanceName NOTIFY activeInstanceNameChanged FINAL)

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

    /// 请求停止所有正在运行的实例（危险操作前置清理），全部收敛后发射 allStopped
    Q_INVOKABLE void stopAll();

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
    /// 设置当前 QML 选中的实例名（仅记录，展示数据的刷新由 VpnRuntimeService 完成）
    void setActiveInstanceName(const QString &name);

    /// 获取指定实例的节点信息列表（当前缓存，供应用服务层填充展示模型）
    QVariantList nodeInfosFor(const QString &instanceName) const;
    /// 获取指定实例的运行时日志列表（当前缓存，供应用服务层填充展示模型）
    QVariantList logEntriesFor(const QString &instanceName) const;

signals:
    /// 通知上层：某配置的状态已变更
    void configStateChanged(const QString &instanceName, ConfigRunState state);

    /// 通知上层：某配置的停止操作失败（daemon 返回错误或超时）
    void stopFailed(const QString &instanceName, const QString &error);

    /// 通知上层：stopAll() 已收敛完成，success 为 false 表示有实例停止失败或超时
    void allStopped(bool success);

    /// 通知上层：当前选中实例名已变更
    void activeInstanceNameChanged();

    /**
     * @brief 通知上层：实例的运行状态信息已更新
     *
     * StatusMonitor 解析完成或心跳刷新后发射，携带该实例最新的节点信息与日志。
     * 展示模型由应用服务层（VpnRuntimeService）根据此信号填充。
     * @param instName   实例名称
     * @param nodeInfos  节点信息列表
     * @param logEntries 事件日志列表
     */
    void instanceInfoUpdated(const QString &instName,
                             const QVariantList &nodeInfos,
                             const QVariantList &logEntries);

public slots:
    /// StatusMonitor 异步解析完成后回调：将解析结果缓存到对应 VpnController 并通知上层
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

    /// stopAll 收敛追踪：某个等待实例状态变化时更新收敛集合
    void onStopAllStateChanged(const QString &instanceName, VpnController::State state);

    /// stopAll 收敛追踪：某个等待实例停止失败（回到 Running）时按失败收敛
    void onStopAllStopFailed(const QString &instanceName);

    /// stopAll 安全超时：超过时限仍未收敛时按失败结束
    void onStopAllTimeout();

private:
    /// 获取或懒创建一个 VpnController 实例
    VpnController *getOrCreate(const QString &instanceName);

    /// 检查 stopAll 是否全部收敛，是则停止超时定时器并发射 allStopped
    void tryFinishStopAll();

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

    /// 心跳定时器：每 kHeartbeatIntervalMs 毫秒触发一次，向 daemon 轮询运行状态
    QTimer *m_heartbeatTimer;

    /// 当前 QML 选中的实例名（运行状态页数据源指向，展示刷新由上层服务完成）
    QString m_activeInstanceName;

    /// 心跳进行中标志：上一轮心跳未完成时跳过本次，防止并发堆积
    bool m_heartbeatInFlight = false;

    /// 已发停止请求的实例集合（stopAll 使用，避免对 Starting→Running 的实例重复发停止）
    QSet<QString> m_stopAllStopIssued;

    /// 仍在等待停止收敛的实例集合（stopAll 使用）
    QSet<QString> m_stopAllPending;

    /// stopAll 是否已有实例停止失败（任一失败则整体按失败结束）
    bool m_stopAllFailed = false;

    /// stopAll 安全超时定时器（防止 daemon 无响应时流程挂死）
    QTimer *m_stopAllTimer = nullptr;

    /// 心跳间隔（毫秒）
    static constexpr int kHeartbeatIntervalMs = 3000;

    /// stopAll 安全超时（毫秒）
    static constexpr int kStopAllTimeoutMs = 30000;
};
