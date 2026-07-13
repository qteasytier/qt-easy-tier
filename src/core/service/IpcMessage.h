/**
 * @file IpcMessage.h
 * @brief JSON-RPC 风格 IPC 消息定义
 *
 * 与 qtet-daemon 守护进程通信所使用的消息结构。
 * 对应 daemon README 中描述的消息格式，支持三种消息类型：
 *
 * 请求消息（客户端 → daemon）：
 *   { "id": N, "method": "名称", "params": {...} }
 *
 * 成功响应（daemon → 客户端）：
 *   { "id": N, "method": "名称", "result": {...} }
 *
 * 错误响应（daemon → 客户端）：
 *   { "id": N, "method": "名称", "error": { "message": "..." } }
 *
 * 通知消息（daemon 主动推送，id = 0）：
 *   { "id": 0, "method": "名称", "params": {...} }
 *
 * 注意：
 *   - 通知消息的 id 固定为 0，用于区分请求-响应和主动推送
 *   - 错误响应通过 isError 标志和 errorMessage 字段表达
 *   - 不需要 method 字段时（如纯响应）可留空字符串
 *
 * @see DaemonClient  使用此消息进行 IPC 调用
 * @see FrameProtocol  消息通过帧协议在字节流上传输
 */

#pragma once
#include <QJsonObject>
#include <QString>
#include <QByteArray>

/**
 * @struct IpcMessage
 * @brief 单条 IPC 消息的完整表示（值类型，可复制）
 *
 * 这是一个纯数据 struct，没有继承 QObject，可以安全地按值传递和复制。
 * 提供静态工厂方法构造不同类型消息，以及 fromJson/toJson 序列化方法。
 *
 * 字段说明：
 *   - id:           请求 ID（客户端自增分配），通知消息为 0
 *   - method:       方法名（如 "list_instances"）
 *   - params:       请求参数 / 通知数据
 *   - result:       成功响应的返回数据
 *   - errorMessage: 错误响应的错误消息文本
 *   - isError:      是否为错误响应
 */
struct IpcMessage {
    int id = 0;                    ///< 请求/响应 ID，通知时为 0
    QString method;                ///< 方法名
    QJsonObject params;            ///< 请求参数或通知数据
    QJsonObject result;            ///< 成功响应结果
    QString errorMessage;          ///< 错误消息文本（仅 isError 时有效）
    bool isError = false;          ///< 是否为错误响应
    bool parseOk = true;           ///< JSON 解析是否成功（非法帧时为 false）

    /**
     * @brief 构造请求消息
     *
     * 客户端调用 daemon 方法时使用。
     *
     * @param id 自增请求 ID，由 DaemonClient 管理
     * @param method 方法名
     * @param params 请求参数
     * @return 构造好的 IpcMessage（id > 0, method 非空, 含 params）
     */
    static IpcMessage request(int id, const QString &method, const QJsonObject &params);

    /**
     * @brief 构造成功响应消息
     *
     * daemon 处理请求成功后返回。
     *
     * @param id 匹配的请求 ID
     * @param method 方法名
     * @param result 处理结果
     * @return 构造好的 IpcMessage（含 result，不含 error）
     */
    static IpcMessage response(int id, const QString &method, const QJsonObject &result);

    /**
     * @brief 构造错误响应消息
     *
     * daemon 处理请求失败时返回。
     *
     * @param id 匹配的请求 ID
     * @param method 方法名
     * @param message 错误描述
     * @return 构造好的 IpcMessage（isError = true，含 errorMessage）
     */
    static IpcMessage error(int id, const QString &method, const QString &message);

    /**
     * @brief 从 JSON 字节数组解析 IPC 消息
     *
     * 解析接收到的帧 payload 为 IpcMessage 结构。
     * 自动检测消息类型：通过是否存在 "error" / "result" 字段区分。
     *
     * **健壮性注意**：当前实现不校验 JSON 有效性。
     * 如果输入不是合法 JSON，QJsonDocument::fromJson() 返回 null 文档，
     * 后续 QJsonObject 操作在空对象上进行，所有字段取默认值。
     * 调用方应确保传入的是合法 JSON 数据。
     *
     * @param json UTF-8 编码的 JSON 字节
     * @return 解析后的 IpcMessage
     */
    static IpcMessage fromJson(const QByteArray &json);

    /**
     * @brief 将 IPC 消息序列化为紧凑 JSON 字节
     *
     * 生成符合帧协议的 payload 字节，可直接通过 FrameProtocol::encode() 封装。
     * 使用 QJsonDocument::Compact 格式（无缩进、无换行）。
     *
     * 序列化规则：
     *   - 始终输出 id 和 method 字段
     *   - params 非空时输出 params 字段
     *   - isError = true → 输出 error: { message: "..." }
     *   - result 非空时输出 result 字段
     *
     * @return UTF-8 编码的紧凑 JSON 字节
     */
    QByteArray toJson() const;
};
