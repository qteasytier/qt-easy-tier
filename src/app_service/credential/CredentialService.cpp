/**
 * @file CredentialService.cpp
 * @brief CredentialService 实现
 *
 * 各操作流程：
 * 1. 构造 protobuf JSON 请求体（snake_case 字段，携带实例选择器）
 * 2. 通过 DaemonApi::callJsonRpc 调用 daemon 的 call_json_rpc 桥接方法
 * 3. 用 QFutureWatcher 异步等待结果，解码 Base64 response 后提取字段
 * 4. 发射对应成功 / 失败信号
 */
#include "CredentialService.h"

#include "app_service/credential/CredentialListModel.h"
#include "core/daemon_service/DaemonApi.h"

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
/** @brief 查询凭证列表方法名 */
const QString kListMethod = QStringLiteral("list_credentials");
/** @brief 新增/更新凭证方法名 */
const QString kUpsertMethod = QStringLiteral("upsert_credential");
/** @brief 撤销凭证方法名 */
const QString kRevokeMethod = QStringLiteral("revoke_credential");

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

/**
 * @brief 构造携带实例选择器的公共请求体
 * @param instanceName 目标实例名
 * @return 请求体对象（调用方继续插入业务字段）
 */
QJsonObject instancePayload(const QString &instanceName)
{
    return QJsonObject{{QStringLiteral("instance"),
                        QJsonObject{{QStringLiteral("instance_selector"),
                                     QJsonObject{{QStringLiteral("name"), instanceName}}}}}};
}

/**
 * @brief 将字符串列表转为 QJsonArray
 * @param list 字符串列表
 * @return 对应的 JSON 数组
 */
QJsonArray toJsonArray(const QStringList &list)
{
    QJsonArray arr;
    for (const QString &s : list)
        arr.append(s);
    return arr;
}

/**
 * @brief 解码 daemon 返回结果中的 Base64 response 字段为 JSON 对象
 * @param result DaemonApi 返回的异步结果（含 Base64 编码的 response 字段）
 * @param ok     输出：解码与解析是否成功
 * @return 解析得到的响应对象；失败时为空对象
 */
QJsonObject decodeResponse(const QJsonObject &result, bool *ok)
{
    const QByteArray respB64 = result.value(QStringLiteral("response")).toString().toLatin1();
    const QByteArray resp = QByteArray::fromBase64(respB64);

    QJsonParseError parseError;
    const QJsonDocument respDoc = QJsonDocument::fromJson(resp, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        *ok = false;
        return {};
    }
    *ok = true;
    return respDoc.object();
}
} // namespace

CredentialService::CredentialService(DaemonApi *daemonApi, QObject *parent)
    : QObject(parent)
    , m_daemonApi(daemonApi)
{
    m_credentialListModel = new CredentialListModel(this);
}

CredentialOperation CredentialService::operation() const
{
    return m_operation;
}

bool CredentialService::busy() const
{
    return credentialOperationIsBusy(m_operation);
}

CredentialListModel *CredentialService::credentialListModel() const
{
    return m_credentialListModel;
}

void CredentialService::setOperation(CredentialOperation op)
{
    if (m_operation == op)
        return;
    m_operation = op;
    emit operationChanged();
}

