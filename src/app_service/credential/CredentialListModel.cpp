/**
 * @file CredentialListModel.cpp
 * @brief CredentialListModel 实现
 *
 * setFromVariantList 将 CredentialService 从 daemon 响应的 credentials 数组
 * 反序列化为 CredentialItem 列表。expiry_unix 为 protobuf uint64，JSON 中可能
 * 输出为数字或字符串，解析时两种形式均需兼容。
 */
#include "CredentialListModel.h"

#include <QVariantMap>

namespace {

/**
 * @brief 从 QVariantMap 中解析 Unix 秒级时间戳（兼容数字与字符串两种 JSON 形态）
 * @param map 凭证字段映射
 * @return 解析成功返回时间戳，字段缺失或非法返回 0
 */
qint64 parseExpiryUnix(const QVariantMap &map)
{
    const QVariant v = map.value(QStringLiteral("expiry_unix"));
    if (v.metaType().id() == QMetaType::Double || v.metaType().id() == QMetaType::Int
        || v.metaType().id() == QMetaType::LongLong)
        return v.toLongLong();
    if (v.metaType().id() == QMetaType::QString) {
        bool ok = false;
        const qint64 val = v.toString().toLongLong(&ok);
        return ok ? val : 0;
    }
    return 0;
}

/**
 * @brief 从 QVariantMap 中提取字符串列表字段
 * @param map 凭证字段映射
 * @param key 字段名
 * @return 字符串列表（空值/缺失返回空列表）
 */
QStringList toStringList(const QVariantMap &map, const QString &key)
{
    QStringList out;
    const QVariantList raw = map.value(key).toList();
    for (const QVariant &v : raw)
        out.append(v.toString());
    return out;
}
} // namespace

CredentialListModel::CredentialListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CredentialListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant CredentialListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const auto &item = m_items.at(index.row());
    switch (role) {
    case CredentialIdRole: return item.credentialId;
    case GroupsRole: return item.groups;
    case AllowRelayRole: return item.allowRelay;
    case ExpiryUnixRole: return item.expiryUnix;
    case AllowedProxyCidrsRole: return item.allowedProxyCidrs;
    case ReusableRole: return item.reusable;
    case PublicKeyFingerprintRole: return item.publicKeyFingerprint;
    default: return {};
    }
}

QHash<int, QByteArray> CredentialListModel::roleNames() const
{
    return {
        {CredentialIdRole, "credentialId"},
        {GroupsRole, "groups"},
        {AllowRelayRole, "allowRelay"},
        {ExpiryUnixRole, "expiryUnix"},
        {AllowedProxyCidrsRole, "allowedProxyCidrs"},
        {ReusableRole, "reusable"},
        {PublicKeyFingerprintRole, "publicKeyFingerprint"},
    };
}

int CredentialListModel::count() const
{
    return m_items.size();
}

void CredentialListModel::setItems(const QList<CredentialItem> &items)
{
    const int oldCount = m_items.size();
    beginResetModel();
    m_items = items;
    endResetModel();
    if (oldCount != m_items.size())
        emit countChanged();
}

void CredentialListModel::setFromVariantList(const QVariantList &items)
{
    QList<CredentialItem> converted;
    converted.reserve(items.size());
    for (const QVariant &value : items) {
        const QVariantMap map = value.toMap();
        CredentialItem item;
        item.credentialId = map.value(QStringLiteral("credential_id")).toString();
        item.groups = toStringList(map, QStringLiteral("groups"));
        item.allowRelay = map.value(QStringLiteral("allow_relay")).toBool();
        item.expiryUnix = parseExpiryUnix(map);
        item.allowedProxyCidrs = toStringList(map, QStringLiteral("allowed_proxy_cidrs"));
        item.reusable = map.value(QStringLiteral("reusable"), true).toBool();
        item.publicKeyFingerprint = map.value(QStringLiteral("public_key_fingerprint")).toString();
        converted.append(item);
    }
    setItems(converted);
}
