/**
 * @file NetworkConf.cpp
 * @brief NetworkConf 实现
 *
 * 核心职责：
 * 1. 构造时自动应用默认值（applyDefaults）
 * 2. QVariantMap 互转：toFieldMap() 序列化 / fromFieldMap() 反序列化
 * 3. toStringList()：类型兼容的类型转换辅助
 *
 * 安全性：无 raw pointer / 无动态分配 / 无异常抛出路径。
 */
#include "NetworkConf.h"
#include <QVariantList>

// ============================================================
//  构造 / 基本访问器
// ============================================================

NetworkConf::NetworkConf(const QString &instanceName)
    : m_instanceName(instanceName)
{
    // 构造时自动应用默认值，确保所有可选字段都有合理的初始状态
    applyDefaults(*this);
}

QString NetworkConf::instanceName() const { return m_instanceName; }

void NetworkConf::setInstanceName(const QString &name)
{
    m_instanceName = name;
    m_dirty = true;  // 实例名变更视为脏数据，需要持久化
}

bool NetworkConf::dirty() const { return m_dirty; }

void NetworkConf::markClean() { m_dirty = false; }

void NetworkConf::markDirty() { m_dirty = true; }

// ============================================================
//  默认值填充
// ============================================================

void NetworkConf::applyDefaults(NetworkConf &conf)
{
    // 仅当加密算法为空时才设置默认值，避免覆盖用户已配置的值
    if (conf.encryptionAlgorithm.isEmpty())
        conf.encryptionAlgorithm = QStringLiteral("aes-gcm");
}

// ============================================================
//  QVariantMap 序列化（SQLite 字段级存储用）
// ============================================================

QVariantMap NetworkConf::toFieldMap() const
{
    QVariantMap m;

    // --- 基础设置 ---
    m["hostname"]      = hostname;
    m["network_name"]  = networkName;
    m["network_secret"] = networkSecret;
    m["dhcp"]          = dhcp;
    m["ipv4"]          = ipv4;
    m["latency_first"] = latencyFirst;
    m["private_mode"]  = privateMode;

    // 服务器列表：序列化为 QVariantList<QVariantMap>，跳过空 URI
    {
        QVariantList list;
        for (const auto &peer : servers) {
            if (peer.uri.trimmed().isEmpty()) continue;
            QVariantMap pm;
            pm["uri"] = peer.uri.trimmed();
            if (!peer.publicKey.isEmpty())
                pm["public_key"] = peer.publicKey;
            list.append(pm);
        }
        m["servers"] = list;
    }

    // --- 加密基础 ---
    m["enable_encryption"] = enableEncryption;
    m["encryption_algorithm"] = encryptionAlgorithm;
    m["no_tun"]        = noTun;
    m["mtu"]           = mtu;
    m["listen_addresses"] = QVariant::fromValue(listenAddresses);
    {
        QVariantList list;
        for (const auto &network : proxyNetworks) {
            const QString cidr = network.cidr.trimmed();
            if (cidr.isEmpty()) continue;

            QVariantMap item;
            item["cidr"] = cidr;
            if (!network.mappedCidr.trimmed().isEmpty())
                item["mapped_cidr"] = network.mappedCidr.trimmed();
            item["allow"] = network.allow;
            list.append(item);
        }
        m["proxy_networks"] = list;
    }
    m["custom_routes"]    = QVariant::fromValue(customRoutes);

    // --- 传输协议 ---
    m["enable_kcp_proxy"] = enableKcpProxy;
    m["disable_kcp_input"] = disableKcpInput;
    m["enable_quic_proxy"] = enableQuicProxy;
    m["disable_quic_input"] = disableQuicInput;
    m["disable_relay_kcp"]  = disableRelayKcp;
    m["disable_relay_quic"] = disableRelayQuic;
    m["enable_relay_foreign_network_kcp"]  = enableRelayForeignNetworkKcp;
    m["enable_relay_foreign_network_quic"] = enableRelayForeignNetworkQuic;

    // --- P2P 连接 ---
    m["disable_udp_hole_punching"] = disableUdpHolePunching;
    m["disable_tcp_hole_punching"] = disableTcpHolePunching;
    m["disable_upnp"]     = disableUpnp;
    m["need_p2p"]         = needP2p;
    m["lazy_p2p"]         = lazyP2p;
    m["p2p_only"]         = p2pOnly;
    m["disable_p2p"]      = disableP2p;
    m["disable_sym_hole_punching"] = disableSymHolePunching;
    m["relay_all_peer_rpc"] = relayAllPeerRpc;
    m["bind_device"]      = bindDevice;

    // --- 性能与系统 ---
    m["multi_thread"]     = multiThread;
    m["use_smoltcp"]      = useSmoltcp;
    m["enable_ipv6"]      = enableIpv6;
    m["dev_name"]         = devName;

    // --- 网络服务与列表 ---
    m["enable_exit_node"] = enableExitNode;
    m["system_forwarding"] = systemForwarding;
    m["accept_dns"]       = acceptDns;
    m["default_protocol"] = defaultProtocol;
    m["exit_nodes"]       = QVariant::fromValue(exitNodes);
    m["enable_foreign_network_whitelist"] = enableForeignNetworkWhitelist;
    m["foreign_network_whitelist"] = foreignNetworkWhitelist;

    // --- 安全模式 ---
    m["secure_mode_enabled"] = secureModeEnabled;
    m["local_private_key"] = localPrivateKey;

    return m;
}

