/**
 * @file ConfigEditorViewModel.cpp
 * @brief ConfigEditorViewModel 实现
 *
 * 本文件实现了配置编辑器所有属性的 getter/setter、以及
 * load/save/cancel/clear 四个操作。每个 setter 遵循统一的"比较-赋值-标记-通知"模板。
 *
 * 核心数据流：
 *   QML 用户编辑 → setter → m_conf 字段更新 → markDirty → emit 信号
 *   loadConfig() → repo.load() → m_conf 替换 → emitCurrentChanged
 *   save()       → repo.save(m_conf) → 清除 dirty 标记
 *   cancel()     → loadConfig(currentInstanceName) → 恢复仓库版本
 *   clear()      → m_conf 重置 → emitCurrentChanged
 */
#include "ConfigEditorViewModel.h"
#include "core/repository/NetworkConfigRepository.h"

ConfigEditorViewModel::ConfigEditorViewModel(NetworkConfigRepository *repo, QObject *parent)
    : QObject(parent)
    , m_repo(repo)
{
    // 构造时不从仓库加载任何数据，需由外部调用 loadConfig 或 clear 初始化编辑器状态
}

// ==================== 内部辅助方法 ====================

/**
 * @brief 标记当前编辑有未保存的变更
 *
 * 采用信号防抖模式：仅在 m_hasUnsavedChanges 从 false 变 true 时发射 hasUnsavedChangesChanged，
 * 避免对同一状态反复发射无意义信号。
 *
 * 注意：本方法只负责"标记为脏"，永远不会将 m_hasUnsavedChanges 从 true 改回 false。
 * 重置为 false 的时机由 save / load / clear 操作触发。
 */
void ConfigEditorViewModel::markDirty()
{
    if (!m_hasUnsavedChanges) {
        m_hasUnsavedChanges = true;
        emit hasUnsavedChangesChanged();
    }
}

// ==================== 编辑器状态 getter ====================

QString ConfigEditorViewModel::currentInstanceName() const { return m_conf.instanceName(); }
bool ConfigEditorViewModel::hasUnsavedChanges() const { return m_hasUnsavedChanges; }
QStringList ConfigEditorViewModel::errorMessages() const { return m_errorMessages; }

// ==================== 元数据 ====================

QString ConfigEditorViewModel::displayName() const { return m_conf.displayName; }
void ConfigEditorViewModel::setDisplayName(const QString &v) {
    if (m_conf.displayName == v) return;
    m_conf.displayName = v; markDirty(); emit displayNameChanged();
}

// ==================== 基础设置 ====================

QString ConfigEditorViewModel::hostname() const { return m_conf.hostname; }
void ConfigEditorViewModel::setHostname(const QString &v) {
    if (m_conf.hostname == v) return;
    m_conf.hostname = v; markDirty(); emit hostnameChanged();
}
QString ConfigEditorViewModel::networkName() const { return m_conf.networkName; }
void ConfigEditorViewModel::setNetworkName(const QString &v) {
    if (m_conf.networkName == v) return;
    m_conf.networkName = v; markDirty(); emit networkNameChanged();
}
QString ConfigEditorViewModel::networkSecret() const { return m_conf.networkSecret; }
void ConfigEditorViewModel::setNetworkSecret(const QString &v) {
    if (m_conf.networkSecret == v) return;
    m_conf.networkSecret = v; markDirty(); emit networkSecretChanged();
}
bool ConfigEditorViewModel::dhcp() const { return m_conf.dhcp; }
void ConfigEditorViewModel::setDhcp(bool v) {
    if (m_conf.dhcp == v) return;
    m_conf.dhcp = v; markDirty(); emit dhcpChanged();
}
QString ConfigEditorViewModel::ipv4() const { return m_conf.ipv4; }
void ConfigEditorViewModel::setIpv4(const QString &v) {
    if (m_conf.ipv4 == v) return;
    m_conf.ipv4 = v; markDirty(); emit ipv4Changed();
}
bool ConfigEditorViewModel::latencyFirst() const { return m_conf.latencyFirst; }
void ConfigEditorViewModel::setLatencyFirst(bool v) {
    if (m_conf.latencyFirst == v) return;
    m_conf.latencyFirst = v; markDirty(); emit latencyFirstChanged();
}
bool ConfigEditorViewModel::privateMode() const { return m_conf.privateMode; }
void ConfigEditorViewModel::setPrivateMode(bool v) {
    if (m_conf.privateMode == v) return;
    m_conf.privateMode = v; markDirty(); emit privateModeChanged();
}

