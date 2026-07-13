/**
 * @file VpnController.cpp
 * @brief VpnController 实现
 *
 * 实现单个网络配置的启动/停止状态机：
 * - start()：从 repo 加载配置 → 转 TOML → Base64 编码 → 通过 IPC 发送 run_network_instance
 * - stop()：通过 IPC 发送 delete_network_instance
 * - 异步等待 IPC 响应，成功则切换状态，失败则回退
 */
#include "VpnController.h"
#include "core/application/config/ConfigPayloadBuilder.h"
#include "core/config/NetworkConfToml.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include "core/util/LogHelper.h"
#include <QFutureWatcher>
#include <QJsonObject>
#include <QJsonArray>

namespace {
constexpr int kMaxRuntimeLogEntries = 200;

QString runtimeLogTimestamp(const QVariant &entry)
{
    const QVariantMap map = entry.toMap();
    const QString raw = map.value(QStringLiteral("rawTimestamp")).toString();
    return raw.isEmpty() ? map.value(QStringLiteral("timestamp")).toString() : raw;
}
}

/// 将 State 枚举值转为可读字符串（用于日志输出）
static const char *stateName(VpnController::State s)
{
    switch (s) {
    case VpnController::State::Unstarted: return "Unstarted";
    case VpnController::State::Starting:  return "Starting";
    case VpnController::State::Running:   return "Running";
    case VpnController::State::Stopping:  return "Stopping";
    }
    return "?";
}

// ==================== 构造 ====================

VpnController::VpnController(const QString &instanceName, DaemonApi *daemonApi,
                             NetworkConfigRepository *repo, QObject *parent)
    : QObject(parent)
    , m_instanceName(instanceName)
    , m_daemonApi(daemonApi)
    , m_repo(repo)
{
}

// ==================== 启动流程 ====================

void VpnController::start()
{
    // 仅 Unstarted 状态允许启动，防止重复启动或在不合适的状态操作
    if (m_state != State::Unstarted) {
        LogHelper::logInfo(QStringLiteral("[%1] start 忽略: 当前状态=%2").arg(m_instanceName).arg(stateName(m_state)), "VpnController");
        return;
    }

    LogHelper::logInfo(QStringLiteral("[%1] 发起启动请求 ...").arg(m_instanceName), "VpnController");

    // 步骤 1：进入 Starting 状态，UI 显示"启动中"
    setState(State::Starting);

    // 步骤 2：从仓库加载配置
    const auto loaded = m_repo->load(m_instanceName);
    if (!loaded.has_value()) {
        LogHelper::logWarning(QStringLiteral("[%1] 启动失败: 仓库中找不到该配置").arg(m_instanceName), "VpnController");
        setState(State::Unstarted);
        return;
    }

    // 步骤 3：构造 daemon IPC 参数（运行时 TOML 的 Base64 格式）
    const QJsonObject params = ConfigPayloadBuilder::daemonConfigPayload(loaded.value());

    // 步骤 4：异步发起 run_network_instance IPC 调用
    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        // 先调度删除 watcher，防止在回调中多次触发
        watcher->deleteLater();

        // 校验当前状态仍为 Starting（可能在等待期间被 reset 了）
        if (m_state != State::Starting)
            return;

        try {
            // 检查 Future 是否成功完成（异常则进入 catch）
            watcher->result();
            LogHelper::logInfo(QStringLiteral("[%1] daemon 启动成功 → Running").arg(m_instanceName), "VpnController");
            setState(State::Running);
        } catch (...) {
            LogHelper::logWarning(QStringLiteral("[%1] daemon 启动失败 → Unstarted").arg(m_instanceName), "VpnController");
            setState(State::Unstarted);
        }
    });

    watcher->setFuture(m_daemonApi->runNetworkInstance(params));
}

// ==================== 停止流程 ====================

