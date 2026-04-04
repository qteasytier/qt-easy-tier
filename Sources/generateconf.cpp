//
// Created by YMHuang on 2026/1/27.
//

#include "generateconf.h"
#include "netpage.h"
#include <QString>
#include <iostream>
#include <QTcpServer>
#include <random>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QApplication>
#include <QDir>

#ifdef Q_OS_WIN
// Windows 平台特定头文件
#include <winsock2.h>
#include <iphlpapi.h>
#include <Windows.h>
#elif defined(Q_OS_LINUX)
// Linux 平台特定头文件
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif


/// @brief 生成随机端口号
int getRandomPort()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 50000);
    return dis(gen);
}

/**
 * @brief: 生成EasyTier配置参数（配置文件启动 - 选择文件模式）
 * @param netPage: 网络配置页面指针
 * @param configFilePath: 配置文件路径
 * @return: 生成的EasyTier配置参数Qt字符串列表
 * @note: 该函数生成的是配置参数字符串，须在运行core时添加到命令行参数中
 */
QStringList generateConfFile(NetPage *netPage, const QString& configFilePath)
{
    QStringList conf;

    // 选择文件模式：直接使用所选文件
    conf << "-c" << configFilePath;

    // 处理 RPC 端口配置
    const int rpcPort = netPage->getRpcPort();
    const bool autoRpc = netPage->isAutoRpcEnabled();

    if (rpcPort != 0 && !autoRpc) {
        // 用户指定端口，检查是否被占用
        if (isPortOccupied(rpcPort)) {
            throw std::runtime_error(QString("RPC 端口 %1 已被占用").arg(rpcPort).toStdString());
        }
        netPage->realRpcPort = rpcPort;
        conf << "--rpc-portal" << QString::number(rpcPort);
    } else {
        // 自动分配端口
        for (int i = 0; i < 100; ++i) {
            int port = getRandomPort();
            if (!isPortOccupied(port)) {
                netPage->realRpcPort = port;
                conf << "--rpc-portal" << QString::number(port);
                break;
            }
        }
        if (netPage->realRpcPort == 0) {
            throw std::runtime_error("无法找到可用的 RPC 端口");
        }
    }

    return conf;
}

/**
 * @brief: 生成EasyTier配置参数（配置文件启动 - 下方输入模式）
 * @param netPage: 网络设置页面指针
 * @param configContent: 配置文件内容
 * @param tempConfigFilePath: 输出参数，临时配置文件路径
 * @return: 生成的EasyTier配置参数Qt字符串列表
 * @note: 该函数生成的是配置参数字符串，须在运行core时添加到命令行参数中
 */
QStringList generateConfFile(NetPage *netPage, const QString& configContent, QString& tempConfigFilePath)
{
    QStringList conf;

    // 下方输入模式：创建临时配置文件
    QString networkName = netPage->getNetworkName();
    QString base32Name = base32Encode(networkName.isEmpty() ? "default" : networkName.toUtf8());
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    tempConfigFilePath = QString("%1/QtEasyTier_%2.toml").arg(tempDir, base32Name);

    // 写入临时配置文件
    QFile tempFile(tempConfigFilePath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error(QString("无法创建临时配置文件: %1").arg(tempFile.errorString()).toStdString());
    }
    tempFile.write(configContent.toUtf8());
    tempFile.close();

    conf << "-c" << tempConfigFilePath;

    // 处理 RPC 端口配置
    const int rpcPort = netPage->getRpcPort();
    const bool autoRpc = netPage->isAutoRpcEnabled();

    if (rpcPort != 0 && !autoRpc) {
        // 用户指定端口，检查是否被占用
        if (isPortOccupied(rpcPort)) {
            throw std::runtime_error(QString("RPC 端口 %1 已被占用").arg(rpcPort).toStdString());
        }
        netPage->realRpcPort = rpcPort;
        conf << "--rpc-portal" << QString::number(rpcPort);
    } else {
        // 自动分配端口
        for (int i = 0; i < 100; ++i) {
            int port = getRandomPort();
            if (!isPortOccupied(port)) {
                netPage->realRpcPort = port;
                conf << "--rpc-portal" << QString::number(port);
                break;
            }
        }
        if (netPage->realRpcPort == 0) {
            throw std::runtime_error("无法找到可用的 RPC 端口");
        }
    }

    return conf;
}

