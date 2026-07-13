/**
 * @file TrayMessageHelper.cpp
 * @brief 系统托盘消息便捷入口实现
 */
#include "TrayMessageHelper.h"
#include "core/system_tray/TrayMessageDispatcher.h"

namespace {

void showMessage(TrayMessageLevel level, const QString &title, const QString &message, int durationMs)
{
    TrayMessageDispatcher::instance()->showMessage({level, title, message, durationMs});
}

} // namespace

void TrayMessageHelper::showInfo(const QString &title, const QString &message, int durationMs)
{
    showMessage(TrayMessageLevel::Info, title, message, durationMs);
}

void TrayMessageHelper::showWarning(const QString &title, const QString &message, int durationMs)
{
    showMessage(TrayMessageLevel::Warning, title, message, durationMs);
}

void TrayMessageHelper::showError(const QString &title, const QString &message, int durationMs)
{
    showMessage(TrayMessageLevel::Error, title, message, durationMs);
}
