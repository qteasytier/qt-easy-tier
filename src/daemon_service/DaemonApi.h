/**
 * @file DaemonApi.h
 * @brief daemon API 调用封装
 *
 * 对 DaemonClient 的二次封装，将常用的 daemon RPC 调用包装为语义明确的方法。
 * 每个方法对应一个 daemon JSON-RPC 方法名，调用方无需手动构造方法名字符串和参数。
 *
 * 需要经 daemon 内嵌 RPC 桥接（call_json_rpc）转发的调用（当前为安全模式临时凭证），
 * 其服务名、方法名与 Base64 信封编解码只在本类实现内出现，不对外暴露通用 JSON-RPC 入口。
 */
#pragma once

#include <QFuture>
#include <QJsonObject>
#include <QObject>
#include <QString>

class DaemonClient;

/**
 * @class DaemonApi
 * @brief daemon JSON-RPC 接口的高层封装
 *
 * 提供解析配置、运行/删除网络实例、列出实例、采集网络信息等常用接口。
 * 所有方法均返回 QFuture<QJsonObject>，支持异步等待结果。
 *
 * 设计要点：
 * - 不持有 DaemonClient 所有权，仅通过外部传入的指针调用
 * - 每个方法内部调用 DaemonClient::call()，屏蔽了 method 字符串的拼写差异
 * - 通过此抽象层，调用方不需要了解 daemon 的底层 JSON-RPC 方法名
 */
class DaemonApi : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造 API 封装
     * @param client 已初始化的 DaemonClient 指针（非空，由外部管理生命周期）
     * @param parent Qt 父对象
     */
    explicit DaemonApi(DaemonClient *client, QObject *parent = nullptr);

    /**
     * @brief 解析网络配置文件
     * @param payload 包含配置内容的 JSON 对象
     * @return 解析结果的 QFuture
     */
    QFuture<QJsonObject> parseConfig(const QJsonObject &payload);

    /**
     * @brief 运行网络实例
     * @param payload 包含实例配置的 JSON 对象
     * @return 运行结果的 QFuture
     */
    QFuture<QJsonObject> runNetworkInstance(const QJsonObject &payload);

    /**
     * @brief 删除网络实例
     * @param instanceName 要删除的实例名称
     * @return 删除结果的 QFuture
     */
    QFuture<QJsonObject> deleteNetworkInstance(const QString &instanceName);

    /**
     * @brief 列出所有运行中的网络实例
     * @return 实例列表的 QFuture
     */
    QFuture<QJsonObject> listInstances();

    /**
     * @brief 采集网络信息
     * @param maxLength 最大采集长度
     * @return 网络信息的 QFuture
     */
    QFuture<QJsonObject> collectNetworkInfos(int maxLength);

    QFuture<QJsonObject> setAutoReconnect(bool enabled);
    QFuture<QJsonObject> getAutoReconnect();

    /**
     * @brief 签发安全模式临时凭证 → daemon RPC: call_json_rpc(CredentialManageRpcService.generate_credential)
     *
     * 请求体由调用方按业务语义构造为 protobuf JSON（snake_case 字段，含实例选择器）；
     * 服务名、方法名与请求/响应信封的 Base64 编解码均在本类实现内完成。
     *
     * @param payload protobuf JSON 格式的请求体
     * @return 异步结果 QFuture，result 为已解码的 protobuf JSON 响应对象；
     *         daemon 报错时以 QException 形式抛出
     */
    QFuture<QJsonObject> generateCredential(const QJsonObject &payload);

    /**
     * @brief 查询实例已签发的临时凭证 → daemon RPC: call_json_rpc(CredentialManageRpcService.list_credentials)
     * @param payload protobuf JSON 格式的请求体（仅含实例选择器）
     * @return 异步结果 QFuture，result 为已解码的 protobuf JSON 响应对象
     */
    QFuture<QJsonObject> listCredentials(const QJsonObject &payload);

    /**
     * @brief 新增/更新临时凭证 → daemon RPC: call_json_rpc(CredentialManageRpcService.upsert_credential)
     * @param payload protobuf JSON 格式的请求体
     * @return 异步结果 QFuture，result 为已解码的 protobuf JSON 响应对象
     */
    QFuture<QJsonObject> upsertCredential(const QJsonObject &payload);

    /**
     * @brief 撤销临时凭证 → daemon RPC: call_json_rpc(CredentialManageRpcService.revoke_credential)
     * @param payload protobuf JSON 格式的请求体（实例选择器 + 凭证 ID）
     * @return 异步结果 QFuture，result 为已解码的 protobuf JSON 响应对象
     */
    QFuture<QJsonObject> revokeCredential(const QJsonObject &payload);

private:
    /**
     * @brief 经 daemon 的 call_json_rpc 桥接调用 easytier 内嵌 RPC 服务（私有实现，非对外接口）
     *
     * 负责将 protobuf JSON 请求体编码为 Base64 后透传给 daemon，并把响应信封中
     * Base64 编码的 response 字段解码回 protobuf JSON 对象；daemon 错误原样透传。
     *
     * @param serviceName RPC 服务名（如 "api.instance.CredentialManageRpcService"）
     * @param methodName  RPC 方法名（snake_case）
     * @param payloadJson protobuf JSON 格式的请求体
     * @return 异步结果 QFuture，result 为解码后的响应对象
     */
    QFuture<QJsonObject> callBridge(const QString &serviceName,
                                    const QString &methodName,
                                    const QJsonObject &payloadJson);

    DaemonClient *m_client = nullptr; ///< daemon IPC 客户端指针（外部管理生命周期）
};
