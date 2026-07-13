/**
 * @file AppLaunchOptions.cpp
 * @brief 应用启动参数解析工具实现
 */
#include "AppLaunchOptions.h"

bool isAutoStartLaunch(const QStringList &arguments)
{
    // 精确匹配自启动参数，避免 --autostarted 等相似参数误判。
    return arguments.contains(QStringLiteral("--autostart"));
}
