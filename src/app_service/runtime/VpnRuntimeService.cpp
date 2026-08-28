/**
 * @file VpnRuntimeService.cpp
 * @brief VpnRuntimeService 实现
 *
 * 应用级 VPN runtime 协调器，实现：
 * - 配置启停的入口方法（startConfig / stopConfig）
 * - 心跳驱动的状态同步（轮询 daemon 运行实例列表）
 * - stopAll 收敛、失败处理与安全超时
 * - controller 生命周期管理（懒创建 / 移除）
 * - 当前查看实例与运行状态展示模型（NodeInfoModel / RuntimeLogModel）填充
 *
 * 单实例状态机由 VpnController 承担，daemon 数据异步解析由 StatusMonitor 承担。
 */
#include "VpnRuntimeService.h"
#include "StatusMonitor.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include "core/log/LogHelper.h"
#include <QFutureWatcher>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QUrl>

// ==================== 构造与初始化 ====================

VpnRuntimeService::VpnRuntimeService(DaemonClient *client, DaemonApi *daemonApi,
                                     NetworkConfigRepository *repo,
                                     StatusMonitor *statusMonitor, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_daemonApi(daemonApi)
    , m_repo(repo)
    , m_statusMonitor(statusMonitor)
{
    // 创建运行状态展示模型，本服务持有所有权（父对象为本服务）
    m_nodeInfoModel = new NodeInfoModel(this);
    m_runtimeLogModel = new RuntimeLogModel(this);

    // 创建心跳定时器，每 3 秒触发一次
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(kHeartbeatIntervalMs);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &VpnRuntimeService::onHeartbeat);

    // 创建 stopAll 安全超时定时器（仅停止流程运行时生效）
    m_stopAllTimer = new QTimer(this);
    m_stopAllTimer->setInterval(kStopAllTimeoutMs);
    m_stopAllTimer->setSingleShot(true);
    connect(m_stopAllTimer, &QTimer::timeout, this, &VpnRuntimeService::onStopAllTimeout);

    // 监听 daemon 连接状态变更
    connect(m_client, &DaemonClient::connectionStateChanged,
            this, &VpnRuntimeService::onDaemonConnectionChanged);

    // 监听 StatusMonitor 解析完成事件
    connect(m_statusMonitor, &StatusMonitor::instanceInfoParsed,
            this, &VpnRuntimeService::onInstanceInfoParsed);

    // 为每个已保存的配置预创建本地 controller，确保心跳纠偏时有对应的状态机
    const auto allConfigs = m_repo->loadAll();
    for (const auto &cfg : allConfigs) {
        getOrCreateLocal(cfg.instanceName());
    }

    LogHelper::logInfo(QStringLiteral("VpnRuntimeService 初始化完成，心跳间隔=%1ms，预创建 controller=%2个")
            .arg(kHeartbeatIntervalMs).arg(allConfigs.size()), "VpnRuntimeService");

    // 如果 daemon 已连接，立即启动心跳；否则等待连接信号触发
    if (m_client->connectionState() == DaemonClient::ConnectionState::Connected) {
        m_heartbeatTimer->start();
        LogHelper::logInfo("daemon 已连接，心跳已启动", "VpnRuntimeService");
    } else {
        LogHelper::logInfo("daemon 未连接，等待连接后启动心跳", "VpnRuntimeService");
    }

    // 初始填充：若装配时已有查看实例（通常为空），同步一次展示数据
    refreshModels();
}

// ==================== 配置启停 ====================

