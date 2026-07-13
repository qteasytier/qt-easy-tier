/**
 * @file ConfigEditorViewModel.h
 * @brief Schema 驱动的配置编辑器 ViewModel
 *
 * 本类是配置编辑器的核心 ViewModel，负责：
 * 1. 将 NetworkConf 的每个字段暴露为独立的 Q_PROPERTY，供 QML 双向绑定
 * 2. 管理编辑状态（未保存变更追踪 / 错误消息列表）
 * 3. 通过 NetworkConfigRepository 实现配置的加载、保存、取消、清除
 *
 * 设计特点：
 * - 每个字段都有独立的 getter/setter/NOTIFY 信号，QML 可按需绑定单个字段
 * - setter 采用"值比较 → 赋值 → markDirty → emit"的统一防抖模式
 * - loadConfig/clear 之后通过 emitCurrentChanged() 一次性刷新所有字段绑定
 * - 不持有 repo 的所有权（由外部传入，生命周期由调用方管理）
 *
 * 字段分组对应 NetworkConf 的成员分组：
 *   元数据 → 基础设置 → 加密基础 → 传输协议 → P2P 连接 → 性能与系统 →
 *   网络服务与列表 → 安全模式
 */
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include "core/config/NetworkConf.h"

class NetworkConfigRepository;

/**
 * @brief Schema 驱动的配置编辑器 ViewModel，将 NetworkConf 各字段暴露为独立 Q_PROPERTY 供 QML 双向绑定
 */
class ConfigEditorViewModel : public QObject {
    Q_OBJECT

    // ==================== 编辑器状态属性 ====================

    /// 当前正在编辑的配置实例名称（如 "my-vpn-1"）
    Q_PROPERTY(QString currentInstanceName READ currentInstanceName NOTIFY currentInstanceNameChanged FINAL)

