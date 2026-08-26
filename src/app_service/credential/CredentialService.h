/**
 * @file CredentialService.h
 * @brief 临时节点密钥（安全模式临时凭证）管理服务（应用服务层）
 *
 * 桥接 UI 层与 daemon 的 CredentialManageRpcService：
 * - generateCredential：签发安全模式临时凭证（generate_credential）
 *
 * 生成的 credential_secret（Base64 私钥）分发给其他节点即可临时加入网络，
 * 典型用法：`easytier-core --network-name <name> --secure-mode --credential <secret> -p <url>`。
 *
 * 实现通过 DaemonApi::callJsonRpc 将 protobuf JSON 请求体透传给 daemon，
 * daemon 再在进程内调用 easytier-core 的 RPC 服务（IPC 路径已打通）。
 */
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class DaemonApi;

/** @brief 临时节点密钥（安全模式临时凭证）管理服务 */
class CredentialService : public QObject {
    Q_OBJECT

    /// 生成操作是否进行中（进行中时禁用重复触发）
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)

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

    /**
     * @brief 构造临时凭证服务
     * @param daemonApi daemon API（非所有权，由装配层管理生命周期）
     * @param parent    父对象
     */
    explicit CredentialService(DaemonApi *daemonApi, QObject *parent = nullptr);

    /// 查询生成操作是否进行中
    bool busy() const;

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

signals:
    /// 生成进行中状态变化
    void busyChanged();
    /// 签发成功：凭证 ID、Base64 私钥、过期 Unix 时间戳
    void generateSucceeded(const QString &credentialId,
                           const QString &credentialSecret,
                           qint64 expiryUnix);
    /// 签发失败：message 描述失败原因
    void generateFailed(const QString &message);

private:
    /// 设置生成进行中状态并发射信号
    void setBusy(bool busy);

    DaemonApi *m_daemonApi = nullptr; ///< daemon API（非所有权）
    bool m_busy = false;              ///< 生成进行中标志
};
