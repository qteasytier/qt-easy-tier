/**
 * @file DaemonClient.cpp
 * @brief daemon IPC 客户端实现
 *
 * 实现基于 QLocalSocket 的 IPC 通信客户端，负责：
 *   - 连接管理（连接/断开/自动重连）
 *   - 请求-响应匹配（自增 ID + QHash 存储）
 *   - 帧收发（委托 FrameProtocol 编解码）
 *   - 通知分发（daemon 主动推送的消息）
 */
#include "DaemonClient.h"
#include "FrameProtocol.h"
#include <QCoreApplication>
#include <QDebug>

namespace {

/**
 * @brief daemon 通信相关异常
 *
 * 继承自 QException，支持 QFuture 通过 QFutureInterface::reportException 传递。
 * Qt 6 中 QException 已标记废弃（推荐 QUnhandledException），但目前仍可使用。
 */
struct DaemonException : public QException {
    QString msg;
    explicit DaemonException(QString m) : msg(std::move(m)) {}
    void raise() const override { throw *this; }
    DaemonException *clone() const override { return new DaemonException(*this); }
};

} // anonymous namespace

/**
 * @brief 构造函数
 *
 * 创建 QLocalSocket 和重连定时器，并连接所有信号槽。
 * 所有子对象以 this 为父，随 DaemonClient 生命周期管理。
 */
DaemonClient::DaemonClient(QObject *parent)
    : QObject(parent)
    , m_state(ConnectionState::Disconnected)
{
    // 创建本地 socket 通道
    m_socket = new QLocalSocket(this);

    // 创建重连定时器：单次触发，2s 后执行 tryConnect()
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(2000);

    // ---- 信号槽连接 ----
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &DaemonClient::tryConnect);
    connect(m_socket, &QLocalSocket::connected,
            this, &DaemonClient::onSocketConnected);
    connect(m_socket, &QLocalSocket::errorOccurred,
            this, &DaemonClient::onSocketError);
    connect(m_socket, &QLocalSocket::readyRead,
            this, &DaemonClient::onReadyRead);
    connect(m_socket, &QLocalSocket::disconnected,
            this, &DaemonClient::onDisconnected);
}

DaemonClient::~DaemonClient()
{
    // 确保销毁时停止重连定时器
    m_reconnectTimer->stop();
}

DaemonClient::ConnectionState DaemonClient::connectionState() const
{
    return m_state;
}

/**
 * @brief 启动连接
 *
 * 记录目标 socket 名称，清除手动断开标记，然后调用 tryConnect() 发起连接。
 * 连接结果（成功/失败/断开）由信号槽异步通知。
 */
void DaemonClient::connectToDaemon(const QString &socketName)
{
    m_socketName = socketName;
    m_manualDisconnect = false;
    tryConnect();
}

/**
 * @brief 主动断开连接
 *
 * 设置手动断开标记，停止重连定时器，然后调用 QLocalSocket::disconnectFromServer()。
 * 注意：disconnectFromServer() 是异步操作，实际断开由 onDisconnected() 处理。
 */
void DaemonClient::disconnectFromDaemon()
{
    m_manualDisconnect = true;
    m_reconnectTimer->stop();
    m_socket->disconnectFromServer();
}

/**
 * @brief 发送 IPC 请求
 *
 * 执行流程：
 *   1. 创建 QFutureInterface 并报告已启动
 *   2. 如果未连接，直接报告异常并返回
 *   3. 分配自增 ID，将 future 存入 m_pending 待匹配表
 *   4. 构造 IpcMessage 请求，通过 FrameProtocol::encode() 封装为帧
 *   5. 写入 socket 并立即 flush()
 *   6. 返回 QFuture，供调用方异步等待
 *
 * 当 daemon 返回匹配 id 的响应时，onReadyRead() 中会通过 QFutureInterface
 * 将结果回传给调用方。
 */
QFuture<QJsonObject> DaemonClient::call(const QString &method, const QJsonObject &params)
{
    QFutureInterface<QJsonObject> future;
    future.reportStarted();

    // 未连接时立即返回异常
    if (m_state != ConnectionState::Connected) {
        DaemonException e(QStringLiteral("daemon not connected"));
        future.reportException(e);
        future.reportFinished();
        return future.future();
    }

    // 分配自增请求 ID，溢出后回绕时跳过 0（id=0 是通知消息约定值）
    const int id = m_nextId++;
    if (m_nextId <= 0) m_nextId = 1;

    // 构造请求消息，编码为帧并发送
    const QByteArray frame = FrameProtocol::encode(IpcMessage::request(id, method, params).toJson());
    const qint64 written = m_socket->write(frame);
    if (written != frame.size()) {
        DaemonException e(QStringLiteral("socket write failed"));
        future.reportException(e);
        future.reportFinished();
        return future.future();
    }
    m_socket->flush();

    m_pending.insert(id, future);

    // 设置请求超时：时间到而响应未回来则通知调用方异常
    if (m_requestTimeoutMs > 0) {
        QTimer::singleShot(m_requestTimeoutMs, this, [this, id]() {
            auto it = m_pending.find(id);
            if (it != m_pending.end()) {
                DaemonException e(QStringLiteral("request timeout"));
                it->reportException(e);
                it->reportFinished();
                m_pending.erase(it);
            }
        });
    }

    return future.future();
}