    /// 是否有尚未保存的修改（true 时 QML 启用"保存"按钮、"放弃修改"提示等）
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY hasUnsavedChangesChanged FINAL)

    /// 操作结果的错误消息列表（空列表表示无错误，QML 据此显示/隐藏错误提示）
    Q_PROPERTY(QStringList errorMessages READ errorMessages NOTIFY errorMessagesChanged FINAL)

    // ==================== 元数据 ====================

    /// 配置的显示名称（仅存储于 network_configs 元数据表，不进入 TOML 导出）
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged FINAL)

    // ==================== 基础设置 ====================

    /// 当前设备的主机名标识
    Q_PROPERTY(QString hostname READ hostname WRITE setHostname NOTIFY hostnameChanged FINAL)
    /// 网络名称（节点间加入同一网络用）
    Q_PROPERTY(QString networkName READ networkName WRITE setNetworkName NOTIFY networkNameChanged FINAL)
    /// 网络密钥（加入网络的认证凭证）
    Q_PROPERTY(QString networkSecret READ networkSecret WRITE setNetworkSecret NOTIFY networkSecretChanged FINAL)
    /// 是否使用 DHCP 自动分配 IP（关闭时手动配置 ipv4）
    Q_PROPERTY(bool dhcp READ dhcp WRITE setDhcp NOTIFY dhcpChanged FINAL)
    /// 静态 IPv4 地址（dhcp 为 false 时生效）
    Q_PROPERTY(QString ipv4 READ ipv4 WRITE setIpv4 NOTIFY ipv4Changed FINAL)
    /// 是否优先选择低延迟路径（而非默认的高吞吐路径）
    Q_PROPERTY(bool latencyFirst READ latencyFirst WRITE setLatencyFirst NOTIFY latencyFirstChanged FINAL)
    /// 私密模式（不共享节点信息给其他 peer）
    Q_PROPERTY(bool privateMode READ privateMode WRITE setPrivateMode NOTIFY privateModeChanged FINAL)
    /// 服务器列表（QVariantList 包装，每项含 uri 和 publicKey 两个字符串字段）
    Q_PROPERTY(QVariantList servers READ servers WRITE setServers NOTIFY serversChanged FINAL)

    // ==================== 加密基础 ====================

    /// 是否启用端到端加密
    Q_PROPERTY(bool enableEncryption READ enableEncryption WRITE setEnableEncryption NOTIFY enableEncryptionChanged FINAL)
    /// 加密算法名称（如 "aes-gcm"）
    Q_PROPERTY(QString encryptionAlgorithm READ encryptionAlgorithm WRITE setEncryptionAlgorithm NOTIFY encryptionAlgorithmChanged FINAL)
    /// 不创建 TUN 虚拟网卡（仅作代理转发，不接管系统路由）
    Q_PROPERTY(bool noTun READ noTun WRITE setNoTun NOTIFY noTunChanged FINAL)
    /// TUN 设备 MTU 值（默认 1380）
    Q_PROPERTY(int mtu READ mtu WRITE setMtu NOTIFY mtuChanged FINAL)
    /// 监听的本地地址列表
    Q_PROPERTY(QStringList listenAddresses READ listenAddresses WRITE setListenAddresses NOTIFY listenAddressesChanged FINAL)
    /// 代理网络列表
    Q_PROPERTY(QVariantList proxyNetworks READ proxyNetworks WRITE setProxyNetworks NOTIFY proxyNetworksChanged FINAL)
    /// 自定义路由列表
    Q_PROPERTY(QStringList customRoutes READ customRoutes WRITE setCustomRoutes NOTIFY customRoutesChanged FINAL)

    // ==================== 传输协议 ====================

    /// 是否启用 KCP 代理隧道
    Q_PROPERTY(bool enableKcpProxy READ enableKcpProxy WRITE setEnableKcpProxy NOTIFY enableKcpProxyChanged FINAL)
    /// 是否禁用 KCP 入站连接
    Q_PROPERTY(bool disableKcpInput READ disableKcpInput WRITE setDisableKcpInput NOTIFY disableKcpInputChanged FINAL)
    /// 是否启用 QUIC 代理隧道
    Q_PROPERTY(bool enableQuicProxy READ enableQuicProxy WRITE setEnableQuicProxy NOTIFY enableQuicProxyChanged FINAL)
    /// 是否禁用 QUIC 入站连接
    Q_PROPERTY(bool disableQuicInput READ disableQuicInput WRITE setDisableQuicInput NOTIFY disableQuicInputChanged FINAL)
    /// 是否禁用中继节点的 KCP 传输
    Q_PROPERTY(bool disableRelayKcp READ disableRelayKcp WRITE setDisableRelayKcp NOTIFY disableRelayKcpChanged FINAL)
    /// 是否禁用中继节点的 QUIC 传输
    Q_PROPERTY(bool disableRelayQuic READ disableRelayQuic WRITE setDisableRelayQuic NOTIFY disableRelayQuicChanged FINAL)
    /// 是否允许外部网络中继走 KCP
    Q_PROPERTY(bool enableRelayForeignNetworkKcp READ enableRelayForeignNetworkKcp WRITE setEnableRelayForeignNetworkKcp NOTIFY enableRelayForeignNetworkKcpChanged FINAL)
    /// 是否允许外部网络中继走 QUIC
    Q_PROPERTY(bool enableRelayForeignNetworkQuic READ enableRelayForeignNetworkQuic WRITE setEnableRelayForeignNetworkQuic NOTIFY enableRelayForeignNetworkQuicChanged FINAL)

    // ==================== P2P 连接 ====================

    /// 是否禁用 UDP 打洞
    Q_PROPERTY(bool disableUdpHolePunching READ disableUdpHolePunching WRITE setDisableUdpHolePunching NOTIFY disableUdpHolePunchingChanged FINAL)
    /// 是否禁用 TCP 打洞
    Q_PROPERTY(bool disableTcpHolePunching READ disableTcpHolePunching WRITE setDisableTcpHolePunching NOTIFY disableTcpHolePunchingChanged FINAL)
    /// 是否禁用 UPnP 端口映射
    Q_PROPERTY(bool disableUpnp READ disableUpnp WRITE setDisableUpnp NOTIFY disableUpnpChanged FINAL)
    /// 是否需要尝试建立 P2P 直连
    Q_PROPERTY(bool needP2p READ needP2p WRITE setNeedP2p NOTIFY needP2pChanged FINAL)
    /// 是否启用懒加载 P2P（按需建立而非启动即连）
    Q_PROPERTY(bool lazyP2p READ lazyP2p WRITE setLazyP2p NOTIFY lazyP2pChanged FINAL)
    /// 是否仅使用 P2P 模式（强制直连，不走中继）
    Q_PROPERTY(bool p2pOnly READ p2pOnly WRITE setP2pOnly NOTIFY p2pOnlyChanged FINAL)
    /// 是否完全禁用 P2P 连接
    Q_PROPERTY(bool disableP2p READ disableP2p WRITE setDisableP2p NOTIFY disableP2pChanged FINAL)
    /// 是否禁用对称 NAT 打洞
    Q_PROPERTY(bool disableSymHolePunching READ disableSymHolePunching WRITE setDisableSymHolePunching NOTIFY disableSymHolePunchingChanged FINAL)
    /// 是否将所有 peer 的 RPC 都通过中继转发
    Q_PROPERTY(bool relayAllPeerRpc READ relayAllPeerRpc WRITE setRelayAllPeerRpc NOTIFY relayAllPeerRpcChanged FINAL)
    /// 是否绑定特定网络设备
    Q_PROPERTY(bool bindDevice READ bindDevice WRITE setBindDevice NOTIFY bindDeviceChanged FINAL)

    // ==================== 性能与系统 ====================

    /// 是否启用多线程处理
    Q_PROPERTY(bool multiThread READ multiThread WRITE setMultiThread NOTIFY multiThreadChanged FINAL)
    /// 是否使用 smoltcp 用户态协议栈（替代系统内核协议栈）
    Q_PROPERTY(bool useSmoltcp READ useSmoltcp WRITE setUseSmoltcp NOTIFY useSmoltcpChanged FINAL)
    /// 是否启用 IPv6 支持
    Q_PROPERTY(bool enableIpv6 READ enableIpv6 WRITE setEnableIpv6 NOTIFY enableIpv6Changed FINAL)
    /// TUN 虚拟网卡设备名称
    Q_PROPERTY(QString devName READ devName WRITE setDevName NOTIFY devNameChanged FINAL)

    // ==================== 网络服务与列表 ====================

    /// 是否启用出口节点功能（作为网关转发流量）
    Q_PROPERTY(bool enableExitNode READ enableExitNode WRITE setEnableExitNode NOTIFY enableExitNodeChanged FINAL)
    /// 是否启用系统级 IP 转发
    Q_PROPERTY(bool systemForwarding READ systemForwarding WRITE setSystemForwarding NOTIFY systemForwardingChanged FINAL)
    /// 是否接受 DNS 请求
    Q_PROPERTY(bool acceptDns READ acceptDns WRITE setAcceptDns NOTIFY acceptDnsChanged FINAL)
    /// 默认传输协议（如 "tcp"、"kcp"、"quic"）
    Q_PROPERTY(QString defaultProtocol READ defaultProtocol WRITE setDefaultProtocol NOTIFY defaultProtocolChanged FINAL)
    /// 出口节点列表
    Q_PROPERTY(QStringList exitNodes READ exitNodes WRITE setExitNodes NOTIFY exitNodesChanged FINAL)
    /// 是否启用外部网络白名单模式
    Q_PROPERTY(bool enableForeignNetworkWhitelist READ enableForeignNetworkWhitelist WRITE setEnableForeignNetworkWhitelist NOTIFY enableForeignNetworkWhitelistChanged FINAL)
    /// 外部网络白名单内容（启用白名单时生效）
    Q_PROPERTY(QString foreignNetworkWhitelist READ foreignNetworkWhitelist WRITE setForeignNetworkWhitelist NOTIFY foreignNetworkWhitelistChanged FINAL)

    // ==================== 安全模式 ====================

    /// 是否启用安全模式（要求本地私钥认证）
    Q_PROPERTY(bool secureModeEnabled READ secureModeEnabled WRITE setSecureModeEnabled NOTIFY secureModeEnabledChanged FINAL)
    /// 本地私钥（安全模式下使用）
    Q_PROPERTY(QString localPrivateKey READ localPrivateKey WRITE setLocalPrivateKey NOTIFY localPrivateKeyChanged FINAL)

