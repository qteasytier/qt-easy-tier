/**
 * @file DangerousOperationViewModel.h
 * @brief 危险操作 ViewModel（设置页，薄壳）
 *
 * 为设置页「危险操作」卡片提供 QML 入口，全部业务逻辑委托应用服务层
 * DangerousOperationService 执行，本类只做 QML 友好的属性/方法/信号转发。
 */
#pragma once

#include <QObject>
#include <QString>

class DangerousOperationService;

/** @brief 危险操作 ViewModel，供 QML 调用后端安装/卸载与全量数据清空（薄壳转发） */
class DangerousOperationViewModel : public QObject {
    Q_OBJECT

    /// 操作是否进行中（进行中时禁用全部按钮）
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    /// 后端是否已注册且运行中（true 时按钮显示"卸载后端"，否则显示"安装后端"）
    Q_PROPERTY(bool daemonInstalled READ daemonInstalled NOTIFY daemonStatusChanged FINAL)
    /// 后端操作是否可用（daemon 二进制存在且平台支持）
    Q_PROPERTY(bool daemonOperationEnabled READ daemonOperationEnabled NOTIFY daemonStatusChanged FINAL)

public:
    /**
     * @brief 构造危险操作 ViewModel
     * @param service 危险操作服务（应用服务层，非所有权）
     * @param parent  父对象
     */
    explicit DangerousOperationViewModel(DangerousOperationService *service, QObject *parent = nullptr);

    /// 查询操作是否进行中
    bool busy() const;
    /// 查询后端是否已注册且运行中
    bool daemonInstalled() const;
    /// 查询后端操作是否可用
    bool daemonOperationEnabled() const;

    /// 刷新后端按钮状态（委托危险操作服务）
    Q_INVOKABLE void refreshDaemonStatus();
    /// 执行后端安装或卸载（委托危险操作服务）
    Q_INVOKABLE void performDaemonOperation();
    /// 清空全部数据（异步流程，委托危险操作服务）
    Q_INVOKABLE void clearAllData();

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
    DangerousOperationService *m_service = nullptr; ///< 危险操作服务（非所有权）
};
