#include "webdashboardworker.h"
#include <QTimer>
#include <iostream>

/// 启动超时时间（毫秒）
static constexpr int START_TIMEOUT_MS = 10000;
/// 进程终止等待时间（毫秒）
static constexpr int KILL_WAIT_MS = 5000;

WebDashboardWorker::WebDashboardWorker(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    // 连接进程信号（使用 QueuedConnection 确保异步执行）
    connect(m_process, &QProcess::started, this, &WebDashboardWorker::onProcessStarted, Qt::QueuedConnection);
    connect(m_process, &QProcess::errorOccurred, this, &WebDashboardWorker::onProcessError, Qt::QueuedConnection);
    connect(m_process, &QProcess::finished, this, &WebDashboardWorker::onProcessFinished, Qt::QueuedConnection);
}

WebDashboardWorker::~WebDashboardWorker()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->disconnect();
        m_process->kill();
        m_process->waitForFinished(KILL_WAIT_MS);
    }
}

bool WebDashboardWorker::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void WebDashboardWorker::startProcess(const QString &program, const QStringList &arguments)
{
    if (m_processState == ProcessState::Starting || m_processState == ProcessState::Stopping) {
        emit processStarted(false, tr("进程正在启动或停止中，请稍候"));
        return;
    }

    if (m_process->state() != QProcess::NotRunning) {
        emit processStarted(false, tr("进程已在运行中"));
        return;
    }

    setProcessState(ProcessState::Starting);
    std::clog << "启动Web控制台: " << program.toStdString() 
              << " " << arguments.join(" ").toStdString() << std::endl;

    m_process->start(program, arguments);

    // 设置启动超时检测
    QTimer::singleShot(START_TIMEOUT_MS, m_process, [this]() {
        if (m_processState == ProcessState::Starting && m_process->state() != QProcess::Running) {
            std::cerr << "Web控制台启动超时" << std::endl;
            m_process->kill();
            setProcessState(ProcessState::Stopped);
            emit processStarted(false, tr("Web控制台启动超时"));
        }
    });
}

void WebDashboardWorker::stopProcess()
{
    if (m_processState == ProcessState::Stopped) {
        emit processStopped(true);
        return;
    }

    if (m_processState == ProcessState::Stopping) {
        return;
    }

    std::clog << "正在停止Web控制台进程..." << std::endl;
    setProcessState(ProcessState::Stopping);

    // 断开所有信号连接
    m_process->disconnect();

    // 连接 finished 信号用于发射停止完成信号
    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitCode);
        Q_UNUSED(exitStatus);
        setProcessState(ProcessState::Stopped);
        std::clog << "Web控制台进程已停止" << std::endl;
        emit processStopped(true);
    }, Qt::QueuedConnection);

    // 直接杀死进程
    m_process->kill();
}

void WebDashboardWorker::onProcessStarted()
{
    setProcessState(ProcessState::Running);
    std::clog << "Web控制台进程启动成功" << std::endl;
    emit processStarted(true, tr("Web控制台启动成功"));
}

void WebDashboardWorker::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = tr("Web控制台启动失败：无法启动进程");
        break;
    case QProcess::Crashed:
        errorMsg = tr("Web控制台进程崩溃");
        break;
    case QProcess::Timedout:
        errorMsg = tr("Web控制台进程操作超时");
        break;
    case QProcess::WriteError:
        errorMsg = tr("Web控制台进程写入错误");
        break;
    case QProcess::ReadError:
        errorMsg = tr("Web控制台进程读取错误");
        break;
    default:
        errorMsg = tr("Web控制台进程发生未知错误");
        break;
    }

    std::cerr << "Web控制台错误: " << errorMsg.toStdString() << std::endl;

    if (m_processState == ProcessState::Starting) {
        setProcessState(ProcessState::Stopped);
        emit processStarted(false, errorMsg);
    }
}

void WebDashboardWorker::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // 手动停止时已断开此连接，这里只有异常停止才会触发
    setProcessState(ProcessState::Stopped);
    
    std::cerr << "Web控制台进程异常终止，退出码: " << exitCode << std::endl;
    emit processCrashed(exitCode, exitStatus);
}

void WebDashboardWorker::setProcessState(ProcessState state)
{
    if (m_processState != state) {
        m_processState = state;
        emit stateChanged(state);
    }
}