/**
 * @brief 将 ServerPeer 列表序列化为 QVariantList，供 QML 使用
 *
 * 每个元素为 QVariantMap，包含 "uri" 和 "publicKey" 两个键。
 * QML 侧通过 modelData.uri / modelData.publicKey 访问。
 *
 * @note 每次调用都重新构建 QVariantList（浅拷贝），因为 QML 需要全新的列表引用
 *       来检测变更（QML 的 === 比较是引用比较）。
 */
QVariantList ConfigEditorViewModel::servers() const {
    QVariantList list;
    for (const auto &peer : m_conf.servers) {
        QVariantMap m;
        m["uri"] = peer.uri;
        m["publicKey"] = peer.publicKey;
        list.append(m);
    }
    return list;
}

/**
 * @brief 从 QVariantList 反序列化并设置服务器列表
 *
 * 过滤逻辑：如果某个元素的 uri 为空字符串，则跳过该条目，
 * 避免将空白行持久化到数据库。这允许用户在 UI 中删除一行后直接保存。
 *
 * @param v QML 传入的服务器列表（ListModel → QVariantList）
 */
void ConfigEditorViewModel::setServers(const QVariantList &v) {
    QVector<ServerPeer> list;
    for (const auto &item : v) {
        QVariantMap m = item.toMap();
        ServerPeer peer;
        peer.uri = m.value("uri").toString().trimmed();
        peer.publicKey = m.value("publicKey").toString().trimmed();
        // 过滤空白行：没有 uri 的条目不加入列表
        if (!peer.uri.isEmpty())
            list.append(peer);
    }
    // 先比较大小，简化快速路径：大小不同直接判定为变更
    if (m_conf.servers.size() == list.size()) {
        bool same = true;
        for (int i = 0; i < list.size(); ++i) {
            if (m_conf.servers[i].uri != list[i].uri || m_conf.servers[i].publicKey != list[i].publicKey) {
                same = false;
                break;
            }
        }
        if (same) return;  // 完全相同，不触发更新
    }
    m_conf.servers = list; markDirty(); emit serversChanged();
}

// ==================== 加密基础 ====================