void VpnRuntimeService::startConfig(const QString &instanceName)
{
    LogHelper::logInfo(QStringLiteral("收到启动请求: %1").arg(instanceName), "VpnRuntimeService");

    // 懒创建本地 controller
    auto *ctrl = getOrCreateLocal(instanceName);

    // 只允许从未启动状态启动
    if (ctrl->state() == VpnController::State::Running) {
        LogHelper::logWarning(QStringLiteral("启动忽略: %1 已在运行中").arg(instanceName), "VpnRuntimeService");
        return;
    }
    if (ctrl->state() == VpnController::State::Starting) {
        LogHelper::logWarning(QStringLiteral("启动忽略: %1 已在启动中").arg(instanceName), "VpnRuntimeService");
        return;
    }

    // VpnController::start() 内部会进一步检查状态（Stopping / Unstarted 状态也会被拦截）
    ctrl->start();
}

void VpnRuntimeService::stopConfig(const QString &instanceName)
{
    LogHelper::logInfo(QStringLiteral("收到停止请求: %1").arg(instanceName), "VpnRuntimeService");

    auto *ctrl = findController(instanceName);
    if (!ctrl) {
        LogHelper::logInfo(QStringLiteral("停止忽略: %1 无对应的 controller").arg(instanceName), "VpnRuntimeService");
        return;
    }

    if (ctrl->state() != VpnController::State::Running) {
        LogHelper::logWarning(QStringLiteral("停止忽略: %1 当前未在运行中").arg(instanceName), "VpnRuntimeService");
        return;
    }
    ctrl->stop();
}

void VpnRuntimeService::stopAll()
{
    LogHelper::logInfo("收到停止所有实例请求", "VpnRuntimeService");

    // 重置收敛追踪状态
    m_stopAllPending.clear();
    m_stopAllStopIssued.clear();
    m_stopAllFailed = false;

    // 收集所有非 Unstarted 的实例（Starting / Running / Stopping 均需等待收敛）
    // 本地配置与外部临时实例都要停止
    for (auto it = m_localControllers.begin(); it != m_localControllers.end(); ++it) {
        if (it.value()->state() != VpnController::State::Unstarted)
            m_stopAllPending.insert(it.key());
    }
    for (auto it = m_externalControllers.begin(); it != m_externalControllers.end(); ++it) {
        if (it.value()->state() != VpnController::State::Unstarted)
            m_stopAllPending.insert(it.key());
    }

    // 没有需要停止的实例，直接按成功收敛
    if (m_stopAllPending.isEmpty()) {
        LogHelper::logInfo("停止所有实例: 无运行中的实例", "VpnRuntimeService");
        emit allStopped(true);
        return;
    }

    // 启动安全超时，防止 daemon 无响应导致流程挂死
    m_stopAllTimer->start();

    // 对 Running 状态的实例立即发送停止请求
    // Starting 状态的实例等待其收敛到 Running 后再补发（见 onStopAllStateChanged）
    for (const QString &name : std::as_const(m_stopAllPending)) {
        auto *ctrl = findController(name);
        if (ctrl && ctrl->state() == VpnController::State::Running) {
            m_stopAllStopIssued.insert(name);
            ctrl->stop();
        }
    }
}

void VpnRuntimeService::onStopAllStateChanged(const QString &instanceName, VpnController::State state)
{
    if (!m_stopAllPending.contains(instanceName))
        return;

    // 实例已收敛到 Unstarted，视为该实例停止成功
    if (state == VpnController::State::Unstarted) {
        m_stopAllPending.remove(instanceName);
        m_stopAllStopIssued.remove(instanceName);
        tryFinishStopAll();
        return;
    }

    // Starting 收敛到 Running 后补发停止请求
    if (state == VpnController::State::Running && !m_stopAllStopIssued.contains(instanceName)) {
        m_stopAllStopIssued.insert(instanceName);
        if (auto *ctrl = findController(instanceName))
            ctrl->stop();
    }
}

void VpnRuntimeService::onStopAllStopFailed(const QString &instanceName)
{
    if (!m_stopAllPending.contains(instanceName))
        return;

    // 停止失败：实例回到 Running 状态，无法继续等待，标记整体失败后收敛
    LogHelper::logWarning(QStringLiteral("停止所有实例: %1 停止失败").arg(instanceName), "VpnRuntimeService");
    m_stopAllFailed = true;
    m_stopAllPending.remove(instanceName);
    m_stopAllStopIssued.remove(instanceName);
    tryFinishStopAll();
}

