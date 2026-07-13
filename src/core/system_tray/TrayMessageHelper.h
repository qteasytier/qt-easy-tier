/**
 * @file TrayMessageHelper.h
 * @brief 系统托盘消息便捷入口
 *
 * 业务代码优先使用本命名空间发送托盘消息，保持调用形式与 LogHelper 类似。
 */
#pragma once

#include <QString>

namespace TrayMessageHelper {
void showInfo(const QString &title, const QString &message, int durationMs = 3000);
void showWarning(const QString &title, const QString &message, int durationMs = 3000);
void showError(const QString &title, const QString &message, int durationMs = 3000);
}
