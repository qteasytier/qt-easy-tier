/**
 * @file AutoStartHelper.cpp
 * @brief 开机自启动辅助工具类实现
 *
 * 实现 Windows 注册表和 Linux XDG Autostart 两种平台的开机自启动管理。
 * 使用匿名命名空间存放常量，避免全局符号污染。
 */
#include "AutoStartHelper.h"
#include "core/util/LogHelper.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

// Windows 平台需要 QSettings 来操作注册表
// 其他平台不引入此头文件以减少编译依赖
#if defined(Q_OS_WIN)
#include <QSettings>
#endif

// ========== 匿名命名空间：存放平台无关常量 ==========
namespace {
// Windows：开机自启动注册表路径（HKCU 当前用户，无需管理员权限）
constexpr char kRegistryKey[] = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
// 应用名称，同时作为注册表键值名和 Linux .desktop 文件名基础
constexpr char kAppName[] = "QtEasyTier";
// 启动参数，应用启动时通过此参数判断是否为开机自启动
constexpr char kAutoStartArg[] = "--autostart";
#if defined(Q_OS_LINUX)
QString g_desktopFilePathOverride;
#endif
} // namespace

bool AutoStartHelper::setAutoStart(bool enabled)
{
    // ========== Windows 平台：注册表操作 ==========
#if defined(Q_OS_WIN)
    // 使用 QSettings 以 NativeFormat 打开注册表 Run 键
    QSettings settings(QLatin1String(kRegistryKey), QSettings::NativeFormat);
    if (enabled) {
        // 构建带引号的可执行文件路径 + 启动参数
        // 格式如："C:\Program Files\QtEasyTier\QtEasyTier.exe" --autostart
        const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        const QString value = QStringLiteral("\"%1\" %2").arg(appPath, QLatin1String(kAutoStartArg));
        settings.setValue(QLatin1String(kAppName), value);
    } else {
        settings.remove(QLatin1String(kAppName));
    }
    // 显式调用 sync() 确保立即写入注册表
    settings.sync();

    // 检查 QSettings 状态判断操作是否成功
    const bool ok = settings.status() == QSettings::NoError;
    if (ok) {
        LogHelper::logInfo(QStringLiteral("开机自启动%1").arg(enabled ? QStringLiteral("设置成功") : QStringLiteral("取消成功")), "AutoStart");
    } else {
        LogHelper::logWarning(QStringLiteral("开机自启动%1失败").arg(enabled ? QStringLiteral("设置") : QStringLiteral("取消")), "AutoStart");
    }
    return ok;

    // ========== Linux 平台：XDG Autostart 规范 ==========
#elif defined(Q_OS_LINUX)
    // .desktop 文件路径：~/.config/autostart/QtEasyTier.desktop
    const QString desktopPath = desktopFilePath();
    if (enabled) {
        // 确保 autostart 目录存在（通常 ~/.config/autostart/）
        QDir dir(QFileInfo(desktopPath).path());
        if (!dir.exists() && !dir.mkpath(".")) {
            LogHelper::logWarning("创建 Linux 自启动目录失败", "AutoStart");
            return false;
        }

        // 写入符合 XDG 规范的 .desktop 文件
        // 注意：Overwrite 模式会完全替换原有内容，不会残留
        QFile file(desktopPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            LogHelper::logWarning("打开 Linux 自启动 desktop 文件失败", "AutoStart");
            return false;
        }

        // 逐行写入 .desktop 文件内容
        QTextStream stream(&file);
        stream << "[Desktop Entry]\n";
        stream << "Type=Application\n";
        stream << "Name=" << QLatin1String(kAppName) << "\n";
        stream << "Exec=" << executablePath() << " " << QLatin1String(kAutoStartArg) << "\n";
        stream << "Comment=Start QtEasyTier at login\n";
        stream << "X-GNOME-Autostart-enabled=true\n";
        file.close();

        // 赋予可执行权限（部分桌面环境如 GNOME 要求 .desktop 文件具有执行权限）
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

        // close() 后会刷新 QFile::error()，检查写入过程是否有错误
        const bool ok = file.error() == QFile::NoError;
        if (ok) {
            LogHelper::logInfo(QStringLiteral("Linux 开机自启动设置成功：%1").arg(desktopPath), "AutoStart");
        } else {
            LogHelper::logWarning("Linux 开机自启动设置失败", "AutoStart");
        }
        return ok;
    } else {
        // 取消自启动：直接删除 .desktop 文件
        QFile file(desktopPath);
        if (!file.exists())
            return true; // 文件本就不存在，视为取消成功（幂等操作）
        if (!file.remove()) {
            LogHelper::logWarning("删除 Linux 自启动 desktop 文件失败", "AutoStart");
            return false;
        }
        LogHelper::logInfo(QStringLiteral("Linux 开机自启动取消成功：%1").arg(desktopPath), "AutoStart");
        return true;
    }

    // ========== 其他平台（macOS 等）：暂不支持 ==========
#else
    Q_UNUSED(enabled)
    LogHelper::logWarning("当前平台不支持开机自启动设置", "AutoStart");
    return false;
#endif
}

bool AutoStartHelper::isAutoStartEnabled()
{
    // ========== Windows 平台：检查注册表 ==========
#if defined(Q_OS_WIN)
    QSettings settings(QLatin1String(kRegistryKey), QSettings::NativeFormat);
    return settings.contains(QLatin1String(kAppName));

    // ========== Linux 平台：检查 .desktop 文件是否存在 ==========
#elif defined(Q_OS_LINUX)
    return QFile::exists(desktopFilePath());

    // ========== 其他平台：始终认为未设置 ==========
#else
    return false;
#endif
}

// ========== Linux 平台专用辅助方法实现 ==========
#if defined(Q_OS_LINUX)

void AutoStartHelper::setDesktopFilePathOverrideForTesting(const QString &path)
{
    g_desktopFilePathOverride = path;
}

/**
 * @brief 返回 Linux 自启动 .desktop 文件完整路径
 *
 * 路径拼接规则：
 *   QStandardPaths::ConfigLocation = ~/.config
 *   → ~/.config/autostart/QtEasyTier.desktop
 *
 * 这符合 XDG Autostart 规范，桌面环境（GNOME、KDE 等）会自动扫描此目录。
 */
QString AutoStartHelper::desktopFilePath()
{
    if (!g_desktopFilePathOverride.isEmpty())
        return g_desktopFilePathOverride;

    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QDir(configDir).filePath(QStringLiteral("autostart/%1.desktop").arg(QLatin1String(kAppName)));
}

/**
 * @brief 返回当前可执行文件完整路径
 *
 * 直接调用 QCoreApplication::applicationFilePath() 获取，
 * 用于填充 .desktop 文件中的 Exec 字段。
 * 启动时会额外附加 --autostart 参数。
 */
QString AutoStartHelper::executablePath()
{
    return QCoreApplication::applicationFilePath();
}
#endif