// ============================================================
//  QVariantMap 反序列化
// ============================================================

NetworkConf NetworkConf::fromFieldMap(const QVariantMap &map, const QString &instanceName)
{
    NetworkConf conf(instanceName);

    // Lambda 辅助函数：类型安全地从 map 读取各类值

    /// @brief 读取字符串值
    auto s = [&](const QString &k) -> QString {
        return map.value(k).toString();
    };

    /// @brief 读取布尔值，key 不存在时返回默认值 def
    auto b = [&](const QString &k, bool def = false) -> bool {
        if (!map.contains(k)) return def;
        return map.value(k).toBool();
    };

    /// @brief 读取整数值，key 不存在时返回默认值 def
    auto i = [&](const QString &k, int def = 0) -> int {
        if (!map.contains(k)) return def;
        return map.value(k).toInt();
    };

    /// @brief 读取字符串列表（兼容 QStringList 和 QVariantList 格式）
    auto sl = [&](const QString &k) -> QStringList {
        return toStringList(map.value(k));
    };

    // --- 基础设置 ---
    conf.hostname      = s("hostname");
    conf.networkName   = s("network_name");
    conf.networkSecret = s("network_secret");
    conf.dhcp          = b("dhcp", true);
    conf.ipv4          = s("ipv4");
    conf.latencyFirst  = b("latency_first");
    conf.privateMode   = b("private_mode", true);

    // 服务器列表反序列化：兼容 QStringList（仅 URI）和 QVariantList<QVariantMap> 两种格式
    {
        QVector<ServerPeer> list;
        QVariant serversVar = map.value("servers");

        // 格式 1：纯字符串数组 ["tcp://1.2.3.4:1000", ...]
        if (serversVar.metaType() == QMetaType(QMetaType::QStringList)) {
            for (const auto &uri : serversVar.toStringList()) {
                if (!uri.trimmed().isEmpty())
                    list.append({uri.trimmed(), {}});
            }
        }
        // 格式 2：对象数组 [{"uri": "...", "public_key": "..."}, ...]
        else {
            QVariantList vl = serversVar.toList();
            for (const auto &v : vl) {
                QVariantMap pm = v.toMap();
                ServerPeer peer;
                peer.uri = pm.value("uri").toString().trimmed();
                peer.publicKey = pm.value("public_key").toString().trimmed();
                if (!peer.uri.isEmpty())
                    list.append(peer);
            }
        }
        conf.servers = list;
    }

    // --- 加密基础 ---
    conf.enableEncryption = b("enable_encryption", true);
    conf.encryptionAlgorithm = s("encryption_algorithm");
    conf.noTun         = b("no_tun");
    conf.mtu           = i("mtu", 1380);
    conf.listenAddresses = sl("listen_addresses");
    {
        QVector<ProxyNetwork> list;
        const QVariantList values = map.value("proxy_networks").toList();
        for (const auto &value : values) {
            const QVariantMap item = value.toMap();

            ProxyNetwork network;
            network.cidr = item.value("cidr").toString().trimmed();
            network.mappedCidr = item.value("mapped_cidr").toString().trimmed();
            network.allow = toStringList(item.value("allow"));

            if (!network.cidr.isEmpty())
                list.append(network);
        }
        conf.proxyNetworks = list;
    }
    conf.customRoutes    = sl("custom_routes");

    // --- 传输协议 ---
    conf.enableKcpProxy  = b("enable_kcp_proxy", true);
    conf.disableKcpInput = b("disable_kcp_input");
    conf.enableQuicProxy = b("enable_quic_proxy");
    conf.disableQuicInput = b("disable_quic_input");
    conf.disableRelayKcp  = b("disable_relay_kcp");
    conf.disableRelayQuic = b("disable_relay_quic");
    conf.enableRelayForeignNetworkKcp  = b("enable_relay_foreign_network_kcp");
    conf.enableRelayForeignNetworkQuic = b("enable_relay_foreign_network_quic");

    // --- P2P 连接 ---
    conf.disableUdpHolePunching = b("disable_udp_hole_punching");
    conf.disableTcpHolePunching = b("disable_tcp_hole_punching");
    conf.disableUpnp     = b("disable_upnp");
    conf.needP2p         = b("need_p2p");
    conf.lazyP2p         = b("lazy_p2p");
    conf.p2pOnly         = b("p2p_only");
    conf.disableP2p      = b("disable_p2p");
    conf.disableSymHolePunching = b("disable_sym_hole_punching");
    conf.relayAllPeerRpc = b("relay_all_peer_rpc");
    conf.bindDevice      = b("bind_device", true);

    // --- 性能与系统 ---
    conf.multiThread     = b("multi_thread", true);
    conf.useSmoltcp      = b("use_smoltcp");
    conf.enableIpv6      = b("enable_ipv6");
    conf.devName         = s("dev_name");

    // --- 网络服务与列表 ---
    conf.enableExitNode  = b("enable_exit_node");
    conf.systemForwarding = b("system_forwarding");
    conf.acceptDns       = b("accept_dns");
    conf.defaultProtocol = s("default_protocol");
    conf.exitNodes       = sl("exit_nodes");
    conf.enableForeignNetworkWhitelist = b("enable_foreign_network_whitelist");
    conf.foreignNetworkWhitelist = s("foreign_network_whitelist");

    // --- 安全模式 ---
    conf.secureModeEnabled = b("secure_mode_enabled");
    conf.localPrivateKey   = s("local_private_key");

    // 反序列化后补充默认值并清除脏标记
    applyDefaults(conf);
    conf.markClean();
    return conf;
}

// ============================================================
//  辅助函数：QVariant → QStringList 类型兼容转换
// ============================================================

QStringList NetworkConf::toStringList(const QVariant &v)
{
    // 空值或无效值 → 空列表
    if (!v.isValid() || v.isNull()) return {};

    // 已经是 QStringList → 直接返回
    if (v.metaType() == QMetaType(QMetaType::QStringList))
        return v.toStringList();

    // QVariantList → 逐个元素转字符串
    if (v.metaType() == QMetaType(QMetaType::QVariantList)) {
        QStringList r;
        for (const auto &e : v.toList())
            r.append(e.toString());
        return r;
    }

    // 无法识别的类型 → 返回空列表
    return {};
}
