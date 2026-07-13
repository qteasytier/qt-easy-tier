/**
 * @file NetworkConf.h
 * @brief 网络配置强类型数据模型
 *
 * 所有字段均为显式成员变量，不再使用 QVariantMap / Schema 驱动。
 * 提供 toFieldMap()/fromFieldMap() 供 SQLite 字段级存取。
 * 内存管理：纯值类型，无 raw pointer，无所有权问题。
 */
#pragma once
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

/**
 * @brief 服务器节点信息
 *
 * 包含节点地址 URI 和可选的公钥。
 * 纯数据结构，无行为逻辑。
 */
struct ServerPeer {
    QString uri;        ///< 服务器地址，格式如 tcp://host:port
    QString publicKey;  ///< 对等节点公钥（可选）
};

/**
 * @brief 代理子网信息
 *
 * cidr 为必填代理网段；mappedCidr 为可选映射网段；
 * allow 表示允许代理的协议，支持 tcp/udp/icmp。
 */
struct ProxyNetwork {
    QString cidr;
    QString mappedCidr;
    QStringList allow;
};

/**
 * @brief 网络配置的数据模型类
 *
 * 将所有 EasyTier 配置字段作为公开成员变量暴露，
 * 配合 QML/C++ 双向绑定使用。提供脏标记机制追踪变更，
 * 以及 QVariantMap 互转用于 SQLite 持久化。
 *
 * 注意：除 instanceName 外，其余字段均为公开成员直接读写，
 * 不会自动设置脏标记。外部代码需自行调用 markClean() 管理生命周期。
 */
class NetworkConf {
public:
    /**
     * @brief 构造网络配置对象，自动应用默认值
     * @param instanceName 实例名，作为配置的主键标识
     */
    explicit NetworkConf(const QString &instanceName = QString());

    // ==================== 属性访问（instanceName 有 setter/getter，其余为公开成员）====================

    /// @brief 获取配置实例名（主键标识）
    QString instanceName() const;
    /// @brief 设置实例名，并标记为脏
    void setInstanceName(const QString &name);
    /// @brief 查询脏标记（自上次 markClean 后是否有变更）
    bool dirty() const;
    /// @brief 清除脏标记
    void markClean();
    void markDirty();
    /// @brief 手动标记为脏（外部直接修改公开字段时使用）

    /**
     * @brief 将配置序列化为 QVariantMap（用于 SQLite 字段级存储）
     * @return 字段名 → 值的映射
     */
    QVariantMap toFieldMap() const;

    /**
     * @brief 从 QVariantMap 反序列化配置对象
     * @param map 字段名 → 值的映射
     * @param instanceName 实例名
     * @return 反序列化并应用默认值后的新配置对象（dirty = false）
     */
    static NetworkConf fromFieldMap(const QVariantMap &map, const QString &instanceName);

    /**
     * @brief 为配置对象填充默认值（仅填充缺失字段，已设置的字段不受影响）
     * @param conf 要填充默认值的配置对象引用
     */
    static void applyDefaults(NetworkConf &conf);

    // ==================== 元数据（不入字段表、不入导入/导出 TOML）====================

    QString displayName;  ///< 用户可见的显示名称

    // ==================== 基础设置 ====================

    QString hostname;           ///< 主机名
    QString networkName;        ///< 网络名称
    QString networkSecret;      ///< 网络密钥
    bool    dhcp = true;        ///< 是否启用 DHCP
    QString ipv4;               ///< 静态 IPv4 地址
    bool    latencyFirst = false;  ///< 是否延迟优先
    bool    privateMode = true;    ///< 是否为私有模式
    QVector<ServerPeer> servers;   ///< 服务器节点列表

    // ==================== 加密基础 ====================

    bool    enableEncryption = true;      ///< 是否启用加密
    QString encryptionAlgorithm;          ///< 加密算法（默认 aes-gcm）
    bool    noTun = false;                ///< 是否禁用 TUN 设备
    int     mtu = 1380;                   ///< MTU 值
    QStringList listenAddresses;          ///< 监听地址列表
    QVector<ProxyNetwork> proxyNetworks;  ///< 代理子网列表
    QStringList customRoutes;             ///< 自定义路由列表

    // ==================== 传输协议 ====================

    bool enableKcpProxy = false;              ///< 是否启用 KCP 代理
    bool disableKcpInput = false;            ///< 是否禁用 KCP 入站
    bool enableQuicProxy = false;            ///< 是否启用 QUIC 代理
    bool disableQuicInput = false;           ///< 是否禁用 QUIC 入站
    bool disableRelayKcp = false;            ///< 是否禁用中继 KCP
    bool disableRelayQuic = false;           ///< 是否禁用中继 QUIC
    bool enableRelayForeignNetworkKcp = false;   ///< 是否启用外部网络中继 KCP
    bool enableRelayForeignNetworkQuic = false;  ///< 是否启用外部网络中继 QUIC

    // ==================== P2P 连接 ====================

    bool disableUdpHolePunching = false;    ///< 是否禁用 UDP 打洞
    bool disableTcpHolePunching = false;    ///< 是否禁用 TCP 打洞
    bool disableUpnp = false;               ///< 是否禁用 UPnP
    bool needP2p = false;                   ///< 是否需要 P2P
    bool lazyP2p = false;                   ///< 是否启用懒 P2P
    bool p2pOnly = false;                   ///< 是否仅 P2P
    bool disableP2p = false;                ///< 是否禁用 P2P
    bool disableSymHolePunching = false;    ///< 是否禁用对称 NAT 打洞
    bool relayAllPeerRpc = false;           ///< 是否中继所有对等 RPC
    bool bindDevice = true;                 ///< 是否绑定设备

    // ==================== 性能与系统 ====================

    bool    multiThread = true;    ///< 是否启用多线程
    bool    useSmoltcp = false;    ///< 是否使用 smoltcp 栈
    bool    enableIpv6 = true;     ///< 是否启用 IPv6
    QString devName;               ///< 设备名称

    // ==================== 网络服务与列表 ====================

    bool    enableExitNode = false;                    ///< 是否启用出口节点
    bool    systemForwarding = false;                  ///< 是否系统级转发
    bool    acceptDns = false;                         ///< 是否接受 DNS 请求
    QString defaultProtocol;                           ///< 默认协议
    QStringList exitNodes;                             ///< 出口节点列表
    bool    enableForeignNetworkWhitelist = false;     ///< 是否启用外部网络白名单
    QString foreignNetworkWhitelist;                   ///< 外部网络白名单

    // ==================== 安全模式 ====================

    bool    secureModeEnabled = false;   ///< 是否启用安全模式
    QString localPrivateKey;             ///< 本地私钥

private:
    QString m_instanceName;  ///< 实例名（私有，通过 setter/getter 访问）
    bool m_dirty = false;    ///< 脏标记：自上次 markClean 后是否有字段变更

    /**
     * @brief 将 QVariant 转换为 QStringList
     *
     * 兼容两种格式：QStringList 和 QVariantList。
     * @param v 要转换的值
     * @return 转换后的字符串列表
     */
    static QStringList toStringList(const QVariant &v);
};
