/**
 * @file NodeInfoModel.h
 * @brief 在线节点信息列表 Model（QAbstractListModel 子类）
 *
 * 展示 VPN 网络中在线节点的详细信息，包括虚拟 IP、主机名、连接类型（直连/中转）、
 * 延迟数据、传输协议等。数据由外部 VpnManager 通过 setItems/setFromVariantList 注入。
 */
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QVariantList>

/**
 * @brief 节点信息条目，描述一个在线节点的各项属性
 */
struct NodeInfoItem {
    QString virtualIp;           ///< 虚拟 IP 地址
    QString hostname;            ///< 主机名
    bool isLocalNode = false;    ///< 是否为本机节点
    QString connectionType;      ///< 原始连接类型（P2P / Relay / Server / Local），用于非展示逻辑
    QString connectionTypeText;  ///< 连接类型文本（"直连" / "中转" / "服务器"）
    QString connectionTypeLevel; ///< 连接类型等级（"green" / "orange" / "blue" / "neutral"）
    QString connectionTypeTextColor; ///< 连接类型文本颜色（"normal" / "onColor"）
    QString latencyText;         ///< 延迟文本（如 "35 ms"）
    QString latencyLevel;        ///< 延迟等级（"green" / "orange" / "red" / "unknown"）
    bool showLatency = true;     ///< 是否显示延迟信息（本地节点无延迟时不显示）
    QString protocol;            ///< 传输协议
};

/**
 * @brief 在线节点信息列表 Model，供 QML 展示 VPN 网络中的节点详情
 */
class NodeInfoModel : public QAbstractListModel
{
    Q_OBJECT
    /// 当前在线节点数量
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    /// 是否隐藏公共服务器节点，仅影响 UI 可见行，不清空原始节点缓存
    Q_PROPERTY(bool hideServerNodes READ hideServerNodes WRITE setHideServerNodes NOTIFY hideServerNodesChanged FINAL)

public:
    /// QML 可访问的数据角色枚举
    enum Roles {
        VirtualIpRole = Qt::UserRole + 1,      ///< 虚拟 IP
        HostnameRole,                           ///< 主机名
        IsLocalNodeRole,                        ///< 是否本机节点
        ConnectionTypeTextRole,                 ///< 连接类型文本
        ConnectionTypeLevelRole,                ///< 连接类型等级
        ConnectionTypeTextColorRole,            ///< 连接类型文本颜色
        LatencyTextRole,                        ///< 延迟文本
        LatencyLevelRole,                       ///< 延迟等级
        ShowLatencyRole,                        ///< 是否显示延迟
        ProtocolRole,                           ///< 传输协议
    };
    Q_ENUM(Roles)

    explicit NodeInfoModel(QObject *parent = nullptr);

    // ---- QAbstractListModel 核心接口 ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const;
    bool hideServerNodes() const;
    void setHideServerNodes(bool value);

    /// 直接设置节点列表（从结构体列表）
    void setItems(const QList<NodeInfoItem> &items);
    /// 从 QVariantList 反序列化并设置节点列表（由 VpnManager 传入）
    void setFromVariantList(const QVariantList &items);

signals:
    /// 节点数量变化时发射
    void countChanged();
    /// 服务节点过滤开关变化时发射
    void hideServerNodesChanged();

private:
    void rebuildVisibleItems();

    QList<NodeInfoItem> m_sourceItems; ///< daemon 返回的完整节点信息缓存
    QList<NodeInfoItem> m_items;       ///< 根据设置过滤后的 UI 可见节点
    bool m_hideServerNodes = false;
};