/**
 * @brief: 生成EasyTier配置参数（Web管理启动）
 * @param netPage: 网络配置页面指针
 * @param webConnectAddr: Web控制台连接地址
 * @param connectToLocal: 是否连接到本地控制台
 * @param localConfigPort: 本地控制台配置端口（仅当 connectToLocal 为 true 时使用）
 * @param localConfigProtocol: 本地控制台配置协议（仅当 connectToLocal 为 true 时使用）
 * @return: 生成的EasyTier配置参数Qt字符串列表
 * @note: 该函数生成的是配置参数字符串，须在运行core时添加到命令行参数中
 */
QStringList generateConfWeb(NetPage *netPage, const QString& webConnectAddr,
                            bool connectToLocal, int localConfigPort,
                            const QString& localConfigProtocol)
{
    Q_UNUSED(netPage)  // Web 模式下暂不需要 netPage 的配置

    QStringList conf;

    // 构建 Web 控制台连接参数
    QString webArg;
    if (connectToLocal) {
        // 连接到本地控制台
        webArg = QString("%1://127.0.0.1:%2/admin").arg(localConfigProtocol).arg(localConfigPort);
    } else {
        // 使用自定义连接地址
        if (webConnectAddr.isEmpty()) {
            throw std::runtime_error("未指定 Web 控制台连接地址");
        }
        webArg = webConnectAddr.trimmed();
    }

    conf << "-w" << webArg;

    // Web 控制台管理模式不需要 CLI 监测，设置特殊的 RPC 端口值
    netPage->realRpcPort = NO_USE_CLI;

    return conf;
}

/** @brief: 生成EasyTier配置文件(常规启动)
 *  @param netPage: 网络配置页面指针
 *  @return: 生成的EasyTier配置参数Qt字符串
 *  @note: 该函数生成的是配置参数字符串，须在运行core时添加到命令行参数中
 */
