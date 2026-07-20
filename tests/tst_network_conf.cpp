/**
 * @file tst_network_conf.cpp
 * @brief NetworkConf 模块的单元测试。测试内容：构造函数默认值验证、fieldMap 序列化/反序列化往返、TOML 序列化/反序列化往返、TOML 完整输出内容校验、daemon 配置 payload 生成、脏标记追踪。
 *
 * 覆盖：
 * - 构造函数默认值验证
 * - fieldMap 序列化/反序列化往返（不含 instance_name）
 * - TOML 序列化/反序列化往返（静态输出 vs 运行时输出）
 * - TOML 完整输出内容校验（布尔字段、网络身份、peer 列表、flags 节）
 * - 脏标记追踪（dirty / markClean）
 */
#include <QTest>
#include <QJsonObject>
#include "core/config/NetworkConf.h"
#include "core/config/NetworkConfToml.h"
#include "core/application/config/ConfigPayloadBuilder.h"

class TestNetworkConf : public QObject
{
    Q_OBJECT

private slots:
    /// 测试目标: 验证 NetworkConf 默认构造函数在所有字段上设置的默认值正确
    void constructorDefaults()
    {
        // 准备测试数据：使用默认构造函数创建实例
        NetworkConf conf("default-id");
        // 检查 instanceName 是否符合预期
        QCOMPARE(conf.instanceName(), QString("default-id"));
        // 检查各字段默认值是否符合预期
        QVERIFY(conf.hostname.isEmpty());
        QCOMPARE(conf.dhcp, true);
        QCOMPARE(conf.enableEncryption, true);
        QCOMPARE(conf.privateMode, true);
        QCOMPARE(conf.bindDevice, true);
        QCOMPARE(conf.multiThread, true);
        QCOMPARE(conf.mtu, 1380);
    }

    /// 测试目标: 验证 fieldMap 往返转换的一致性
    /// toFieldMap() 不应包含 instance_name，fromFieldMap() 应正确恢复所有字段
    void fieldMapRoundtrip()
    {
        // 准备测试数据：填充多个字段
        NetworkConf conf("test-conf");
        conf.hostname = QStringLiteral("node-a");
        conf.networkName = QStringLiteral("my-net");
        conf.dhcp = false;
        conf.servers = {{QStringLiteral("tcp://s1"), QString()}, {QStringLiteral("tcp://s2"), QString()}};

        auto map = conf.toFieldMap();
        // 检查 fieldMap 不包含 instance_name
        QVERIFY(!map.contains("instance_name"));
        auto restored = NetworkConf::fromFieldMap(map, "restored-id");

        // 验证结果：各字段往返后值一致
        QCOMPARE(restored.instanceName(), QString("restored-id"));
        QCOMPARE(restored.hostname, QStringLiteral("node-a"));
        QCOMPARE(restored.networkName, QStringLiteral("my-net"));
        QCOMPARE(restored.dhcp, false);
        QCOMPARE(restored.servers.size(), 2);
        QCOMPARE(restored.servers.at(0).uri, QStringLiteral("tcp://s1"));
    }

    /// 测试目标: 验证 TOML 序列化与反序列化完整往返
    /// 覆盖所有主要字段:
    ///   hostname / networkName / networkSecret / dhcp / ipv4
    ///   latencyFirst / servers / encryption / mtu / kcp / 白名单
    /// 同时验证运行时输出（传入 true）会包含 instance_name
    void tomlRoundtrip()
    {
        // 准备测试数据：填充几乎所有字段
        NetworkConf conf("test-conf");
        conf.hostname = QStringLiteral("node-b");
        conf.networkName = QStringLiteral("test-net");
        conf.networkSecret = QStringLiteral("secret123");
        conf.dhcp = false;
        conf.ipv4 = QStringLiteral("10.0.0.1");
        conf.latencyFirst = true;
        conf.servers = {{QStringLiteral("tcp://peer1:11010"), QString()}};
        conf.enableEncryption = true;
        conf.encryptionAlgorithm = QStringLiteral("aes-gcm");
        conf.mtu = 1400;
        conf.enableKcpProxy = true;
        conf.disableKcpInput = false;
        conf.enableForeignNetworkWhitelist = true;
        conf.foreignNetworkWhitelist = QStringLiteral("10.0.0.0/8");

        QString toml = NetworkConfToml::toToml(conf);
        // 检查 TOML 输出不为空
        QVERIFY(!toml.isEmpty());
        QVERIFY(!toml.contains("instance_name"));
        // 检查各节是否存在
        QVERIFY(toml.contains("hostname"));
        QVERIFY(toml.contains("[network_identity]"));
        QVERIFY(toml.contains("[[peer]]"));
        QVERIFY(toml.contains("[flags]"));
        QVERIFY(toml.contains("relay_network_whitelist"));

        // 运行时模式: 需包含 instance_name 供 easy-tier-core 使用
        QString runtimeToml = NetworkConfToml::toToml(conf, true);
        QVERIFY(runtimeToml.contains("instance_name = \"test-conf\""));

        auto restored = NetworkConfToml::fromToml(toml, "restored-id");
        // 验证结果：反序列化后字段值一致
        QCOMPARE(restored.instanceName(), QString("restored-id"));
        QCOMPARE(restored.hostname, QStringLiteral("node-b"));
        QCOMPARE(restored.networkName, QStringLiteral("test-net"));
        QCOMPARE(restored.dhcp, false);
        QCOMPARE(restored.ipv4, QStringLiteral("10.0.0.1"));
        QCOMPARE(restored.encryptionAlgorithm, QStringLiteral("aes-gcm"));
        QCOMPARE(restored.servers.size(), 1);
        QCOMPARE(restored.servers.at(0).uri, QStringLiteral("tcp://peer1:11010"));
        QCOMPARE(restored.latencyFirst, true);
        QCOMPARE(restored.enableForeignNetworkWhitelist, true);
        QCOMPARE(restored.foreignNetworkWhitelist, QStringLiteral("10.0.0.0/8"));
    }

