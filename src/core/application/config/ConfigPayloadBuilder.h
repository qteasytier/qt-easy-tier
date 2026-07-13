/** @file ConfigPayloadBuilder.h @brief 配置载荷构建器，将 NetworkConf 转换为 TOML / Base64 / daemon JSON 格式 */
#pragma once

#include <QJsonObject>
#include <QString>

class NetworkConf;

/** @brief 配置载荷构建器，负责将网络配置序列化为不同传输格式 */
class ConfigPayloadBuilder
{
public:
    /** @brief 将配置序列化为 TOML 字符串 @param conf 网络配置 @return TOML 字符串 */
    static QString toToml(const NetworkConf &conf);
    /** @brief 将配置序列化为 Base64 编码的 TOML 字符串 @param conf 网络配置 @return Base64 TOML 字符串 */
    static QString toBase64Toml(const NetworkConf &conf);
    /** @brief 构建 daemon 可用的配置 JSON 载荷 @param conf 网络配置 @return 包含 cfg_str 字段的 JSON 对象 */
    static QJsonObject daemonConfigPayload(const NetworkConf &conf);
};
