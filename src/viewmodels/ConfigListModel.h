/**
 * @file ConfigListModel.h
 * @brief 网络配置列表模型（QAbstractListModel 子类）
 *
 * 作为 QML ListView / GridView 的数据源，提供网络配置的增删改查、
 * 导入导出、运行状态展示等功能。每个配置项通过 role 映射到 QML 属性。
 *
 * 模型不直接访问数据存储——所有数据读写均通过应用服务层
 * （ConfigCommandService / ConfigImportExportService）完成。
 * 与 VpnRuntimeService 平权解耦，通过信号/槽通信，互不持有对方指针。
 */
#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QSet>
#include <QStringList>
#include "core/config/ConfigRunState.h"
#include "core/config/NetworkConf.h"

class ConfigCommandService;
class ConfigImportExportService;

class ConfigListModel : public QAbstractListModel {
    Q_OBJECT

public:
    /// QML 可访问的数据角色枚举，Qt::DisplayRole 自动映射到 displayName
    enum Roles {
        InstanceNameRole = Qt::UserRole + 1, ///< 配置唯一标识名（内部 key）
        DisplayNameRole,                      ///< 用户可见的配置显示名称
        HostnameRole,                         ///< 目标主机名 / IP
        RunningRole,                          ///< 该配置当前是否正在运行
        RunStateRole,                         ///< 完整运行状态（ConfigRunState 枚举整数值）
        UpdatedAtRole,                        ///< 最后更新时间（预留，当前返回空）
        IsExternalRole                        ///< 是否为外部实例（daemon 中存在但本地配置列表中没有）
    };

    /**
     * @brief 构造配置列表模型
     * @param commandService 配置命令服务（读写、增删改，非空）
     * @param importExportService 配置导入导出服务（可为空，为空时导入导出功能不可用）
     * @param parent 父对象
     */
    explicit ConfigListModel(ConfigCommandService *commandService,
                             ConfigImportExportService *importExportService = nullptr,
                             QObject *parent = nullptr);

    // ---- QAbstractListModel 核心接口 ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ---- QML 可调用方法 ----

    /// 重新从仓库加载全部配置（会触发模型重置）
    Q_INVOKABLE void refresh();

    /// 创建新配置，自动生成唯一实例名和递增的显示名 "新配置 N"
    Q_INVOKABLE QString createNewConfig();

    /// 删除指定配置（未运行则立即删除；正在运行则先请求停止，待状态变为 Unstarted 后再删除）
    Q_INVOKABLE bool deleteConfig(const QString &instanceName);

    /// 重命名指定配置的显示名称
    Q_INVOKABLE bool renameConfig(const QString &instanceName, const QString &newDisplayName);

    /// 从 TOML 文件导入配置（异步：先本地校验，再远程 daemon 校验）
    Q_INVOKABLE void importConfigFile(const QString &filePath);

    /// 从 qtet:// URL 导入配置（异步）
    Q_INVOKABLE void importConfigUrl(const QString &url);

    /// 导出指定配置为 TOML 文件
    Q_INVOKABLE bool exportConfigFile(const QString &instanceName, const QString &filePath);

    /// 导出指定配置为 qtet:// URL 字符串，失败时返回空字符串并通过 errorOccurred 信号通知
    Q_INVOKABLE QString exportConfigUrl(const QString &instanceName);

signals:
    /// 操作失败时发射，QML 层展示错误提示
    void errorOccurred(const QString &message);
    /// 导入流程已启动（daemon 远程校验进行中）
    void importStarted();
    /// 导入成功（本地 + 远程校验均通过，已保存到仓库）
    void importSucceeded();
    /// 导入失败（本地校验失败 或 远程校验不通过）
    void importFailed(const QString &message);

    /// 请求停止指定配置（由外部连接方决定如何处理）
    void requestStopConfig(const QString &instanceName);
    /// 配置已删除，通知外部清理相关资源
    void configDeleted(const QString &instanceName);
    /// 本地配置已创建或导入成功，通知外部同步本地 controller（保持与数据库配置集合一致）
    void configCreated(const QString &instanceName);
    /// 配置重命名成功，通知协调方同步共享快照的显示名称（避免后续完整保存覆盖重命名）
    void configRenamed(const QString &instanceName, const QString &displayName);

public slots:
    /**
     * @brief 响应外部运行状态变更通知
     * @param instanceName 状态变化的配置实例名
     * @param state 新状态
     *
     * 更新 m_runningInstances 缓存并发射 dataChanged 信号
     * 更新对应行的 RunningRole，QML 绑定自动刷新
     */
    void onRunningStateChanged(const QString &instanceName, ConfigRunState state);

    /**
     * @brief 响应外部实例集合变更（VpnRuntimeService 心跳同步发现）
     * @param instanceNames 当前全部外部实例名列表（按 daemon 返回顺序）
     *
     * 外部实例指 daemon 中存在但本地配置列表中不存在的运行中实例，
     * 在配置列表末尾追加/移除对应条目。
     */
    void onExternalInstancesChanged(const QStringList &instanceNames);

    /// 查询指定实例名是否为外部实例（daemon 中存在但本地配置列表中没有）
    Q_INVOKABLE bool isExternal(const QString &instanceName) const;

    /// 查询指定实例名是否为本地配置（存在于仓库加载的配置列表中）
    Q_INVOKABLE bool isLocalInstance(const QString &instanceName) const;

    /// 查询指定实例的完整运行状态（外部实例固定 Running，未知实例返回 Stopped）
    ConfigRunState instanceState(const QString &instanceName) const;

private:
    /// 从数据库删除指定配置并通知外部
    bool performDelete(const QString &instanceName);

    ConfigCommandService *m_commandService; ///< 配置命令服务（非所有权）
    ConfigImportExportService *m_importExportService; ///< 配置导入导出服务（非所有权）
    QList<NetworkConf> m_configs;                ///< 内存中的完整配置列表缓存
    QHash<QString, ConfigRunState> m_instanceStates; ///< 实例名 → 运行状态缓存
    QSet<QString> m_pendingDeletion;              ///< 已请求停止、等待删除的实例名集合
    QStringList m_externalInstanceList;           ///< 外部实例名列表（按 daemon 返回顺序，展示在本地条目之后）
};
