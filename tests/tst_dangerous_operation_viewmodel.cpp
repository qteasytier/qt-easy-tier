/**
 * @file tst_dangerous_operation_viewmodel.cpp
 * @brief 危险操作 ViewModel 单元测试。
 *
 * 覆盖：
 * - clearAllData() 全链路：停止所有服务后清空各表与设置文件，并请求退出应用
 * - 停止网络服务失败时中止清空，不请求退出
 * - refreshDaemonStatus() 依据后端状态计算按钮属性（安装/卸载切换与可用性）
 */
#include <QTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QDir>
#include <QTemporaryDir>
#include <QUuid>
#include <QFile>

#include "core/application/dangerous/DangerousOperationService.h"
#include "core/repository/DatabaseConnection.h"
#include "core/repository/FavoriteNodeRepository.h"
#include "core/repository/LogRepository.h"
#include "core/repository/NetworkConfigRepository.h"
#include "core/service/DaemonApi.h"
#include "core/service/DaemonClient.h"
#include "core/service/FrameProtocol.h"
#include "core/service/IpcMessage.h"
#include "core/util/DaemonRegisterHelper.h"
#include "core/viewmodel/DangerousOperationViewModel.h"
#include "core/vpn_manager/StatusMonitor.h"
#include "core/vpn_manager/VpnManager.h"

/// 内存模拟 daemon：list_instances 返回固定实例列表，delete_network_instance 总是失败
class FakeDaemon : public QObject {
    Q_OBJECT

public:
    /// list_instances 返回的运行中实例 key 列表
    QStringList runningKeys;

