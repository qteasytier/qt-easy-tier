/**
 * @file VpnController.h
 * @brief 单个网络配置的生命周期状态机
 *
 * 每个 VpnController 实例对应一个 NetworkConf（以 instanceName 标识），
 * 管理其启动/停止的完整生命周期。
 *
 * ## 状态转换图
 *
 *   Unstarted ──start()──▶ Starting ──IPC ok──▶ Running
 *       ▲                      │                    │
 *       │       IPC err         │    stop()          │
 *       ◀──────────────────────┘                    ▼
 *       ▲                                     Stopping
 *       │                                         │
 *       ◀──────────── IPC ok/err ─────────────────┘
 *
 * - reset() 可以从任何非 Unstarted 状态强制回到 Unstarted（daemon 断线场景）
 * - setState() 是公开的，供 VpnManager 心跳同步时直接设置状态（跳过状态机校验）
 *
 * ## 异步操作
 *
 * start() / stop() 通过 DaemonClient 向 daemon 发送 IPC 请求，
 * 使用 QFutureWatcher 异步等待响应。响应通过 try/catch 判断：
 * - 成功：进入目标状态（Running / Unstarted）
 * - 失败：回退到之前的状态（stop 失败回到 Running 并发出 stopFailed 信号）
 *
 * ## RunningStatus 缓存
 *
 * RunningStatus 存储节点信息和事件日志，由 StatusMonitor 异步解析后
 * 通过 VpnManager 回调 setRunningStatus() 写入，供 QML 显示。
 */
#pragma once
#include "core/application/runtime/ConfigRunState.h"

#include <QObject>
#include <QVariantList>

class DaemonApi;
class NetworkConfigRepository;

/// @brief 运行状态快照：缓存当前运行实例的节点信息与事件日志
/// 由 StatusMonitor 解析后写入，VpnManager 通过 QML 属性暴露
struct RunningStatus {
    QVariantList nodeInfos;   ///< 节点信息列表（每个节点为 QVariantMap）
    QVariantList logEntries;  ///< 事件日志列表（每条为 {timestamp, message}）
};

/** @brief 单个网络配置的生命周期状态机，管理启停完整流程 */
class VpnController : public QObject {
    Q_OBJECT

    /// QML 属性：当前状态枚举值（0=Unstarted, 1=Starting, 2=Running, 3=Stopping）
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)

public:
    /**
     * @enum State
     * @brief 网络实例生命周期状态
     *
     * 状态转换规则：
     * - Unstarted → Starting：仅 start() 方法可以从该状态触发
     * - Starting  → Running：daemon run_network_instance IPC 成功返回
     * - Starting  → Unstarted：daemon IPC 失败或配置加载失败
     * - Running   → Stopping：仅 stop() 方法可以从该状态触发
     * - Stopping  → Unstarted：daemon delete_network_instance IPC 成功返回
     * - Stopping  → Running：daemon IPC 失败（回退，保持运行）
     * - 任意状态  → Unstarted：reset() 强制重置（daemon 断线场景）
     */
    enum class State {
        Unstarted, ///< 未启动（初始状态，或停止完成后的状态）
        Starting,  ///< 启动中（等待 daemon run_network_instance 响应）
        Running,   ///< 运行中（daemon 确认实例正在运行）
        Stopping,  ///< 停止中（等待 daemon delete_network_instance 响应）
    };
    Q_ENUM(State)

    /**
     * @brief 构造函数
     * @param instanceName 对应的配置 instance_name（唯一标识）
     * @param daemonApi    daemon API
     * @param repo         配置仓库（启动时读取 TOML 配置）
     * @param parent       父对象（为 VpnManager，由其管理生命周期）
     */
    VpnController(const QString &instanceName, DaemonApi *daemonApi,
                  NetworkConfigRepository *repo, QObject *parent = nullptr);

    /// 请求启动实例（仅 Unstarted 状态有效，进入 Starting 状态）
    /// 内部会从 repo 加载配置、转换为 TOML、Base64 编码后发送 IPC
    void start();

    /// 请求停止实例（仅 Running 状态有效，进入 Stopping 状态）
    /// 通过 daemon delete_network_instance IPC 发送停止请求
    void stop();

    /// 强制重置为 Unstarted（用于 daemon 断线等意外场景）
    /// 会清除运行状态缓存并重置状态机
    void reset();

    /// 公开 setState 供 VpnManager 心跳同步使用
    /// 注意：此方法不校验状态转换合法性，调用方需确保合理性
    void setState(State state);

    // 状态访问器
    /// 获取当前生命周期状态枚举值
    State state() const;
    /// 将内部 State 枚举映射为 QML 层使用的 ConfigRunState 运行状态
    ConfigRunState runState() const;
    /// 获取对应的配置实例名（唯一标识）
    QString instanceName() const;

    /// 缓存节点信息与事件日志（由 StatusMonitor 异步解析后回调写入）
    void setRunningStatus(const QVariantList &nodeInfos, const QVariantList &logEntries);

    /// 清空运行状态缓存（状态回退到 Unstarted 时自动调用）
    void clearRunningStatus();

    // RunningStatus 只读访问器
    /// 获取缓存的节点信息列表
    QVariantList nodeInfos() const;
    /// 获取缓存的事件日志列表
    QVariantList logEntries() const;
    /// 判断是否已有运行状态数据（节点信息或日志非空）
    bool hasRunningStatus() const;

signals:
    /// 状态变更通知（携带实例名和新状态，转发给 VpnManager → QML）
    void stateChanged(const QString &instanceName, VpnController::State state);

    /// 停止失败通知（携带实例名和错误描述，转发给 VpnManager → QML）
    void stopFailed(const QString &instanceName, const QString &error);

private:
    QString m_instanceName;               ///< 对应的配置实例名
    State m_state = State::Unstarted;     ///< 当前状态（初始为未启动）
    DaemonApi *m_daemonApi;               ///< daemon API
    NetworkConfigRepository *m_repo;      ///< 配置仓库（启动时读取 TOML）
    RunningStatus m_runningStatus;        ///< 运行状态缓存
};
