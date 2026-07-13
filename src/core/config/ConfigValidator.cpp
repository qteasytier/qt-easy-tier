/**
 * @file ConfigValidator.cpp
 * @brief 配置验证器实现
 *
 * 校验策略：
 * 1. 必填字段检查：instance_name、displayName
 * 2. 服务器去重：检查 servers 中是否有重复 URI
 * 3. 代理子网校验：检查 proxyNetworks 中是否有重复 CIDR、空 CIDR、空协议或非法协议
 *
 * 无内存分配：QStringList 值语义，QSet 为 RAII。
 * 无异常风险：所有操作均为 Qt 容器安全操作。
 */
#include "ConfigValidator.h"
#include "core/config/NetworkConf.h"
#include <QSet>

QStringList ConfigValidator::validate(const NetworkConf &conf)
{
    QStringList errors;

    // --- 必填字段检查 ---
    if (conf.instanceName().isEmpty())
        errors << QStringLiteral("instance_name 不能为空");
    if (conf.displayName.isEmpty())
        errors << QStringLiteral("配置名称不能为空");

    // --- 服务器地址去重 ---
    // 使用 QSet 记录已出现的 URI，检测重复
    {
        QSet<QString> seen;
        for (const auto &s : conf.servers) {
            const QString uri = s.uri.trimmed();
            if (uri.isEmpty()) continue;
            if (seen.contains(uri))
                errors << QStringLiteral("服务器地址重复: %1").arg(uri);
            else
                seen.insert(uri);
        }
    }

    // --- 代理子网 CIDR / 协议校验 ---
    {
        const QSet<QString> allowedProtocols = {
            QStringLiteral("tcp"),
            QStringLiteral("udp"),
            QStringLiteral("icmp")
        };

        QSet<QString> seen;
        for (const auto &network : conf.proxyNetworks) {
            const QString cidr = network.cidr.trimmed();
            if (cidr.isEmpty()) {
                errors << QStringLiteral("子网代理 CIDR 不能为空");
                continue;
            }

            if (seen.contains(cidr))
                errors << QStringLiteral("子网代理 CIDR 重复: %1").arg(cidr);
            else
                seen.insert(cidr);

            if (network.allow.isEmpty()) {
                errors << QStringLiteral("子网代理协议不能为空: %1").arg(cidr);
                continue;
            }

            for (const auto &protocol : network.allow) {
                if (!allowedProtocols.contains(protocol))
                    errors << QStringLiteral("子网代理协议无效: %1").arg(protocol);
            }
        }
    }

    return errors;
}
