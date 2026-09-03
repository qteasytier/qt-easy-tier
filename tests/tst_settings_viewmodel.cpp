/**
 * @file tst_settings_viewmodel.cpp
 * @brief SettingsViewModel 开机自启与自动回连逻辑单元测试。
 *
 * 自启动验证以系统实际状态为唯一权威源（settings3.json 不再持久化该字段）：
 * - autoStart() 实时读取系统状态，不依赖内存缓存
 * - setAutoStart() 创建/删除系统自启动项，系统状态实际变化时发射 autoStartChanged
 * - 同状态设置保持幂等，不重复修改系统或发射信号
 * - refreshAutoStart() 重新发射属性通知
 *
 * 自动回连验证通过内存 QLocalServer 模拟 daemon（get/set_auto_reconnect RPC）：
 * - refreshAutoReconnect() 查询成功/失败后的状态与忙状态收敛
 * - setAutoReconnectEnabled() 使用 daemon 返回的实际状态、失败回滚与失败信号
 * - daemon 未连接时立即发射失败信号
 */
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include "daemon_service/DaemonApi.h"
#include "daemon_service/DaemonClient.h"
#include "daemon_service/FrameProtocol.h"
#include "daemon_service/IpcMessage.h"
#include "platform/AutoStartHelper.h"
#include "core/viewmodels/SettingsViewModel.h"

class TestSettingsViewModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
#if defined(Q_OS_LINUX)
        QVERIFY(m_tempDir.isValid());
        m_desktopPath = QDir(m_tempDir.path()).filePath(QStringLiteral("autostart/QtEasyTier.desktop"));
        AutoStartHelper::setDesktopFilePathOverrideForTesting(m_desktopPath);
        AutoStartHelper::setAutoStart(false);
#else
        QSKIP("SettingsViewModel autostart tests require an isolated backend on this platform");
