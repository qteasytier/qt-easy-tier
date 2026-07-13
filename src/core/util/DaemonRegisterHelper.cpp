/**
 * @file DaemonRegisterHelper.cpp
 * @brief daemon 系统服务注册辅助工具类实现
 */
#include "DaemonRegisterHelper.h"
#include "core/util/LogHelper.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>

namespace {
constexpr char kServiceName[] = "qtet-daemon.service";
constexpr char kDaemonName[] = "qtet-daemon";
constexpr int kProcessTimeoutMs = 30000;

#if defined(Q_OS_LINUX)
QString g_servicePathOverride;
QString g_daemonPathOverride;
bool g_processRunningOverrideEnabled = false;
bool g_processRunningOverride = false;

QString systemdServicePath()
{
    if (!g_servicePathOverride.isEmpty())
        return g_servicePathOverride;
    return QStringLiteral("/etc/systemd/system/%1").arg(QLatin1String(kServiceName));
}

QString linuxDaemonBinaryPath()
{
    if (!g_daemonPathOverride.isEmpty())
        return g_daemonPathOverride;
    return QDir(QCoreApplication::applicationDirPath()).filePath(QLatin1String(kDaemonName));
}

QString systemdEscapePath(QString path)
{
    path.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    path.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return path;
}

QString shellQuote(QString value)
{
    value.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'%1'").arg(value);
}

QString serviceContent(const QString &daemonPath)
{
    const QFileInfo daemonInfo(daemonPath);
    const QString execStart = systemdEscapePath(daemonInfo.absoluteFilePath());
    const QString workingDirectory = systemdEscapePath(daemonInfo.absolutePath());

    QString content;
    QTextStream stream(&content);
    stream << "[Unit]\n";
    stream << "Description=EasyTier Service\n";
    stream << "After=network.target network-online.target syslog.target\n";
    stream << "Wants=network.target network-online.target\n";
    stream << "\n";
    stream << "[Service]\n";
    stream << "Type=simple\n";
    stream << "ExecStart=" << execStart << "\n";
    stream << "WorkingDirectory=" << workingDirectory << "\n";
    stream << "Restart=always\n";
    stream << "RestartSec=3s\n";
    stream << "StartLimitIntervalSec=0\n";
    stream << "\n";
    stream << "[Install]\n";
    stream << "WantedBy=multi-user.target\n";
    return content;
}

bool runProcess(const QString &program, const QStringList &arguments, int timeoutMs = kProcessTimeoutMs)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(timeoutMs))
        return false;
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished();
        return false;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

bool writeTemporaryServiceFile(QTemporaryFile &file, const QString &daemonPath)
{
    if (!file.open())
        return false;

    QTextStream stream(&file);
    stream << serviceContent(daemonPath);
    stream.flush();
    if (stream.status() != QTextStream::Ok)
        return false;

    file.close();
    return file.error() == QFileDevice::NoError;
}

bool installAndStartService(const QString &daemonPath)
{
    QTemporaryFile serviceFile(QDir::tempPath() + QStringLiteral("/qtet-daemon-service-XXXXXX.service"));
    serviceFile.setAutoRemove(true);
    if (!writeTemporaryServiceFile(serviceFile, daemonPath)) {
        LogHelper::logWarning(QStringLiteral("生成 qtet-daemon systemd service 临时文件失败"), "DaemonRegister");
        return false;
    }

    const QString command = QStringLiteral("install -m 0644 %1 %2 && systemctl daemon-reload && systemctl enable %3 && systemctl start %3")
                                .arg(shellQuote(serviceFile.fileName()),
                                     shellQuote(systemdServicePath()),
                                     shellQuote(QLatin1String(kServiceName)));
    return runProcess(QStringLiteral("pkexec"), {QStringLiteral("sh"), QStringLiteral("-c"), command}, 120000);
}

bool startService()
{
    return runProcess(QStringLiteral("pkexec"), {QStringLiteral("systemctl"), QStringLiteral("start"), QLatin1String(kServiceName)}, 120000);
}
#endif
} // namespace

