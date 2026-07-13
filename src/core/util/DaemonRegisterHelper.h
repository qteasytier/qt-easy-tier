/**
 * @file DaemonRegisterHelper.h
 * @brief daemon 系统服务注册辅助工具类
 */
#pragma once

#include <QString>
#include <QtGlobal>

class DaemonRegisterHelper {
public:
    enum class EnsureResult {
        AlreadyRunning,
        RegisteredAndStartRequested,
        StartRequested,
        RegisterFailed,
        StartFailed,
        DaemonBinaryMissing,
        UnsupportedPlatform,
    };

    enum class RequiredAction {
        None,
        RegisterService,
        StartService,
        DaemonBinaryMissing,
        UnsupportedPlatform,
    };

    static RequiredAction requiredAction();
    static EnsureResult ensureDaemonService();
    static bool isServiceRegistered();
    static bool isDaemonProcessRunning();
    static QString daemonBinaryPath();

#if defined(Q_OS_LINUX)
    static void setSystemdServicePathOverrideForTesting(const QString &path);
    static void setDaemonBinaryPathOverrideForTesting(const QString &path);
    static void setDaemonProcessRunningOverrideForTesting(bool enabled, bool running);
    static QString systemdServiceContentForTesting(const QString &daemonPath);
#endif

private:
    DaemonRegisterHelper() = default;
};