#endif
    }

    void cleanupTestCase()
    {
#if defined(Q_OS_LINUX)
        AutoStartHelper::setAutoStart(false);
        AutoStartHelper::setDesktopFilePathOverrideForTesting(QString());
#endif
    }

    /// 目标：autoStart() 直接读取系统实际状态（外部修改后 getter 返回最新值，无缓存）
    void autoStart_readsSystemState()
    {
        SettingsViewModel vm;
        QVERIFY(!vm.autoStart());

        // 绕过 ViewModel 直接修改系统自启动项
        QVERIFY(AutoStartHelper::setAutoStart(true));
        QVERIFY(vm.autoStart());

        QVERIFY(AutoStartHelper::setAutoStart(false));
        QVERIFY(!vm.autoStart());
    }

    /// 目标：setAutoStart(true) 创建系统自启动项并发射 autoStartChanged
    void setAutoStart_enablesAndEmitsSignal()
    {
        SettingsViewModel vm;
        QSignalSpy spy(&vm, &SettingsViewModel::autoStartChanged);
        QVERIFY(vm.setAutoStart(true));
        QVERIFY(AutoStartHelper::isAutoStartEnabled());
        QCOMPARE(spy.count(), 1);
    }

    /// 目标：setAutoStart(false) 删除系统自启动项并发射 autoStartChanged
    void setAutoStart_disablesAndEmitsSignal()
    {
        SettingsViewModel vm;
        QSignalSpy spy(&vm, &SettingsViewModel::autoStartChanged);
        QVERIFY(vm.setAutoStart(false));
        QVERIFY(!AutoStartHelper::isAutoStartEnabled());
        QCOMPARE(spy.count(), 1);
    }

    /// 目标：同状态设置保持幂等，不重复修改系统状态或发射信号
    void setAutoStart_isIdempotent()
    {
        SettingsViewModel vm;
        QVERIFY(vm.setAutoStart(true));

        QSignalSpy spy(&vm, &SettingsViewModel::autoStartChanged);
        QVERIFY(vm.setAutoStart(true));
        QCOMPARE(spy.count(), 0);
        QVERIFY(AutoStartHelper::isAutoStartEnabled());
    }

    /// 目标：setThemeMode() 即时落盘 settings3.json，新实例可恢复；非法值回退 auto；同值幂等
    void setThemeMode_persistsAndRestores()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString path = QDir(tempDir.path()).filePath(QStringLiteral("settings3.json"));

        {
            SettingsViewModel vm(nullptr, nullptr, path);
            QCOMPARE(vm.themeMode(), QStringLiteral("auto"));

            QSignalSpy spy(&vm, &SettingsViewModel::themeModeChanged);
            vm.setThemeMode(QStringLiteral("dark"));
            QCOMPARE(vm.themeMode(), QStringLiteral("dark"));
            QCOMPARE(spy.count(), 1);
            // 同值设置幂等：不发信号
            vm.setThemeMode(QStringLiteral("dark"));
            QCOMPARE(spy.count(), 1);
        }

        // 新实例从 settings3.json 恢复持久化的 dark
        {
            SettingsViewModel vm(nullptr, nullptr, path);
            QCOMPARE(vm.themeMode(), QStringLiteral("dark"));
            // 非法主题值规范化回 auto 并落盘
            vm.setThemeMode(QStringLiteral("blue"));
            QCOMPARE(vm.themeMode(), QStringLiteral("auto"));
        }
        QCOMPARE(SettingsViewModel(nullptr, nullptr, path).themeMode(), QStringLiteral("auto"));
    }

    /// 目标：setLanguage() 即时落盘 settings3.json，新实例可恢复；非法值回退 system；同值幂等
    void setLanguage_persistsAndRestores()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QString path = QDir(tempDir.path()).filePath(QStringLiteral("settings3.json"));

        {
            SettingsViewModel vm(nullptr, nullptr, path);
            QCOMPARE(vm.language(), QStringLiteral("system"));

            QSignalSpy spy(&vm, &SettingsViewModel::languageChanged);
            vm.setLanguage(QStringLiteral("en"));
            QCOMPARE(vm.language(), QStringLiteral("en"));
            QCOMPARE(spy.count(), 1);
            // 同值设置幂等：不发信号
            vm.setLanguage(QStringLiteral("en"));
            QCOMPARE(spy.count(), 1);
            // 非法语言值规范化回 system
            vm.setLanguage(QStringLiteral("fr"));
            QCOMPARE(vm.language(), QStringLiteral("system"));
        }

        // 新实例从 settings3.json 恢复：最后一次写入为 system（非法值回退后落盘）
        QCOMPARE(SettingsViewModel(nullptr, nullptr, path).language(), QStringLiteral("system"));

        // 再次写入合法值并验证恢复
        {
            SettingsViewModel vm(nullptr, nullptr, path);
            vm.setLanguage(QStringLiteral("zh_TW"));
        }
        QCOMPARE(SettingsViewModel(nullptr, nullptr, path).language(), QStringLiteral("zh_TW"));
    }

    /// 目标：refreshAutoStart() 重新发射属性通知，供 QML 重新读取系统状态
    void refreshAutoStart_emitsSignal()
    {
        SettingsViewModel vm;
        QSignalSpy spy(&vm, &SettingsViewModel::autoStartChanged);
        vm.refreshAutoStart();
        QCOMPARE(spy.count(), 1);
    }

    /// 目标：refreshAutoReconnect() 查询成功时更新状态并收敛忙状态
    void autoReconnectRefresh_readsDaemonState()
    {
        QLocalServer server;
        QString sockPath;
        bool autoReconnectValue = false;
        bool failRequests = false;
        QVERIFY(startAutoReconnectFakeDaemon(server, sockPath, autoReconnectValue, failRequests));

        DaemonClient client;
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        SettingsViewModel vm(&api, nullptr);

        QSignalSpy busySpy(&vm, &SettingsViewModel::autoReconnectBusyChanged);
        QSignalSpy stateSpy(&vm, &SettingsViewModel::autoReconnectChanged);

        autoReconnectValue = true;
        vm.refreshAutoReconnect();

        QTRY_COMPARE_WITH_TIMEOUT(stateSpy.count(), 1, 3000);
        QVERIFY(vm.autoReconnect());
        QVERIFY(!vm.autoReconnectBusy());
        // busy true→false 各一次
        QCOMPARE(busySpy.count(), 2);

        client.disconnectFromDaemon();
        server.close();
        QFile::remove(sockPath);
    }

    /// 目标：refreshAutoReconnect() 查询失败时只收敛忙状态，不发射失败信号、不改状态
    void autoReconnectRefresh_failureKeepsState()
    {
        QLocalServer server;
        QString sockPath;
        bool autoReconnectValue = false;
        bool failRequests = true;
        QVERIFY(startAutoReconnectFakeDaemon(server, sockPath, autoReconnectValue, failRequests));

        DaemonClient client;
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        SettingsViewModel vm(&api, nullptr);

        QSignalSpy busySpy(&vm, &SettingsViewModel::autoReconnectBusyChanged);
        QSignalSpy stateSpy(&vm, &SettingsViewModel::autoReconnectChanged);
        QSignalSpy failSpy(&vm, &SettingsViewModel::autoReconnectOperationFailed);

        vm.refreshAutoReconnect();

        QTRY_COMPARE_WITH_TIMEOUT(busySpy.count(), 2, 3000);
        QVERIFY(!vm.autoReconnectBusy());
        QVERIFY(!vm.autoReconnect());
        // 查询失败只记录日志，不发射失败信号、不改状态
        QCOMPARE(stateSpy.count(), 0);
        QCOMPARE(failSpy.count(), 0);

        client.disconnectFromDaemon();
        server.close();
        QFile::remove(sockPath);
    }

    /// 目标：setAutoReconnectEnabled() 成功时以 daemon 返回的实际状态为准
    void autoReconnectSet_succeedsUsingDaemonResult()
    {
        QLocalServer server;
        QString sockPath;
        bool autoReconnectValue = false;
        bool failRequests = false;
        QVERIFY(startAutoReconnectFakeDaemon(server, sockPath, autoReconnectValue, failRequests));

        DaemonClient client;
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        SettingsViewModel vm(&api, nullptr);

        QSignalSpy busySpy(&vm, &SettingsViewModel::autoReconnectBusyChanged);
        QSignalSpy stateSpy(&vm, &SettingsViewModel::autoReconnectChanged);

        vm.setAutoReconnectEnabled(true);

        QTRY_COMPARE_WITH_TIMEOUT(stateSpy.count(), 1, 3000);
        QVERIFY(vm.autoReconnect());
        QVERIFY(!vm.autoReconnectBusy());
        QCOMPARE(busySpy.count(), 2);

        client.disconnectFromDaemon();
        server.close();
        QFile::remove(sockPath);
    }

    /// 目标：setAutoReconnectEnabled() 失败时回滚状态、收敛忙状态并发射失败信号
    void autoReconnectSet_failsAndRollsBack()
    {
        QLocalServer server;
        QString sockPath;
        bool autoReconnectValue = false;
        bool failRequests = true;
        QVERIFY(startAutoReconnectFakeDaemon(server, sockPath, autoReconnectValue, failRequests));

        DaemonClient client;
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);
        SettingsViewModel vm(&api, nullptr);

        QSignalSpy busySpy(&vm, &SettingsViewModel::autoReconnectBusyChanged);
        QSignalSpy stateSpy(&vm, &SettingsViewModel::autoReconnectChanged);
        QSignalSpy failSpy(&vm, &SettingsViewModel::autoReconnectOperationFailed);

        vm.setAutoReconnectEnabled(true);

        QTRY_COMPARE_WITH_TIMEOUT(failSpy.count(), 1, 3000);
        QVERIFY(!vm.autoReconnect());
        QVERIFY(!vm.autoReconnectBusy());
        // 失败回滚到 previous（false），状态未变化不发射 autoReconnectChanged
        QCOMPARE(stateSpy.count(), 0);
        QCOMPARE(busySpy.count(), 2);
        QVERIFY(failSpy.first().at(0).toString().startsWith(QStringLiteral("设置自动回连失败")));

        client.disconnectFromDaemon();
        server.close();
        QFile::remove(sockPath);
    }

    /// 目标：daemon 未连接时设置自动回连立即发射失败信号，忙状态保持 false
    void autoReconnectSet_unconnectedEmitsFailure()
    {
        DaemonClient client;
        DaemonApi api(&client);
        SettingsViewModel vm(&api, nullptr);

        QSignalSpy failSpy(&vm, &SettingsViewModel::autoReconnectOperationFailed);
        vm.setAutoReconnectEnabled(true);
        // DaemonApi 非空但 daemon 未连接：DaemonClient 返回携带异常的已完成 future，
        // watcher 在事件循环中发射失败信号，消息前缀为"设置自动回连失败"
        QTRY_COMPARE_WITH_TIMEOUT(failSpy.count(), 1, 3000);
        QVERIFY(failSpy.first().at(0).toString().startsWith(QStringLiteral("设置自动回连失败")));
        QVERIFY(!vm.autoReconnectBusy());
        QVERIFY(!vm.autoReconnect());
    }

