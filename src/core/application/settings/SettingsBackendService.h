/**
 * @file SettingsBackendService.h
 * @brief 设置后端服务（应用服务层）
 *
 * 桥接基础服务层与 UI 层，封装设置页中两类"后端交互"能力：
 * - daemon 自动回连开关（get_auto_reconnect / set_auto_reconnect RPC）
 * - 版本更新检查（UpdateCheckService）
 *
 * 异步请求的忙状态与最新值统一由本服务持有，UI 层（SettingsViewModel）
 * 只做属性转发，不直接接触 DaemonApi / UpdateCheckService。
 */
#pragma once

#include <QObject>
#include <QString>

class DaemonApi;
class UpdateCheckService;

/** @brief 设置后端服务，封装 daemon 自动回连与版本更新检查的异步交互 */
class SettingsBackendService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param daemonApi daemon API（非所有权，可为空）
     * @param updateCheckService 版本更新检查服务（非所有权，可为空）
     * @param parent 父对象
     */
    explicit SettingsBackendService(DaemonApi *daemonApi = nullptr,
                                    UpdateCheckService *updateCheckService = nullptr,
                                    QObject *parent = nullptr);

    /// 当前自动回连开关状态
    bool autoReconnect() const;
    /// 自动回连请求是否进行中
    bool autoReconnectBusy() const;
    /// 更新检查请求是否进行中
    bool updateCheckBusy() const;

    /** @brief 从后端查询当前自动回连状态（异步），成功后发射 autoReconnectChanged */
    void refreshAutoReconnect();
    /** @brief 向后端设置自动回连开关（异步），成功后发射 autoReconnectChanged */
    void setAutoReconnect(bool enabled);
    /**
     * @brief 触发一次版本更新检查（异步）
     * @param frontendVersion 当前前端版本号（用于与远端最新版本比较）
     * @param manual 是否为用户手动触发（true 时失败/无更新会提示用户）
     */
    void checkForUpdates(const QString &frontendVersion, bool manual);

signals:
    /// 自动回连开关状态变化
    void autoReconnectChanged();
    /// 自动回连请求进行中状态变化
    void autoReconnectBusyChanged();
    /// 自动回连操作失败（参数为错误消息）
    void autoReconnectOperationFailed(const QString &message);
    /// 更新检查请求进行中状态变化
    void updateCheckBusyChanged();

private:
    void setAutoReconnectBusy(bool busy);
    void setUpdateCheckBusy(bool busy);

    DaemonApi *m_daemonApi = nullptr; ///< daemon API（非所有权）
    UpdateCheckService *m_updateCheckService = nullptr; ///< 更新检查服务（非所有权）
    bool m_autoReconnect = false;    ///< 自动回连开关状态缓存
    bool m_autoReconnectBusy = false; ///< 自动回连请求进行中
    bool m_updateCheckBusy = false;  ///< 更新检查进行中
};
