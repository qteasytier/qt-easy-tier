/**
 * @file AutoStartHelper.h
 * @brief 开机自启动辅助工具类
 *
 * 负责管理应用的开机自启动功能，支持以下平台：
 * - Windows：通过注册表 HKCU\...\Run 项实现
 * - Linux：通过 XDG Autostart 规范（~/.config/autostart/xxx.desktop）实现
 * - macOS：暂不支持，相关方法返回 false
 *
 * 所有方法均为静态方法，构造函数私有，禁止实例化。
 * 启动时携带 --autostart 参数以区分用户手动启动和系统自动启动。
 */
#pragma once

#include <QString>
#include <QtGlobal>

class AutoStartHelper {
public:
    /**
     * @brief 设置或取消应用的开机自启动
     * @param enabled true = 启用开机自启动，false = 取消开机自启动
     * @return true = 操作成功；false = 操作失败（会自动输出日志说明原因）
     *
     * Windows 平台：写入/删除 HKCU\Software\Microsoft\Windows\CurrentVersion\Run 注册表项
     * Linux 平台：创建/删除 ~/.config/autostart/QtEasyTier.desktop 文件
     * 其他平台：始终返回 false 并输出警告
     */
    static bool setAutoStart(bool enabled);

    /**
     * @brief 查询当前系统是否已设置开机自启动
     * @return true = 已设置开机自启动，false = 未设置或当前平台不支持
     *
     * Windows：检查注册表 Run 键中是否存在对应条目
     * Linux：检查 ~/.config/autostart/QtEasyTier.desktop 文件是否存在
     * 其他平台：始终返回 false
     */
    static bool isAutoStartEnabled();

#if defined(Q_OS_LINUX)
    static void setDesktopFilePathOverrideForTesting(const QString &path);
#endif

private:
    /// 私有构造函数，禁止外部实例化
    AutoStartHelper() = default;

    // Linux 平台专用辅助方法（在其他平台不可见）
#if defined(Q_OS_LINUX)
    /**
     * @brief 获取 Linux 自启动 .desktop 文件的完整路径
     * @return ~/.config/autostart/QtEasyTier.desktop 的绝对路径
     */
    static QString desktopFilePath();

    /**
     * @brief 获取当前可执行文件的绝对路径
     * @return QCoreApplication::applicationFilePath() 的返回值
     *
     * 用于填充 .desktop 文件的 Exec 字段，
     * 确保自启动时能准确定位到应用程序。
     */
    static QString executablePath();
#endif
};
