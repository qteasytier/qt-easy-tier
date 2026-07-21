/**
 * @file FavoriteNodeViewModel.h
 * @brief 收藏节点列表视图模型（QAbstractListModel）
 *
 * 管理 favorite_nodes 数据库表（通过 FavoriteNodeRepository）的 CRUD 操作，
 * 作为 QML ListView 的数据源，暴露节点名称、URI、公钥等数据给视图。
 */
#pragma once
#include <QAbstractListModel>
#include <QList>
#include <QSqlDatabase>
#include "core/favorite/FavoriteNode.h"

class FavoriteNodeImportExportService;
class FavoriteNodeRepository;
class QQmlEngine;
class QJSEngine;

/**
 * @brief 收藏节点列表 ViewModel，封装 FavoriteNodeRepository 的 CRUD 操作，作为 QML ListView 数据源
 */
class FavoriteNodeViewModel : public QAbstractListModel {
    Q_OBJECT

    /// 当前收藏节点数量（便于 QML 中显示"共 N 个"或控制空状态视图）
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    /// QML 可访问的数据角色枚举
    enum Roles {
        IdRole = Qt::UserRole + 1, ///< 节点主键 ID
        NameRole,                   ///< 用户自定义的节点名称
        UriRole,                    ///< 节点连接 URI
        PublicKeyRole               ///< 节点公钥
    };
    Q_ENUM(Roles)

    explicit FavoriteNodeViewModel(FavoriteNodeRepository *repository,
                                   FavoriteNodeImportExportService *importExportService,
                                   QObject *parent = nullptr);

    // ---- QAbstractListModel 核心接口 ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ---- QML 可调用方法 ----

    /// 从数据库重新加载全部收藏节点
    Q_INVOKABLE void loadNodes();

    /// 添加新收藏节点（自动检查 URI 去重）
    Q_INVOKABLE bool addNode(const QString &name, const QString &uri, const QString &publicKey);

    /// 更新指定节点的所有字段
    Q_INVOKABLE bool updateNode(qint64 id, const QString &name, const QString &uri, const QString &publicKey);

    /// 删除指定节点
    Q_INVOKABLE bool removeNode(qint64 id);

    /// 清空全部收藏节点
    Q_INVOKABLE bool clearAll();

    /// 从 JSON 文件批量导入收藏节点
    Q_INVOKABLE bool importNodesFromFile(const QString &fileUrl);

    /// 从 http/https URL 批量导入收藏节点
    Q_INVOKABLE void importNodesFromUrl(const QString &url);

    /// 将当前收藏节点批量导出为 JSON 文件
    Q_INVOKABLE bool exportNodesToFile(const QString &fileUrl);

    /// 检查 URI 是否已存在（编辑时可排除当前节点自身）
    Q_INVOKABLE bool uriExists(const QString &uri, qint64 excludeId);

    /// 获取当前节点数量
    int count() const;

signals:
    /// 操作失败时发射，QML 层展示错误提示
    void errorOccurred(const QString &message);
    /// 批量导入完成时发射，由应用装配层转发为系统托盘消息
    void importCompleted(int importedCount, int skippedCount);
    /// 批量导出完成时发射，由应用装配层转发为系统托盘消息
    void exportCompleted();
    /// 节点数量变化时发射（增/删/清空后）
    void countChanged();

private:
    void wireImportExportService();

    FavoriteNodeRepository *m_repo = nullptr; ///< 节点持久化仓库，非所有权
    FavoriteNodeImportExportService *m_importExportService = nullptr; ///< 批量导入导出服务，非所有权
    QList<FavoriteNode> m_nodes;       ///< 内存中的完整节点列表缓存
};
