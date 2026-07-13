/**
 * @file LogSink.h
 * @brief 日志输出槽抽象接口
 *
 * 定义日志消费者的统一接口。所有需要接收日志事件的组件
 * （如数据库写入器、文件写入器、UI 日志列表等）均实现此接口。
 * 通过 LogDispatcher 的订阅机制分发日志事件到各个 sink。
 */
#pragma once

#include "core/log/LogTypes.h"

/**
 * @brief 日志输出槽抽象基类
 *
 * 纯虚接口类，子类需实现 write() 方法定义具体的日志消费行为。
 * 虚析构确保通过基类指针安全释放派生类对象。
 */
class LogSink
{
public:
    virtual ~LogSink() = default;

    /**
     * @brief 写入一条日志记录
     * @param entry 待写入的日志条目（const 引用，不修改）
     *
     * 由 LogDispatcher 在所有订阅者上调用。实现需保证线程安全。
     */
    virtual void write(const LogEntry &entry) = 0;
};
