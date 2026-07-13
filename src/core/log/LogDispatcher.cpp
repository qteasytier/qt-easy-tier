/**
 * @file LogDispatcher.cpp
 * @brief 日志分发器实现
 *
 * 核心逻辑：
 * 1. instance() 提供线程安全的静态单例（C++11 保证局部静态变量初始化线程安全）
 * 2. addSink()/removeSink() 管理 sink 列表，防止重复注册
 * 3. log() 进行级别过滤后构造 LogEntry 并分发到所有 sink
 *
 * 线程安全说明：
 * - 当前未加锁，适用于单线程（主线程）日志写入场景
 * - 若未来需多线程支持，需在 m_sinks 操作和 log() 中加锁
 */
#include "LogDispatcher.h"

#include <QDateTime>

LogDispatcher::LogDispatcher(QObject *parent)
    : QObject(parent)
{
}

LogDispatcher *LogDispatcher::instance()
{
    // C++11 保证局部静态变量初始化的线程安全
    static LogDispatcher dispatcher;
    return &dispatcher;
}

void LogDispatcher::addSink(LogSink *sink)
{
    // 空指针检查和重复注册检查
    if (!sink || m_sinks.contains(sink))
        return;
    m_sinks.append(sink);
}

void LogDispatcher::removeSink(LogSink *sink)
{
    m_sinks.removeAll(sink);
}

void LogDispatcher::clearSinks()
{
    m_sinks.clear();
}

LogLevel LogDispatcher::minimumLevel() const
{
    return m_minimumLevel;
}

void LogDispatcher::setMinimumLevel(LogLevel level)
{
    m_minimumLevel = level;
}

void LogDispatcher::log(LogLevel level, const QString &message, const QString &context)
{
    // None 级别直接丢弃所有日志
    if (m_minimumLevel == LogLevel::None)
        return;
    // 级别过滤：低于阈值的日志不分发
    if (static_cast<int>(level) < static_cast<int>(m_minimumLevel))
        return;

    // 构造日志条目，自动填充时间戳
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    entry.level = level;
    entry.message = message;
    entry.context = context;

    // 拷贝 sink 列表快照，避免分发过程中列表被修改导致迭代器失效
    const auto sinks = m_sinks;
    for (LogSink *sink : sinks) {
        if (sink)
            sink->write(entry);
    }
}
