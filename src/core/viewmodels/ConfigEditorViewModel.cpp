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
#include "core/config/ConfigCommandService.h"
#include "config/X25519KeyHelper.h"

#include <QUrl>

namespace {
/// 自动保存防抖间隔（毫秒）：停止编辑约 300ms 后统一落库
constexpr int kAutoSaveDelayMs = 300;
}

ConfigEditorViewModel::ConfigEditorViewModel(ConfigCommandService *commandService, QObject *parent)
    : QObject(parent)
    , m_commandService(commandService)
{
    // 构造时不从仓库加载任何数据，需由外部调用 loadConfig 或 clear 初始化编辑器状态
    // 配置自动保存防抖定时器：单次触发，超时后自动落库
    m_autoSaveTimer.setSingleShot(true);
    m_autoSaveTimer.setInterval(kAutoSaveDelayMs);
    connect(&m_autoSaveTimer, &QTimer::timeout, this, &ConfigEditorViewModel::autoSaveTimeout);

    // 表单结构元数据进程内不变，构造时构建一次缓存
    m_formSections = buildFormSections();
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
 *
 * 每个 setter 在值变更后都会调用本方法，因此自动保存的调度也统一收敛在这里，
 * 所有字段共用同一个防抖定时器，无需在单个 setter 中单独处理。
 */
void ConfigEditorViewModel::markDirty()
{
    if (!m_hasUnsavedChanges) {
        m_hasUnsavedChanges = true;
        emit hasUnsavedChangesChanged();
    }
    scheduleAutoSave();
}

/**
 * @brief 调度一次防抖自动保存
 *
 * 定时器尚未运行时才启动，连续的编辑操作会被合并为单次落库。
 * 使用 singleShot 定时器，停止编辑 kAutoSaveDelayMs 后触发 autoSaveTimeout()。
 */
void ConfigEditorViewModel::scheduleAutoSave()
{
    if (!m_autoSaveTimer.isActive())
        m_autoSaveTimer.start();
}

/**
 * @brief 防抖定时器超时槽，执行自动保存
 *
 * 仅当仍有未保存修改时才真正保存；loadConfig / clear 已重置 dirty 标记后，
 * 定时器残留触发会在此被跳过，避免把新加载的配置误写回仓库。
 */
void ConfigEditorViewModel::autoSaveTimeout()
{
    if (m_hasUnsavedChanges)
        save();
}

/**
 * @brief 立即刷写待保存修改
 *
 * 在 loadConfig / clear 及外部主动退出编辑场景前调用：
 * 停止尚未触发的防抖定时器，并立即保存当前 dirty 配置，
 * 避免最后一次编辑因定时器未到期而丢失。
 */
void ConfigEditorViewModel::flushPendingSave()
{
    if (!m_autoSaveTimer.isActive())
        return;
    m_autoSaveTimer.stop();
    if (m_hasUnsavedChanges)
        save();
}

/**
 * @brief 供 QML 调用的立即刷写入口，语义与 flushPendingSave 相同
 *
 * 用于编辑页面销毁等场景，确保防抖窗口内的修改不丢失。
 */
void ConfigEditorViewModel::flushAutoSave()
{
    flushPendingSave();
}

// ==================== 编辑器状态 getter ====================

QString ConfigEditorViewModel::currentInstanceName() const { return m_conf.instanceName(); }
bool ConfigEditorViewModel::hasUnsavedChanges() const { return m_hasUnsavedChanges; }
QStringList ConfigEditorViewModel::errorMessages() const { return m_errorMessages; }

// ==================== 表单元数据与反射读写 ====================

QVariantList ConfigEditorViewModel::formSections() const
{
    return m_formSections;
}

QVariant ConfigEditorViewModel::fieldValue(const QString &key) const
{
    return property(key.toUtf8().constData());
}

bool ConfigEditorViewModel::setFieldValue(const QString &key, const QVariant &value)
{
    // 反射写入具名属性：不存在或只读的 key 返回 false，其余语义（值比较、
    // markDirty、防抖自动保存、NOTIFY）全部由具名 setter 提供
    return setProperty(key.toUtf8().constData(), value);
}

/**
 * @brief 构建表单结构元数据
 *
 * 分组、字段顺序、显示名与控件类型沿用原 NetworkOptions.qml 的静态布局，
 * 由 QML 薄壳按 type 分发到对应渲染器。字段间联动禁用（dhcp→ipv4、
 * 白名单开关→输入框）是 UI 逻辑，不在此描述，由 QML 侧实现。
 */
QVariantList ConfigEditorViewModel::buildFormSections() const
{
    // ---- 字段描述辅助（lambda 捕获 this 以支持 tr() 中文源）----
    auto switchField = [&](const QString &key, const QString &title) {
        return QVariantMap{{"key", key}, {"title", title}, {"type", QStringLiteral("switch")}};
    };
    auto textField = [&](const QString &key, const QString &title, const QString &placeholder = {}) {
        QVariantMap f{{"key", key}, {"title", title}, {"type", QStringLiteral("textField")}};
        if (!placeholder.isEmpty())
            f.insert("placeholder", placeholder);
        return f;
    };
    auto passwordField = [&](const QString &key, const QString &title, const QString &placeholder = {}) {
        QVariantMap f{{"key", key}, {"title", title}, {"type", QStringLiteral("password")}};
        if (!placeholder.isEmpty())
            f.insert("placeholder", placeholder);
        return f;
    };
    auto comboField = [&](const QString &key, const QString &title, const QVariantList &options) {
        return QVariantMap{{"key", key}, {"title", title},
                           {"type", QStringLiteral("comboBox")}, {"options", options}};
    };
    auto spinField = [&](const QString &key, const QString &title, int from, int to) {
        return QVariantMap{{"key", key}, {"title", title},
                           {"type", QStringLiteral("spinBox")}, {"from", from}, {"to", to}};
    };
    auto listField = [&](const QString &key, const QString &title, const QString &type,
                         const QString &addTitle = {}, const QString &addDefault = {}, bool dedupe = false) {
        QVariantMap f{{"key", key}, {"title", title}, {"type", type}};
        if (!addTitle.isEmpty())
            f.insert("addTitle", addTitle);
        if (!addDefault.isEmpty())
            f.insert("addDefault", addDefault);
        if (dedupe)
            f.insert("dedupe", true);
        return f;
    };
    auto actionField = [&](const QString &type) {
        return QVariantMap{{"key", QString()}, {"title", QString()}, {"type", type}};
    };
    auto card = [&](const QString &tab, const QString &cardKey, const QString &cardTitle,
                    QVariantList fields) {
        return QVariantMap{{"tab", tab}, {"cardKey", cardKey},
                           {"cardTitle", cardTitle}, {"fields", fields}};
    };
    auto option = [&](const QString &text, const QString &value) {
        return QVariantMap{{"text", text}, {"value", value}};
    };

    // ---- 下拉选项表（从原 QML 硬编码下沉）----
    const QVariantList kEncryptionAlgorithms = {
        option("aes-gcm", "aes-gcm"), option("xor", "xor"),
        option("chacha20", "chacha20"), option("aes-gcm256", "aes-gcm256"),
    };
    const QVariantList kDefaultProtocols = {
        option(tr("不指定"), QString()), option("udp", "udp"), option("tcp", "tcp"),
        option("wg", "wg"), option("ws", "ws"), option("wss", "wss"),
    };

    // ---- 基础设置页：身份与网络基本信息 + 初始节点 ----
    const QVariantList basicCards = {
        card(QStringLiteral("basic"), QStringLiteral("basicIdentity"), QString(), {
            textField(QStringLiteral("hostname"), tr("主机名")),
            textField(QStringLiteral("networkName"), tr("网络名称")),
            passwordField(QStringLiteral("networkSecret"), tr("网络密钥")),
            switchField(QStringLiteral("dhcp"), tr("DHCP")),
            textField(QStringLiteral("ipv4"), tr("虚拟 IPv4")),
            switchField(QStringLiteral("latencyFirst"), tr("低延迟优先")),
            switchField(QStringLiteral("privateMode"), tr("私有模式")),
        }),
        card(QStringLiteral("basic"), QStringLiteral("basicServers"), tr("初始节点（服务器）"), {
            listField(QStringLiteral("servers"), QString(), QStringLiteral("serverList")),
        }),
    };

    // ---- 高级设置页：传输协议 / P2P / 性能与系统 / 网络服务 / 安全模式 ----
    const QVariantList advancedCards = {
        card(QStringLiteral("advanced"), QStringLiteral("advTransport"), tr("传输协议"), {
            switchField(QStringLiteral("enableKcpProxy"), tr("启用 KCP 代理")),
            switchField(QStringLiteral("disableKcpInput"), tr("禁用 KCP 输入")),
            switchField(QStringLiteral("enableQuicProxy"), tr("启用 QUIC 代理")),
            switchField(QStringLiteral("disableQuicInput"), tr("禁用 QUIC 输入")),
            switchField(QStringLiteral("disableRelayKcp"), tr("禁止转发 KCP")),
            switchField(QStringLiteral("disableRelayQuic"), tr("禁止转发 QUIC")),
            switchField(QStringLiteral("enableRelayForeignNetworkKcp"), tr("允许转发其他网络 KCP")),
            switchField(QStringLiteral("enableRelayForeignNetworkQuic"), tr("允许转发其他网络 QUIC")),
            switchField(QStringLiteral("enableEncryption"), tr("启用加密")),
            comboField(QStringLiteral("encryptionAlgorithm"), tr("加密算法"), kEncryptionAlgorithms),
            comboField(QStringLiteral("defaultProtocol"), tr("默认连接协议"), kDefaultProtocols),
        }),
        card(QStringLiteral("advanced"), QStringLiteral("advP2p"), tr("P2P 连接"), {
            switchField(QStringLiteral("p2pOnly"), tr("仅 P2P")),
            switchField(QStringLiteral("disableP2p"), tr("禁用 P2P")),
            switchField(QStringLiteral("needP2p"), tr("需要 P2P")),
            switchField(QStringLiteral("lazyP2p"), tr("按需 P2P")),
            switchField(QStringLiteral("disableUdpHolePunching"), tr("禁用 UDP 打洞")),
            switchField(QStringLiteral("disableTcpHolePunching"), tr("禁用 TCP 打洞")),
            switchField(QStringLiteral("disableUpnp"), tr("禁用 UPnP")),
            switchField(QStringLiteral("disableSymHolePunching"), tr("禁用对称 NAT 打洞")),
            switchField(QStringLiteral("relayAllPeerRpc"), tr("转发 RPC 包")),
            switchField(QStringLiteral("bindDevice"), tr("仅使用物理网卡")),
        }),
        card(QStringLiteral("advanced"), QStringLiteral("advPerformance"), tr("性能与系统"), {
            switchField(QStringLiteral("multiThread"), tr("启用多线程")),
            switchField(QStringLiteral("useSmoltcp"), tr("使用用户态协议栈")),
            switchField(QStringLiteral("noTun"), tr("不创建 TUN")),
            switchField(QStringLiteral("enableIpv6"), tr("启用 IPv6")),
            spinField(QStringLiteral("mtu"), tr("MTU"), 576, 1500),
            textField(QStringLiteral("devName"), tr("TUN 设备名")),
        }),
        card(QStringLiteral("advanced"), QStringLiteral("advServices"), tr("网络服务"), {
            switchField(QStringLiteral("enableExitNode"), tr("启用出口节点")),
            switchField(QStringLiteral("systemForwarding"), tr("系统转发（子网代理禁用内置 NAT）")),
            switchField(QStringLiteral("acceptDns"), tr("启用魔法 DNS")),
            switchField(QStringLiteral("enableForeignNetworkWhitelist"), tr("启用网络白名单")),
            textField(QStringLiteral("foreignNetworkWhitelist"), QString(), tr("多个网络用空格分开")),
            listField(QStringLiteral("listenAddresses"), tr("监听地址"), QStringLiteral("stringList"),
                      tr("添加监听地址"), QStringLiteral("tcp://0.0.0.0:11010"), true),
            listField(QStringLiteral("proxyNetworks"), tr("代理子网"), QStringLiteral("proxyNetworkList")),
            listField(QStringLiteral("customRoutes"), tr("自定义路由"), QStringLiteral("stringList"),
                      tr("添加自定义路由规则"), QStringLiteral("0.0.0.0/24"), true),
            listField(QStringLiteral("exitNodes"), tr("出口节点列表"), QStringLiteral("stringList"),
                      tr("添加出口节点地址"), QStringLiteral("10.126.126.1"), true),
        }),
        card(QStringLiteral("advanced"), QStringLiteral("advSecurity"), tr("安全模式"), {
            switchField(QStringLiteral("secureModeEnabled"), tr("启用安全模式")),
            passwordField(QStringLiteral("localPrivateKey"), tr("节点私钥Base64"), tr("可选，留空使用随机密钥")),
            actionField(QStringLiteral("keyActions")),
            listField(QStringLiteral("credentialFile"), tr("临时密钥文件(.json)"), QStringLiteral("filePath")),
        }),
    };

    QVariantList sections = basicCards;
    sections.append(advancedCards);
    return sections;
}

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

bool ConfigEditorViewModel::generateRandomPrivateKey()
{
    const QString key = X25519KeyHelper::generatePrivateKeyBase64();
    if (key.isEmpty())
        return false;
    setLocalPrivateKey(key);
    return true;
}

QString ConfigEditorViewModel::derivePublicKey()
{
    if (m_conf.localPrivateKey.isEmpty())
        return {};
    QString publicKey;
    if (!X25519KeyHelper::derivePublicKeyBase64(m_conf.localPrivateKey, &publicKey))
        return {};
    return publicKey;
}

QString ConfigEditorViewModel::credentialFile() const { return m_conf.credentialFile; }
void ConfigEditorViewModel::setCredentialFile(const QString &v)
{
    if (m_conf.credentialFile == v)
        return;
    m_conf.credentialFile = v.trimmed();
    markDirty();
    emit credentialFileChanged();
}

QString ConfigEditorViewModel::toLocalFilePath(const QString &urlOrPath)
{
    if (urlOrPath.isEmpty())
        return {};
    const QUrl url(urlOrPath);
    if (url.isLocalFile())
        return url.toLocalFile();
    // 已是普通本地路径（手动输入），原样返回
    return urlOrPath;
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
    // 切换配置前先刷写防抖窗口内的修改，避免最后几秒的编辑丢失
    flushPendingSave();

    auto loaded = m_commandService ? m_commandService->load(instanceName) : std::nullopt;
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
    if (!m_commandService || !m_commandService->save(m_conf)) {
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
    // 清空前先刷写防抖窗口内的修改，避免尚未落库的编辑被丢弃
    flushPendingSave();

    m_conf = NetworkConf();
    m_hasUnsavedChanges = false;
    m_errorMessages.clear();
    emit currentInstanceNameChanged();
    emit hasUnsavedChangesChanged();
    emit errorMessagesChanged();
    emitCurrentChanged();
}

/**
 * @brief 同步外部重命名的显示名称到当前编辑快照
 *
 * 仅当 instanceName 是当前编辑实例时更新；不标记 dirty、不触发自动保存，
 * 因为重命名已由协调方（ConfigListModel → ConfigCommandService）落库。
 */
void ConfigEditorViewModel::syncDisplayName(const QString &instanceName, const QString &displayName)
{
    if (instanceName != m_conf.instanceName())
        return;
    if (m_conf.displayName == displayName)
        return;

    m_conf.displayName = displayName;
    emit displayNameChanged();
}

/**
 * @brief 丢弃当前编辑快照并清空编辑器，不刷写待保存修改
 *
 * 用于配置已被删除的场景：若调用 clear() 会先 flushPendingSave()，
 * 可能把已删除配置的编辑器快照重新写回仓库（"复活"配置）。
 */
void ConfigEditorViewModel::discardAndClear()
{
    m_autoSaveTimer.stop();
    m_conf = NetworkConf();
    m_hasUnsavedChanges = false;
    m_errorMessages.clear();
    emit currentInstanceNameChanged();
    emit hasUnsavedChangesChanged();
    emit errorMessagesChanged();
    emitCurrentChanged();
}

/**
 * @brief 将当前实例的全部网络设置恢复为默认值并立即落库
 *
 * 流程：
 * 1. 无当前实例名 → 直接失败（按钮已禁用，此处为防御性检查）
 * 2. 构造默认 NetworkConf（保留显示名称 displayName），写回仓库
 * 3. 成功 → 停止防抖定时器、直接替换 m_conf、清 dirty，并全量刷新 QML 绑定
 *
 * @return true 重置成功，false 失败（errorMessages 已写入失败原因）
 */
bool ConfigEditorViewModel::resetToDefaults()
{
    const QString name = m_conf.instanceName();
    if (name.isEmpty()) {
        m_errorMessages = QStringList{QStringLiteral("无当前配置可重置")};
        emit errorMessagesChanged();
        return false;
    }

    // 构造默认配置并保留显示名称（元数据，不属于网络设置）
    NetworkConf defaults(name);
    defaults.displayName = m_conf.displayName;

    if (!m_commandService || !m_commandService->save(defaults)) {
        m_errorMessages = QStringList{QStringLiteral("重置配置失败")};
        emit errorMessagesChanged();
        return false;
    }

    // 直接替换内存副本并刷新绑定；重置本身已写入仓库，无需再走防抖保存
    m_autoSaveTimer.stop();
    m_conf = defaults;
    m_hasUnsavedChanges = false;
    m_errorMessages.clear();
    emitCurrentChanged();
    emit hasUnsavedChangesChanged();
    emit errorMessagesChanged();
    return true;
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
    emit credentialFileChanged();
}
