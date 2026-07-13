/**
 * @file StatusMonitor.cpp
 * @brief StatusMonitor 实现
 *
 * 实现运行中实例状态异步解析的核心逻辑：
 * - 构造函数：连接 onProcessRequested 信号到 QtConcurrent 异步解析
 * - processNetworkInfos：入口方法，由 VpnManager 心跳回调调用
 * - parseNodeInfos：解析本机和远端节点信息（IP、延迟、连接方式等）
 * - parseLogs：解析 Base64 编码的事件日志
 * - formatLogEntry：将各类事件格式化为中文可读日志
 */
#include "StatusMonitor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent/QtConcurrent>
#include <QMetaObject>
#include <QPointer>
#include <QDebug>
#include <algorithm>

// ==================== 构造 ====================

StatusMonitor::StatusMonitor(QObject *parent)
    : QObject(parent)
{
    // 连接内部信号：当 VpnManager 调用 processNetworkInfos 后，
    // 通过 emit onProcessRequested 触发后台线程异步解析
    connect(this, &StatusMonitor::onProcessRequested, this,
            [this](const QJsonArray &instances) {
                QPointer<StatusMonitor> self(this);
                (void)QtConcurrent::run([self, instances]() {
                    for (const QJsonValue &val : instances) {
                        if (!self) return;  // StatusMonitor 已析构，安全退出
                        const QJsonObject obj = val.toObject();
                        const QString key = obj[QStringLiteral("key")].toString();
                        const QString encoded = obj[QStringLiteral("value")].toString();

                        // 步骤 1：Base64 解码
                        const QByteArray decoded = QByteArray::fromBase64(
                            encoded.toUtf8());

                        // 步骤 2：从 JSON 字符串解析为 QJsonDocument
                        const QJsonDocument doc = QJsonDocument::fromJson(decoded);
                        if (!doc.isObject())
                            continue;

                        const QJsonObject jsonObj = doc.object();

                        // 步骤 3：解析节点信息和事件日志（静态方法，线程安全）
                        QVariantList nodeInfos = parseNodeInfos(jsonObj);
                        QVariantList logEntries = parseLogs(jsonObj);

                        // 步骤 4：通过 QueuedConnection 将结果抛回主线程
                        // 注意：内部 lambda 捕获 key/nodeInfos/logEntries 的副本
                        QMetaObject::invokeMethod(self,
                            [self, key, nodeInfos, logEntries]() {
                                if (!self) return;
                                emit self->instanceInfoParsed(key, nodeInfos, logEntries);
                            },
                            Qt::QueuedConnection);
                    }
                });
            });
}

// ==================== 入口方法 ====================

void StatusMonitor::processNetworkInfos(const QJsonObject &result)
{
    // 从 daemon collect_network_infos 响应中提取 instances 数组
    const QJsonArray instances = result[QStringLiteral("instances")].toArray();
    if (instances.isEmpty())
        return;

    // 发射内部信号，触发后台线程异步解析
    emit onProcessRequested(instances);
}

// ==================== IPv4 地址转换 ====================

QString StatusMonitor::addrToIpv4(quint32 addr)
{
    // 将 32 位网络字节序整数转为点分十进制（如 0xC0A80101 → 192.168.1.1）
    return QStringLiteral("%1.%2.%3.%4")
        .arg((addr >> 24) & 0xFF)
        .arg((addr >> 16) & 0xFF)
        .arg((addr >> 8) & 0xFF)
        .arg(addr & 0xFF);
}

// ==================== 节点信息解析 ====================