bool ConfigEditorViewModel::enableEncryption() const { return m_conf.enableEncryption; }
void ConfigEditorViewModel::setEnableEncryption(bool v) {
    if (m_conf.enableEncryption == v) return;
    m_conf.enableEncryption = v; markDirty(); emit enableEncryptionChanged();
}
QString ConfigEditorViewModel::encryptionAlgorithm() const { return m_conf.encryptionAlgorithm; }
void ConfigEditorViewModel::setEncryptionAlgorithm(const QString &v) {
    if (m_conf.encryptionAlgorithm == v) return;
    m_conf.encryptionAlgorithm = v; markDirty(); emit encryptionAlgorithmChanged();
}
bool ConfigEditorViewModel::noTun() const { return m_conf.noTun; }
void ConfigEditorViewModel::setNoTun(bool v) {
    if (m_conf.noTun == v) return;
    m_conf.noTun = v; markDirty(); emit noTunChanged();
}
int ConfigEditorViewModel::mtu() const { return m_conf.mtu; }
void ConfigEditorViewModel::setMtu(int v) {
    if (m_conf.mtu == v) return;
    m_conf.mtu = v; markDirty(); emit mtuChanged();
}
QStringList ConfigEditorViewModel::listenAddresses() const { return m_conf.listenAddresses; }
void ConfigEditorViewModel::setListenAddresses(const QStringList &v) {
    if (m_conf.listenAddresses == v) return;
    m_conf.listenAddresses = v; markDirty(); emit listenAddressesChanged();
}
QVariantList ConfigEditorViewModel::proxyNetworks() const {
    QVariantList list;
    for (const auto &network : m_conf.proxyNetworks) {
        QVariantMap item;
        item["cidr"] = network.cidr;
        item["mappedCidr"] = network.mappedCidr;
        item["allow"] = network.allow;
        list.append(item);
    }
    return list;
}
void ConfigEditorViewModel::setProxyNetworks(const QVariantList &v) {
    auto toStringList = [](const QVariant &value) {
        if (value.metaType() == QMetaType(QMetaType::QStringList))
            return value.toStringList();

        QStringList result;
        for (const auto &item : value.toList()) {
            const QString protocol = item.toString().trimmed();
            if (!protocol.isEmpty())
                result.append(protocol);
        }
        return result;
    };

    QVector<ProxyNetwork> list;
    for (const auto &item : v) {
        const QVariantMap m = item.toMap();

        ProxyNetwork network;
        network.cidr = m.value("cidr").toString().trimmed();
        network.mappedCidr = m.value("mappedCidr").toString().trimmed();
        network.allow = toStringList(m.value("allow"));

        if (!network.cidr.isEmpty())
            list.append(network);
    }

    if (m_conf.proxyNetworks.size() == list.size()) {
        bool same = true;
        for (int i = 0; i < list.size(); ++i) {
            if (m_conf.proxyNetworks[i].cidr != list[i].cidr
                || m_conf.proxyNetworks[i].mappedCidr != list[i].mappedCidr
                || m_conf.proxyNetworks[i].allow != list[i].allow) {
                same = false;
                break;
            }
        }
        if (same) return;
    }

    m_conf.proxyNetworks = list; markDirty(); emit proxyNetworksChanged();
}
QStringList ConfigEditorViewModel::customRoutes() const { return m_conf.customRoutes; }
void ConfigEditorViewModel::setCustomRoutes(const QStringList &v) {
    if (m_conf.customRoutes == v) return;
    m_conf.customRoutes = v; markDirty(); emit customRoutesChanged();
}

// ==================== 传输协议 ====================