void VpnRuntimeService::onStopAllTimeout()
{
    LogHelper::logWarning("停止所有实例超时，按失败结束", "VpnRuntimeService");
    m_stopAllPending.clear();
    m_stopAllStopIssued.clear();
    m_stopAllFailed = true;
    tryFinishStopAll();
}

void VpnRuntimeService::tryFinishStopAll()
{
    if (!m_stopAllPending.isEmpty())
        return;

    m_stopAllTimer->stop();
    const bool success = !m_stopAllFailed;
    m_stopAllFailed = false;
    LogHelper::logInfo(QStringLiteral("停止所有实例完成: success=%1").arg(success), "VpnRuntimeService");
    emit allStopped(success);
}

// ==================== 状态查询 ====================

int VpnRuntimeService::configState(const QString &instanceName) const
{
    auto *ctrl = findController(instanceName);
    if (!ctrl)
        return static_cast<int>(ConfigRunState::Stopped);
    return static_cast<int>(ctrl->runState());
}

bool VpnRuntimeService::isRunning(const QString &instanceName) const
{
    auto *ctrl = findController(instanceName);
    return ctrl && ctrl->state() == VpnController::State::Running;
}

// ==================== 日志导出 ====================

bool VpnRuntimeService::exportLog(const QString &filePath)
{
    auto *ctrl = findController(m_activeInstanceName);
    if (!ctrl || !ctrl->hasRunningStatus())
        return false;

    const QVariantList entries = ctrl->logEntries();

    // 拼接日志文本
    QString text;
    for (const QVariant &entry : entries) {
        const QVariantMap map = entry.toMap();
        text += QStringLiteral("[%1] %2\n")
                    .arg(map.value(QStringLiteral("timestamp")).toString(),
                         map.value(QStringLiteral("message")).toString());
    }

    // 通过 QUrl 解析 file:// 等 URL 为本地路径
    QFile file(QUrl(filePath).toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    file.write(text.toUtf8());
    file.close();
    return true;
}

// ==================== 展示模型控制 ====================

void VpnRuntimeService::setHideServerNodes(bool value)
{
    if (m_nodeInfoModel)
        m_nodeInfoModel->setHideServerNodes(value);
}

// ==================== Controller 生命周期 ====================

void VpnRuntimeService::cleanupController(const QString &instanceName)
{
    removeLocalController(instanceName);
}

void VpnRuntimeService::removeLocalController(const QString &instanceName)
{
    auto it = m_localControllers.find(instanceName);
    if (it == m_localControllers.end())
        return;

    // 如果当前正查看的实例被删除，清空查看状态并刷新展示模型
    if (it.value()->instanceName() == m_activeInstanceName) {
        m_activeInstanceName.clear();
        refreshModels();
        emit activeInstanceNameChanged();
    }

    // 删除 controller 对象（同时删除其子 QObject，包括正在执行的 QFutureWatcher）
    delete it.value();
    m_localControllers.erase(it);
}

// ==================== QML 属性 ====================

QString VpnRuntimeService::activeInstanceName() const
{
    return m_activeInstanceName;
}

void VpnRuntimeService::setActiveInstanceName(const QString &name)
{
    if (m_activeInstanceName == name)
        return;
    m_activeInstanceName = name;
    // 刷新展示模型并通知 UI
    refreshModels();
    emit activeInstanceNameChanged();
}

NodeInfoModel *VpnRuntimeService::nodeInfoModel() const
{
    return m_nodeInfoModel;
}

RuntimeLogModel *VpnRuntimeService::runtimeLogModel() const
{
    return m_runtimeLogModel;
}

QVariantList VpnRuntimeService::nodeInfosFor(const QString &instanceName) const
{
    auto *ctrl = findController(instanceName);
    return ctrl ? ctrl->nodeInfos() : QVariantList();
}

QVariantList VpnRuntimeService::logEntriesFor(const QString &instanceName) const
{
    auto *ctrl = findController(instanceName);
    return ctrl ? ctrl->logEntries() : QVariantList();
}

QStringList VpnRuntimeService::externalInstances() const
{
    // 返回当前全部外部实例名（顺序不保证，仅供诊断/测试使用）
    return m_externalControllers.keys();
}

// ==================== StatusMonitor 回调 ====================

void VpnRuntimeService::onInstanceInfoParsed(const QString &instName,
                                             const QVariantList &nodeInfos,
                                             const QVariantList &logEntries)
{
    auto *ctrl = findController(instName);
    if (!ctrl)
        return;

    // 将 StatusMonitor 异步解析的结果缓存到对应 controller（含日志去重合并）
    ctrl->setRunningStatus(nodeInfos, logEntries);

    // 若该实例是当前查看实例，直接刷新展示模型
    if (instName == m_activeInstanceName) {
        m_nodeInfoModel->setFromVariantList(ctrl->nodeInfos());
        m_runtimeLogModel->setFromVariantList(ctrl->logEntries());
    }
}

// ==================== 心跳机制 ====================

void VpnRuntimeService::onHeartbeat()
{
    // 上一轮心跳（list_instances + collect_network_infos）未完成时跳过
    if (m_heartbeatInFlight)
        return;

    m_heartbeatInFlight = true;

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        try {
            onGotInstList(watcher->result());
        } catch (...) {
            // 心跳请求失败的原因通常是 daemon 未连接或临时不可用
            m_heartbeatInFlight = false;
            LogHelper::logInfo("心跳请求失败 (daemon 可能未连接)", "VpnRuntimeService");
        }
    });

    watcher->setFuture(m_daemonApi->listInstances());
}

