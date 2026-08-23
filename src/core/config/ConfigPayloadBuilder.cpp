/** @file ConfigPayloadBuilder.cpp @brief ConfigPayloadBuilder 实现 */
#include "ConfigPayloadBuilder.h"

#include "core/config/NetworkConf.h"
#include "core/config/NetworkConfToml.h"

#include <QByteArray>

QString ConfigPayloadBuilder::toBase64Toml(const NetworkConf &conf)
{
    // 先序列化为带 instance_name 的运行时 TOML，再以 UTF-8 编码做 Base64 编码
    return QString::fromUtf8(NetworkConfToml::toToml(conf, true).toUtf8().toBase64());
}

QJsonObject ConfigPayloadBuilder::daemonConfigPayload(const NetworkConf &conf)
{
    // 构建 daemon 可识别的 JSON 载荷，cfg_str 字段存放 Base64 编码的配置
    return QJsonObject{{QStringLiteral("cfg_str"), toBase64Toml(conf)}};
}
