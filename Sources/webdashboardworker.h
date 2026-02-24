#ifndef WEBDASHBOARDWORKER_H
#define WEBDASHBOARDWORKER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

/**
 * @brief Web控制台进程管理工作类
 *
 * 该类用于在独立线程中管理 Web 控制台进程的启动和停止，
 * 避免阻塞 UI 主线程。
 */
class WebDashboardWorker : public QObject
{
    Q_OBJECT

public:
    /// 进程状态枚举
    enum class ProcessState {
        Stopped,    ///< 已停止
        Starting,   ///< 启动中
        Running,    ///< 运行中
        Stopping    ///< 停止中
    };

    explicit WebDashboardWorker(QObject *parent = nullptr);
    ~WebDashboardWorker();

    /// 获取当前进程状态
    ProcessState processState() const { return m_processState; }

    /// 检查进程是否正在运行
    bool isRunning() const;

    /// 获取进程对象（用于信号连接）
    QProcess* process() const { return m_process; }

public slots:
    /// 启动 Web 控制台进程
    /// @param program 程序路径
    /// @param arguments 启动参数
    void startProcess(const QString &program, const QStringList &arguments);

    /// 停止 Web 控制台进程
    void stopProcess();

signals:
    /// 进程启动完成信号
    /// @param success 是否成功
    /// @param message 结果消息
    void processStarted(bool success, const QString &message);

    /// 进程停止完成信号
    /// @param success 是否成功
    void processStopped(bool success);

    /// 进程状态变化信号
    void stateChanged(ProcessState newState);

    /// 进程异常终止信号
    /// @param exitCode 退出码
    /// @param exitStatus 退出状态
    void processCrashed(int exitCode, QProcess::ExitStatus exitStatus);

private slots:
    void onProcessStarted();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setProcessState(ProcessState state);

    QProcess *m_process = nullptr;
    ProcessState m_processState = ProcessState::Stopped;
};

#endif // WEBDASHBOARDWORKER_H