bool ConfigEditorViewModel::enableKcpProxy() const { return m_conf.enableKcpProxy; }
void ConfigEditorViewModel::setEnableKcpProxy(bool v) {
    if (m_conf.enableKcpProxy == v) return;
    m_conf.enableKcpProxy = v; markDirty(); emit enableKcpProxyChanged();
}
bool ConfigEditorViewModel::disableKcpInput() const { return m_conf.disableKcpInput; }
void ConfigEditorViewModel::setDisableKcpInput(bool v) {
    if (m_conf.disableKcpInput == v) return;
    m_conf.disableKcpInput = v; markDirty(); emit disableKcpInputChanged();
}
bool ConfigEditorViewModel::enableQuicProxy() const { return m_conf.enableQuicProxy; }
void ConfigEditorViewModel::setEnableQuicProxy(bool v) {
    if (m_conf.enableQuicProxy == v) return;
    m_conf.enableQuicProxy = v; markDirty(); emit enableQuicProxyChanged();
}
bool ConfigEditorViewModel::disableQuicInput() const { return m_conf.disableQuicInput; }
void ConfigEditorViewModel::setDisableQuicInput(bool v) {
    if (m_conf.disableQuicInput == v) return;
    m_conf.disableQuicInput = v; markDirty(); emit disableQuicInputChanged();
}
bool ConfigEditorViewModel::disableRelayKcp() const { return m_conf.disableRelayKcp; }
void ConfigEditorViewModel::setDisableRelayKcp(bool v) {
    if (m_conf.disableRelayKcp == v) return;
    m_conf.disableRelayKcp = v; markDirty(); emit disableRelayKcpChanged();
}
bool ConfigEditorViewModel::disableRelayQuic() const { return m_conf.disableRelayQuic; }
void ConfigEditorViewModel::setDisableRelayQuic(bool v) {
    if (m_conf.disableRelayQuic == v) return;
    m_conf.disableRelayQuic = v; markDirty(); emit disableRelayQuicChanged();
}
bool ConfigEditorViewModel::enableRelayForeignNetworkKcp() const { return m_conf.enableRelayForeignNetworkKcp; }
void ConfigEditorViewModel::setEnableRelayForeignNetworkKcp(bool v) {
    if (m_conf.enableRelayForeignNetworkKcp == v) return;
    m_conf.enableRelayForeignNetworkKcp = v; markDirty(); emit enableRelayForeignNetworkKcpChanged();
}
bool ConfigEditorViewModel::enableRelayForeignNetworkQuic() const { return m_conf.enableRelayForeignNetworkQuic; }
void ConfigEditorViewModel::setEnableRelayForeignNetworkQuic(bool v) {
    if (m_conf.enableRelayForeignNetworkQuic == v) return;
    m_conf.enableRelayForeignNetworkQuic = v; markDirty(); emit enableRelayForeignNetworkQuicChanged();
}

// ==================== P2P 连接 ====================

bool ConfigEditorViewModel::disableUdpHolePunching() const { return m_conf.disableUdpHolePunching; }
void ConfigEditorViewModel::setDisableUdpHolePunching(bool v) {
    if (m_conf.disableUdpHolePunching == v) return;
    m_conf.disableUdpHolePunching = v; markDirty(); emit disableUdpHolePunchingChanged();
}
bool ConfigEditorViewModel::disableTcpHolePunching() const { return m_conf.disableTcpHolePunching; }
void ConfigEditorViewModel::setDisableTcpHolePunching(bool v) {
    if (m_conf.disableTcpHolePunching == v) return;
    m_conf.disableTcpHolePunching = v; markDirty(); emit disableTcpHolePunchingChanged();
}
bool ConfigEditorViewModel::disableUpnp() const { return m_conf.disableUpnp; }
void ConfigEditorViewModel::setDisableUpnp(bool v) {
    if (m_conf.disableUpnp == v) return;
    m_conf.disableUpnp = v; markDirty(); emit disableUpnpChanged();
}
bool ConfigEditorViewModel::needP2p() const { return m_conf.needP2p; }
void ConfigEditorViewModel::setNeedP2p(bool v) {
    if (m_conf.needP2p == v) return;
    m_conf.needP2p = v; markDirty(); emit needP2pChanged();
}
bool ConfigEditorViewModel::lazyP2p() const { return m_conf.lazyP2p; }
void ConfigEditorViewModel::setLazyP2p(bool v) {
    if (m_conf.lazyP2p == v) return;
    m_conf.lazyP2p = v; markDirty(); emit lazyP2pChanged();
}
bool ConfigEditorViewModel::p2pOnly() const { return m_conf.p2pOnly; }
void ConfigEditorViewModel::setP2pOnly(bool v) {
    if (m_conf.p2pOnly == v) return;
    m_conf.p2pOnly = v; markDirty(); emit p2pOnlyChanged();
}
bool ConfigEditorViewModel::disableP2p() const { return m_conf.disableP2p; }
void ConfigEditorViewModel::setDisableP2p(bool v) {
    if (m_conf.disableP2p == v) return;
    m_conf.disableP2p = v; markDirty(); emit disableP2pChanged();
}
bool ConfigEditorViewModel::disableSymHolePunching() const { return m_conf.disableSymHolePunching; }
void ConfigEditorViewModel::setDisableSymHolePunching(bool v) {
    if (m_conf.disableSymHolePunching == v) return;
    m_conf.disableSymHolePunching = v; markDirty(); emit disableSymHolePunchingChanged();
}
bool ConfigEditorViewModel::relayAllPeerRpc() const { return m_conf.relayAllPeerRpc; }
void ConfigEditorViewModel::setRelayAllPeerRpc(bool v) {
    if (m_conf.relayAllPeerRpc == v) return;
    m_conf.relayAllPeerRpc = v; markDirty(); emit relayAllPeerRpcChanged();
}
bool ConfigEditorViewModel::bindDevice() const { return m_conf.bindDevice; }
void ConfigEditorViewModel::setBindDevice(bool v) {
    if (m_conf.bindDevice == v) return;
    m_conf.bindDevice = v; markDirty(); emit bindDeviceChanged();
}

