/**
 * @file TrayMessageDispatcher.cpp
 * @brief 系统托盘消息分发器实现
 */
#include "TrayMessageDispatcher.h"

TrayMessageDispatcher *TrayMessageDispatcher::instance()
{
    // C++11 保证局部静态变量初始化线程安全。
    static TrayMessageDispatcher dispatcher;
    return &dispatcher;
}

void TrayMessageDispatcher::addSink(TrayMessageSink *sink)
{
    if (!sink || m_sinks.contains(sink))
        return;
    m_sinks.append(sink);
}

void TrayMessageDispatcher::removeSink(TrayMessageSink *sink)
{
    m_sinks.removeAll(sink);
}

void TrayMessageDispatcher::clearSinks()
{
    m_sinks.clear();
}

void TrayMessageDispatcher::showMessage(const TrayMessage &message)
{
    // 使用快照遍历，避免 sink 在回调期间修改列表导致迭代失效。
    const auto sinks = m_sinks;
    for (TrayMessageSink *sink : sinks) {
        if (sink)
            sink->showTrayMessage(message);
    }
}
