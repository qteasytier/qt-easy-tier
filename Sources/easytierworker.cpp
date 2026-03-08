/*
 * @author: YMHuang
 * @date: 2026-02-21
 *
 * @brief: EasyTier进程管理工作类实现
 */

#include "easytierworker.h"
#include "generateconf.h"

#include <QCoreApplication>
#include <QDir>
#include <QDateTime>

EasyTierWorker::EasyTierWorker(QObject *parent)
    : QObject(parent)
{
    // CLI更新定时器
    m_cliTimer = new QTimer(this);
    m_cliTimer->setInterval(CLI_UPDATE_INTERVAL_MS);
    connect(m_cliTimer, &QTimer::timeout, this, &EasyTierWorker::onUpdatePeerInfo);
}

EasyTierWorker::~EasyTierWorker()
{
    // 停止所有进程
    cleanupProcesses();
}

bool EasyTierWorker::isRunning() const
{
    return m_easytierProcess != nullptr &&
           m_easytierProcess->state() == QProcess::Running;
}

void EasyTierWorker::startEasyTier(const QString& networkName, const QStringList& arguments, const int &rpcPort)
{
    // 如果进程已经在运行，先停止
    if (isRunning()) {
        emit logOutput(tr("进程已在运行，先停止当前进程..."), false);
        stopEasyTier();
    }

    setProcessState(ProcessState::Starting);

    // 内部检测并准备 EasyTier 程序路径
    m_appDir = QCoreApplication::applicationDirPath() + "/etcore";
#ifdef Q_OS_WIN
    const QString &easytierPath = m_appDir + "/easytier-core.exe";
#else
    const QString &easytierPath = m_appDir + "/easytier-core";
#endif

    // 检查程序是否存在
    QFileInfo fileInfo(easytierPath);
    if (!fileInfo.exists()) {
        QString errorMsg = tr("错误: 找不到 %1").arg(easytierPath);
        emit logOutput(errorMsg, true);
        setProcessState(ProcessState::Stopped);
        emit processStarted(false, errorMsg);
        return;
    }
    m_rpcPort = rpcPort;

    emit logOutput(tr("正在启动EasyTier进程..."), false);
    emit logOutput(tr("启动参数: %1").arg(arguments.join(" ")), false);

    // 创建EasyTier进程
    m_easytierProcess = new QProcess(this);
    m_easytierProcess->setWorkingDirectory(m_appDir);

    // 连接信号槽
    connect(m_easytierProcess, &QProcess::readyReadStandardOutput,
            this, &EasyTierWorker::onEasyTierStdoutReady);
    connect(m_easytierProcess, &QProcess::readyReadStandardError,
            this, &EasyTierWorker::onEasyTierStderrReady);
    connect(m_easytierProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &EasyTierWorker::onEasyTierFinished);

    // 启动进程
#ifdef Q_OS_WIN
    m_easytierProcess->start(easytierPath, arguments);
#elif defined(Q_OS_LINUX)
    const QStringList &tureArgs = QStringList(easytierPath) + arguments;
    m_easytierProcess->start("pkexec", tureArgs);
#endif

    // 等待进程启动
    if (!m_easytierProcess->waitForStarted(PROCESS_START_WAIT_MS)) {
        QString errorMsg = tr("EasyTier进程启动超时");
        emit logOutput(errorMsg, true);
        m_easytierProcess->kill();
        m_easytierProcess->deleteLater();
        m_easytierProcess = nullptr;
        setProcessState(ProcessState::Stopped);
        emit processStarted(false, errorMsg);
        return;
    }

    emit logOutput(tr("EasyTier进程启动成功"), false);
    setProcessState(ProcessState::Running);

    // 如果 RPC 端口不是 NO_USE_CLI，则启动 CLI 定时器
    if (m_rpcPort != NO_USE_CLI) {
        startCliTimer();
    } else {
        emit logOutput(tr("未配置 RPC 端口，不启动 CLI 监测"), false);
    }

    emit processStarted(true, QString());
}

void EasyTierWorker::stopEasyTier()
{
    if (!m_easytierProcess) {
        emit processStopped(true);
        return;
    }

    setProcessState(ProcessState::Stopping);

    // 先停止CLI定时器
    stopCliTimer();

    // 停止CLI进程
    if (m_cliProcess) {
        m_cliProcess->disconnect();
        m_cliProcess->kill();
        m_cliProcess->waitForFinished(1000);
        m_cliProcess->deleteLater();
        m_cliProcess = nullptr;
    }

    emit logOutput(tr("正在停止EasyTier进程..."), false);

    // 断开信号连接，避免在kill过程中触发finished信号
    m_easytierProcess->disconnect();

    // 终止EasyTier进程
    m_easytierProcess->kill();

    // 等待进程结束
    bool finished = m_easytierProcess->waitForFinished(PROCESS_KILL_WAIT_MS);

    if (finished) {
        emit logOutput(tr("EasyTier进程已停止"), false);
    } else {
        emit logOutput(tr("警告: EasyTier进程可能未完全终止"), true);
    }

    m_easytierProcess->deleteLater();
    m_easytierProcess = nullptr;

    setProcessState(ProcessState::Stopped);
    emit processStopped(finished);
}