// ==================== 性能与系统 ====================

bool ConfigEditorViewModel::multiThread() const { return m_conf.multiThread; }
void ConfigEditorViewModel::setMultiThread(bool v) {
    if (m_conf.multiThread == v) return;
    m_conf.multiThread = v; markDirty(); emit multiThreadChanged();
}
bool ConfigEditorViewModel::useSmoltcp() const { return m_conf.useSmoltcp; }
void ConfigEditorViewModel::setUseSmoltcp(bool v) {
    if (m_conf.useSmoltcp == v) return;
    m_conf.useSmoltcp = v; markDirty(); emit useSmoltcpChanged();
}
bool ConfigEditorViewModel::enableIpv6() const { return m_conf.enableIpv6; }
void ConfigEditorViewModel::setEnableIpv6(bool v) {
    if (m_conf.enableIpv6 == v) return;
    m_conf.enableIpv6 = v; markDirty(); emit enableIpv6Changed();
}
QString ConfigEditorViewModel::devName() const { return m_conf.devName; }
void ConfigEditorViewModel::setDevName(const QString &v) {
    if (m_conf.devName == v) return;
    m_conf.devName = v; markDirty(); emit devNameChanged();
}

// ==================== 网络服务与列表 ====================

bool ConfigEditorViewModel::enableExitNode() const { return m_conf.enableExitNode; }
void ConfigEditorViewModel::setEnableExitNode(bool v) {
    if (m_conf.enableExitNode == v) return;
    m_conf.enableExitNode = v; markDirty(); emit enableExitNodeChanged();
}
bool ConfigEditorViewModel::systemForwarding() const { return m_conf.systemForwarding; }
void ConfigEditorViewModel::setSystemForwarding(bool v) {
    if (m_conf.systemForwarding == v) return;
    m_conf.systemForwarding = v; markDirty(); emit systemForwardingChanged();
}
bool ConfigEditorViewModel::acceptDns() const { return m_conf.acceptDns; }
void ConfigEditorViewModel::setAcceptDns(bool v) {
    if (m_conf.acceptDns == v) return;
    m_conf.acceptDns = v; markDirty(); emit acceptDnsChanged();
}
QString ConfigEditorViewModel::defaultProtocol() const { return m_conf.defaultProtocol; }
void ConfigEditorViewModel::setDefaultProtocol(const QString &v) {
    if (m_conf.defaultProtocol == v) return;
    m_conf.defaultProtocol = v; markDirty(); emit defaultProtocolChanged();
}
QStringList ConfigEditorViewModel::exitNodes() const { return m_conf.exitNodes; }
void ConfigEditorViewModel::setExitNodes(const QStringList &v) {
    if (m_conf.exitNodes == v) return;
    m_conf.exitNodes = v; markDirty(); emit exitNodesChanged();
}
bool ConfigEditorViewModel::enableForeignNetworkWhitelist() const { return m_conf.enableForeignNetworkWhitelist; }
void ConfigEditorViewModel::setEnableForeignNetworkWhitelist(bool v) {
    if (m_conf.enableForeignNetworkWhitelist == v) return;
    m_conf.enableForeignNetworkWhitelist = v; markDirty(); emit enableForeignNetworkWhitelistChanged();
}
QString ConfigEditorViewModel::foreignNetworkWhitelist() const { return m_conf.foreignNetworkWhitelist; }
void ConfigEditorViewModel::setForeignNetworkWhitelist(const QString &v) {
    if (m_conf.foreignNetworkWhitelist == v) return;
    m_conf.foreignNetworkWhitelist = v; markDirty(); emit foreignNetworkWhitelistChanged();
}