QStringList generateConfCommand(NetPage *netPage)
{
    QStringList conf;

    // 先解决RPC端口号
    if (netPage->getRpcPort() != 0) {
        if (isPortOccupied(netPage->getRpcPort())) {
            throw std::runtime_error("RPC port is in use");
        }
        netPage->realRpcPort = netPage->getRpcPort();
        conf << "--rpc-portal" << QString::number(netPage->getRpcPort());

    } else {

        // 循环找到一个未被占用的端口
        for (int i = 10000; i < 50000; i++) {
            int port = getRandomPort();
            if (!isPortOccupied(port)) {
                conf << "--rpc-portal" << QString::number(port);
                netPage->realRpcPort = port;
                break;
            }
        }
        if (netPage->realRpcPort == 0) {
            throw std::runtime_error("could not find an available RPC port");
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
    if (netPage->isSystemForwardingEnabled()) conf << "--proxy-forward-by-system";      // 系统转发
    if (netPage->isSymmetricNatHolePunchingDisabled()) conf << "--disable-sym-hole-punching";  // 禁用对称NAT打洞
    if (netPage->isIpv6Disabled()) conf << "--disable-ipv6";             // 禁用IPv6

    if (netPage->isBindDevice()) {
        conf << "--bind-device" << "true";                // 仅物理网卡
    } else {
        conf << "--bind-device" << "false";                // 所有网卡
    }

    if (netPage->isRpcPacketForwardingEnabled()) conf << "--relay-all-peer-rpc";  // 转发RPC包
    if (netPage->isEncryptionDisabled()) conf << "--disable-encryption"; // 禁用加密
    if (netPage->isMagicDnsEnabled()) conf << "--accept-dns" << "true";    // 启用Magic DNS

    // DHCP和IP设置
    if (netPage->isDhcpEnabled()) {
        conf << "--dhcp" << "true";  // 启用DHCP
    } else {
        conf << "--dhcp" << "false";  // 禁用DHCP
        // IP地址设置
        if (!netPage->getIpAddress().isEmpty()) {
            conf << "--ipv4" << netPage->getIpAddress();  // IPv4地址
        } else {
            std::clog << "No IP address set. This node will only forward packets." << std::endl;
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

#ifdef Q_OS_WIN
    // Windows 平台：使用 IP Helper API 查询端口状态
    // 通过 GetTcpTable2 获取 TCP 连接表，检查端口是否被监听或占用
    
    PMIB_TCPTABLE2 pTcpTable = nullptr;
    ULONG size = 0;   // 缓冲区大小
    
    // 第一次调用获取所需缓冲区大小
    DWORD result = GetTcpTable2(nullptr, &size, TRUE);
    if (result != ERROR_INSUFFICIENT_BUFFER) {
        std::cerr << "[PortDetect] GetTcpTable2 failed to get size, error: " << result << std::endl;
        return true;
    }
    
    // 分配缓冲区
    pTcpTable = static_cast<PMIB_TCPTABLE2>(malloc(size));
    if (pTcpTable == nullptr) {
        std::cerr << "[PortDetect] Memory allocation failed" << std::endl;
        return true;
    }
    
    // 第二次调用获取实际数据
    result = GetTcpTable2(pTcpTable, &size, TRUE);
    if (result != NO_ERROR) {
        std::cerr << "[PortDetect] GetTcpTable2 failed, error: " << result << std::endl;
        free(pTcpTable);
        return true;
    }
    
    // 遍历 TCP 表检查端口是否被占用
    bool isOccupied = false;
    u_short targetPort = htons(static_cast<u_short>(port)); // 转换为网络字节序
    
    for (DWORD i = 0; i < pTcpTable->dwNumEntries; ++i) {
        MIB_TCPROW2* pRow = &pTcpTable->table[i];
        if (pRow->dwLocalPort == targetPort) {
            // 端口被占用
            isOccupied = true;
            std::clog << "[PortDetect] Port " << port << " is in use (state: " 
                      << pRow->dwState << ", PID: " << pRow->dwOwningPid << ")" << std::endl;
            break;
        }
    }
    
    free(pTcpTable);
    
    if (!isOccupied) {
        std::clog << "[PortDetect] Port " << port << " is available" << std::endl;
    }
    
    return isOccupied;

#elif defined(Q_OS_LINUX)
    // Linux 平台：使用 socket 系统 API 检测端口占用
    // 创建 TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "[PortDetect] Failed to create socket: " << strerror(errno) << std::endl;
        return true;
    }
    
    // 设置 SO_REUSEADDR 选项，允许端口重用
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 尝试绑定端口
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));
    
    bool isOccupied = false;
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errno == EADDRINUSE) {
            isOccupied = true;
            std::clog << "[PortDetect] Port " << port << " is in use (EADDRINUSE)" << std::endl;
        } else {
            std::cerr << "[PortDetect] bind() failed: " << strerror(errno) << std::endl;
            isOccupied = true;  // 其他错误也认为端口不可用
        }
    } else {
        std::clog << "[PortDetect] Port " << port << " is available" << std::endl;
    }
    
    // 关闭 socket
    close(sockfd);
    
    return isOccupied;

#else
    // 其他平台：使用 Qt 通用方式检测
    QTcpServer server;
    bool isOccupied = !server.listen(QHostAddress::LocalHost, port);
    if (!isOccupied) {
        server.close();
        std::clog << "[PortDetect] Port " << port << " is available" << std::endl;
    } else {
        std::clog << "[PortDetect] Port " << port << " is in use" << std::endl;
    }
    return isOccupied;
#endif
}

// =================== Base32 编码/解码相关函数 ===================

const QString BASE32_CHARS = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

