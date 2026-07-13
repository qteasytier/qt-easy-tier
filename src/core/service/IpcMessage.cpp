/**
 * @file IpcMessage.cpp
 * @brief IPC 消息的工厂构造与序列化实现
 *
 * 实现 IpcMessage 的工厂方法和 JSON 序列化/反序列化。
 *
 * JSON-RPC 风格消息的三种格式：
 *
 * 请求（客户端 → daemon）：
 *   { "id": 1, "method": "list_instances", "params": {} }
 *
 * 成功响应（daemon → 客户端）：
 *   { "id": 1, "method": "list_instances", "result": { ... } }
 *
 * 错误响应（daemon → 客户端）：
 *   { "id": 1, "method": "list_instances", "error": { "message": "something went wrong" } }
 *
 * 通知（daemon → 客户端，id = 0）：
 *   { "id": 0, "method": "instance_changed", "params": { ... } }
 */

#include "IpcMessage.h"
#include <QJsonDocument>

/**
 * @brief 构造请求消息
 *
 * 填充 id、method、params 三个字段。
 * 不设置 result 和 error 相关字段。
 */
IpcMessage IpcMessage::request(int id, const QString &method, const QJsonObject &params)
{
    IpcMessage msg;
    msg.id = id;
    msg.method = method;
    msg.params = params;
    return msg;
}

/**
 * @brief 构造成功响应消息
 *
 * 填充 id、method、result 三个字段。
 * isError 保持默认值 false。
 */
IpcMessage IpcMessage::response(int id, const QString &method, const QJsonObject &result)
{
    IpcMessage msg;
    msg.id = id;
    msg.method = method;
    msg.result = result;
    return msg;
}

/**
 * @brief 构造错误响应消息
 *
 * 填充 id、method、errorMessage 字段，并设置 isError = true。
 * 注意：不填充 result 字段。
 */
IpcMessage IpcMessage::error(int id, const QString &method, const QString &message)
{
    IpcMessage msg;
    msg.id = id;
    msg.method = method;
    msg.isError = true;
    msg.errorMessage = message;
    return msg;
}

/**
 * @brief 从 JSON 字节解析 IPC 消息
 *
 * 解析流程：
 *   1. QJsonDocument::fromJson(json) → 解析 UTF-8 JSON
 *      **未校验 doc.isNull()**：如果输入非法 JSON，返回空文档，后续操作在空对象上进行
 *   2. doc.object() → 获取根 JSON 对象
 *   3. 逐字段提取：
 *      - id:      obj["id"].toInt()     → 默认为 0
 *      - method:  obj["method"].toString() → 默认为 ""
 *      - params:  若存在则提取，否则为空对象
 *      - result:  若存在则提取，否则为空对象
 *      - error:   若存在则提取 error.message，设置 isError = true
 *
 * 健壮性分析：
 *   - 非 JSON 输入：返回默认构造的 IpcMessage（id=0, method=""），
 *     会被 DaemonClient::onReadyRead() 当作通知消息处理并发出信号
 *   - 缺少字段：使用 QJsonValue 的默认转换值（0/空字符串/空对象）
 *   - 类型不匹配：QJsonValue 提供默认值（如字符串"123"的 toInt() 返回 0）
 *
 * @param json UTF-8 编码的 JSON 字节数组
 * @return 解析后的 IpcMessage 结构
 */
IpcMessage IpcMessage::fromJson(const QByteArray &json)
{
    // 步骤 1: 解析 JSON
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (doc.isNull()) {
        // 非法 JSON 输入：返回带 parseOk=false 的消息，防止被当作通知处理
        IpcMessage msg;
        msg.parseOk = false;
        msg.isError = true;
        msg.errorMessage = QStringLiteral("invalid json");
        return msg;
    }

    // 步骤 2: 获取根对象
    const QJsonObject obj = doc.object();

    IpcMessage msg;

    // 步骤 3: 逐字段提取
    msg.id = obj.value("id").toInt();           // 默认为 0（与通知消息的 id=0 约定一致）
    msg.method = obj.value("method").toString(); // 默认为空字符串

    // 步骤 4: 提取可选字段
    if (obj.contains("params")) {
        // 只提取 params 字段，类型不对时 toObject() 返回空对象
        msg.params = obj.value("params").toObject();
    }

    if (obj.contains("result")) {
        // 只提取 result 字段
        msg.result = obj.value("result").toObject();
    }

    // 步骤 5: 检测错误响应
    if (obj.contains("error")) {
        msg.isError = true;
        // 错误响应格式：{ "error": { "message": "..." } }
        msg.errorMessage = obj.value("error").toObject().value("message").toString();
    }

    return msg;
}

/**
 * @brief 将 IPC 消息序列化为紧凑 JSON 字节数组
 *
 * 序列化流程：
 *   1. 创建 QJsonObject，始终写入 id 和 method
 *   2. 如果 params 非空 → 写入 "params" 字段
 *   3. 如果 isError 为 true → 构造 { "error": { "message": "..." } } 写入
 *   4. 如果 result 非空 → 写入 "result" 字段
 *   5. 通过 QJsonDocument(obj).toJson(Compact) 转为紧凑 JSON 字节
 *
 * 紧凑格式示例（无缩进和换行）：
 *   {"id":1,"method":"list_instances","params":{}}
 *   {"id":1,"method":"list_instances","result":{"instances":[]}}
 *   {"id":1,"method":"list_instances","error":{"message":"daemon error: not found"}}
 *
 * @return UTF-8 编码的紧凑 JSON 字节数组
 */
QByteArray IpcMessage::toJson() const
{
    QJsonObject obj;

    // 步骤 1: 始终写入 id 和 method
    obj["id"] = id;
    obj["method"] = method;

    // 步骤 2: 写入 params（仅当非空）
    if (!params.isEmpty()) {
        obj["params"] = params;
    }

    // 步骤 3-4: 写入 result 或 error
    if (isError) {
        // 构造错误响应对象
        QJsonObject err;
        err["message"] = errorMessage;
        obj["error"] = err;
    } else if (!result.isEmpty()) {
        // 构造成功响应
        obj["result"] = result;
    }

    // 步骤 5: 转为紧凑 JSON 字节
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
