/**
 * @file CredentialService.cpp
 * @brief CredentialService 实现
 *
 * generateCredential 流程：
 * 1. 构造 protobuf JSON 请求体（snake_case 字段，携带实例选择器）
 * 2. 通过 DaemonApi::callJsonRpc 调用 daemon 的 call_json_rpc 桥接方法
 * 3. 用 QFutureWatcher 异步等待结果，解码 Base64 response 后提取凭证字段
 * 4. 发射 generateSucceeded / generateFailed 信号
 */
#include "CredentialService.h"

#include "core/service/DaemonApi.h"

#include <QDateTime>
#include <QException>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
/** @brief 凭证管理 RPC 服务名（与 easytier-cli credential 对应） */
const QString kCredentialService = QStringLiteral("api.instance.CredentialManageRpcService");
/** @brief 签发临时凭证方法名 */
const QString kGenerateMethod = QStringLiteral("generate_credential");

/**
 * @brief 从响应对象中提取过期时刻（Unix 秒级时间戳）
 *
 * easytier 的 protobuf uint64 字段在 JSON 序列化时可能输出为数字或字符串，
 * 两种形式都需要支持；字段缺失或非法时返回 0。
 */
qint64 parseExpiryUnix(const QJsonObject &obj)
{
    const QJsonValue v = obj.value(QStringLiteral("expiry_unix"));
    if (v.isDouble())
        return static_cast<qint64>(v.toDouble());
    if (v.isString()) {
        bool ok = false;
        const qint64 val = v.toString().toLongLong(&ok);
        return ok ? val : 0;
    }
    return 0;
}
} // namespace

CredentialService::CredentialService(DaemonApi *daemonApi, QObject *parent)
    : QObject(parent)
    , m_daemonApi(daemonApi)
{
}

bool CredentialService::busy() const
{
    return m_busy;
}

void CredentialService::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void CredentialService::generateCredential(const GenerateRequest &request)
{
    if (m_busy)
        return;
    if (!m_daemonApi) {
        emit generateFailed(QStringLiteral("daemon API 不可用"));
        return;
    }

    // 构造 protobuf JSON 请求体：实例选择器 + 凭证参数（字段名 snake_case）
    QJsonObject payload;
    payload.insert(QStringLiteral("instance"),
                   QJsonObject{{QStringLiteral("instance_selector"),
                                QJsonObject{{QStringLiteral("name"), request.instanceName}}}});

    QJsonArray groups;
    for (const QString &g : request.groups)
        groups.append(g);
    payload.insert(QStringLiteral("groups"), groups);
    payload.insert(QStringLiteral("allow_relay"), request.allowRelay);

    QJsonArray cidrs;
    for (const QString &c : request.allowedProxyCidrs)
        cidrs.append(c);
    payload.insert(QStringLiteral("allowed_proxy_cidrs"), cidrs);
    payload.insert(QStringLiteral("ttl_seconds"), request.ttlSeconds);
    if (!request.credentialId.isEmpty())
        payload.insert(QStringLiteral("credential_id"), request.credentialId);
    payload.insert(QStringLiteral("reusable"), request.reusable);

    const QByteArray payloadJson = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    setBusy(true);

    QFuture<QJsonObject> future = m_daemonApi->callJsonRpc(kCredentialService, kGenerateMethod,
                                                           QString(),
                                                           QString::fromUtf8(payloadJson));

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this,
            [this, watcher, request]() {
                watcher->deleteLater();
                setBusy(false);
                try {
                    const QJsonObject result = watcher->result();
                    // daemon 返回的 response 字段按 Base64 编码
                    const QByteArray respB64 = result.value(QStringLiteral("response")).toString().toLatin1();
                    const QByteArray resp = QByteArray::fromBase64(respB64);

                    QJsonParseError parseError;
                    const QJsonDocument respDoc = QJsonDocument::fromJson(resp, &parseError);
                    if (parseError.error != QJsonParseError::NoError) {
                        emit generateFailed(QStringLiteral("解析 daemon 响应失败: %1")
                                                .arg(parseError.errorString()));
                        return;
                    }

                    const QJsonObject obj = respDoc.object();
                    qint64 expiryUnix = parseExpiryUnix(obj);
                    // 部分 daemon 版本不返回 expiry_unix 字段，用签发时刻 + ttl 估算过期时刻
                    if (expiryUnix <= 0 && request.ttlSeconds > 0)
                        expiryUnix = QDateTime::currentSecsSinceEpoch() + request.ttlSeconds;

                    emit generateSucceeded(
                        obj.value(QStringLiteral("credential_id")).toString(),
                        obj.value(QStringLiteral("credential_secret")).toString(),
                        expiryUnix);
                } catch (const QException &e) {
                    emit generateFailed(QString::fromUtf8(e.what()));
                }
            });
    watcher->setFuture(future);
}