// ==================== 安全模式 ====================

bool ConfigEditorViewModel::secureModeEnabled() const { return m_conf.secureModeEnabled; }
void ConfigEditorViewModel::setSecureModeEnabled(bool v) {
    if (m_conf.secureModeEnabled == v) return;
    m_conf.secureModeEnabled = v; markDirty(); emit secureModeEnabledChanged();
}
QString ConfigEditorViewModel::localPrivateKey() const { return m_conf.localPrivateKey; }
void ConfigEditorViewModel::setLocalPrivateKey(const QString &v) {
    if (m_conf.localPrivateKey == v) return;
    m_conf.localPrivateKey = v; markDirty(); emit localPrivateKeyChanged();
}

// ==================== 编辑操作（load / save / cancel / clear）====================

/**
 * @brief 从仓库加载指定配置到编辑器
 *
 * 加载流程：
 * 1. 通过 repo 查询 instanceName 对应的配置
 * 2. 配置不存在 → 设置错误消息 "配置不存在: {name}"
 *    - m_conf 重置为默认空配置（instanceName 为空）
 *    - 全量刷新所有字段绑定
 * 3. 配置存在 → 用仓库数据替换 m_conf
 *    - 清除所有错误消息
 *    - 全量刷新所有字段绑定
 *
 * 无论成功或失败，结束前都会重置 m_hasUnsavedChanges 为 false，
 * 因为此时的 m_conf 与仓库版本一致（空配置也算一致）。
 *
 * @param instanceName 配置实例名
 */
void ConfigEditorViewModel::loadConfig(const QString &instanceName)
{
    auto loaded = m_repo->load(instanceName);
    if (!loaded.has_value()) {
        // 仓库中无此实例：显示错误，重置编辑器为零状态
        m_errorMessages = QStringList{QStringLiteral("配置不存在: %1").arg(instanceName)};
        m_conf = NetworkConf();
        m_hasUnsavedChanges = false;
        emitCurrentChanged();
        emit errorMessagesChanged();
        emit hasUnsavedChangesChanged();
        emit currentInstanceNameChanged();
        return;
    }
    // 仓库返回正常配置：替换编辑器内部的配置副本
    m_conf = loaded.value();
    m_hasUnsavedChanges = false;
    m_errorMessages.clear();
    emitCurrentChanged();
    emit hasUnsavedChangesChanged();
    emit errorMessagesChanged();
    emit currentInstanceNameChanged();
}

/**
 * @brief 将当前编辑的配置保存到仓库
 * @return true 保存成功，false 保存失败
 *
 * 保存失败时（如 instanceName 为空），错误消息写入 errorMessages，
 * hasUnsavedChanges 保持为 true 不变——因为数据尚未持久化，用户仍可重试。
 * QML 通过监听 errorMessagesChanged 展示错误提示。
 *
 * @note 无论增还是改，都走同样的 save 路径，repo 内部通过 INSERT OR REPLACE 处理。
 */
bool ConfigEditorViewModel::save()
{
    if (!m_repo->save(m_conf)) {
        m_errorMessages = QStringList{QStringLiteral("保存配置失败")};
        emit errorMessagesChanged();
        return false;
    }
    m_hasUnsavedChanges = false;
    m_errorMessages.clear();
    emit hasUnsavedChangesChanged();
    emit errorMessagesChanged();
    return true;
}

