/**
 * @file NetworkConfToml.cpp
 * @brief TOML 序列化/反序列化实现
 *
 * 序列化策略：
 * - 按 TOML 节组织：根级字段 → [network_identity] → [[peer]] → [flags] → [secure_mode]
 * - 数组字段（listeners, exit_nodes, routes）使用内联数组语法
 * - 代理子网使用 [[proxy_network]] 表格数组
 * - 仅输出有值的字段，避免冗余
 *
 * 反序列化策略：
 * - 使用 toml.hpp 库解析，异常安全：parse 失败时静默返回含默认值的对象
 * - 按 TOML 节结构分层读取
 */
#include "NetworkConfToml.h"
#include <QVariantList>
#include <QLatin1Char>
#include <QSet>
#include <toml.hpp>
#include "core/util/LogHelper.h"

namespace {

/**
 * @brief TOML 字符串转义
 *
 * 将字符串中的反斜杠、双引号和换行符进行转义，
 * 并用双引号包裹。用于手写 TOML 输出。
 * @param s 原始字符串
 * @return 转义并加引号后的 TOML 字符串值
 */
QString esc(const QString &s)
{
    QString r = s;
    r.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    r.replace(QLatin1Char('"'),  QStringLiteral("\\\""));
    r.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    return QLatin1Char('"') + r + QLatin1Char('"');
}

QStringList defaultProxyNetworkAllow()
{
    return {QStringLiteral("tcp"), QStringLiteral("udp"), QStringLiteral("icmp")};
}

bool isDefaultProxyNetworkAllow(const QStringList &allow)
{
    const QSet<QString> values(allow.cbegin(), allow.cend());
    return values.size() == 3
        && values.contains(QStringLiteral("tcp"))
        && values.contains(QStringLiteral("udp"))
        && values.contains(QStringLiteral("icmp"));
}

} // anonymous namespace

