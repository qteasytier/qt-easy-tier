/**
 * @file tst_credential_service.cpp
 * @brief 临时凭证（临时节点密钥）链路单元测试
 *
 * 使用内存 QLocalServer 模拟 daemon，验证：
 * - DaemonApi::callJsonRpc 发出正确的 call_json_rpc 请求（payload Base64）
 * - CredentialService 构造的 protobuf JSON 请求体正确（实例选择器 + 全部字段）
 * - CredentialService 正确解析 generate_credential 响应并发射 generateSucceeded
 * - daemon 返回错误时发射 generateFailed
 * - CredentialViewModel 转发参数（逗号分隔拆分）并转发信号
 * - 生成期间 busy 状态正确
 */
#include <QTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QFuture>
#include <QDir>
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>
#include <QFile>
#include <QSignalSpy>

#include "core/service/DaemonClient.h"
#include "core/service/DaemonApi.h"
#include "core/service/FrameProtocol.h"
#include "core/service/IpcMessage.h"
#include "app_service/credential/CredentialService.h"
#include "viewmodels/credential/CredentialViewModel.h"

class TestCredentialService : public QObject {
    Q_OBJECT

private:
    /// 生成随机临时 socket 路径，避免并发测试冲突
    static QString tempSockPath()
    {
        return QDir::temp().filePath(QStringLiteral("qtet-cred-%1.sock")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    }

    /// 连接 DaemonClient 并等待进入 Connected 状态
    static void connectClient(DaemonClient &client, const QString &sockPath)
    {
        client.connectToDaemon(sockPath);
        QTRY_VERIFY_WITH_TIMEOUT(client.connectionState()
                                     == DaemonClient::ConnectionState::Connected,
                                 3000);
    }

private slots:
    /// 测试目标: DaemonApi::callJsonRpc 发出 call_json_rpc 请求，payload 按 Base64 编码
    void daemonApiCallJsonRpcBuildsCorrectMessage()
    {
        QLocalServer server;
        const QString sockPath = tempSockPath();
        QVERIFY(server.listen(sockPath));

        QString method;
        QJsonObject params;
        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [&, sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    method = req.method;
                    params = req.params;
                    auto resp = IpcMessage::response(req.id, req.method,
                                                     {{"response", ""}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        const QString payload = QStringLiteral("{\"a\":1}");
        QFuture<QJsonObject> future = api.callJsonRpc(QStringLiteral("Svc"),
                                                      QStringLiteral("Meth"),
                                                      QStringLiteral("domain"),
                                                      payload);
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 3000);

        // 检查请求方法名与参数
        QCOMPARE(method, QStringLiteral("call_json_rpc"));
        QCOMPARE(params.value(QStringLiteral("service_name")).toString(), QStringLiteral("Svc"));
        QCOMPARE(params.value(QStringLiteral("method_name")).toString(), QStringLiteral("Meth"));
        QCOMPARE(params.value(QStringLiteral("domain_name")).toString(), QStringLiteral("domain"));
        // payload 字段为 Base64，解码后应与原始 JSON 一致
        const QByteArray decoded = QByteArray::fromBase64(
            params.value(QStringLiteral("payload")).toString().toUtf8());
        QCOMPARE(QString::fromUtf8(decoded), payload);

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: CredentialService 构造正确的 protobuf JSON 请求体
    void credentialServiceBuildsCorrectPayload()
    {
        QLocalServer server;
        const QString sockPath = tempSockPath();
        QVERIFY(server.listen(sockPath));

        QJsonObject capturedParams;
        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [&, sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    capturedParams = req.params;
                    auto resp = IpcMessage::response(req.id, req.method,
                                                     {{"response", ""}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);

        CredentialService::GenerateRequest req;
        req.instanceName = QStringLiteral("inst-a");
        req.ttlSeconds = 7200;
        req.groups = {QStringLiteral("ops"), QStringLiteral("dev")};
        req.allowRelay = true;
        req.allowedProxyCidrs = {QStringLiteral("10.0.0.0/8")};
        req.credentialId = QStringLiteral("my-cred");
        req.reusable = false;
        service.generateCredential(req);

        QTRY_VERIFY_WITH_TIMEOUT(!capturedParams.isEmpty(), 3000);

        // 校验服务与方法名
        QCOMPARE(capturedParams.value(QStringLiteral("service_name")).toString(),
                 QStringLiteral("api.instance.CredentialManageRpcService"));
        QCOMPARE(capturedParams.value(QStringLiteral("method_name")).toString(),
                 QStringLiteral("generate_credential"));
        QVERIFY(capturedParams.value(QStringLiteral("domain_name")).toString().isEmpty());

        // 解码 payload 并校验字段
        const QByteArray payload = QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8());
        QJsonParseError parseError;
        const QJsonObject obj = QJsonDocument::fromJson(payload, &parseError).object();
        QCOMPARE(parseError.error, QJsonParseError::NoError);

        QCOMPARE(obj.value(QStringLiteral("instance"))
                     .toObject()
                     .value(QStringLiteral("instance_selector"))
                     .toObject()
                     .value(QStringLiteral("name"))
                     .toString(),
                 QStringLiteral("inst-a"));
        QCOMPARE(obj.value(QStringLiteral("ttl_seconds")).toInt(), 7200);
        QCOMPARE(obj.value(QStringLiteral("allow_relay")).toBool(), true);
        QCOMPARE(obj.value(QStringLiteral("reusable")).toBool(), false);
        QCOMPARE(obj.value(QStringLiteral("credential_id")).toString(), QStringLiteral("my-cred"));
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().size(), 2);
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().at(0).toString(), QStringLiteral("ops"));
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().at(1).toString(), QStringLiteral("dev"));
        QCOMPARE(obj.value(QStringLiteral("allowed_proxy_cidrs")).toArray().size(), 1);
        QCOMPARE(obj.value(QStringLiteral("allowed_proxy_cidrs")).toArray().at(0).toString(),
                 QStringLiteral("10.0.0.0/8"));

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: CredentialService 正确解析 generate_credential 响应并发射成功信号
    void credentialServiceGeneratesAndParses()
    {
        QLocalServer server;
        const QString sockPath = tempSockPath();
        QVERIFY(server.listen(sockPath));

        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    // 模拟 daemon 返回 generate_credential 响应（response 字段 Base64）
                    const QByteArray respPayload = QByteArrayLiteral(
                        "{\"credential_id\":\"cred-1\",\"credential_secret\":\"U2VjcmV0MTIz\",\"expiry_unix\":1786000000}");
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method,
                                                     {{"response", respB64}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);

        QSignalSpy successSpy(&service, &CredentialService::generateSucceeded);
        QSignalSpy failSpy(&service, &CredentialService::generateFailed);

        CredentialService::GenerateRequest req;
        req.instanceName = QStringLiteral("inst-a");
        req.ttlSeconds = 3600;
        service.generateCredential(req);

        // 生成过程中 busy 应为 true
        QVERIFY(service.busy());

        QTRY_VERIFY_WITH_TIMEOUT(successSpy.count() == 1, 3000);
        QCOMPARE(failSpy.count(), 0);
        QCOMPARE(successSpy.at(0).at(0).toString(), QStringLiteral("cred-1"));
        QCOMPARE(successSpy.at(0).at(1).toString(), QStringLiteral("U2VjcmV0MTIz"));
        QCOMPARE(successSpy.at(0).at(2).toLongLong(), qint64(1786000000));
        // 完成后 busy 应复位
        QVERIFY(!service.busy());

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: daemon 返回错误时 CredentialService 发射 generateFailed
    void credentialServiceHandlesDaemonError()
    {
        QLocalServer server;
        const QString sockPath = tempSockPath();
        QVERIFY(server.listen(sockPath));

        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    auto err = IpcMessage::error(req.id, req.method, "No instance matches...");
                    sock->write(FrameProtocol::encode(err.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);

        QSignalSpy successSpy(&service, &CredentialService::generateSucceeded);
        QSignalSpy failSpy(&service, &CredentialService::generateFailed);

        CredentialService::GenerateRequest req;
        req.instanceName = QStringLiteral("inst-a");
        req.ttlSeconds = 3600;
        service.generateCredential(req);

        QTRY_VERIFY_WITH_TIMEOUT(failSpy.count() == 1, 3000);
        QCOMPARE(successSpy.count(), 0);
        // DaemonClient 会在错误消息前加 "daemon error: " 前缀
        QVERIFY(failSpy.at(0).at(0).toString().contains(QStringLiteral("No instance matches")));
        QVERIFY(!service.busy());

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: expiry_unix 为字符串形式（protobuf uint64 JSON 序列化惯例）时也能解析
    void credentialServiceParsesStringExpiryUnix()
    {
        QLocalServer server;
        const QString sockPath = tempSockPath();
        QVERIFY(server.listen(sockPath));

        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    // 与真实 easytier 一致：expiry_unix 为字符串
                    const QByteArray respPayload = QByteArrayLiteral(
                        "{\"credential_id\":\"cred-2\",\"credential_secret\":\"U2Vj\",\"expiry_unix\":\"1787729903\"}");
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method,
                                                     {{"response", respB64}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);

        QSignalSpy successSpy(&service, &CredentialService::generateSucceeded);

        CredentialService::GenerateRequest req;
        req.instanceName = QStringLiteral("inst-a");
        req.ttlSeconds = 3600;
        service.generateCredential(req);

        QTRY_VERIFY_WITH_TIMEOUT(successSpy.count() == 1, 3000);
        QCOMPARE(successSpy.at(0).at(2).toLongLong(), qint64(1787729903));

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: 响应缺少 expiry_unix 字段时，按签发时刻 + ttl 估算过期时刻
    void credentialServiceFallsBackToTtlWhenExpiryMissing()
    {
        QLocalServer server;
        const QString sockPath = tempSockPath();
        QVERIFY(server.listen(sockPath));

        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    // 真实 daemon 的 generate_credential 响应不包含 expiry_unix
                    const QByteArray respPayload = QByteArrayLiteral(
                        "{\"credential_id\":\"cred-3\",\"credential_secret\":\"U2Vj\"}");
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method,
                                                     {{"response", respB64}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);

        QSignalSpy successSpy(&service, &CredentialService::generateSucceeded);

        CredentialService::GenerateRequest req;
        req.instanceName = QStringLiteral("inst-a");
        req.ttlSeconds = 7200;
        service.generateCredential(req);

        QTRY_VERIFY_WITH_TIMEOUT(successSpy.count() == 1, 3000);
        const qint64 expiry = successSpy.at(0).at(2).toLongLong();
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        // 估算值 = 签发时刻 + ttl，与 now + ttl 的偏差应小于 5 秒（网络往返 + 处理时间）
        QVERIFY(expiry >= now + 7200 - 5);
        QVERIFY(expiry <= now + 7200 + 5);

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: CredentialViewModel 拆分逗号分隔参数并转发信号
    void credentialViewModelSplitsAndForwards()
    {
        QLocalServer server;
        const QString sockPath = tempSockPath();
        QVERIFY(server.listen(sockPath));

        QJsonObject capturedParams;
        connect(&server, &QLocalServer::newConnection, [&]() {
            auto *sock = server.nextPendingConnection();
            connect(sock, &QLocalSocket::readyRead, [&, sock]() {
                QByteArray buf;
                buf.append(sock->readAll());
                auto frames = FrameProtocol::decode(buf);
                for (const QByteArray &f : frames) {
                    IpcMessage req = IpcMessage::fromJson(f);
                    capturedParams = req.params;
                    const QByteArray respPayload = QByteArrayLiteral(
                        "{\"credential_id\":\"c\",\"credential_secret\":\"s\",\"expiry_unix\":100}");
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method,
                                                     {{"response", respB64}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);
        CredentialViewModel vm(&service);

        QSignalSpy successSpy(&vm, &CredentialViewModel::generateSucceeded);
        QSignalSpy failSpy(&vm, &CredentialViewModel::generateFailed);

        vm.generateCredential(QStringLiteral("inst-a"),
                              3600,
                              QStringLiteral(" ops, dev , "),
                              false,
                              QStringLiteral(" 10.0.0.0/8 , "),
                              QStringLiteral(""),
                              true);

        QTRY_VERIFY_WITH_TIMEOUT(successSpy.count() == 1, 3000);
        QCOMPARE(failSpy.count(), 0);

        // 校验拆分的 groups / cidrs 已写入 payload
        const QByteArray payload = QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8());
        const QJsonObject obj = QJsonDocument::fromJson(payload).object();
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().size(), 2);
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().at(0).toString(), QStringLiteral("ops"));
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().at(1).toString(), QStringLiteral("dev"));
        QCOMPARE(obj.value(QStringLiteral("allowed_proxy_cidrs")).toArray().size(), 1);
        QCOMPARE(obj.value(QStringLiteral("allowed_proxy_cidrs")).toArray().at(0).toString(),
                 QStringLiteral("10.0.0.0/8"));
        // 空的 credential_id 不应出现在 payload 中
        QVERIFY(!obj.contains(QStringLiteral("credential_id")));

        client.disconnectFromDaemon();
        server.close();
    }
};

QTEST_MAIN(TestCredentialService)
#include "tst_credential_service.moc"