QVariantList StatusMonitor::parseNodeInfos(const QJsonObject &json)
{
    QVariantList result;

    // 从 JSON 中提取路由表、节点列表和本机信息
    QJsonArray routes = json[QStringLiteral("routes")].toArray();
    if (routes.isEmpty())
        return result;

    QJsonArray peers = json[QStringLiteral("peers")].toArray();
    QJsonObject myNodeInfo = json[QStringLiteral("my_node_info")].toObject();
    qint64 myPeerId = -1;
    if (myNodeInfo.contains(QStringLiteral("peer_id")))
        myPeerId = myNodeInfo[QStringLiteral("peer_id")].toVariant().toLongLong();

    // ============ 建立直接连接节点集合与连接信息 ============

    // 直接连接的节点集合
    QSet<qint64> directlyConnectedPeers;
    // 每个节点的连接协议列表
    QHash<qint64, QStringList> peerConnMap;
    // 每个节点的最小延迟（毫秒）
    QHash<qint64, int> peerLatencyMap;

    for (const QJsonValue &peerVal : peers) {
        QJsonObject peerObj = peerVal.toObject();
        qint64 peerId = peerObj[QStringLiteral("peer_id")].toVariant().toLongLong();
        directlyConnectedPeers.insert(peerId);

        // 收集该节点的连接信息和统计
        QJsonArray conns = peerObj[QStringLiteral("conns")].toArray();
        QStringList protocols;
        QSet<QString> addedProtocols;
        int minLatencyUs = INT_MAX;

        for (const QJsonValue &connVal : conns) {
            QJsonObject connObj = connVal.toObject();

            // 提取隧道类型作为协议标识
            QString tunnelType = connObj[QStringLiteral("tunnel")]
                .toObject()[QStringLiteral("tunnel_type")].toString();
            if (!tunnelType.isEmpty()) {
                QString proto = tunnelType.toUpper();
                if (!addedProtocols.contains(proto)) {
                    protocols.append(proto);
                    addedProtocols.insert(proto);
                }
            }

            // 提取延迟（取最小值）
            QJsonObject stats = connObj[QStringLiteral("stats")].toObject();
            int latencyUs = stats[QStringLiteral("latency_us")].toInt();
            if (latencyUs > 0 && latencyUs < minLatencyUs)
                minLatencyUs = latencyUs;
        }
        peerConnMap[peerId] = protocols;

        // 将微秒转换为毫秒（无效值记为 -1）
        int latencyMs = (minLatencyUs != INT_MAX && minLatencyUs > 0)
            ? minLatencyUs / 1000 : -1;
        peerLatencyMap[peerId] = latencyMs;
    }

    // ============ 本机节点 ============

    if (!myNodeInfo.isEmpty()) {
        QVariantMap local;
        local[QStringLiteral("isLocalNode")] = true;

        // 解析本机虚拟 IPv4 地址
        QJsonObject myIpv4 = myNodeInfo[QStringLiteral("virtual_ipv4")].toObject();
        quint32 myAddr = myIpv4[QStringLiteral("address")].toObject()
            [QStringLiteral("addr")].toVariant().toUInt();
        local[QStringLiteral("virtualIp")] = addrToIpv4(myAddr);
        local[QStringLiteral("hostname")] = myNodeInfo[QStringLiteral("hostname")].toString();
        local[QStringLiteral("latencyMs")] = 0;  // 本机延迟为 0
        local[QStringLiteral("connType")] = QStringLiteral("Local");
        local[QStringLiteral("connMethod")] = QStringLiteral("Local");

        // 从监听器列表提取协议类型
        QJsonArray listeners = myNodeInfo[QStringLiteral("listeners")].toArray();
        QStringList localProtocols;
        for (const QJsonValue &lv : listeners) {
            QString url = lv.toObject()[QStringLiteral("url")].toString();
            if (url.startsWith(QStringLiteral("tcp://"))
                || url.startsWith(QStringLiteral("udp://"))) {
                QString proto = url.section(QStringLiteral("://"), 0, 0).toUpper();
                if (!localProtocols.contains(proto))
                    localProtocols.append(proto);
            }
        }
        local[QStringLiteral("protocol")] = localProtocols.join(QStringLiteral(","));

        result.append(local);
    }

    // ============ 远端节点 ============

    for (const QJsonValue &routeVal : routes) {
        QJsonObject routeObj = routeVal.toObject();
        qint64 peerId = routeObj[QStringLiteral("peer_id")].toVariant().toLongLong();

        // 跳过本机节点（路由表包含自身）
        if (peerId == myPeerId)
            continue;

        QVariantMap info;
        info[QStringLiteral("isLocalNode")] = false;

        // 解析虚拟 IPv4 地址
        QJsonObject ipv4Addr = routeObj[QStringLiteral("ipv4_addr")].toObject();
        quint32 addr = ipv4Addr[QStringLiteral("address")].toObject()
            [QStringLiteral("addr")].toVariant().toUInt();
        info[QStringLiteral("virtualIp")] = addrToIpv4(addr);
        info[QStringLiteral("hostname")] = routeObj[QStringLiteral("hostname")].toString();

        bool isDirectlyConnected = directlyConnectedPeers.contains(peerId);

        // 延迟取值优先级：直接连接延迟 > 路由路径延迟
        if (isDirectlyConnected && peerLatencyMap[peerId] > 0) {
            info[QStringLiteral("latencyMs")] = peerLatencyMap[peerId];
        } else {
            info[QStringLiteral("latencyMs")] = routeObj[QStringLiteral("path_latency")].toInt();
        }

        // 根据节点类型确定连接方式和描述
        QJsonObject featureFlag = routeObj[QStringLiteral("feature_flag")].toObject();
        bool isPublicServer = featureFlag[QStringLiteral("is_public_server")].toBool();

        if (isPublicServer) {
            // 公共服务器节点
            info[QStringLiteral("connType")] = QStringLiteral("Server");
            if (isDirectlyConnected)
                info[QStringLiteral("protocol")] = peerConnMap[peerId].join(QStringLiteral(","));
            info[QStringLiteral("connMethod")] = QStringLiteral("Server");
        } else if (isDirectlyConnected) {
            // P2P 直连节点
            info[QStringLiteral("connType")] = QStringLiteral("P2P");
            info[QStringLiteral("protocol")] = peerConnMap[peerId].join(QStringLiteral(","));
            info[QStringLiteral("connMethod")] = QStringLiteral("P2P");
        } else {
            // 中继转发节点
            info[QStringLiteral("connType")] = QStringLiteral("Relay");
            info[QStringLiteral("connMethod")] = QStringLiteral("Relay");
        }

        result.append(info);
    }

    return result;
}

