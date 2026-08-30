/**
 * @file TrayStatusService.h
 * @brief DDE 托盘插件的轻量网络状态监测服务
 */
#pragma once

#include "TrayStatusTypes.h"

#include <QObject>

#include <QHash>
#include <QJsonObject>
#include <QSet>

class DaemonApi;
class DatabaseConnection;
class NetworkConfigRepository;
class QTimer;

/** @brief 独立于主程序运行的 daemon 状态、实例状态和节点数量协调器 */
class TrayStatusService final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param databasePath 本地配置数据库路径；为空时先设置与主应用一致的
     *        组织/应用标识，再使用默认路径（AppConfigLocation/qteasytier.db）
     * @param parent 父对象
     */
    explicit TrayStatusService(const QString &databasePath = QString(), QObject *parent = nullptr);
    ~TrayStatusService() override;

    TrayStatusSnapshot snapshot() const;
    DaemonClient::ConnectionState daemonState() const;

    /// 请求启动本地配置实例；外部实例不允许启动
    void startInstance(const QString &instanceName);
    /// 请求停止本地或外部运行实例
    void stopInstance(const QString &instanceName);
    /// 立即触发一轮状态刷新
    void refresh();

    /// 解析 collect_network_infos 中的节点数量，不读取事件日志
    static QHash<QString, int> parseNodeCounts(const QJsonObject &result);

    /**
     * @brief 解析数据库路径
     * @param explicitPath 调用方显式指定的路径；为空时设置应用标识并使用默认路径
     * @return 实际使用的数据库路径
     */
    static QString resolveDatabasePath(const QString &explicitPath);

signals:
    void snapshotChanged();
    void daemonStateChanged(DaemonClient::ConnectionState state);
    void operationFailed(const QString &instanceName, const QString &message);

private slots:
    void onDaemonStateChanged(DaemonClient::ConnectionState state);
    void onHeartbeat();

private:
    void queryInstances();
    void handleInstances(const QJsonObject &result);
    void handleNetworkInfos(const QJsonObject &result);
    void setDaemonState(DaemonClient::ConnectionState state);
    void clearRuntimeState();
    void reloadLocalConfigs();
    TrayInstanceStatus *findInstance(const QString &name);
    const TrayInstanceStatus *findInstance(const QString &name) const;

    DaemonClient *m_client = nullptr;
    DaemonApi *m_api = nullptr;
    DatabaseConnection *m_database = nullptr;
    NetworkConfigRepository *m_repository = nullptr;
    QTimer *m_timer = nullptr;
    TrayStatusSnapshot m_snapshot;
    QHash<QString, QString> m_localDisplayNames;
    bool m_requestInFlight = false;
    QSet<QString> m_pendingStart;
    QSet<QString> m_pendingStop;
};
