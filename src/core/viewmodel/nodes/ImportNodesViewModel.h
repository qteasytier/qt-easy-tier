/**
 * @file ImportNodesViewModel.h
 * @brief 导入节点列表 ViewModel（QAbstractListModel 子类）
 *
 * 合并展示"收藏节点"和"公共服务器节点"，供用户在导入节点页面勾选并批量导入。
 * 支持多选勾选（CheckedRole）和分区显示（SectionRole："收藏节点" / "公共节点"）。
 */
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QPointer>
#include <QVariantList>

class FavoriteNodeViewModel;
class PublicServerProvider;

/**
 * @brief 导入节点列表 ViewModel，合并收藏节点和公共服务器节点，支持多选批量导入
 */
class ImportNodesViewModel : public QAbstractListModel {
    Q_OBJECT
    /// 当前可选节点总数
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    /// QML 可访问的数据角色枚举
    enum Roles {
        NameRole = Qt::UserRole + 1, ///< 节点显示名称
        UriRole,                      ///< 节点连接 URI
        PublicKeyRole,                ///< 节点公钥
        CheckedRole,                  ///< 是否已勾选（多选状态）
        SectionRole                   ///< 分组标签（"收藏节点" / "公共节点"）
    };
    Q_ENUM(Roles)

    /**
     * @brief 构造导入节点 ViewModel
     * @param favoriteNodes 收藏节点 ViewModel（用于读取已收藏节点列表）
     * @param publicServerProvider 公共服务器提供者（用于读取公共节点列表）
     * @param parent 父 QObject
     */
    explicit ImportNodesViewModel(FavoriteNodeViewModel *favoriteNodes,
                                  PublicServerProvider *publicServerProvider,
                                  QObject *parent = nullptr);

    // ---- QAbstractListModel 核心接口 ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// 重新加载所有可选节点（收藏 + 公共）
    Q_INVOKABLE void reload();
    /// 设置指定行的勾选状态
    Q_INVOKABLE void setChecked(int row, bool checked);
    /// 获取当前所有已勾选节点的 URI 和公钥列表
    Q_INVOKABLE QVariantList selectedNodes() const;

    /// 获取当前节点总数
    int count() const;

signals:
    /// 节点数量变化时发射
    void countChanged();

private:
    /// 内部数据结构：表示一个可导入的节点条目
    struct ImportNodeItem {
        QString name;       ///< 节点名称
        QString uri;        ///< 节点 URI
        QString publicKey;  ///< 节点公钥
        bool checked = false; ///< 是否已勾选
        QString section;    ///< 分组标签
    };

    QPointer<FavoriteNodeViewModel> m_favoriteNodes;          ///< 收藏节点数据源
    QPointer<PublicServerProvider> m_publicServerProvider;    ///< 公共服务器数据源
    QList<ImportNodeItem> m_items;                            ///< 合并后的节点条目缓存
};
