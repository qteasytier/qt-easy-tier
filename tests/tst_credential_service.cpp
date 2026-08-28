/**
 * @file tst_credential_service.cpp
 * @brief 临时凭证（临时节点密钥）链路单元测试
 *
 * 使用内存 QLocalServer 模拟 daemon，验证：
 * - DaemonApi::callJsonRpc 发出正确的 call_json_rpc 请求（payload Base64）
 * - CredentialService 构造的 protobuf JSON 请求体正确（实例选择器 + 全部字段）
 * - CredentialService 正确解析 generate_credential 响应并发射 generateSucceeded
 * - CredentialService 正确解析 list_credentials 响应并填充凭证列表模型
 * - CredentialService 正确解析 upsert_credential / revoke_credential 响应
 * - daemon 返回错误时发射对应 Failed 信号
 * - CredentialViewModel 转发参数（逗号分隔拆分）并转发信号
 * - 操作期间统一状态机（operation / busy）状态正确
 */
#include <QTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QFuture>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QUuid>
#include <QFile>
#include <QSignalSpy>

#include "daemon_service/DaemonClient.h"
#include "daemon_service/DaemonApi.h"
#include "daemon_service/FrameProtocol.h"
#include "daemon_service/IpcMessage.h"
#include "core/credential/CredentialListModel.h"
#include "core/credential/CredentialService.h"
#include "core/viewmodels/credential/CredentialViewModel.h"

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

    /// 测试目标: listCredentials 构造正确的 protobuf JSON 请求体（仅实例选择器）
    void credentialServiceBuildsListPayload()
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
                    auto resp = IpcMessage::response(req.id, req.method, {{"response", ""}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);
        service.listCredentials(QStringLiteral("inst-a"));

        QTRY_VERIFY_WITH_TIMEOUT(!capturedParams.isEmpty(), 3000);
        QCOMPARE(capturedParams.value(QStringLiteral("method_name")).toString(),
                 QStringLiteral("list_credentials"));
        QCOMPARE(capturedParams.value(QStringLiteral("service_name")).toString(),
                 QStringLiteral("api.instance.CredentialManageRpcService"));
        QVERIFY(capturedParams.value(QStringLiteral("domain_name")).toString().isEmpty());

        const QByteArray payload = QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8());
        const QJsonObject obj = QJsonDocument::fromJson(payload).object();
        // 只应包含实例选择器
        QCOMPARE(obj.size(), 1);
        QCOMPARE(obj.value(QStringLiteral("instance"))
                     .toObject()
                     .value(QStringLiteral("instance_selector"))
                     .toObject()
                     .value(QStringLiteral("name"))
                     .toString(),
                 QStringLiteral("inst-a"));

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: listCredentials 解析 credentials 数组并填充列表模型（expiry 兼容数字与字符串）
    void credentialServiceListsAndParses()
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
                    // 两条凭证：expiry 分别以数字与字符串形式返回
                    const QByteArray respPayload = QByteArrayLiteral(
                        "{\"credentials\":["
                        "{\"credential_id\":\"c1\",\"groups\":[\"ops\"],\"allow_relay\":false,"
                        "\"expiry_unix\":1786000000,\"allowed_proxy_cidrs\":[],\"reusable\":true,"
                        "\"public_key_fingerprint\":\"aa:bb\"},"
                        "{\"credential_id\":\"c2\",\"groups\":[],\"allow_relay\":true,"
                        "\"expiry_unix\":\"1786000123\",\"allowed_proxy_cidrs\":[\"10.0.0.0/8\"],"
                        "\"reusable\":false,\"public_key_fingerprint\":\"cc:dd\"}"
                        "]}");
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method, {{"response", respB64}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);

        QSignalSpy listSpy(&service, &CredentialService::listSucceeded);
        QSignalSpy failSpy(&service, &CredentialService::listFailed);

        service.listCredentials(QStringLiteral("inst-a"));
        QCOMPARE(service.operation(), CredentialOperation::List);

        QTRY_VERIFY_WITH_TIMEOUT(listSpy.count() == 1, 3000);
        QCOMPARE(failSpy.count(), 0);
        QCOMPARE(service.operation(), CredentialOperation::Idle);

        auto *model = service.credentialListModel();
        QVERIFY(model);
        QCOMPARE(model->count(), 2);

        const QModelIndex idx0 = model->index(0, 0);
        QCOMPARE(model->data(idx0, CredentialListModel::CredentialIdRole).toString(),
                 QStringLiteral("c1"));
        QCOMPARE(model->data(idx0, CredentialListModel::PublicKeyFingerprintRole).toString(),
                 QStringLiteral("aa:bb"));
        QCOMPARE(model->data(idx0, CredentialListModel::ExpiryUnixRole).toLongLong(),
                 qint64(1786000000));
        QCOMPARE(model->data(idx0, CredentialListModel::AllowRelayRole).toBool(), false);
        QCOMPARE(model->data(idx0, CredentialListModel::ReusableRole).toBool(), true);
        QCOMPARE(model->data(idx0, CredentialListModel::GroupsRole).toStringList().size(), 1);
        QCOMPARE(model->data(idx0, CredentialListModel::GroupsRole).toStringList().at(0),
                 QStringLiteral("ops"));

        const QModelIndex idx1 = model->index(1, 0);
        QCOMPARE(model->data(idx1, CredentialListModel::CredentialIdRole).toString(),
                 QStringLiteral("c2"));
        QCOMPARE(model->data(idx1, CredentialListModel::ExpiryUnixRole).toLongLong(),
                 qint64(1786000123));
        QCOMPARE(model->data(idx1, CredentialListModel::AllowRelayRole).toBool(), true);
        QCOMPARE(model->data(idx1, CredentialListModel::ReusableRole).toBool(), false);
        QCOMPARE(model->data(idx1, CredentialListModel::AllowedProxyCidrsRole).toStringList().size(), 1);
        QCOMPARE(model->data(idx1, CredentialListModel::AllowedProxyCidrsRole).toStringList().at(0),
                 QStringLiteral("10.0.0.0/8"));

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: upsertCredential 构造正确请求体并解析 {"changed":true}
    void credentialServiceUpsertsAndParses()
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
                    const QByteArray respPayload = QByteArrayLiteral("{\"changed\":true}");
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method, {{"response", respB64}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);

        QSignalSpy upSpy(&service, &CredentialService::upsertSucceeded);
        QSignalSpy failSpy(&service, &CredentialService::upsertFailed);

        CredentialService::UpsertRequest req;
        req.instanceName = QStringLiteral("inst-a");
        req.credentialId = QStringLiteral("my-cred");
        req.credentialSecret = QStringLiteral("U2VjcmV0MTIz");
        req.groups = {QStringLiteral("ops")};
        req.allowRelay = true;
        req.allowedProxyCidrs = {QStringLiteral("10.0.0.0/8")};
        req.expiryUnix = 1786000000;
        req.reusable = false;
        service.upsertCredential(req);
        QCOMPARE(service.operation(), CredentialOperation::Upsert);

        QTRY_VERIFY_WITH_TIMEOUT(upSpy.count() == 1, 3000);
        QCOMPARE(failSpy.count(), 0);
        QCOMPARE(upSpy.at(0).at(0).toBool(), true);
        QCOMPARE(service.operation(), CredentialOperation::Idle);

        QCOMPARE(capturedParams.value(QStringLiteral("method_name")).toString(),
                 QStringLiteral("upsert_credential"));
        const QByteArray payload = QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8());
        const QJsonObject obj = QJsonDocument::fromJson(payload).object();
        QCOMPARE(obj.value(QStringLiteral("credential_id")).toString(), QStringLiteral("my-cred"));
        QCOMPARE(obj.value(QStringLiteral("credential_secret")).toString(), QStringLiteral("U2VjcmV0MTIz"));
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().size(), 1);
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().at(0).toString(), QStringLiteral("ops"));
        QCOMPARE(obj.value(QStringLiteral("allow_relay")).toBool(), true);
        QCOMPARE(obj.value(QStringLiteral("allowed_proxy_cidrs")).toArray().size(), 1);
        QCOMPARE(obj.value(QStringLiteral("expiry_unix")).toDouble(), 1786000000.0);
        QCOMPARE(obj.value(QStringLiteral("reusable")).toBool(), false);

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: revokeCredential 构造正确请求体并解析 {"success":false}
    void credentialServiceRevokesAndParses()
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
                    const QByteArray respPayload = QByteArrayLiteral("{\"success\":false}");
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method, {{"response", respB64}});
                    sock->write(FrameProtocol::encode(resp.toJson()));
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);

        QSignalSpy revSpy(&service, &CredentialService::revokedSucceeded);
        QSignalSpy failSpy(&service, &CredentialService::revokedFailed);

        service.revokeCredential(QStringLiteral("inst-a"), QStringLiteral("my-cred"));

        QTRY_VERIFY_WITH_TIMEOUT(revSpy.count() == 1, 3000);
        QCOMPARE(failSpy.count(), 0);
        QCOMPARE(revSpy.at(0).at(0).toBool(), false);
        QCOMPARE(service.operation(), CredentialOperation::Idle);

        QCOMPARE(capturedParams.value(QStringLiteral("method_name")).toString(),
                 QStringLiteral("revoke_credential"));
        const QByteArray payload = QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8());
        const QJsonObject obj = QJsonDocument::fromJson(payload).object();
        QCOMPARE(obj.value(QStringLiteral("credential_id")).toString(), QStringLiteral("my-cred"));
        QCOMPARE(obj.value(QStringLiteral("instance"))
                     .toObject()
                     .value(QStringLiteral("instance_selector"))
                     .toObject()
                     .value(QStringLiteral("name"))
                     .toString(),
                 QStringLiteral("inst-a"));

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: daemon 返回错误时 upsert 发射 upsertFailed 且操作状态复位
    void credentialServiceHandlesManagementError()
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

        QSignalSpy upSpy(&service, &CredentialService::upsertSucceeded);
        QSignalSpy failSpy(&service, &CredentialService::upsertFailed);

        CredentialService::UpsertRequest req;
        req.instanceName = QStringLiteral("inst-a");
        req.credentialId = QStringLiteral("my-cred");
        req.credentialSecret = QStringLiteral("U2Vj");
        req.expiryUnix = 1786000000;
        service.upsertCredential(req);

        QTRY_VERIFY_WITH_TIMEOUT(failSpy.count() == 1, 3000);
        QCOMPARE(upSpy.count(), 0);
        QVERIFY(failSpy.at(0).at(0).toString().contains(QStringLiteral("No instance matches")));
        QCOMPARE(service.operation(), CredentialOperation::Idle);

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: CredentialViewModel 转发 list/upsert/revoke（CSV 拆分、空 secret 透传）
    void credentialViewModelForwardsListUpsertRevoke()
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
                    // 按方法名返回不同响应（方法名在 params.method_name，IPC 层固定为 call_json_rpc）
                    const QString methodName = req.params.value(QStringLiteral("method_name")).toString();
                    QJsonObject respObj;
                    if (methodName == QStringLiteral("list_credentials")) {
                        respObj = QJsonObject{{QStringLiteral("credentials"),
                                               QJsonArray{QJsonObject{
                                                   {QStringLiteral("credential_id"), QStringLiteral("c1")},
                                                   {QStringLiteral("expiry_unix"), 1786000000}}}}};
                    } else if (methodName == QStringLiteral("upsert_credential")) {
                        respObj = QJsonObject{{QStringLiteral("changed"), true}};
                    } else {
                        respObj = QJsonObject{{QStringLiteral("success"), true}};
                    }
                    const QByteArray respPayload =
                        QJsonDocument(respObj).toJson(QJsonDocument::Compact);
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method, {{"response", respB64}});
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

        // 列表
        QSignalSpy listSpy(&vm, &CredentialViewModel::listSucceeded);
        vm.listCredentials(QStringLiteral("inst-a"));
        QTRY_VERIFY_WITH_TIMEOUT(listSpy.count() == 1, 3000);
        QCOMPARE(vm.credentialListModel()->count(), 1);
        QCOMPARE(vm.credentialListModel()->data(vm.credentialListModel()->index(0, 0),
                                                CredentialListModel::CredentialIdRole).toString(),
                 QStringLiteral("c1"));

        // 更新：CSV 拆分 + 空 secret 透传
        QSignalSpy upSpy(&vm, &CredentialViewModel::upsertSucceeded);
        vm.upsertCredential(QStringLiteral("inst-a"),
                            QStringLiteral("my-cred"),
                            QString(), // 空 secret：不设必填，原样透传
                            QStringLiteral(" ops, dev , "),
                            true,
                            QStringLiteral(" 10.0.0.0/8 , "),
                            1786000000,
                            false);
        QTRY_VERIFY_WITH_TIMEOUT(upSpy.count() == 1, 3000);
        const QByteArray upPayload = QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8());
        const QJsonObject upObj = QJsonDocument::fromJson(upPayload).object();
        QCOMPARE(upObj.value(QStringLiteral("credential_id")).toString(), QStringLiteral("my-cred"));
        QCOMPARE(upObj.value(QStringLiteral("credential_secret")).toString(), QString());
        QCOMPARE(upObj.value(QStringLiteral("groups")).toArray().size(), 2);
        QCOMPARE(upObj.value(QStringLiteral("groups")).toArray().at(0).toString(), QStringLiteral("ops"));
        QCOMPARE(upObj.value(QStringLiteral("groups")).toArray().at(1).toString(), QStringLiteral("dev"));
        QCOMPARE(upObj.value(QStringLiteral("allowed_proxy_cidrs")).toArray().size(), 1);
        QCOMPARE(upObj.value(QStringLiteral("expiry_unix")).toDouble(), 1786000000.0);
        QCOMPARE(upObj.value(QStringLiteral("reusable")).toBool(), false);

        // 撤销
        QSignalSpy revSpy(&vm, &CredentialViewModel::revokedSucceeded);
        vm.revokeCredential(QStringLiteral("inst-a"), QStringLiteral("my-cred"));
        QTRY_VERIFY_WITH_TIMEOUT(revSpy.count() == 1, 3000);
        const QByteArray revPayload = QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8());
        const QJsonObject revObj = QJsonDocument::fromJson(revPayload).object();
        QCOMPARE(revObj.value(QStringLiteral("credential_id")).toString(), QStringLiteral("my-cred"));

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: prepareEdit 自动取回原密钥并置 editSecretReady（请求 ttl=1、携带原 id）
    void credentialViewModelPrepareEditFetchesSecret()
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
                        "{\"credential_id\":\"cred-1\",\"credential_secret\":\"S3c\",\"expiry_unix\":100}");
                    const QString respB64 = QString::fromLatin1(respPayload.toBase64());
                    auto resp = IpcMessage::response(req.id, req.method, {{"response", respB64}});
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

        QSignalSpy genSpy(&vm, &CredentialViewModel::generateSucceeded);
        vm.prepareEdit(QStringLiteral("inst-a"), QStringLiteral("cred-1"));
        // 取钥中：签发操作进行中且处于编辑取钥会话
        QCOMPARE(vm.generating(), true);
        QCOMPARE(vm.fetchingSecret(), true);

        QTRY_VERIFY_WITH_TIMEOUT(genSpy.count() == 1, 3000);
        QCOMPARE(vm.fetchingSecret(), false);
        QCOMPARE(vm.editSecretReady(), true);
        QCOMPARE(vm.generating(), false);

        // 请求体：ttl=1、原 credential_id、无 groups
        QCOMPARE(capturedParams.value(QStringLiteral("method_name")).toString(),
                 QStringLiteral("generate_credential"));
        const QByteArray payload = QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8());
        const QJsonObject obj = QJsonDocument::fromJson(payload).object();
        QCOMPARE(obj.value(QStringLiteral("ttl_seconds")).toInt(), 1);
        QCOMPARE(obj.value(QStringLiteral("credential_id")).toString(), QStringLiteral("cred-1"));
        QCOMPARE(obj.value(QStringLiteral("groups")).toArray().size(), 0);

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: 提交时密钥留空自动回填 prepareEdit 取到的原密钥；提供新密钥则直接使用
    void credentialViewModelUpsertUsesFetchedSecretWhenBlank()
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
                    const QString method = req.params.value(QStringLiteral("method_name")).toString();
                    QJsonObject respObj;
                    if (method == QStringLiteral("generate_credential")) {
                        respObj = QJsonObject{{QStringLiteral("credential_id"), QStringLiteral("cred-1")},
                                              {QStringLiteral("credential_secret"), QStringLiteral("S3c")},
                                              {QStringLiteral("expiry_unix"), 100}};
                    } else {
                        respObj = QJsonObject{{QStringLiteral("changed"), true}};
                    }
                    const QByteArray respPayload =
                        QJsonDocument(respObj).toJson(QJsonDocument::Compact);
                    auto resp = IpcMessage::response(req.id, req.method,
                                                     {{"response", QString::fromLatin1(respPayload.toBase64())}});
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

        QSignalSpy upSpy(&vm, &CredentialViewModel::upsertSucceeded);
        vm.prepareEdit(QStringLiteral("inst-a"), QStringLiteral("cred-1"));
        QTRY_VERIFY_WITH_TIMEOUT(vm.editSecretReady(), 3000);

        // 密钥留空 → 回填 prepareEdit 取到的原密钥
        vm.upsertCredential(QStringLiteral("inst-a"), QStringLiteral("cred-1"),
                            QString(), QString(), false, QString(), 1786000000, true);
        QTRY_VERIFY_WITH_TIMEOUT(upSpy.count() == 1, 3000);
        QJsonObject upObj = QJsonDocument::fromJson(QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8())).object();
        QCOMPARE(upObj.value(QStringLiteral("credential_secret")).toString(), QStringLiteral("S3c"));

        // 提供新密钥 → 不回填
        vm.upsertCredential(QStringLiteral("inst-a"), QStringLiteral("cred-1"),
                            QStringLiteral("NewSec"), QString(), false, QString(),
                            1786000000, true);
        QTRY_VERIFY_WITH_TIMEOUT(upSpy.count() == 2, 3000);
        upObj = QJsonDocument::fromJson(QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8())).object();
        QCOMPARE(upObj.value(QStringLiteral("credential_secret")).toString(), QStringLiteral("NewSec"));

        client.disconnectFromDaemon();
        server.close();
    }

    /// 测试目标: prepareEdit 失败时 editSecretReady 为 false，空密钥不会回填（透传空）
    void credentialViewModelPrepareEditFailureFallsBack()
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
                    const QString method = req.params.value(QStringLiteral("method_name")).toString();
                    if (method == QStringLiteral("generate_credential")) {
                        auto err = IpcMessage::error(req.id, req.method,
                                                     "only admin nodes (with network_secret) can generate credentials");
                        sock->write(FrameProtocol::encode(err.toJson()));
                    } else {
                        const QByteArray respPayload = QByteArrayLiteral("{\"changed\":true}");
                        auto resp = IpcMessage::response(req.id, req.method,
                                                         {{"response", QString::fromLatin1(respPayload.toBase64())}});
                        sock->write(FrameProtocol::encode(resp.toJson()));
                    }
                    sock->flush();
                }
            });
        });

        DaemonClient client;
        connectClient(client, sockPath);

        DaemonApi api(&client);
        CredentialService service(&api);
        CredentialViewModel vm(&service);

        QSignalSpy failSpy(&vm, &CredentialViewModel::generateFailed);
        QSignalSpy upSpy(&vm, &CredentialViewModel::upsertSucceeded);
        vm.prepareEdit(QStringLiteral("inst-a"), QStringLiteral("cred-1"));
        QTRY_VERIFY_WITH_TIMEOUT(failSpy.count() == 1, 3000);
        QCOMPARE(vm.fetchingSecret(), false);
        QCOMPARE(vm.editSecretReady(), false);

        // 空密钥 → 不回填（透传空，服务端将拒绝）
        vm.upsertCredential(QStringLiteral("inst-a"), QStringLiteral("cred-1"),
                            QString(), QString(), false, QString(), 1786000000, true);
        QTRY_VERIFY_WITH_TIMEOUT(upSpy.count() == 1, 3000);
        const QJsonObject upObj = QJsonDocument::fromJson(QByteArray::fromBase64(
            capturedParams.value(QStringLiteral("payload")).toString().toUtf8())).object();
        QCOMPARE(upObj.value(QStringLiteral("credential_secret")).toString(), QString());

        client.disconnectFromDaemon();
        server.close();
    }
};

QTEST_MAIN(TestCredentialService)
#include "tst_credential_service.moc"
