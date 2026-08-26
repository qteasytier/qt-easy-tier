/**
 * @file CredentialViewModel.cpp
 * @brief CredentialViewModel 实现（薄壳）
 *
 * 所有属性和操作全部委托 CredentialService，
 * 构造时转发其信号为 QML 可绑定的属性通知。
 */
#include "CredentialViewModel.h"

#include "app_service/credential/CredentialService.h"

#include <QLatin1Char>

CredentialViewModel::CredentialViewModel(CredentialService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
{
    if (!m_service)
        return;

    connect(m_service, &CredentialService::busyChanged,
            this, &CredentialViewModel::busyChanged);
    connect(m_service, &CredentialService::generateSucceeded,
            this, &CredentialViewModel::generateSucceeded);
    connect(m_service, &CredentialService::generateFailed,
            this, &CredentialViewModel::generateFailed);
}

bool CredentialViewModel::busy() const
{
    return m_service && m_service->busy();
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
