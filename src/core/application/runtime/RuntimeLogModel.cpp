/**
 * @file RuntimeLogModel.cpp
 * @brief RuntimeLogModel 实现
 *
 * setFromVariantList 将 VpnManager 传入的原始 QVariantList 反序列化为
 * RuntimeLogItem 列表，levelText 自动转为大写。
 * plainText 将所有日志格式化为换行分隔的纯文本，用于复制或导出。
 */
#include "RuntimeLogModel.h"

#include <QVariantMap>

RuntimeLogModel::RuntimeLogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RuntimeLogModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant RuntimeLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const auto &item = m_items.at(index.row());
    switch (role) {
    case TimestampRole: return item.timestamp;
    case RawTimestampRole: return item.rawTimestamp;
    case LevelRole: return item.level;
    case LevelTextRole: return item.levelText;
    case MessageRole: return item.message;
    // DisplayTextRole: 组合时间戳和消息为一行显示文本
    case DisplayTextRole: return QStringLiteral("[%1] %2").arg(item.timestamp, item.message);
    default: return {};
    }
}

QHash<int, QByteArray> RuntimeLogModel::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {RawTimestampRole, "rawTimestamp"},
        {LevelRole, "level"},
        {LevelTextRole, "levelText"},
        {MessageRole, "message"},
        {DisplayTextRole, "displayText"},
    };
}

int RuntimeLogModel::count() const
{
    return m_items.size();
}

QString RuntimeLogModel::plainText() const
{
    // 将所有日志格式化为 "[时间戳] 消息" 的纯文本，以换行符分隔
    QStringList lines;
    lines.reserve(m_items.size());
    for (const auto &item : m_items) {
        lines.append(QStringLiteral("[%1] %2").arg(item.timestamp, item.message));
    }
    return lines.join(QLatin1Char('\n'));
}

void RuntimeLogModel::setItems(const QList<RuntimeLogItem> &items)
{
    const int oldCount = m_items.size();
    // 全量替换模型数据
    beginResetModel();
    m_items = items;
    endResetModel();
    // 仅在数量变化时发射 countChanged，避免无谓刷新
    if (oldCount != m_items.size())
        emit countChanged();
    // 纯文本内容始终需要刷新（即使数量不变，内容也可能变化）
    emit plainTextChanged();
}

void RuntimeLogModel::setFromVariantList(const QVariantList &items)
{
    QList<RuntimeLogItem> converted;
    converted.reserve(items.size());
    for (const QVariant &value : items) {
        const QVariantMap map = value.toMap();
        RuntimeLogItem item;
        item.rawTimestamp = map.value(QStringLiteral("rawTimestamp")).toString();
        item.timestamp = map.value(QStringLiteral("timestamp")).toString();
        item.level = map.value(QStringLiteral("level")).toString();
        // levelText: 有 level 值时转为大写，否则保持 level 原值
        item.levelText = item.level.isEmpty() ? item.level : item.level.toUpper();
        item.message = map.value(QStringLiteral("message")).toString();
        converted.append(item);
    }
    setItems(converted);
}
