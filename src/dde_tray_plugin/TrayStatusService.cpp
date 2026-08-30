/** @file TrayStatusService.cpp @brief 托盘状态监测服务实现 */
#include "TrayStatusService.h"

#include "config/ConfigPayloadBuilder.h"
#include "daemon_service/DaemonApi.h"
#include "sqlite_repository/DatabaseConnection.h"
#include "sqlite_repository/NetworkConfigRepository.h"

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

TrayStatusService::TrayStatusService(const QString &databasePath, QObject *parent)
    : QObject(parent)
{
    m_database = new DatabaseConnection(resolveDatabasePath(databasePath));
    if (m_database->open()) {
        m_repository = new NetworkConfigRepository(m_database->database());
        reloadLocalConfigs();
    }

    m_client = new DaemonClient(this);
    m_api = new DaemonApi(m_client, this);
    connect(m_client, &DaemonClient::connectionStateChanged,
            this, &TrayStatusService::onDaemonStateChanged);

    m_timer = new QTimer(this);
    m_timer->setInterval(3000);
    connect(m_timer, &QTimer::timeout, this, &TrayStatusService::onHeartbeat);
    setDaemonState(m_client->connectionState());
    m_client->connectToDaemon(QStringLiteral("qtet-daemon.sock"));
}

TrayStatusService::~TrayStatusService()
{
    delete m_repository;
    delete m_database;
}

QString TrayStatusService::resolveDatabasePath(const QString &explicitPath)
{
    if (!explicitPath.isEmpty())
        return explicitPath;
    // 插件运行在 trayplugin-loader 进程中，与主应用不同，未设置过应用标识；
    // 此处按主应用（src/main.cpp）的设置补齐，使 AppConfigLocation 指向同一数据库。
    if (auto *app = QCoreApplication::instance()) {
        app->setOrganizationName(QStringLiteral("qteasytier"));
        app->setApplicationName(QStringLiteral("QtEasyTier"));
    }
    return DatabaseConnection::defaultDatabasePath();
}

TrayStatusSnapshot TrayStatusService::snapshot() const { return m_snapshot; }

DaemonClient::ConnectionState TrayStatusService::daemonState() const
{
    return m_snapshot.daemonState;
}

void TrayStatusService::setDaemonState(DaemonClient::ConnectionState state)
{
    if (m_snapshot.daemonState == state)
        return;
    m_snapshot.daemonState = state;
    emit daemonStateChanged(state);
    emit snapshotChanged();
}

void TrayStatusService::onDaemonStateChanged(DaemonClient::ConnectionState state)
{
    setDaemonState(state);
    if (state == DaemonClient::ConnectionState::Connected) {
        m_timer->start();
        refresh();
    } else if (state == DaemonClient::ConnectionState::Disconnected) {
        m_timer->stop();
        m_requestInFlight = false;
        clearRuntimeState();
    }
}

void TrayStatusService::clearRuntimeState()
{
    // daemon 断开时清空实例列表，避免用户在未连接状态下误操作
    if (m_snapshot.instances.isEmpty())
        return;
    m_snapshot.instances.clear();
    emit snapshotChanged();
}

void TrayStatusService::reloadLocalConfigs()
{
    m_localDisplayNames.clear();
    if (!m_repository)
        return;
    for (const auto &config : m_repository->loadAll())
        m_localDisplayNames.insert(config.instanceName(), config.displayName);
}

void TrayStatusService::refresh()
{
    if (m_snapshot.daemonState != DaemonClient::ConnectionState::Connected
        || m_requestInFlight)
        return;
    queryInstances();
}

void TrayStatusService::onHeartbeat() { refresh(); }

void TrayStatusService::queryInstances()
{
    m_requestInFlight = true;
    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher] {
        QJsonObject result;
        try {
            result = watcher->result();
        } catch (...) {
            m_requestInFlight = false;
            watcher->deleteLater();
            emit snapshotChanged();
            return;
        }
        watcher->deleteLater();
        handleInstances(result);
    });
    watcher->setFuture(m_api->listInstances());
}

void TrayStatusService::handleInstances(const QJsonObject &result)
{
    reloadLocalConfigs();
    const QJsonArray instances = result.value(QStringLiteral("instances")).toArray();
    QSet<QString> runningNames;
    for (const auto &value : instances) {
        const QString name = value.toObject().value(QStringLiteral("key")).toString();
        if (!name.isEmpty())
            runningNames.insert(name);
    }

    QList<TrayInstanceStatus> next;
    for (const QString &name : std::as_const(runningNames)) {
        const bool local = m_localDisplayNames.contains(name);
        TrayInstanceStatus status;
        if (const auto *old = findInstance(name))
            status = *old;
        status.instanceName = name;
        status.displayName = local && !m_localDisplayNames.value(name).isEmpty()
            ? m_localDisplayNames.value(name) : name;
        status.local = local;
        if (!m_pendingStart.contains(name) && !m_pendingStop.contains(name))
            status.state = ConfigRunState::Running;
        next.append(status);
    }
    for (auto it = m_localDisplayNames.cbegin(); it != m_localDisplayNames.cend(); ++it) {
        if (runningNames.contains(it.key()))
            continue;
        TrayInstanceStatus status;
        status.instanceName = it.key();
        status.displayName = it.value().isEmpty() ? it.key() : it.value();
        status.local = true;
        status.state = m_pendingStart.contains(it.key()) ? ConfigRunState::Starting
                     : ConfigRunState::Stopped;
        next.append(status);
    }
    m_snapshot.instances = next;

    if (instances.isEmpty()) {
        m_requestInFlight = false;
        emit snapshotChanged();
        return;
    }
    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher] {
        try {
            handleNetworkInfos(watcher->result());
        } catch (...) {
            m_requestInFlight = false;
            emit snapshotChanged();
        }
        watcher->deleteLater();
    });
    watcher->setFuture(m_api->collectNetworkInfos(instances.size()));
}

