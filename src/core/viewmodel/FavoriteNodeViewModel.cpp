/**
 * @file FavoriteNodeViewModel.cpp
 * @brief FavoriteNodeViewModel 实现
 */
#include "FavoriteNodeViewModel.h"
#include "core/application/favorite/FavoriteNodeImportExportService.h"
#include "core/repository/FavoriteNodeRepository.h"

#include <QUrl>

FavoriteNodeViewModel::FavoriteNodeViewModel(FavoriteNodeRepository *repository,
                                             FavoriteNodeImportExportService *importExportService,
                                             QObject *parent)
    : QAbstractListModel(parent)
    , m_repo(repository)
    , m_importExportService(importExportService)
{
    wireImportExportService();
    loadNodes();
}

int FavoriteNodeViewModel::rowCount(const QModelIndex &parent) const
{
    // 标准扁平 ListModel 模式：顶层项数 = 列表大小
    if (parent.isValid()) return 0;
    return m_nodes.size();
}

QVariant FavoriteNodeViewModel::data(const QModelIndex &index, int role) const
{
    // 越界检查：索引无效或超出范围时返回空值
    if (!index.isValid() || index.row() >= m_nodes.size())
        return {};

    const auto &node = m_nodes.at(index.row());

    // 按角色分发数据
    switch (role) {
    case IdRole:        return node.id;
    case NameRole:      return node.name;
    case UriRole:       return node.uri;
    case PublicKeyRole: return node.publicKey;
    case Qt::DisplayRole: return node.name; // 默认显示为节点名称
    default: return {};
    }
}

QHash<int, QByteArray> FavoriteNodeViewModel::roleNames() const
{
    // 将 C++ 角色枚举映射到 QML 属性名
    // 例如：在 QML 中通过 model.nodeName / model.nodeUri / model.nodePublicKey 访问
    return {
        { IdRole,        "nodeId" },
        { NameRole,      "nodeName" },
        { UriRole,       "nodeUri" },
        { PublicKeyRole, "nodePublicKey" }
    };
}

void FavoriteNodeViewModel::loadNodes()
{
    if (!m_repo)
        return;

    // 完整的模型重置加载
    beginResetModel();
    m_nodes = m_repo->loadAll();
    endResetModel();
    emit countChanged();
}

bool FavoriteNodeViewModel::addNode(const QString &name, const QString &uri, const QString &publicKey)
{
    if (!m_repo) {
        emit errorOccurred(QStringLiteral("收藏节点仓库不可用"));
        return false;
    }

    // 步骤 1：输入校验（名称和地址都不能为空）
    if (name.trimmed().isEmpty() || uri.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("节点名称和地址不能为空"));
        return false;
    }

    // 步骤 2：URI 去重检查
    if (m_repo->existsByUri(uri.trimmed())) {
        emit errorOccurred(QStringLiteral("节点地址已存在"));
        return false;
    }

    // 步骤 3：写入数据库
    auto result = m_repo->add(name, uri, publicKey);
    if (!result.has_value()) {
        emit errorOccurred(QStringLiteral("添加节点失败"));
        return false;
    }

    // 步骤 4：在模型头部插入新行（与 loadNodes 的最新在前排序一致）
    beginInsertRows(QModelIndex(), 0, 0);
    m_nodes.prepend(result.value());
    endInsertRows();
    emit countChanged();
    return true;
}

bool FavoriteNodeViewModel::updateNode(qint64 id, const QString &name, const QString &uri, const QString &publicKey)
{
    if (!m_repo) {
        emit errorOccurred(QStringLiteral("收藏节点仓库不可用"));
        return false;
    }

    // 步骤 1：空值校验
    if (name.trimmed().isEmpty() || uri.trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("节点名称和地址不能为空"));
        return false;
    }

    // 步骤 2：写回数据库
    if (!m_repo->update(id, name, uri, publicKey)) {
        emit errorOccurred(QStringLiteral("更新节点失败"));
        return false;
    }

    // 步骤 3：在内存列表中查找并更新对应行
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == id) {
            // 更新内存中的缓存数据
            m_nodes[i].name = name.trimmed();
            m_nodes[i].uri = uri.trimmed();
            m_nodes[i].publicKey = publicKey.trimmed();

            // 通知视图该行所有角色均已变化
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx); // 空角色列表 = 所有角色
            return true;
        }
    }
    return false;
}

bool FavoriteNodeViewModel::removeNode(qint64 id)
{
    if (!m_repo) {
        emit errorOccurred(QStringLiteral("收藏节点仓库不可用"));
        return false;
    }

    // 在内存列表中查找目标节点
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == id) {
            // 步骤 1：从数据库删除
            if (!m_repo->remove(id)) {
                emit errorOccurred(QStringLiteral("删除节点失败"));
                return false;
            }

            // 步骤 2：从模型中移除对应行（增量删除）
            beginRemoveRows(QModelIndex(), i, i);
            m_nodes.removeAt(i);
            endRemoveRows();
            emit countChanged();
            return true;
        }
    }
    // 未找到指定节点
    return false;
}

bool FavoriteNodeViewModel::clearAll()
{
    if (!m_repo) {
        emit errorOccurred(QStringLiteral("收藏节点仓库不可用"));
        return false;
    }

    // 步骤 1：清空数据库中的全部记录
    if (!m_repo->clear()) {
        emit errorOccurred(QStringLiteral("清空节点列表失败"));
        return false;
    }

    // 步骤 2：清空内存列表并通知模型重置
    beginResetModel();
    m_nodes.clear();
    endResetModel();
    emit countChanged();
    return true;
}

bool FavoriteNodeViewModel::importNodesFromFile(const QString &fileUrl)
{
    if (!m_importExportService) {
        emit errorOccurred(QStringLiteral("收藏节点导入导出服务不可用"));
        return false;
    }
    return m_importExportService->importFromFile(QUrl(fileUrl));
}

void FavoriteNodeViewModel::importNodesFromUrl(const QString &url)
{
    if (!m_importExportService) {
        emit errorOccurred(QStringLiteral("收藏节点导入导出服务不可用"));
        return;
    }
    m_importExportService->importFromUrl(QUrl(url.trimmed()));
}

bool FavoriteNodeViewModel::exportNodesToFile(const QString &fileUrl)
{
    if (!m_importExportService) {
        emit errorOccurred(QStringLiteral("收藏节点导入导出服务不可用"));
        return false;
    }
    return m_importExportService->exportToFile(QUrl(fileUrl), m_nodes);
}

bool FavoriteNodeViewModel::uriExists(const QString &uri, qint64 excludeId)
{
    if (!m_repo)
        return false;

    // 代理到 Repository 层查询 URI 是否已存在
    // excludeId 用于编辑场景：检查时排除当前节点自身
    return m_repo->existsByUri(uri.trimmed(), excludeId);
}

int FavoriteNodeViewModel::count() const
{
    return m_nodes.size();
}

void FavoriteNodeViewModel::wireImportExportService()
{
    if (!m_importExportService)
        return;

    connect(m_importExportService, &FavoriteNodeImportExportService::operationFailed,
            this, &FavoriteNodeViewModel::errorOccurred);
    connect(m_importExportService, &FavoriteNodeImportExportService::importCompleted,
            this, [this](int importedCount, int skippedCount) {
                loadNodes();
                emit importCompleted(importedCount, skippedCount);
            });
    connect(m_importExportService, &FavoriteNodeImportExportService::exportCompleted,
            this, &FavoriteNodeViewModel::exportCompleted);
}
