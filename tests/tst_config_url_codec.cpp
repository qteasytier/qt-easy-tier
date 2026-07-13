#include <QTest>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include "core/config/ConfigUrlCodec.h"
#include "core/config/NetworkConfToml.h"

static const char *kFieldMappingJson = R"({
  "version": 1,
  "reserved": { "0": false, "1": true },
  "mapping": {
    "hostname": 2, "ipv4": 3, "dhcp": 4, "instance_name": 5,
    "listeners": 6, "exit_nodes": 7, "routes": 8,
    "network_identity": 9, "network_name": 10, "network_secret": 11,
    "peer": 12, "uri": 13, "peer_public_key": 14,
    "proxy_network": 15, "cidr": 16, "mapped_cidr": 17, "allow": 18,
    "flags": 19,
    "enable_encryption": 20, "encryption_algorithm": 21, "latency_first": 22,
    "private_mode": 23, "no_tun": 24, "mtu": 25, "enable_kcp_proxy": 26,
    "disable_kcp_input": 27, "enable_quic_proxy": 28, "disable_quic_input": 29,
    "disable_relay_kcp": 30, "disable_relay_quic": 31,
    "enable_relay_foreign_network_kcp": 32, "enable_relay_foreign_network_quic": 33,
    "disable_udp_hole_punching": 34, "disable_tcp_hole_punching": 35,
    "disable_upnp": 36, "need_p2p": 37, "lazy_p2p": 38, "p2p_only": 39,
    "disable_p2p": 40, "disable_sym_hole_punching": 41, "relay_all_peer_rpc": 42,
    "bind_device": 43, "multi_thread": 44, "use_smoltcp": 45, "enable_ipv6": 46,
    "dev_name": 47, "enable_exit_node": 48, "proxy_forward_by_system": 49,
    "accept_dns": 50, "default_protocol": 51, "relay_network_whitelist": 52,
    "secure_mode": 53, "enabled": 54, "local_private_key": 55
  }
})";

class TestConfigUrlCodec : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;

    void writeMapping()
    {
        QString path = m_tempDir.path() + QStringLiteral("/field_mapping.json");
        QFile f(path);
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        f.write(kFieldMappingJson);
        f.close();
        ConfigUrlCodec::setMappingFilePath(path);
    }

