//
// Created by YMHuang on 2026/4/5.
//

#ifndef QTEASYTIER_NETWORKCONF_H
#define QTEASYTIER_NETWORKCONF_H

#include <string>
#include <vector>
#include <random>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

class QtETNetwork;

class NetworkConf
{
    // 声明 QtETNetwork 为友元，允许其直接访问私有成员
    friend class QtETNetwork;
    // 声明全局保存/读取函数为友元
    friend bool saveAllNetworkConf(const std::vector<NetworkConf>&, QString*);
    friend std::vector<NetworkConf> readAllNetworkConf();

public:
    NetworkConf();
    ~NetworkConf() = default;

    /**
     * @brief 从 QtETNetwork 的前端 UI 中读取数据并保存到成员变量
     * @param network QtETNetwork 界面对象指针
     */
    void readFromUI(const QtETNetwork* network);

    /**
     * @brief 从 JSON 对象中读取数据并保存到成员变量
     * @param json JSON 对象
     */
    void readFromJson(const QJsonObject &json);

    /**
     * @brief 输出为 JSON 对象
     * @return JSON 对象
     */
    [[nodiscard]] QJsonObject toJson() const;

    /**
     * @brief 输出为 TOML 格式字符串（暂未实现）
     * @return TOML 格式字符串
     */
    [[nodiscard]] std::string toToml() const;

    /**
     * @brief 生成随机的 instanceName
     * @return 格式为 "QtET-xxxxxxxxxx" 的随机名称
     */
    static std::string generateInstanceName();

    // Getter 方法
    [[nodiscard]] const std::string& getInstanceName() const { return m_instanceName; }
    void setInstanceName(const std::string &name) { m_instanceName = name; }
    [[nodiscard]] bool isRunning() const { return m_isRunning; }
    void setRunning(bool running) { m_isRunning = running; }

private:
    // ==================== 基础设置 ====================
    std::string m_hostname;                 ///< 用户名
    std::string m_networkName;              ///< 网络号
    std::string m_networkSecret;            ///< 网络密钥
    bool m_dhcp = true;                     ///< DHCP 开关
    std::string m_ipv4;                     ///< IPv4 地址
    bool m_latencyFirst = false;            ///< 低延迟优先开关
    bool m_privateMode = true;              ///< 私有模式开关
    std::vector<std::string> m_servers;     ///< 服务器列表

    // ==================== 高级设置 - 功能开关 ====================
    bool m_enableKcpProxy = true;           ///< 启用 KCP 代理
    bool m_disableKcpInput = false;         ///< 禁用 KCP 输入
    bool m_noTun = false;                   ///< 无 TUN 模式
    bool m_enableQuicProxy = false;         ///< 启用 QUIC 代理
    bool m_disableQuicInput = false;        ///< 禁用 QUIC 输入
    bool m_disableUdpHolePunching = false;  ///< 禁用 UDP 打洞
    bool m_multiThread = true;              ///< 启用多线程
    bool m_useSmoltcp = false;              ///< 使用用户态协议栈
    bool m_bindDevice = true;               ///< 仅使用物理网卡
    bool m_disableP2p = false;              ///< 禁用 P2P
    bool m_enableExitNode = false;          ///< 启用出口节点
    bool m_systemForwarding = false;        ///< 系统转发
    bool m_disableSymHolePunching = false;  ///< 禁用对称 NAT 打洞
    bool m_disableIpv6 = false;             ///< 禁用 IPv6
    bool m_relayAllPeerRpc = false;         ///< 转发 RPC 包
    bool m_enableEncryption = true;         ///< 启用加密
    bool m_acceptDns = false;               ///< 启用魔法 DNS

    // ==================== 高级设置 - 其他 ====================
    int m_rpcPort = 0;                      ///< RPC 端口号（0 表示随机）
    bool m_foreignNetworkWhitelistEnabled = false;  ///< 启用网络白名单
    std::vector<std::string> m_foreignNetworkWhitelist;  ///< 网络白名单
    std::vector<std::string> m_listenAddresses;    ///< 监听地址列表
    std::vector<std::string> m_proxyNetworks;      ///< 子网代理 CIDR 列表

    // ==================== 实例标识 ====================
    std::string m_instanceName;             ///< 实例名称（用于 FFI 管理）

    // ==================== 运行状态 ====================
    bool m_isRunning = false;               ///< 网络运行状态
};

/**
 * @brief 保存所有网络配置到文件
 *
 * @param confList 网络配置列表
 * @param errorMsg 存储保存失败时的错误信息的指针，若成功则为 nullptr
 *
 * @return true 成功
 * @return false 失败
 */
bool saveAllNetworkConf(const std::vector<NetworkConf> &confList, QString *errorMsg = nullptr);

/**
 * @brief 从配置文件中读取所有网络配置信息
 *
 * @return 网络配置类的列表
 */
std::vector<NetworkConf> readAllNetworkConf();

#endif //QTEASYTIER_NETWORKCONF_H
