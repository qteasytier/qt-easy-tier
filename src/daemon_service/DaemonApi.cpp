/**
 * @file DaemonApi.cpp
 * @brief daemon API 调用封装实现
 *
 * 将高层 API 方法映射为 DaemonClient::call() 的 JSON-RPC 调用。
 * 每个方法封装了对应的方法名字符串和参数构造逻辑，
 * 调用方只需关心业务语义，无需了解底层协议细节。
 *
 * 安全模式临时凭证相关方法统一经私有桥接 callBridge() 走 daemon 的 call_json_rpc，
 * 服务名、方法名与 Base64 信封编解码只在本文件内出现。
 */
#include "DaemonApi.h"

#include "DaemonClient.h"

#include <QByteArray>
#include <QException>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QPromise>

#include <memory>
#include <utility>

/**
 * @brief daemon 响应信封解码失败异常
 *
 * 当 call_json_rpc 返回的 response 字段不是合法 Base64/JSON 时抛出，
 * 经 QPromise 透传给调用方；消息与改造前的解码失败提示保持一致。
 */
struct BridgeDecodeException : public QException {
    QString msg;
    explicit BridgeDecodeException(QString m) : msg(std::move(m)) {}
    void raise() const override { throw *this; }
    BridgeDecodeException *clone() const override { return new BridgeDecodeException(*this); }
    const char *what() const noexcept override
    {
        // 缓存 UTF-8 表示，供上层 QException::what() 读取具体错误文本
        m_msgUtf8 = msg.toUtf8();
        return m_msgUtf8.constData();
    }

private:
    mutable QByteArray m_msgUtf8; ///< what() 的 UTF-8 缓存
};

/**
 * @brief 构造函数：保存 DaemonClient 引用
 * @param client daemon IPC 客户端指针
 * @param parent Qt 父对象
 */
DaemonApi::DaemonApi(DaemonClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
}

/**
 * @brief 解析配置 → daemon RPC: parse_config
 * @param payload 配置 JSON 对象
 * @return 异步解析结果
 */
QFuture<QJsonObject> DaemonApi::parseConfig(const QJsonObject &payload)
{
    return m_client->call(QStringLiteral("parse_config"), payload);
}

/**
 * @brief 运行网络实例 → daemon RPC: run_network_instance
 * @param payload 实例配置 JSON 对象
 * @return 异步运行结果
 */
QFuture<QJsonObject> DaemonApi::runNetworkInstance(const QJsonObject &payload)
{
    return m_client->call(QStringLiteral("run_network_instance"), payload);
}

/**
 * @brief 删除网络实例 → daemon RPC: delete_network_instance
 * @param instanceName 实例名称
 * @return 异步删除结果
 *
 * daemon 接口要求 inst_names 参数为 JSON 数组（支持批量），
 * 此处将单个实例名包装为单元素数组传递。
 */
QFuture<QJsonObject> DaemonApi::deleteNetworkInstance(const QString &instanceName)
{
    QJsonArray names;
    names.append(instanceName);
    return m_client->call(QStringLiteral("delete_network_instance"),
                          QJsonObject{{QStringLiteral("inst_names"), names}});
}

/**
 * @brief 列出所有实例 → daemon RPC: list_instances
 * @return 异步实例列表
 */
QFuture<QJsonObject> DaemonApi::listInstances()
{
    return m_client->call(QStringLiteral("list_instances"), QJsonObject{});
}

/**
 * @brief 采集网络信息 → daemon RPC: collect_network_infos
 * @param maxLength 最大采集长度
 * @return 异步网络信息
 */
QFuture<QJsonObject> DaemonApi::collectNetworkInfos(int maxLength)
{
    return m_client->call(QStringLiteral("collect_network_infos"),
                          QJsonObject{{QStringLiteral("max_length"), maxLength}});
}

QFuture<QJsonObject> DaemonApi::setAutoReconnect(bool enabled)
{
    return m_client->call(QStringLiteral("set_auto_reconnect"),
                          QJsonObject{{QStringLiteral("enabled"), enabled}});
}

QFuture<QJsonObject> DaemonApi::getAutoReconnect()
{
    return m_client->call(QStringLiteral("get_auto_reconnect"), QJsonObject{});
}

/**
 * @brief 签发安全模式临时凭证 → call_json_rpc(CredentialManageRpcService.generate_credential)
 * @param payload protobuf JSON 格式的请求体
 * @return 异步结果，result 为已解码的 protobuf JSON 响应对象
 */
