/**
 * @file FrameProtocol.h
 * @brief 4 字节大端长度前缀帧协议
 *
 * 对应 daemon README 中定义的帧格式，用于在字节流（如 QLocalSocket）上
 * 传输变长 JSON 消息。协议通过在 payload 前追加 4 字节大端无符号整数
 * 作为长度字段，实现消息边界的自动识别。
 *
 * 帧格式（大端字节序）：
 * @code
 *   ┌───────────────────────────────────────┐
 *   │ Payload 长度 (4 bytes, BE uint32)     │  ← 长度头，不含自身的 4 字节
 *   ├───────────────────────────────────────┤
 *   │ Payload (JSON bytes, UTF-8 编码)      │  ← 实际消息内容
 *   └───────────────────────────────────────┘
 * @endcode
 *
 * 半包/粘包处理：decode() 方法以循环方式从缓冲区中提取完整帧，
 * 数据不足时自动保留在 buffer 中等待下次到达，多帧粘包时一次性拆解。
 *
 * 使用示例：
 * @code
 *   // 发送
 *   socket->write(FrameProtocol::encode(payload));
 *
 *   // 接收
 *   buffer.append(socket->readAll());
 *   const auto frames = FrameProtocol::decode(buffer);
 *   for (const auto &payload : frames) {
 *       // 处理每帧 payload
 *   }
 *   // buffer 中仅剩未完成的半包数据
 * @endcode
 *
 * @see DaemonClient  使用帧协议进行 IPC 通信
 */

#pragma once
#include <QByteArray>
#include <QList>

/**
 * @class FrameProtocol
 * @brief 字节流帧编解码器（纯静态方法）
 *
 * 提供 encode() 和 decode() 两个静态方法，分别负责：
 *   - encode: 将 payload 封装为 [4B长度头 + payload] 的完整帧
 *   - decode: 从缓冲区拆解所有完整帧，处理半包和粘包
 *
 * 该类无状态，无需实例化。
 */
class FrameProtocol {
public:
    /**
     * @brief 将 payload 封装为一帧
     *
     * 在 payload 前追加 4 字节大端无符号整数作为长度头。
     * 长度头编码的是 payload 的字节数，不含自身 4 字节。
     *
     * @param payload 消息体字节（UTF-8 编码的 JSON）
     * @return 完整帧字节（4B 长度头 + payload）
     *
     * @note payload 大小最多 2^32-1 字节（约 4GB），实际 JSON 消息远小于此值
     */
    static QByteArray encode(const QByteArray &payload);

    /**
     * @brief 从字节缓冲区解码所有完整帧
     *
     * 核心逻辑：
     *   1. 检查 buffer 是否至少有 4 字节（长度头的最小尺寸）
     *   2. 解析 4 字节大端 uint32 → 得到 payload 长度
     *   3. 检查 buffer 是否有足够数据（4 + 长度 + 1 字节？实际是 4 + len）
     *   4. 如果足够：提取 payload，从 buffer 移除已消费的字节，重复步骤 1
     *   5. 如果不够：break 退出循环（半包），等待下次数据到达
     *
     * 一次调用可以解析多帧（处理粘包），buffer 中只保留最后未完成的半包数据。
     *
     * @param buffer [输入/输出] 接收缓冲区
     *               输入: 累积的字节流（可能含多帧或半包）
     *               输出: 仅保留未消费的残留字节（半包尾部）
     * @return 完整帧的 payload 列表（按接收顺序排列）
     *
     * @note 没有对长度头进行上限校验，恶意或在异常情况下可导致等待大量数据。
     *       实际使用中 daemon 消息远小于典型上限（如 1MB），当前实现可满足需求。
     */
    static QList<QByteArray> decode(QByteArray &buffer);
};