public:
    /**
     * @brief 构造编辑器 ViewModel
     * @param repo 配置持久化仓库指针（外部管理生命周期，不转移所有权）
     * @param parent 父 QObject，通常为 QQmlApplicationEngine 以避免 double-free
     */
    explicit ConfigEditorViewModel(NetworkConfigRepository *repo, QObject *parent = nullptr);

    // 编辑器状态访问器
    QString currentInstanceName() const;
    bool hasUnsavedChanges() const;
    QStringList errorMessages() const;

    // ==================== 元数据 getter/setter ====================
    QString displayName() const;         void setDisplayName(const QString &v);

    // ==================== 基础设置 getters/setters ====================
    QString hostname() const;            void setHostname(const QString &v);
    QString networkName() const;         void setNetworkName(const QString &v);
    QString networkSecret() const;       void setNetworkSecret(const QString &v);
    bool dhcp() const;                   void setDhcp(bool v);
    QString ipv4() const;                void setIpv4(const QString &v);
    bool latencyFirst() const;           void setLatencyFirst(bool v);
    bool privateMode() const;            void setPrivateMode(bool v);
    QVariantList servers() const;         void setServers(const QVariantList &v);

    // ==================== 加密基础 getters/setters ====================
    bool enableEncryption() const;       void setEnableEncryption(bool v);
    QString encryptionAlgorithm() const; void setEncryptionAlgorithm(const QString &v);
    bool noTun() const;                  void setNoTun(bool v);
    int mtu() const;                     void setMtu(int v);
    QStringList listenAddresses() const; void setListenAddresses(const QStringList &v);
    QVariantList proxyNetworks() const;  void setProxyNetworks(const QVariantList &v);
    QStringList customRoutes() const;    void setCustomRoutes(const QStringList &v);

    // ==================== 传输协议 getters/setters ====================
    bool enableKcpProxy() const;          void setEnableKcpProxy(bool v);
    bool disableKcpInput() const;         void setDisableKcpInput(bool v);
    bool enableQuicProxy() const;         void setEnableQuicProxy(bool v);
    bool disableQuicInput() const;        void setDisableQuicInput(bool v);
    bool disableRelayKcp() const;         void setDisableRelayKcp(bool v);
    bool disableRelayQuic() const;        void setDisableRelayQuic(bool v);
    bool enableRelayForeignNetworkKcp() const;  void setEnableRelayForeignNetworkKcp(bool v);
    bool enableRelayForeignNetworkQuic() const; void setEnableRelayForeignNetworkQuic(bool v);

    // ==================== P2P 连接 getters/setters ====================
    bool disableUdpHolePunching() const;  void setDisableUdpHolePunching(bool v);
    bool disableTcpHolePunching() const;  void setDisableTcpHolePunching(bool v);
    bool disableUpnp() const;             void setDisableUpnp(bool v);
    bool needP2p() const;                 void setNeedP2p(bool v);
    bool lazyP2p() const;                 void setLazyP2p(bool v);
    bool p2pOnly() const;                 void setP2pOnly(bool v);
    bool disableP2p() const;              void setDisableP2p(bool v);
    bool disableSymHolePunching() const;  void setDisableSymHolePunching(bool v);
    bool relayAllPeerRpc() const;         void setRelayAllPeerRpc(bool v);
    bool bindDevice() const;              void setBindDevice(bool v);

    // ==================== 性能与系统 getters/setters ====================
    bool multiThread() const;             void setMultiThread(bool v);
    bool useSmoltcp() const;              void setUseSmoltcp(bool v);
    bool enableIpv6() const;              void setEnableIpv6(bool v);
    QString devName() const;              void setDevName(const QString &v);

    // ==================== 网络服务与列表 getters/setters ====================
    bool enableExitNode() const;          void setEnableExitNode(bool v);
    bool systemForwarding() const;        void setSystemForwarding(bool v);
    bool acceptDns() const;               void setAcceptDns(bool v);
    QString defaultProtocol() const;      void setDefaultProtocol(const QString &v);
    QStringList exitNodes() const;        void setExitNodes(const QStringList &v);
    bool enableForeignNetworkWhitelist() const; void setEnableForeignNetworkWhitelist(bool v);
    QString foreignNetworkWhitelist() const; void setForeignNetworkWhitelist(const QString &v);

    // ==================== 安全模式 getters/setters ====================
    bool secureModeEnabled() const;        void setSecureModeEnabled(bool v);
    QString localPrivateKey() const;        void setLocalPrivateKey(const QString &v);

    /**
     * @brief 从仓库加载指定实例名称的配置到编辑器
     * @param instanceName 配置实例名（对应 network_configs 表主键）
     *
     * 加载流程：
     * 1. 通过 repo 查询实例
     * 2. 不存在 → 重置为空配置 + 设置错误消息 + 全量刷新绑定
     * 3. 存在 → 替换 m_conf + 清除错误 + 全量刷新绑定
     *
     * 此方法被 QML 直接调用（Q_INVOKABLE），也会被 cancel() 复用。
     */
    Q_INVOKABLE void loadConfig(const QString &instanceName);

    /**
     * @brief 将当前编辑的配置保存到仓库
     * @return true 保存成功，false 保存失败（错误消息写入 errorMessages）
     *
     * 保存成功后重置 hasUnsavedChanges 为 false。
     * 如果 repo 返回 false（如 instanceName 为空），不会自动弹窗，
     * 由 QML 侧监听 errorMessagesChanged 来展示错误。
     */
    Q_INVOKABLE bool save();

    /**
     * @brief 放弃所有未保存的修改，重新从仓库加载
     *
     * 实际上是 loadConfig(currentInstanceName()) 的语义糖。
     * 当 currentInstanceName 为空（即新建配置尚未保存过）时，什么都不做。
     */
    Q_INVOKABLE void cancel();

    /**
     * @brief 清空当前编辑器状态为默认值
     *
     * 将 m_conf 重置为默认构造的 NetworkConf，状态全部复位。
     * 通常用于"新建配置"场景——先 clear() 再进入空白编辑界面。
     */
    Q_INVOKABLE void clear();

