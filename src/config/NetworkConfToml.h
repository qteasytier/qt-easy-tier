/**
 * @file NetworkConfToml.h
 * @brief NetworkConf 的 TOML 序列化/反序列化自由函数
 *
 * 提供 NetworkConf ↔ TOML 字符串的转换能力。
 * 用于配置文件的导入/导出功能，与 toml.hpp (toml11) 库对接。
 * 所有函数均为纯函数，无副作用，无状态。
 */
#pragma once

#include <QString>
#include "NetworkConf.h"

namespace NetworkConfToml {

/**
 * @brief 将 NetworkConf 对象序列化为 TOML 格式字符串
 * @param conf 源配置对象
 * @param includeInstanceName 是否在输出中包含 instance_name（导出时通常为 true）
 * @return TOML 格式的配置文本
 */
QString toToml(const NetworkConf &conf, bool includeInstanceName = false);

/**
 * @brief 从 TOML 字符串反序列化为 NetworkConf 对象
 * @param toml TOML 格式的配置文本
 * @param instanceName 分配给新对象的实例名
 * @return 反序列化后的配置对象（解析失败时返回含默认值的对象）
 */
NetworkConf fromToml(const QString &toml, const QString &instanceName);

} // namespace NetworkConfToml