void VpnRuntimeService::onGotInstList(const QJsonObject &result)
{
    // 步骤 1：从返回的 JSON 中提取 instances 数组，与内部状态机对比纠偏
    const QJsonArray instances = result[QStringLiteral("instances")].toArray();
    syncStatesFromDaemon(instances);

    const int count = instances.size();

    // 如果没有运行中的实例，释放心跳锁，无需请求更详细的信息
    if (count == 0) {
        m_heartbeatInFlight = false;
        return;
    }

    // 步骤 2：发起 collect_network_infos 请求，获取节点信息和事件日志
    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        try {
            onGotNetworkInfos(watcher->result());
        } catch (...) {
            LogHelper::logInfo("collect_network_infos 请求失败", "VpnRuntimeService");
        }
        m_heartbeatInFlight = false;
    });

    watcher->setFuture(m_daemonApi->collectNetworkInfos(count));
}

void VpnRuntimeService::onGotNetworkInfos(const QJsonObject &result)
{
    // 将原始 JSON 交给 StatusMonitor，由它异步解码 Base64 并解析
    m_statusMonitor->processNetworkInfos(result);
}

// ==================== daemon 连接管理 ====================

void VpnRuntimeService::onDaemonConnectionChanged(DaemonClient::ConnectionState state)
{
    if (state == DaemonClient::ConnectionState::Connected) {
        LogHelper::logInfo("daemon 已连接，启动心跳", "VpnRuntimeService");
        m_heartbeatTimer->start();
        return;
    }

    // Connecting 是重连过程中的瞬态，不触发断连处理
    if (state == DaemonClient::ConnectionState::Connecting)
        return;

    LogHelper::logWarning("daemon 已断开，即将尝试重连", "VpnRuntimeService");

    // 停止心跳，避免在 daemon 不可用时继续发起无效请求
    m_heartbeatTimer->stop();
    m_heartbeatInFlight = false;

    // 使用 QTimer::singleShot(0) 延迟重置，而非直接重置：
    // 这样在 daemon 快速重连（断开→重连在同一个事件循环周期内）时，
    // 可以避免不必要的重置操作——回调中会重新检查连接状态
    QTimer::singleShot(0, this, [this]() {
        if (m_client->connectionState() == DaemonClient::ConnectionState::Connected)
            return;

        // daemon 确实断开了：本地 controller 强制重置为 Unstarted；
        // 外部临时 controller 的唯一信息来源是 daemon，全部移除并通知列表清空
        for (auto it = m_localControllers.begin(); it != m_localControllers.end(); ++it) {
            it.value()->reset();
        }
        if (!m_externalControllers.isEmpty()) {
            qDeleteAll(m_externalControllers);
            m_externalControllers.clear();
            emit externalInstancesChanged(QStringList{});
        }
    });
}

