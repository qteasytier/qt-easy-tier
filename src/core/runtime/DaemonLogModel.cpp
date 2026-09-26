/**
 * @file DaemonLogModel.cpp
 * @brief DaemonLogModel 实现
 *
 * setFromVariantList 将 VpnRuntimeService 传入的原始 QVariantList 反序列化为
 * DaemonLogItem 列表（内容为 qtet-daemon 返回的运行实例事件日志）。
 * plainText 将所有日志格式化为换行分隔的纯文本，用于复制或导出。
 */
#include "DaemonLogModel.h"

#include <QVariantMap>

DaemonLogModel::DaemonLogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int DaemonLogModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant DaemonLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const auto &item = m_items.at(index.row());
    switch (role) {
    case TimestampRole: return item.timestamp;
    case RawTimestampRole: return item.rawTimestamp;
    case MessageRole: return item.message;
    // DisplayTextRole: 组合时间戳和消息为一行显示文本
    case DisplayTextRole: return QStringLiteral("[%1] %2").arg(item.timestamp, item.message);
    default: return {};
    }
}

QHash<int, QByteArray> DaemonLogModel::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {RawTimestampRole, "rawTimestamp"},
        {MessageRole, "message"},
        {DisplayTextRole, "displayText"},
    };
}

int DaemonLogModel::count() const
{
    return m_items.size();
}

QString DaemonLogModel::plainText() const
{
    // 纯文本拼接代价随日志条数增长，这里做缓存：只有数据变化时才重新拼接，
    // 避免 QML 每次求值绑定都重建一次整段字符串
    if (!m_plainTextDirty)
        return m_plainText;

    // 将所有日志格式化为 "[时间戳] 消息" 的纯文本，以换行符分隔
    QStringList lines;
    lines.reserve(m_items.size());
    for (const auto &item : m_items) {
        lines.append(QStringLiteral("[%1] %2").arg(item.timestamp, item.message));
    }
    m_plainText = lines.join(QLatin1Char('\n'));
    m_plainTextDirty = false;
    return m_plainText;
}

void DaemonLogModel::setItems(const QList<DaemonLogItem> &items)
{
    // 数据与当前内容完全一致时不做任何通知：运行状态页每 3 秒心跳都会重新注入一次，
    // 无变化还整表重置会让 QML 反复销毁/重建列表委托，白白制造分配与 GC 压力。
    if (m_items == items)
        return;

    const int oldCount = m_items.size();
    // 全量替换模型数据
    beginResetModel();
    m_items = items;
    endResetModel();
    m_plainTextDirty = true;
    // 仅在数量变化时发射 countChanged，避免无谓刷新
    if (oldCount != m_items.size())
        emit countChanged();
    // 纯文本内容始终需要刷新（即使数量不变，内容也可能变化）
    emit plainTextChanged();
}

void DaemonLogModel::setFromVariantList(const QVariantList &items)
{
    QList<DaemonLogItem> converted;
    converted.reserve(items.size());
    for (const QVariant &value : items) {
        const QVariantMap map = value.toMap();
        DaemonLogItem item;
        item.rawTimestamp = map.value(QStringLiteral("rawTimestamp")).toString();
        item.timestamp = map.value(QStringLiteral("timestamp")).toString();
        item.message = map.value(QStringLiteral("message")).toString();
        converted.append(item);
    }
    setItems(converted);
}
