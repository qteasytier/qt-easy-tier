/**
 * @file NodeInfoModel.cpp
 * @brief NodeInfoModel 实现
 *
 * 包含匿名命名空间中的工具函数，用于将原始连接类型和延迟毫秒数
 * 转换为 QML 显示所需的文本和等级标签。
 * setFromVariantList 将 VpnManager 传入的 QVariantList 反序列化为 NodeInfoItem 列表。
 */
#include "NodeInfoModel.h"

#include <QVariantMap>

namespace {
/**
 * @brief 将连接类型枚举映射为中文显示文本
 * @param connType 连接类型（"P2P" / "Relay" / "Server"）
 * @param connMethod 连接方式（作为回退显示值）
 * @return 中文连接类型文本
 */
QString connectionTypeText(const QString &connType, const QString &connMethod)
{
    if (connType == QStringLiteral("P2P"))
        return QStringLiteral("直连");
    if (connType == QStringLiteral("Relay"))
        return QStringLiteral("中转");
    if (connType == QStringLiteral("Server"))
        return QStringLiteral("服务器");
    return connMethod;
}

/**
 * @brief 将连接类型映射为 UI 等级标签（对应 Theme 中的颜色等级）
 * @param connType 连接类型
 * @return 等级字符串（"green" / "orange" / "blue" / "neutral"）
 */
QString connectionTypeLevel(const QString &connType)
{
    if (connType == QStringLiteral("P2P"))
        return QStringLiteral("green");
    if (connType == QStringLiteral("Relay"))
        return QStringLiteral("orange");
    if (connType == QStringLiteral("Server"))
        return QStringLiteral("blue");
    return QStringLiteral("neutral");
}

/**
 * @brief 将延迟毫秒数映射为 UI 等级标签
 * @param latencyMs 延迟（毫秒），<=0 视为未知
 * @return 等级字符串（"unknown" / "green" / "orange" / "red"）
 */
QString latencyLevel(int latencyMs)
{
    if (latencyMs <= 0)
        return QStringLiteral("unknown");
    if (latencyMs < 60)
        return QStringLiteral("green");
    if (latencyMs < 200)
        return QStringLiteral("orange");
    return QStringLiteral("red");
}
}

NodeInfoModel::NodeInfoModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int NodeInfoModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant NodeInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const auto &item = m_items.at(index.row());
    switch (role) {
    case VirtualIpRole: return item.virtualIp;
    case HostnameRole: return item.hostname;
    case IsLocalNodeRole: return item.isLocalNode;
    case ConnectionTypeTextRole: return item.connectionTypeText;
    case ConnectionTypeLevelRole: return item.connectionTypeLevel;
    case ConnectionTypeTextColorRole: return item.connectionTypeTextColor;
    case LatencyTextRole: return item.latencyText;
    case LatencyLevelRole: return item.latencyLevel;
    case ShowLatencyRole: return item.showLatency;
    case ProtocolRole: return item.protocol;
    default: return {};
    }
}

QHash<int, QByteArray> NodeInfoModel::roleNames() const
{
    return {
        {VirtualIpRole, "virtualIp"},
        {HostnameRole, "hostname"},
        {IsLocalNodeRole, "isLocalNode"},
        {ConnectionTypeTextRole, "connectionTypeText"},
        {ConnectionTypeLevelRole, "connectionTypeLevel"},
        {ConnectionTypeTextColorRole, "connectionTypeTextColor"},
        {LatencyTextRole, "latencyText"},
        {LatencyLevelRole, "latencyLevel"},
        {ShowLatencyRole, "showLatency"},
        {ProtocolRole, "protocol"},
    };
}

int NodeInfoModel::count() const
{
    return m_items.size();
}

bool NodeInfoModel::hideServerNodes() const
{
    return m_hideServerNodes;
}

void NodeInfoModel::setHideServerNodes(bool value)
{
    if (m_hideServerNodes == value)
        return;

    m_hideServerNodes = value;
    emit hideServerNodesChanged();
    rebuildVisibleItems();
}

void NodeInfoModel::setItems(const QList<NodeInfoItem> &items)
{
    m_sourceItems = items;
    rebuildVisibleItems();
}

void NodeInfoModel::rebuildVisibleItems()
{
    const int oldCount = m_items.size();
    QList<NodeInfoItem> visibleItems;
    visibleItems.reserve(m_sourceItems.size());
    for (const NodeInfoItem &item : m_sourceItems) {
        // 只在展示层隐藏公共服务器节点，保留 m_sourceItems 供关闭过滤后恢复。
        if (m_hideServerNodes && item.connectionType == QStringLiteral("Server"))
            continue;
        visibleItems.append(item);
    }

    // 全量替换 UI 可见模型数据
    beginResetModel();
    m_items = visibleItems;
    endResetModel();
    // 仅在数量变化时发射信号，避免无谓的 QML 属性刷新
    if (oldCount != m_items.size())
        emit countChanged();
}

void NodeInfoModel::setFromVariantList(const QVariantList &items)
{
    QList<NodeInfoItem> converted;
    converted.reserve(items.size());
    for (const QVariant &value : items) {
        const QVariantMap map = value.toMap();
        // 从 QVariantMap 中提取原始数据
        const QString connType = map.value(QStringLiteral("connType")).toString();
        const QString connMethod = map.value(QStringLiteral("connMethod")).toString();
        const int latencyMs = map.value(QStringLiteral("latencyMs"), -1).toInt();
        const bool isLocalNode = map.value(QStringLiteral("isLocalNode")).toBool();

        NodeInfoItem item;
        item.virtualIp = map.value(QStringLiteral("virtualIp")).toString();
        item.hostname = map.value(QStringLiteral("hostname")).toString();
        item.isLocalNode = isLocalNode;
        item.connectionType = connType;
        // 调用工具函数将连接类型和延迟转换为显示文本和等级
        item.connectionTypeText = connectionTypeText(connType, connMethod);
        item.connectionTypeLevel = connectionTypeLevel(connType);
        item.connectionTypeTextColor = item.connectionTypeLevel == QStringLiteral("neutral")
            ? QStringLiteral("normal") : QStringLiteral("onColor");
        item.latencyText = latencyMs >= 0
            ? QStringLiteral("%1 ms").arg(latencyMs)
            : QStringLiteral("未知延迟");
        item.latencyLevel = latencyLevel(latencyMs);
        // 本地节点且无有效延迟时不显示延迟信息
        item.showLatency = !isLocalNode || latencyMs > 0;
        item.protocol = map.value(QStringLiteral("protocol")).toString();
        converted.append(item);
    }
    setItems(converted);
}
