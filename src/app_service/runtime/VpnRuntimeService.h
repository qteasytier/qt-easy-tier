/**
 * @file VpnRuntimeService.h
 * @brief VPN 运行服务（应用服务层）
 *
 * 应用级 VPN runtime 协调器，统一管理：
 * - 所有网络配置实例的生命周期（本地 / 外部 controller 集合）
 * - 启停、状态查询、stopAll 收敛
 * - daemon 心跳轮询与实例真实状态同步
 * - 当前查看实例与运行状态展示模型（NodeInfoModel / RuntimeLogModel）
 *
 * 单实例生命周期状态机由 VpnController 承担，daemon 数据异步解析由
 * StatusMonitor 承担，本服务不复制这些逻辑，只负责跨实例协调与展示数据注入。
 */
#pragma once

#include "app_service/runtime/NodeInfoModel.h"
#include "app_service/runtime/RuntimeLogModel.h"
#include "core/config/ConfigRunState.h"
#include "core/service/DaemonClient.h"
#include "VpnController.h"

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

class DaemonApi;
class NetworkConfigRepository;
class StatusMonitor;
class QJsonArray;

/** @brief VPN 运行服务：应用级 runtime 协调器，管理实例生命周期、心跳同步与展示模型 */
class VpnRuntimeService : public QObject {
    Q_OBJECT

    /// 当前正在查看的实例名称（运行状态页数据源指向）
    Q_PROPERTY(QString activeInstanceName READ activeInstanceName
               WRITE setActiveInstanceName NOTIFY activeInstanceNameChanged FINAL)

    /// 节点信息模型（当前查看实例的节点列表）
    Q_PROPERTY(NodeInfoModel *nodeInfoModel READ nodeInfoModel CONSTANT)
    /// 运行时日志模型（当前查看实例的事件日志）
    Q_PROPERTY(RuntimeLogModel *runtimeLogModel READ runtimeLogModel CONSTANT)

public:
    /**
     * @brief 构造函数
     * @param client        daemon IPC 客户端（用于发送 run/delete/list 等 RPC 调用）
     * @param daemonApi     daemon API 封装
     * @param repo          配置仓库（启动时读取 TOML 配置）
     * @param statusMonitor 状态监视器（接收 collect_network_infos 结果并异步解析）
     * @param parent        父对象
     */
    explicit VpnRuntimeService(DaemonClient *client,
                               DaemonApi *daemonApi,
                               NetworkConfigRepository *repo,
                               StatusMonitor *statusMonitor,
                               QObject *parent = nullptr);

    /// 请求启动指定配置（如果未运行则创建 controller 并进入 Starting 状态）
    void startConfig(const QString &instanceName);

    /// 请求停止指定配置（仅 Running 状态下有效，进入 Stopping 状态）
    void stopConfig(const QString &instanceName);

    /// 请求停止所有正在运行的实例（危险操作前置清理），全部收敛后发射 allStopped
    void stopAll();

    /// 查询指定配置的当前状态（ConfigRunState 枚举整数值）
    int configState(const QString &instanceName) const;

    /// 查询指定配置是否正在运行中
    bool isRunning(const QString &instanceName) const;

    /// 导出当前选中实例的运行日志到本地文件（供 QML 直接调用，须为 Q_INVOKABLE）
    Q_INVOKABLE bool exportLog(const QString &filePath);

    /// 设置运行状态页是否隐藏公共服务器节点（作用于节点信息模型）
    void setHideServerNodes(bool value);

    // ---- QML 属性访问器 ----

    /// 获取当前查看的实例名
    QString activeInstanceName() const;
    /// 设置当前查看的实例名，并刷新节点与日志展示模型
    void setActiveInstanceName(const QString &name);
    /// 获取节点信息模型（当前查看实例的节点数据）
    NodeInfoModel *nodeInfoModel() const;
    /// 获取运行时日志模型（当前查看实例的事件日志）
    RuntimeLogModel *runtimeLogModel() const;

    /// 获取指定实例的节点信息列表（当前缓存，供展示模型填充）
    QVariantList nodeInfosFor(const QString &instanceName) const;
    /// 获取指定实例的运行时日志列表（当前缓存，供展示模型填充）
    QVariantList logEntriesFor(const QString &instanceName) const;
    /// 获取当前全部外部实例名列表（daemon 中存在但本地配置列表中不存在的运行中实例）
    QStringList externalInstances() const;

public slots:
    /// 清理指定配置的 controller 资源（由外部在删除配置后调用）
    void cleanupController(const QString &instanceName);

