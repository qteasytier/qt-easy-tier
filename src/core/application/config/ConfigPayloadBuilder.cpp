/** @file ConfigPayloadBuilder.cpp @brief ConfigPayloadBuilder 实现 */
#include "ConfigPayloadBuilder.h"

#include "core/config/NetworkConf.h"
#include "core/config/NetworkConfToml.h"

#include <QByteArray>

QString ConfigPayloadBuilder::toToml(const NetworkConf &conf)
{
    // 调用 TOML 序列化工具生成带校验的 TOML 字符串
    return NetworkConfToml::toToml(conf, true);
}

QString ConfigPayloadBuilder::toBase64Toml(const NetworkConf &conf)
{
    // 先序列化为 TOML，再以 UTF-8 编码做 Base64 编码
    return QString::fromUtf8(toToml(conf).toUtf8().toBase64());
}

QJsonObject ConfigPayloadBuilder::daemonConfigPayload(const NetworkConf &conf)
{
    // 构建 daemon 可识别的 JSON 载荷，cfg_str 字段存放 Base64 编码的配置
    return QJsonObject{{QStringLiteral("cfg_str"), toBase64Toml(conf)}};
}
