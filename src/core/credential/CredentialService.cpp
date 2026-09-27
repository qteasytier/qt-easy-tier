/**
 * @file CredentialService.cpp
 * @brief CredentialService 实现
 *
 * 各操作流程：
 * 1. 构造 protobuf JSON 请求体（snake_case 字段，携带实例选择器）
 * 2. 通过 DaemonApi 的凭证语义方法（generateCredential 等）调用 daemon，
 *    服务名/方法名与 Base64 信封编解码由 DaemonApi 内部完成
 * 3. 用 QFutureWatcher 异步等待结果（响应已解码为 protobuf JSON 对象），提取字段
 * 4. 发射对应成功 / 失败信号
 */
#include "CredentialService.h"

#include "core/credential/CredentialListModel.h"
#include "daemon_service/DaemonApi.h"

#include <QDateTime>
#include <QException>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonObject>

namespace {

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

    setOperation(CredentialOperation::Generate);

    QFuture<QJsonObject> future = m_daemonApi->generateCredential(payload);

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this,
            [this, watcher, request]() {
                watcher->deleteLater();
                setOperation(CredentialOperation::Idle);
                try {
                    // 响应信封（Base64）已由 DaemonApi 解码为 protobuf JSON 对象
                    const QJsonObject obj = watcher->result();

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

    setOperation(CredentialOperation::List);

    QFuture<QJsonObject> future = m_daemonApi->listCredentials(instancePayload(instanceName));

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        setOperation(CredentialOperation::Idle);
        try {
            // 响应信封（Base64）已由 DaemonApi 解码为 protobuf JSON 对象
            const QJsonObject obj = watcher->result();

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

    setOperation(CredentialOperation::Upsert);

    QFuture<QJsonObject> future = m_daemonApi->upsertCredential(payload);

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        setOperation(CredentialOperation::Idle);
        try {
            // 响应信封（Base64）已由 DaemonApi 解码为 protobuf JSON 对象
            const QJsonObject obj = watcher->result();
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

    setOperation(CredentialOperation::Revoke);

    QFuture<QJsonObject> future = m_daemonApi->revokeCredential(payload);

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [this, watcher]() {
        watcher->deleteLater();
        setOperation(CredentialOperation::Idle);
        try {
            // 响应信封（Base64）已由 DaemonApi 解码为 protobuf JSON 对象
            const QJsonObject obj = watcher->result();
            emit revokedSucceeded(obj.value(QStringLiteral("success")).toBool());
        } catch (const QException &e) {
            emit revokedFailed(QString::fromUtf8(e.what()));
        }
    });
    watcher->setFuture(future);
}
