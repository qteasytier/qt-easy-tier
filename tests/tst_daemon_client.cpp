/**
 * @file tst_daemon_client.cpp
 * @brief Daemon IPC 客户端模块的单元测试。测试内容：帧协议编解码往返（单帧/多帧）、异步请求-响应匹配、DaemonApi 方法映射、后端状态 ViewModel 初始状态、断连时写失败、请求超时、错误消息序列化/反序列化、非法 JSON 帧过滤、错误响应完成 future。
 *
 * 使用内存中的 QLocalServer 模拟 daemon 行为，验证：
 * - 帧协议编解码往返（单帧/多帧）
 * - 请求-响应匹配（异步 RPC）
 * - 错误消息序列化/反序列化
 *
 * 注意: 断线重连场景尚未覆盖，因为重连逻辑涉及定时器与真实 socket 断连时序。
 */
#include <QTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QFuture>
#include <QDir>
#include <QJsonArray>
#include <QUuid>
#include <QFile>
#include "core/service/DaemonClient.h"
#include "core/service/DaemonApi.h"
#include "core/viewmodel/runtime/BackendStatusViewModel.h"
#include "core/service/FrameProtocol.h"
#include "core/service/IpcMessage.h"

class TestDaemonClient : public QObject {
    Q_OBJECT

private slots:
    /// 测试目标: 验证帧协议编解码一次完整往返
    /// 构造 IpcMessage → JSON 序列化 → FrameProtocol 编码 → 解码 → 反序列化 → 断言字段一致
    void frameProtocolRoundTrip()
    {
        QByteArray buffer;
        auto msg = IpcMessage::request(42, "test", {{"p1", "v1"}});
        buffer.append(FrameProtocol::encode(msg.toJson()));

        auto frames = FrameProtocol::decode(buffer);
        // 检查单帧解码数量
        QCOMPARE(frames.size(), 1);

        auto decoded = IpcMessage::fromJson(frames[0]);
        // 检查往返后字段一致
        QCOMPARE(decoded.id, 42);
        QCOMPARE(decoded.method, QString("test"));
        QCOMPARE(decoded.params.value("p1").toString(), QString("v1"));
    }

    /// 测试目标: 验证缓冲区中多个帧能一次性正确解码
    /// 模拟 TCP 粘包场景——多个消息连续到达同一缓冲区
    void multipleFramesDecoded()
    {
        QByteArray buffer;
        auto m1 = IpcMessage::request(1, "a", {}).toJson();
        auto m2 = IpcMessage::request(2, "b", {}).toJson();
        buffer.append(FrameProtocol::encode(m1));
        buffer.append(FrameProtocol::encode(m2));

        auto frames = FrameProtocol::decode(buffer);
        // 检查多帧解码数量正确
        QCOMPARE(frames.size(), 2);
        QCOMPARE(IpcMessage::fromJson(frames[0]).id, 1);
        QCOMPARE(IpcMessage::fromJson(frames[1]).id, 2);
    }