DaemonRegisterHelper::RequiredAction DaemonRegisterHelper::requiredAction()
{
#if defined(Q_OS_LINUX)
    const QString path = daemonBinaryPath();
    const QFileInfo daemonInfo(path);

    if (!daemonInfo.exists() || !daemonInfo.isFile() || !daemonInfo.isExecutable()) {
        LogHelper::logWarning(QStringLiteral("daemon 二进制缺失或不可执行: exists=%1 file=%2 executable=%3 path=%4")
                                  .arg(daemonInfo.exists())
                                  .arg(daemonInfo.isFile())
                                  .arg(daemonInfo.isExecutable())
                                  .arg(path),
                              "DaemonRegister");
        LogHelper::logInfo(QStringLiteral("检测结果: daemon 二进制缺失，服务注册状态与进程状态未继续检测"), "DaemonRegister");
        return RequiredAction::DaemonBinaryMissing;
    }

    const bool registered = isServiceRegistered();
    if (!registered) {
        LogHelper::logInfo(QStringLiteral("检测结果: daemon 二进制可用，systemd 服务未注册，进程状态未继续检测"), "DaemonRegister");
        return RequiredAction::RegisterService;
    }

    const bool running = isDaemonProcessRunning();
    if (!running) {
        LogHelper::logInfo(QStringLiteral("检测结果: daemon 二进制可用，systemd 服务已注册，qtet-daemon 进程未运行"), "DaemonRegister");
        return RequiredAction::StartService;
    }

    LogHelper::logInfo(QStringLiteral("检测结果: daemon 二进制可用，systemd 服务已注册，qtet-daemon 进程运行中"), "DaemonRegister");
    return RequiredAction::None;
#else
    LogHelper::logInfo(QStringLiteral("检测结果: 当前平台不支持自动注册后端服务"), "DaemonRegister");
    return RequiredAction::UnsupportedPlatform;
#endif
}

DaemonRegisterHelper::EnsureResult DaemonRegisterHelper::ensureDaemonService()
{
#if defined(Q_OS_LINUX)
    const QString daemonPath = daemonBinaryPath();
    switch (requiredAction()) {
    case RequiredAction::None:
        return EnsureResult::AlreadyRunning;
    case RequiredAction::DaemonBinaryMissing:
        LogHelper::logWarning(QStringLiteral("qtet-daemon 不存在或不可执行：%1").arg(daemonPath), "DaemonRegister");
        return EnsureResult::DaemonBinaryMissing;
    case RequiredAction::RegisterService:
        if (!installAndStartService(daemonPath)) {
            LogHelper::logWarning(QStringLiteral("注册并启动 qtet-daemon systemd 服务失败"), "DaemonRegister");
            return EnsureResult::RegisterFailed;
        }
        LogHelper::logInfo(QStringLiteral("已请求注册并启动 qtet-daemon systemd 服务"), "DaemonRegister");
        return EnsureResult::RegisteredAndStartRequested;
    case RequiredAction::StartService:
        if (!startService()) {
            LogHelper::logWarning(QStringLiteral("启动 qtet-daemon systemd 服务失败"), "DaemonRegister");
            return EnsureResult::StartFailed;
        }
        LogHelper::logInfo(QStringLiteral("已请求启动 qtet-daemon systemd 服务"), "DaemonRegister");
        return EnsureResult::StartRequested;
    case RequiredAction::UnsupportedPlatform:
        return EnsureResult::UnsupportedPlatform;
    }
    return EnsureResult::UnsupportedPlatform;
#else
    return EnsureResult::UnsupportedPlatform;
#endif
}

bool DaemonRegisterHelper::isServiceRegistered()
{
#if defined(Q_OS_LINUX)
    return QFile::exists(systemdServicePath());
#else
    return false;
#endif
}

bool DaemonRegisterHelper::isDaemonProcessRunning()
{
#if defined(Q_OS_LINUX)
    if (g_processRunningOverrideEnabled)
        return g_processRunningOverride;
    return runProcess(QStringLiteral("pidof"), {QLatin1String(kDaemonName)}, 5000);
#else
    return false;
#endif
}

QString DaemonRegisterHelper::daemonBinaryPath()
{
#if defined(Q_OS_LINUX)
    return linuxDaemonBinaryPath();
#else
    return QString();
#endif
}

#if defined(Q_OS_LINUX)
void DaemonRegisterHelper::setSystemdServicePathOverrideForTesting(const QString &path)
{
    g_servicePathOverride = path;
}

void DaemonRegisterHelper::setDaemonBinaryPathOverrideForTesting(const QString &path)
{
    g_daemonPathOverride = path;
}

void DaemonRegisterHelper::setDaemonProcessRunningOverrideForTesting(bool enabled, bool running)
{
    g_processRunningOverrideEnabled = enabled;
    g_processRunningOverride = running;
}

QString DaemonRegisterHelper::systemdServiceContentForTesting(const QString &daemonPath)
{
    return serviceContent(daemonPath);
}
#endif
