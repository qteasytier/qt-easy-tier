/**
 * @file ImportNodesViewModel.cpp
 * @brief ImportNodesViewModel 实现
 *
 * reload() 合并两个数据源（收藏节点 + 公共服务器节点）到一个统一列表中。
 * 支持通过 CheckedRole 编辑勾选状态，selectedNodes() 返回已勾选的节点列表。
 */
#include "ImportNodesViewModel.h"

#include "core/application/nodes/PublicServerProvider.h"
#include "core/viewmodel/FavoriteNodeViewModel.h"

ImportNodesViewModel::ImportNodesViewModel(FavoriteNodeViewModel *favoriteNodes,
                                           PublicServerProvider *publicServerProvider,
                                           QObject *parent)
    : QAbstractListModel(parent)
    , m_favoriteNodes(favoriteNodes)
    , m_publicServerProvider(publicServerProvider)
{
}

int ImportNodesViewModel::rowCount(const QModelIndex &parent) const
{
    // 标准扁平列表
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant ImportNodesViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const ImportNodeItem &item = m_items.at(index.row());
    switch (role) {
    case NameRole:
    case Qt::DisplayRole:
        return item.name;
    case UriRole:
        return item.uri;
    case PublicKeyRole:
        return item.publicKey;
    case CheckedRole:
        return item.checked;
    case SectionRole:
        return item.section;
    default:
        return {};
    }
}

bool ImportNodesViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // 仅允许修改 CheckedRole
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size() || role != CheckedRole)
        return false;

    ImportNodeItem &item = m_items[index.row()];
    const bool checked = value.toBool();
    // 值未变化时跳过信号发射
    if (item.checked == checked)
        return true;

    item.checked = checked;
    emit dataChanged(index, index, { CheckedRole });
    return true;
}

Qt::ItemFlags ImportNodesViewModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags = QAbstractListModel::flags(index);
    // 有效索引设置为可编辑，以支持 CheckedRole 的 CheckBox 交互
    if (index.isValid())
        itemFlags |= Qt::ItemIsEditable;
    return itemFlags;
}

QHash<int, QByteArray> ImportNodesViewModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { UriRole, "uri" },
        { PublicKeyRole, "publicKey" },
        { CheckedRole, "checked" },
        { SectionRole, "section" }
    };
}

void ImportNodesViewModel::reload()
{
    beginResetModel();
    m_items.clear();

    // 数据源 1：收藏节点
    if (m_favoriteNodes) {
        for (int row = 0; row < m_favoriteNodes->rowCount(); ++row) {
            const QModelIndex idx = m_favoriteNodes->index(row, 0);
            const QString uri = m_favoriteNodes->data(idx, FavoriteNodeViewModel::UriRole).toString();
            // 跳过无 URI 的空条目
            if (uri.isEmpty())
                continue;

            ImportNodeItem item;
            item.name = m_favoriteNodes->data(idx, FavoriteNodeViewModel::NameRole).toString();
            item.uri = uri;
            item.publicKey = m_favoriteNodes->data(idx, FavoriteNodeViewModel::PublicKeyRole).toString();
            item.section = QStringLiteral("收藏节点");
            m_items.append(item);
        }
    }

    // 数据源 2：公共服务器节点
    if (m_publicServerProvider) {
        const QVariantList publicServers = m_publicServerProvider->loadPublicServers();
        for (const QVariant &value : publicServers) {
            const QVariantMap server = value.toMap();
            const QString uri = server.value(QStringLiteral("uri")).toString();
            if (uri.isEmpty())
                continue;

            ImportNodeItem item;
            item.name = QStringLiteral("【公共节点】由%1提供")
                            .arg(server.value(QStringLiteral("contributor")).toString());
            item.uri = uri;
            item.publicKey = server.value(QStringLiteral("publicKey")).toString();
            item.section = QStringLiteral("公共节点");
            m_items.append(item);
        }
    }

    endResetModel();
    emit countChanged();
}

void ImportNodesViewModel::setChecked(int row, bool checked)
{
    // 代理到 setData，通过 CheckedRole 设置勾选状态
    setData(index(row, 0), checked, CheckedRole);
}

QVariantList ImportNodesViewModel::selectedNodes() const
{
    QVariantList selected;
    // 遍历所有条目，收集已勾选的节点
    for (const ImportNodeItem &item : m_items) {
        if (!item.checked)
            continue;

        QVariantMap node;
        node[QStringLiteral("uri")] = item.uri;
        node[QStringLiteral("publicKey")] = item.publicKey;
        selected.append(node);
    }
    return selected;
}

int ImportNodesViewModel::count() const
{
    return m_items.size();
}
