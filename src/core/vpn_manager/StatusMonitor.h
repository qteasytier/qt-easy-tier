/**
 * @file StatusMonitor.h
 * @brief 运行中实例状态监视器
 *
 * 接收 daemon collect_network_infos 返回的原始 JSON，
 * 通过 QtConcurrent 在后台线程异步解码 Base64、解析节点信息与事件日志，
 * 解析完成后通过 signal 通知 VpnManager 缓存到对应 VpnController。
 *
 * ## 设计原则
 *
 * - 自身不持有 DaemonClient，由 VpnManager 在心跳回调中喂入数据
 * - 解析逻辑为纯函数（静态方法），天然线程安全，可在 QtConcurrent 线程中并行执行
 * - 解析完成后通过 QueuedConnection 将结果抛回主线程
 *
 * ## 线程安全
 *
 * - parseNodeInfos() / parseLogs() / formatLogEntry() 均为静态方法，无状态
 * - onProcessRequested 信号在后台线程处理 JSON 解析
 * - 解析结果通过 QMetaObject::invokeMethod + QueuedConnection 发射回主线程
 */
#pragma once
#include <QObject>
#include <QJsonArray>
#include <QVariantList>

/** @brief 运行中实例状态监视器，异步解码并解析 daemon 网络信息 */
class StatusMonitor : public QObject {
    Q_OBJECT

public:
    explicit StatusMonitor(QObject *parent = nullptr);

    /// 由 VpnManager 在 collect_network_infos 回调中调用
    /// 提取 JSON 中的 instances 数组，通过 onProcessRequested 信号触发异步解析
    void processNetworkInfos(const QJsonObject &result);

signals:
    /// 异步解码 + 解析完成后发射，通知 VpnManager 缓存到对应 VpnController
    /// @param instName   实例名称
    /// @param nodeInfos  解析后的节点信息列表（每个节点为 QVariantMap）
    /// @param logEntries 解析后的事件日志列表（每条为 {timestamp, message}）
    void instanceInfoParsed(const QString &instName,
                            const QVariantList &nodeInfos,
                            const QVariantList &logEntries);

    /// 内部信号：由 processNetworkInfos() 发射，触发异步解码 + 解析（QtConcurrent::run）
    /// instances 参数通过值传递（QJsonArray 隐式共享），跨线程安全
    void onProcessRequested(const QJsonArray &instances);

private:
    /// 将 32 位网络字节序整数转为点分十进制 IPv4 字符串（如 192.168.1.1）
    static QString addrToIpv4(quint32 addr);

    /// 从 daemon 返回的 JSON 中解析节点信息列表
    /// 包括本机节点和远端节点，提取虚拟 IP、主机名、延迟、连接方式等信息
    static QVariantList parseNodeInfos(const QJsonObject &json);

    /// 从 daemon 返回的 JSON 中解析事件日志列表
    /// 事件按时间正序排列（最早在前），每条包含格式化后的时间戳和消息
    static QVariantList parseLogs(const QJsonObject &json);

    /// 根据事件类型格式化单条日志消息（中文描述 + 关键参数）
    /// 支持的事件类型：TunDeviceReady, DhcpIpv4Changed, PeerAdded/Removed,
    ///                 PeerConnAdded/Removed, Connecting, ConnectError,
    ///                 ConnectionAccepted, ListenerAdded/Removed, ProxyCidrsUpdated
    static QString formatLogEntry(const QString &eventType, const QJsonValue &eventData);
};
