/**
 * @file tst_vpn_runtime_service.cpp
 * @brief VpnRuntimeService stopAll 与外部实例同步单元测试。
 *
 * 使用内存 QLocalServer 模拟 daemon，覆盖：
 * - 无运行实例时 stopAll() 立即以成功收敛
 * - 运行实例经 delete_network_instance 确认停止后 allStopped(true)
 * - daemon 拒绝停止时 allStopped(false)
 * - 心跳发现外部实例（daemon 中存在但本地配置列表中没有）→ 创建 controller
 * - 外部实例从 daemon 消失 → 心跳清理 controller
 * - 停止外部实例 → 从外部实例集合移除
 * - 外部实例名成为本地配置（ensureLocalController）→ 外部临时 controller 被移除，心跳纠正为本地运行
 * - daemon 断开 → 外部临时 controller 全部清除
 */
#include <QTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QDir>
#include <QUuid>
#include <QFile>

#include "core/sqlite_repository/DatabaseConnection.h"
#include "core/sqlite_repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include "core/service/DaemonClient.h"
#include "core/service/FrameProtocol.h"
#include "core/service/IpcMessage.h"
#include "app_service/runtime/StatusMonitor.h"
#include "app_service/runtime/VpnRuntimeService.h"

/// 内存模拟 daemon：响应 run/delete/list 三类请求
/// delete_network_instance 成功后会同步移除对应实例（与真实 daemon 行为一致）
class FakeDaemon : public QObject {
    Q_OBJECT

public:
    /// delete_network_instance 是否返回错误（模拟停止失败）
    bool failDelete = false;
    /// list_instances 返回的运行中实例 key 列表
    QStringList runningKeys;

    bool start()
    {
        m_sockPath = QDir::temp().filePath(QStringLiteral("qtet-vpn-test-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        if (!m_server.listen(m_sockPath))
            return false;
        connect(&m_server, &QLocalServer::newConnection, this, &FakeDaemon::onNewConnection);
        return true;
    }

    QString socketPath() const { return m_sockPath; }

private slots:
    void onNewConnection()
    {
        auto *sock = m_server.nextPendingConnection();
        connect(sock, &QLocalSocket::readyRead, this, [this, sock]() {
            // 累积数据并拆解完整帧（处理半包/粘包）
            m_buffer.append(sock->readAll());
            const auto frames = FrameProtocol::decode(m_buffer);
            for (const QByteArray &f : frames) {
                const IpcMessage req = IpcMessage::fromJson(f);
                QJsonObject payload;

                if (req.method == QLatin1String("list_instances")) {
                    QJsonArray instances;
                    for (const QString &key : std::as_const(runningKeys))
                        instances.append(QJsonObject{{QStringLiteral("key"), key}});
                    payload = QJsonObject{{QStringLiteral("instances"), instances}};
                    sock->write(FrameProtocol::encode(IpcMessage::response(req.id, req.method, payload).toJson()));
                } else if (req.method == QLatin1String("delete_network_instance")) {
                    if (failDelete) {
                        sock->write(FrameProtocol::encode(IpcMessage::error(req.id, req.method,
                                                                             QStringLiteral("模拟停止失败")).toJson()));
                    } else {
                        // 从请求参数中读取实例名并移除（模拟真实 daemon 停止行为）
                        const QJsonArray names = req.params[QStringLiteral("inst_names")].toArray();
                        for (const QJsonValue &nameVal : names)
                            runningKeys.removeAll(nameVal.toString());
                        sock->write(FrameProtocol::encode(IpcMessage::response(req.id, req.method, payload).toJson()));
                    }
                } else {
                    // run_network_instance 等其余请求默认成功
                    sock->write(FrameProtocol::encode(IpcMessage::response(req.id, req.method, payload).toJson()));
                }
                sock->flush();
            }
        });
    }

private:
    QLocalServer m_server;
    QString m_sockPath;
    QByteArray m_buffer;
};

class TestVpnRuntimeService : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标: 无运行实例时 stopAll() 立即以成功收敛
    void stopAll_immediateSuccessWhenNothingRunning()
    {
        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository repo(conn.database());

        DaemonClient client;
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnRuntimeService runtime(&client, &api, &repo, &monitor);

        QSignalSpy spy(&runtime, &VpnRuntimeService::allStopped);
        runtime.stopAll();

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toBool(), true);
    }