void CredentialService::generateCredential(const GenerateRequest &request)
{
    if (m_operation != CredentialOperation::Idle)
        return;
    if (!m_daemonApi) {
        emit generateFailed(QStringLiteral("daemon API 不可用"));
        return;
    }

    // 构造 protobuf JSON 请求体：实例选择器 + 凭证参数（字段名 snake_case）
    QJsonObject payload = instancePayload(request.instanceName);
    payload.insert(QStringLiteral("groups"), toJsonArray(request.groups));
    payload.insert(QStringLiteral("allow_relay"), request.allowRelay);
    payload.insert(QStringLiteral("allowed_proxy_cidrs"), toJsonArray(request.allowedProxyCidrs));
    payload.insert(QStringLiteral("ttl_seconds"), request.ttlSeconds);
    if (!request.credentialId.isEmpty())
        payload.insert(QStringLiteral("credential_id"), request.credentialId);
    payload.insert(QStringLiteral("reusable"), request.reusable);

    const QByteArray payloadJson = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    setOperation(CredentialOperation::Generate);

    QFuture<QJsonObject> future = m_daemonApi->callJsonRpc(kCredentialService, kGenerateMethod,
                                                           QString(),
                                                           QString::fromUtf8(payloadJson));

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this,
            [this, watcher, request]() {
                watcher->deleteLater();
                setOperation(CredentialOperation::Idle);
                try {
                    bool ok = false;
                    const QJsonObject obj = decodeResponse(watcher->result(), &ok);
                    if (!ok) {
                        emit generateFailed(QStringLiteral("解析 daemon 响应失败"));
                        return;
                    }

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

void CredentialService::listCredentials(const QString &instanceName)
{
    if (m_operation != CredentialOperation::Idle)
        return;
    if (!m_daemonApi) {
        emit listFailed(QStringLiteral("daemon API 不可用"));
        return;
    }

    const QByteArray payloadJson =
        QJsonDocument(instancePayload(instanceName)).toJson(QJsonDocument::Compact);
    setOperation(CredentialOperation::List);

    QFuture<QJsonObject> future = m_daemonApi->callJsonRpc(kCredentialService, kListMethod,
                                                           QString(),
                                                           QString::fromUtf8(payloadJson));

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        setOperation(CredentialOperation::Idle);
        try {
            bool ok = false;
            const QJsonObject obj = decodeResponse(watcher->result(), &ok);
            if (!ok) {
                emit listFailed(QStringLiteral("解析 daemon 响应失败"));
                return;
            }

            QVariantList items;
            const QJsonArray creds = obj.value(QStringLiteral("credentials")).toArray();
            for (const QJsonValue &v : creds)
                items.append(v.toObject().toVariantMap());
            m_credentialListModel->setFromVariantList(items);
            emit listSucceeded();
        } catch (const QException &e) {
            emit listFailed(QString::fromUtf8(e.what()));
        }
    });
    watcher->setFuture(future);
}

void CredentialService::upsertCredential(const UpsertRequest &request)
{
    if (m_operation != CredentialOperation::Idle)
        return;
    if (!m_daemonApi) {
        emit upsertFailed(QStringLiteral("daemon API 不可用"));
        return;
    }

    QJsonObject payload = instancePayload(request.instanceName);
    payload.insert(QStringLiteral("credential_id"), request.credentialId);
    payload.insert(QStringLiteral("credential_secret"), request.credentialSecret);
    payload.insert(QStringLiteral("groups"), toJsonArray(request.groups));
    payload.insert(QStringLiteral("allow_relay"), request.allowRelay);
    payload.insert(QStringLiteral("allowed_proxy_cidrs"), toJsonArray(request.allowedProxyCidrs));
    payload.insert(QStringLiteral("expiry_unix"), request.expiryUnix);
    payload.insert(QStringLiteral("reusable"), request.reusable);

    const QByteArray payloadJson = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    setOperation(CredentialOperation::Upsert);

    QFuture<QJsonObject> future = m_daemonApi->callJsonRpc(kCredentialService, kUpsertMethod,
                                                           QString(),
                                                           QString::fromUtf8(payloadJson));

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        setOperation(CredentialOperation::Idle);
        try {
            bool ok = false;
            const QJsonObject obj = decodeResponse(watcher->result(), &ok);
            if (!ok) {
                emit upsertFailed(QStringLiteral("解析 daemon 响应失败"));
                return;
            }
            emit upsertSucceeded(obj.value(QStringLiteral("changed")).toBool());
        } catch (const QException &e) {
            emit upsertFailed(QString::fromUtf8(e.what()));
        }
    });
    watcher->setFuture(future);
}

void CredentialService::revokeCredential(const QString &instanceName, const QString &credentialId)
{
    if (m_operation != CredentialOperation::Idle)
        return;
    if (!m_daemonApi) {
        emit revokedFailed(QStringLiteral("daemon API 不可用"));
        return;
    }

    QJsonObject payload = instancePayload(instanceName);
    payload.insert(QStringLiteral("credential_id"), credentialId);

    const QByteArray payloadJson = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    setOperation(CredentialOperation::Revoke);

    QFuture<QJsonObject> future = m_daemonApi->callJsonRpc(kCredentialService, kRevokeMethod,
                                                           QString(),
                                                           QString::fromUtf8(payloadJson));

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        setOperation(CredentialOperation::Idle);
        try {
            bool ok = false;
            const QJsonObject obj = decodeResponse(watcher->result(), &ok);
            if (!ok) {
                emit revokedFailed(QStringLiteral("解析 daemon 响应失败"));
                return;
            }
            emit revokedSucceeded(obj.value(QStringLiteral("success")).toBool());
        } catch (const QException &e) {
            emit revokedFailed(QString::fromUtf8(e.what()));
        }
    });
    watcher->setFuture(future);
}
