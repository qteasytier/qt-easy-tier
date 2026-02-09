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
#include <random>
#include <QCryptographicHash>

/// @brief 生成随机端口号
int getRandomPort()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 65535);
    return dis(gen);
}

bool isPortOccupied(const int &port);
bool isRpcPortOccupied(const int &port);

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
        if (isPortOccupied(netPage->getRpcPort()) || isRpcPortOccupied(netPage->getRpcPort())) {
            throw std::runtime_error("RPC端口已被占用");
        }
        netPage->realRpcPort = netPage->getRpcPort();
        conf << "--rpc-portal" << QString::number(netPage->getRpcPort());

    } else {

        // 循环找到一个未被占用的端口
        for (int i = 10000; i < 50000; i++) {
            int port = getRandomPort();
            if (!isPortOccupied(port) && !isRpcPortOccupied(port)) {
                conf << "--rpc-portal" << QString::number(port);
                netPage->realRpcPort = port;
                break;
            }
        }
        if (netPage->realRpcPort == 0) {
            throw std::runtime_error("未找到未被占用的RPC端口");
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

    // 网络白名单
    if (netPage->isRelayNetworkWhitelistEnabled()) {
        conf << "--relay-network-whitelist";
        const auto &whitelist = netPage->getRelayNetworkWhitelist();
        if (!whitelist.empty()) {
            for (const auto &network : whitelist) {
                conf << network ;
            }
        }
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

/// @brief 检测端口是否被其他实例的RPC占用
/// @return 如果端口被其他实例的RPC占用则返回true，否则返回false
bool isRpcPortOccupied(const int &port)
{
    // 端口合法性校验
    if (port < 1 || port > 65535) {
        std::cerr << "[RpcDetect] Invalid port:" << port << "(must be 1-65535)" << std::endl;
        return true;
    }

    // 使用QProcess执行easytier-cli -p 127.0.0.1:port peer 命令
    // 将输出返回给QString
    // 获取当前程序目录
    QString appDir = QCoreApplication::applicationDirPath() + "/etcore";
    QString cliPath = appDir + "/easytier-cli.exe";

    // 检查CLI程序是否存在
    QFileInfo fileInfo(cliPath);
    if (!fileInfo.exists()) {
        std::cerr << "[RpcDetect] Error: CLI program not found at " << cliPath.toStdString() << std::endl;
        //QMessageBox::critical(nullptr, "端口占用检测", QString("错误: 找不到 %1").arg(cliPath));
        throw std::runtime_error("RPC端口占用检测失败,找不到CLI程序");
    }

    // 创建临时进程获取节点信息
    QProcess process;
    process.setWorkingDirectory(appDir);
    process.start(cliPath, QStringList() << "-p" << "127.0.0.1:" + QString::number(port) << "peer");
    if (!process.waitForFinished(5000)) { // 5秒超时
        std::cerr << "[RpcDetect] RPC port " << port << " check timeout" << std::endl;
        //QMessageBox::critical(nullptr, "端口占用检测", "RPC端口检查超时");
        throw std::runtime_error("端口占用检测失败, RPC端口检查超时");
    }
    QString output = QString::fromLocal8Bit(process.readAllStandardError());

    // 检查输出是否包含"Error",包含则表示端口没有实例占用
    if (output.contains("Error: failed to get peer manager client")) {
        std::clog << "[RpcDetect] RPC port " << port << " is available:"<< std::endl;
        return false;
    }

    std::clog << "[RpcDetect] RPC port " << port << " is occupied:"<< std::endl;
    return true;
}

// =================== Base32 编码/解码相关函数 ===================

const QString BASE32_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

/// @brief Base32编码
/// @param data: 需要编码的字节数组
QString base32Encode(const QByteArray& data) {
    QString result;
    int buffer = 0;
    int bitsLeft = 0;

    for (int i = 0; i < data.size(); ++i) {
        buffer = (buffer << 8) | static_cast<unsigned char>(data[i]);
        bitsLeft += 8;

        while (bitsLeft >= 5) {
            result += BASE32_CHARS[(buffer >> (bitsLeft - 5)) & 0x1F];
            bitsLeft -= 5;
        }
    }

    if (bitsLeft > 0) {
        result += BASE32_CHARS[(buffer << (5 - bitsLeft)) & 0x1F];
    }

    // 补齐到4的倍数
    while (result.length() % 8 != 0) {
        result += '=';
    }

    return result;
}

/// @brief Base32解码
/// @param encoded: Base32编码字符串
QByteArray base32Decode(const QString& encoded) {
    QByteArray result;
    int buffer = 0;
    int bitsLeft = 0;

    for (int i = 0; i < encoded.length(); ++i) {
        QChar ch = encoded[i];
        if (ch == '=') continue;

        int val = BASE32_CHARS.indexOf(ch.toUpper());
        if (val == -1) continue;

        buffer = (buffer << 5) | val;
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            result.append(static_cast<char>((buffer >> (bitsLeft - 8)) & 0xFF));
            bitsLeft -= 8;
        }
    }

    return result;
}

/// @brief 生成房间凭证
/// @return 返回房间凭证(房间号，密码)
QPair<QString, QString> generateRoomCredentials() {
    std::string str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*&#$!";  //字符集
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 56); // ASCII可打印字符范围 32-126

    // 生成5字节的原始网络号数据（对应Base32编码8字符）
    QByteArray networkIdRaw(5, 0);
    for (int i = 0; i < 5; ++i) {
        networkIdRaw[i] = str[dis(gen)];
    }

    // 生成5字节的原始密码数据（对应Base32编码8字符）
    QByteArray passwordRaw(5, 0);
    for (int i = 0; i < 5; ++i) {
        passwordRaw[i] = str[dis(gen)];
    }

    // 将原始数据转换为ASCII字符串作为实际使用的网络号和密码
    QString networkId = "QtET-OneClick-" + QString::fromLatin1(networkIdRaw);
    QString password = QString::fromLatin1(passwordRaw);

    return qMakePair(networkId, password);
}

/// @brief 生成连接码（Base32）
QString encodeConnectionCode(const QString& networkId, const QString& password) {

    QString networkPureId = networkId; // 删除 "QtET-OneClick-" 前缀
    networkPureId.remove("QtET-OneClick-");
    // 将ASCII字符串转换回二进制数据
    QByteArray networkIdData = networkPureId.toLatin1();
    QByteArray passwordData = password.toLatin1();

    // 使用Base32编码
    QString encodedNetworkId = base32Encode(networkIdData);
    QString encodedPassword = base32Encode(passwordData);

    // 确保长度正确（5字节数据Base32编码后正好8个字符）
    if (encodedNetworkId.length() != 8 || encodedPassword.length() != 8) {
        return ""; // 返回空字符串表示错误
    }

    // 组合成格式：XXXX-XXXX-XXXX-XXXX
    QString code = encodedNetworkId.left(4) + "-" +
                   encodedNetworkId.mid(4, 4) + "-" +
                   encodedPassword.left(4) + "-" +
                   encodedPassword.mid(4, 4);

    return code.toUpper();
}

QPair<QString, QString> decodeConnectionCode(const QString& code) {
    // 移除所有非Base32字符并转为大写
    QString cleanCode = code;
    const static QRegularExpression re("[^A-Za-z2-7]");
    cleanCode.remove(re);
    cleanCode = cleanCode.toUpper();

    // 检查长度是否正确（应该是16个Base32字符）
    if (cleanCode.length() != 16) {
        return qMakePair(QString(), QString()); // 返回空pair表示错误
    }

    // 分离网络号和密码的Base32编码（各8个字符）
    QString encodedNetworkId = cleanCode.left(8);
    QString encodedPassword = cleanCode.mid(8, 8);

    // Base32解码（5字节数据编码后正好8字符，不需要额外填充）
    QByteArray networkIdData = base32Decode(encodedNetworkId);
    QByteArray passwordData = base32Decode(encodedPassword);

    // 转换为ASCII字符串
    QString networkPureId = QString::fromLatin1(networkIdData);
    QString password = QString::fromLatin1(passwordData);

    return qMakePair("QtET-OneClick-" + networkPureId, password);
}