/**
 * @brief socket 数据到达槽
 *
 * 帧拆解流程：
 *   1. 将 socket 中新到达的数据追加到 m_readBuffer
 *   2. 调用 FrameProtocol::decode() 从缓冲区拆解所有完整帧
 *   3. decode() 会消耗已拆解的字节，只保留残留数据
 *   4. 遍历每帧 payload：
 *      a. 通过 IpcMessage::fromJson() 解析 JSON
 *      b. 如果 msg.id == 0 → 这是通知（daemon 主动推送），触发 notificationReceived 信号
 *      c. 如果 msg.id != 0 → 在 m_pending 中查找对应的请求：
 *         - 找到 → 根据 isError 区分成功/失败，通过 QFutureInterface 通知调用方
 *         - 未找到 → 丢弃（可能是重复响应或过期请求）
 *
 * 半包/粘包处理：
 *   - 半包：FrameProtocol::decode() 内部检测到数据不足时 break，剩余数据留在 buffer 中
 *   - 粘包：decode() 的 while 循环会连续拆解所有完整帧，一次处理多帧
 */
void DaemonClient::onReadyRead()
{
    // 追加新到达的数据
    m_readBuffer.append(m_socket->readAll());

    // 从缓冲区拆解所有完整帧（自动处理半包/粘包）
    const auto frames = FrameProtocol::decode(m_readBuffer);
    for (const QByteArray &payload : frames) {
        IpcMessage msg = IpcMessage::fromJson(payload);

        // 非法 JSON 帧不是通知也不是响应，静默丢弃
        if (!msg.parseOk)
            continue;

        // id == 0 表示 daemon 主动推送的通知，不是请求的响应
        if (msg.id == 0) {
            emit notificationReceived(msg.method, msg.params);
            continue;
        }

        // 在待处理请求表中查找对应 ID
        auto it = m_pending.find(msg.id);
        if (it == m_pending.end()) {
            // 未找到匹配的请求，可能是超时已清理或重复响应，静默丢弃
            continue;
        }

        // 根据消息类型报告结果或异常
        if (msg.isError) {
            DaemonException e(QStringLiteral("daemon error: ") + msg.errorMessage);
            it->reportException(e);
        } else {
            it->reportResult(msg.result);
        }

        // 标记 future 结束并从待处理表中移除
        it->reportFinished();
        m_pending.erase(it);
    }
}

/**
 * @brief socket 连接成功处理
 *
 * 重置通信状态：
 *   - 将请求 ID 计数器重置为 1（新连接从头开始编号）
 *   - 清空接收缓冲区（避免残留旧连接的脏数据）
 *   - 停止重连定时器
 *   - 将状态设为 Connected
 */
void DaemonClient::onSocketConnected()
{
    m_nextId = 1;
    m_readBuffer.clear();
    m_reconnectTimer->stop();
    setState(ConnectionState::Connected);
}

/**
 * @brief socket 连接错误处理
 *
 * 仅处理 Connecting 状态下的连接错误。此时将状态切回 Disconnected，
 * 并启动重连定时器在 2s 后重试。
 *
 * 如果在 Connected 状态下收到错误信号，Qt 会随后触发 disconnected 信号，
 * 由 onDisconnected() 统一处理，此处无需重复处理。
 */
void DaemonClient::onSocketError()
{
    if (m_state != ConnectionState::Connecting)
        return;

    setState(ConnectionState::Disconnected);
    m_reconnectTimer->start();
}

/**
 * @brief socket 断开连接处理
 *
 * 断开后的清理流程：
 *   1. 遍历所有待处理请求 → 报告 "daemon disconnected" 异常并结束 future
 *   2. 清空待处理请求表
 *   3. 将状态设为 Disconnected
 *   4. 判断是否需要自动重连：
 *      - 用户主动断开（m_manualDisconnect = true）→ 清除标记，不重连
 *      - 应用正在退出（QCoreApplication::closingDown()）→ 不重连
 *      - 其他情况（意外断线）→ 调用 tryConnect() 自动重连
 */
void DaemonClient::onDisconnected()
{
    // 1. 将所有待处理请求标记为异常并结束
    for (auto &fi : m_pending) {
        DaemonException e(QStringLiteral("daemon disconnected"));
        fi.reportException(e);
        fi.reportFinished();
    }
    m_pending.clear();

    // 2. 更新连接状态
    setState(ConnectionState::Disconnected);

    // 3. 判断是否需要自动重连
    if (m_manualDisconnect) {
        // 用户主动断开，清除标记后不再重连
        m_manualDisconnect = false;
        return;
    }

    // 应用正在退出，不触发重连
    if (QCoreApplication::closingDown())
        return;

    // 意外断线，自动尝试重连
    tryConnect();
}

/**
 * @brief 设置连接状态
 *
 * 仅在状态确实发生变化时才更新并发送信号，避免重复通知。
 */
void DaemonClient::setState(ConnectionState state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit connectionStateChanged(m_state);
}

/**
 * @brief 尝试建立到 daemon 的连接
 *
 * 预检查：
 *   - 已处于 Connected 状态 → 直接返回
 *   - 应用正在退出 → 直接返回
 *
 * 满足条件后：将状态设为 Connecting，然后调用 connectToServer() 发起异步连接。
 * 连接成功/失败由 Qt 信号异步通知。
 */
void DaemonClient::tryConnect()
{
    if (m_state == ConnectionState::Connected)
        return;
    if (QCoreApplication::closingDown())
        return;

    setState(ConnectionState::Connecting);
    m_socket->connectToServer(m_socketName);
}
