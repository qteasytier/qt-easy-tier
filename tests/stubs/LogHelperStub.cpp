/**
 * @file LogHelperStub.cpp
 * @brief LogHelper 存根（Stub）实现，用于测试中替换真实日志输出。
 *
 * 将 logInfo / logWarning / logError 重定向到 Qt 标准 qInfo / qWarning / qCritical 输出，
 * 避免测试环境依赖完整的平台日志业务逻辑。
 */
#include "core/util/LogHelper.h"
#include <QDebug>

void LogHelper::logInfo(const QString &message, const QString &context)
{
    if (context.isEmpty())
        qInfo().noquote() << message;
    else
        qInfo().noquote() << QStringLiteral("[%1] %2").arg(context, message);
}

void LogHelper::logWarning(const QString &message, const QString &context)
{
    if (context.isEmpty())
        qWarning().noquote() << message;
    else
        qWarning().noquote() << QStringLiteral("[%1] %2").arg(context, message);
}

void LogHelper::logError(const QString &message, const QString &context)
{
    if (context.isEmpty())
        qCritical().noquote() << message;
    else
        qCritical().noquote() << QStringLiteral("[%1] %2").arg(context, message);
}