    /// 测试目标: 运行实例经 daemon 确认停止后 allStopped(true)
    void stopAll_successAfterDaemonStopConfirmed()
    {
        FakeDaemon daemon;
        daemon.runningKeys = QStringList{QStringLiteral("inst-a")};
        QVERIFY(daemon.start());

        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository repo(conn.database());
        NetworkConf cfg(QStringLiteral("inst-a"));
        cfg.hostname = QStringLiteral("node-a");
        QVERIFY(repo.save(cfg));

        DaemonClient client;
        client.connectToDaemon(daemon.socketPath());
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnRuntimeService runtime(&client, &api, &repo, &monitor);

        // 启动实例并等待其进入 Running
        runtime.startConfig(QStringLiteral("inst-a"));
        QTRY_VERIFY_WITH_TIMEOUT(runtime.isRunning(QStringLiteral("inst-a")), 3000);

        QSignalSpy spy(&runtime, &VpnRuntimeService::allStopped);
        runtime.stopAll();

        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
        QCOMPARE(spy.takeFirst().at(0).toBool(), true);
        QVERIFY(!runtime.isRunning(QStringLiteral("inst-a")));

        client.disconnectFromDaemon();
        QFile::remove(daemon.socketPath());
    }

    /// 测试目标: daemon 拒绝停止时 allStopped(false)
    void stopAll_failsWhenDaemonRejectsStop()
    {
        FakeDaemon daemon;
        daemon.failDelete = true;
        daemon.runningKeys = QStringList{QStringLiteral("inst-a")};
        QVERIFY(daemon.start());

        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository repo(conn.database());
        NetworkConf cfg(QStringLiteral("inst-a"));
        cfg.hostname = QStringLiteral("node-a");
        QVERIFY(repo.save(cfg));

        DaemonClient client;
        client.connectToDaemon(daemon.socketPath());
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnRuntimeService runtime(&client, &api, &repo, &monitor);

        runtime.startConfig(QStringLiteral("inst-a"));
        QTRY_VERIFY_WITH_TIMEOUT(runtime.isRunning(QStringLiteral("inst-a")), 3000);

        QSignalSpy spy(&runtime, &VpnRuntimeService::allStopped);
        runtime.stopAll();

        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
        QCOMPARE(spy.takeFirst().at(0).toBool(), false);
        // 停止失败后实例仍保持运行
        QVERIFY(runtime.isRunning(QStringLiteral("inst-a")));

        client.disconnectFromDaemon();
        QFile::remove(daemon.socketPath());
    }

    /// 测试目标: 心跳发现 daemon 中存在但本地配置列表中没有的实例 → 创建外部 controller
    void heartbeat_discoversExternalInstance()
    {
        FakeDaemon daemon;
        daemon.runningKeys = QStringList{QStringLiteral("ext-inst")};
        QVERIFY(daemon.start());

        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository repo(conn.database());
        // 仓库中不包含 ext-inst 配置

        DaemonClient client;
        client.connectToDaemon(daemon.socketPath());
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnRuntimeService runtime(&client, &api, &repo, &monitor);

        QSignalSpy spy(&runtime, &VpnRuntimeService::externalInstancesChanged);
        // 等待心跳完成外部实例同步（心跳间隔 3s，超时放宽到 4s）
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 4000);
        QVERIFY(runtime.isRunning(QStringLiteral("ext-inst")));
        QCOMPARE(runtime.externalInstances(), QStringList{QStringLiteral("ext-inst")});
        QCOMPARE(spy.first().at(0).toStringList(), QStringList{QStringLiteral("ext-inst")});