void VpnController::stop()
{
    // 仅 Running 状态允许停止
    if (m_state != State::Running) {
        LogHelper::logInfo(QStringLiteral("[%1] stop 忽略: 当前状态=%2").arg(m_instanceName).arg(stateName(m_state)), "VpnController");
        return;
    }

    LogHelper::logInfo(QStringLiteral("[%1] 发起停止请求 ...").arg(m_instanceName), "VpnController");

    // 步骤 1：进入 Stopping 状态，UI 显示"停止中"
    setState(State::Stopping);

    // 步骤 2：异步发起 IPC 调用
    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();

        // 校验当前状态仍为 Stopping
        if (m_state != State::Stopping)
            return;

        try {
            watcher->result();
            LogHelper::logInfo(QStringLiteral("[%1] daemon 停止完成 → Unstarted").arg(m_instanceName), "VpnController");
            setState(State::Unstarted);
        } catch (...) {
            // 停止失败时回退到 Running 状态，通知 UI 显示错误
            LogHelper::logWarning(QStringLiteral("[%1] daemon 停止失败，回退 → Running").arg(m_instanceName), "VpnController");
            setState(State::Running);
            emit stopFailed(m_instanceName, QStringLiteral("停止失败，daemon 返回错误"));
        }
    });

    watcher->setFuture(m_daemonApi->deleteNetworkInstance(m_instanceName));
}

// ==================== 强制重置 ====================

void VpnController::reset()
{
    // 已经是 Unstarted 则无需操作
    if (m_state == State::Unstarted)
        return;
    LogHelper::logInfo(QStringLiteral("[%1] 强制 reset: %2 → Unstarted").arg(m_instanceName).arg(stateName(m_state)), "VpnController");
    clearRunningStatus();
    setState(State::Unstarted);
}

// ==================== 状态管理 ====================

void VpnController::setState(State state)
{
    if (m_state == state)
        return;
    LogHelper::logInfo(QStringLiteral("[%1] 状态切换: %2 → %3").arg(m_instanceName).arg(stateName(m_state)).arg(stateName(state)), "VpnController");
    m_state = state;

    // 进入 Unstarted 状态时清空运行缓存，确保 UI 不显示过期数据
    if (state == State::Unstarted)
        m_runningStatus = RunningStatus{};

    emit stateChanged(m_instanceName, m_state);
}

VpnController::State VpnController::state() const
{
    return m_state;
}

ConfigRunState VpnController::runState() const
{
    switch (m_state) {
    case State::Unstarted:
        return ConfigRunState::Stopped;
    case State::Starting:
        return ConfigRunState::Starting;
    case State::Running:
        return ConfigRunState::Running;
    case State::Stopping:
        return ConfigRunState::Stopping;
    }
    return ConfigRunState::Error;
}

QString VpnController::instanceName() const
{
    return m_instanceName;
}

// ==================== 运行状态缓存 ====================

void VpnController::setRunningStatus(const QVariantList &nodeInfos, const QVariantList &logEntries)
{
    m_runningStatus.nodeInfos = nodeInfos;

    // 首次注入：直接设置日志列表
    if (m_runningStatus.logEntries.isEmpty()) {
        m_runningStatus.logEntries = logEntries;
    } else {
        // 后续注入：只追加时间戳更新的条目，避免重复
        const QString latestTimestamp = runtimeLogTimestamp(m_runningStatus.logEntries.last());
        for (const QVariant &entry : logEntries) {
            if (runtimeLogTimestamp(entry) > latestTimestamp)
                m_runningStatus.logEntries.append(entry);
        }
    }

    // 日志条目数超过上限时，只保留最新的 kMaxRuntimeLogEntries 条
    if (m_runningStatus.logEntries.size() > kMaxRuntimeLogEntries) {
        m_runningStatus.logEntries = m_runningStatus.logEntries.mid(
            m_runningStatus.logEntries.size() - kMaxRuntimeLogEntries);
    }
}

void VpnController::clearRunningStatus()
{
    m_runningStatus = RunningStatus{};
}

QVariantList VpnController::nodeInfos() const
{
    return m_runningStatus.nodeInfos;
}

QVariantList VpnController::logEntries() const
{
    return m_runningStatus.logEntries;
}

bool VpnController::hasRunningStatus() const
{
    return !m_runningStatus.nodeInfos.isEmpty() || !m_runningStatus.logEntries.isEmpty();
}