private:
    /// 启动内存 fake daemon，响应 get/set_auto_reconnect RPC
    /// failRequests=true 时 set_auto_reconnect 返回错误
    bool startAutoReconnectFakeDaemon(QLocalServer &server, QString &sockPath,
                                      bool &autoReconnectValue, bool &failRequests)
    {
        sockPath = QDir::temp().filePath(QStringLiteral("qtet-test-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        if (!server.listen(sockPath))
            return false;

        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [sock, &autoReconnectValue, &failRequests]() {
                QByteArray buf;
                buf.append(sock->readAll());
                const auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    const IpcMessage req = IpcMessage::fromJson(f);
                    if (failRequests && req.method == QLatin1String("set_auto_reconnect")) {
                        sock->write(FrameProtocol::encode(
                            IpcMessage::error(req.id, req.method, QStringLiteral("模拟失败")).toJson()));
                    } else if (req.method == QLatin1String("get_auto_reconnect")) {
                        sock->write(FrameProtocol::encode(IpcMessage::response(req.id, req.method,
                            QJsonObject{{QStringLiteral("autoReconnect"), autoReconnectValue}}).toJson()));
                    } else if (req.method == QLatin1String("set_auto_reconnect")) {
                        const bool enabled = req.params.value(QStringLiteral("enabled")).toBool(false);
                        autoReconnectValue = enabled;
                        sock->write(FrameProtocol::encode(IpcMessage::response(req.id, req.method,
                            QJsonObject{{QStringLiteral("autoReconnect"), autoReconnectValue}}).toJson()));
                    } else {
                        sock->write(FrameProtocol::encode(IpcMessage::response(req.id, req.method, QJsonObject{}).toJson()));
                    }
                    sock->flush();
                }
            });
        });
        return true;
    }

    QTemporaryDir m_tempDir;
    QString m_desktopPath;
};

QTEST_MAIN(TestSettingsViewModel)
#include "tst_settings_viewmodel.moc"