void EasyTierWorker::onEasyTierStdoutReady()
{
    if (!m_easytierProcess) return;

    QString output = m_easytierProcess->readAllStandardOutput();
    emit logOutput(output, false);
}

void EasyTierWorker::onEasyTierStderrReady()
{
    if (!m_easytierProcess) return;

    QString error = m_easytierProcess->readAllStandardError();
    QString errorText = tr("错误: ") + error;
    emit logOutput(errorText, true);
}

void EasyTierWorker::onEasyTierFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString msg;
    if (exitStatus == QProcess::NormalExit) {
        msg = tr("EasyTier已停止，退出码: %1").arg(exitCode);
    } else {
        msg = tr("EasyTier异常终止，退出码: %1").arg(exitCode);
    }

    emit logOutput(msg, exitStatus != QProcess::NormalExit);

    // 停止CLI定时器
    stopCliTimer();

    // 清理CLI进程
    if (m_cliProcess) {
        m_cliProcess->disconnect();
        m_cliProcess->kill();
        m_cliProcess->deleteLater();
        m_cliProcess = nullptr;
    }

    // 清理EasyTier进程
    if (m_easytierProcess) {
        m_easytierProcess->deleteLater();
        m_easytierProcess = nullptr;
    }

    setProcessState(ProcessState::Stopped);

    // 如果是异常终止，发送崩溃信号
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        emit processCrashed(exitCode);
    } else
    {
        emit processCrashed(114514);
    }
}

void EasyTierWorker::onCliFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    if (!m_cliProcess || exitCode != 0) {
        return;
    }

    QByteArray output = m_cliProcess->readAllStandardOutput();
    QString errorOutput = m_cliProcess->readAllStandardError();

    if (!errorOutput.isEmpty()) {
        emit logOutput(tr("CLI错误: ") + errorOutput, true);
        return;
    }

    if (output.isEmpty()) {
        return;
    }

    // 解析JSON
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(output, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        emit logOutput(tr("JSON解析错误: ") + jsonError.errorString(), true);
        return;
    }

    if (!doc.isArray()) {
        emit logOutput(tr("JSON数据格式错误: 应为数组格式"), true);
        return;
    }

    emit peerInfoUpdated(doc.array());
}

void EasyTierWorker::onUpdatePeerInfo()
{
    // 检查EasyTier进程是否还在运行
    if (!isRunning()) {
        stopCliTimer();
        return;
    }

    // 如果CLI进程还在运行，跳过本次更新
    if (m_cliProcess && m_cliProcess->state() == QProcess::Running) {
        return;
    }

    // 检查CLI程序是否存在
#ifdef Q_OS_WIN
    const QString &cliPath = m_appDir + "/easytier-cli.exe";
#elif defined(Q_OS_LINUX)
    const QString &cliPath = m_appDir + "/easytier-cli";
#endif

    QFileInfo cliFileInfo(cliPath);
    if (!cliFileInfo.exists()) {
        emit logOutput(tr("错误: 找不到CLI程序: %1").arg(cliPath), true);
        return;
    }

    // 创建或重用CLI进程
    if (!m_cliProcess) {
        m_cliProcess = new QProcess(this);
        m_cliProcess->setWorkingDirectory(m_appDir);

        connect(m_cliProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &EasyTierWorker::onCliFinished);
    }

    // 启动CLI获取节点信息
    QStringList cliArgs;
    cliArgs << "-p" << QString("127.0.0.1:%1").arg(m_rpcPort)
            << "-o" << "json" << "peer";

    m_cliProcess->start(cliPath, cliArgs);
}

void EasyTierWorker::setProcessState(ProcessState state)
{
    m_processState = state;
}

void EasyTierWorker::startCliTimer()
{
    if (m_cliTimer && !m_cliTimer->isActive()) {
        m_cliTimer->start();
    }
}

void EasyTierWorker::stopCliTimer()
{
    if (m_cliTimer && m_cliTimer->isActive()) {
        m_cliTimer->stop();
    }
}

void EasyTierWorker::cleanupProcesses()
{
    // 停止定时器
    stopCliTimer();

    // 停止CLI进程
    if (m_cliProcess) {
        m_cliProcess->disconnect();
        m_cliProcess->kill();
        m_cliProcess->waitForFinished(1000);
        m_cliProcess->deleteLater();
        m_cliProcess = nullptr;
    }

    // 停止EasyTier进程
    if (m_easytierProcess) {
        m_easytierProcess->disconnect();
        m_easytierProcess->kill();
        m_easytierProcess->waitForFinished(PROCESS_KILL_WAIT_MS);
        m_easytierProcess->deleteLater();
        m_easytierProcess = nullptr;
    }

    setProcessState(ProcessState::Stopped);
}