QFuture<QJsonObject> DaemonApi::generateCredential(const QJsonObject &payload)
{
    return callBridge(QStringLiteral("api.instance.CredentialManageRpcService"),
                      QStringLiteral("generate_credential"), payload);
}

/**
 * @brief 查询实例已签发的临时凭证 → call_json_rpc(CredentialManageRpcService.list_credentials)
 * @param payload protobuf JSON 格式的请求体（仅含实例选择器）
 * @return 异步结果，result 为已解码的 protobuf JSON 响应对象
 */
QFuture<QJsonObject> DaemonApi::listCredentials(const QJsonObject &payload)
{
    return callBridge(QStringLiteral("api.instance.CredentialManageRpcService"),
                      QStringLiteral("list_credentials"), payload);
}

/**
 * @brief 新增/更新临时凭证 → call_json_rpc(CredentialManageRpcService.upsert_credential)
 * @param payload protobuf JSON 格式的请求体
 * @return 异步结果，result 为已解码的 protobuf JSON 响应对象
 */
QFuture<QJsonObject> DaemonApi::upsertCredential(const QJsonObject &payload)
{
    return callBridge(QStringLiteral("api.instance.CredentialManageRpcService"),
                      QStringLiteral("upsert_credential"), payload);
}

/**
 * @brief 撤销临时凭证 → call_json_rpc(CredentialManageRpcService.revoke_credential)
 * @param payload protobuf JSON 格式的请求体（实例选择器 + 凭证 ID）
 * @return 异步结果，result 为已解码的 protobuf JSON 响应对象
 */
QFuture<QJsonObject> DaemonApi::revokeCredential(const QJsonObject &payload)
{
    return callBridge(QStringLiteral("api.instance.CredentialManageRpcService"),
                      QStringLiteral("revoke_credential"), payload);
}

/**
 * @brief 经 daemon 的 call_json_rpc 桥接转发请求，并解码响应信封
 *
 * 流程：protobuf JSON 请求体 → Base64 → call_json_rpc → 等待响应 →
 * 解码 response 字段（Base64 的 protobuf JSON）→ 交付给调用方。
 *
 * @param serviceName RPC 服务名
 * @param methodName  RPC 方法名（snake_case）
 * @param payloadJson protobuf JSON 格式的请求体
 * @return 异步结果 QFuture；daemon 错误与解码错误均以 QException 形式透传
 */
QFuture<QJsonObject> DaemonApi::callBridge(const QString &serviceName,
                                           const QString &methodName,
                                           const QJsonObject &payloadJson)
{
    // daemon IPC 约定：call_json_rpc 的 payload 字段按 Base64 编码传输；
    // 凭证管理服务不使用服务注册域，domain_name 固定为空串。
    const QString payloadB64 =
        QString::fromLatin1(QJsonDocument(payloadJson).toJson(QJsonDocument::Compact).toBase64());

    // 用 QPromise 包装原始 future：等 daemon 响应到达后再解码信封
    auto promise = std::make_shared<QPromise<QJsonObject>>();
    promise->start();

    auto *watcher = new QFutureWatcher<QJsonObject>(this);
    connect(watcher, &QFutureWatcher<QJsonObject>::finished, this, [watcher, promise]() {
        watcher->deleteLater();
        try {
            const QJsonObject envelope = watcher->result();
            const QByteArray response = QByteArray::fromBase64(
                envelope.value(QStringLiteral("response")).toString().toLatin1());

            QJsonParseError parseError;
            const QJsonDocument responseDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error != QJsonParseError::NoError)
                throw BridgeDecodeException(QStringLiteral("解析 daemon 响应失败"));

            promise->addResult(responseDoc.object());
            promise->finish();
        } catch (const QException &e) {
            // daemon 报错（如 daemon error / 超时）与解码失败都经 future 透传
            promise->setException(e);
            promise->finish();
        }
    });
    watcher->setFuture(m_client->call(QStringLiteral("call_json_rpc"),
                                      QJsonObject{{QStringLiteral("service_name"), serviceName},
                                                  {QStringLiteral("method_name"), methodName},
                                                  {QStringLiteral("domain_name"), QString()},
                                                  {QStringLiteral("payload"), payloadB64}}));

    return promise->future();
}