    /// 测试目标: DHCP 开启时即使配置中残留 ipv4，导出的 TOML 也不能包含 ipv4 字段
    void tomlOmitsIpv4WhenDhcpEnabled()
    {
        NetworkConf conf("dhcp-ipv4");
        conf.hostname = QStringLiteral("node");
        conf.dhcp = true;
        conf.ipv4 = QStringLiteral("10.0.0.1");

        const QString toml = NetworkConfToml::toToml(conf);

        QVERIFY(!toml.contains(QStringLiteral("ipv4 =")));
    }

    /// 测试目标: 验证 TOML 输出省略与默认值一致的布尔字段
    /// 使用最简配置，确保默认布尔值不会产生冗余输出
    void tomlFullOutput()
    {
        // 准备测试数据：最简配置
        NetworkConf conf("test");
        conf.hostname = QStringLiteral("node");
        conf.enableKcpProxy = true;
        conf.mtu = 1400;

        QString toml = NetworkConfToml::toToml(conf);

        // 默认布尔值不输出，非默认布尔值仍输出
        QVERIFY(!toml.contains("latency_first = false"));
        QVERIFY(!toml.contains("private_mode = true"));
        QVERIFY(!toml.contains("[network_identity]"));
        QVERIFY(!toml.contains("enable_encryption = true"));
        QVERIFY(!toml.contains("no_tun = false"));
        QVERIFY(toml.contains("mtu = 1400"));
        QVERIFY(toml.contains("enable_kcp_proxy = true"));
        QVERIFY(!toml.contains("enable_quic_proxy = false"));
    }

    /// 测试目标: 验证 TOML 省略默认布尔值后反序列化仍恢复默认值
    void tomlDefaultBoolOmissionRoundtrip()
    {
        NetworkConf conf("defaults");
        conf.hostname = QStringLiteral("node");

        const QString toml = NetworkConfToml::toToml(conf);

        QVERIFY(!toml.contains(QStringLiteral("enable_kcp_proxy = false")));

        const auto restored = NetworkConfToml::fromToml(toml, "defaults");

        QCOMPARE(restored.dhcp, true);
        QCOMPARE(restored.enableEncryption, true);
        QCOMPARE(restored.privateMode, true);
        QCOMPARE(restored.enableKcpProxy, false);
        QCOMPARE(restored.bindDevice, true);
        QCOMPARE(restored.multiThread, true);
        QCOMPARE(restored.enableIpv6, true);
    }

