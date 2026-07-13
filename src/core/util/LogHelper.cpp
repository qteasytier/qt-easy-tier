/**
 * @file LogHelper.cpp
 * @brief 日志工具函数实现
 *
 * 核心策略：
 * - 每条日志同时写入控制台和所有 LogSink，确保终端可见 + 持久化
 * - 控制台输出使用 Qt 的 qInfo/qWarning/qCritical 流，不使用格式化字符串避免开销
 * - 匿名命名空间中的 writeToConsole 为内部实现细节，不对外暴露
 */
#include "LogHelper.h"
#include "core/log/LogDispatcher.h"
#include <QDebug>

namespace {

/**
 * @brief 将日志写入控制台（qDebug 流）
 * @param level 日志级别，决定使用 qInfo/qWarning/qCritical
 * @param context 日志来源上下文
 * @param message 日志正文
 *
 * 使用 noquote() 避免 Qt 自动为字符串添加引号，保持日志输出整洁。
 * context 非空时采用 [context] message 格式；为空时直接输出 message。
 */
void writeToConsole(LogLevel level, const QString &context, const QString &message)
{
    // 格式化带上下文的日志前缀
    QString full = context.isEmpty() ? message : QStringLiteral("[%1] %2").arg(context, message);
    switch (level) {
    case LogLevel::Info:
        qInfo().noquote() << full;
        break;
    case LogLevel::Warning:
        qWarning().noquote() << full;
        break;
    case LogLevel::Error:
        qCritical().noquote() << full;
        break;
    default:
        break;
    }
}

} // anonymous namespace

void LogHelper::logInfo(const QString &message, const QString &context)
{
    writeToConsole(LogLevel::Info, context, message);
    LogDispatcher::instance()->log(LogLevel::Info, message, context);
}

void LogHelper::logWarning(const QString &message, const QString &context)
{
    writeToConsole(LogLevel::Warning, context, message);
    LogDispatcher::instance()->log(LogLevel::Warning, message, context);
}

void LogHelper::logError(const QString &message, const QString &context)
{
    writeToConsole(LogLevel::Error, context, message);
    LogDispatcher::instance()->log(LogLevel::Error, message, context);
}