        client.disconnectFromDaemon();
        QFile::remove(daemon.socketPath());
    }

    /// 测试目标: 外部实例从 daemon 消失后心跳清理 controller 并移出外部实例集合
    void heartbeat_removesVanishedExternalInstance()
    {
        FakeDaemon daemon;
        daemon.runningKeys = QStringList{QStringLiteral("ext-inst")};
        QVERIFY(daemon.start());

        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository repo(conn.database());

        DaemonClient client;
        client.connectToDaemon(daemon.socketPath());
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnRuntimeService runtime(&client, &api, &repo, &monitor);

        QTRY_VERIFY_WITH_TIMEOUT(runtime.isRunning(QStringLiteral("ext-inst")), 4000);
        QCOMPARE(runtime.externalInstances(), QStringList{QStringLiteral("ext-inst")});

        // daemon 中移除该实例，等待下一轮心跳清理
        daemon.runningKeys.clear();
        QTRY_VERIFY_WITH_TIMEOUT(!runtime.isRunning(QStringLiteral("ext-inst")), 4000);
        QVERIFY(runtime.externalInstances().isEmpty());

        client.disconnectFromDaemon();
        QFile::remove(daemon.socketPath());
    }

    /// 测试目标: 停止外部实例（delete_network_instance）后从外部实例集合移除
    void stopExternalInstance_removesFromList()
    {
        FakeDaemon daemon;
        daemon.runningKeys = QStringList{QStringLiteral("ext-inst")};
        QVERIFY(daemon.start());

        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository repo(conn.database());

        DaemonClient client;
        client.connectToDaemon(daemon.socketPath());
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnRuntimeService runtime(&client, &api, &repo, &monitor);

        QTRY_VERIFY_WITH_TIMEOUT(runtime.isRunning(QStringLiteral("ext-inst")), 4000);

        // 外部实例可停止：发送 delete_network_instance，daemon 移除 runningKeys，
        // 下一轮心跳将外部实例从集合中清理
        runtime.stopConfig(QStringLiteral("ext-inst"));
        QTRY_VERIFY_WITH_TIMEOUT(runtime.externalInstances().isEmpty(), 4000);
        QVERIFY(!runtime.isRunning(QStringLiteral("ext-inst")));

        client.disconnectFromDaemon();
        QFile::remove(daemon.socketPath());
    }

    /// 测试目标: 外部实例名成为本地配置（ensureLocalController）后，外部临时 controller 被移除，
    /// 下一轮心跳将本地 controller 纠正为 Running（外部/本地天然纠偏）
    void ensureLocalController_convertsExternalToLocal()
    {
        FakeDaemon daemon;
        daemon.runningKeys = QStringList{QStringLiteral("ext-inst")};
        QVERIFY(daemon.start());

        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository repo(conn.database());

        DaemonClient client;
        client.connectToDaemon(daemon.socketPath());
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnRuntimeService runtime(&client, &api, &repo, &monitor);

        // 先被心跳识别为外部实例
        QTRY_VERIFY_WITH_TIMEOUT(runtime.isRunning(QStringLiteral("ext-inst")), 4000);
        QCOMPARE(runtime.externalInstances(), QStringList{QStringLiteral("ext-inst")});

        // 该实例名成为本地配置：外部临时 controller 立即移除，列表不再有外部项
        runtime.ensureLocalController(QStringLiteral("ext-inst"));
        QVERIFY(runtime.externalInstances().isEmpty());

        // daemon 仍运行该实例：下一轮心跳把本地 controller 纠正为 Running
        QTRY_VERIFY_WITH_TIMEOUT(runtime.isRunning(QStringLiteral("ext-inst")), 4000);
        QVERIFY(runtime.externalInstances().isEmpty());

        client.disconnectFromDaemon();
        QFile::remove(daemon.socketPath());
    }

    /// 测试目标: daemon 断开后外部临时 controller 全部清除并通知列表清空
    void daemonDisconnect_clearsExternalInstances()
    {
        FakeDaemon daemon;
        daemon.runningKeys = QStringList{QStringLiteral("ext-inst")};
        QVERIFY(daemon.start());

        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository repo(conn.database());

        DaemonClient client;
        client.connectToDaemon(daemon.socketPath());
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnRuntimeService runtime(&client, &api, &repo, &monitor);

        // 先被心跳识别为外部实例
        QTRY_VERIFY_WITH_TIMEOUT(runtime.isRunning(QStringLiteral("ext-inst")), 4000);
        QCOMPARE(runtime.externalInstances(), QStringList{QStringLiteral("ext-inst")});

        QSignalSpy spy(&runtime, &VpnRuntimeService::externalInstancesChanged);
        // 断开 daemon：外部临时 controller 全部清除并通知列表清空
        client.disconnectFromDaemon();
        QTRY_VERIFY_WITH_TIMEOUT(runtime.externalInstances().isEmpty(), 4000);
        QVERIFY(spy.count() >= 1);
        QVERIFY(spy.last().at(0).toStringList().isEmpty());

        QFile::remove(daemon.socketPath());
    }

private:
    /// 创建随机命名的临时 SQLite 数据库，测试完全隔离
    static DatabaseConnection makeTempDb()
    {
        const QString path = QDir::temp().filePath(
            QStringLiteral("qtet-test-%1.db")
                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        return DatabaseConnection(path);
    }
};

QTEST_MAIN(TestVpnRuntimeService)
#include "tst_vpn_runtime_service.moc"
