/**
 * @file AppLaunchOptions.h
 * @brief 应用启动参数解析工具
 *
 * 只放与应用入口相关的轻量解析逻辑，避免在 main.cpp 中堆叠不可测试的判断。
 */
#pragma once

#include <QStringList>

/** @brief 判断当前启动是否来自开机自启动入口 */
bool isAutoStartLaunch(const QStringList &arguments);
