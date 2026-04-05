//
// Created by YMHuang on 2026/4/5.
//

#include "networkconf.h"
#include "qtetnetwork.h"

#include <sstream>
#include <QListWidget>
#include <QLineEdit>

// ==================== NetworkConf 类实现 ====================

NetworkConf::NetworkConf()
{
    // 初始化实例名称
    m_instanceName = generateInstanceName();

    // 初始化默认服务器列表
    m_servers.push_back("tcp://public.easytier.top:11010");

    // 初始化默认监听地址
    m_listenAddresses.push_back("tcp://0.0.0.0:11010");
    m_listenAddresses.push_back("udp://0.0.0.0:11010");
}

void NetworkConf::readFromUI(const QtETNetwork *network)
{
    if (!network) {
        return;
    }

    // ==================== 基础设置 ====================
    m_hostname = network->m_hostnameEdit->text().toStdString();
    m_networkName = network->m_networkNameEdit->text().toStdString();
    m_networkSecret = network->m_networkSecretEdit->text().toStdString();
    m_dhcp = network->m_dhcpCheckBox->isChecked();
    m_ipv4 = network->m_ipv4Edit->text().toStdString();
    m_latencyFirst = network->m_latencyFirstCheckBox->isChecked();
    m_privateMode = network->m_privateModeCheckBox->isChecked();

    // 服务器列表
    m_servers.clear();
    for (int i = 0; i < network->m_serverListWidget->count(); ++i) {
        m_servers.push_back(network->m_serverListWidget->item(i)->text().toStdString());
    }

    // ==================== 高级设置 - 功能开关 ====================
    m_enableKcpProxy = network->m_enableKcpProxyCheckBox->isChecked();
    m_disableKcpInput = network->m_disableKcpInputCheckBox->isChecked();
    m_noTun = network->m_noTunCheckBox->isChecked();
    m_enableQuicProxy = network->m_enableQuicProxyCheckBox->isChecked();
    m_disableQuicInput = network->m_disableQuicInputCheckBox->isChecked();
    m_disableUdpHolePunching = network->m_disableUdpHolePunchingCheckBox->isChecked();
    m_multiThread = network->m_multiThreadCheckBox->isChecked();
    m_useSmoltcp = network->m_useSmoltcpCheckBox->isChecked();
    m_bindDevice = network->m_bindDeviceCheckBox->isChecked();
    m_disableP2p = network->m_disableP2pCheckBox->isChecked();
    m_enableExitNode = network->m_enableExitNodeCheckBox->isChecked();
    m_systemForwarding = network->m_systemForwardingCheckBox->isChecked();
    m_disableSymHolePunching = network->m_disableSymHolePunchingCheckBox->isChecked();
    m_disableIpv6 = network->m_disableIpv6CheckBox->isChecked();
    m_relayAllPeerRpc = network->m_relayAllPeerRpcCheckBox->isChecked();
    m_enableEncryption = network->m_enableEncryptionCheckBox->isChecked();
    m_acceptDns = network->m_acceptDnsCheckBox->isChecked();

    // ==================== 高级设置 - 其他 ====================
    // RPC 端口
    bool ok = false;
    int port = network->m_rpcPortEdit->text().toInt(&ok);
    m_rpcPort = ok ? port : 0;

    // 网络白名单
    m_foreignNetworkWhitelistEnabled = network->m_foreignNetworkWhitelistCheckBox->isChecked();
    m_foreignNetworkWhitelist.clear();
    for (int i = 0; i < network->m_whitelistListWidget->count(); ++i) {
        m_foreignNetworkWhitelist.push_back(network->m_whitelistListWidget->item(i)->text().toStdString());
    }

    // 监听地址
    m_listenAddresses.clear();
    for (int i = 0; i < network->m_listenAddrListWidget->count(); ++i) {
        m_listenAddresses.push_back(network->m_listenAddrListWidget->item(i)->text().toStdString());
    }

    // 子网代理 CIDR
    m_proxyNetworks.clear();
    for (int i = 0; i < network->m_proxyNetworkListWidget->count(); ++i) {
        m_proxyNetworks.push_back(network->m_proxyNetworkListWidget->item(i)->text().toStdString());
    }
}