/**
 * @brief 放弃未保存的修改，从仓库重新加载最新版本
 *
 * 实现为 loadConfig(currentInstanceName()) 的语义糖。
 * 当当前配置没有 instanceName 时（例如新建后尚未保存的空白配置），
 * 什么都不做——因为没有"仓库版本"可回退。
 *
 * @note 如果需要在无 instanceName 时也清空编辑器，应在调用方先调用 clear()。
 */
void ConfigEditorViewModel::cancel()
{
    if (m_conf.instanceName().isEmpty()) {
        // 无实例名 → 无法加载仓库版本，清空编辑器
        clear();
        return;
    }
    loadConfig(m_conf.instanceName());
}

/**
 * @brief 清空编辑器为默认状态
 *
 * 将所有内部状态重置为初始值：
 * - m_conf 替换为默认构造的 NetworkConf
 * - hasUnsavedChanges 置 false
 * - errorMessages 清空
 * - 发射所有字段变更信号 + 状态信号
 *
 * 典型使用场景：用户点击"新建配置"时先 clear 再打开空白编辑器。
 */
void ConfigEditorViewModel::clear()
{
    m_conf = NetworkConf();
    m_hasUnsavedChanges = false;
    m_errorMessages.clear();
    emit currentInstanceNameChanged();
    emit hasUnsavedChangesChanged();
    emit errorMessagesChanged();
    emitCurrentChanged();
}

/**
 * @brief 一次性发射所有字段的变更信号
 *
 * 用于批量替换 m_conf 的场景（load / clear），不需要逐个比较新旧值，
 * 直接通知 QML 侧所有属性绑定重新求值。
 *
 * 每个 Q_PROPERTY 的 NOTIFY 信号对应一行 emit 语句。
 * 信号命名规则：{propertyName}Changed()。
 *
 * @warning 新增 NetworkConf 字段时，此处必须同步新增对应的 emit 行，
 *          否则 QML 绑定不会自动刷新新字段的显示。
 */
void ConfigEditorViewModel::emitCurrentChanged()
{
    emit displayNameChanged();
    emit hostnameChanged();
    emit networkNameChanged();
    emit networkSecretChanged();
    emit dhcpChanged();
    emit ipv4Changed();
    emit latencyFirstChanged();
    emit privateModeChanged();
    emit serversChanged();
    emit enableEncryptionChanged();
    emit encryptionAlgorithmChanged();
    emit noTunChanged();
    emit mtuChanged();
    emit listenAddressesChanged();
    emit proxyNetworksChanged();
    emit customRoutesChanged();
    emit enableKcpProxyChanged();
    emit disableKcpInputChanged();
    emit enableQuicProxyChanged();
    emit disableQuicInputChanged();
    emit disableRelayKcpChanged();
    emit disableRelayQuicChanged();
    emit enableRelayForeignNetworkKcpChanged();
    emit enableRelayForeignNetworkQuicChanged();
    emit disableUdpHolePunchingChanged();
    emit disableTcpHolePunchingChanged();
    emit disableUpnpChanged();
    emit needP2pChanged();
    emit lazyP2pChanged();
    emit p2pOnlyChanged();
    emit disableP2pChanged();
    emit disableSymHolePunchingChanged();
    emit relayAllPeerRpcChanged();
    emit bindDeviceChanged();
    emit multiThreadChanged();
    emit useSmoltcpChanged();
    emit enableIpv6Changed();
    emit devNameChanged();
    emit enableExitNodeChanged();
    emit systemForwardingChanged();
    emit acceptDnsChanged();
    emit defaultProtocolChanged();
    emit exitNodesChanged();
    emit enableForeignNetworkWhitelistChanged();
    emit foreignNetworkWhitelistChanged();
    emit secureModeEnabledChanged();
    emit localPrivateKeyChanged();
}