    /// 同步本地 controller（配置创建/导入成功后调用，保持与数据库配置集合一致）
    void ensureLocalController(const QString &instanceName);

    /// StatusMonitor 异步解析完成后回调：缓存解析结果并刷新当前查看实例的展示模型
    void onInstanceInfoParsed(const QString &instName,
                              const QVariantList &nodeInfos,
                              const QVariantList &logEntries);

signals:
    /// 通知 UI 层：某配置的状态已变更
    void configStateChanged(const QString &instanceName, ConfigRunState state);

    /// 通知 UI 层：某配置的停止操作失败（daemon 返回错误或超时）
    void stopFailed(const QString &instanceName, const QString &error);

    /// 通知 UI 层：stopAll() 已收敛完成，success 为 false 表示有实例停止失败或超时
    void allStopped(bool success);

    /// 通知 UI 层：当前查看实例名已变更
    void activeInstanceNameChanged();

    /// 通知 UI 层：外部实例集合已变化（新增/移除外部运行中实例）
    void externalInstancesChanged(const QStringList &instanceNames);

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
    /// 根据当前查看实例的 controller 缓存刷新两个展示模型
    void refreshModels();

    /// 统一查找 controller：本地表优先，外部表兜底
    VpnController *findController(const QString &instanceName) const;

    /// 获取或懒创建一个本地配置的 VpnController 实例（本地表）
    VpnController *getOrCreateLocal(const QString &instanceName);

    /// 创建外部临时 controller（外部表，daemon 存在而本地无对应配置的实例）
    VpnController *createExternal(const QString &instanceName);

    /// 删除本地配置的 controller（配置删除路径）
    void removeLocalController(const QString &instanceName);

    /// 删除外部临时 controller 并通知上层外部实例集合变化
    void removeExternal(const QString &instanceName);

    /// 连接 controller 信号到本服务（状态转发、stopAll 收敛追踪）
    void wireController(VpnController *ctrl);

    /// 检查 stopAll 是否全部收敛，是则停止超时定时器并发射 allStopped
    void tryFinishStopAll();

    /// 根据 daemon 返回的实例列表，与内部状态机对比纠偏
    void syncStatesFromDaemon(const QJsonArray &instances);

    /// 本地配置 controller 哈希表：key=实例名, value=VpnController 实例
    QHash<QString, VpnController *> m_localControllers;

    /// 外部临时 controller 哈希表：daemon 中存在但本地配置列表中不存在的运行中实例
    QHash<QString, VpnController *> m_externalControllers;

    DaemonClient *m_client;
    DaemonApi *m_daemonApi;
    NetworkConfigRepository *m_repo;
    StatusMonitor *m_statusMonitor;

    /// 心跳定时器：每 kHeartbeatIntervalMs 毫秒触发一次，向 daemon 轮询运行状态
    QTimer *m_heartbeatTimer;

    /// stopAll 安全超时定时器（仅停止流程运行时生效）
    QTimer *m_stopAllTimer;

    /// 节点信息展示模型（本服务所有）
    NodeInfoModel *m_nodeInfoModel = nullptr;
    /// 运行时日志展示模型（本服务所有）
    RuntimeLogModel *m_runtimeLogModel = nullptr;

    /// 当前查看的实例名（运行状态页数据源指向）
    QString m_activeInstanceName;

    /// 心跳进行中标志：上一轮心跳未完成时跳过本次，防止并发堆积
    bool m_heartbeatInFlight = false;

    /// 已发停止请求的实例集合（stopAll 使用，避免对 Starting→Running 的实例重复发停止）
    QSet<QString> m_stopAllStopIssued;

    /// 仍在等待停止收敛的实例集合（stopAll 使用）
    QSet<QString> m_stopAllPending;

    /// stopAll 是否已有实例停止失败（任一失败则整体按失败结束）
    bool m_stopAllFailed = false;

    /// 心跳间隔（毫秒）
    static constexpr int kHeartbeatIntervalMs = 3000;

    /// stopAll 安全超时（毫秒）
    static constexpr int kStopAllTimeoutMs = 30000;
};
