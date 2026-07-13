/**
 * @file DaemonClient.h
 * @brief daemon IPC 客户端
 *
 * 与后台守护进程 qtet-daemon 通信的 IPC 客户端。
 * 通过 QLocalSocket 连接到 Unix Domain Socket，实现 JSON-RPC 风格的请求-响应匹配。
 *
 * 核心功能：
 *   - 三态连接状态机（Disconnected / Connecting / Connected）
 *   - 断线自动重连（失败后延时 2s 重试）
 *   - connectToDaemon / disconnectFromDaemon 手动控制连接
 *   - call(method, params) → QFuture<QJsonObject> 异步请求
 *   - connectionStateChanged / notificationReceived 信号通知
 *   - 自增请求 ID 匹配请求与响应
 *
 * @see FrameProtocol  帧编解码器
 * @see IpcMessage     IPC 消息构造与解析
 */

#pragma once
#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include <QFuture>
#include <QFutureInterface>
#include <QHash>
#include <QJsonObject>
#include "IpcMessage.h"

/**
 * @class DaemonClient
 * @brief 与 qtet-daemon 守护进程通信的 IPC 客户端
 *
 * 内部维护一个自增的请求 ID（m_nextId），每次 call() 时自增并作为消息
 * 的唯一标识。发出的请求存入 m_pending 哈希表，收到响应后按 ID 匹配并
 * 通过 QFutureInterface 将结果通知调用方。
 *
 * 连接状态机流转：
 * @dot
 *   Disconnected ──connectToDaemon()──→ Connecting ──socket::connected──→ Connected
 *   Connected    ──socket::disconnected──→ Disconnected ──tryConnect()──→ ...
 * @enddot
 *
 * 注意：所有 QML 单例必须以 QQmlApplicationEngine 为父对象，参见项目架构文档。
 */
class DaemonClient : public QObject {
    Q_OBJECT

    /**
     * @brief 连接状态属性，供 QML 绑定使用
     * 取值：Disconnected(0) / Connecting(1) / Connected(2)
     */
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged FINAL)

public:
    /**
     * @enum ConnectionState
     * @brief 与 daemon 的连接状态枚举
     *
     * - Disconnected: 未连接或已断开
     * - Connecting:   正在尝试建立连接
     * - Connected:    已建立连接，可收发消息
     */
    enum class ConnectionState {
        Disconnected,  ///< 未连接
        Connecting,    ///< 连接中
        Connected,     ///< 已连接
    };
    Q_ENUM(ConnectionState)

    /**
     * @brief 构造函数
     * @param parent 父 QObject，推荐传入 QQmlApplicationEngine 指针
     */
    explicit DaemonClient(QObject *parent = nullptr);

    /// 析构时停止重连定时器，终止待处理请求
    ~DaemonClient() override;

    /**
     * @brief 获取当前连接状态
     * @return ConnectionState 枚举值
     */
    ConnectionState connectionState() const;

    /**
     * @brief 启动到 daemon 的异步连接
     *
     * 调用后状态立即切换为 Connecting，实际连接成功或失败由
     * connectionStateChanged 信号异步通知。默认 socket 名称为
     * "qtet-daemon.sock"，可通过参数指定不同的 socket 名称。
     *
     * @param socketName 本地 Unix Domain Socket 名称
     */
    void connectToDaemon(const QString &socketName = QStringLiteral("qtet-daemon.sock"));

    /**
     * @brief 断开与 daemon 的连接
     *
     * 主动断开连接，停止自动重连机制。断开后所有待处理请求
     * 会收到 "daemon disconnected" 异常通知。
     */
    void disconnectFromDaemon();

    /**
     * @brief 发送 IPC 请求并返回异步结果
     *
     * 将 method 和 params 封装为 JSON-RPC 请求，通过帧协议发送到 daemon。
     * 返回的 QFuture 可通过以下方式使用：
     *   - future.waitForFinished() 同步等待（阻塞当前线程）
     *   - QFutureWatcher 或 QtPromise 异步监听
     *
     * @param method 要调用的方法名，如 "list_instances", "get_instance" 等
     * @param params 请求参数，JSON 对象
     * @return QFuture<QJsonObject> 异步结果
     *
     * @note 如果当前未连接，返回的 future 会立即携带异常信息
     */
    QFuture<QJsonObject> call(const QString &method, const QJsonObject &params);

    static constexpr int kDefaultRequestTimeoutMs = 30000;
    void setRequestTimeout(int ms) { m_requestTimeoutMs = ms; }

signals:
    /**
     * @brief 连接状态变更信号
     * @param state 新的连接状态
     */
    void connectionStateChanged(ConnectionState state);

    /**
     * @brief daemon 主动推送的通知
     *
     * 当 daemon 发送 id=0 的消息时触发此信号。
     * 这类消息是 daemon 主动发起的（非客户端请求的响应），
     * 例如实例状态变更通知、服务端主动推送的数据等。
     *
     * @param method 通知方法名
     * @param params 通知携带的参数
     */
    void notificationReceived(const QString &method, const QJsonObject &params);

private slots:
    /**
     * @brief 本地 socket 有数据到达时的处理
     *
     * 将新数据追加到 m_readBuffer，然后通过 FrameProtocol::decode()
     * 拆解出所有完整帧，逐帧解析为 IpcMessage，再按 id 匹配合适的
     * 待处理请求或触发 notificationReceived 信号。
     */
    void onReadyRead();

    /// socket 成功连接后的处理：重置状态、清理缓冲区、停止重连定时器
    void onSocketConnected();

    /**
     * @brief socket 连接错误处理
     *
     * 只在 Connecting 状态下处理错误。此时将状态切回 Disconnected
     * 并启动重连定时器。如果在 Connected 状态下收到错误，
     * Qt 会随后触发 disconnected 信号，由 onDisconnected() 统一处理。
     */
    void onSocketError();

    /**
     * @brief socket 断开连接处理
     *
     * 断开后执行以下操作：
     *   1. 遍历所有待处理请求，逐一报告异常并结束
     *   2. 清空待处理请求列表
     *   3. 将状态设为 Disconnected
     *   4. 除非是手动断开（m_manualDisconnect）或应用退出中，否则触发自动重连
     */
    void onDisconnected();

private:
    /**
     * @brief 设置连接状态并发出信号
     *
     * 如果新旧状态相同则不触发信号，避免重复通知。
     * @param state 新状态
     */
    void setState(ConnectionState state);

    /**
     * @brief 尝试建立连接
     *
     * 仅在非 Connected 状态下执行。先将状态设为 Connecting，
     * 然后调用 QLocalSocket::connectToServer() 发起异步连接。
     * 连接结果由 onSocketConnected() / onSocketError() 处理。
     */
    void tryConnect();

    ConnectionState m_state = ConnectionState::Disconnected;  ///< 当前连接状态
    QLocalSocket *m_socket = nullptr;                          ///< 本地 socket 对象
    QTimer *m_reconnectTimer = nullptr;                        ///< 重连延时定时器（单次触发，2s 间隔）
    QString m_socketName;                                      ///< 目标 socket 名称
    bool m_manualDisconnect = false;                           ///< 是否为用户主动断开（区分自动断线重连）

    QByteArray m_readBuffer;                                  ///< 接收缓冲区，累积未拆解的帧数据
    QHash<int, QFutureInterface<QJsonObject>> m_pending;      ///< 待匹配请求表（请求ID → Future接口）
    int m_nextId = 1;                                          ///< 自增请求 ID 计数器
    int m_requestTimeoutMs = kDefaultRequestTimeoutMs;         ///< 单次请求超时毫秒数
};
