/**
 * @file LogHelper.h
 * @brief 日志工具函数命名空间
 *
 * 提供便捷的静态日志函数，封装 LogDispatcher 单例调用。
 * 所有函数同时将日志输出到控制台（qInfo/qWarning/qCritical）和
 * 所有已注册的 LogSink，确保日志既在终端可见又可持久化。
 *
 * 使用方式：
 *   LogHelper::logInfo("操作成功", "ModuleName");
 *   LogHelper::logWarning("配置缺失", "Config");
 *   LogHelper::logError("连接失败: " + err, "Network");
 */
#pragma once
#include "core/log/LogTypes.h"

#include <QString>

/**
 * @brief 日志辅助命名空间
 *
 * 对 LogDispatcher 进行简单封装，统一日志写入入口。
 * 各模块通过此命名空间输出日志，无需直接依赖 LogDispatcher。
 */
namespace LogHelper {
void logInfo(const QString &message, const QString &context = {});
void logWarning(const QString &message, const QString &context = {});
void logError(const QString &message, const QString &context = {});
}