// ==================== 事件日志解析 ====================

QVariantList StatusMonitor::parseLogs(const QJsonObject &json)
{
    QVariantList result;

    QJsonArray events = json[QStringLiteral("events")].toArray();
    if (events.isEmpty())
        return result;

    // 遍历事件数组（daemon 返回顺序为最新在前）
    for (const QJsonValue &eventVal : events) {
        QString eventStr = eventVal.toString();
        if (eventStr.isEmpty())
            continue;

        // 事件字符串本身是一个 JSON 对象，需要二次解析
        QJsonParseError parseError;
        QJsonDocument eventDoc = QJsonDocument::fromJson(eventStr.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !eventDoc.isObject())
            continue;

        QJsonObject eventObj = eventDoc.object();
        QString time = eventObj[QStringLiteral("time")].toString();
        QJsonObject event = eventObj[QStringLiteral("event")].toObject();

        // 事件类型作为第一个 key
        QStringList keys = event.keys();
        if (keys.isEmpty())
            continue;

        QString eventType = keys.first();
        QJsonValue eventData = event[eventType];

        // 格式化为中文可读日志
        QString message = formatLogEntry(eventType, eventData);
        if (message.isEmpty())
            continue;

        // 格式化时间戳：2026-04-05T08:52:59 → 04-05 08:52:59
        QString formattedTime = time;
        int tIndex = time.indexOf('T');
        if (tIndex >= 0) {
            QString fullDate = time.left(tIndex);
            QStringList dateParts = fullDate.split('-');
            QString datePart = (dateParts.size() >= 3)
                ? dateParts[1] + QStringLiteral("-") + dateParts[2]
                : fullDate;
            QString timePart = (time.length() > tIndex + 9)
                ? time.mid(tIndex + 1, 8)
                : time.mid(tIndex + 1);
            formattedTime = datePart + QStringLiteral(" ") + timePart;
        }

        QVariantMap entry;
        entry[QStringLiteral("rawTimestamp")] = time;
        entry[QStringLiteral("timestamp")] = formattedTime;
        entry[QStringLiteral("message")] = message;
        result.append(entry);
    }

    // daemon 按最新在前返回，反转为最早在前（UI 按时间升序显示）
    std::reverse(result.begin(), result.end());
    return result;
}

// ==================== 日志消息格式化 ====================

