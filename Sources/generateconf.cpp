//
// Created by YMHuang on 2026/1/27.
//

#include "generateconf.h"
#include "netpage.h"
#include <QString>
#include <iostream>
#include <QTcpServer>
#include <QUdpSocket>
#include <QHostAddress>
#include <QAbstractSocket>

bool isPortOccupied(const int &port);

// @brief: 生成EasyTier配置文件
// @param netPage: 网络配置页面指针
// @return: 生成的EasyTier配置参数Qt字符串
//
// @note: 该函数生成的是配置参数字符串，须在运行core时添加到命令行参数中
QStringList generateConfCommand(NetPage *netPage)
{
    QStringList conf;

    // 先解决RPC端口号
    if (netPage->getRpcPort() != 0) {
        if (isPortOccupied(netPage->getRpcPort())) {
            throw std::runtime_error("RPC端口号已被占用");
        }
        conf << "--rpc-portal" << QString::number(netPage->getRpcPort());

    } else {
        // 查找15888端口是否被占用
        if (!isPortOccupied(15888)) {
            conf << "--rpc-portal" << "15888";
            netPage->realRpcPort = 15888;
        } else {
            // 循环找到一个未被占用的端口
            for (int i = 10000; i < 65535; i++) {
                if (!isPortOccupied(i)) {
                    conf << "--rpc-portal" << QString::number(i);
                    netPage->realRpcPort = i;
                    break;
                }
            }
        }
    }

    //===================== 其他参数 =====================
    if (!netPage->getNetworkName().isEmpty() && !netPage->getPassword().isEmpty()) {
        conf << "--network-name" << netPage->getNetworkName();  // 网络名称
        conf << "--network-secret" << netPage->getPassword();   // 网络密钥
    }
    if (!netPage->getUsername().isEmpty()) {
        conf << "--hostname" << netPage->getUsername();         // 主机名
    }


    if (netPage->isLowLatencyPriority()) conf << "--latency-first";      // 低延迟优先

    if (netPage->isPrivateMode()) {
        conf << "--private-mode" << "true";             // 私有模式
    } else {
        conf << "--private-mode" << "false";             // 非私有模式
    }

    if (netPage->isKcpProxyEnabled()) conf << "--enable-kcp-proxy";      // KCP代理
    if (netPage->isKcpInputDisabled()) conf << "--disable-kcp-input";    // 禁用KCP输入
    if (netPage->isQuicProxyEnabled()) conf << "--enable-quic-proxy";    // QUIC代理
    if (netPage->isQuicInputDisabled()) conf << "--disable-quic-input";  // 禁用QUIC输入
    if (netPage->isTunModeDisabled()) conf << "--no-tun";                // 无TUN模式
    if (netPage->isMultithreadEnabled()) conf << "--multi-thread";       // 多线程
    if (netPage->isUdpHolePunchingDisabled()) conf << "--disable-udp-hole-punching";  // 禁用UDP打洞
    if (netPage->isUserModeStackEnabled()) conf << "--use-smoltcp";      // 用户模式栈
    if (netPage->isP2pDisabled()) conf << "--disable-p2p";               // 禁用P2P
    if (netPage->isExitNodeEnabled()) conf << "--enable-exit-node";      // 出口节点
    if (netPage->isSystemForwardingEnabled()) conf << "--proxy-forward-by-system";  // 系统转发
    if (netPage->isSymmetricNatHolePunchingDisabled()) conf << "--disable-sym-hole-punching";  // 禁用对称NAT打洞
    if (netPage->isIpv6Disabled()) conf << "--disable-ipv6";             // 禁用IPv6

    if (netPage->isBindDevice()) {
        conf << "--bind-device" << "true";                // 仅物理网卡
    } else {
        conf << "--bind-device" << "false";                // 所有网卡
    }

    if (netPage->isRpcPacketForwardingEnabled()) conf << "--relay-all-peer-rpc";  // 转发RPC包
    if (netPage->isEncryptionDisabled()) conf << "--disable-encryption"; // 禁用加密
    if (netPage->isMagicDnsEnabled()) conf << "--accept-dns";    // 启用Magic DNS

    // DHCP和IP设置
    if (netPage->isDhcpEnabled()) {
        conf << "--dhcp" << "true";  // 启用DHCP
    } else {
        conf << "--dhcp" << "false";  // 禁用DHCP
        // IP地址设置
        if (!netPage->getIpAddress().isEmpty()) {
            conf << "--ipv4" << netPage->getIpAddress();  // IPv4地址
        } else {
            std::clog << "注意：则此节点将仅转发数据包，不会创建TUN设备" << std::endl;
        }
    }

    // 服务器列表
    for (const auto &server : netPage->getServerList()) {
        conf << "--peers" << server;
    }

    // 监听地址端口列表
    const auto &listenersAddr = netPage->getListenAddrList();
    if (listenersAddr.empty()) {
        conf << "--no-listener";
    } else {
        for (const auto &listenAddr : netPage->getListenAddrList()) {
            conf << "--listeners" << listenAddr;
        }
    }

    // 子网代理CIDR列表
    for (const auto &cidr : netPage->getCidrList()) {
        conf << "-n" << cidr;
    }

    return conf;
}

// @brief: 检测端口是否被占用
// @param port: 端口号
// @return: 如果端口被占用则返回true，否则返回false
//
// @note: 该函数用于检测指定端口是否被其他进程占用
bool isPortOccupied(const int &port)
{
    // 端口合法性校验
    if (port < 1 || port > 65535) {
        std::cerr << "[PortDetect] Invalid port:" << port << "(must be 1-65535)" << std::endl;
        return true;
    }

    // 检测UDP端口：绑定失败 → 端口被占用
    QUdpSocket udpSocket;
    // ShareAddress：允许其他进程绑定同一端口；ReuseAddressHint：允许端口快速复用
    bool udpBindOk = udpSocket.bind(
        QHostAddress::LocalHost,
        port,
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
    );
    if (!udpBindOk) {
        std::clog << "[PortDetect] UDP port " << port << " is occupied:" << udpSocket.errorString().toStdString() << std::endl;
        return true;
    }
    udpSocket.close(); // 显式释放UDP端口

    // 检测TCP端口：监听失败 → 端口被占用
    QTcpServer tcpServer;
    bool tcpListenOk = tcpServer.listen(QHostAddress::LocalHost, port);
    if (!tcpListenOk) {
        std::cerr << "[PortDetect] TCP port " << port << " is occupied:" << tcpServer.errorString().toStdString() << std::endl;
        return true;
    }
    tcpServer.close(); // 显式关闭TCP监听

    // 4. UDP和TCP均未被占用
    std::clog << "[PortDetect] Port " << port << " is available (UDP + TCP)" << std::endl;
    return false;
}
