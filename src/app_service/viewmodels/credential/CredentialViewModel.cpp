/**
 * @file CredentialViewModel.cpp
 * @brief CredentialViewModel 实现（薄壳）
 *
 * 所有属性和操作全部委托 CredentialService；busy/generating/listing/mutating
 * 等派生属性由服务统一状态机（CredentialOperation）推导，只读转发不重复维护
 * 状态。编辑取钥会话（prepareEdit）在此层编排：复用 generate_credential 取回
 * 原密钥并暂存，提交时留空自动回填，密钥不对外暴露。
 */
#include "CredentialViewModel.h"

#include "app_service/credential/CredentialListModel.h"
#include "app_service/credential/CredentialService.h"

#include <QLatin1Char>

CredentialViewModel::CredentialViewModel(CredentialService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
{
    if (!m_service)
        return;

    // 服务状态机变化 → 刷新派生属性并通知编辑取钥（其派生依赖 operation）
    connect(m_service, &CredentialService::operationChanged, this, [this]() {
        emit operationChanged();
        emit fetchingSecretChanged();
    });

    connect(m_service, &CredentialService::generateSucceeded,
            this, [this](const QString &id, const QString &secret, qint64 expiry) {
                // 编辑取钥会话：匹配成功后暂存原密钥（不对外暴露）
                if (!m_pendingEditId.isEmpty() && m_pendingEditId == id) {
                    m_editSecret = secret;
                    m_editSecretForId = id;
                    setEditSecretReady(true);
                    m_pendingEditId.clear();
                    emit fetchingSecretChanged();
                }
                // 始终转发给 QML（生成对话框也监听该信号）
                emit generateSucceeded(id, secret, expiry);
            });
    connect(m_service, &CredentialService::generateFailed,
            this, [this](const QString &message) {
                // 编辑取钥会话失败：复位状态，退回手动粘贴
                if (!m_pendingEditId.isEmpty()) {
                    setEditSecretReady(false);
                    m_pendingEditId.clear();
                    emit fetchingSecretChanged();
                }
                emit generateFailed(message);
            });

    connect(m_service, &CredentialService::listSucceeded,
            this, &CredentialViewModel::listSucceeded);
    connect(m_service, &CredentialService::listFailed,
            this, &CredentialViewModel::listFailed);
    connect(m_service, &CredentialService::upsertSucceeded,
            this, &CredentialViewModel::upsertSucceeded);
    connect(m_service, &CredentialService::upsertFailed,
            this, &CredentialViewModel::upsertFailed);
    connect(m_service, &CredentialService::revokedSucceeded,
            this, &CredentialViewModel::revokedSucceeded);
    connect(m_service, &CredentialService::revokedFailed,
            this, &CredentialViewModel::revokedFailed);
}

bool CredentialViewModel::busy() const
{
    return m_service && m_service->busy();
}

bool CredentialViewModel::generating() const
{
    return m_service && m_service->operation() == CredentialOperation::Generate;
}

bool CredentialViewModel::listing() const
{
    return m_service && m_service->operation() == CredentialOperation::List;
}

bool CredentialViewModel::mutating() const
{
    return m_service && (m_service->operation() == CredentialOperation::Upsert
                         || m_service->operation() == CredentialOperation::Revoke);
}

bool CredentialViewModel::fetchingSecret() const
{
    // 派生：签发操作进行中且处于编辑取钥会话
    return m_service && m_service->operation() == CredentialOperation::Generate
           && !m_pendingEditId.isEmpty();
}

bool CredentialViewModel::editSecretReady() const
{
    return m_editSecretReady;
}

CredentialListModel *CredentialViewModel::credentialListModel() const
{
    return m_service ? m_service->credentialListModel() : nullptr;
}

void CredentialViewModel::generateCredential(const QString &instanceName,
                                             int ttlSeconds,
                                             const QString &groupsCsv,
                                             bool allowRelay,
                                             const QString &allowedProxyCidrsCsv,
                                             const QString &credentialId,
                                             bool reusable)
{
    if (!m_service)
        return;

    CredentialService::GenerateRequest req;
    req.instanceName = instanceName;
    req.ttlSeconds = ttlSeconds;
    req.groups = splitCsv(groupsCsv);
    req.allowRelay = allowRelay;
    req.allowedProxyCidrs = splitCsv(allowedProxyCidrsCsv);
    req.credentialId = credentialId.trimmed();
    req.reusable = reusable;
    m_service->generateCredential(req);
}

void CredentialViewModel::listCredentials(const QString &instanceName)
{
    if (m_service)
        m_service->listCredentials(instanceName);
}

void CredentialViewModel::prepareEdit(const QString &instanceName, const QString &credentialId)
{
    if (!m_service)
        return;

    m_pendingEditId = credentialId.trimmed();
    if (m_pendingEditId.isEmpty()) {
        setEditSecretReady(false);
        emit fetchingSecretChanged();
        return;
    }
    if (m_service->busy()) {
        m_pendingEditId.clear();
        setEditSecretReady(false);
        emit fetchingSecretChanged();
        emit generateFailed(QStringLiteral("服务忙，请稍后重试"));
        return;
    }

    m_editSecret.clear();
    m_editSecretForId.clear();
    setEditSecretReady(false);
    emit fetchingSecretChanged();

    // 复用 generate_credential：凭证已存在且未过期时，服务端返回原密钥而不改动凭证
    CredentialService::GenerateRequest req;
    req.instanceName = instanceName;
    req.ttlSeconds = 1;
    req.credentialId = m_pendingEditId;
    req.reusable = true;
    m_service->generateCredential(req);
}

void CredentialViewModel::setEditSecretReady(bool v)
{
    if (m_editSecretReady == v)
        return;
    m_editSecretReady = v;
    emit editSecretReadyChanged();
}

void CredentialViewModel::upsertCredential(const QString &instanceName,
                                           const QString &credentialId,
                                           const QString &credentialSecret,
                                           const QString &groupsCsv,
                                           bool allowRelay,
                                           const QString &allowedProxyCidrsCsv,
                                           qint64 expiryUnix,
                                           bool reusable)
{
    if (!m_service)
        return;

    CredentialService::UpsertRequest req;
    req.instanceName = instanceName;
    req.credentialId = credentialId.trimmed();
    // 密钥留空且已通过 prepareEdit 取到原密钥时，自动回填原密钥
    QString secret = credentialSecret.trimmed();
    if (secret.isEmpty() && req.credentialId == m_editSecretForId)
        secret = m_editSecret;
    req.credentialSecret = secret;
    req.groups = splitCsv(groupsCsv);
    req.allowRelay = allowRelay;
    req.allowedProxyCidrs = splitCsv(allowedProxyCidrsCsv);
    req.expiryUnix = expiryUnix;
    req.reusable = reusable;
    m_service->upsertCredential(req);
}

void CredentialViewModel::revokeCredential(const QString &instanceName,
                                           const QString &credentialId)
{
    if (m_service)
        m_service->revokeCredential(instanceName, credentialId);
}

QStringList CredentialViewModel::splitCsv(const QString &csv)
{
    QStringList out;
    const QStringList parts = csv.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (QString p : parts) {
        p = p.trimmed();
        if (!p.isEmpty())
            out.append(p);
    }
    return out;
}