    bool start()
    {
        m_sockPath = QDir::temp().filePath(QStringLiteral("qtet-danger-test-%1.sock")
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
            m_buffer.append(sock->readAll());
            const auto frames = FrameProtocol::decode(m_buffer);
            for (const QByteArray &f : frames) {
                const IpcMessage req = IpcMessage::fromJson(f);
                if (req.method == QLatin1String("list_instances")) {
                    QJsonArray instances;
                    for (const QString &key : std::as_const(runningKeys))
                        instances.append(QJsonObject{{QStringLiteral("key"), key}});
                    sock->write(FrameProtocol::encode(IpcMessage::response(req.id, req.method,
                        QJsonObject{{QStringLiteral("instances"), instances}}).toJson()));
                } else if (req.method == QLatin1String("delete_network_instance")) {
                    // 模拟停止失败，用于验证清空流程中止
                    sock->write(FrameProtocol::encode(IpcMessage::error(req.id, req.method,
                                                                         QStringLiteral("模拟停止失败")).toJson()));
                } else {
                    // run_network_instance 等其余请求默认成功
                    sock->write(FrameProtocol::encode(IpcMessage::response(req.id, req.method, QJsonObject{}).toJson()));
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

class TestDangerousOperationViewModel : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标: 清空全部数据后各表与设置文件恢复默认，并发出退出请求
    void clearAllData_clearsEverythingAndRequestsQuit()
    {
        auto conn = makeTempDb();
        QVERIFY(conn.open());

        // 预置数据：网络配置、收藏节点、日志、非默认设置
        NetworkConfigRepository configRepo(conn.database());
        NetworkConf cfg(QStringLiteral("net-a"));
        cfg.hostname = QStringLiteral("node-a");
        QVERIFY(configRepo.save(cfg));

        FavoriteNodeRepository favoriteRepo(conn.database());
        QVERIFY(favoriteRepo.add(QStringLiteral("fav-a"), QStringLiteral("tcp://1.2.3.4:11010"),
                                 QStringLiteral("pubkey")).has_value());

        LogRepository logRepo(conn.database());
        QVERIFY(logRepo.insertLog(LogLevel::Info, QStringLiteral("测试日志"), QStringLiteral("Test"), 100));

        const QString settingsPath = QDir::temp().filePath(QStringLiteral("qtet-settings-%1.json")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        SettingsStore store(settingsPath);
        SettingsStore::Settings nonDefault;
        nonDefault.autoStart = true;
        nonDefault.logLevel = 3;
        nonDefault.maxLogEntries = 500;
        QVERIFY(store.save(nonDefault));

        DaemonClient client;
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnManager vpn(&client, &api, &configRepo, &monitor);
        DangerousOperationService service(&vpn, &configRepo, &favoriteRepo, &logRepo, settingsPath);
        DangerousOperationViewModel vm(&service);

        QSignalSpy finishedSpy(&vm, &DangerousOperationViewModel::operationFinished);
        QSignalSpy quitSpy(&vm, &DangerousOperationViewModel::quitRequested);

        vm.clearAllData();

        // 无运行实例 → stopAll 同步收敛成功 → 清空流程同步完成
        QCOMPARE(finishedSpy.count(), 1);
        QVERIFY(finishedSpy.takeFirst().at(0).toBool());
        QCOMPARE(quitSpy.count(), 1);
        QVERIFY(!vm.busy());

        // 验证各业务表已清空
        QVERIFY(configRepo.loadAll().isEmpty());
        QVERIFY(favoriteRepo.loadAll().isEmpty());
        QCOMPARE(logRepo.count(), 0);

        // 验证设置文件已恢复默认值
        const SettingsStore::Settings loaded = SettingsStore(settingsPath).load();
        QCOMPARE(loaded.autoStart, false);
        QCOMPARE(loaded.logLevel, 1);
        QCOMPARE(loaded.maxLogEntries, 100);

        QFile::remove(settingsPath);
    }

    /// 测试目标: 停止网络服务失败时中止清空，数据保持原样且不请求退出
    void clearAllData_abortsWhenStopFails()
    {
        FakeDaemon daemon;
        daemon.runningKeys = QStringList{QStringLiteral("net-a")};
        QVERIFY(daemon.start());

        auto conn = makeTempDb();
        QVERIFY(conn.open());

        NetworkConfigRepository configRepo(conn.database());
        FavoriteNodeRepository favoriteRepo(conn.database());
        LogRepository logRepo(conn.database());
        NetworkConf cfg(QStringLiteral("net-a"));
        cfg.hostname = QStringLiteral("node-a");
        QVERIFY(configRepo.save(cfg));

        DaemonClient client;
        client.connectToDaemon(daemon.socketPath());
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnManager vpn(&client, &api, &configRepo, &monitor);
        DangerousOperationService service(&vpn, &configRepo, &favoriteRepo, &logRepo, QString());
        DangerousOperationViewModel vm(&service);

        // 让实例进入 Running（FakeDaemon 对 run 请求返回成功）
        vpn.startConfig(QStringLiteral("net-a"));
        QTRY_VERIFY_WITH_TIMEOUT(vpn.isRunning(QStringLiteral("net-a")), 3000);

        QSignalSpy finishedSpy(&vm, &DangerousOperationViewModel::operationFinished);
        QSignalSpy quitSpy(&vm, &DangerousOperationViewModel::quitRequested);

        vm.clearAllData();

        // 停止失败 → 清空中止，不请求退出
        QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 3000);
        QVERIFY(!finishedSpy.takeFirst().at(0).toBool());
        QCOMPARE(quitSpy.count(), 0);
        QVERIFY(!vm.busy());

        // 数据未被清空
        QCOMPARE(configRepo.loadAll().size(), 1);

        client.disconnectFromDaemon();
        QFile::remove(daemon.socketPath());
    }

    /// 测试目标: 依据后端注册与运行状态计算按钮属性
    void refreshDaemonStatus_computesButtonState()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        auto conn = makeTempDb();
        QVERIFY(conn.open());
        NetworkConfigRepository configRepo(conn.database());
        FavoriteNodeRepository favoriteRepo(conn.database());
        LogRepository logRepo(conn.database());
        DaemonClient client;
        DaemonApi api(&client);
        StatusMonitor monitor;
        VpnManager vpn(&client, &api, &configRepo, &monitor);
        DangerousOperationService service(&vpn, &configRepo, &favoriteRepo, &logRepo, QString());
        DangerousOperationViewModel vm(&service);

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
#if defined(Q_OS_WIN)
        const QString daemonPath = QDir(tempDir.path()).filePath(QStringLiteral("qtet-daemon.exe"));
#else
        const QString daemonPath = QDir(tempDir.path()).filePath(QStringLiteral("qtet-daemon"));
#endif
        DaemonRegisterHelper::setDaemonBinaryPathOverrideForTesting(daemonPath);
#if defined(Q_OS_LINUX)
        const QString servicePath = QDir(tempDir.path()).filePath(QStringLiteral("qtet-daemon.service"));
        DaemonRegisterHelper::setSystemdServicePathOverrideForTesting(servicePath);
#endif
        // 创建 daemon 二进制文件
        QFile daemonFile(daemonPath);
        QVERIFY(daemonFile.open(QIODevice::WriteOnly));
        daemonFile.close();
#if defined(Q_OS_LINUX)
        QVERIFY(daemonFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
#endif
        // 二进制可用但未注册 → 仅安装可用
        // Linux 下未创建 service 文件即视为未注册；Windows 通过测试 override 模拟
#if defined(Q_OS_WIN)
        DaemonRegisterHelper::setServiceRegisteredOverrideForTesting(true, false);
#endif
        DaemonRegisterHelper::setDaemonProcessRunningOverrideForTesting(true, false);
        vm.refreshDaemonStatus();
        QVERIFY(!vm.daemonInstalled());
        QVERIFY(vm.daemonOperationEnabled());

        // 已注册且运行中 → 按钮切换为卸载且可用
#if defined(Q_OS_LINUX)
        // Linux 下创建 service 文件即视为已注册
        QFile serviceFile(servicePath);
        QVERIFY(serviceFile.open(QIODevice::WriteOnly | QIODevice::Text));
        serviceFile.write("[Unit]\nDescription=EasyTier Service\n");
        serviceFile.close();
#elif defined(Q_OS_WIN)
        DaemonRegisterHelper::setServiceRegisteredOverrideForTesting(true, true);
#endif
        DaemonRegisterHelper::setDaemonProcessRunningOverrideForTesting(true, true);
        vm.refreshDaemonStatus();
        QVERIFY(vm.daemonInstalled());
        QVERIFY(vm.daemonOperationEnabled());

        // 清理 override
        DaemonRegisterHelper::setDaemonBinaryPathOverrideForTesting(QString());
#if defined(Q_OS_WIN)
        DaemonRegisterHelper::setServiceRegisteredOverrideForTesting(false, false);
#endif
        DaemonRegisterHelper::setDaemonProcessRunningOverrideForTesting(false, false);
#if defined(Q_OS_LINUX)
        DaemonRegisterHelper::setSystemdServicePathOverrideForTesting(QString());
#endif
#else
        // 其他平台不支持后端管理，操作不可用
        vm.refreshDaemonStatus();
        QVERIFY(!vm.daemonOperationEnabled());
#endif
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

QTEST_MAIN(TestDangerousOperationViewModel)
#include "tst_dangerous_operation_viewmodel.moc"
