/**
 * @file VpnManager.cpp
 * @brief VpnManager 实现
 *
 * 实现 VPN 全局管理器的核心逻辑：
 * - 配置启停的入口方法（startConfig / stopConfig）
 * - 心跳驱动的状态同步（轮询 daemon 运行实例列表）
 * - QML 属性访问与日志导出
 * - controller 生命周期管理（懒创建 / 移除）
 */
#include "VpnManager.h"
#include "StatusMonitor.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include "core/util/LogHelper.h"
#include "core/viewmodel/runtime/NodeInfoModel.h"
#include "core/viewmodel/runtime/RuntimeLogModel.h"
#include <QFutureWatcher>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QTimer>
#include <QFile>
#include <QUrl>

// ==================== 构造与初始化 ====================

VpnManager::VpnManager(DaemonClient *client, DaemonApi *daemonApi, NetworkConfigRepository *repo,
                       StatusMonitor *statusMonitor, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_daemonApi(daemonApi)
    , m_repo(repo)
    , m_statusMonitor(statusMonitor)
{
    m_nodeInfoModel = new NodeInfoModel(this);
    m_runtimeLogModel = new RuntimeLogModel(this);

    // 创建心跳定时器，每 3 秒触发一次
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(kHeartbeatIntervalMs);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &VpnManager::onHeartbeat);

    // 监听 daemon 连接状态变更
    connect(m_client, &DaemonClient::connectionStateChanged,
            this, &VpnManager::onDaemonConnectionChanged);

    // 监听 StatusMonitor 解析完成事件
    connect(m_statusMonitor, &StatusMonitor::instanceInfoParsed,
            this, &VpnManager::onInstanceInfoParsed);

    // 为每个已保存的配置预创建 controller，确保心跳纠偏时有对应的状态机
    const auto allConfigs = m_repo->loadAll();
    for (const auto &cfg : allConfigs) {
        getOrCreate(cfg.instanceName());
    }

    LogHelper::logInfo(QStringLiteral("VpnManager 初始化完成，心跳间隔=%1ms，预创建 controller=%2个")
            .arg(kHeartbeatIntervalMs).arg(allConfigs.size()), "VpnManager");

    // 如果 daemon 已连接，立即启动心跳；否则等待连接信号触发
    if (m_client->connectionState() == DaemonClient::ConnectionState::Connected) {
        m_heartbeatTimer->start();
        LogHelper::logInfo("daemon 已连接，心跳已启动", "VpnManager");
    } else {
        LogHelper::logInfo("daemon 未连接，等待连接后启动心跳", "VpnManager");
    }
}

// ==================== 配置启停 ====================

void VpnManager::startConfig(const QString &instanceName)
{
    LogHelper::logInfo(QStringLiteral("收到启动请求: %1").arg(instanceName), "VpnManager");

    // 懒创建 controller
    auto *ctrl = getOrCreate(instanceName);

    // 只允许从未启动状态启动
    if (ctrl->state() == VpnController::State::Running) {
        LogHelper::logWarning(QStringLiteral("启动忽略: %1 已在运行中").arg(instanceName), "VpnManager");
        return;
    }
    if (ctrl->state() == VpnController::State::Starting) {
        LogHelper::logWarning(QStringLiteral("启动忽略: %1 已在启动中").arg(instanceName), "VpnManager");
        return;
    }

    // VpnController::start() 内部会进一步检查状态（Stopping / Unstarted 状态也会被拦截）
    ctrl->start();
}

void VpnManager::stopConfig(const QString &instanceName)
{
    LogHelper::logInfo(QStringLiteral("收到停止请求: %1").arg(instanceName), "VpnManager");

    auto it = m_controllers.find(instanceName);
    if (it == m_controllers.end()) {
        LogHelper::logInfo(QStringLiteral("停止忽略: %1 无对应的 controller").arg(instanceName), "VpnManager");
        return;
    }

    auto *ctrl = it.value();
    if (ctrl->state() != VpnController::State::Running) {
        LogHelper::logWarning(QStringLiteral("停止忽略: %1 当前未在运行中").arg(instanceName), "VpnManager");
        return;
    }
    ctrl->stop();
}