    /// 测试目标: 验证同时存在 peer、根级数组、proxy_network 时 round-trip 不丢字段
    /// 回归检查: toml 根级数组(listeners/exit_nodes/routes)在 [[peer]] 后输出会被误归入最后一个 peer
    void serversAndRootArraysRoundtrip()
    {
        // 准备测试数据：同时包含 peer、根级数组和 proxy_network
        NetworkConf conf("roundtrip-test");
        conf.hostname = QStringLiteral("node");
        conf.dhcp = true;
        conf.servers = {{QStringLiteral("tcp://peer1:11010"), QString()},
                         {QStringLiteral("tcp://peer2:11020"), QStringLiteral("pubkey-abc")}};
        conf.listenAddresses = {QStringLiteral("tcp://0.0.0.0:11010"), QStringLiteral("udp://0.0.0.0:12010")};
        conf.exitNodes = {QStringLiteral("10.0.0.1"), QStringLiteral("10.0.0.2")};
        conf.customRoutes = {QStringLiteral("192.168.1.0/24")};
        conf.proxyNetworks = {
            {QStringLiteral("10.10.0.0/16"), QStringLiteral("10.20.0.0/16"),
             {QStringLiteral("tcp"), QStringLiteral("udp")}}
        };
        conf.enableEncryption = true;
        conf.mtu = 1380;

        const QString toml = NetworkConfToml::toToml(conf, true);
        const auto restored = NetworkConfToml::fromToml(toml, "roundtrip-test");

        // 验证结果：往返后各字段不丢失
        QCOMPARE(restored.instanceName(), QStringLiteral("roundtrip-test"));
        QCOMPARE(restored.servers.size(), 2);
        QCOMPARE(restored.servers.at(0).uri, QStringLiteral("tcp://peer1:11010"));
        QCOMPARE(restored.servers.at(1).uri, QStringLiteral("tcp://peer2:11020"));
        QCOMPARE(restored.servers.at(1).publicKey, QStringLiteral("pubkey-abc"));
        QCOMPARE(restored.listenAddresses.size(), 2);
        QCOMPARE(restored.listenAddresses.at(0), QStringLiteral("tcp://0.0.0.0:11010"));
        QCOMPARE(restored.listenAddresses.at(1), QStringLiteral("udp://0.0.0.0:12010"));
        QCOMPARE(restored.exitNodes.size(), 2);
        QCOMPARE(restored.exitNodes.at(0), QStringLiteral("10.0.0.1"));
        QCOMPARE(restored.exitNodes.at(1), QStringLiteral("10.0.0.2"));
        QCOMPARE(restored.customRoutes.size(), 1);
        QCOMPARE(restored.customRoutes.at(0), QStringLiteral("192.168.1.0/24"));
        QCOMPARE(restored.proxyNetworks.size(), 1);
        QCOMPARE(restored.proxyNetworks.at(0).cidr, QStringLiteral("10.10.0.0/16"));
        QCOMPARE(restored.proxyNetworks.at(0).mappedCidr, QStringLiteral("10.20.0.0/16"));
        QCOMPARE(restored.proxyNetworks.at(0).allow, QStringList({QStringLiteral("tcp"), QStringLiteral("udp")}));
    }

    /// 测试目标: 验证 proxy_network 全协议时省略 allow，解析缺失 allow 时恢复为全选
    void proxyNetworkAllowDefaultsRoundtrip()
    {
        NetworkConf conf("proxy-defaults");
        conf.hostname = QStringLiteral("node");
        conf.proxyNetworks = {
            {QStringLiteral("192.168.1.0/24"), QString(),
             {QStringLiteral("tcp"), QStringLiteral("udp"), QStringLiteral("icmp")}}
        };

        const QString toml = NetworkConfToml::toToml(conf);

        QVERIFY(toml.contains(QStringLiteral("[[proxy_network]]")));
        QVERIFY(toml.contains(QStringLiteral("cidr = \"192.168.1.0/24\"")));
        QVERIFY(!toml.contains(QStringLiteral("allow =")));

        const auto restored = NetworkConfToml::fromToml(toml, "proxy-defaults");

        QCOMPARE(restored.proxyNetworks.size(), 1);
        QCOMPARE(restored.proxyNetworks.at(0).cidr, QStringLiteral("192.168.1.0/24"));
        QCOMPARE(restored.proxyNetworks.at(0).mappedCidr, QString());
        QCOMPARE(restored.proxyNetworks.at(0).allow,
                 QStringList({QStringLiteral("tcp"), QStringLiteral("udp"), QStringLiteral("icmp")}));
    }

    /// 测试目标: 验证 daemon 配置 payload 统一生成 cfg_str，且内容是运行时 TOML 的 base64
    void payloadBuilderCreatesCfgStr()
    {
        // 准备测试数据：创建带基本字段的配置
        NetworkConf conf("payload-test");
        conf.hostname = QStringLiteral("host-a");
        conf.networkName = QStringLiteral("net-a");

        const QJsonObject payload = ConfigPayloadBuilder::daemonConfigPayload(conf);

        // 检查 payload 包含 cfg_str 字段
        QVERIFY(payload.contains(QStringLiteral("cfg_str")));
        const QString encoded = payload.value(QStringLiteral("cfg_str")).toString();
        QVERIFY(!encoded.isEmpty());

        // 解码 base64 后检查内容包含关键字段
        const QString decoded = QString::fromUtf8(QByteArray::fromBase64(encoded.toUtf8()));
        QVERIFY(decoded.contains(QStringLiteral("instance_name = \"payload-test\"")));
        QVERIFY(decoded.contains(QStringLiteral("hostname = \"host-a\"")));
    }

    /// 测试目标: 验证脏标记追踪机制
    /// 修改字段后 dirty() 返回 true，markClean() 后重置为 false
    void dirtyTracking()
    {
        // 准备测试数据：新创建的配置不应为脏
        NetworkConf conf("dirty-test");
        QCOMPARE(conf.dirty(), false);

        // 修改字段后检查 dirty 标记
        conf.setInstanceName(QStringLiteral("new-id"));
        QCOMPARE(conf.dirty(), true);

        // markClean 后检查 dirty 标记重置
        conf.markClean();
        QCOMPARE(conf.dirty(), false);
    }
};

QTEST_MAIN(TestNetworkConf)
#include "tst_network_conf.moc"
