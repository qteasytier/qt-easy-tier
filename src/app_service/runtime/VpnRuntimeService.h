/**
 * @file VpnRuntimeService.h
 * @brief VPN 运行服务（应用服务层）
 *
 * 桥接基础服务层（VpnManager 状态机）与 UI 层（QML / ViewModel）：
 * - 持有并暴露节点信息模型（NodeInfoModel）与运行时日志模型（RuntimeLogModel）
 * - 监听 VpnManager 的 instanceInfoUpdated / activeInstanceNameChanged 信号，
 *   将运行状态数据填充到展示模型
 * - 转发启停、状态查询、日志导出等操作给 VpnManager
 *
 * 分层约束：本服务是 UI 层访问 VPN 运行能力的唯一入口，
 * QML 与 ViewModel 不得直接接触 VpnManager。
 */
#pragma once

#include "app_service/runtime/NodeInfoModel.h"
#include "app_service/runtime/RuntimeLogModel.h"
#include "core/config/ConfigRunState.h"

#include <QObject>
#include <QString>
#include <QVariantList>

class VpnManager;

/** @brief VPN 运行服务，桥接 VpnManager 与 UI 层，负责运行状态展示数据的填充与转发 */
class VpnRuntimeService : public QObject {
    Q_OBJECT

    /// 当前正在查看的实例名称（运行状态页数据源指向）
    Q_PROPERTY(QString activeInstanceName READ activeInstanceName
               WRITE setActiveInstanceName NOTIFY activeInstanceNameChanged FINAL)

    /// 节点信息模型（当前查看实例的节点列表）
    Q_PROPERTY(NodeInfoModel *nodeInfoModel READ nodeInfoModel CONSTANT)
    /// 运行时日志模型（当前查看实例的事件日志）
    Q_PROPERTY(RuntimeLogModel *runtimeLogModel READ runtimeLogModel CONSTANT)

public:
    /**
     * @brief 构造函数
     * @param vpnManager VPN 管理器（非所有权，生命周期由装配层管理）
     * @param parent 父对象
     */
    explicit VpnRuntimeService(VpnManager *vpnManager, QObject *parent = nullptr);

    /// 请求启动指定配置
    void startConfig(const QString &instanceName);
    /// 请求停止指定配置
    void stopConfig(const QString &instanceName);
    /// 请求停止所有正在运行的实例（危险操作前置清理）
    void stopAll();

    /// 查询指定配置是否正在运行中
    bool isRunning(const QString &instanceName) const;
    /// 查询指定配置的当前状态（ConfigRunState 枚举整数值）
    int configState(const QString &instanceName) const;

    /// 导出当前选中实例的运行日志到本地文件（供 QML 直接调用，须为 Q_INVOKABLE）
    Q_INVOKABLE bool exportLog(const QString &filePath);

    /// 设置运行状态页是否隐藏公共服务器节点（作用于节点信息模型）
    void setHideServerNodes(bool value);

    // ---- QML 属性访问器 ----

    /// 获取当前选中的实例名
    QString activeInstanceName() const;
    /// 设置当前选中的实例名，并刷新节点与日志模型
    void setActiveInstanceName(const QString &name);
    /// 获取节点信息模型（当前选中实例的节点数据）
    NodeInfoModel *nodeInfoModel() const;
    /// 获取运行时日志模型（当前选中实例的事件日志）
    RuntimeLogModel *runtimeLogModel() const;

public slots:
    /// 清理指定配置的 controller 资源（由外部在删除配置后调用，转发给 VpnManager）
    void cleanupController(const QString &instanceName);

    /// 同步本地 controller（配置创建/导入成功后调用，转发给 VpnManager，保持与数据库配置集合一致）
    void ensureLocalController(const QString &instanceName);

signals:
    /// 通知 UI 层：某配置的状态已变更
    void configStateChanged(const QString &instanceName, ConfigRunState state);
    /// 通知 UI 层：某配置的停止操作失败（daemon 返回错误或超时）
    void stopFailed(const QString &instanceName, const QString &error);
    /// 通知 UI 层：stopAll() 已收敛完成，success 为 false 表示有实例停止失败或超时
    void allStopped(bool success);
    /// 通知 UI 层：当前选中实例名已变更
    void activeInstanceNameChanged();

private:
    /// 根据 VpnManager 中缓存的当前选中实例数据刷新两个展示模型
    void refreshModels();

    VpnManager *m_vpnManager = nullptr; ///< VPN 管理器（非所有权）
    NodeInfoModel *m_nodeInfoModel = nullptr; ///< 节点信息模型（本服务所有）
    RuntimeLogModel *m_runtimeLogModel = nullptr; ///< 运行时日志模型（本服务所有）
};
