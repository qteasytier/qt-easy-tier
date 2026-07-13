/**
 * @file FrameProtocol.cpp
 * @brief 4 字节大端长度前缀帧协议实现
 *
 * 实现帧的编码（封包）和解码（拆包）。
 *
 * 封包流程：
 *   [payload 原始字节] → 计算长度 → 追加 4B 大端长度头 → 完整帧
 *
 * 拆包流程（循环处理，支持粘包）：
 *   [累积缓冲区] → 读 4B 长度 → 检查数据是否足够 → 提取 payload → 移除已消费字节
 *                  ↑不足：break 等待下次数据
 *                  ↑足够：提取后继续循环检查下一帧
 */

#include "FrameProtocol.h"
#include "core/util/LogHelper.h"

// 帧 payload 最大长度限制，防止恶意帧造成 OOM
static constexpr quint32 kMaxFrameSize = 16 * 1024 * 1024;  // 16 MiB

/**
 * @brief 将 payload 编码为包含长度头的帧
 *
 * 步骤：
 *   1. 获取 payload 的字节长度，转为 quint32
 *   2. 预分配 4 + payload.size() 字节容量
 *   3. 按大端序（Big Endian）逐字节写入长度：
 *      帧[0] = (len >> 24) & 0xFF  // 最高字节
 *      帧[1] = (len >> 16) & 0xFF
 *      帧[2] = (len >> 8)  & 0xFF
 *      帧[3] =  len        & 0xFF  // 最低字节
 *   4. 追加 payload 原始字节
 *
 * @param payload 要封装的 JSON 字节
 * @return 完整帧字节
 */
QByteArray FrameProtocol::encode(const QByteArray &payload)
{
    // 1. 获取 payload 长度
    const quint32 len = static_cast<quint32>(payload.size());

    QByteArray frame;
    // 2. 预分配空间，避免多次 realloc
    frame.reserve(4 + payload.size());

    // 3. 按大端序写入 4 字节长度头
    frame.append(static_cast<char>((len >> 24) & 0xFF));  // 字节0: 最高位
    frame.append(static_cast<char>((len >> 16) & 0xFF));  // 字节1
    frame.append(static_cast<char>((len >> 8)  & 0xFF));  // 字节2
    frame.append(static_cast<char>( len        & 0xFF));  // 字节3: 最低位

    // 4. 追加 payload
    frame.append(payload);

    return frame;
}

/**
 * @brief 从缓冲区解码所有完整帧
 *
 * 拆包循环：
 *   1. 检查缓冲区是否足够 4 字节（长度头最小尺寸）
 *      → 不足：退出循环
 *   2. 从缓冲区前 4 字节按大端序解析 payload 长度：
 *      len = buf[0]<<24 | buf[1]<<16 | buf[2]<<8 | buf[3]
 *   3. 检查缓冲区是否有 4 + len 字节
 *      → 不足：半包，退出循环等待更多数据
 *   4. 提取 payload（buffer.mid(4, len)），追加到结果列表
 *   5. 从缓冲区移除已消费的帧（buffer.remove(0, 4 + len)）
 *   6. 继续循环，处理可能的下一帧（粘包处理）
 *
 * @param buffer [输入/输出] 接收缓冲区
 * @return 所有完整帧的 payload 列表
 */
QList<QByteArray> FrameProtocol::decode(QByteArray &buffer)
{
    QList<QByteArray> result;

    // 循环拆解，一次可能解析多帧（粘包情况）
    while (buffer.size() >= 4) {
        // --- 步骤 2: 解析大端 4 字节长度头 ---
        // 将每个字节转为 quint8 避免符号扩展问题（char 可能是 signed）
        const quint32 len = (static_cast<quint8>(buffer[0]) << 24) |
                            (static_cast<quint8>(buffer[1]) << 16) |
                            (static_cast<quint8>(buffer[2]) << 8)  |
                             static_cast<quint8>(buffer[3]);

        // 帧长上限校验：超长帧为垃圾数据，清空缓冲区避免无限循环
        if (len > kMaxFrameSize) {
            LogHelper::logWarning(QStringLiteral("frame too large %1 bytes, discarding buffer").arg(len), "FrameProtocol");
            buffer.clear();
            return result;
        }

        // --- 步骤 3: 检查数据完整性（半包检测） ---
        // 如果缓冲区数据不足 4 + len，说明帧不完整，等待下次数据到达
        if (static_cast<quint32>(buffer.size()) < 4 + len)
            break;  // 半包，退出循环

        // --- 步骤 4: 提取 payload ---
        // mid(4, len) 从第 5 字节（索引 4）开始取 len 字节
        result.append(buffer.mid(4, len));

        // --- 步骤 5: 移除已消费的帧数据 ---
        // 从缓冲区头部删除 4 字节长度头 + len 字节 payload
        buffer.remove(0, 4 + len);

        // 循环继续，处理下一帧（粘包情况）
    }

    return result;
}