// ==================== 状态查询 ====================

int VpnManager::configState(const QString &instanceName) const
{
    auto it = m_controllers.find(instanceName);
    if (it == m_controllers.end())
        return static_cast<int>(VpnController::State::Unstarted);
    return static_cast<int>(it.value()->state());
}

bool VpnManager::isRunning(const QString &instanceName) const
{
    auto it = m_controllers.find(instanceName);
    if (it == m_controllers.end())
        return false;
    return it.value()->state() == VpnController::State::Running;
}

// ==================== 日志导出 ====================

bool VpnManager::exportLog(const QString &filePath)
{
    auto it = m_controllers.find(m_activeInstanceName);
    if (it == m_controllers.end() || !it.value()->hasRunningStatus())
        return false;

    const QVariantList entries = it.value()->logEntries();

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

// ==================== Controller 生命周期 ====================

void VpnManager::cleanupController(const QString &instanceName)
{
    removeController(instanceName);
}

void VpnManager::removeController(const QString &instanceName)
{
    auto it = m_controllers.find(instanceName);
    if (it == m_controllers.end())
        return;

    // 如果当前正选中的实例被删除，清空选中状态并通知 QML
    if (it.value()->instanceName() == m_activeInstanceName) {
        m_activeInstanceName.clear();
        m_nodeInfoModel->setFromVariantList({});
        m_runtimeLogModel->setFromVariantList({});
        emit activeInstanceNameChanged();
    }

    // 删除 controller 对象（同时删除其子 QObject，包括正在执行的 QFutureWatcher）
    delete it.value();
    m_controllers.erase(it);
}

// ==================== QML 属性 ====================

QString VpnManager::activeInstanceName() const
{
    return m_activeInstanceName;
}

void VpnManager::setActiveInstanceName(const QString &name)
{
    if (m_activeInstanceName == name)
        return;
    m_activeInstanceName = name;
    auto it = m_controllers.find(m_activeInstanceName);
    m_nodeInfoModel->setFromVariantList(it == m_controllers.end() ? QVariantList() : it.value()->nodeInfos());
    m_runtimeLogModel->setFromVariantList(it == m_controllers.end() ? QVariantList() : it.value()->logEntries());
    emit activeInstanceNameChanged();
}

NodeInfoModel *VpnManager::nodeInfoModel() const
{
    return m_nodeInfoModel;
}

RuntimeLogModel *VpnManager::runtimeLogModel() const
{
    return m_runtimeLogModel;
}

void VpnManager::setHideServerNodes(bool value)
{
    if (!m_nodeInfoModel)
        return;
    m_nodeInfoModel->setHideServerNodes(value);
}

// ==================== StatusMonitor 回调 ====================

void VpnManager::onInstanceInfoParsed(const QString &instName,
                                       const QVariantList &nodeInfos,
                                       const QVariantList &logEntries)
{
    auto it = m_controllers.find(instName);
    if (it == m_controllers.end())
        return;

    // 将 StatusMonitor 异步解析的结果缓存到对应 controller
    it.value()->setRunningStatus(nodeInfos, logEntries);

    // 如果当前 QML 正在查看该实例，刷新显示
    if (instName == m_activeInstanceName) {
        m_nodeInfoModel->setFromVariantList(nodeInfos);
        m_runtimeLogModel->setFromVariantList(it.value()->logEntries());
    }
}

// ==================== 心跳机制 ====================

void VpnManager::onHeartbeat()
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
            LogHelper::logInfo("心跳请求失败 (daemon 可能未连接)", "VpnManager");
        }
    });

    watcher->setFuture(m_daemonApi->listInstances());
}

void VpnManager::onGotInstList(const QJsonObject &result)
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
            LogHelper::logInfo("collect_network_infos 请求失败", "VpnManager");
        }
        m_heartbeatInFlight = false;
    });

    watcher->setFuture(m_daemonApi->collectNetworkInfos(count));
}

