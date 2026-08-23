/**
 * @file tst_vpn_manager.cpp
 * @brief VpnManager stopAll 单元测试。
 *
 * 使用内存 QLocalServer 模拟 daemon，覆盖：
 * - 无运行实例时 stopAll() 立即以成功收敛
 * - 运行实例经 delete_network_instance 确认停止后 allStopped(true)
 * - daemon 拒绝停止时 allStopped(false)
 */
#include <QTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QDir>
#include <QUuid>
#include <QFile>

#include "core/repository/DatabaseConnection.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include "core/service/DaemonClient.h"
#include "core/service/FrameProtocol.h"
#include "core/service/IpcMessage.h"
#include "core/vpn_manager/StatusMonitor.h"
#include "core/vpn_manager/VpnManager.h"

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

class TestVpnManager : public QObject {
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
        VpnManager vpn(&client, &api, &repo, &monitor);

        QSignalSpy spy(&vpn, &VpnManager::allStopped);
        vpn.stopAll();

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
        VpnManager vpn(&client, &api, &repo, &monitor);

        // 启动实例并等待其进入 Running
        vpn.startConfig(QStringLiteral("inst-a"));
        QTRY_VERIFY_WITH_TIMEOUT(vpn.isRunning(QStringLiteral("inst-a")), 3000);

        QSignalSpy spy(&vpn, &VpnManager::allStopped);
        vpn.stopAll();

        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
        QCOMPARE(spy.takeFirst().at(0).toBool(), true);
        QVERIFY(!vpn.isRunning(QStringLiteral("inst-a")));

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
        VpnManager vpn(&client, &api, &repo, &monitor);

        vpn.startConfig(QStringLiteral("inst-a"));
        QTRY_VERIFY_WITH_TIMEOUT(vpn.isRunning(QStringLiteral("inst-a")), 3000);

        QSignalSpy spy(&vpn, &VpnManager::allStopped);
        vpn.stopAll();

        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
        QCOMPARE(spy.takeFirst().at(0).toBool(), false);
        // 停止失败后实例仍保持运行
        QVERIFY(vpn.isRunning(QStringLiteral("inst-a")));

        client.disconnectFromDaemon();
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

QTEST_MAIN(TestVpnManager)
#include "tst_vpn_manager.moc"
