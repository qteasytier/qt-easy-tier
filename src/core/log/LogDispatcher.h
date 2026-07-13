/**
 * @file LogDispatcher.h
 * @brief 日志分发器
 *
 * 日志系统的核心中枢组件，负责：
 * - 接收来自各模块的日志消息
 * - 按最低日志级别过滤
 * - 分发给所有注册的 LogSink 订阅者
 *
 * 采用单例模式，全局唯一实例通过 instance() 访问。
 * 继承 QObject 以利用 Qt 的父子对象生命周期管理。
 */
#pragma once

#include "core/log/LogSink.h"

#include <QList>
#include <QObject>
#include <QString>

/**
 * @brief 日志分发器
 *
 * 管理日志输出槽集合和日志级别阈值。
 * 仅当日志项的级别 >= minimumLevel 时才分发给所有 sink。
 * minimumLevel 默认为 Warning，即默认不输出 Info 级别日志。
 */
class LogDispatcher : public QObject
{
    Q_OBJECT

public:
    explicit LogDispatcher(QObject *parent = nullptr);

    /**
     * @brief 获取全局单例实例
     * @return LogDispatcher 的静态单例指针
     */
    static LogDispatcher *instance();

    /**
     * @brief 注册日志输出槽
     * @param sink 要添加的 sink 指针，为 nullptr 或已存在时忽略
     */
    void addSink(LogSink *sink);

    /**
     * @brief 移除指定的日志输出槽
     * @param sink 要移除的 sink 指针
     */
    void removeSink(LogSink *sink);

    /**
     * @brief 移除所有已注册的日志输出槽
     */
    void clearSinks();

    /**
     * @brief 获取当前日志级别阈值
     * @return 最低日志级别，低于此级别的日志将被丢弃
     */
    LogLevel minimumLevel() const;

    /**
     * @brief 设置日志级别阈值
     * @param level 新的最低日志级别
     */
    void setMinimumLevel(LogLevel level);

    /**
     * @brief 提交一条日志消息
     * @param level 日志级别
     * @param message 日志正文
     * @param context 日志来源上下文（如模块名），默认为空
     *
     * 自动填充时间戳等元数据后分发给所有注册的 sink。
     * 若 level 低于 minimumLevel 或 minimumLevel 为 None，则直接丢弃。
     */
    void log(LogLevel level, const QString &message, const QString &context = {});

private:
    QList<LogSink *> m_sinks;                  ///< 注册的日志输出槽列表
    LogLevel m_minimumLevel = LogLevel::Warning; ///< 日志级别阈值，默认 Warning
};