void NetworkConf::readFromJson(const QJsonObject &json)
{
    // ==================== 实例标识 ====================
    m_instanceName = json["instance_name"].toString().toStdString();
    if (m_instanceName.empty()) {
        m_instanceName = generateInstanceName();
    }

    // ==================== 运行状态 ====================
    m_isRunning = json["is_running"].toBool(false);

    // ==================== 基础设置 ====================
    m_hostname = json["hostname"].toString().toStdString();
    m_networkName = json["network_name"].toString().toStdString();
    m_networkSecret = json["network_secret"].toString().toStdString();
    m_dhcp = json["dhcp"].toBool(true);
    m_ipv4 = json["ipv4"].toString().toStdString();
    m_latencyFirst = json["latency_first"].toBool(false);
    m_privateMode = json["private_mode"].toBool(true);

    // 服务器列表
    m_servers.clear();
    QJsonArray serversArray = json["servers"].toArray();
    for (const QJsonValue &value : serversArray) {
        m_servers.push_back(value.toString().toStdString());
    }

    // ==================== 高级设置 - 功能开关 ====================
    m_enableKcpProxy = json["enable_kcp_proxy"].toBool(true);
    m_disableKcpInput = json["disable_kcp_input"].toBool(false);
    m_noTun = json["no_tun"].toBool(false);
    m_enableQuicProxy = json["enable_quic_proxy"].toBool(false);
    m_disableQuicInput = json["disable_quic_input"].toBool(false);
    m_disableUdpHolePunching = json["disable_udp_hole_punching"].toBool(false);
    m_multiThread = json["multi_thread"].toBool(true);
    m_useSmoltcp = json["use_smoltcp"].toBool(false);
    m_bindDevice = json["bind_device"].toBool(true);
    m_disableP2p = json["disable_p2p"].toBool(false);
    m_enableExitNode = json["enable_exit_node"].toBool(false);
    m_systemForwarding = json["system_forwarding"].toBool(false);
    m_disableSymHolePunching = json["disable_sym_hole_punching"].toBool(false);
    m_disableIpv6 = json["disable_ipv6"].toBool(false);
    m_relayAllPeerRpc = json["relay_all_peer_rpc"].toBool(false);
    m_enableEncryption = json["enable_encryption"].toBool(true);
    m_acceptDns = json["accept_dns"].toBool(false);

    // ==================== 高级设置 - 其他 ====================
    m_rpcPort = json["rpc_port"].toInt(0);
    m_foreignNetworkWhitelistEnabled = json["foreign_network_whitelist_enabled"].toBool(false);

    // 网络白名单
    m_foreignNetworkWhitelist.clear();
    QJsonArray whitelistArray = json["foreign_network_whitelist"].toArray();
    for (const QJsonValue &value : whitelistArray) {
        m_foreignNetworkWhitelist.push_back(value.toString().toStdString());
    }

    // 监听地址
    m_listenAddresses.clear();
    QJsonArray listenAddrArray = json["listen_addresses"].toArray();
    for (const QJsonValue &value : listenAddrArray) {
        m_listenAddresses.push_back(value.toString().toStdString());
    }

    // 子网代理 CIDR
    m_proxyNetworks.clear();
    QJsonArray proxyNetworkArray = json["proxy_networks"].toArray();
    for (const QJsonValue &value : proxyNetworkArray) {
        m_proxyNetworks.push_back(value.toString().toStdString());
    }
}