QString StatusMonitor::formatLogEntry(const QString &eventType, const QJsonValue &eventData)
{
    // ===== TUN 设备 =====
    if (eventType == QStringLiteral("TunDeviceReady")) {
        return QStringLiteral("TUN 设备就绪: %1").arg(eventData.toString());
    }

    // ===== DHCP 变更 =====
    if (eventType == QStringLiteral("DhcpIpv4Changed")) {
        if (eventData.isArray()) {
            QJsonArray arr = eventData.toArray();
            QString oldIp = arr.size() > 0 ? arr.at(0).toString(QStringLiteral("无")) : QStringLiteral("无");
            QString newIp = arr.size() > 1 ? arr.at(1).toString() : QString();
            return QStringLiteral("DHCP IP 变更: %1 -> %2").arg(oldIp, newIp);
        }
    }

    // ===== 节点加入/离开 =====
    if (eventType == QStringLiteral("PeerAdded")) {
        return QStringLiteral("节点加入: ID=%1")
            .arg(eventData.toVariant().toLongLong());
    }
    if (eventType == QStringLiteral("PeerRemoved")) {
        return QStringLiteral("节点离开: ID=%1")
            .arg(eventData.toVariant().toLongLong());
    }

    // ===== 连接建立/断开 =====
    if (eventType == QStringLiteral("PeerConnAdded")) {
        QJsonObject conn = eventData.toObject();
        QString tunnel = conn[QStringLiteral("tunnel")].toObject()
            [QStringLiteral("tunnel_type")].toString().toUpper();
        qint64 pid = conn[QStringLiteral("peer_id")].toVariant().toLongLong();
        QString remoteAddr = conn[QStringLiteral("tunnel")].toObject()
            [QStringLiteral("remote_addr")].toObject()
            [QStringLiteral("url")].toString();
        return QStringLiteral("连接建立: [%1] ID=%2 %3")
            .arg(tunnel).arg(pid).arg(remoteAddr);
    }
    if (eventType == QStringLiteral("PeerConnRemoved")) {
        QJsonObject conn = eventData.toObject();
        QString tunnel = conn[QStringLiteral("tunnel")].toObject()
            [QStringLiteral("tunnel_type")].toString().toUpper();
        qint64 pid = conn[QStringLiteral("peer_id")].toVariant().toLongLong();
        return QStringLiteral("连接断开: [%1] ID=%2").arg(tunnel).arg(pid);
    }

    // ===== 连接状态 =====
    if (eventType == QStringLiteral("Connecting")) {
        return QStringLiteral("正在连接: %1").arg(eventData.toString());
    }
    if (eventType == QStringLiteral("ConnectError")) {
        if (eventData.isArray()) {
            QJsonArray arr = eventData.toArray();
            QString addr = arr.size() > 0 ? arr.at(0).toString() : QString();
            QString msg = arr.size() > 1 ? arr.at(1).toString() : QString();
            return QStringLiteral("连接错误: %1 - %2").arg(addr, msg);
        }
        return QStringLiteral("连接错误: %1").arg(eventData.toString());
    }
    if (eventType == QStringLiteral("ConnectionAccepted")) {
        if (eventData.isArray()) {
            QJsonArray arr = eventData.toArray();
            QString localAddr = arr.size() > 0 ? arr.at(0).toString() : QString();
            QString remoteAddr = arr.size() > 1 ? arr.at(1).toString() : QString();
            return QStringLiteral("连接接受: %1 <- %2").arg(localAddr, remoteAddr);
        }
    }

    // ===== 监听器 =====
    if (eventType == QStringLiteral("ListenerAdded")) {
        return QStringLiteral("监听器添加: %1").arg(eventData.toString());
    }
    if (eventType == QStringLiteral("ListenerRemoved")) {
        return QStringLiteral("监听器移除: %1").arg(eventData.toString());
    }

    // ===== 子网代理 =====
    if (eventType == QStringLiteral("ProxyCidrsUpdated")) {
        if (eventData.isArray()) {
            QJsonArray arr = eventData.toArray();
            QStringList added, removed;
            if (arr.size() > 0 && arr.at(0).isArray()) {
                for (const auto &v : arr.at(0).toArray())
                    added.append(v.toString());
            }
            if (arr.size() > 1 && arr.at(1).isArray()) {
                for (const auto &v : arr.at(1).toArray())
                    removed.append(v.toString());
            }
            if (!added.isEmpty())
                return QStringLiteral("子网代理添加: %1").arg(added.join(QStringLiteral(", ")));
            if (!removed.isEmpty())
                return QStringLiteral("子网代理移除: %1").arg(removed.join(QStringLiteral(", ")));
        }
        return QStringLiteral("子网代理更新");
    }

    // ===== 其他未识别事件（兜底：在消息中带上原始 JSON） =====
    QString dataStr;
    if (eventData.isObject()) {
        dataStr = QString::fromUtf8(
            QJsonDocument(eventData.toObject()).toJson(QJsonDocument::Compact));
    } else if (eventData.isArray()) {
        dataStr = QString::fromUtf8(
            QJsonDocument(eventData.toArray()).toJson(QJsonDocument::Compact));
    } else {
        dataStr = eventData.toVariant().toString();
    }
    return QStringLiteral("%1: %2").arg(eventType, dataStr);
}