/// @brief Base32编码
/// @param data: 需要编码的字节数组
QString base32Encode(const QByteArray& data) {
    QString result;
    int buffer = 0;
    int bitsLeft = 0;

    for (int i = 0; i < static_cast<int>(data.size()); ++i) {
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

    for (int i = 0; i < static_cast<int>(encoded.length()); ++i) {
        QChar ch = encoded[i];
        if (ch == '=') continue;

        int val = static_cast<int>(BASE32_CHARS.indexOf(ch.toUpper()));
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
    std::uniform_int_distribution<> dis(0, 66); // 字符集范围 0-66

    // 生成7字节的原始网络号数据（前2位固定为"QE"，后5位随机）
    // 对应Base32编码约12字符
    QByteArray networkIdRaw(7, 0);
    networkIdRaw[0] = 'Q';  // 固定前缀
    networkIdRaw[1] = 'E';  // 固定前缀
    for (int i = 2; i < 7; ++i) {
        networkIdRaw[i] = str[dis(gen)];
    }

    // 生成8字节的原始密码数据（对应Base32编码约13字符）
    QByteArray passwordRaw(8, 0);
    for (int i = 0; i < 8; ++i) {
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

    // 验证网络号前两位是否为"QE"
    if (networkIdData.size() < 2 || networkIdData[0] != 'Q' || networkIdData[1] != 'E') {
        return ""; // 返回空字符串表示错误
    }

    // 使用Base32编码
    QString encodedNetworkId = base32Encode(networkIdData);
    QString encodedPassword = base32Encode(passwordData);

    // 7字节数据Base32编码后为12字符，8字节数据编码后为13字符
    // 移除Base32填充字符'='
    encodedNetworkId.remove('=');
    encodedPassword.remove('=');

    if (encodedNetworkId.length() != 12 || encodedPassword.length() != 13) {
        return ""; // 返回空字符串表示错误
    }

    // 组合成格式：LFCV-GVLR-PKGSNW5YU-6CQNF-WYW
    QString code = encodedNetworkId.left(4) + "-" +           // 4字符
                   encodedNetworkId.mid(4, 4) + "-" +         // 4字符
                   encodedNetworkId.mid(8, 4) +               // 4字符（网络号共12字符）
                   encodedPassword.left(5) + "-" +            // 5字符（密码开始）
                   encodedPassword.mid(5, 5) + "-" +          // 5字符
                   encodedPassword.mid(10, 3);                // 3字符（密码共13字符）

    return code.toUpper();
}

QPair<QString, QString> decodeConnectionCode(const QString& code) {
    // 移除所有非Base32字符并转为大写
    QString cleanCode = code;
    const static QRegularExpression re("[^A-Za-z2-7]");
    cleanCode.remove(re);
    cleanCode = cleanCode.toUpper();

    // 检查长度是否正确（应该是25个Base32字符）
    if (cleanCode.length() != 25) {
        return qMakePair(QString(), QString()); // 返回空pair表示错误
    }

    // 分离网络号和密码的Base32编码
    // 网络号：前12字符，密码：后13字符
    QString encodedNetworkId = cleanCode.left(12);
    QString encodedPassword = cleanCode.mid(12, 13);

    // Base32解码
    QByteArray networkIdData = base32Decode(encodedNetworkId);
    QByteArray passwordData = base32Decode(encodedPassword);

    // 验证解码结果长度
    if (networkIdData.size() != 7 || passwordData.size() != 8) {
        return qMakePair(QString(), QString()); // 返回空pair表示错误
    }

    // 验证网络号前两位是否为"QE"
    if (networkIdData[0] != 'Q' || networkIdData[1] != 'E') {
        return qMakePair(QString(), QString()); // 返回空pair表示验证失败
    }

    // 转换为ASCII字符串
    QString networkPureId = QString::fromLatin1(networkIdData);
    QString password = QString::fromLatin1(passwordData);

    return qMakePair("QtET-OneClick-" + networkPureId, password);
}