// ==================== 内部辅助 ====================

void VpnRuntimeService::refreshModels()
{
    // 从 controller 缓存中读取当前查看实例的节点与日志数据并填充模型
    const QString name = m_activeInstanceName;
    m_nodeInfoModel->setFromVariantList(nodeInfosFor(name));
    m_runtimeLogModel->setFromVariantList(logEntriesFor(name));
}

VpnController *VpnRuntimeService::findController(const QString &instanceName) const
{
    auto local = m_localControllers.constFind(instanceName);
    if (local != m_localControllers.constEnd())
        return local.value();
    auto external = m_externalControllers.constFind(instanceName);
    return external != m_externalControllers.constEnd() ? external.value() : nullptr;
}

VpnController *VpnRuntimeService::getOrCreateLocal(const QString &instanceName)
{
    auto it = m_localControllers.find(instanceName);
    if (it != m_localControllers.end())
        return it.value();

    LogHelper::logInfo(QStringLiteral("新建本地 VpnController: %1").arg(instanceName), "VpnRuntimeService");

    // 创建新的 controller 并连接信号到本服务，转发给上层
    auto *ctrl = new VpnController(instanceName, m_daemonApi, m_repo, this);
    wireController(ctrl);
    m_localControllers.insert(instanceName, ctrl);
    return ctrl;
}

VpnController *VpnRuntimeService::createExternal(const QString &instanceName)
{
    auto it = m_externalControllers.find(instanceName);
    if (it != m_externalControllers.end())
        return it.value();

    LogHelper::logInfo(QStringLiteral("新建外部临时 VpnController: %1").arg(instanceName), "VpnRuntimeService");

    auto *ctrl = new VpnController(instanceName, m_daemonApi, m_repo, this);
    wireController(ctrl);
    m_externalControllers.insert(instanceName, ctrl);
    return ctrl;
}

void VpnRuntimeService::wireController(VpnController *ctrl)
{
    connect(ctrl, &VpnController::stateChanged, this,
            [this](const QString &name, VpnController::State state) {
                if (auto *controller = findController(name))
                    emit configStateChanged(name, controller->runState());
                onStopAllStateChanged(name, state);
            });
    connect(ctrl, &VpnController::stopFailed, this,
            [this](const QString &name, const QString &error) {
                emit stopFailed(name, error);
                onStopAllStopFailed(name);
            });
}

void VpnRuntimeService::ensureLocalController(const QString &instanceName)
{
    // 若该名字此前是外部临时实例，先清除外部身份（本地配置接管该实例名）
    if (m_externalControllers.contains(instanceName))
        removeExternal(instanceName);

    getOrCreateLocal(instanceName);
}

void VpnRuntimeService::removeExternal(const QString &instanceName)
{
    auto it = m_externalControllers.find(instanceName);
    if (it == m_externalControllers.end())
        return;

    // 如果当前正查看的外部实例被移除，清空查看状态并刷新展示模型
    if (it.value()->instanceName() == m_activeInstanceName) {
        m_activeInstanceName.clear();
        refreshModels();
        emit activeInstanceNameChanged();
    }

    // 删除 controller 对象（同时删除其子 QObject，包括正在执行的 QFutureWatcher）
    delete it.value();
    m_externalControllers.erase(it);

    emit externalInstancesChanged(m_externalControllers.keys());
}