signals:
    // ==================== 编辑器状态信号 ====================
    /// 当前编辑的配置实例名称发生变更（loadConfig / clear 时触发）
    void currentInstanceNameChanged();
    /// 未保存变更状态发生变化（字段被编辑 / 保存 / 加载 / 清空时触发）
    void hasUnsavedChangesChanged();
    /// 错误消息列表发生变化（加载失败 / 保存失败 / 错误被清除时触发）
    void errorMessagesChanged();

    // ==================== 字段变更信号（由 setter 或 emitCurrentChanged 触发）====================
    // 每个 Q_PROPERTY 对应一个 NOTIFY 信号，QML 引擎依赖这些信号实现属性绑定更新
    // 命名规则：{propertyName}Changed

    void displayNameChanged();
    void hostnameChanged();
    void networkNameChanged();
    void networkSecretChanged();
    void dhcpChanged();
    void ipv4Changed();
    void latencyFirstChanged();
    void privateModeChanged();
    void serversChanged();
    void enableEncryptionChanged();
    void encryptionAlgorithmChanged();
    void noTunChanged();
    void mtuChanged();
    void listenAddressesChanged();
    void proxyNetworksChanged();
    void customRoutesChanged();
    void enableKcpProxyChanged();
    void disableKcpInputChanged();
    void enableQuicProxyChanged();
    void disableQuicInputChanged();
    void disableRelayKcpChanged();
    void disableRelayQuicChanged();
    void enableRelayForeignNetworkKcpChanged();
    void enableRelayForeignNetworkQuicChanged();
    void disableUdpHolePunchingChanged();
    void disableTcpHolePunchingChanged();
    void disableUpnpChanged();
    void needP2pChanged();
    void lazyP2pChanged();
    void p2pOnlyChanged();
    void disableP2pChanged();
    void disableSymHolePunchingChanged();
    void relayAllPeerRpcChanged();
    void bindDeviceChanged();
    void multiThreadChanged();
    void useSmoltcpChanged();
    void enableIpv6Changed();
    void devNameChanged();
    void enableExitNodeChanged();
    void systemForwardingChanged();
    void acceptDnsChanged();
    void defaultProtocolChanged();
    void exitNodesChanged();
    void enableForeignNetworkWhitelistChanged();
    void foreignNetworkWhitelistChanged();
    void secureModeEnabledChanged();
    void localPrivateKeyChanged();

private:
    /**
     * @brief 标记当前编辑有未保存的修改
     *
     * 采用防抖：只有从 false → true 时才发射信号，避免重复通知。
     * 每个 setter 在值变更后都会调用此方法。
     */
    void markDirty();

    /**
     * @brief 一次性发射所有字段变更信号
     *
     * 用于 load / clear 场景：底层 m_conf 批量替换后，
     * 需要通知 QML 侧所有绑定的属性重新读取当前值。
     * 不逐个比较新旧值——因为整个对象已替换，最优策略是全量发射。
     */
    void emitCurrentChanged();

    /// 配置持久化仓库（非本类所有，生命周期由外部管理）
    NetworkConfigRepository *m_repo;

    /// 当前在编辑器中暂存的网络配置副本
    NetworkConf m_conf;

    /// 未保存变更标记（true 表示 m_conf 与仓库中版本不同）
    bool m_hasUnsavedChanges = false;

    /// 操作结果错误消息列表（空列表表示无错误）
    QStringList m_errorMessages;
};