void TrayStatusService::handleNetworkInfos(const QJsonObject &result)
{
    const auto counts = parseNodeCounts(result);
    for (auto &instance : m_snapshot.instances) {
        if (counts.contains(instance.instanceName))
            instance.nodeCount = counts.value(instance.instanceName);
        else if (instance.state != ConfigRunState::Running)
            instance.nodeCount.reset();
    }
    m_requestInFlight = false;
    emit snapshotChanged();
}

TrayInstanceStatus *TrayStatusService::findInstance(const QString &name)
{
    for (auto &instance : m_snapshot.instances)
        if (instance.instanceName == name)
            return &instance;
    return nullptr;
}

const TrayInstanceStatus *TrayStatusService::findInstance(const QString &name) const
{
    for (const auto &instance : m_snapshot.instances)
        if (instance.instanceName == name)
            return &instance;
    return nullptr;
}

void TrayStatusService::startInstance(const QString &instanceName)
{
    if (!m_repository || !m_localDisplayNames.contains(instanceName)
        || m_snapshot.daemonState != DaemonClient::ConnectionState::Connected
        || m_pendingStart.contains(instanceName) || m_pendingStop.contains(instanceName))
        return;
    auto *instance = findInstance(instanceName);
    if (instance && instance->state != ConfigRunState::Stopped)
        return;
    const auto config = m_repository->load(instanceName);
    if (!config) {
        emit operationFailed(instanceName, QStringLiteral("本地配置不存在"));
        return;
    }
    m_pendingStart.insert(instanceName);
    if (instance)
        instance->state = ConfigRunState::Starting;
    emit snapshotChanged();
    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher, instanceName] {
        try {
            watcher->result();
            m_pendingStart.remove(instanceName);
            refresh();
        } catch (...) {
            m_pendingStart.remove(instanceName);
            if (auto *instance = findInstance(instanceName))
                instance->state = ConfigRunState::Stopped;
            emit operationFailed(instanceName, QStringLiteral("启动实例失败"));
            emit snapshotChanged();
        }
        watcher->deleteLater();
    });
    watcher->setFuture(m_api->runNetworkInstance(ConfigPayloadBuilder::daemonConfigPayload(*config)));
}

void TrayStatusService::stopInstance(const QString &instanceName)
{
    auto *instance = findInstance(instanceName);
    if (!instance || instance->state != ConfigRunState::Running
        || m_snapshot.daemonState != DaemonClient::ConnectionState::Connected
        || m_pendingStart.contains(instanceName) || m_pendingStop.contains(instanceName))
        return;
    m_pendingStop.insert(instanceName);
    instance->state = ConfigRunState::Stopping;
    emit snapshotChanged();
    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher, instanceName] {
        try {
            watcher->result();
            m_pendingStop.remove(instanceName);
            refresh();
        } catch (...) {
            m_pendingStop.remove(instanceName);
            if (auto *instance = findInstance(instanceName))
                instance->state = ConfigRunState::Running;
            emit operationFailed(instanceName, QStringLiteral("停止实例失败"));
            emit snapshotChanged();
        }
        watcher->deleteLater();
    });
    watcher->setFuture(m_api->deleteNetworkInstance(instanceName));
}

QHash<QString, int> TrayStatusService::parseNodeCounts(const QJsonObject &result)
{
    QHash<QString, int> counts;
    for (const auto &value : result.value(QStringLiteral("instances")).toArray()) {
        const auto object = value.toObject();
        const QString key = object.value(QStringLiteral("key")).toString();
        const auto decoded = QByteArray::fromBase64(
            object.value(QStringLiteral("value")).toString().toUtf8());
        const auto document = QJsonDocument::fromJson(decoded);
        if (key.isEmpty() || !document.isObject())
            continue;
        const auto network = document.object();
        const auto routes = network.value(QStringLiteral("routes")).toArray();
        // 本机 peer_id 仅用于排除本机节点，节点数统计不含本机
        const qint64 localPeer = network.value(QStringLiteral("my_node_info"))
            .toObject().value(QStringLiteral("peer_id")).toVariant().toLongLong();
        int count = 0;
        for (const auto &route : routes) {
            if (route.toObject().value(QStringLiteral("peer_id")).toVariant().toLongLong()
                != localPeer)
                ++count;
        }
        counts.insert(key, count);
    }
    return counts;
}