QJsonObject NetworkConf::toJson() const
{
    QJsonObject json;

    // ==================== 实例标识 ====================
    json["instance_name"] = QString::fromStdString(m_instanceName);

    // ==================== 运行状态 ====================
    json["is_running"] = m_isRunning;

    // ==================== 基础设置 ====================
    json["hostname"] = QString::fromStdString(m_hostname);
    json["network_name"] = QString::fromStdString(m_networkName);
    json["network_secret"] = QString::fromStdString(m_networkSecret);
    json["dhcp"] = m_dhcp;
    json["ipv4"] = QString::fromStdString(m_ipv4);
    json["latency_first"] = m_latencyFirst;
    json["private_mode"] = m_privateMode;

    // 服务器列表
    QJsonArray serversArray;
    for (const auto &server : m_servers) {
        serversArray.append(QString::fromStdString(server));
    }
    json["servers"] = serversArray;

    // ==================== 高级设置 - 功能开关 ====================
    json["enable_kcp_proxy"] = m_enableKcpProxy;
    json["disable_kcp_input"] = m_disableKcpInput;
    json["no_tun"] = m_noTun;
    json["enable_quic_proxy"] = m_enableQuicProxy;
    json["disable_quic_input"] = m_disableQuicInput;
    json["disable_udp_hole_punching"] = m_disableUdpHolePunching;
    json["multi_thread"] = m_multiThread;
    json["use_smoltcp"] = m_useSmoltcp;
    json["bind_device"] = m_bindDevice;
    json["disable_p2p"] = m_disableP2p;
    json["enable_exit_node"] = m_enableExitNode;
    json["system_forwarding"] = m_systemForwarding;
    json["disable_sym_hole_punching"] = m_disableSymHolePunching;
    json["disable_ipv6"] = m_disableIpv6;
    json["relay_all_peer_rpc"] = m_relayAllPeerRpc;
    json["enable_encryption"] = m_enableEncryption;
    json["accept_dns"] = m_acceptDns;

    // ==================== 高级设置 - 其他 ====================
    json["rpc_port"] = m_rpcPort;
    json["foreign_network_whitelist_enabled"] = m_foreignNetworkWhitelistEnabled;

    // 网络白名单
    QJsonArray whitelistArray;
    for (const auto &item : m_foreignNetworkWhitelist) {
        whitelistArray.append(QString::fromStdString(item));
    }
    json["foreign_network_whitelist"] = whitelistArray;

    // 监听地址
    QJsonArray listenAddrArray;
    for (const auto &addr : m_listenAddresses) {
        listenAddrArray.append(QString::fromStdString(addr));
    }
    json["listen_addresses"] = listenAddrArray;

    // 子网代理 CIDR
    QJsonArray proxyNetworkArray;
    for (const auto &cidr : m_proxyNetworks) {
        proxyNetworkArray.append(QString::fromStdString(cidr));
    }
    json["proxy_networks"] = proxyNetworkArray;

    return json;
}