namespace NetworkConfToml {

QString toToml(const NetworkConf &conf, bool includeInstanceName)
{
    QStringList lines;

    // --- 根级字段 ---
    if (includeInstanceName)
        lines << QStringLiteral("instance_name = %1").arg(esc(conf.instanceName()));
    if (!conf.hostname.isEmpty())
        lines << QStringLiteral("hostname = %1").arg(esc(conf.hostname));
    lines << QStringLiteral("dhcp = %1").arg(conf.dhcp ? QStringLiteral("true") : QStringLiteral("false"));

    if (!conf.ipv4.isEmpty())
        lines << QStringLiteral("ipv4 = %1").arg(esc(conf.ipv4));

    // --- 辅助输出 Lambda ---

    /// @brief 输出布尔标志
    auto flagBool = [&](const QString &key, bool v) {
        lines << QStringLiteral("%1 = %2").arg(key, v ? QStringLiteral("true") : QStringLiteral("false"));
    };

    /// @brief 输出字符串标志（仅非空时输出）
    auto flagStr = [&](const QString &key, const QString &v) {
        if (!v.isEmpty())
            lines << QStringLiteral("%1 = %2").arg(key, esc(v));
    };

    /// @brief 输出整数标志
    auto flagInt = [&](const QString &key, int v) {
        lines << QStringLiteral("%1 = %2").arg(key).arg(v);
    };

    /// @brief 输出内联字符串数组 [\"a\", \"b\"]
    auto makeArray = [&](const QString &key, const QStringList &values) {
        QStringList quoted;
        for (const auto &v : values) {
            const QString t = v.trimmed();
            if (!t.isEmpty())
                quoted << esc(t);
        }
        if (!quoted.isEmpty())
            lines << QStringLiteral("%1 = [%2]").arg(key, quoted.join(QStringLiteral(", ")));
    };

    // --- 根级数组字段（必须在所有 [table] / [[array_table]] 之前输出） ---
    makeArray(QStringLiteral("listeners"), conf.listenAddresses);
    makeArray(QStringLiteral("exit_nodes"), conf.exitNodes);
    makeArray(QStringLiteral("routes"), conf.customRoutes);

    // --- [network_identity] 节 ---
    if (!conf.networkName.isEmpty() || !conf.networkSecret.isEmpty()) {
        lines << QString{};
        lines << QStringLiteral("[network_identity]");
        if (!conf.networkName.isEmpty())
            lines << QStringLiteral("network_name = %1").arg(esc(conf.networkName));
        if (!conf.networkSecret.isEmpty())
            lines << QStringLiteral("network_secret = %1").arg(esc(conf.networkSecret));
    }

    // --- [[peer]] 表格数组节 ---
    for (const auto &peer : conf.servers) {
        if (!peer.uri.trimmed().isEmpty()) {
            lines << QString{};
            lines << QStringLiteral("[[peer]]");
            lines << QStringLiteral("uri = %1").arg(esc(peer.uri.trimmed()));
            if (!peer.publicKey.isEmpty())
                lines << QStringLiteral("peer_public_key = %1").arg(esc(peer.publicKey));
        }
    }

    // --- [[proxy_network]] 表格数组节 ---
    for (const auto &network : conf.proxyNetworks) {
        const QString cidr = network.cidr.trimmed();
        if (cidr.isEmpty()) continue;

        lines << QString{};
        lines << QStringLiteral("[[proxy_network]]");
        lines << QStringLiteral("cidr = %1").arg(esc(cidr));

        const QString mappedCidr = network.mappedCidr.trimmed();
        if (!mappedCidr.isEmpty())
            lines << QStringLiteral("mapped_cidr = %1").arg(esc(mappedCidr));

        if (!isDefaultProxyNetworkAllow(network.allow))
            makeArray(QStringLiteral("allow"), network.allow);
    }

    // --- [flags] 节 ---
    lines << QString{};
    lines << QStringLiteral("[flags]");

    flagBool("enable_encryption", conf.enableEncryption);
    flagStr("encryption_algorithm", conf.encryptionAlgorithm);
    flagBool("latency_first", conf.latencyFirst);
    flagBool("private_mode", conf.privateMode);
    flagBool("no_tun", conf.noTun);
    flagInt("mtu", conf.mtu);
    flagBool("enable_kcp_proxy", conf.enableKcpProxy);
    flagBool("disable_kcp_input", conf.disableKcpInput);
    flagBool("enable_quic_proxy", conf.enableQuicProxy);
    flagBool("disable_quic_input", conf.disableQuicInput);
    flagBool("disable_relay_kcp", conf.disableRelayKcp);
    flagBool("disable_relay_quic", conf.disableRelayQuic);
    flagBool("enable_relay_foreign_network_kcp", conf.enableRelayForeignNetworkKcp);
    flagBool("enable_relay_foreign_network_quic", conf.enableRelayForeignNetworkQuic);
    flagBool("disable_udp_hole_punching", conf.disableUdpHolePunching);
    flagBool("disable_tcp_hole_punching", conf.disableTcpHolePunching);
    flagBool("disable_upnp", conf.disableUpnp);
    flagBool("need_p2p", conf.needP2p);
    flagBool("lazy_p2p", conf.lazyP2p);
    flagBool("p2p_only", conf.p2pOnly);
    flagBool("disable_p2p", conf.disableP2p);
    flagBool("disable_sym_hole_punching", conf.disableSymHolePunching);
    flagBool("relay_all_peer_rpc", conf.relayAllPeerRpc);
    flagBool("bind_device", conf.bindDevice);
    flagBool("multi_thread", conf.multiThread);
    flagBool("use_smoltcp", conf.useSmoltcp);
    flagBool("enable_ipv6", conf.enableIpv6);
    flagStr("dev_name", conf.devName);
    flagBool("enable_exit_node", conf.enableExitNode);
    flagBool("proxy_forward_by_system", conf.systemForwarding);
    flagBool("accept_dns", conf.acceptDns);
    flagStr("default_protocol", conf.defaultProtocol);

    // 外部网络白名单：只有在启用且有值时输出
    if (conf.enableForeignNetworkWhitelist && !conf.foreignNetworkWhitelist.isEmpty())
        lines << QStringLiteral("relay_network_whitelist = %1").arg(esc(conf.foreignNetworkWhitelist));

    // --- [secure_mode] 节 ---
    if (conf.secureModeEnabled) {
        lines << QString{};
        lines << QStringLiteral("[secure_mode]");
        lines << QStringLiteral("enabled = true");
        if (!conf.localPrivateKey.isEmpty())
            lines << QStringLiteral("local_private_key = %1").arg(esc(conf.localPrivateKey));
    }

    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

NetworkConf fromToml(const QString &toml, const QString &instanceName)
{
    NetworkConf conf(instanceName);

    // TOML 值读取辅助 Lambda（基于 toml11 库的 optional 返回值）

    /// @brief 读取字符串值
    auto readStr = [](const toml::table &t, const char *key) -> QString {
        if (auto val = t[key].value<std::string>())
            return QString::fromStdString(*val);
        return {};
    };

    /// @brief 读取布尔值，key 不存在时返回默认值
    auto readBool = [](const toml::table &t, const char *key, bool def = false) -> bool {
        if (auto val = t[key].value<bool>())
            return *val;
        return def;
    };

    /// @brief 读取整数值（TOML 整数为 int64_t，显式转换为 int）
    auto readInt = [](const toml::table &t, const char *key, int def = 0) -> int {
        if (auto val = t[key].value<int64_t>())
            return static_cast<int>(*val);
        return def;
    };

    // 转为 std::string 供 toml11 解析
    std::string stdToml = toml.toStdString();
    toml::table tbl;

    // 解析 TOML：异常安全，失败时静默返回含默认值的空配置
    try {
        tbl = toml::parse(stdToml);

    // --- 根级字段 ---
    // 注意：instance_name 不从文件读取，由调用方通过参数传入
    conf.hostname     = readStr(tbl, "hostname");
    conf.dhcp         = readBool(tbl, "dhcp", true);
    conf.ipv4         = readStr(tbl, "ipv4");

    // --- [network_identity] 节 ---
    if (auto ident = tbl["network_identity"].as_table()) {
        conf.networkName   = readStr(*ident, "network_name");
        conf.networkSecret = readStr(*ident, "network_secret");
    }

    // --- [[peer]] 表格数组 ---
    if (auto peers = tbl["peer"].as_array()) {
        for (const auto &node : *peers) {
            if (auto peerTbl = node.as_table()) {
                // 仅当 uri 存在时才创建节点（uri 为必填字段）
                if (auto uri = (*peerTbl)["uri"].value<std::string>()) {
                    ServerPeer peer;
                    peer.uri = QString::fromStdString(*uri);
                    if (auto key = (*peerTbl)["peer_public_key"].value<std::string>())
                        peer.publicKey = QString::fromStdString(*key);
                    conf.servers.append(peer);
                }
            }
        }
    }

    // --- listeners（字符串数组）---
    if (auto listeners = tbl["listeners"].as_array()) {
        for (const auto &val : *listeners) {
            if (auto s = val.value<std::string>())
                conf.listenAddresses.append(QString::fromStdString(*s));
        }
    }

    // --- [[proxy_network]] 表格数组 ---
    if (auto pns = tbl["proxy_network"].as_array()) {
        for (const auto &node : *pns) {
            if (auto pn = node.as_table()) {
                if (auto cidr = (*pn)["cidr"].value<std::string>()) {
                    ProxyNetwork network;
                    network.cidr = QString::fromStdString(*cidr);

                    if (auto mappedCidr = (*pn)["mapped_cidr"].value<std::string>())
                        network.mappedCidr = QString::fromStdString(*mappedCidr);

                    if (auto allow = (*pn)["allow"].as_array()) {
                        for (const auto &value : *allow) {
                            if (auto protocol = value.value<std::string>())
                                network.allow.append(QString::fromStdString(*protocol));
                        }
                    } else {
                        network.allow = defaultProxyNetworkAllow();
                    }

                    conf.proxyNetworks.append(network);
                }
            }
        }
    }

    // --- exit_nodes（字符串数组）---
    if (auto ens = tbl["exit_nodes"].as_array()) {
        for (const auto &val : *ens) {
            if (auto s = val.value<std::string>())
                conf.exitNodes.append(QString::fromStdString(*s));
        }
    }

    // --- routes（字符串数组）---
    if (auto routesArr = tbl["routes"].as_array()) {
        for (const auto &val : *routesArr) {
            if (auto r = val.value<std::string>())
                conf.customRoutes.append(QString::fromStdString(*r));
        }
    }

    // --- [flags] 节 ---
    if (auto flags = tbl["flags"].as_table()) {
        conf.enableEncryption = readBool(*flags, "enable_encryption", true);
        conf.encryptionAlgorithm = readStr(*flags, "encryption_algorithm");
        conf.latencyFirst = readBool(*flags, "latency_first");
        conf.privateMode  = readBool(*flags, "private_mode", true);
        conf.noTun         = readBool(*flags, "no_tun");
        conf.mtu           = readInt(*flags, "mtu", 1380);
        conf.enableKcpProxy  = readBool(*flags, "enable_kcp_proxy", true);
        conf.disableKcpInput = readBool(*flags, "disable_kcp_input");
        conf.enableQuicProxy = readBool(*flags, "enable_quic_proxy");
        conf.disableQuicInput = readBool(*flags, "disable_quic_input");
        conf.disableRelayKcp  = readBool(*flags, "disable_relay_kcp");
        conf.disableRelayQuic = readBool(*flags, "disable_relay_quic");
        conf.enableRelayForeignNetworkKcp  = readBool(*flags, "enable_relay_foreign_network_kcp");
        conf.enableRelayForeignNetworkQuic = readBool(*flags, "enable_relay_foreign_network_quic");
        conf.disableUdpHolePunching = readBool(*flags, "disable_udp_hole_punching");
        conf.disableTcpHolePunching = readBool(*flags, "disable_tcp_hole_punching");
        conf.disableUpnp     = readBool(*flags, "disable_upnp");
        conf.needP2p         = readBool(*flags, "need_p2p");
        conf.lazyP2p         = readBool(*flags, "lazy_p2p");
        conf.p2pOnly         = readBool(*flags, "p2p_only");
        conf.disableP2p      = readBool(*flags, "disable_p2p");
        conf.disableSymHolePunching = readBool(*flags, "disable_sym_hole_punching");
        conf.relayAllPeerRpc = readBool(*flags, "relay_all_peer_rpc");
        conf.bindDevice      = readBool(*flags, "bind_device", true);
        conf.multiThread     = readBool(*flags, "multi_thread", true);
        conf.useSmoltcp      = readBool(*flags, "use_smoltcp");
        conf.enableIpv6      = readBool(*flags, "enable_ipv6", true);
        conf.devName         = readStr(*flags, "dev_name");
        conf.enableExitNode  = readBool(*flags, "enable_exit_node");
        conf.systemForwarding = readBool(*flags, "proxy_forward_by_system");
        conf.acceptDns       = readBool(*flags, "accept_dns");
        conf.defaultProtocol = readStr(*flags, "default_protocol");

        // 外部网络白名单：有 relay_network_whitelist 字段则认为已启用
        if (auto val = (*flags)["relay_network_whitelist"].value<std::string>()) {
            conf.enableForeignNetworkWhitelist = true;
            conf.foreignNetworkWhitelist = QString::fromStdString(*val);
        }
    }

    // --- [secure_mode] 节 ---
    if (auto sm = tbl["secure_mode"].as_table()) {
        conf.secureModeEnabled = readBool(*sm, "enabled");
        conf.localPrivateKey   = readStr(*sm, "local_private_key");
    }

    // 应用默认值补充缺失字段，并清除脏标记
    NetworkConf::applyDefaults(conf);
    conf.markClean();
    } catch (const toml::parse_error &e) {
        LogHelper::logWarning(QStringLiteral("TOML 解析错误: %1").arg(e.what()), "Config");
    } catch (const std::exception &e) {
        LogHelper::logWarning(QStringLiteral("TOML 处理错误: %1").arg(e.what()), "Config");
    }
    return conf;
}

} // namespace NetworkConfToml
