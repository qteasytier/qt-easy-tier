/**
 * @file CredentialViewModel.h
 * @brief 临时节点密钥 ViewModel（运行状态页，薄壳）
 *
 * 为运行状态页"添加临时节点密钥"功能提供 QML 入口，
 * 全部业务逻辑委托应用服务层 CredentialService 执行，本类只做转发。
 */
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class CredentialService;

/** @brief 临时节点密钥 ViewModel，供 QML 签发安全模式临时凭证（薄壳转发） */
class CredentialViewModel : public QObject {
    Q_OBJECT

    /// 生成操作是否进行中（进行中时禁用按钮）
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)

public:
    /**
     * @brief 构造临时凭证 ViewModel
     * @param service 临时凭证服务（应用服务层，非所有权）
     * @param parent  父对象
     */
    explicit CredentialViewModel(CredentialService *service, QObject *parent = nullptr);

    /// 查询生成操作是否进行中
    bool busy() const;

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
    /// 将逗号分隔字符串拆分为去空白的字符串列表
    static QStringList splitCsv(const QString &csv);

    CredentialService *m_service = nullptr; ///< 临时凭证服务（非所有权）
};