std::string NetworkConf::toToml() const
{
    std::ostringstream oss;

    // ==================== 顶层字段 ====================
    // instance_name 使用网络名称
    oss << "instance_name = \"" << m_instanceName << "\"\n";

    oss << "hostname = \"" << m_hostname << "\"\n";
    oss << "dhcp = " << (m_dhcp ? "true" : "false") << "\n";
    if (!m_ipv4.empty() && !m_dhcp) {
        oss << "ipv4 = \"" << m_ipv4 << "\"\n";
    }

    // ==================== listeners 监听地址 ====================
    if (!m_listenAddresses.empty()) {
        oss << "\nlisteners = [\n";
        for (const auto &addr : m_listenAddresses) {
            oss << "\"" << addr << "\",\n";
        }
        oss << "]\n";
    }

    // ==================== rpc_portal RPC 端口 ====================
    if (m_rpcPort > 0) {
        oss << "\nrpc_portal = \"127.0.0.1:" << m_rpcPort << "\"\n";
    }

    // ==================== [network_identity] 网络身份 ====================
    oss << "\n[network_identity]\n";
    oss << "network_name = \"" << m_networkName << "\"\n";
    oss << "network_secret = \"" << m_networkSecret << "\"\n";

    // ==================== [[peer]] 服务器节点 ====================
    for (const auto &server : m_servers) {
        oss << "\n[[peer]]\n";
        oss << "uri = \"" << server << "\"\n";
    }

    // ==================== [[proxy_network]] 子网代理 ====================
    for (const auto &cidr : m_proxyNetworks) {
        oss << "\n[[proxy_network]]\n";
        oss << "cidr = \"" << cidr << "\"\n";
    }

    // ==================== [flags] 功能标志 ====================
    oss << "\n[flags]\n";
    oss << "enable_encryption = " << (m_enableEncryption ? "true" : "false") << "\n";
    oss << "enable_ipv6 = " << (!m_disableIpv6 ? "true" : "false") << "\n";
    oss << "latency_first = " << (m_latencyFirst ? "true" : "false") << "\n";
    oss << "enable_exit_node = " << (m_enableExitNode ? "true" : "false") << "\n";
    oss << "no_tun = " << (m_noTun ? "true" : "false") << "\n";
    oss << "use_smoltcp = " << (m_useSmoltcp ? "true" : "false") << "\n";

    // foreign_network_whitelist: 启用时才输出该字段，多个网络用空格分隔
    if (m_foreignNetworkWhitelistEnabled) {
        oss << "foreign_network_whitelist = \"";
        for (size_t i = 0; i < m_foreignNetworkWhitelist.size(); ++i) {
            if (i > 0) oss << " ";
            oss << m_foreignNetworkWhitelist[i];
        }
        oss << "\"\n";
    }

    oss << "enable_quic_proxy = " << (m_enableQuicProxy ? "true" : "false") << "\n";
    oss << "disable_quic_input = " << (m_disableQuicInput ? "true" : "false") << "\n";
    oss << "enable_kcp_proxy = " << (m_enableKcpProxy ? "true" : "false") << "\n";
    oss << "disable_kcp_input = " << (m_disableKcpInput ? "true" : "false") << "\n";
    oss << "bind_device = " << (m_bindDevice ? "true" : "false") << "\n";
    oss << "private_mode = " << (m_privateMode ? "true" : "false") << "\n";
    oss << "disable_p2p = " << (m_disableP2p ? "true" : "false") << "\n";
    oss << "multi_thread = " << (m_multiThread ? "true" : "false") << "\n";
    oss << "accept_dns = " << (m_acceptDns ? "true" : "false") << "\n";
    oss << "disable_sym_hole_punching = " << (m_disableSymHolePunching ? "true" : "false") << "\n";
    oss << "relay_all_peer_rpc = " << (m_relayAllPeerRpc ? "true" : "false") << "\n";
    oss << "disable_udp_hole_punching = " << (m_disableUdpHolePunching ? "true" : "false") << "\n";
    oss << "proxy_forward_by_system = " << (m_systemForwarding ? "true" : "false") << "\n";

    return oss.str();
}

std::string NetworkConf::generateInstanceName()
{
    static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    static constexpr size_t charsetSize = sizeof(charset) - 1;
    static constexpr size_t nameLength = 10;
    static constexpr const char *prefix = "QtET-";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distribution(0, charsetSize - 1);

    std::string result = prefix;
    result.reserve(strlen(prefix) + nameLength);

    for (size_t i = 0; i < nameLength; ++i) {
        result += charset[distribution(generator)];
    }

    return result;
}

// ==================== 全局函数实现 ====================

bool saveAllNetworkConf(const std::vector<NetworkConf> &confList, QString *errorMsg)
{
    // 获取配置文件路径
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);

    // 确保目录存在
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            if (errorMsg) {
                *errorMsg = QObject::tr("无法创建配置目录: %1").arg(configPath);
            }
            return false;
        }
    }

    // 构建完整的 JSON 文档
    QJsonArray jsonArray;
    for (const NetworkConf &conf : confList) {
        jsonArray.append(conf.toJson());
    }

    QJsonDocument doc(jsonArray);

    // 写入文件
    QString filePath = configPath + "/network2.json";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) {
            *errorMsg = QObject::tr("无法打开配置文件: %1").arg(filePath);
        }
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

std::vector<NetworkConf> readAllNetworkConf()
{
    std::vector<NetworkConf> confList;

    // 获取配置文件路径
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString filePath = configPath + "/network2.json";

    QFile file(filePath);
    if (!file.exists()) {
        // 文件不存在，返回空列表
        return confList;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        // 无法打开文件，返回空列表
        return confList;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        // JSON 解析错误，返回空列表
        return confList;
    }

    if (!doc.isArray()) {
        // 不是数组格式，返回空列表
        return confList;
    }

    QJsonArray jsonArray = doc.array();
    for (const QJsonValue &value : jsonArray) {
        if (value.isObject()) {
            NetworkConf conf;
            conf.readFromJson(value.toObject());
            confList.push_back(conf);
        }
    }

    return confList;
}