void VpnManager::onGotNetworkInfos(const QJsonObject &result)
{
    // 将原始 JSON 交给 StatusMonitor，由它异步解码 Base64 并解析
    m_statusMonitor->processNetworkInfos(result);
}

// ==================== daemon 连接管理 ====================

void VpnManager::onDaemonConnectionChanged(DaemonClient::ConnectionState state)
{
    if (state == DaemonClient::ConnectionState::Connected) {
        LogHelper::logInfo("daemon 已连接，启动心跳", "VpnManager");
        m_heartbeatTimer->start();
        return;
    }

    // Connecting 是重连过程中的瞬态，不触发断连处理
    if (state == DaemonClient::ConnectionState::Connecting)
        return;

    LogHelper::logWarning("daemon 已断开，即将尝试重连", "VpnManager");

    // 停止心跳，避免在 daemon 不可用时继续发起无效请求
    m_heartbeatTimer->stop();
    m_heartbeatInFlight = false;

    // 使用 QTimer::singleShot(0) 延迟重置，而非直接重置：
    // 这样在 daemon 快速重连（断开→重连在同一个事件循环周期内）时，
    // 可以避免不必要的重置操作——回调中会重新检查连接状态
    QTimer::singleShot(0, this, [this]() {
        if (m_client->connectionState() == DaemonClient::ConnectionState::Connected)
            return;

        // daemon 确实断开了，将所有运行中的 controller 强制重置为 Unstarted
        for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
            it.value()->reset();
        }
    });
}

// ==================== 内部辅助 ====================

VpnController *VpnManager::getOrCreate(const QString &instanceName)
{
    auto it = m_controllers.find(instanceName);
    if (it != m_controllers.end())
        return it.value();

    LogHelper::logInfo(QStringLiteral("新建 VpnController: %1").arg(instanceName), "VpnManager");

    // 创建新的 controller 并连接信号到 VpnManager，转发给 QML
    auto *ctrl = new VpnController(instanceName, m_daemonApi, m_repo, this);
    connect(ctrl, &VpnController::stateChanged, this,
            [this](const QString &name, VpnController::State state) {
                Q_UNUSED(state)
                if (auto *controller = m_controllers.value(name, nullptr))
                    emit configStateChanged(name, controller->runState());
            });
    connect(ctrl, &VpnController::stopFailed, this,
            [this](const QString &name, const QString &error) {
                emit stopFailed(name, error);
            });
    m_controllers.insert(instanceName, ctrl);
    return ctrl;
}

void VpnManager::syncStatesFromDaemon(const QJsonArray &instances)
{
    // 构建 daemon 中存在的实例名集合
    QSet<QString> daemonInstances;
    for (const auto &val : instances) {
        const QJsonObject obj = val.toObject();
        const QString key = obj[QStringLiteral("key")].toString();
        if (!key.isEmpty())
            daemonInstances.insert(key);
    }

    // 遍历所有本地 controller，与 daemon 实际状态对比纠偏
    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        const QString &name = it.key();
        VpnController *ctrl = it.value();
        const bool inDaemon = daemonInstances.contains(name);

        if (inDaemon && ctrl->state() != VpnController::State::Running) {
            // 跳过正在停止中的 controller，避免覆盖 stop 操作的回调
            if (ctrl->state() == VpnController::State::Stopping)
                continue;
            LogHelper::logInfo(QStringLiteral("心跳发现 daemon 中存在 %1，纠正为 Running").arg(name), "VpnManager");
            ctrl->setState(VpnController::State::Running);
        } else if (!inDaemon && ctrl->state() != VpnController::State::Unstarted) {
            // daemon 中没有但状态不是 Unstarted → 该实例可能在 daemon 侧已崩溃或被其他途径停止
            LogHelper::logWarning(QStringLiteral("心跳发现 %1 在 daemon 中已消失，重置为 Unstarted").arg(name), "VpnManager");
            ctrl->reset();
        }
    }
}
