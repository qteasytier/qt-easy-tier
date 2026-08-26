/**
 * @file CredentialService.h
 * @brief 临时节点密钥（安全模式临时凭证）管理服务（应用服务层）
 *
 * 桥接 UI 层与 daemon 的 CredentialManageRpcService：
 * - generateCredential：签发安全模式临时凭证（generate_credential），
 *   编辑凭证时复用其取回原密钥（详见 CredentialViewModel::prepareEdit）
 * - listCredentials：查询当前实例已签发的临时凭证（list_credentials），填充凭证列表模型
 * - upsertCredential：新增/更新临时凭证（upsert_credential）
 * - revokeCredential：撤销临时凭证（revoke_credential）
 *
 * 生成的 credential_secret（Base64 私钥）分发给其他节点即可临时加入网络，
 * 典型用法：`easytier-core --network-name <name> --secure-mode --credential <secret> -p <url>`。
 *
 * 实现通过 DaemonApi::callJsonRpc 将 protobuf JSON 请求体透传给 daemon，
 * daemon 再在进程内调用 easytier-core 的 RPC 服务（IPC 路径已打通）。
 *
 * 状态管理：所有与 daemon 的通信操作共用单一操作状态机（CredentialOperation），
 * 同一时刻至多一个操作进行中；busy 属性由 operation 派生，UI 据此统一禁用重复触发。
 */
#pragma once

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>

class DaemonApi;
class CredentialListModel;

/** @brief 临时凭证服务的操作状态（统一状态机） */
enum class CredentialOperation {
    Idle = 0,   ///< 空闲（无操作进行中）
    Generate,   ///< 签发临时凭证（含编辑取钥复用）
    List,       ///< 查询凭证列表
    Upsert,     ///< 新增/更新凭证
    Revoke,     ///< 撤销凭证
};

/** @brief 判断是否处于忙碌状态（有操作进行中） @param op 操作状态 @return 是否忙碌 */
constexpr bool credentialOperationIsBusy(CredentialOperation op)
{
    return op != CredentialOperation::Idle;
}

Q_DECLARE_METATYPE(CredentialOperation)

/** @brief 临时节点密钥（安全模式临时凭证）管理服务 */
class CredentialService : public QObject {
    Q_OBJECT

    /// 是否有操作进行中（由 operation 派生）
    Q_PROPERTY(bool busy READ busy NOTIFY operationChanged FINAL)
    /// 当前操作状态（统一状态机）
    Q_PROPERTY(CredentialOperation operation READ operation NOTIFY operationChanged FINAL)

public:
    /** @brief 生成临时凭证的请求参数 */
    struct GenerateRequest {
        QString instanceName;        ///< 目标实例名（实例选择器）
        int ttlSeconds = 0;          ///< 凭证有效期（秒），必须大于 0
        QStringList groups;          ///< ACL 组（可空）
        bool allowRelay = false;     ///< 是否允许通过该凭证节点中继
        QStringList allowedProxyCidrs; ///< 允许代理的 CIDR（可空）
        QString credentialId;        ///< 自定义凭证 ID（可空，为空则省略该字段）
        bool reusable = true;        ///< 是否允许多个节点并发复用
    };

    /** @brief 新增/更新临时凭证的请求参数（credential_secret 必填，list 不返回 secret） */
    struct UpsertRequest {
        QString instanceName;          ///< 目标实例名（实例选择器）
        QString credentialId;          ///< 凭证 ID，必填非空；不存在则新增，存在则更新
        QString credentialSecret;      ///< 32 字节 X25519 私钥的 Base64 编码（协议必填）
        QStringList groups;            ///< ACL 组（可空）
        bool allowRelay = false;       ///< 是否允许通过该凭证节点中继
        QStringList allowedProxyCidrs; ///< 允许代理的 CIDR（可空）
        qint64 expiryUnix = 0;         ///< 到期 Unix 时间戳（秒），必须晚于当前时间
        bool reusable = true;          ///< 是否允许多个节点并发复用
    };

    /**
     * @brief 构造临时凭证服务
     * @param daemonApi daemon API（非所有权，由装配层管理生命周期）
     * @param parent    父对象
     */
    explicit CredentialService(DaemonApi *daemonApi, QObject *parent = nullptr);

    /// 查询当前操作状态
    CredentialOperation operation() const;
    /// 查询是否有操作进行中
    bool busy() const;

    /// 获取凭证列表模型（当前实例已签发凭证，本服务所有）
    CredentialListModel *credentialListModel() const;

    /**
     * @brief 异步签发临时凭证
     *
     * 内部构造 protobuf JSON 请求体（携带实例选择器），通过
     * DaemonApi::callJsonRpc 调用 daemon 的 generate_credential 方法。
     * 完成后发射 generateSucceeded / generateFailed 信号。
     *
     * @param request 生成参数
     */
    void generateCredential(const GenerateRequest &request);

    /**
     * @brief 异步查询当前实例已签发的全部临时凭证
     *
     * 调用 daemon 的 list_credentials 方法，成功后解析 credentials 数组并
     * 注入 credentialListModel，发射 listSucceeded；失败发射 listFailed。
     *
     * @param instanceName 目标实例名（实例选择器）
     */
    void listCredentials(const QString &instanceName);

    /**
     * @brief 异步新增/更新临时凭证
     *
     * 调用 daemon 的 upsert_credential 方法。协议要求 credential_secret 与
     * expiry_unix 均必填；凭证 ID 不存在则新增，存在则更新其授权约束。
     * 完成后发射 upsertSucceeded / upsertFailed 信号。
     *
     * @param request 新增/更新参数
     */
    void upsertCredential(const UpsertRequest &request);

    /**
     * @brief 异步撤销临时凭证
     *
     * 调用 daemon 的 revoke_credential 方法。成功后发射 revokedSucceeded，
     * success 为 false 表示凭证不存在；失败发射 revokedFailed。
     *
     * @param instanceName 目标实例名（实例选择器）
     * @param credentialId 待撤销的凭证 ID
     */
    void revokeCredential(const QString &instanceName, const QString &credentialId);

signals:
    /// 操作状态机变化（operation 或派生的 busy）
    void operationChanged();
    /// 签发成功：凭证 ID、Base64 私钥、过期 Unix 时间戳
    void generateSucceeded(const QString &credentialId,
                           const QString &credentialSecret,
                           qint64 expiryUnix);
    /// 签发失败：message 描述失败原因
    void generateFailed(const QString &message);

    /// 列表查询成功（credentialListModel 已刷新）
    void listSucceeded();
    /// 列表查询失败：message 描述失败原因
    void listFailed(const QString &message);

    /// 新增/更新成功：changed 为 true 表示产生变更，false 表示与已有内容一致
    void upsertSucceeded(bool changed);
    /// 新增/更新失败：message 描述失败原因
    void upsertFailed(const QString &message);
    /// 撤销成功：success 为 false 表示凭证不存在
    void revokedSucceeded(bool success);
    /// 撤销失败：message 描述失败原因
    void revokedFailed(const QString &message);

private:
    /// 切换到指定操作状态；仅 Idle 可转入新操作，操作完成/失败均回到 Idle
    void setOperation(CredentialOperation op);

    DaemonApi *m_daemonApi = nullptr; ///< daemon API（非所有权）
    CredentialOperation m_operation = CredentialOperation::Idle; ///< 当前操作状态
    CredentialListModel *m_credentialListModel = nullptr; ///< 凭证列表模型（本服务所有）
};
