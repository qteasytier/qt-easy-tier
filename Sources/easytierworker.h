/*
 * @author: YMHuang
 * @date: 2026-02-21
 *
 * @brief: EasyTier进程管理工作类，用于在独立线程中启动和维持EasyTier相关进程
 * @note: 该类设计为在QThread中使用，遵循Qt线程绑定工作对象的规范
 *        所有信号槽连接使用Qt::QueuedConnection确保线程安全
 */

#ifndef EASYTIERWORKER_H
#define EASYTIERWORKER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

/// 启动超时时间（毫秒）
constexpr int EASYTIER_START_TIMEOUT_MS = 30000;

/// CLI状态更新间隔（毫秒）
constexpr int CLI_UPDATE_INTERVAL_MS = 2000;

/// 进程启动等待时间（毫秒）
constexpr int PROCESS_START_WAIT_MS = 3000;

/// 进程终止等待时间（毫秒）
constexpr int PROCESS_KILL_WAIT_MS = 5000;

/// 最大日志显示行数
constexpr int MAX_LOG_LINES = 1000;

class NetPage;  // 前向声明

class EasyTierWorker : public QObject
{
    Q_OBJECT

public:
    /// @brief 进程状态枚举
    enum class ProcessState {
        Stopped,    ///< 进程已停止
        Starting,   ///< 进程正在启动
        Running,    ///< 进程正在运行
        Stopping    ///< 进程正在停止
    };

    explicit EasyTierWorker(QObject *parent = nullptr);
    ~EasyTierWorker() override;

    /// @brief 检查进程是否正在运行
    /// @return true如果进程正在运行
    bool isRunning() const;

    /// @brief 获取当前进程状态
    /// @return 当前进程状态
    ProcessState getProcessState() const { return m_processState; }

public slots:
    /// @brief 启动EasyTier进程
    /// @param networkName 网络名称，用于日志文件命名
    /// @param arguments 启动参数列表
    /// @param appDir 应用程序目录
    /// @param easytierPath EasyTier可执行文件路径
    /// @param rpcPort RPC端口号
    void startEasyTier(const QString& networkName, const QStringList& arguments,
                       const QString& appDir, const QString& easytierPath, int rpcPort);

    /// @brief 停止EasyTier进程
    void stopEasyTier();

signals:
    /// @brief 进程启动完成信号
    /// @param success 是否成功启动
    /// @param errorMessage 错误信息（如果失败）
    void processStarted(bool success, const QString& errorMessage);

    /// @brief 进程停止完成信号
    /// @param success 是否成功停止
    void processStopped(bool success);

    /// @brief 日志输出信号
    /// @param logText 日志文本
    /// @param isError 是否为错误日志
    void logOutput(const QString& logText, bool isError = false);

    /// @brief 节点信息更新信号
    /// @param peers 节点信息JSON数组
    void peerInfoUpdated(const QJsonArray& peers);

    /// @brief 进程异常终止信号
    /// @param exitCode 退出码
    void processCrashed(int exitCode);

private slots:
    /// @brief EasyTier进程标准输出就绪
    void onEasyTierStdoutReady();

    /// @brief EasyTier进程标准错误就绪
    void onEasyTierStderrReady();

    /// @brief EasyTier进程完成
    /// @param exitCode 退出码
    /// @param exitStatus 退出状态
    void onEasyTierFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /// @brief CLI进程完成
    /// @param exitCode 退出码
    /// @param exitStatus 退出状态
    void onCliFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /// @brief 定时更新节点信息
    void onUpdatePeerInfo();

private:
    /// @brief 初始化日志文件
    /// @param networkName 网络名称
    /// @return 是否成功初始化
    bool initLogFile(const QString& networkName);

    /// @brief 关闭日志文件
    void closeLogFile();

    /// @brief 保存日志到文件
    /// @param text 日志文本
    void saveLogToFile(const QString& text);

    /// @brief 设置进程状态
    /// @param state 新状态
    void setProcessState(ProcessState state);

    /// @brief 启动CLI定时器
    void startCliTimer();

    /// @brief 停止CLI定时器
    void stopCliTimer();

    /// @brief 清理所有进程
    void cleanupProcesses();

private:
    // EasyTier核心进程
    QProcess* m_easytierProcess = nullptr;

    // CLI进程（用于获取节点信息）
    QProcess* m_cliProcess = nullptr;

    // CLI更新定时器
    QTimer* m_cliTimer = nullptr;

    // 进程状态
    ProcessState m_processState = ProcessState::Stopped;

    // RPC端口
    int m_rpcPort = 0;

    // 应用程序目录
    QString m_appDir;

    // CLI路径
    QString m_cliPath;

    // 日志文件相关
    QFile* m_logFile = nullptr;
    QTextStream* m_logStream = nullptr;
    QString m_currentLogFileName;

    // 日志行计数器
    int m_logLineCount = 0;
};

#endif // EASYTIERWORKER_H
