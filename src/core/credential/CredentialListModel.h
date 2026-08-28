/**
 * @file CredentialListModel.h
 * @brief 临时凭证列表 Model（QAbstractListModel 子类）
 *
 * 展示当前运行实例已签发的临时凭证（安全模式临时节点密钥），包括凭证 ID、
 * 公钥指纹、过期时间、ACL 组、是否允许中继等。数据由 CredentialService
 * 通过 listCredentials 获取后以 setFromVariantList 注入。
 */
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QVariantList>

/**
 * @brief 临时凭证条目，描述一个已签发凭证的展示属性
 */
struct CredentialItem {
    QString credentialId;            ///< 凭证 ID
    QStringList groups;              ///< ACL 组
    bool allowRelay = false;         ///< 是否允许通过该凭证节点中继数据
    qint64 expiryUnix = 0;           ///< 到期 Unix 时间戳（秒）
    QStringList allowedProxyCidrs;   ///< 允许代理的 CIDR（可空）
    bool reusable = true;            ///< 是否允许多个节点并发复用
    QString publicKeyFingerprint;    ///< 公钥指纹
};

/**
 * @brief 临时凭证列表 Model，供 QML 展示与管理当前实例已签发的临时凭证
 */
class CredentialListModel : public QAbstractListModel
{
    Q_OBJECT
    /// 当前凭证数量
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    /// QML 可访问的数据角色枚举
    enum Roles {
        CredentialIdRole = Qt::UserRole + 1,   ///< 凭证 ID
        GroupsRole,                            ///< ACL 组
        AllowRelayRole,                        ///< 是否允许中继
        ExpiryUnixRole,                        ///< 到期 Unix 时间戳（秒）
        AllowedProxyCidrsRole,                 ///< 允许代理的 CIDR
        ReusableRole,                          ///< 是否可并发复用
        PublicKeyFingerprintRole,              ///< 公钥指纹
    };
    Q_ENUM(Roles)

    explicit CredentialListModel(QObject *parent = nullptr);

    // ---- QAbstractListModel 核心接口 ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// 当前凭证数量
    int count() const;

    /// 直接设置凭证列表（从结构体列表）
    void setItems(const QList<CredentialItem> &items);
    /// 从 QVariantList 反序列化并设置凭证列表（由 CredentialService 注入）
    void setFromVariantList(const QVariantList &items);

signals:
    /// 凭证数量变化时发射
    void countChanged();

private:
    QList<CredentialItem> m_items; ///< 凭证列表缓存
};
