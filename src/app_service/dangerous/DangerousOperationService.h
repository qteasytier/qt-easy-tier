/**
 * @file DangerousOperationService.h
 * @brief 危险操作服务（应用服务层）
 *
 * 编排设置页「危险操作」相关的跨基础服务流程：
 * - 后端安装/卸载（DaemonRegisterHelper，UAC / pkexec 提权）
 * - 清空全部数据（VpnRuntimeService.stopAll → 各仓库清库 → 设置文件重置）
 *
 * 本服务是编排型应用服务：UI 层（DangerousOperationViewModel）只做薄壳转发，
 * 不直接接触 VpnRuntimeService / 仓库 / 平台工具。
 */
#pragma once

#include "app_service/settings/SettingsStore.h"

#include <QObject>
#include <QString>

class FavoriteNodeRepository;
class LogRepository;
class NetworkConfigRepository;
class VpnRuntimeService;

/** @brief 危险操作服务，编排后端安装/卸载与全量数据清空流程 */
class DangerousOperationService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param vpnRuntimeService  VPN 运行服务（停止所有网络服务）
     * @param configRepository    网络配置仓库（清空配置表）
     * @param favoriteRepository  收藏节点仓库（清空收藏表）
     * @param logRepository       日志仓库（清空日志表）
     * @param settingsFilePath    设置文件路径（空则使用默认路径，测试可注入临时路径）
     * @param parent              父对象
     */
    explicit DangerousOperationService(VpnRuntimeService *vpnRuntimeService,
                                       NetworkConfigRepository *configRepository,
                                       FavoriteNodeRepository *favoriteRepository,
                                       LogRepository *logRepository,
                                       const QString &settingsFilePath = QString(),
                                       QObject *parent = nullptr);

    /// 查询操作是否进行中
    bool busy() const;
    /// 查询后端是否已注册且运行中
    bool daemonInstalled() const;
    /// 查询后端操作是否可用
    bool daemonOperationEnabled() const;

    /**
     * @brief 刷新后端按钮状态（进入页面与每次操作完成后调用）
     *
     * 内部通过 DaemonRegisterHelper::requiredAction() 检测后端状态，
     * 结果缓存到成员变量并发射 daemonStatusChanged 供 QML 刷新。
     */
    void refreshDaemonStatus();

    /**
     * @brief 执行后端安装或卸载（按当前状态自动路由）
     *
     * - 后端已注册且运行中 → 停止并卸载后端服务
     * - 否则 → 安装（未注册）或仅启动（已注册未运行）后端服务
     *
     * 操作同步执行（UAC / pkexec 提权），完成后刷新状态并发射 operationFinished。
     */
    void performDaemonOperation();

    /**
     * @brief 清空全部数据（异步流程）
     *
     * 流程：
     * 1. 请求 VpnRuntimeService 停止所有正在运行的网络服务
     * 2. 全部成功收敛后清空磁盘数据（配置表/收藏表/日志表/设置文件）
     * 3. 清空成功 → 发射 quitRequested() 请求退出应用
     *
     * 任一步失败则中止并发射 operationFinished(false, message)。
     */
    void clearAllData();

signals:
    /// 操作进行中状态变化
    void busyChanged();
    /// 后端安装/卸载按钮状态变化
    void daemonStatusChanged();
    /// 操作完成通知（success 为 false 时 message 描述失败原因）
    void operationFinished(bool success, const QString &message);
    /// 清空数据成功后请求退出应用（由 AppServices 连接执行退出）
    void quitRequested();

private:
    /// 设置操作进行中状态并发射信号
    void setBusy(bool busy);
    /// stopAll 收敛回调：成功则执行清空，失败则中止
    void onAllStopped(bool success);
    /// 执行磁盘数据清空，成功后请求退出
    void performClear();

    VpnRuntimeService *m_vpnRuntimeService;      ///< VPN 运行服务（非所有权）
    NetworkConfigRepository *m_configRepository; ///< 网络配置仓库（非所有权）
    FavoriteNodeRepository *m_favoriteRepository; ///< 收藏节点仓库（非所有权）
    LogRepository *m_logRepository;           ///< 日志仓库（非所有权）
    SettingsStore m_store;                    ///< 全局设置存储（清空时写回默认值）
    bool m_busy = false;                      ///< 操作进行中标志
    bool m_daemonInstalled = false;           ///< 后端是否已注册且运行中（缓存）
    bool m_daemonOperationEnabled = false;    ///< 后端操作是否可用（缓存）
};
