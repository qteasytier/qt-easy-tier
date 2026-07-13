/**
 * @file BackendStatusViewModel.h
 * @brief 后端守护进程连接状态 ViewModel
 *
 * 监视 DaemonClient 的连接状态变化，暴露 connected / connecting / statusText
 * 三个 Q_PROPERTY 供 QML 绑定显示，实时反映后端守护进程的连接状况。
 */
#pragma once

#include <QObject>
#include <QString>

class DaemonClient;

/**
 * @brief 后端守护进程连接状态 ViewModel，将 DaemonClient 连接状态映射为 QML 可绑定的属性
 */
class BackendStatusViewModel : public QObject
{
    Q_OBJECT
    /// 是否已连接到后端守护进程
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged FINAL)
    /// 是否正在连接后端守护进程
    Q_PROPERTY(bool connecting READ connecting NOTIFY connectingChanged FINAL)
    /// 连接状态文本（"后端已连接" / "连接中..." / "后端未连接"）
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged FINAL)

public:
    /**
     * @brief 构造后端状态 ViewModel
     * @param client 守护进程客户端（非所有权）
     * @param parent 父 QObject
     */
    explicit BackendStatusViewModel(DaemonClient *client, QObject *parent = nullptr);

    /// 查询当前是否已连接
    bool connected() const;
    /// 查询当前是否正在连接中
    bool connecting() const;
    /// 获取连接状态的本地化文本
    QString statusText() const;

signals:
    /// 连接状态变化时发射
    void connectedChanged();
    /// 连接中状态变化时发射
    void connectingChanged();
    /// 状态文本变化时发射
    void statusTextChanged();

private:
    DaemonClient *m_client = nullptr; ///< 守护进程客户端（非所有权）
};