    /// 测试目标: 验证异步请求-响应完整流程
    /// 通过 QLocalServer 模拟 daemon，使用 QFuture 异步获取响应结果
    /// 使用随机 socket 路径避免并发测试冲突
    void requestResponseRoundTrip()
    {
        // 准备模拟 daemon 服务端
        QLocalServer server;
        const QString sockPath = QDir::temp().filePath(QStringLiteral("qtet-test-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        QVERIFY(server.listen(sockPath));

        bool gotConnection = false;
        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            gotConnection = true;
            connect(sock, &QLocalSocket::readyRead, [sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    // 将请求方法名作为响应的 method 字段回显
                    auto resp = IpcMessage::response(req.id, req.method, {{"status", "ok"}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);

        QFuture<QJsonObject> future = client.call("list_instances", {});

        // 驱动事件循环以处理异步 socket I/O
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 3000);
        QVERIFY(!future.isCanceled());
        // 检查响应内容符合预期
        QCOMPARE(future.result().value("status").toString(), QString("ok"));
        QVERIFY(gotConnection);

        client.disconnectFromDaemon();
        server.close();
        QFile::remove(sockPath);
    }

    /// 测试目标: DaemonApi 将高层方法映射为 daemon 当前协议的 method/params
    void daemonApiUsesExpectedMethods()
    {
        // 准备模拟 daemon 服务端并记录收到的 method 和 params
        QLocalServer server;
        const QString sockPath = QDir::temp().filePath(QStringLiteral("qtet-test-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        QVERIFY(server.listen(sockPath));

        QStringList methods;
        QList<QJsonObject> paramsList;
        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [&, sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    methods.append(req.method);
                    paramsList.append(req.params);
                    auto resp = IpcMessage::response(req.id, req.method, {{"status", "ok"}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        DaemonApi api(&client);

        auto parseFuture = api.parseConfig({{QStringLiteral("cfg_str"), QStringLiteral("abc")}});
        QTRY_VERIFY_WITH_TIMEOUT(parseFuture.isFinished(), 3000);
        auto runFuture = api.runNetworkInstance({{QStringLiteral("cfg_str"), QStringLiteral("def")}});
        QTRY_VERIFY_WITH_TIMEOUT(runFuture.isFinished(), 3000);
        auto deleteFuture = api.deleteNetworkInstance(QStringLiteral("inst-a"));
        QTRY_VERIFY_WITH_TIMEOUT(deleteFuture.isFinished(), 3000);
        auto setReconFuture = api.setAutoReconnect(true);
        QTRY_VERIFY_WITH_TIMEOUT(setReconFuture.isFinished(), 3000);
        auto getReconFuture = api.getAutoReconnect();
        QTRY_VERIFY_WITH_TIMEOUT(getReconFuture.isFinished(), 3000);

        // 检查 DaemonApi 发出的 method 名称和参数正确
        QCOMPARE(methods.size(), 5);
        QCOMPARE(methods.at(0), QStringLiteral("parse_config"));
        QCOMPARE(paramsList.at(0).value(QStringLiteral("cfg_str")).toString(), QStringLiteral("abc"));
        QCOMPARE(methods.at(1), QStringLiteral("run_network_instance"));
        QCOMPARE(paramsList.at(1).value(QStringLiteral("cfg_str")).toString(), QStringLiteral("def"));
        QCOMPARE(methods.at(2), QStringLiteral("delete_network_instance"));
        QCOMPARE(paramsList.at(2).value(QStringLiteral("inst_names")).toArray().at(0).toString(), QStringLiteral("inst-a"));
        QCOMPARE(methods.at(3), QStringLiteral("set_auto_reconnect"));
        QCOMPARE(paramsList.at(3).value(QStringLiteral("enabled")).toBool(), true);
        QCOMPARE(methods.at(4), QStringLiteral("get_auto_reconnect"));
        QVERIFY(paramsList.at(4).isEmpty());

        client.disconnectFromDaemon();
        server.close();
        QFile::remove(sockPath);
    }

    /// 测试目标: 后端状态 ViewModel 隐藏 DaemonClient 枚举，提供 QML 友好状态
    void backendStatusViewModelInitialState()
    {
        DaemonClient client;
        BackendStatusViewModel vm(&client);

        // 检查初始状态为未连接
        QVERIFY(!vm.connected());
        QVERIFY(!vm.connecting());
        QCOMPARE(vm.statusText(), QStringLiteral("后端未连接"));
    }

    /// 测试目标: 未连接时调用 call() 应立即返回异常
    void writeFailureWhenDisconnected()
    {
        DaemonClient client;
        QFuture<QJsonObject> future = client.call("test", {});
        QVERIFY(future.isFinished());
        bool caught = false;
        try {
            future.result();
        } catch (const QException &) {
            caught = true;
        }
        // 检查调用时抛出异常
        QVERIFY(caught);
    }

    /// 测试目标: 请求在超时后应完成并携带异常
    void requestTimeout()
    {
        // 准备服务端：收到消息但不回复，触发超时
        QLocalServer server;
        const QString sockPath = QDir::temp().filePath(QStringLiteral("qtet-test-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        QVERIFY(server.listen(sockPath));

        DaemonClient client;
        client.setRequestTimeout(500);
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);

        // 服务端收到消息但不回复，触发超时
        bool serverGotMessage = false;
        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [&, sock]() {
                QByteArray buf = sock->readAll();
                auto frames = FrameProtocol::decode(buf);
                if (!frames.isEmpty())
                    serverGotMessage = true;
                // 故意不回复
            });
        });

        QFuture<QJsonObject> future = client.call("list_instances", {});
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 3000);
        QVERIFY(serverGotMessage);
        bool caught = false;
        try {
            future.result();
        } catch (const QException &) {
            caught = true;
        }
        // 检查超时后抛出异常
        QVERIFY(caught);

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: 验证 IpcMessage 错误消息的序列化/反序列化往返
    /// 确保错误类型、错误消息文本在编解码后不丢失
    void errorMessageRoundTrip()
    {
        auto err = IpcMessage::error(99, "run", "something broke");
        auto decoded = IpcMessage::fromJson(err.toJson());
        // 检查错误标记被正确保留
        QVERIFY(decoded.isError);
        QCOMPARE(decoded.errorMessage, QString("something broke"));
    }

    /// 测试目标: 收到非法 JSON 帧不触发 notificationReceived 信号
    void invalidJsonFrameIsNotNotification()
    {
        // 准备服务端：写入非法帧
        QLocalServer server;
        const QString sockPath = QDir::temp().filePath(QStringLiteral("qtet-test-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        QVERIFY(server.listen(sockPath));

        DaemonClient client;
        bool notified = false;
        connect(&client, &DaemonClient::notificationReceived, [&]() { notified = true; });

        // 客户端连接后服务端立即写入非法帧
        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            sock->write(FrameProtocol::encode(QByteArrayLiteral("{invalid")));
            sock->flush();
        });

        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);
        QTest::qWait(200);

        // 检查非法帧不触发通知
        QVERIFY(!notified);

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: 错误响应仍能正确完成 future 并携带异常
    void errorResponseStillCompletesFuture()
    {
        // 准备服务端：回复错误消息
        QLocalServer server;
        const QString sockPath = QDir::temp().filePath(QStringLiteral("qtet-test-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        QVERIFY(server.listen(sockPath));

        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [sock]() {
                QByteArray buf = sock->readAll();
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    auto err = IpcMessage::error(req.id, req.method, "daemon says boom");
                    sock->write(FrameProtocol::encode(err.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState() == DaemonClient::ConnectionState::Connected, 3000);

        QFuture<QJsonObject> future = client.call("list_instances", {});
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 3000);
        bool caught = false;
        try {
            future.result();
        } catch (const QException &) {
            caught = true;
        }
        // 检查错误响应仍抛出异常
        QVERIFY(caught);

        client.disconnectFromDaemon();
        server.close();
    }
};

QTEST_MAIN(TestDaemonClient)
#include "tst_daemon_client.moc"