void VpnRuntimeService::syncStatesFromDaemon(const QJsonArray &instances)
{
    // 构建 daemon 中存在的实例名集合
    QSet<QString> daemonInstances;
    for (const auto &val : instances) {
        const QJsonObject obj = val.toObject();
        const QString key = obj[QStringLiteral("key")].toString();
        if (!key.isEmpty())
            daemonInstances.insert(key);
    }

    // 步骤 1：本地 controller 与 daemon 实际状态对比纠偏
    for (auto it = m_localControllers.begin(); it != m_localControllers.end(); ++it) {
        const QString &name = it.key();
        VpnController *ctrl = it.value();
        const bool inDaemon = daemonInstances.contains(name);

        if (inDaemon && ctrl->state() != VpnController::State::Running) {
            // 跳过正在停止中的 controller，避免覆盖 stop 操作的回调
            if (ctrl->state() == VpnController::State::Stopping)
                continue;
            LogHelper::logInfo(QStringLiteral("心跳发现 daemon 中存在 %1，纠正为 Running").arg(name), "VpnRuntimeService");
            ctrl->setState(VpnController::State::Running);
        } else if (!inDaemon && ctrl->state() != VpnController::State::Unstarted) {
            // daemon 中没有但状态不是 Unstarted → 该实例可能在 daemon 侧已崩溃或被其他途径停止
            LogHelper::logWarning(QStringLiteral("心跳发现 %1 在 daemon 中已消失，重置为 Unstarted").arg(name), "VpnRuntimeService");
            ctrl->reset();
        }
    }

    // 步骤 2：清理外部临时 controller
    // - daemon 不再返回该实例 → 外部实例已消失
    // - 本地 controller 表已出现同名实例（外部与本地 instance_name 一致）→ 心跳纠偏为本地
    QList<QString> vanishedExternal;
    for (const QString &name : m_externalControllers.keys()) {
        if (!daemonInstances.contains(name) || m_localControllers.contains(name))
            vanishedExternal.append(name);
    }
    for (const QString &name : std::as_const(vanishedExternal)) {
        if (m_localControllers.contains(name)) {
            LogHelper::logInfo(QStringLiteral("心跳发现外部实例 %1 已成为本地配置，移除外部临时 controller").arg(name), "VpnRuntimeService");
        } else {
            LogHelper::logInfo(QStringLiteral("心跳发现外部实例 %1 已从 daemon 消失，移除 controller").arg(name), "VpnRuntimeService");
        }
        removeExternal(name);
    }

    // 步骤 3：外部临时实例保持 Running（修复 daemon 断连重连后遗留的 Unstarted；
    // 跳过 Stopping，避免覆盖正在进行的停止流程）
    for (auto it = m_externalControllers.begin(); it != m_externalControllers.end(); ++it) {
        if (it.value()->state() == VpnController::State::Unstarted)
            it.value()->setState(VpnController::State::Running);
    }

    // 步骤 4：发现新的外部实例（daemon 中存在、本地无 controller 且外部表无记录）
    bool newExternalFound = false;
    for (const QString &name : std::as_const(daemonInstances)) {
        if (m_localControllers.contains(name) || m_externalControllers.contains(name))
            continue;
        LogHelper::logInfo(QStringLiteral("心跳发现外部实例 %1，创建外部临时 controller").arg(name), "VpnRuntimeService");
        auto *ctrl = createExternal(name);
        ctrl->setState(VpnController::State::Running);
        newExternalFound = true;
    }

    // 步骤 5：新增外部实例后统一通知上层（删除路径由 removeExternal 各自发射）
    if (newExternalFound)
        emit externalInstancesChanged(m_externalControllers.keys());
}