private slots:
    void init()
    {
        QVERIFY(m_tempDir.isValid());
        writeMapping();
    }

    void encodeBasicConfig()
    {
        QString toml = "hostname = \"test-node\"\ndhcp = true\nmtu = 1400\n\n[flags]\nenable_encryption = true\n";
        ConfigTextResult result = ConfigUrlCodec::encodeToml(toml);
        QVERIFY2(result.success, qPrintable(result.error));
        QVERIFY(result.value.startsWith("qtet://"));
    }

    void decodeInvalidPrefix()
    {
        ConfigTextResult result = ConfigUrlCodec::decodeUrl("http://example.com");
        QVERIFY(!result.success);
        QVERIFY(result.error.contains("qtet://"));
    }

    void decodeInvalidBase64()
    {
        ConfigTextResult result = ConfigUrlCodec::decodeUrl("qtet://!!!invalid!!!");
        QVERIFY(!result.success);
    }

    void roundtripBasicFields()
    {
        QString toml = "hostname = \"node-x\"\ndhcp = false\nipv4 = \"10.0.0.5\"\n\n[flags]\nmtu = 1420\nenable_encryption = false\nlatency_first = true\nencryption_algorithm = \"chacha20-poly1305\"\n";

        ConfigTextResult encoded = ConfigUrlCodec::encodeToml(toml);
        QVERIFY2(encoded.success, qPrintable(encoded.error));

        ConfigTextResult decoded = ConfigUrlCodec::decodeUrl(encoded.value);
        QVERIFY2(decoded.success, qPrintable(decoded.error));

        auto restored = NetworkConfToml::fromToml(decoded.value, "test-roundtrip");
        QCOMPARE(restored.hostname, QStringLiteral("node-x"));
        QCOMPARE(restored.dhcp, false);
        QCOMPARE(restored.ipv4, QStringLiteral("10.0.0.5"));
        QCOMPARE(restored.mtu, 1420);
        QCOMPARE(restored.enableEncryption, false);
        QCOMPARE(restored.latencyFirst, true);
        QCOMPARE(restored.encryptionAlgorithm, QStringLiteral("chacha20-poly1305"));
    }

    void roundtripWithSections()
    {
        QString toml = "hostname = \"section-node\"\n\n[network_identity]\nnetwork_name = \"my-net\"\nnetwork_secret = \"secret\"\n\n[flags]\nenable_encryption = true\nprivate_mode = false\nenable_kcp_proxy = true\n";
        ConfigTextResult encoded = ConfigUrlCodec::encodeToml(toml);
        QVERIFY2(encoded.success, qPrintable(encoded.error));

        ConfigTextResult decoded = ConfigUrlCodec::decodeUrl(encoded.value);
        QVERIFY2(decoded.success, qPrintable(decoded.error));

        auto restored = NetworkConfToml::fromToml(decoded.value, "test-sections");
        QCOMPARE(restored.hostname, QStringLiteral("section-node"));
        QCOMPARE(restored.networkName, QStringLiteral("my-net"));
        QCOMPARE(restored.networkSecret, QStringLiteral("secret"));
        QCOMPARE(restored.enableEncryption, true);
        QCOMPARE(restored.privateMode, false);
        QCOMPARE(restored.enableKcpProxy, true);
    }

    void roundtripPeers()
    {
        QString toml = "hostname = \"peer-node\"\n\n[[peer]]\nuri = \"tcp://server1:11010\"\npeer_public_key = \"pubkey-abc\"\n\n[[peer]]\nuri = \"tcp://server2:11020\"\n";
        ConfigTextResult encoded = ConfigUrlCodec::encodeToml(toml);
        QVERIFY2(encoded.success, qPrintable(encoded.error));

        ConfigTextResult decoded = ConfigUrlCodec::decodeUrl(encoded.value);
        QVERIFY2(decoded.success, qPrintable(decoded.error));

        auto restored = NetworkConfToml::fromToml(decoded.value, "test-peers");
        QCOMPARE(restored.servers.size(), 2);
        QCOMPARE(restored.servers.at(0).uri, QStringLiteral("tcp://server1:11010"));
        QCOMPARE(restored.servers.at(0).publicKey, QStringLiteral("pubkey-abc"));
        QCOMPARE(restored.servers.at(1).uri, QStringLiteral("tcp://server2:11020"));
        QVERIFY(restored.servers.at(1).publicKey.isEmpty());
    }

    void roundtripProxyNetwork()
    {
        QString toml = "hostname = \"proxy-node\"\n\n[[proxy_network]]\ncidr = \"10.10.0.0/16\"\nmapped_cidr = \"10.20.0.0/16\"\nallow = [\"tcp\", \"udp\"]\n\n[flags]\nenable_encryption = true\n";
        ConfigTextResult encoded = ConfigUrlCodec::encodeToml(toml);
        QVERIFY2(encoded.success, qPrintable(encoded.error));

        ConfigTextResult decoded = ConfigUrlCodec::decodeUrl(encoded.value);
        QVERIFY2(decoded.success, qPrintable(decoded.error));

        auto restored = NetworkConfToml::fromToml(decoded.value, "test-proxy");
        QCOMPARE(restored.proxyNetworks.size(), 1);
        QCOMPARE(restored.proxyNetworks.at(0).cidr, QStringLiteral("10.10.0.0/16"));
        QCOMPARE(restored.proxyNetworks.at(0).mappedCidr, QStringLiteral("10.20.0.0/16"));
        QCOMPARE(restored.proxyNetworks.at(0).allow, QStringList({QStringLiteral("tcp"), QStringLiteral("udp")}));
    }

    void roundtripStringArrays()
    {
        QString toml = "hostname = \"array-node\"\nlisteners = [\"tcp://0.0.0.0:11010\", \"udp://0.0.0.0:12010\"]\nexit_nodes = [\"10.0.0.1\", \"10.0.0.2\"]\nroutes = [\"192.168.1.0/24\"]\n";
        ConfigTextResult encoded = ConfigUrlCodec::encodeToml(toml);
        QVERIFY2(encoded.success, qPrintable(encoded.error));

        ConfigTextResult decoded = ConfigUrlCodec::decodeUrl(encoded.value);
        QVERIFY2(decoded.success, qPrintable(decoded.error));

        auto restored = NetworkConfToml::fromToml(decoded.value, "test-arrays");
        QCOMPARE(restored.listenAddresses.size(), 2);
        QCOMPARE(restored.listenAddresses.at(0), QStringLiteral("tcp://0.0.0.0:11010"));
        QCOMPARE(restored.listenAddresses.at(1), QStringLiteral("udp://0.0.0.0:12010"));
        QCOMPARE(restored.exitNodes.size(), 2);
        QCOMPARE(restored.customRoutes.size(), 1);
        QCOMPARE(restored.customRoutes.at(0), QStringLiteral("192.168.1.0/24"));
    }

    void booleanEncodesAsZeroOne()
    {
        QString toml = "dhcp = true\nhostname = \"b\"\n\n[flags]\nenable_encryption = false\n";
        ConfigTextResult encoded = ConfigUrlCodec::encodeToml(toml);
        QVERIFY2(encoded.success, qPrintable(encoded.error));

        QString payload = encoded.value.mid(7);
        QByteArray decoded = QByteArray::fromBase64(payload.toUtf8(), QByteArray::Base64UrlEncoding);
        QJsonDocument doc = QJsonDocument::fromJson(decoded);
        QJsonObject root = doc.object();
        QJsonObject data = root["d"].toObject();
        QCOMPARE(data["4"].toInt(), 1);

        QJsonObject flags = data["19"].toObject();
        QCOMPARE(flags["20"].toInt(), 0);
    }

    void fullConfigRoundtrip()
    {
        QString toml =
            "instance_name = \"full-config\"\n"
            "hostname = \"full-node\"\n"
            "dhcp = false\n"
            "ipv4 = \"192.168.1.100\"\n"
            "listeners = [\"tcp://0.0.0.0:11010\"]\n"
            "exit_nodes = [\"10.0.0.1\"]\n"
            "routes = [\"192.168.0.0/16\"]\n"
            "\n"
            "[network_identity]\n"
            "network_name = \"full-net\"\n"
            "network_secret = \"full-secret\"\n"
            "\n"
            "[[peer]]\n"
            "uri = \"tcp://peer1:11010\"\n"
            "peer_public_key = \"key-1\"\n"
            "\n"
            "[[proxy_network]]\n"
            "cidr = \"10.0.0.0/8\"\n"
            "allow = [\"tcp\", \"udp\", \"icmp\"]\n"
            "\n"
            "[flags]\n"
            "enable_encryption = true\n"
            "encryption_algorithm = \"aes-gcm\"\n"
            "mtu = 1380\n"
            "enable_kcp_proxy = true\n"
            "disable_udp_hole_punching = true\n"
            "bind_device = true\n"
            "multi_thread = true\n"
            "enable_ipv6 = true\n"
            "dev_name = \"eth0\"\n"
            "enable_exit_node = false\n"
            "accept_dns = true\n"
            "default_protocol = \"tcp\"\n"
            "relay_network_whitelist = \"10.0.0.0/8\"\n"
            "\n"
            "[secure_mode]\n"
            "enabled = true\n"
            "local_private_key = \"my-private-key\"\n";

        ConfigTextResult encoded = ConfigUrlCodec::encodeToml(toml);
        QVERIFY2(encoded.success, qPrintable(encoded.error));

        ConfigTextResult decoded = ConfigUrlCodec::decodeUrl(encoded.value);
        QVERIFY2(decoded.success, qPrintable(decoded.error));

        auto restored = NetworkConfToml::fromToml(decoded.value, "test-full");

        QCOMPARE(restored.hostname, QStringLiteral("full-node"));
        QCOMPARE(restored.dhcp, false);
        QCOMPARE(restored.ipv4, QStringLiteral("192.168.1.100"));
        QCOMPARE(restored.listenAddresses.size(), 1);
        QCOMPARE(restored.exitNodes.size(), 1);
        QCOMPARE(restored.customRoutes.size(), 1);

        QCOMPARE(restored.networkName, QStringLiteral("full-net"));
        QCOMPARE(restored.networkSecret, QStringLiteral("full-secret"));

        QCOMPARE(restored.servers.size(), 1);
        QCOMPARE(restored.servers.at(0).uri, QStringLiteral("tcp://peer1:11010"));

        QCOMPARE(restored.proxyNetworks.size(), 1);

        QCOMPARE(restored.enableEncryption, true);
        QCOMPARE(restored.mtu, 1380);
        QCOMPARE(restored.enableKcpProxy, true);
        QCOMPARE(restored.disableUdpHolePunching, true);
        QCOMPARE(restored.enableIpv6, true);
        QCOMPARE(restored.devName, QStringLiteral("eth0"));
        QCOMPARE(restored.acceptDns, true);
        QCOMPARE(restored.defaultProtocol, QStringLiteral("tcp"));
        QCOMPARE(restored.enableForeignNetworkWhitelist, true);

        QCOMPARE(restored.secureModeEnabled, true);
        QCOMPARE(restored.localPrivateKey, QStringLiteral("my-private-key"));
    }
};

QTEST_MAIN(TestConfigUrlCodec)
#include "tst_config_url_codec.moc"
