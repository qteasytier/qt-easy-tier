/**
 * @file CredentialViewModel.h
 * @brief 临时节点密钥 ViewModel（运行状态页，薄壳）
 *
 * 为运行状态页"添加临时节点密钥 / 管理临时节点密钥"功能提供 QML 入口，
 * 全部业务逻辑委托应用服务层 CredentialService 执行，本类只做转发。
 */
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "app_service/credential/CredentialListModel.h"

class CredentialService;

/** @brief 临时节点密钥 ViewModel，供 QML 签发/查询/更新/撤销安全模式临时凭证（薄壳转发） */
class CredentialViewModel : public QObject {
    Q_OBJECT

    /// 是否有任一操作进行中（由服务状态机 operation 派生）
    Q_PROPERTY(bool busy READ busy NOTIFY operationChanged FINAL)
    /// 签发操作进行中（含编辑取钥）
    Q_PROPERTY(bool generating READ generating NOTIFY operationChanged FINAL)
    /// 列表查询进行中
    Q_PROPERTY(bool listing READ listing NOTIFY operationChanged FINAL)
    /// 变更操作（新增/更新/撤销）进行中
    Q_PROPERTY(bool mutating READ mutating NOTIFY operationChanged FINAL)
    /// 编辑取钥进行中（编辑凭证时自动获取原密钥）
    Q_PROPERTY(bool fetchingSecret READ fetchingSecret NOTIFY fetchingSecretChanged FINAL)
    /// 编辑取钥是否已成功（原密钥已暂存，提交时留空可自动回填）
    Q_PROPERTY(bool editSecretReady READ editSecretReady NOTIFY editSecretReadyChanged FINAL)
    /// 凭证列表模型（当前实例已签发凭证）
    Q_PROPERTY(CredentialListModel *credentialListModel READ credentialListModel CONSTANT)

public:
    /**
     * @brief 构造临时凭证 ViewModel
     * @param service 临时凭证服务（应用服务层，非所有权）
     * @param parent  父对象
     */
    explicit CredentialViewModel(CredentialService *service, QObject *parent = nullptr);

    /// 查询是否有任一操作进行中
    bool busy() const;
    /// 查询签发操作是否进行中
    bool generating() const;
    /// 查询列表查询是否进行中
    bool listing() const;
    /// 查询变更操作是否进行中
    bool mutating() const;
    /// 查询编辑取钥是否进行中
    bool fetchingSecret() const;
    /// 查询编辑取钥是否已成功
    bool editSecretReady() const;
    /// 获取凭证列表模型（当前实例已签发凭证）
    CredentialListModel *credentialListModel() const;

    /**
     * @brief 签发临时凭证（QML 友好入口）
     * @param instanceName         目标实例名（实例选择器）
     * @param ttlSeconds           凭证有效期（秒），必须大于 0
     * @param groupsCsv            逗号分隔的 ACL 组（可空）
     * @param allowRelay           是否允许通过该凭证节点中继
     * @param allowedProxyCidrsCsv 逗号分隔的允许代理 CIDR（可空）
     * @param credentialId         自定义凭证 ID（可空）
     * @param reusable             是否允许多个节点并发复用
     */
    Q_INVOKABLE void generateCredential(const QString &instanceName,
                                        int ttlSeconds,
                                        const QString &groupsCsv,
                                        bool allowRelay,
                                        const QString &allowedProxyCidrsCsv,
                                        const QString &credentialId,
                                        bool reusable);

    /**
     * @brief 编辑凭证前自动获取原密钥（QML 友好入口）
     *
     * 复用 generate_credential（ttl=1、携带原 credential_id）：当凭证已存在且
     * 未过期时，服务端直接返回原密钥而不改变凭证内容。取到的密钥仅暂存于本
     * 类（不展示给用户），成功后置 editSecretReady 供提交时留空自动回填。
     * 仅 admin 节点（持有 network_secret）可执行。
     *
     * @param instanceName 目标实例名（实例选择器）
     * @param credentialId 待编辑的凭证 ID
     */
    Q_INVOKABLE void prepareEdit(const QString &instanceName, const QString &credentialId);

    /**
     * @brief 查询当前实例已签发的全部临时凭证（QML 友好入口）
     * @param instanceName 目标实例名（实例选择器）
     */
    Q_INVOKABLE void listCredentials(const QString &instanceName);

    /**
     * @brief 新增/更新临时凭证（QML 友好入口）
     * @param instanceName         目标实例名（实例选择器）
     * @param credentialId         凭证 ID，必填非空
     * @param credentialSecret     X25519 私钥的 Base64 编码（协议必填，list 不返回 secret）
     * @param groupsCsv            逗号分隔的 ACL 组（可空）
     * @param allowRelay           是否允许通过该凭证节点中继
     * @param allowedProxyCidrsCsv 逗号分隔的允许代理 CIDR（可空）
     * @param expiryUnix           到期 Unix 时间戳（秒），必须晚于当前时间
     * @param reusable             是否允许多个节点并发复用
     */
    Q_INVOKABLE void upsertCredential(const QString &instanceName,
                                      const QString &credentialId,
                                      const QString &credentialSecret,
                                      const QString &groupsCsv,
                                      bool allowRelay,
                                      const QString &allowedProxyCidrsCsv,
                                      qint64 expiryUnix,
                                      bool reusable);

    /**
     * @brief 撤销临时凭证（QML 友好入口）
     * @param instanceName 目标实例名（实例选择器）
     * @param credentialId 待撤销的凭证 ID
     */
    Q_INVOKABLE void revokeCredential(const QString &instanceName,
                                      const QString &credentialId);

signals:
    /// 服务操作状态机变化（刷新 busy/generating/listing/mutating 派生属性）
    void operationChanged();
    /// 编辑取钥进行中状态变化
    void fetchingSecretChanged();
    /// 编辑取钥成功状态变化
    void editSecretReadyChanged();
    /// 签发成功：凭证 ID、Base64 私钥、过期 Unix 时间戳
    void generateSucceeded(const QString &credentialId,
                           const QString &credentialSecret,
                           qint64 expiryUnix);
    /// 签发失败：message 描述失败原因
    void generateFailed(const QString &message);

    /// 列表查询成功
    void listSucceeded();
    /// 列表查询失败：message 描述失败原因
    void listFailed(const QString &message);

    /// 新增/更新成功：changed 为 true 表示产生变更
    void upsertSucceeded(bool changed);
    /// 新增/更新失败：message 描述失败原因
    void upsertFailed(const QString &message);
    /// 撤销成功：success 为 false 表示凭证不存在
    void revokedSucceeded(bool success);
    /// 撤销失败：message 描述失败原因
    void revokedFailed(const QString &message);

private:
    /// 将逗号分隔字符串拆分为去空白的字符串列表
    static QStringList splitCsv(const QString &csv);

    /// 设置编辑取钥成功状态并发射信号
    void setEditSecretReady(bool v);

    CredentialService *m_service = nullptr; ///< 临时凭证服务（非所有权）
    bool m_editSecretReady = false;         ///< 编辑取钥成功标志
    QString m_pendingEditId;                ///< 正在取钥的凭证 ID（空表示无编辑取钥会话）
    QString m_editSecret;                   ///< 暂存的原密钥（不对外暴露）
    QString m_editSecretForId;              ///< 暂存密钥对应的凭证 ID
};
