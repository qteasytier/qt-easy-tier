/** @file AutoStartService.cpp @brief AutoStartService 实现 */
#include "AutoStartService.h"

#include "core/util/AutoStartHelper.h"

bool AutoStartService::setEnabled(bool enabled) const
{
    // 委托 AutoStartHelper 执行系统级自启动设置（Linux 下一般为 desktop 文件）
    return AutoStartHelper::setAutoStart(enabled);
}

bool AutoStartService::isEnabled() const
{
    return AutoStartHelper::isAutoStartEnabled();
}
